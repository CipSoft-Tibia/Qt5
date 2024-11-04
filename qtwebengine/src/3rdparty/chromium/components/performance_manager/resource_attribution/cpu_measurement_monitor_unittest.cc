// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/performance_manager/resource_attribution/cpu_measurement_monitor.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/process/kill.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_timeouts.h"
#include "base/test/test_waitable_event.h"
#include "base/time/time.h"
#include "components/performance_manager/embedder/graph_features.h"
#include "components/performance_manager/graph/frame_node_impl.h"
#include "components/performance_manager/graph/page_node_impl.h"
#include "components/performance_manager/graph/process_node_impl.h"
#include "components/performance_manager/graph/worker_node_impl.h"
#include "components/performance_manager/public/graph/process_node.h"
#include "components/performance_manager/public/performance_manager.h"
#include "components/performance_manager/public/resource_attribution/frame_context_registry.h"
#include "components/performance_manager/public/resource_attribution/resource_contexts.h"
#include "components/performance_manager/test_support/graph_test_harness.h"
#include "components/performance_manager/test_support/mock_graphs.h"
#include "components/performance_manager/test_support/performance_manager_test_harness.h"
#include "components/performance_manager/test_support/test_harness_helper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/process_type.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/navigation_simulator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace performance_manager::resource_attribution {

namespace {

using ::testing::_;
using ::testing::DoubleEq;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Optional;

constexpr base::TimeDelta kTimeBetweenMeasurements = base::Minutes(5);

// A struct holding node pointers for a renderer process containing a single
// page with a single main frame.
struct SinglePageRendererNodes {
  TestNodeWrapper<ProcessNodeImpl> process_node;
  TestNodeWrapper<PageNodeImpl> page_node;
  TestNodeWrapper<FrameNodeImpl> frame_node;
};

// State of a simulated process for CPU measurements.
class SimulatedCPUMeasurementDelegate final
    : public CPUMeasurementMonitor::CPUMeasurementDelegate {
 public:
  struct CPUUsagePeriod {
    base::TimeTicks start_time;
    base::TimeTicks end_time;
    double cpu_usage;
  };

  explicit SimulatedCPUMeasurementDelegate(
      base::OnceClosure unregister_callback)
      : unregister_callback_(std::move(unregister_callback)) {}

  ~SimulatedCPUMeasurementDelegate() final {
    std::move(unregister_callback_).Run();
  }

  // Returns the simulated CPU usage of the process by summing
  // `cpu_usage_periods`.
  base::TimeDelta GetCumulativeCPUUsage() final;

  // List of periods of varying CPU usage.
  std::vector<CPUUsagePeriod> cpu_usage_periods;

  // If not nullopt, GetCumulativeCPUUsage() will ignore `cpu_usage_periods` and
  // return this value to simulate an error.
  absl::optional<base::TimeDelta> usage_error;

 private:
  base::OnceClosure unregister_callback_;
};

base::TimeDelta SimulatedCPUMeasurementDelegate::GetCumulativeCPUUsage() {
  if (usage_error.has_value()) {
    return usage_error.value();
  }
  base::TimeDelta cumulative_usage;
  for (const auto& usage_period : cpu_usage_periods) {
    CHECK(!usage_period.start_time.is_null());
    // The last interval in the list will have no end time.
    const base::TimeTicks end_time = usage_period.end_time.is_null()
                                         ? base::TimeTicks::Now()
                                         : usage_period.end_time;
    CHECK(end_time >= usage_period.start_time);
    cumulative_usage +=
        (end_time - usage_period.start_time) * usage_period.cpu_usage;
  }
  return cumulative_usage;
}

// Helpers to lookup measurement results from TestNodeWrapper's.

absl::optional<double> GetMeasurementResult(
    const CPUMeasurementMonitor::CPUUsageMap& cpu_usage_map,
    const ResourceContext& context) {
  const auto it = cpu_usage_map.find(context);
  if (it == cpu_usage_map.end()) {
    return absl::nullopt;
  }
  return it->second;
}

void RunOnPMSequence(base::OnceClosure closure) {
  base::RunLoop run_loop;
  PerformanceManager::CallOnGraph(
      FROM_HERE, base::BindLambdaForTesting([&run_loop, &closure] {
        std::move(closure).Run();
        run_loop.Quit();
      }));
  run_loop.Run();
}

void RunOnPMSequence(base::OnceCallback<void(Graph*)> callback) {
  base::RunLoop run_loop;
  PerformanceManager::CallOnGraph(
      FROM_HERE,
      base::BindLambdaForTesting([&run_loop, &callback](Graph* graph) {
        std::move(callback).Run(graph);
        run_loop.Quit();
      }));
  run_loop.Run();
}

}  // namespace

