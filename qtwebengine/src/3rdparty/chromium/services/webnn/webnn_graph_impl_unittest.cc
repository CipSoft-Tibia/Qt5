// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/webnn/webnn_graph_impl.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "components/ml/webnn/graph_validation_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/webnn/public/mojom/webnn_graph.mojom.h"
#include "services/webnn/public/mojom/webnn_service.mojom.h"
#include "services/webnn/webnn_context_impl.h"
#include "services/webnn/webnn_context_provider_impl.h"
#include "services/webnn/webnn_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace webnn {

namespace {

// A fake WebNNGraph Mojo interface implementation that binds a pipe for
// computing graph message.
class FakeWebNNGraphImpl final : public WebNNGraphImpl {
 public:
  explicit FakeWebNNGraphImpl(
      std::unique_ptr<ComputeResourceInfo> compute_resource_info)
      : WebNNGraphImpl(std::move(compute_resource_info)) {}
  ~FakeWebNNGraphImpl() override = default;

  static void CreateAndBuild(
      const mojom::GraphInfoPtr& graph_info,
      mojom::WebNNContext::CreateGraphCallback callback) {
    mojo::PendingRemote<mojom::WebNNGraph> blink_remote;
    // The receiver bound to FakeWebNNGraphImpl.
    mojo::MakeSelfOwnedReceiver<mojom::WebNNGraph>(
        std::make_unique<FakeWebNNGraphImpl>(
            std::make_unique<ComputeResourceInfo>(graph_info)),
        blink_remote.InitWithNewPipeAndPassReceiver());
    std::move(callback).Run(std::move(blink_remote));
  }

 private:
  // Return the `kOk` result for testing the validation of inputs and outputs in
  // `WebNNGraphImpl::Compute()` function.
  void ComputeImpl(base::flat_map<std::string, mojo_base::BigBuffer> inputs,
                   mojom::WebNNGraph::ComputeCallback callback) override {
    std::move(callback).Run(mojom::ComputeResult::kOk, absl::nullopt);
  }
};

// A fake WebNNContext Mojo interface implementation that binds a pipe for
// creating graph message.
class FakeWebNNContextImpl final : public WebNNContextImpl {
 public:
  FakeWebNNContextImpl(mojo::PendingReceiver<mojom::WebNNContext> receiver,
                       WebNNContextProviderImpl* context_provider)
      : WebNNContextImpl(std::move(receiver), context_provider) {}
  ~FakeWebNNContextImpl() override = default;

 private:
  void CreateGraphImpl(
      mojom::GraphInfoPtr graph_info,
      mojom::WebNNContext::CreateGraphCallback callback) override {
    FakeWebNNGraphImpl::CreateAndBuild(std::move(graph_info),
                                       std::move(callback));
  }
};

// Helper class to create the FakeWebNNContext that is intended to test
// the graph validation steps and computation resources.
class FakeWebNNBackend : public WebNNContextProviderImpl::BackendForTesting {
 public:
  void CreateWebNNContext(
      std::vector<std::unique_ptr<WebNNContextImpl>>& context_impls,
      WebNNContextProviderImpl* context_provider_impl,
      mojom::CreateContextOptionsPtr options,
      mojom::WebNNContextProvider::CreateWebNNContextCallback callback)
      override {
    mojo::PendingRemote<mojom::WebNNContext> blink_remote;
    // The receiver bound to FakeWebNNContext.
    context_impls.push_back(std::make_unique<FakeWebNNContextImpl>(
        blink_remote.InitWithNewPipeAndPassReceiver(), context_provider_impl));
    std::move(callback).Run(mojom::CreateContextResult::kOk,
                            std::move(blink_remote));
  }
};

bool ValidateInputsForComputing(
    mojom::GraphInfoPtr graph_info,
    base::flat_map<std::string, mojo_base::BigBuffer> inputs) {
  // Creates WebNN Context mojo interface with the provider.
  mojo::Remote<mojom::WebNNContextProvider> provider_remote;
  WebNNContextProviderImpl::Create(
      provider_remote.BindNewPipeAndPassReceiver());
  base::RunLoop run_loop_create_context;
  bool is_callback_called = false;
  mojo::Remote<mojom::WebNNContext> webnn_context;
  auto options = mojom::CreateContextOptions::New();
  provider_remote->CreateWebNNContext(
      std::move(options),
      base::BindLambdaForTesting(
          [&](mojom::CreateContextResult result,
              mojo::PendingRemote<mojom::WebNNContext> remote) {
            EXPECT_EQ(result, mojom::CreateContextResult::kOk);
            webnn_context.Bind(std::move(remote));
            is_callback_called = true;
            run_loop_create_context.Quit();
          }));
  run_loop_create_context.Run();
  EXPECT_TRUE(is_callback_called);

  // Creates WebNN Graph mojo interface with the graph information which is
  // validated before compiling.
  mojo::Remote<mojom::WebNNGraph> webnn_graph;
  base::RunLoop run_loop_create_graph;
  is_callback_called = false;
  webnn_context->CreateGraph(
      std::move(graph_info),
      base::BindLambdaForTesting(
          [&](mojo::PendingRemote<mojom::WebNNGraph> remote) {
            webnn_graph.Bind(std::move(remote));
            is_callback_called = true;
            run_loop_create_graph.Quit();
          }));
  run_loop_create_graph.Run();
  EXPECT_TRUE(is_callback_called);

  // Validate the inputs in the `Compute` function.
  base::RunLoop run_loop_compute;
  is_callback_called = false;
  bool valid = false;
  webnn_graph->Compute(
      std::move(inputs),
      base::BindLambdaForTesting(
          [&](mojom::ComputeResult result,
              absl::optional<base::flat_map<std::string, mojo_base::BigBuffer>>
                  ouputs) {
            valid =
                result == mojom::ComputeResult::kInvalidInputs ? false : true;
            is_callback_called = true;
            run_loop_compute.Quit();
          }));
  run_loop_compute.Run();
  EXPECT_TRUE(is_callback_called);
  return valid;
}

}  // namespace

