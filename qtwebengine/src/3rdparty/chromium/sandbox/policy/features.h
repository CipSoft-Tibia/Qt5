// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the content
// module.

#ifndef SANDBOX_POLICY_FEATURES_H_
#define SANDBOX_POLICY_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"
#include "sandbox/policy/export.h"

namespace sandbox {
namespace policy {
namespace features {

#if defined(TOOLKIT_QT) || !defined(OS_MAC)
SANDBOX_POLICY_EXPORT extern const base::Feature kNetworkServiceSandbox;
#endif

#if defined(OS_WIN)
SANDBOX_POLICY_EXPORT extern const base::Feature kWinSboxDisableExtensionPoints;
SANDBOX_POLICY_EXPORT extern const base::Feature kGpuAppContainer;
SANDBOX_POLICY_EXPORT extern const base::Feature kGpuLPAC;
#endif  // defined(OS_WIN)

#if !defined(OS_ANDROID)
SANDBOX_POLICY_EXPORT extern const base::Feature kXRSandbox;
#endif  // !defined(OS_ANDROID)

}  // namespace features
}  // namespace policy
}  // namespace sandbox

#endif  // SANDBOX_POLICY_FEATURES_H_