class CPUMeasurementMonitorTest : public GraphTestHarness {
 protected:
  using Super = GraphTestHarness;

  void SetUp() override {
    Super::SetUp();

    mock_graph_ =
        std::make_unique<MockMultiplePagesAndWorkersWithMultipleProcessesGraph>(
            graph());
    mock_utility_process_ =
        CreateNode<ProcessNodeImpl>(content::PROCESS_TYPE_UTILITY);
    mock_utility_process_->SetProcess(base::Process::Current(),
                                      /*launch_time=*/base::TimeTicks::Now());

    cpu_monitor_.SetCPUMeasurementDelegateFactoryForTesting(base::BindRepeating(
        &CPUMeasurementMonitorTest::CPUMeasurementDelegateFactory,
        base::Unretained(this)));
  }

  // Creates a renderer process containing a single page and frame, for simple
  // tracking of CPU usage.
  SinglePageRendererNodes CreateSimpleCPUTrackingRenderer() {
    // CreateNode's default arguments create a renderer process node.
    auto process_node = CreateNode<ProcessNodeImpl>();
    auto page_node = CreateNode<PageNodeImpl>();
    auto frame_node =
        CreateFrameNodeAutoId(process_node.get(), page_node.get());

    // By default simulate 100% CPU usage in the renderer. To override this call
    // SetProcessCPUUsage again before advancing the clock.
    SetProcessCPUUsage(process_node.get(), 1.0);

    return SinglePageRendererNodes{
        .process_node = std::move(process_node),
        .page_node = std::move(page_node),
        .frame_node = std::move(frame_node),
    };
  }

  void SetProcessId(ProcessNodeImpl* process_node) {
    // Assigns the current process object to the node, including its pid.
    process_node->SetProcess(base::Process::Current(), base::TimeTicks::Now());
  }

  void SetProcessExited(ProcessNodeImpl* process_node) {
    process_node->SetProcessExitStatus(0);
    // After a process exits, GetCumulativeCPUUsage() starts returning an error.
    SetProcessCPUUsageError(process_node, base::TimeDelta());
  }

  void SetProcessCPUUsage(const ProcessNodeImpl* process_node, double usage) {
    SimulatedCPUMeasurementDelegate::CPUUsagePeriod usage_period{
        .start_time = base::TimeTicks::Now(),
        .cpu_usage = usage,
    };
    auto& delegate = GetOrCreateCPUMeasurementDelegate(process_node);
    if (!delegate.cpu_usage_periods.empty()) {
      delegate.cpu_usage_periods.back().end_time = usage_period.start_time;
    }
    delegate.cpu_usage_periods.push_back(std::move(usage_period));
  }

  void SetProcessCPUUsageError(const ProcessNodeImpl* process_node,
                               base::TimeDelta usage_error) {
    GetOrCreateCPUMeasurementDelegate(process_node).usage_error = usage_error;
  }

