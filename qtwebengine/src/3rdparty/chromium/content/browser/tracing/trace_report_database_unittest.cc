// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/trace_report_database.h"

#include <string>

#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {

class TraceReportDatabaseTest : public testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(trace_report_.OpenDatabaseForTesting()); }

  TraceReportDatabase trace_report_;
};

TEST_F(TraceReportDatabaseTest, CreatingAndDroppingLocalTraceTable) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);
}

// Test without Initializing the database before
TEST(TraceReportDatabaseNoOpenTest, OpenDatabaseIfExists) {
  base::ScopedTempDir temp_dir;
  TraceReportDatabase trace_report;

  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  EXPECT_FALSE(trace_report.OpenDatabaseIfExists(temp_dir.GetPath()));

  EXPECT_TRUE(trace_report.OpenDatabase(temp_dir.GetPath()));
}

TEST_F(TraceReportDatabaseTest, AddingNewReport) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  // Create Report for the local traces database.
  TraceReportDatabase::NewReport new_report;
  new_report.uuid = base::Uuid::GenerateRandomV4();
  new_report.scenario_name = "scenario1";
  new_report.upload_rule_name = "rules1";
  new_report.total_size = 23192873129873128;
  new_report.proto = "Proto1";
  new_report.skip_reason = TraceReportDatabase::SkipUploadReason::kNoSkip;

  const auto new_size = new_report.total_size;

  ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));

  auto received_reports = trace_report_.GetAllReports();

  EXPECT_EQ(received_reports.size(), 1u);
  EXPECT_EQ(received_reports[0].scenario_name, "scenario1");
  EXPECT_EQ(received_reports[0].upload_rule_name, "rules1");
  EXPECT_EQ(received_reports[0].total_size, new_size);
  EXPECT_EQ(received_reports[0].state,
            TraceReportDatabase::ReportUploadState::kPending);
}

TEST_F(TraceReportDatabaseTest, RetreiveProtoFromTrace) {
  // Create Report for the local traces database.
  TraceReportDatabase::NewReport new_report;
  new_report.uuid = base::Uuid::GenerateRandomV4();
  new_report.scenario_name = "scenario2";
  new_report.upload_rule_name = "rules2";
  new_report.total_size = 23192873129873128;
  new_report.proto = "Proto2";

  const auto copie_value = new_report.uuid;

  ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));

  absl::optional<std::string> received_value =
      trace_report_.GetProtoValue(copie_value);
  ASSERT_TRUE(received_value);

  EXPECT_EQ(received_value, "Proto2");
}

TEST_F(TraceReportDatabaseTest, DeletingSingleTrace) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  // Create Report for the local traces database.
  TraceReportDatabase::NewReport new_report;
  new_report.uuid = base::Uuid::GenerateRandomV4();
  new_report.scenario_name = "scenario3";
  new_report.upload_rule_name = "rules3";
  new_report.total_size = 23192873129873128;
  new_report.proto = "Proto3";

  const auto copie_value = new_report.uuid;

  ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  EXPECT_EQ(trace_report_.GetAllReports().size(), 1u);

  ASSERT_TRUE(trace_report_.DeleteTrace(copie_value));
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);
}

TEST_F(TraceReportDatabaseTest, DeletingAllTraces) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  // Create multiple NewReport and add to the local_traces table.

  for (int i = 0; i < 5; i++) {
    TraceReportDatabase::NewReport new_report;
    new_report.uuid = base::Uuid::GenerateRandomV4();
    new_report.scenario_name = "scenario";
    new_report.upload_rule_name = "rules";
    new_report.total_size = 23192873129873128;
    new_report.proto = "Proto";

    ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  }

  EXPECT_EQ(trace_report_.GetAllReports().size(), 5u);

  ASSERT_TRUE(trace_report_.DeleteAllTraces());
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);
}

TEST_F(TraceReportDatabaseTest, DeletingTracesInRange) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  const base::Time today = base::Time::Now();
  // Create multiple NewReport and add to the local_traces table.

  for (int i = 0; i < 5; i++) {
    TraceReportDatabase::NewReport new_report;
    new_report.uuid = base::Uuid::GenerateRandomV4();
    new_report.creation_time = today;
    new_report.scenario_name = "scenario";
    new_report.upload_rule_name = "rules";
    new_report.total_size = 23192873129873128;
    new_report.proto = "Proto";

    ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  }

  for (int i = 0; i < 3; i++) {
    TraceReportDatabase::NewReport new_report;
    new_report.uuid = base::Uuid::GenerateRandomV4();
    new_report.creation_time = base::Time(today - base::Days(20));
    new_report.scenario_name = "scenario";
    new_report.upload_rule_name = "rules";
    new_report.total_size = 23192873129873128;
    new_report.proto = "Proto";

    ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  }

  for (int i = 0; i < 2; i++) {
    TraceReportDatabase::NewReport new_report;
    new_report.uuid = base::Uuid::GenerateRandomV4();
    new_report.creation_time = base::Time(today - base::Days(10));
    new_report.scenario_name = "scenario";
    new_report.upload_rule_name = "rules";
    new_report.total_size = 23192873129873128;
    new_report.proto = "Proto";

    ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  }

  EXPECT_EQ(trace_report_.GetAllReports().size(), 10u);

  const base::Time start = base::Time(today - base::Days(20));
  const base::Time end = base::Time(today - base::Days(10));

  ASSERT_TRUE(trace_report_.DeleteTracesInDateRange(start, end));
  EXPECT_EQ(trace_report_.GetAllReports().size(), 5u);
}