class WebNNGraphImplTest : public testing::Test {
 public:
  WebNNGraphImplTest(const WebNNGraphImplTest&) = delete;
  WebNNGraphImplTest& operator=(const WebNNGraphImplTest&) = delete;

  void SetUp() override {
    WebNNContextProviderImpl::SetBackendForTesting(&backend_for_testing);
  }
  void TearDown() override {
    WebNNContextProviderImpl::SetBackendForTesting(nullptr);
  }

 protected:
  WebNNGraphImplTest() = default;
  ~WebNNGraphImplTest() override = default;

 private:
  FakeWebNNBackend backend_for_testing;
  base::test::TaskEnvironment task_environment_;
};

struct OperandInfo {
  mojom::Operand::DataType type;
  std::vector<uint32_t> dimensions;
};

struct ClampTester {
  OperandInfo input;
  struct ClampAttributes {
    float min_value;
    float max_value;
  };
  ClampAttributes attributes;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t input_operand_id =
        builder.BuildInput("input", input.dimensions, input.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    mojom::ClampAttributesPtr mojo_attributes = mojom::ClampAttributes::New();
    mojo_attributes->min_value = attributes.min_value;
    mojo_attributes->max_value = attributes.max_value;
    builder.BuildOperator(
        mojom::Operator::Kind::kClamp, {input_operand_id}, {output_operand_id},
        mojom::OperatorAttributes::NewClamp(std::move(mojo_attributes)));
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, ClampTest) {
  {
    // Test clamp operator with both the minimum and maximum values.
    ClampTester{.input = {.type = mojom::Operand::DataType::kInt8,
                          .dimensions = {3, 4}},
                .attributes = {.min_value = 0.0, .max_value = 6.0},
                .output = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {3, 4}},
                .expected = true}
        .Test();
  }
  {
    // Test clamp operator with the min value is infinite.
    ClampTester{.input = {.type = mojom::Operand::DataType::kInt32,
                          .dimensions = {2, 3, 4}},
                .attributes = {.min_value = static_cast<float>(-1.0 / 0.0),
                               .max_value = 3.0},
                .output = {.type = mojom::Operand::DataType::kInt32,
                           .dimensions = {2, 3, 4}},
                .expected = true}
        .Test();
  }
  {
    // Test clamp operator with the max value is infinite.
    ClampTester{.input = {.type = mojom::Operand::DataType::kInt32,
                          .dimensions = {2, 3, 4}},
                .attributes = {.min_value = 0.0,
                               .max_value = static_cast<float>(1.0 / 0.0)},
                .output = {.type = mojom::Operand::DataType::kInt32,
                           .dimensions = {2, 3, 4}},
                .expected = true}
        .Test();
  }
  {
    // Test the invalid graph when max value = 0 and min value = 0.
    ClampTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                          .dimensions = {1, 2, 2, 7}},
                .output = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 2, 2, 7}},
                .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the max value is less than the min value.
    ClampTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                          .dimensions = {4, 2}},
                .attributes = {.min_value = 7.0, .max_value = 3.0},
                .output = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {4, 2}},
                .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the min value is NAN.
    ClampTester{.input = {.type = mojom::Operand::DataType::kInt32,
                          .dimensions = {2, 3, 4}},
                .attributes = {.min_value = NAN, .max_value = 3.0},
                .output = {.type = mojom::Operand::DataType::kInt32,
                           .dimensions = {2, 3, 4}},
                .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the max value is NAN.
    ClampTester{.input = {.type = mojom::Operand::DataType::kInt32,
                          .dimensions = {2, 3, 4}},
                .attributes = {.min_value = 0.0, .max_value = NAN},
                .output = {.type = mojom::Operand::DataType::kInt32,
                           .dimensions = {2, 3, 4}},
                .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    ClampTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                          .dimensions = {4, 2}},
                .output = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {2}},
                .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    ClampTester{
        .input = {.type = mojom::Operand::DataType::kFloat32,
                  .dimensions = {2}},
        .output = {.type = mojom::Operand::DataType::kInt32, .dimensions = {2}},
        .expected = false}
        .Test();
  }
}