  std::unique_ptr<SimulatedCPUMeasurementDelegate>
  CreateSimulatedCPUMeasurementDelegate(const ProcessNode* process_node) {
    CHECK(!base::Contains(pending_cpu_delegates_, process_node));
    CHECK(!base::Contains(simulated_cpu_delegates_, process_node));
    auto delegate = std::make_unique<SimulatedCPUMeasurementDelegate>(
        // Clear pointers to this delegate when it's deleted.
        base::BindLambdaForTesting([this, process_node] {
          this->simulated_cpu_delegates_.erase(process_node);
        }));
    simulated_cpu_delegates_.emplace(process_node, delegate.get());
    return delegate;
  }

  std::unique_ptr<CPUMeasurementMonitor::CPUMeasurementDelegate>
  CPUMeasurementDelegateFactory(const ProcessNode* process_node) {
    auto it = pending_cpu_delegates_.find(process_node);
    if (it != pending_cpu_delegates_.end()) {
      auto delegate = std::move(it->second);
      pending_cpu_delegates_.erase(it);
      return delegate;
    }
    return CreateSimulatedCPUMeasurementDelegate(process_node);
  }

  SimulatedCPUMeasurementDelegate& GetOrCreateCPUMeasurementDelegate(
      const ProcessNodeImpl* process_node) {
    auto it = simulated_cpu_delegates_.find(process_node);
    if (it != simulated_cpu_delegates_.end()) {
      return *(it->second);
    }
    CHECK(!base::Contains(pending_cpu_delegates_, process_node));
    auto new_delegate = CreateSimulatedCPUMeasurementDelegate(process_node);
    auto* delegate_ptr = new_delegate.get();
    CHECK_EQ(simulated_cpu_delegates_.at(process_node), delegate_ptr);
    pending_cpu_delegates_.emplace(process_node, std::move(new_delegate));
    return *delegate_ptr;
  }

  CPUMeasurementMonitor cpu_monitor_;

  TestNodeWrapper<ProcessNodeImpl> mock_utility_process_;

  // Map of ProcessNode to CPUMeasurementDelegate that simulates that process.
  // The delegates are owned by `cpu_monitor_` or `pending_cpu_delegates_`.
  std::map<const ProcessNode*, SimulatedCPUMeasurementDelegate*>
      simulated_cpu_delegates_;

  // CPUMeasurementDelegates that have been created but not passed to
  // `cpu_monitor_` yet.
  std::map<const ProcessNode*, std::unique_ptr<SimulatedCPUMeasurementDelegate>>
      pending_cpu_delegates_;

  std::unique_ptr<MockMultiplePagesAndWorkersWithMultipleProcessesGraph>
      mock_graph_;
};