TEST_F(TraceReportDatabaseTest, DeleteTracesOlderThan) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  const base::Time today = base::Time::Now();

  // Create multiple NewReport and add to the local_traces table.
  for (int i = 0; i < 5; i++) {
    TraceReportDatabase::NewReport new_report;
    new_report.uuid = base::Uuid::GenerateRandomV4();
    new_report.creation_time = today;
    new_report.scenario_name = "scenario";
    new_report.upload_rule_name = "rules";
    new_report.total_size = 23192873129873128;
    new_report.proto = "Proto";

    ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  }

  for (int i = 0; i < 3; i++) {
    TraceReportDatabase::NewReport new_report;
    new_report.uuid = base::Uuid::GenerateRandomV4();
    new_report.creation_time = base::Time(today - base::Days(20));
    new_report.scenario_name = "scenario";
    new_report.upload_rule_name = "rules";
    new_report.total_size = 23192873129873128;
    new_report.proto = "Proto";

    ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  }

  EXPECT_EQ(trace_report_.GetAllReports().size(), 8u);

  ASSERT_TRUE(trace_report_.DeleteTracesOlderThan(base::Days(10)));
  EXPECT_EQ(trace_report_.GetAllReports().size(), 5u);
}

TEST_F(TraceReportDatabaseTest, UserRequestedUpload) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  // Create Report for the local traces database.
  TraceReportDatabase::NewReport new_report;
  new_report.uuid = base::Uuid::GenerateRandomV4();
  new_report.scenario_name = "scenario3";
  new_report.upload_rule_name = "rules3";
  new_report.total_size = 23192873129873128;
  new_report.proto = "Proto3";

  const auto copie_value = new_report.uuid;

  ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  EXPECT_EQ(trace_report_.GetAllReports().size(), 1u);

  ASSERT_TRUE(trace_report_.UserRequestedUpload(copie_value));

  auto all_traces = trace_report_.GetAllReports();
  EXPECT_EQ(all_traces.size(), 1u);
  EXPECT_EQ(all_traces[0].state,
            TraceReportDatabase::ReportUploadState::kPending_UserRequested);
}

TEST_F(TraceReportDatabaseTest, UploadComplete) {
  EXPECT_EQ(trace_report_.GetAllReports().size(), 0u);

  // Create Report for the local traces database.
  TraceReportDatabase::NewReport new_report;
  new_report.uuid = base::Uuid::GenerateRandomV4();
  new_report.scenario_name = "scenario3";
  new_report.upload_rule_name = "rules3";
  new_report.total_size = 23192873129873128;
  new_report.proto = "Proto3";
  new_report.skip_reason = TraceReportDatabase::SkipUploadReason::kNoSkip;

  const auto report_uuid = new_report.uuid;

  ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));
  EXPECT_EQ(trace_report_.GetAllReports().size(), 1u);

  auto uploaded_time = base::Time::Now();
  ASSERT_TRUE(trace_report_.UploadComplete(report_uuid, uploaded_time));

  auto all_traces = trace_report_.GetAllReports();
  EXPECT_EQ(all_traces.size(), 1u);
  EXPECT_EQ(all_traces[0].state,
            TraceReportDatabase::ReportUploadState::kUploaded);
  EXPECT_EQ(all_traces[0].upload_time, uploaded_time);

  EXPECT_FALSE(trace_report_.GetProtoValue(report_uuid));
}

TEST_F(TraceReportDatabaseTest, GetNextReportPendingUpload) {
  EXPECT_FALSE(trace_report_.GetNextReportPendingUpload());

  // Create Report for the local traces database.
  TraceReportDatabase::NewReport new_report;
  new_report.uuid = base::Uuid::GenerateRandomV4();
  new_report.scenario_name = "scenario3";
  new_report.upload_rule_name = "rules3";
  new_report.total_size = 23192873129873128;
  new_report.proto = "Proto3";
  new_report.skip_reason = TraceReportDatabase::SkipUploadReason::kNoSkip;

  const auto copie_value = new_report.uuid;

  ASSERT_TRUE(trace_report_.AddTrace(std::move(new_report)));

  auto upload_report = trace_report_.GetNextReportPendingUpload();
  ASSERT_TRUE(upload_report);
  EXPECT_EQ(upload_report->uuid, copie_value);

  auto uploaded_time = base::Time::Now();
  ASSERT_TRUE(trace_report_.UploadComplete(copie_value, uploaded_time));

  EXPECT_FALSE(trace_report_.GetNextReportPendingUpload());
}

}  // namespace content
