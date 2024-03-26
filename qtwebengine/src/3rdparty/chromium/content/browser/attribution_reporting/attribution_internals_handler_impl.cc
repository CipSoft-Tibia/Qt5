// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/attribution_reporting/attribution_internals_handler_impl.h"

#include <stdint.h>

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/functional/overloaded.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "components/aggregation_service/parsing_utils.h"
#include "components/attribution_reporting/aggregation_keys.h"
#include "components/attribution_reporting/parsing_utils.h"
#include "components/attribution_reporting/source_registration_error.mojom.h"
#include "components/attribution_reporting/source_type.mojom-forward.h"
#include "components/attribution_reporting/suitable_origin.h"
#include "components/attribution_reporting/trigger_registration.h"
#include "content/browser/attribution_reporting/attribution_debug_report.h"
#include "content/browser/attribution_reporting/attribution_info.h"
#include "content/browser/attribution_reporting/attribution_internals.mojom.h"
#include "content/browser/attribution_reporting/attribution_observer_types.h"
#include "content/browser/attribution_reporting/attribution_report.h"
#include "content/browser/attribution_reporting/attribution_trigger.h"
#include "content/browser/attribution_reporting/attribution_utils.h"
#include "content/browser/attribution_reporting/common_source_info.h"
#include "content/browser/attribution_reporting/send_result.h"
#include "content/browser/attribution_reporting/storable_source.h"
#include "content/browser/attribution_reporting/stored_source.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "net/base/net_errors.h"
#include "net/base/schemeful_site.h"
#include "third_party/abseil-cpp/absl/numeric/int128.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "third_party/abseil-cpp/absl/utility/utility.h"
#include "url/origin.h"