struct Conv2dTester {
  OperandInfo input;
  OperandInfo filter;
  struct Conv2dAttributes {
    std::vector<uint32_t> padding = {0, 0, 0, 0};
    std::vector<uint32_t> strides = {1, 1};
    std::vector<uint32_t> dilations = {1, 1};
    uint32_t groups = 1;
    mojom::InputOperandLayout input_layout =
        mojom::InputOperandLayout::kChannelsFirst;
    absl::optional<OperandInfo> bias;
    absl::optional<ClampTester::ClampAttributes> activation;
  };
  Conv2dAttributes attributes;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t input_operand_id =
        builder.BuildInput("input", input.dimensions, input.type);
    uint64_t filter_operand_id =
        builder.BuildInput("filter", filter.dimensions, filter.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    mojom::Conv2dAttributesPtr mojo_attributes = mojom::Conv2dAttributes::New();
    mojo_attributes->padding = mojom::Padding2d::New(
        mojom::Size2d::New(attributes.padding[0],
                           attributes.padding[2]) /* beginning padding*/,
        mojom::Size2d::New(attributes.padding[1],
                           attributes.padding[3]) /* ending padding*/);
    mojo_attributes->strides =
        mojom::Size2d::New(attributes.strides[0], attributes.strides[1]);
    mojo_attributes->dilations =
        mojom::Size2d::New(attributes.dilations[0], attributes.dilations[1]);
    mojo_attributes->groups = attributes.groups;
    mojo_attributes->input_layout = attributes.input_layout;
    if (attributes.bias) {
      mojo_attributes->bias_operand_id = builder.BuildInput(
          "bias", attributes.bias->dimensions, attributes.bias->type);
    }
    if (attributes.activation) {
      auto activation = mojom::Operator::New();
      activation->kind = mojom::Operator::Kind::kClamp;
      mojom::ClampAttributesPtr clamp_attributes =
          mojom::ClampAttributes::New();
      clamp_attributes->min_value = attributes.activation->min_value;
      clamp_attributes->max_value = attributes.activation->max_value;
      activation->attributes =
          mojom::OperatorAttributes::NewClamp(std::move(clamp_attributes));
      mojo_attributes->activation = std::move(activation);
    }
    builder.BuildOperator(
        mojom::Operator::Kind::kConv2d, {input_operand_id, filter_operand_id},
        {output_operand_id},
        mojom::OperatorAttributes::NewConv2d(std::move(mojo_attributes)));
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, Conv2dTest) {
  {
    // Test conv2d with default attributes.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = true}
        .Test();
  }
  {
    // Test conv2d for same upper or lower padding.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 1, 3, 3}},
                 .attributes = {.padding = {1, 1, 1, 1}},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 1, 5, 5}},
                 .expected = true}
        .Test();
  }
  {
    // Test conv2d with strides=2 and padding=1.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 1, 3, 3}},
                 .attributes = {.padding = {1, 1, 1, 1}, .strides = {2, 2}},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = true}
        .Test();
  }
  {
    // Test depthwise conv2d by setting groups to input channels.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 4, 2, 2}},
                 .filter = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {4, 1, 2, 2}},
                 .attributes = {.groups = 4},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 4, 1, 1}},
                 .expected = true}
        .Test();
  }
  {
    // Test conv2d with inputLayout="nchw" and filterLayout="oihw".
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 2, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 2, 3, 3}},
                 .attributes = {.input_layout =
                                    mojom::InputOperandLayout::kChannelsFirst},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = true}
        .Test();
  }
  {
    // Test conv2d with clamp activation.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .attributes = {.activation =
                                    ClampTester::ClampAttributes{
                                        .min_value = 1.0, .max_value = 6.0}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = true}
        .Test();
  }
  {
    // Test the invalid graph when the input is not a 4-D tensor.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the filter is not a 4-D tensor.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 3, 3}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the filter type doesn't match the input
    // type.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kInt32,
                            .dimensions = {1, 1, 3, 3}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the bias type doesn't match input type.
    Conv2dTester{
        .input = {.type = mojom::Operand::DataType::kFloat32,
                  .dimensions = {1, 1, 5, 5}},
        .filter = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {1, 1, 3, 3}},
        .attributes = {.bias =
                           OperandInfo{.type = mojom::Operand::DataType::kInt32,
                                       .dimensions = {1}}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {1, 1, 3, 3}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the bias shape is not equal to
    // [output_channels].
    Conv2dTester{
        .input = {.type = mojom::Operand::DataType::kFloat32,
                  .dimensions = {1, 1, 5, 5}},
        .filter = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {1, 1, 3, 3}},
        .attributes = {.bias =
                           OperandInfo{
                               .type = mojom::Operand::DataType::kFloat32,
                               .dimensions = {2}}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {1, 1, 3, 3}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the max value is less than the min value.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .attributes = {.activation =
                                    ClampTester::ClampAttributes{
                                        .min_value = 6.0, .max_value = 1.0}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 2, 1, 1}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    Conv2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 1, 5, 5}},
                 .filter = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 3, 3}},
                 .output = {.type = mojom::Operand::DataType::kInt32,
                            .dimensions = {1, 1, 3, 3}},
                 .expected = false}
        .Test();
  }
}

