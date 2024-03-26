// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/core/common/features.h"

#include <stddef.h>
#include <algorithm>
#include <utility>
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/system/sys_info.h"
#include "base/values.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "components/safe_browsing/buildflags.h"
#include "components/variations/variations_associated_data.h"

namespace safe_browsing {
// Please define any new SafeBrowsing related features in this file, and add
// them to the ExperimentalFeaturesList below to start displaying their status
// on the chrome://safe-browsing page.

BASE_FEATURE(kAdSamplerTriggerFeature,
             "SafeBrowsingAdSamplerTrigger",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kAddPageLoadTokenToClientSafeBrowsingReport,
             "AddPageLoadTokenToClientSafeBrowsingReport",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kClientSideDetectionKillswitch,
             "ClientSideDetectionKillswitch",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kClientSideDetectionModelIsFlatBuffer,
             "ClientSideDetectionModelIsFlatBuffer",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kClientSideDetectionModelTag,
             "ClientSideDetectionTag",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kClientSideDetectionReferrerChain,
             "ClientSideDetectionReferrerChain",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kClientSideDetectionTypeForceRequest,
             "ClientSideDetectionTypeForceRequest",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kComponentUpdaterAndroidProtegoAllowlist,
             "SafeBrowsingComponentUpdaterAndroidProtegoAllowlist",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kConnectorsScanningAccessToken,
             "ConnectorsScanningAccessToken",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kConnectorsScanningReportOnlyUI,
             "ConnectorsScanningReportOnlyUI",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kDelayedWarnings,
             "SafeBrowsingDelayedWarnings",
             base::FEATURE_DISABLED_BY_DEFAULT);

// If true, a delayed warning will be shown when the user clicks on the page.
// If false, the warning won't be shown, but a metric will be recorded on the
// first click.
const base::FeatureParam<bool> kDelayedWarningsEnableMouseClicks{
    &kDelayedWarnings, "mouse",
    /*default_value=*/false};

BASE_FEATURE(kDownloadBubble,
             "DownloadBubble",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kDownloadBubbleV2,
             "DownloadBubbleV2",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kDownloadTailoredWarnings,
             "DownloadTailoredWarnings",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kEsbIphBubbleAndCollapseSettings,
             "EsbIphBubbleAndCollapseSettings",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<bool> kEsbIphBubbleAndCollapseSettingsEnableIph{
    &kEsbIphBubbleAndCollapseSettings, "EnableEsbIphBubble", false};

const base::FeatureParam<bool> kEsbIphBubbleAndCollapseSettingsEnableCollapse{
    &kEsbIphBubbleAndCollapseSettings, "EnableEsbSettingCollapse", false};

BASE_FEATURE(kExtensionTelemetry,
             "SafeBrowsingExtensionTelemetry",
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<int> kExtensionTelemetryUploadIntervalSeconds{
    &kExtensionTelemetry, "UploadIntervalSeconds",
    /*default_value=*/3600};

const base::FeatureParam<int> kExtensionTelemetryWritesPerInterval{
    &kExtensionTelemetry, "NumberOfWritesInInterval",
    /*default_value=*/1};

BASE_FEATURE(kExtensionTelemetryCookiesGetAllSignal,
             "SafeBrowsingExtensionTelemetryCookiesGetAllSignal",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryPersistence,
             "SafeBrowsingExtensionTelemetryPersistence",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryConfiguration,
             "SafeBrowsingExtensionTelemetryConfiguration",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryFileData,
             "SafeBrowsingExtensionTelemetryFileData",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kExtensionTelemetryFileDataMaxFilesToProcess{
    &kExtensionTelemetryFileData, "MaxFilesToProcess",
    /*default_value=*/50};

const base::FeatureParam<int> kExtensionTelemetryFileDataMaxFileSizeBytes{
    &kExtensionTelemetryFileData, "MaxFileSizeBytes",
    /*default_value=100KB*/ 100 * 1024};

const base::FeatureParam<int>
    kExtensionTelemetryFileDataCollectionIntervalSeconds{
        &kExtensionTelemetryFileData, "CollectionIntervalSeconds",
        /*default_value=*/7200};

const base::FeatureParam<int> kExtensionTelemetryFileDataStartupDelaySeconds{
    &kExtensionTelemetryFileData, "StartupDelaySeconds",
    /*default_value=*/300};

BASE_FEATURE(kExtensionTelemetryPotentialPasswordTheft,
             "SafeBrowsingExtensionTelemetryPotentialPasswordTheft",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryReportContactedHosts,
             "SafeBrowsingExtensionTelemetryReportContactedHosts",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryReportHostsContactedViaWebSocket,
             "SafeBrowsingExtensionTelemetryReportHostsContactedViaWebsocket",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryTabsExecuteScriptSignal,
             "SafeBrowsingExtensionTelemetryTabsExecuteScriptSignal",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kExtensionTelemetryCookiesGetSignal,
             "SafeBrowsingExtensionTelemetryCookiesGetSignal",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kFileTypePoliciesTag,
             "FileTypePoliciesTag",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kLogAccountEnhancedProtectionStateInProtegoPings,
             "TailoredSecurityLogAccountEnhancedProtectionStateInProtegoPings",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kMmapSafeBrowsingDatabase,
             "MmapSafeBrowsingDatabase",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNestedArchives,
             "SafeBrowsingArchiveImprovements",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kOmitNonUserGesturesFromReferrerChain,
             "SafeBrowsingOmitNonUserGesturesFromReferrerChain",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kRealTimeUrlFilteringForEnterprise,
             "RealTimeUrlFilteringForEnterprise",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kRealTimeUrlLookupForEnterpriseAllowlistBypass,
             "SafeBrowsingRealTimeUrlLookupForEnterpriseAllowlistBypass",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSafeBrowsingCsbrrNewDownloadTrigger,
             "SafeBrowsingCsbrrNewDownloadTrigger",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSafeBrowsingDisableConsumerCsdForEnterprise,
             "SafeBrowsingDisableConsumerCsdForEnterprise",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSafeBrowsingEnterpriseCsd,
             "SafeBrowsingEnterpriseCsd",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSafeBrowsingLookupMechanismExperiment,
             "SafeBrowsingLookupMechanismExperiment",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSafeBrowsingRemoveCookiesInAuthRequests,
             "SafeBrowsingRemoveCookiesInAuthRequests",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSevenZipEvaluationEnabled,
             "SafeBrowsingSevenZipEvaluationEnabled",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSimplifiedUrlDisplay,
             "SimplifiedUrlDisplay",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kStrictDownloadTimeout,
             "SafeBrowsingStrictDownloadtimeout",
             base::FEATURE_DISABLED_BY_DEFAULT);

constexpr base::FeatureParam<int> kStrictDownloadTimeoutMilliseconds{
    &kStrictDownloadTimeout, "TimeoutMilliseconds",
    /*default_value=*/7000};

BASE_FEATURE(kSuspiciousSiteTriggerQuotaFeature,
             "SafeBrowsingSuspiciousSiteTriggerQuota",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kTailoredSecurityDesktopNotice,
             "TailoredSecurityDesktopNotice",
             base::FEATURE_ENABLED_BY_DEFAULT);

#if BUILDFLAG(IS_ANDROID)
BASE_FEATURE(kTailoredSecurityDialogRetryMechanism,
             "TailoredSecurityDialogRetryMechanism",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

CONSTINIT const base::Feature kTailoredSecurityIntegration(
             "TailoredSecurityIntegration",
#if BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kThreatDomDetailsTagAndAttributeFeature,
             "ThreatDomDetailsTagAttributes",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kVisualFeaturesForReusePings,
             "VisualFeaturesInReusePings",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kVisualFeaturesSizes,
             "VisualFeaturesSizes",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kClientSideDetectionModelOptimizationGuide,
             "ClientSideDetectionModelOptimizationGuide",
             base::FEATURE_DISABLED_BY_DEFAULT);

namespace {
// List of Safe Browsing features. Boolean value for each list member should
// be set to true if the experiment state should be listed on
// chrome://safe-browsing. Features should be listed in alphabetical order.
constexpr struct {
  const base::Feature* feature;
  // True if the feature's state should be listed on chrome://safe-browsing.
  bool show_state;
} kExperimentalFeatures[]{
    {&kAdSamplerTriggerFeature, false},
    {&kAddPageLoadTokenToClientSafeBrowsingReport, false},
    {&kClientSideDetectionKillswitch, true},
    {&kClientSideDetectionModelOptimizationGuide, true},
    {&kClientSideDetectionModelIsFlatBuffer, true},
    {&kClientSideDetectionReferrerChain, true},
    {&kClientSideDetectionTypeForceRequest, true},
    {&kComponentUpdaterAndroidProtegoAllowlist, true},
    {&kConnectorsScanningAccessToken, true},
    {&kConnectorsScanningReportOnlyUI, true},
    {&kDelayedWarnings, true},
    {&kDownloadBubble, true},
    {&kDownloadBubbleV2, true},
    {&kDownloadTailoredWarnings, true},
    {&kExtensionTelemetry, true},
    {&kExtensionTelemetryCookiesGetAllSignal, true},
    {&kExtensionTelemetryCookiesGetSignal, true},
    {&kExtensionTelemetryPersistence, true},
    {&kExtensionTelemetryPotentialPasswordTheft, true},
    {&kExtensionTelemetryReportContactedHosts, true},
    {&kExtensionTelemetryReportHostsContactedViaWebSocket, true},
    {&kExtensionTelemetryTabsExecuteScriptSignal, true},
    {&kFileTypePoliciesTag, true},
    {&kLogAccountEnhancedProtectionStateInProtegoPings, true},
    {&kMmapSafeBrowsingDatabase, true},
    {&kNestedArchives, true},
    {&kOmitNonUserGesturesFromReferrerChain, true},
    {&kRealTimeUrlFilteringForEnterprise, true},
    {&kRealTimeUrlLookupForEnterpriseAllowlistBypass, true},
    {&kSafeBrowsingCsbrrNewDownloadTrigger, true},
    {&kSafeBrowsingDisableConsumerCsdForEnterprise, true},
    {&kSafeBrowsingEnterpriseCsd, true},
    {&kSafeBrowsingLookupMechanismExperiment, true},
    {&kSafeBrowsingRemoveCookiesInAuthRequests, true},
    {&kSevenZipEvaluationEnabled, true},
    {&kSimplifiedUrlDisplay, true},
    {&kStrictDownloadTimeout, true},
    {&kSuspiciousSiteTriggerQuotaFeature, true},
    {&kTailoredSecurityDesktopNotice, true},
    {&kTailoredSecurityIntegration, true},
    {&kThreatDomDetailsTagAndAttributeFeature, false},
    {&kVisualFeaturesForReusePings, true},
    {&kVisualFeaturesSizes, true},
};

// Adds the name and the enabled/disabled status of a given feature.
void AddFeatureAndAvailability(const base::Feature* exp_feature,
                               base::Value::List* param_list) {
  param_list->Append(exp_feature->name);
  if (base::FeatureList::IsEnabled(*exp_feature)) {
    param_list->Append("Enabled");
  } else {
    param_list->Append("Disabled");
  }
}
}  // namespace

// Returns the list of the experimental features that are enabled or disabled,
// as part of currently running Safe Browsing experiments.
base::Value::List GetFeatureStatusList() {
  base::Value::List param_list;
  for (const auto& feature_status : kExperimentalFeatures) {
    if (feature_status.show_state) {
      AddFeatureAndAvailability(feature_status.feature, &param_list);
    }
  }

  // Manually add experimental features that we want param values for.
  param_list.Append(variations::GetVariationParamValueByFeature(
      safe_browsing::kClientSideDetectionModelTag,
      kClientSideDetectionTagParamName));
  param_list.Append(kClientSideDetectionModelTag.name);
  param_list.Append(variations::GetVariationParamValueByFeature(
      kFileTypePoliciesTag, kFileTypePoliciesTagParamName));
  param_list.Append(kFileTypePoliciesTag.name);

  return param_list;
}

std::string GetClientSideDetectionTag() {
  if (base::FeatureList::IsEnabled(
          safe_browsing::kClientSideDetectionModelTag)) {
    return variations::GetVariationParamValueByFeature(
        safe_browsing::kClientSideDetectionModelTag,
        kClientSideDetectionTagParamName);
  }

#if BUILDFLAG(IS_ANDROID)
  return "android_1";
#else
  return "desktop_1_flatbuffer";
#endif
}

std::string GetFileTypePoliciesTag() {
  if (!base::FeatureList::IsEnabled(kFileTypePoliciesTag)) {
    return "default";
  }
  std::string tag_value = variations::GetVariationParamValueByFeature(
      kFileTypePoliciesTag, kFileTypePoliciesTagParamName);

  return tag_value.empty() ? "default" : tag_value;
}

}  // namespace safe_browsing
