// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/features.h"

#include <string>

#include "base/feature_list.h"
#include "build/build_config.h"

namespace features {

// When enabled, this forces composited textures for SurfaceLayerImpls to be
// aligned to the pixel grid. Lack of alignment can lead to blur, noticeably so
// in text. https://crbug.com/359279545
BASE_FEATURE(kAlignSurfaceLayerImplToPixelGrid,
             "AlignSurfaceLayerImplToPixelGrid",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kNaturalScrollingPersonality,
             "NaturalScrollingPersonality",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsNaturalScrollAnimationEnabled() {
  return base::FeatureList::IsEnabled(features::kNaturalScrollingPersonality);
}

// Whether the compositor should attempt to sync with the scroll handlers before
// submitting a frame.
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
BASE_FEATURE(kSynchronizedScrolling,
             "SynchronizedScrolling",
             base::FEATURE_DISABLED_BY_DEFAULT);
#else
BASE_FEATURE(kSynchronizedScrolling,
             "SynchronizedScrolling",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

BASE_FEATURE(kMainRepaintScrollPrefersNewContent,
             "MainRepaintScrollPrefersNewContent",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kDeferImplInvalidation,
             "DeferImplInvalidation",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kDeferImplInvalidationFrames{
    &kDeferImplInvalidation, "frames", 1};

// Note that kUseDMSAAForTiles only controls vulkan launch on android. We will
// be using a separate flag to control the launch on GL.

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_ANDROID)
BASE_FEATURE(kUseDMSAAForTiles,
             "UseDMSAAForTiles",
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
BASE_FEATURE(kUseDMSAAForTiles,
             "UseDMSAAForTiles",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS)
BASE_FEATURE(kUIEnableSharedImageCacheForGpu,
             "UIEnableSharedImageCacheForGpu",
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
BASE_FEATURE(kUIEnableSharedImageCacheForGpu,
             "UIEnableSharedImageCacheForGpu",
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

BASE_FEATURE(kReclaimResourcesDelayedFlushInBackground,
             "ReclaimResourcesDelayedFlushInBackground",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kDetectHiDpiForMsaa,
             "DetectHiDpiForMsaa",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kReclaimPrepaintTilesWhenIdle,
             "ReclaimPrepaintTilesWhenIdle",
             base::FEATURE_DISABLED_BY_DEFAULT);

// This saves memory on all platforms, but while on Android savings are
// significant (~10MiB or more of foreground memory), on desktop they were
// small, so only enable on Android.
//
// Disabled 04/2024 as it regresses checkerboarding metrics. Feature kept around
// to find a better balance between checkerboarding and memory.
BASE_FEATURE(kSmallerInterestArea,
             "SmallerInterestArea",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kInterestAreaSizeInPixels{
    &kSmallerInterestArea, "size_in_pixels", kDefaultInterestAreaSizeInPixels};

BASE_FEATURE(kReclaimOldPrepaintTiles,
             "ReclaimOldPrepaintTiles",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kReclaimDelayInSeconds{&kSmallerInterestArea,
                                                     "reclaim_delay_s", 30};

BASE_FEATURE(kUseMapRectForPixelMovement,
             "UseMapRectForPixelMovement",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kEvictionThrottlesDraw,
             "EvictionThrottlesDraw",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kAdjustFastMainThreadThreshold,
             "AdjustFastMainThreadThreshold",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kClearCanvasResourcesInBackground,
             "ClearCanvasResourcesInBackground",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kMetricsTracingCalculationReduction,
             "MetricsTracingCalculationReduction",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kWaitForLateScrollEvents,
             "WaitForLateScrollEvents",
             base::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<double> kWaitForLateScrollEventsDeadlineRatio{
    &kWaitForLateScrollEvents, "deadline_ratio", 0.333};

BASE_FEATURE(kNonBatchedCopySharedImage,
             "NonBatchedCopySharedImage",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kDontAlwaysPushPictureLayerImpls,
             "DontAlwaysPushPictureLayerImpls",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kPreserveDiscardableImageMapQuality,
             "PreserveDiscardableImageMapQuality",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kWarmUpCompositor,
             "WarmUpCompositor",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kCCSlimming, "CCSlimming", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsCCSlimmingEnabled() {
  static const bool enabled = base::FeatureList::IsEnabled(kCCSlimming);
  return enabled;
}

constexpr const char kScrollEventDispatchModeDispatchScrollEventsImmediately[] =
    "DispatchScrollEventsImmediately";
constexpr const char kScrollEventDispatchModeUseScrollPredictorForEmptyQueue[] =
    "UseScrollPredictorForEmptyQueue";
constexpr const char kScrollEventDispatchModeUseScrollPredictorForDeadline[] =
    "UseScrollPredictorForDeadline";
constexpr const char
    kScrollEventDispatchModeDispatchScrollEventsUntilDeadline[] =
        "DispatchScrollEventsUntilDeadline";
const base::FeatureParam<std::string> kScrollEventDispatchMode(
    &kWaitForLateScrollEvents,
    "mode",
    kScrollEventDispatchModeDispatchScrollEventsImmediately);

BASE_FEATURE(kTreesInViz, "TreesInViz", base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kTreeAnimationsInViz,
             "kTreeAnimationsInViz",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSendExplicitDecodeRequestsImmediately,
             "SendExplicitDecodeRequestsImmediately",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kThrottleFrameRateOnManyDidNotProduceFrame,
             "ThrottleFrameRateOnManyDidNotProduceFrame",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNewContentForCheckerboardedScrolls,
             "NewContentForCheckerboardedScrolls",
             base::FEATURE_DISABLED_BY_DEFAULT);

// By default, frame rate starts being throttled when 4 consecutive "did not
// produce frame" are observed. It stops being throttled when there's a drawn
// frame.
const base::FeatureParam<int> kNumDidNotProduceFrameBeforeThrottle{
    &kThrottleFrameRateOnManyDidNotProduceFrame,
    "num_did_not_produce_frame_before_throttle", 4};

BASE_FEATURE(kMultipleImplOnlyScrollAnimations,
             "MultipleImplOnlyScrollAnimations",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool MultiImplOnlyScrollAnimationsSupported() {
  return base::FeatureList::IsEnabled(
      features::kMultipleImplOnlyScrollAnimations);
}

BASE_FEATURE(kPreventDuplicateImageDecodes,
             "PreventDuplicateImageDecodes",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kInitImageDecodeLastUseTime,
             "InitImageDecodeLastUseTime",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kDynamicSafeAreaInsetsSupportedByCC,
             "DynamicSafeAreaInsetsSupportedByCC",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kThrottleMainFrameTo60Hz,
             "ThrottleMainFrameTo60Hz",
             base::FEATURE_DISABLED_BY_DEFAULT);

// When enabled, this flag stops the export of most of the
// UKMs calculated by the DroppedFrameCounter.
BASE_FEATURE(kStopExportDFCMetrics,
             "StopExportDFCMetrics",
             base::FEATURE_DISABLED_BY_DEFAULT);
bool StopExportDFCMetrics() {
  return base::FeatureList::IsEnabled(features::kStopExportDFCMetrics);
}

BASE_FEATURE(kZeroScrollMetricsUpdate,
             "ZeroScrollMetricsUpdate",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
