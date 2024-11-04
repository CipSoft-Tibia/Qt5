// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_COMMERCE_CORE_COMMERCE_TYPES_H_
#define COMPONENTS_COMMERCE_CORE_COMMERCE_TYPES_H_

#include <string>
#include <vector>

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace commerce {

// Data containers that are provided by the above callbacks:

// Discount cluster types.
enum class DiscountClusterType {
  kUnspecified = 0,
  kOfferLevel = 1,
  kMaxValue = kOfferLevel,
};

// Discount types.
enum class DiscountType {
  kUnspecified = 0,
  kFreeListingWithCode = 1,
  kMaxValue = kFreeListingWithCode,
};

// Information returned by the discount APIs.
struct DiscountInfo {
  DiscountInfo();
  DiscountInfo(const DiscountInfo&);
  DiscountInfo& operator=(const DiscountInfo&);
  ~DiscountInfo();

  DiscountClusterType cluster_type;
  DiscountType type;
  std::string language_code;
  std::string description_detail;
  absl::optional<std::string> terms_and_conditions;
  std::string value_in_text;
  absl::optional<std::string> discount_code;
  uint64_t id = 0;
  bool is_merchant_wide;
  double expiry_time_sec;
  uint64_t offer_id = 0;
};

// Information returned by the merchant info APIs.
struct MerchantInfo {
  MerchantInfo();
  MerchantInfo(const MerchantInfo&);
  MerchantInfo& operator=(const MerchantInfo&);
  MerchantInfo(MerchantInfo&&);
  MerchantInfo& operator=(MerchantInfo&&) = default;
  ~MerchantInfo();

  float star_rating;
  uint32_t count_rating;
  GURL details_page_url;
  bool has_return_policy;
  float non_personalized_familiarity_score;
  bool contains_sensitive_content;
  bool proactive_message_disabled;
};

// Position of current price with respect to the typical price range.
enum class PriceBucket {
  kUnknown = 0,
  kLowPrice = 1,
  kTypicalPrice = 2,
  kHighPrice = 3,
  kMaxValue = kHighPrice,
};

// Information returned by the price insights APIs.
struct PriceInsightsInfo {
  PriceInsightsInfo();
  PriceInsightsInfo(const PriceInsightsInfo&);
  PriceInsightsInfo& operator=(const PriceInsightsInfo&);
  ~PriceInsightsInfo();

  absl::optional<uint64_t> product_cluster_id;
  std::string currency_code;
  absl::optional<int64_t> typical_low_price_micros;
  absl::optional<int64_t> typical_high_price_micros;
  absl::optional<std::string> catalog_attributes;
  std::vector<std::tuple<std::string, int64_t>> catalog_history_prices;
  absl::optional<GURL> jackpot_url;
  PriceBucket price_bucket;
  bool has_multiple_catalogs;
};

// Information returned by the product info APIs.
struct ProductInfo {
 public:
  ProductInfo();
  ProductInfo(const ProductInfo&);
  ProductInfo& operator=(const ProductInfo&);
  ~ProductInfo();

  std::string title;
  std::string product_cluster_title;
  GURL image_url;
  absl::optional<uint64_t> product_cluster_id;
  absl::optional<uint64_t> offer_id;
  std::string currency_code;
  int64_t amount_micros{0};
  absl::optional<int64_t> previous_amount_micros;
  std::string country_code;

 private:
  friend class ShoppingService;

  // This is used to track whether the server provided an image with the rest
  // of the product info. This value being |true| does not necessarily mean an
  // image is available in the ProductInfo struct (as it is flag gated) and is
  // primarily used for recording metrics.
  bool server_image_available{false};
};

// Callbacks and typedefs for various accessors in the shopping service.
using DiscountsMap = std::map<GURL, std::vector<DiscountInfo>>;
using DiscountInfoCallback = base::OnceCallback<void(const DiscountsMap&)>;
using MerchantInfoCallback =
    base::OnceCallback<void(const GURL&, absl::optional<MerchantInfo>)>;
using PriceInsightsInfoCallback =
    base::OnceCallback<void(const GURL&,
                            const absl::optional<PriceInsightsInfo>&)>;
using ProductInfoCallback =
    base::OnceCallback<void(const GURL&,
                            const absl::optional<const ProductInfo>&)>;
using IsShoppingPageCallback =
    base::OnceCallback<void(const GURL&, absl::optional<bool>)>;

}  // namespace commerce

#endif  // COMPONENTS_COMMERCE_CORE_COMMERCE_TYPES_H_