struct ElementWiseBinaryTester {
  mojom::Operator::Kind kind;
  OperandInfo lhs;
  OperandInfo rhs;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t lhs_operand_id =
        builder.BuildInput("lhs", lhs.dimensions, lhs.type);
    uint64_t rhs_operand_id =
        builder.BuildInput("rhs", rhs.dimensions, rhs.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    builder.BuildOperator(kind, {lhs_operand_id, rhs_operand_id},
                          {output_operand_id});
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, ElementWiseBinaryTest) {
  {
    // Testing building add with two input dimensions - {8, 1, 6, 1} and {7, 1,
    // 5}. Both the a and b dimensions have axes with length one that are
    // expanded to a larger size during the broadcast operation.
    // a_dimensions     (4d) 8 * 1 * 6 * 1
    // b_dimensions     (3d)     7 * 1 * 5
    // output_dimenions (4d) 8 * 7 * 6 * 5
    ElementWiseBinaryTester{
        .kind = mojom::Operator::Kind::kAdd,
        .lhs = {.type = mojom::Operand::DataType::kFloat32,
                .dimensions = {8, 1, 6, 1}},
        .rhs = {.type = mojom::Operand::DataType::kFloat32,
                .dimensions = {7, 1, 5}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {8, 7, 6, 5}},
        .expected = true}
        .Test();
  }
  {
    // Testing building add with two input dimensions - {4, 2, 1} and {4}.
    // a_dimensions     (3d) 4 * 2 * 1
    // b_dimensions     (1d)         4
    // output_dimenions (3d) 4 * 2 * 4
    ElementWiseBinaryTester{
        .kind = mojom::Operator::Kind::kSub,
        .lhs = {.type = mojom::Operand::DataType::kFloat32,
                .dimensions = {4, 2, 1}},
        .rhs = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {4}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {4, 2, 4}},
        .expected = true}
        .Test();
  }
  {
    // Test the invalid graph for the input shapes are not broadcastable.
    ElementWiseBinaryTester{
        .kind = mojom::Operator::Kind::kMul,
        .lhs = {.type = mojom::Operand::DataType::kFloat32,
                .dimensions = {4, 2}},
        .rhs = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {4}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {4, 2}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    ElementWiseBinaryTester{
        .kind = mojom::Operator::Kind::kDiv,
        .lhs = {.type = mojom::Operand::DataType::kFloat32,
                .dimensions = {4, 2}},
        .rhs = {.type = mojom::Operand::DataType::kFloat32,
                .dimensions = {4, 2}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for input types don't match.
    ElementWiseBinaryTester{
        .kind = mojom::Operator::Kind::kMax,
        .lhs = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2}},
        .rhs = {.type = mojom::Operand::DataType::kInt32, .dimensions = {2}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    ElementWiseBinaryTester{
        .kind = mojom::Operator::Kind::kMin,
        .lhs = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2}},
        .rhs = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2}},
        .output = {.type = mojom::Operand::DataType::kInt32, .dimensions = {2}},
        .expected = false}
        .Test();
  }
}

struct GemmTester {
  OperandInfo a;
  OperandInfo b;
  struct GemmAttributes {
    absl::optional<OperandInfo> c;
    float alpha = 1.0;
    float beta = 1.0;
    bool a_transpose = false;
    bool b_transpose = false;
  };
  GemmAttributes attributes;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t a_operand_id = builder.BuildInput("a", a.dimensions, a.type);
    uint64_t b_operand_id = builder.BuildInput("b", b.dimensions, b.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    mojom::GemmAttributesPtr mojo_attributes = mojom::GemmAttributes::New();
    if (attributes.c) {
      mojo_attributes->c_operand_id =
          builder.BuildInput("c", attributes.c->dimensions, attributes.c->type);
    }
    mojo_attributes->alpha = attributes.alpha;
    mojo_attributes->beta = attributes.beta;
    mojo_attributes->a_transpose = attributes.a_transpose;
    mojo_attributes->b_transpose = attributes.b_transpose;
    builder.BuildOperator(
        mojom::Operator::Kind::kGemm, {a_operand_id, b_operand_id},
        {output_operand_id},
        mojom::OperatorAttributes::NewGemm(std::move(mojo_attributes)));
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, GemmTest) {
  {
    // Test building gemm with default option.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {3, 4}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2, 4}},
        .expected = true}
        .Test();
  }
  {
    // Test building gemm with aTranspose = true.
    // Transposed a_dimensions would be {3, 2} and it's compatible with
    // b_dimensions {2, 4}.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 4}},
        .attributes = {.a_transpose = true},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {3, 4}},
        .expected = true}
        .Test();
  }
  {
    // Test building gemm with bTranspose = true.
    // Transposed b_dimensions would be {3, 4} and it's compatible with
    // a_dimensions {2, 3}.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {4, 3}},
        .attributes = {.b_transpose = true},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2, 4}},
        .expected = true}
        .Test();
  }
  {
    // Test building gemm with setting optional input C.
    // The output dimensions of a * b would be {2, 4} and c_dimensions {4} is
    // able to broadcast to {2, 4}.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {3, 4}},
        .attributes = {.c = OperandInfo{.type =
                                            mojom::Operand::DataType::kFloat32,
                                        .dimensions = {4}}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2, 4}},
        .expected = true}
        .Test();
  }
  {
    // Test building gemm with two matrices - {2, 3} and {2, 4} that can't be
    // multiplied together due to incompatible dimensions.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 4}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {3, 4}},
        .expected = false}
        .Test();
  }
  {
    // Test building gemm with aTranspose = true, bTranspose = true.
    // The output dimensions of a * b would be {2, 4} and c_dimension {2, 3} is
    // incompatible with {2, 4}.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {3, 4}},
        .attributes = {.c = OperandInfo{.type =
                                            mojom::Operand::DataType::kFloat32,
                                        .dimensions = {2, 3}}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2, 4}},
        .expected = false}
        .Test();
  }
  {
    // Test building gemm with aTranspose = true, bTranspose = true.
    // Set optional input C with type = int32 and it mismatches with input
    // type float32.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {3, 2}},
        .b = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {4, 3}},
        .attributes = {.c =
                           OperandInfo{.type = mojom::Operand::DataType::kInt32,
                                       .dimensions = {2, 4}},
                       .a_transpose = true,
                       .b_transpose = true},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2, 4}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kInt32, .dimensions = {3, 4}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {3, 4}},
        .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    GemmTester{
        .a = {.type = mojom::Operand::DataType::kFloat32, .dimensions = {2, 3}},
        .b = {.type = mojom::Operand::DataType::kInt32, .dimensions = {3, 4}},
        .output = {.type = mojom::Operand::DataType::kFloat32,
                   .dimensions = {2, 4}},
        .expected = false}
        .Test();
  }
}

