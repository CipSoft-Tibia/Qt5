// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/common/quota/padding_key.h"

#include <inttypes.h>
#include <cstdint>
#include <vector>
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "crypto/hmac.h"
#include "crypto/random.h"
#include "crypto/symmetric_key.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/mojom/url_response_head.mojom-shared.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_canon.h"
#include "url/url_constants.h"
#include "url/url_util.h"

using crypto::SymmetricKey;

namespace storage {

namespace {

const SymmetricKey::Algorithm kPaddingKeyAlgorithm = SymmetricKey::AES;

// The range of the padding added to response sizes for opaque resources.
// Increment the CacheStorage padding version if changed.
constexpr uint64_t kPaddingRange = 14431 * 1024;

std::unique_ptr<SymmetricKey>* GetPaddingKeyInternal() {
  static base::NoDestructor<std::unique_ptr<SymmetricKey>> s_padding_key([] {
    return SymmetricKey::GenerateRandomKey(kPaddingKeyAlgorithm, 128);
  }());
  return s_padding_key.get();
}

static bool IsStandardSchemeWithNetworkHost(base::StringPiece scheme) {
  // file scheme is special. Windows file share origins can have network hosts.
  if (scheme == url::kFileScheme)
    return true;

  url::SchemeType scheme_type;
  if (!url::GetStandardSchemeType(
          scheme.data(), url::Component(0, scheme.length()), &scheme_type)) {
    return false;
  }
  return scheme_type == url::SCHEME_WITH_HOST_PORT_AND_USER_INFORMATION ||
         scheme_type == url::SCHEME_WITH_HOST_AND_PORT;
}


static url::Origin SchemefuleSiteReplacement(
    const url::Origin& origin) {
  // 1. If origin is an opaque origin, then return origin.
  if (origin.opaque())
    return origin;

  std::string registerable_domain;

  // Non-normative step.
  // We only lookup the registerable domain for schemes with network hosts, this
  // is non-normative. Other schemes for non-opaque origins do not
  // meaningfully have a registerable domain for their host, so they are
  // skipped.
  if (IsStandardSchemeWithNetworkHost(origin.scheme())) {
    registerable_domain = GetDomainAndRegistry(
        origin, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  }

  // If origin's host's registrable domain is null, then return (origin's
  // scheme, origin's host).
  //
  // `GetDomainAndRegistry()` returns an empty string for IP literals and
  // effective TLDs.
  //
  // Note that `registerable_domain` could still end up empty, since the
  // `origin` might have a scheme that permits empty hostnames, such as "file".
  bool used_registerable_domain = !registerable_domain.empty();
  if (!used_registerable_domain)
    registerable_domain = origin.host();

  int port = url::DefaultPortForScheme(origin.scheme().c_str(),
                                       origin.scheme().length());

  // Provide a default port of 0 for non-standard schemes.
  if (port == url::PORT_UNSPECIFIED)
    port = 0;

  return url::Origin::CreateFromNormalizedTuple(origin.scheme(),
                                                     registerable_domain, port);
}

}  // namespace

bool ShouldPadResponseType(network::mojom::FetchResponseType type) {
  return type == network::mojom::FetchResponseType::kOpaque ||
         type == network::mojom::FetchResponseType::kOpaqueRedirect;
}

int64_t ComputeRandomResponsePadding() {
  uint64_t raw_random = 0;
  crypto::RandBytes(&raw_random, sizeof(uint64_t));
  return raw_random % kPaddingRange;
}

int64_t ComputeStableResponsePadding(const url::Origin& origin,
                                     const std::string& response_url,
                                     const base::Time& response_time,
                                     const std::string& request_method,
                                     int64_t side_data_size) {
  DCHECK(!response_url.empty());

  url::Origin site = SchemefuleSiteReplacement(origin);

  DCHECK_GT(response_time, base::Time::UnixEpoch());
  int64_t microseconds =
      (response_time - base::Time::UnixEpoch()).InMicroseconds();

  // It should only be possible to have a CORS safelisted method here since
  // the spec does not permit other methods for no-cors requests.
  DCHECK(request_method == net::HttpRequestHeaders::kGetMethod ||
         request_method == net::HttpRequestHeaders::kHeadMethod ||
         request_method == net::HttpRequestHeaders::kPostMethod);

  std::string key = base::StringPrintf(
      "%s-%" PRId64 "-%s-%s-%" PRId64, response_url.c_str(), microseconds,
      site.Serialize().c_str(), request_method.c_str(), side_data_size);

  crypto::HMAC hmac(crypto::HMAC::SHA256);
  CHECK(hmac.Init(GetPaddingKeyInternal()->get()));

  uint64_t digest_start = 0;
  CHECK(hmac.Sign(key, reinterpret_cast<uint8_t*>(&digest_start),
                  sizeof(digest_start)));
  return digest_start % kPaddingRange;
}

}  // namespace storage