namespace content {

namespace {

using Attributability =
    ::attribution_internals::mojom::WebUISource::Attributability;

using Empty = ::attribution_internals::mojom::Empty;
using ReportStatus = ::attribution_internals::mojom::ReportStatus;
using ReportStatusPtr = ::attribution_internals::mojom::ReportStatusPtr;

using ::attribution_internals::mojom::WebUIDebugReport;

attribution_internals::mojom::WebUISourcePtr WebUISource(
    const StoredSource& source,
    Attributability attributability) {
  const CommonSourceInfo& common_info = source.common_info();
  return attribution_internals::mojom::WebUISource::New(
      common_info.source_event_id(), common_info.source_origin(),
      std::vector<net::SchemefulSite>(
          common_info.destination_sites().destinations().begin(),
          common_info.destination_sites().destinations().end()),
      common_info.reporting_origin(), common_info.source_time().ToJsTime(),
      common_info.expiry_time().ToJsTime(),
      common_info.event_report_window_time().ToJsTime(),
      common_info.aggregatable_report_window_time().ToJsTime(),
      common_info.source_type(), common_info.priority(),
      common_info.debug_key(), source.dedup_keys(),
      common_info.filter_data().filter_values(),
      base::MakeFlatMap<std::string, std::string>(
          common_info.aggregation_keys().keys(), {},
          [](const auto& key) {
            return std::make_pair(
                key.first,
                attribution_reporting::HexEncodeAggregationKey(key.second));
          }),
      source.aggregatable_budget_consumed(), source.aggregatable_dedup_keys(),
      attributability);
}

void ForwardSourcesToWebUI2(
    attribution_internals::mojom::Handler::GetActiveSourcesCallback
        web_ui_callback,
    std::vector<StoredSource> active_sources) {
  std::vector<attribution_internals::mojom::WebUISourcePtr> web_ui_sources;
  web_ui_sources.reserve(active_sources.size());

  for (const StoredSource& source : active_sources) {
    Attributability attributability;
    switch (source.attribution_logic()) {
      case StoredSource::AttributionLogic::kTruthfully:
        attributability = Attributability::kAttributable;
        break;
      case StoredSource::AttributionLogic::kNever:
        attributability = Attributability::kNoisedNever;
        break;
      case StoredSource::AttributionLogic::kFalsely:
        attributability = Attributability::kNoisedFalsely;
        break;
    }

    if (attributability == Attributability::kAttributable) {
      switch (source.active_state()) {
        case StoredSource::ActiveState::kActive:
          attributability = Attributability::kAttributable;
          break;
        case StoredSource::ActiveState::kReachedEventLevelAttributionLimit:
          attributability = Attributability::kReachedEventLevelAttributionLimit;
          break;
        case StoredSource::ActiveState::kInactive:
          NOTREACHED();
          return;
      }
    }

    web_ui_sources.push_back(WebUISource(source, attributability));
  }

  std::move(web_ui_callback).Run(std::move(web_ui_sources));
}

attribution_internals::mojom::WebUIReportPtr WebUIReport(
    const AttributionReport& report,
    bool is_debug_report,
    ReportStatusPtr status) {
  namespace ai_mojom = attribution_internals::mojom;

  const AttributionInfo& attribution_info = report.attribution_info();

  ai_mojom::WebUIReportDataPtr data = absl::visit(
      base::Overloaded{
          [attribution_info](
              const AttributionReport::EventLevelData& event_level_data) {
            return ai_mojom::WebUIReportData::NewEventLevelData(
                ai_mojom::WebUIReportEventLevelData::New(
                    event_level_data.priority,
                    attribution_info.source.attribution_logic() ==
                        StoredSource::AttributionLogic::kTruthfully));
          },

          [](const AttributionReport::AggregatableAttributionData&
                 aggregatable_data) {
            std::vector<ai_mojom::AggregatableHistogramContributionPtr>
                contributions;
            base::ranges::transform(
                aggregatable_data.contributions,
                std::back_inserter(contributions),
                [](const auto& contribution) {
                  return ai_mojom::AggregatableHistogramContribution::New(
                      attribution_reporting::HexEncodeAggregationKey(
                          contribution.key()),
                      contribution.value());
                });

            ai_mojom::AttestationTokenPtr attestation_token =
                aggregatable_data.attestation_token
                    ? ai_mojom::AttestationToken::New(
                          *aggregatable_data.attestation_token)
                    : nullptr;

            return ai_mojom::WebUIReportData::NewAggregatableAttributionData(
                ai_mojom::WebUIReportAggregatableAttributionData::New(
                    std::move(contributions), std::move(attestation_token),
                    ::aggregation_service::SerializeAggregationCoordinator(
                        aggregatable_data.aggregation_coordinator)));
          },
      },
      report.data());

  return attribution_internals::mojom::WebUIReport::New(
      report.ReportId(), report.ReportURL(is_debug_report),
      /*trigger_time=*/attribution_info.time.ToJsTime(),
      /*report_time=*/report.report_time().ToJsTime(),
      SerializeAttributionJson(report.ReportBody(), /*pretty_print=*/true),
      std::move(status), std::move(data));
}

void ForwardReportsToWebUI2(
    attribution_internals::mojom::Handler::GetReportsCallback web_ui_callback,
    std::vector<AttributionReport> pending_reports) {
  std::vector<attribution_internals::mojom::WebUIReportPtr> web_ui_reports;
  web_ui_reports.reserve(pending_reports.size());
  for (const AttributionReport& report : pending_reports) {
    web_ui_reports.push_back(
        WebUIReport(report, /*is_debug_report=*/false,
                    ReportStatus::NewPending(Empty::New())));
  }

  std::move(web_ui_callback).Run(std::move(web_ui_reports));
}

}  // namespace

AttributionInternalsHandlerImpl::AttributionInternalsHandlerImpl(
    WebUI* web_ui,
    mojo::PendingRemote<attribution_internals::mojom::Observer> observer,
    mojo::PendingReceiver<attribution_internals::mojom::Handler> handler)
    : web_ui_(web_ui),
      observer_(std::move(observer)),
      handler_(this, std::move(handler)) {
  DCHECK(web_ui_);
  if (auto* manager =
          AttributionManager::FromWebContents(web_ui_->GetWebContents())) {
    manager_observation_.Observe(manager);
    observer_.set_disconnect_handler(
        base::BindOnce(&AttributionInternalsHandlerImpl::OnObserverDisconnected,
                       base::Unretained(this)));
  }
}

AttributionInternalsHandlerImpl::~AttributionInternalsHandlerImpl() = default;

void AttributionInternalsHandlerImpl::IsAttributionReportingEnabled(
    attribution_internals::mojom::Handler::IsAttributionReportingEnabledCallback
        callback) {
  content::WebContents* contents = web_ui_->GetWebContents();
  bool attribution_reporting_enabled =
      AttributionManager::FromWebContents(contents) &&
      GetContentClient()->browser()->IsAttributionReportingOperationAllowed(
          contents->GetBrowserContext(),
          ContentBrowserClient::AttributionReportingOperation::kAny,
          /*rfh=*/nullptr, /*source_origin=*/nullptr,
          /*destination_origin=*/nullptr, /*reporting_origin=*/nullptr);
  bool debug_mode = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAttributionReportingDebugMode);
  std::move(callback).Run(attribution_reporting_enabled, debug_mode);
}