struct Pool2dTester {
  OperandInfo input;
  struct Pool2dAttributes {
    std::vector<uint32_t> window_dimensions;
    std::vector<uint32_t> padding = {0, 0, 0, 0};
    std::vector<uint32_t> strides = {1, 1};
    std::vector<uint32_t> dilations = {1, 1};
    mojom::InputOperandLayout layout;
  };
  Pool2dAttributes attributes;
  OperandInfo output;
  bool expected;

  void Test() {
    Test(mojom::Operator::Kind::kAveragePool2d);
    Test(mojom::Operator::Kind::kMaxPool2d);
  }

  void Test(mojom::Operator::Kind kind) {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t input_operand_id =
        builder.BuildInput("input", input.dimensions, input.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    mojom::Pool2dAttributesPtr mojo_attributes = mojom::Pool2dAttributes::New();
    auto& window_dimensions = attributes.window_dimensions;
    CHECK_EQ(window_dimensions.size(), 2u);
    mojo_attributes->window_dimensions =
        mojom::Size2d::New(window_dimensions[0], window_dimensions[1]);
    mojo_attributes->padding = mojom::Padding2d::New(
        mojom::Size2d::New(attributes.padding[0],
                           attributes.padding[2]) /* beginning padding*/,
        mojom::Size2d::New(attributes.padding[1],
                           attributes.padding[3]) /* ending padding*/);
    mojo_attributes->strides =
        mojom::Size2d::New(attributes.strides[0], attributes.strides[1]);
    mojo_attributes->dilations =
        mojom::Size2d::New(attributes.dilations[0], attributes.dilations[1]);
    mojo_attributes->layout = attributes.layout;
    builder.BuildOperator(
        kind, {input_operand_id}, {output_operand_id},
        mojom::OperatorAttributes::NewPool2d(std::move(mojo_attributes)));
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, Pool2dTest) {
  {
    // Test pool2d with default attributes.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 3, 4, 4}},
                 .attributes = {.window_dimensions = {1, 1}, .strides = {1, 1}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 3, 4, 4}},
                 .expected = true}
        .Test();
  }
  {
    // Test pool2d with window dimensions.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 3, 5, 5}},
                 .attributes = {.window_dimensions = {2, 2}, .strides = {2, 2}},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 3, 3, 3}},
                 .expected = true}
        .Test();
  }
  {
    // Test pool2d with strides=2, padding=1 and floor rounding.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 3, 7, 7}},
                 .attributes = {.window_dimensions = {4, 4},
                                .padding = {1, 1, 1, 1},
                                .strides = {2, 2}},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 3, 3, 3}},
                 .expected = true}
        .Test();
  }
  {
    // Test pool2d with strides=2, padding=1 and ceil rounding.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kInt8,
                           .dimensions = {1, 3, 7, 7}},
                 .attributes = {.window_dimensions = {4, 4},
                                .padding = {1, 1, 1, 1},
                                .strides = {2, 2}},
                 .output = {.type = mojom::Operand::DataType::kInt8,
                            .dimensions = {1, 3, 4, 4}},
                 .expected = true}
        .Test();
  }
  {
    // Test pool2d with layout="nhwc".
    Pool2dTester{
        .input = {.type = mojom::Operand::DataType::kInt8,
                  .dimensions = {1, 5, 5, 2}},
        .attributes = {.window_dimensions = {3, 3},
                       .strides = {1, 1},
                       .layout = mojom::InputOperandLayout::kChannelsLast},
        .output = {.type = mojom::Operand::DataType::kInt8,
                   .dimensions = {1, 3, 3, 2}},
        .expected = true}
        .Test();
  }
  {
    // Test the invalid graph when the input is not a 4-D tensor.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {3, 5, 5}},
                 .attributes = {.window_dimensions = {5, 5},
                                .padding = {2, 2, 2, 2},
                                .strides = {1, 1}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {3, 5, 5}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when window dimensions are 0.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 3, 4, 4}},
                 .attributes = {.window_dimensions = {0, 0}, .strides = {1, 1}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 3, 4, 4}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when strides are 0.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 3, 4, 4}},
                 .attributes = {.window_dimensions = {1, 1}, .strides = {0, 0}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 3, 4, 4}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when dilations are 0.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 3, 4, 4}},
                 .attributes = {.window_dimensions = {1, 1},
                                .strides = {1, 1},
                                .dilations = {0, 0}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 3, 4, 4}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 3, 4, 4}},
                 .attributes = {.window_dimensions = {4, 4}, .strides = {1, 1}},
                 .output = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 2, 1, 1}},
                 .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    Pool2dTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                           .dimensions = {1, 3, 4, 4}},
                 .attributes = {.window_dimensions = {4, 4}, .strides = {1, 1}},
                 .output = {.type = mojom::Operand::DataType::kInt32,
                            .dimensions = {1, 3, 1, 1}},
                 .expected = false}
        .Test();
  }
}