TEST_F(CPUMeasurementMonitorTest, CPUMeasurement) {
  // Create several renderer processes to measure. Put one page and one frame in
  // each renderer, so CPU measurements for the renderer are all assigned to
  // that frame for easy validation.

  // Renderer in existence before StartMonitoring().
  const SinglePageRendererNodes renderer1 = CreateSimpleCPUTrackingRenderer();
  SetProcessId(renderer1.process_node.get());

  // Renderer starts and exits before StartMonitoring().
  const SinglePageRendererNodes early_exit_renderer =
      CreateSimpleCPUTrackingRenderer();
  SetProcessId(early_exit_renderer.process_node.get());
  SetProcessExited(early_exit_renderer.process_node.get());

  // Renderer creation racing with StartMonitoring(). Its pid will not be
  // available until after monitoring starts .
  const SinglePageRendererNodes renderer2 = CreateSimpleCPUTrackingRenderer();
  ASSERT_EQ(renderer2.process_node->process_id(), base::kNullProcessId);

  // `renderer1` begins measurement as soon as StartMonitoring is called.
  // `renderer2` begins measurement when its pid is available.
  cpu_monitor_.StartMonitoring(graph());

  task_env().FastForwardBy(kTimeBetweenMeasurements / 2);
  SetProcessId(renderer2.process_node.get());

  // Renderer created halfway through the measurement interval.
  const SinglePageRendererNodes renderer3 = CreateSimpleCPUTrackingRenderer();
  SetProcessId(renderer3.process_node.get());

  // Renderer creation racing with UpdateCPUMeasurements(). `renderer4`'s pid
  // will become available on the same tick the measurement is taken,
  // `renderer5`'s pid will become available after the measurement.
  const SinglePageRendererNodes renderer4 = CreateSimpleCPUTrackingRenderer();
  const SinglePageRendererNodes renderer5 = CreateSimpleCPUTrackingRenderer();

  // Finish next measurement interval.
  task_env().FastForwardBy(kTimeBetweenMeasurements / 2);
  SetProcessId(renderer4.process_node.get());

  // `renderer1` existed for the entire measurement period.
  // `renderer2` existed for all of it, but was only measured for the last half,
  // after its pid became available.
  // `renderer3` only existed for the last half.
  // `renderer4` existed for the measurement but no time passed so it had 0% CPU
  // usage.
  // `renderer5` is not measured yet.
  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(
        GetMeasurementResult(
            measurements, early_exit_renderer.frame_node->resource_context()),
        Eq(absl::nullopt));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer1.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer2.frame_node->resource_context()),
                Optional(DoubleEq(0.5)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer3.frame_node->resource_context()),
                Optional(DoubleEq(0.5)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer4.frame_node->resource_context()),
                Optional(DoubleEq(0.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer5.frame_node->resource_context()),
                Eq(absl::nullopt));
  }

  SetProcessId(renderer5.process_node.get());

  // Finish next measurement interval.
  task_env().FastForwardBy(kTimeBetweenMeasurements);

  // All nodes existed for entire measurement interval.
  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer1.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer2.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer3.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer4.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer5.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
  }

  // `renderer1` drops to 50% CPU usage for the next period.
  // `renderer2` stays at 100% for the first half, 50% for the last half
  // (average 75%).
  // `renderer3` drops to 0% for a time, returns to 100% for half that time,
  // then drops to 0% again (average 25%).
  // `renderer4` drops to 0% at the end of the period. It should still show 100%
  // since no time passes before measuring.
  SetProcessCPUUsage(renderer1.process_node.get(), 0.5);
  SetProcessCPUUsage(renderer3.process_node.get(), 0.0);
  task_env().FastForwardBy(kTimeBetweenMeasurements / 2);
  SetProcessCPUUsage(renderer2.process_node.get(), 0.5);
  SetProcessCPUUsage(renderer3.process_node.get(), 1.0);
  task_env().FastForwardBy(kTimeBetweenMeasurements / 4);
  SetProcessCPUUsage(renderer3.process_node.get(), 0);

  // Finish next measurement interval.
  task_env().FastForwardBy(kTimeBetweenMeasurements / 4);
  SetProcessCPUUsage(renderer4.process_node.get(), 0);

  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer1.frame_node->resource_context()),
                Optional(DoubleEq(0.5)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer2.frame_node->resource_context()),
                Optional(DoubleEq(0.75)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer3.frame_node->resource_context()),
                Optional(DoubleEq(0.25)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer4.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer5.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
  }

  // `renderer1` exits at the beginning of the next measurement interval.
  // `renderer2` exits halfway through.
  // `renderer3` and `renderer4` are still using 0% CPU.
  // `renderer5` exits at the end of the interval.
  SetProcessExited(renderer1.process_node.get());
  task_env().FastForwardBy(kTimeBetweenMeasurements / 2);
  SetProcessExited(renderer2.process_node.get());

  // Finish next measurement interval.
  task_env().FastForwardBy(kTimeBetweenMeasurements / 2);
  SetProcessExited(renderer5.process_node.get());

  {
    // TODO(crbug.com/1410503): Capture the final CPU usage correctly, and test
    // that the renderers that have exited return their CPU usage for the time
    // they were alive and 0% for the rest of the measurement interval.
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer1.frame_node->resource_context()),
                Eq(absl::nullopt));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer2.frame_node->resource_context()),
                Eq(absl::nullopt));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer3.frame_node->resource_context()),
                Optional(DoubleEq(0.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer4.frame_node->resource_context()),
                Optional(DoubleEq(0.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer5.frame_node->resource_context()),
                Eq(absl::nullopt));
  }

  // `renderer3` exits just before the StopMonitoring call and `renderer4`
  // exits just after. This should not cause any assertion failures.
  SetProcessExited(renderer3.process_node.get());
  cpu_monitor_.StopMonitoring(graph());
  SetProcessExited(renderer4.process_node.get());
}