void AttributionInternalsHandlerImpl::GetActiveSources(
    attribution_internals::mojom::Handler::GetActiveSourcesCallback callback) {
  if (AttributionManager* manager =
          AttributionManager::FromWebContents(web_ui_->GetWebContents())) {
    manager->GetActiveSourcesForWebUI(
        base::BindOnce(&ForwardSourcesToWebUI2, std::move(callback)));
  } else {
    std::move(callback).Run({});
  }
}

void AttributionInternalsHandlerImpl::GetReports(
    AttributionReport::Type report_type,
    attribution_internals::mojom::Handler::GetReportsCallback callback) {
  if (AttributionManager* manager =
          AttributionManager::FromWebContents(web_ui_->GetWebContents())) {
    manager->GetPendingReportsForInternalUse(
        AttributionReport::Types{report_type},
        /*limit=*/1000,
        base::BindOnce(&ForwardReportsToWebUI2, std::move(callback)));
  } else {
    std::move(callback).Run({});
  }
}

void AttributionInternalsHandlerImpl::SendReports(
    const std::vector<AttributionReport::Id>& ids,
    attribution_internals::mojom::Handler::SendReportsCallback callback) {
  if (AttributionManager* manager =
          AttributionManager::FromWebContents(web_ui_->GetWebContents())) {
    manager->SendReportsForWebUI(ids, std::move(callback));
  } else {
    std::move(callback).Run();
  }
}

void AttributionInternalsHandlerImpl::ClearStorage(
    attribution_internals::mojom::Handler::ClearStorageCallback callback) {
  if (AttributionManager* manager =
          AttributionManager::FromWebContents(web_ui_->GetWebContents())) {
    manager->ClearData(base::Time::Min(), base::Time::Max(),
                       /*filter=*/base::NullCallback(),
                       /*filter_builder=*/nullptr,
                       /*delete_rate_limit_data=*/true, std::move(callback));
  } else {
    std::move(callback).Run();
  }
}

void AttributionInternalsHandlerImpl::OnSourcesChanged() {
  observer_->OnSourcesChanged();
}

void AttributionInternalsHandlerImpl::OnReportsChanged(
    AttributionReport::Type report_type) {
  observer_->OnReportsChanged(report_type);
}

namespace {

using WebUISourceRegistration =
    ::attribution_internals::mojom::WebUISourceRegistration;

attribution_internals::mojom::WebUIRegistrationPtr GetRegistration(
    base::Time time,
    const attribution_reporting::SuitableOrigin& context_origin,
    const attribution_reporting::SuitableOrigin& reporting_origin,
    std::string registration_json,
    absl::optional<uint64_t> cleared_debug_key) {
  auto reg = attribution_internals::mojom::WebUIRegistration::New();
  reg->time = time.ToJsTime();
  reg->context_origin = context_origin;
  reg->reporting_origin = reporting_origin;
  reg->registration_json = std::move(registration_json);
  reg->cleared_debug_key = cleared_debug_key;
  return reg;
}

}  // namespace

void AttributionInternalsHandlerImpl::OnSourceHandled(
    const StorableSource& source,
    absl::optional<uint64_t> cleared_debug_key,
    attribution_reporting::mojom::StoreSourceResult result) {
  auto web_ui_source = WebUISourceRegistration::New();
  web_ui_source->registration = GetRegistration(
      source.common_info().source_time(), source.common_info().source_origin(),
      source.common_info().reporting_origin(), source.registration_json(),
      cleared_debug_key);
  web_ui_source->type = source.common_info().source_type();
  web_ui_source->status =
      attribution_internals::mojom::SourceStatus::NewStoreSourceResult(result);

  observer_->OnSourceHandled(std::move(web_ui_source));
}

