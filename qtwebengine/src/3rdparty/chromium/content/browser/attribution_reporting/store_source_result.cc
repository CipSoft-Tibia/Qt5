// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/attribution_reporting/store_source_result.h"

#include "base/functional/overloaded.h"
#include "content/browser/attribution_reporting/store_source_result.mojom.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace content {

namespace {
using StatusSSR = ::attribution_reporting::mojom::StoreSourceResult;
}  // namespace

StatusSSR StoreSourceResult::status() const {
  return absl::visit(
      base::Overloaded{
          [](Success) { return StatusSSR::kSuccess; },
          [](InternalError) { return StatusSSR::kInternalError; },
          [](InsufficientSourceCapacity) {
            return StatusSSR::kInsufficientSourceCapacity;
          },
          [](InsufficientUniqueDestinationCapacity) {
            return StatusSSR::kInsufficientUniqueDestinationCapacity;
          },
          [](ExcessiveReportingOrigins) {
            return StatusSSR::kExcessiveReportingOrigins;
          },
          [](ProhibitedByBrowserPolicy) {
            return StatusSSR::kProhibitedByBrowserPolicy;
          },
          [](SuccessNoised) { return StatusSSR::kSuccessNoised; },
          [](DestinationReportingLimitReached) {
            return StatusSSR::kDestinationReportingLimitReached;
          },
          [](DestinationGlobalLimitReached) {
            return StatusSSR::kDestinationGlobalLimitReached;
          },
          [](DestinationBothLimitsReached) {
            return StatusSSR::kDestinationBothLimitsReached;
          },
          [](ReportingOriginsPerSiteLimitReached) {
            return StatusSSR::kReportingOriginsPerSiteLimitReached;
          },
          [](ExceedsMaxChannelCapacity) {
            return StatusSSR::kExceedsMaxChannelCapacity;
          },
      },
      result_);
}

}  // namespace content