TEST_F(CPUMeasurementMonitorTest, CPUDistribution) {
  // Track CPU usage of the mock utility process to make sure that measuring it
  // doesn't crash. Currently only measurements of renderer processes are
  // stored anywhere, so there are no other expectations to verify.
  SetProcessCPUUsage(mock_utility_process_.get(), 0.7);

  SetProcessCPUUsage(mock_graph_->process.get(), 0.6);
  SetProcessCPUUsage(mock_graph_->other_process.get(), 0.5);

  cpu_monitor_.StartMonitoring(graph());

  {
    // No measurements if no time has passed.
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(measurements, IsEmpty());
  }

  task_env().FastForwardBy(kTimeBetweenMeasurements);

  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();

    // `process` splits its 60% CPU usage evenly between `frame`, `other_frame`
    // and `worker`. `other_process` splits its 50% CPU usage evenly between
    // `child_frame` and `other_worker`. See the chart in
    // MockMultiplePagesAndWorkersWithMultipleProcessesGraph.
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     mock_graph_->frame->resource_context()),
                Optional(DoubleEq(0.2)));
    EXPECT_THAT(GetMeasurementResult(
                    measurements, mock_graph_->other_frame->resource_context()),
                Optional(DoubleEq(0.2)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     mock_graph_->worker->resource_context()),
                Optional(DoubleEq(0.2)));
    EXPECT_THAT(GetMeasurementResult(
                    measurements, mock_graph_->child_frame->resource_context()),
                Optional(DoubleEq(0.25)));
    EXPECT_THAT(
        GetMeasurementResult(measurements,
                             mock_graph_->other_worker->resource_context()),
        Optional(DoubleEq(0.25)));

    // `page` gets its CPU usage from the sum of `frame` and `worker`.
    // `other_page` gets the sum of `other_frame`, `child_frame` and
    // `other_worker`. See the chart in
    // MockMultiplePagesAndWorkersWithMultipleProcessesGraph.
    EXPECT_THAT(CPUMeasurementMonitor::EstimatePageCPUUsage(
                    mock_graph_->page.get(), measurements),
                DoubleEq(0.4));
    EXPECT_THAT(CPUMeasurementMonitor::EstimatePageCPUUsage(
                    mock_graph_->other_page.get(), measurements),
                DoubleEq(0.7));
  }

  // Modify the CPU usage of each process, ensure all frames and workers are
  // updated.
  SetProcessCPUUsage(mock_graph_->process.get(), 0.3);
  SetProcessCPUUsage(mock_graph_->other_process.get(), 0.8);
  task_env().FastForwardBy(kTimeBetweenMeasurements);

  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();

    // `process` splits its 30% CPU usage evenly between `frame`, `other_frame`
    // and `worker`. `other_process` splits its 80% CPU usage evenly between
    // `child_frame` and `other_worker`.
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     mock_graph_->frame->resource_context()),
                Optional(DoubleEq(0.1)));
    EXPECT_THAT(GetMeasurementResult(
                    measurements, mock_graph_->other_frame->resource_context()),
                Optional(DoubleEq(0.1)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     mock_graph_->worker->resource_context()),
                Optional(DoubleEq(0.1)));
    EXPECT_THAT(GetMeasurementResult(
                    measurements, mock_graph_->child_frame->resource_context()),
                Optional(DoubleEq(0.4)));
    EXPECT_THAT(
        GetMeasurementResult(measurements,
                             mock_graph_->other_worker->resource_context()),
        Optional(DoubleEq(0.4)));

    // `page` gets its CPU usage from the sum of `frame` and `worker`.
    // `other_page` gets the sum of `other_frame`, `child_frame` and
    // `other_worker`.
    EXPECT_THAT(CPUMeasurementMonitor::EstimatePageCPUUsage(
                    mock_graph_->page.get(), measurements),
                DoubleEq(0.2));
    EXPECT_THAT(CPUMeasurementMonitor::EstimatePageCPUUsage(
                    mock_graph_->other_page.get(), measurements),
                DoubleEq(0.9));
  }

  // Drop CPU usage of `other_process` to 0%. Only advance part of the normal
  // measurement interval, to be sure that the percentage usage doesn't depend
  // on the length of the interval.
  SetProcessCPUUsage(mock_graph_->other_process.get(), 0.0);
  task_env().FastForwardBy(kTimeBetweenMeasurements / 3);

  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();

    // `process` splits its 30% CPU usage evenly between `frame`, `other_frame`
    // and `worker`. `other_process` splits its 0% CPU usage evenly between
    // `child_frame` and `other_worker`.
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     mock_graph_->frame->resource_context()),
                Optional(DoubleEq(0.1)));
    EXPECT_THAT(GetMeasurementResult(
                    measurements, mock_graph_->other_frame->resource_context()),
                Optional(DoubleEq(0.1)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     mock_graph_->worker->resource_context()),
                Optional(DoubleEq(0.1)));
    EXPECT_THAT(GetMeasurementResult(
                    measurements, mock_graph_->child_frame->resource_context()),
                Optional(DoubleEq(0.0)));
    EXPECT_THAT(
        GetMeasurementResult(measurements,
                             mock_graph_->other_worker->resource_context()),
        Optional(DoubleEq(0.0)));

    // `page` gets its CPU usage from the sum of `frame` and `worker`.
    // `other_page` gets the sum of `other_frame`, `child_frame` and
    // `other_worker`.
    EXPECT_THAT(CPUMeasurementMonitor::EstimatePageCPUUsage(
                    mock_graph_->page.get(), measurements),
                DoubleEq(0.2));
    EXPECT_THAT(CPUMeasurementMonitor::EstimatePageCPUUsage(
                    mock_graph_->other_page.get(), measurements),
                DoubleEq(0.1));
  }

  cpu_monitor_.StopMonitoring(graph());
}