void AttributionInternalsHandlerImpl::OnReportSent(
    const AttributionReport& report,
    bool is_debug_report,
    const SendResult& info) {
  ReportStatusPtr status;
  switch (info.status) {
    case SendResult::Status::kSent:
      status = ReportStatus::NewSent(info.http_response_code);
      break;
    case SendResult::Status::kDropped:
      status = ReportStatus::NewProhibitedByBrowserPolicy(Empty::New());
      break;
    case SendResult::Status::kFailure:
    case SendResult::Status::kTransientFailure:
      status = ReportStatus::NewNetworkError(
          net::ErrorToShortString(info.network_error));
      break;
    case SendResult::Status::kFailedToAssemble:
      status = ReportStatus::NewFailedToAssemble(Empty::New());
      break;
  }

  observer_->OnReportSent(
      WebUIReport(report, is_debug_report, std::move(status)));
}

void AttributionInternalsHandlerImpl::OnDebugReportSent(
    const AttributionDebugReport& report,
    int status,
    base::Time time) {
  auto web_report = WebUIDebugReport::New();
  web_report->url = report.report_url();
  web_report->time = time.ToJsTime();
  web_report->body =
      SerializeAttributionJson(report.ReportBody(), /*pretty_print=*/true);

  web_report->status =
      status > 0
          ? attribution_internals::mojom::DebugReportStatus::
                NewHttpResponseCode(status)
          : attribution_internals::mojom::DebugReportStatus::NewNetworkError(
                net::ErrorToShortString(status));

  observer_->OnDebugReportSent(std::move(web_report));
}

// TODO(crbug/1351843): Consider surfacing this error in devtools instead of
// internals, currently however this error is associated with a redirect
// navigation, rather than a specific committed page.
void AttributionInternalsHandlerImpl::OnFailedSourceRegistration(
    const std::string& header_value,
    base::Time source_time,
    const attribution_reporting::SuitableOrigin& source_origin,
    const attribution_reporting::SuitableOrigin& reporting_origin,
    attribution_reporting::mojom::SourceType source_type,
    attribution_reporting::mojom::SourceRegistrationError error) {
  auto web_ui_source = WebUISourceRegistration::New();
  web_ui_source->registration = GetRegistration(
      source_time, source_origin, reporting_origin, header_value,
      /*cleared_debug_key=*/absl::nullopt);
  web_ui_source->type = source_type;
  web_ui_source->status =
      attribution_internals::mojom::SourceStatus::NewJsonError(error);

  observer_->OnSourceHandled(std::move(web_ui_source));
}