struct ReluTester {
  OperandInfo input;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t input_operand_id =
        builder.BuildInput("input", input.dimensions, input.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    builder.BuildOperator(mojom::Operator::Kind::kRelu, {input_operand_id},
                          {output_operand_id});
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, ReluTest) {
  {
    // Test relu operator for 3-D tensor with float32 input.
    ReluTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                         .dimensions = {2, 6, 4}},
               .output = {.type = mojom::Operand::DataType::kFloat32,
                          .dimensions = {2, 6, 4}},
               .expected = true}
        .Test();
  }
  {
    // Test relu operator for 4-D tensor with int32 input.
    ReluTester{.input = {.type = mojom::Operand::DataType::kInt32,
                         .dimensions = {1, 5, 3, 7}},
               .output = {.type = mojom::Operand::DataType::kInt32,
                          .dimensions = {1, 5, 3, 7}},
               .expected = true}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    ReluTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                         .dimensions = {4, 2}},
               .output = {.type = mojom::Operand::DataType::kFloat32,
                          .dimensions = {2}},
               .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    ReluTester{
        .input = {.type = mojom::Operand::DataType::kFloat32,
                  .dimensions = {2}},
        .output = {.type = mojom::Operand::DataType::kInt32, .dimensions = {2}},
        .expected = false}
        .Test();
  }
}

struct ReshapeTester {
  OperandInfo input;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t input_operand_id =
        builder.BuildInput("input", input.dimensions, input.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    builder.BuildOperator(mojom::Operator::Kind::kReshape, {input_operand_id},
                          {output_operand_id});
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, ReshapeTest) {
  {
    // Test reshape operator from 2-D tensor to 1-D tensor.
    ReshapeTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {2, 4}},
                  .output = {.type = mojom::Operand::DataType::kFloat32,
                             .dimensions = {8}},
                  .expected = true}
        .Test();
  }
  {
    // Test reshape operator from 4-D tensor to 2-D tensor.
    ReshapeTester{.input = {.type = mojom::Operand::DataType::kInt32,
                            .dimensions = {1, 3, 2, 1}},
                  .output = {.type = mojom::Operand::DataType::kInt32,
                             .dimensions = {1, 6}},
                  .expected = true}
        .Test();
  }
  {
    // Test the invalid graph when one value of new shape is 0.
    ReshapeTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {4, 2}},
                  .output = {.type = mojom::Operand::DataType::kFloat32,
                             .dimensions = {2, 0}},
                  .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when the number of input elements are not equal to
    // the number of output elements.
    ReshapeTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {2, 3, 4}},
                  .output = {.type = mojom::Operand::DataType::kInt32,
                             .dimensions = {3, 5}},
                  .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    ReshapeTester{
        .input = {.type = mojom::Operand::DataType::kFloat32,
                  .dimensions = {2}},
        .output = {.type = mojom::Operand::DataType::kInt32, .dimensions = {2}},
        .expected = false}
        .Test();
  }
}

struct SoftmaxTester {
  OperandInfo input;
  OperandInfo output;
  bool expected;

  void Test() {
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t input_operand_id =
        builder.BuildInput("input", input.dimensions, input.type);
    uint64_t output_operand_id =
        builder.BuildOutput("output", output.dimensions, output.type);
    builder.BuildOperator(mojom::Operator::Kind::kSoftmax, {input_operand_id},
                          {output_operand_id});
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, SoftmaxTest) {
  {
    // Test softmax operator for input operand with [2, 2] dimensions.
    SoftmaxTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {2, 2}},
                  .output = {.type = mojom::Operand::DataType::kFloat32,
                             .dimensions = {2, 2}},
                  .expected = true}
        .Test();
  }
  {
    // Test softmax operator for input operand with [1, 4] dimensions.
    SoftmaxTester{.input = {.type = mojom::Operand::DataType::kFloat16,
                            .dimensions = {1, 4}},
                  .output = {.type = mojom::Operand::DataType::kFloat16,
                             .dimensions = {1, 4}},
                  .expected = true}
        .Test();
  }
  {
    // Test the invalid graph when building softmax with 4-D input.
    SoftmaxTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {1, 1, 4, 2}},
                  .output = {.type = mojom::Operand::DataType::kFloat32,
                             .dimensions = {1, 1, 4, 2}},
                  .expected = false}
        .Test();
  }
  {
    // Test the invalid graph when building softmax with int32 input.
    SoftmaxTester{.input = {.type = mojom::Operand::DataType::kInt32,
                            .dimensions = {2, 3}},
                  .output = {.type = mojom::Operand::DataType::kInt32,
                             .dimensions = {2, 3}},
                  .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for the output shapes are not expected.
    SoftmaxTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {4, 2}},
                  .output = {.type = mojom::Operand::DataType::kFloat32,
                             .dimensions = {2}},
                  .expected = false}
        .Test();
  }
  {
    // Test the invalid graph for output types don't match.
    SoftmaxTester{.input = {.type = mojom::Operand::DataType::kFloat32,
                            .dimensions = {2, 5}},
                  .output = {.type = mojom::Operand::DataType::kFloat16,
                             .dimensions = {2, 5}},
                  .expected = false}
        .Test();
  }
}

