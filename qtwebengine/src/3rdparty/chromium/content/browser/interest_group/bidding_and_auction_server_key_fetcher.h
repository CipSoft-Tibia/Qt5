// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INTEREST_GROUP_BIDDING_AND_AUCTION_SERVER_KEY_FETCHER_H_
#define CONTENT_BROWSER_INTEREST_GROUP_BIDDING_AND_AUCTION_SERVER_KEY_FETCHER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "base/functional/callback.h"
#include "content/common/content_export.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace network {
class SimpleURLLoader;

namespace mojom {
class URLLoaderFactory;
}  // namespace mojom
}  // namespace network

namespace content {

struct BiddingAndAuctionServerKey {
  std::string key;  // bytes containing the key.
  uint8_t id;       // key id corresponding to this key.
};

class CONTENT_EXPORT BiddingAndAuctionServerKeyFetcher {
 public:
  using BiddingAndAuctionServerKeyFetcherCallback =
      base::OnceCallback<void(absl::optional<BiddingAndAuctionServerKey>)>;

  BiddingAndAuctionServerKeyFetcher();
  ~BiddingAndAuctionServerKeyFetcher();
  // no copy
  BiddingAndAuctionServerKeyFetcher(const BiddingAndAuctionServerKeyFetcher&) =
      delete;
  BiddingAndAuctionServerKeyFetcher& operator=(
      const BiddingAndAuctionServerKeyFetcher&) = delete;

  // GetOrFetchKey provides a key in the callback, fetching the key over the
  // network with the provided loader_factory if necessary. If the key is
  // immediately available then the callback may be called synchronously.
  void GetOrFetchKey(network::mojom::URLLoaderFactory* loader_factory,
                     BiddingAndAuctionServerKeyFetcherCallback callback);

 private:
  // Called when the JSON blob containing the keys have been successfully
  // fetched over the network.
  void OnFetchKeyComplete(std::unique_ptr<std::string> response);

  // Called when the JSON blob containing the keys has be parsed into
  // base::Values. Uses the parsed result to add keys to the cache and calls
  // queued callbacks.
  void OnParsedKeys(data_decoder::DataDecoder::ValueOrError result);
  void FailAllCallbacks();

  // queue_ contains callbacks waiting for a key to be fetched over the network.
  base::circular_deque<BiddingAndAuctionServerKeyFetcherCallback> queue_;

  // keys_ contains a list of keys received from the server (if any).
  std::vector<BiddingAndAuctionServerKey> keys_;

  // expiration_ contains the expiration time for any keys that are cached by
  // this object.
  base::Time expiration_ = base::Time::Min();

  // loader_ contains the SimpleURLLoader being used to fetch the keys.
  std::unique_ptr<network::SimpleURLLoader> loader_;

  base::WeakPtrFactory<BiddingAndAuctionServerKeyFetcher> weak_ptr_factory_{
      this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_INTEREST_GROUP_BIDDING_AND_AUCTION_SERVER_KEY_FETCHER_H_
