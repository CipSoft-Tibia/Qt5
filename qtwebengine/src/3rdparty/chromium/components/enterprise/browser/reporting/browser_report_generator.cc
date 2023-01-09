// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/enterprise/browser/reporting/browser_report_generator.h"

#include <memory>
#include <utility>

#include "base/version.h"
#include "build/build_config.h"
#include "components/enterprise/browser/reporting/reporting_delegate_factory.h"
#include "components/policy/core/common/cloud/cloud_policy_util.h"
#include "components/version_info/version_info.h"

namespace em = ::enterprise_management;

namespace enterprise_reporting {

BrowserReportGenerator::BrowserReportGenerator(
    ReportingDelegateFactory* delegate_factory)
    : delegate_(delegate_factory->GetBrowserReportGeneratorDelegate()) {}

BrowserReportGenerator::~BrowserReportGenerator() = default;

void BrowserReportGenerator::Generate(ReportCallback callback) {
  auto report = std::make_unique<em::BrowserReport>();
  GenerateBasicInfo(report.get());
  delegate_->GenerateProfileInfo(report.get());

  // std::move is required here because the function completes the report
  // asynchronously.
  delegate_->GeneratePluginsIfNeeded(std::move(callback), std::move(report));
}

void BrowserReportGenerator::GenerateBasicInfo(em::BrowserReport* report) {
#if !defined(OS_CHROMEOS)
  report->set_browser_version(version_info::GetVersionNumber());
  report->set_channel(policy::ConvertToProtoChannel(delegate_->GetChannel()));
  delegate_->GenerateBuildStateInfo(report);
#endif

  report->set_executable_path(delegate_->GetExecutablePath());
}

}  // namespace enterprise_reporting