namespace {

using AggregatableStatus = ::content::AttributionTrigger::AggregatableResult;
using EventLevelStatus = ::content::AttributionTrigger::EventLevelResult;
using WebUITriggerStatus = ::attribution_internals::mojom::WebUITrigger::Status;

WebUITriggerStatus GetWebUITriggerStatus(EventLevelStatus status) {
  switch (status) {
    case EventLevelStatus::kSuccess:
    case EventLevelStatus::kSuccessDroppedLowerPriority:
      return WebUITriggerStatus::kSuccess;
    case EventLevelStatus::kInternalError:
      return WebUITriggerStatus::kInternalError;
    case EventLevelStatus::kNoCapacityForConversionDestination:
      return WebUITriggerStatus::kNoReportCapacityForDestinationSite;
    case EventLevelStatus::kNoMatchingImpressions:
      return WebUITriggerStatus::kNoMatchingSources;
    case EventLevelStatus::kDeduplicated:
      return WebUITriggerStatus::kDeduplicated;
    case EventLevelStatus::kExcessiveAttributions:
      return WebUITriggerStatus::kExcessiveAttributions;
    case EventLevelStatus::kPriorityTooLow:
      return WebUITriggerStatus::kLowPriority;
    case EventLevelStatus::kDroppedForNoise:
    case EventLevelStatus::kFalselyAttributedSource:
      return WebUITriggerStatus::kNoised;
    case EventLevelStatus::kExcessiveReportingOrigins:
      return WebUITriggerStatus::kExcessiveReportingOrigins;
    case EventLevelStatus::kNoMatchingSourceFilterData:
      return WebUITriggerStatus::kNoMatchingSourceFilterData;
    case EventLevelStatus::kProhibitedByBrowserPolicy:
      return WebUITriggerStatus::kProhibitedByBrowserPolicy;
    case EventLevelStatus::kNoMatchingConfigurations:
      return WebUITriggerStatus::kNoMatchingConfigurations;
    case EventLevelStatus::kExcessiveReports:
      return WebUITriggerStatus::kExcessiveEventLevelReports;
    case EventLevelStatus::kReportWindowPassed:
      return WebUITriggerStatus::kReportWindowPassed;
    case EventLevelStatus::kNotRegistered:
      return WebUITriggerStatus::kNotRegistered;
  }
}

WebUITriggerStatus GetWebUITriggerStatus(AggregatableStatus status) {
  switch (status) {
    case AggregatableStatus::kSuccess:
      return WebUITriggerStatus::kSuccess;
    case AggregatableStatus::kInternalError:
      return WebUITriggerStatus::kInternalError;
    case AggregatableStatus::kNoCapacityForConversionDestination:
      return WebUITriggerStatus::kNoReportCapacityForDestinationSite;
    case AggregatableStatus::kNoMatchingImpressions:
      return WebUITriggerStatus::kNoMatchingSources;
    case AggregatableStatus::kExcessiveAttributions:
      return WebUITriggerStatus::kExcessiveAttributions;
    case AggregatableStatus::kExcessiveReportingOrigins:
      return WebUITriggerStatus::kExcessiveReportingOrigins;
    case AggregatableStatus::kNoHistograms:
      return WebUITriggerStatus::kNoHistograms;
    case AggregatableStatus::kInsufficientBudget:
      return WebUITriggerStatus::kInsufficientBudget;
    case AggregatableStatus::kNoMatchingSourceFilterData:
      return WebUITriggerStatus::kNoMatchingSourceFilterData;
    case AggregatableStatus::kNotRegistered:
      return WebUITriggerStatus::kNotRegistered;
    case AggregatableStatus::kProhibitedByBrowserPolicy:
      return WebUITriggerStatus::kProhibitedByBrowserPolicy;
    case AggregatableStatus::kDeduplicated:
      return WebUITriggerStatus::kDeduplicated;
    case AggregatableStatus::kReportWindowPassed:
      return WebUITriggerStatus::kReportWindowPassed;
  }
}

}  // namespace

void AttributionInternalsHandlerImpl::OnTriggerHandled(
    const AttributionTrigger& trigger,
    const absl::optional<uint64_t> cleared_debug_key,
    const CreateReportResult& result) {
  const attribution_reporting::TriggerRegistration& registration =
      trigger.registration();

  auto web_ui_trigger = attribution_internals::mojom::WebUITrigger::New();
  web_ui_trigger->registration =
      GetRegistration(result.trigger_time(), trigger.destination_origin(),
                      trigger.reporting_origin(),
                      SerializeAttributionJson(registration.ToJson(),
                                               /*pretty_print=*/true),
                      cleared_debug_key);
  web_ui_trigger->event_level_status =
      GetWebUITriggerStatus(result.event_level_status());
  web_ui_trigger->aggregatable_status =
      GetWebUITriggerStatus(result.aggregatable_status());
  web_ui_trigger->attestation = trigger.attestation();

  observer_->OnTriggerHandled(std::move(web_ui_trigger));

  if (const absl::optional<AttributionReport>& report =
          result.replaced_event_level_report()) {
    DCHECK_EQ(
        result.event_level_status(),
        AttributionTrigger::EventLevelResult::kSuccessDroppedLowerPriority);
    DCHECK(result.new_event_level_report().has_value());

    observer_->OnReportDropped(
        WebUIReport(*report, /*is_debug_report=*/false,
                    ReportStatus::NewReplacedByHigherPriorityReport(
                        result.new_event_level_report()
                            ->external_report_id()
                            .AsLowercaseString())));
  }
}

void AttributionInternalsHandlerImpl::OnObserverDisconnected() {
  manager_observation_.Reset();
}

}  // namespace content