TEST_F(CPUMeasurementMonitorTest, CPUMeasurementError) {
  const SinglePageRendererNodes renderer1 = CreateSimpleCPUTrackingRenderer();
  SetProcessId(renderer1.process_node.get());
  const SinglePageRendererNodes renderer2 = CreateSimpleCPUTrackingRenderer();
  SetProcessId(renderer2.process_node.get());
  const SinglePageRendererNodes renderer3 = CreateSimpleCPUTrackingRenderer();
  SetProcessId(renderer3.process_node.get());

  cpu_monitor_.StartMonitoring(graph());

  task_env().FastForwardBy(kTimeBetweenMeasurements);

  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer1.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer2.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer3.frame_node->resource_context()),
                Optional(DoubleEq(1.0)));
  }

  SetProcessCPUUsage(renderer1.process_node.get(), 0.5);
  SetProcessCPUUsage(renderer2.process_node.get(), 0.5);
  SetProcessCPUUsage(renderer3.process_node.get(), 0.5);

  // Most platforms returns a zero TimeDelta on error.
  SetProcessCPUUsageError(renderer1.process_node.get(), base::TimeDelta());
  // Linux returns a negative TimeDelta on error.
  SetProcessCPUUsageError(renderer2.process_node.get(), base::TimeDelta::Min());

  task_env().FastForwardBy(kTimeBetweenMeasurements);

  {
    auto measurements = cpu_monitor_.UpdateCPUMeasurements();
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer1.frame_node->resource_context()),
                Eq(absl::nullopt));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer2.frame_node->resource_context()),
                Eq(absl::nullopt));
    EXPECT_THAT(GetMeasurementResult(measurements,
                                     renderer3.frame_node->resource_context()),
                Optional(DoubleEq(0.5)));
  }

  cpu_monitor_.StopMonitoring(graph());
}

