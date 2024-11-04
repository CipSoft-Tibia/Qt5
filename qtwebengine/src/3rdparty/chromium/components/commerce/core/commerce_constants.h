// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_COMMERCE_CORE_COMMERCE_CONSTANTS_H_
#define COMPONENTS_COMMERCE_CORE_COMMERCE_CONSTANTS_H_

namespace commerce {

// The host for the commerce internals page.
extern const char kChromeUICommerceInternalsHost[];

// The host for the shopping insights side panel page.
extern const char kChromeUIShoppingInsightsSidePanelHost[];

// The url for the shopping insights side panel page.
extern const char kChromeUIShoppingInsightsSidePanelUrl[];

// Please do not use below UTM constants beyond commerce use cases.
// UTM source label.
extern const char kUTMSourceLabel[];

// UTM medium label.
extern const char kUTMMediumLabel[];

// UTM campaign label.
extern const char kUTMCampaignLabel[];

// General UTM source value.
extern const char kUTMSourceValue[];

// General UTM medium value.
extern const char kUTMMediumValue[];

// UTM campaign value for discounts in history clusters.
extern const char kUTMCampaignValueForDiscounts[];

// Prefix of UTM labels, including the underscore.
extern const char kUTMPrefix[];
}  // namespace commerce

#endif  // COMPONENTS_COMMERCE_CORE_COMMERCE_CONSTANTS_H_