TEST_F(WebNNGraphImplTest, ValidateInputsTest) {
  const std::vector<uint32_t> dimensions = {3, 5};
  // Build the graph with mojo type.
  GraphInfoBuilder builder;
  uint64_t lhs_operand_id =
      builder.BuildInput("lhs", dimensions, mojom::Operand::DataType::kUint8);
  uint64_t rhs_operand_id =
      builder.BuildInput("rhs", dimensions, mojom::Operand::DataType::kUint8);
  uint64_t output_operand_id = builder.BuildOutput(
      "output", dimensions, mojom::Operand::DataType::kUint8);
  builder.BuildOperator(mojom::Operator::Kind::kAdd,
                        {lhs_operand_id, rhs_operand_id}, {output_operand_id});
  EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), true);

  auto byte_length =
      ValidateAndCalculateByteLength(sizeof(uint8_t), dimensions).value();
  {
    // Validate the inputs match the expected.
    base::flat_map<std::string, mojo_base::BigBuffer> inputs;
    inputs["lhs"] = std::vector<uint8_t>(byte_length);
    inputs["rhs"] = std::vector<uint8_t>(byte_length);
    EXPECT_EQ(
        ValidateInputsForComputing(builder.CloneGraphInfo(), std::move(inputs)),
        true);
  }
  {
    // Test the invalid inputs for invalid input size.
    base::flat_map<std::string, mojo_base::BigBuffer> inputs;
    inputs["lhs"] = std::vector<uint8_t>(byte_length);
    EXPECT_EQ(
        ValidateInputsForComputing(builder.CloneGraphInfo(), std::move(inputs)),
        false);
  }
  {
    // Test the invalid inputs for invalid input name.
    base::flat_map<std::string, mojo_base::BigBuffer> inputs;
    inputs["a_different_input_name"] = std::vector<uint8_t>(byte_length);
    inputs["rhs"] = std::vector<uint8_t>(byte_length);
    EXPECT_EQ(
        ValidateInputsForComputing(builder.CloneGraphInfo(), std::move(inputs)),
        false);
  }
  {
    // Test the invalid inputs for invalid first input byte length.
    base::flat_map<std::string, mojo_base::BigBuffer> inputs;
    inputs["lhs"] = std::vector<uint8_t>(20);
    inputs["rhs"] = std::vector<uint8_t>(byte_length);
    EXPECT_EQ(
        ValidateInputsForComputing(builder.CloneGraphInfo(), std::move(inputs)),
        false);
  }
  {
    // Test the invalid inputs for invalid second input byte length.
    base::flat_map<std::string, mojo_base::BigBuffer> inputs;
    inputs["lhs"] = std::vector<uint8_t>(byte_length);
    inputs["rhs"] = std::vector<uint8_t>(20);
    EXPECT_EQ(
        ValidateInputsForComputing(builder.CloneGraphInfo(), std::move(inputs)),
        false);
  }
}

struct ConstantOperandTester {
  std::vector<uint8_t> values;
  bool expected;

  void Test() {
    const std::vector<uint32_t> dimensions = {3, 5};
    // Build the graph with mojo type.
    GraphInfoBuilder builder;
    uint64_t lhs_operand_id =
        builder.BuildInput("lhs", dimensions, mojom::Operand::DataType::kUint8);
    uint64_t rhs_operand_id = builder.BuildConstant(
        dimensions, mojom::Operand::DataType::kUint8, values);
    uint64_t output_operand_id = builder.BuildOutput(
        "output", dimensions, mojom::Operand::DataType::kUint8);
    builder.BuildOperator(mojom::Operator::Kind::kAdd,
                          {lhs_operand_id, rhs_operand_id},
                          {output_operand_id});
    EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), expected);
  }
};

TEST_F(WebNNGraphImplTest, ValidateConstantOperandTest) {
  {
    // Test valid constant data.
    ConstantOperandTester{.values = std::vector<uint8_t>(15), .expected = true}
        .Test();
  }
  {
    // Test the invalid graph for the byte length of constant data doesn't match
    // the graph's expected.
    ConstantOperandTester{.values = std::vector<uint8_t>(10), .expected = false}
        .Test();
  }
}