class CPUMeasurementMonitorTimingTest : public PerformanceManagerTestHarness {
 protected:
  using Super = PerformanceManagerTestHarness;

  void SetUp() override {
    GetGraphFeatures().EnableResourceAttributionRegistries();
    Super::SetUp();
    RunOnPMSequence(base::BindLambdaForTesting([&](Graph* graph) {
      cpu_monitor_ = std::make_unique<CPUMeasurementMonitor>();
      cpu_monitor_->StartMonitoring(graph);
    }));
  }

  void TearDown() override {
    RunOnPMSequence(base::BindLambdaForTesting([&](Graph* graph) {
      cpu_monitor_->StopMonitoring(graph);
      cpu_monitor_.reset();
    }));
    Super::TearDown();
  }

  // Ensure some time passes to measure.
  void LetTimePass() {
    base::TestWaitableEvent().TimedWait(TestTimeouts::tiny_timeout());
  }

  std::unique_ptr<CPUMeasurementMonitor> cpu_monitor_;
};

TEST_F(CPUMeasurementMonitorTimingTest, ProcessLifetime) {
  SetContents(CreateTestWebContents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://www.example.com/"));

  const FrameContext frame_context =
      FrameContextRegistry::ContextForRenderFrameHost(main_rfh()).value();
  base::WeakPtr<ProcessNode> process_node =
      PerformanceManager::GetProcessNodeForRenderProcessHost(process());

  // Since process() returns a MockRenderProcessHost, ProcessNode is created
  // but has no pid. (Equivalent to the time between OnProcessNodeAdded and
  // OnProcessLifetimeChange.)
  LetTimePass();
  RunOnPMSequence(base::BindLambdaForTesting([&] {
    ASSERT_TRUE(process_node);
    EXPECT_EQ(process_node->GetProcessId(), base::kNullProcessId);

    // Process can't be measured yet.
    EXPECT_THAT(GetMeasurementResult(cpu_monitor_->UpdateCPUMeasurements(),
                                     frame_context),
                Eq(absl::nullopt));
  }));

  // Assign a real process to the ProcessNode. (Will call
  // OnProcessLifetimeChange.)
  LetTimePass();
  RunOnPMSequence(base::BindLambdaForTesting([&] {
    ASSERT_TRUE(process_node);
    ProcessNodeImpl::FromNode(process_node.get())
        ->SetProcess(base::Process::Current(), base::TimeTicks::Now());
    EXPECT_NE(process_node->GetProcessId(), base::kNullProcessId);

    // Process can be measured now.
    EXPECT_THAT(GetMeasurementResult(cpu_monitor_->UpdateCPUMeasurements(),
                                     frame_context),
                Optional(_));
  }));

  // Simulate that the process died.
  LetTimePass();
  process()->SimulateRenderProcessExit(
      base::TERMINATION_STATUS_NORMAL_TERMINATION, 0);
  RunOnPMSequence(base::BindLambdaForTesting([&](Graph* graph) {
    // Process is no longer running, so can't be measured.
    // TODO(crbug.com/1410503): Capture the final CPU usage correctly.
    ASSERT_TRUE(process_node);
    EXPECT_FALSE(process_node->GetProcess().IsValid());
    // Depending on the order that observers fire, `frame_node` may or may not
    // have been deleted already. If it's gone just check that the measurement
    // doesn't crash.
    const FrameNode* frame_node =
        FrameContextRegistry::GetFromGraph(graph)->GetFrameNodeForContext(
            frame_context);
    const auto measurements = cpu_monitor_->UpdateCPUMeasurements();
    if (frame_node) {
      EXPECT_THAT(GetMeasurementResult(measurements, frame_context),
                  Eq(absl::nullopt));
    }
  }));
}

}  // namespace performance_manager::resource_attribution