// Test building a graph with two inputs and two constant in the following
// topology.
//    [input_a] [constant_a] [input_b] [constant_b]
//           \    /                \    /
//            gemm                  gemm
//                \                /
//                       gemm
TEST_F(WebNNGraphImplTest, BuildMultipleInputsAppendingConstants) {
  // Build the mojom graph info.
  GraphInfoBuilder builder;
  // The graph outputs are built first, and then inputs / constants.
  uint64_t output_operand_id =
      builder.BuildOutput("output", {2, 2}, mojom::Operand::DataType::kFloat32);
  uint64_t input_a_operand_id =
      builder.BuildInput("input_a", {2, 2}, mojom::Operand::DataType::kFloat32);
  std::vector<float> constant_data = {5.0, 6.0, 7.0, 8.0};
  uint64_t constant_a_operand_id = builder.BuildConstant(
      {2, 2}, mojom::Operand::DataType::kFloat32,
      base::make_span(reinterpret_cast<const uint8_t*>(constant_data.data()),
                      constant_data.size() * sizeof(float)));
  uint64_t intermediate_1_operand_id = builder.BuildIntermediateOperand(
      {2, 2}, mojom::Operand::DataType::kFloat32);
  builder.BuildOperator(
      mojom::Operator::Kind::kGemm, {input_a_operand_id, constant_a_operand_id},
      {intermediate_1_operand_id},
      mojom::OperatorAttributes::NewGemm(mojom::GemmAttributes::New()));

  uint64_t input_b_operand_id =
      builder.BuildInput("input_b", {2, 2}, mojom::Operand::DataType::kFloat32);
  uint64_t constant_b_operand_id = builder.BuildConstant(
      {2, 2}, mojom::Operand::DataType::kFloat32,
      base::make_span(reinterpret_cast<const uint8_t*>(constant_data.data()),
                      constant_data.size() * sizeof(float)));
  uint64_t intermediate_2_operand_id = builder.BuildIntermediateOperand(
      {2, 2}, mojom::Operand::DataType::kFloat32);
  builder.BuildOperator(
      mojom::Operator::Kind::kGemm, {input_b_operand_id, constant_b_operand_id},
      {intermediate_2_operand_id},
      mojom::OperatorAttributes::NewGemm(mojom::GemmAttributes::New()));
  builder.BuildOperator(
      mojom::Operator::Kind::kGemm,
      {intermediate_1_operand_id, intermediate_2_operand_id},
      {output_operand_id},
      mojom::OperatorAttributes::NewGemm(mojom::GemmAttributes::New()));
  EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), true);
}

// Test building a graph with two inputs and two constant in the following
// topology.
//    [constant_a] [input_a] [constant_b] [input_b]
//           \    /                \    /
//            gemm                  gemm
//                \                /
//                       gemm
TEST_F(WebNNGraphImplTest, BuildMultipleConstantsAppendingInputs) {
  // Build the mojom graph info.
  GraphInfoBuilder builder;
  // The graph outputs are built first, and then inputs / constants.
  uint64_t output_operand_id =
      builder.BuildOutput("output", {2, 2}, mojom::Operand::DataType::kFloat32);
  std::vector<float> constant_data = {5.0, 6.0, 7.0, 8.0};
  uint64_t constant_a_operand_id = builder.BuildConstant(
      {2, 2}, mojom::Operand::DataType::kFloat32,
      base::make_span(reinterpret_cast<const uint8_t*>(constant_data.data()),
                      constant_data.size() * sizeof(float)));
  uint64_t input_a_operand_id =
      builder.BuildInput("input_a", {2, 2}, mojom::Operand::DataType::kFloat32);
  uint64_t intermediate_1_operand_id = builder.BuildIntermediateOperand(
      {2, 2}, mojom::Operand::DataType::kFloat32);
  builder.BuildOperator(
      mojom::Operator::Kind::kGemm, {constant_a_operand_id, input_a_operand_id},
      {intermediate_1_operand_id},
      mojom::OperatorAttributes::NewGemm(mojom::GemmAttributes::New()));

  uint64_t input_b_operand_id =
      builder.BuildInput("input_b", {2, 2}, mojom::Operand::DataType::kFloat32);
  uint64_t constant_b_operand_id = builder.BuildConstant(
      {2, 2}, mojom::Operand::DataType::kFloat32,
      base::make_span(reinterpret_cast<const uint8_t*>(constant_data.data()),
                      constant_data.size() * sizeof(float)));
  uint64_t intermediate_2_operand_id = builder.BuildIntermediateOperand(
      {2, 2}, mojom::Operand::DataType::kFloat32);
  builder.BuildOperator(
      mojom::Operator::Kind::kGemm, {constant_b_operand_id, input_b_operand_id},
      {intermediate_2_operand_id},
      mojom::OperatorAttributes::NewGemm(mojom::GemmAttributes::New()));

  builder.BuildOperator(
      mojom::Operator::Kind::kGemm,
      {intermediate_1_operand_id, intermediate_2_operand_id},
      {output_operand_id},
      mojom::OperatorAttributes::NewGemm(mojom::GemmAttributes::New()));
  EXPECT_EQ(WebNNGraphImpl::ValidateGraph(builder.GetGraphInfo()), true);
}

}  // namespace webnn
