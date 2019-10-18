// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/site_for_cookies_mojom_traits.h"
#include "net/base/features.h"
#include "services/network/public/cpp/crash_keys.h"

namespace mojo {

bool StructTraits<network::mojom::SiteForCookiesDataView, net::SiteForCookies>::
    Read(network::mojom::SiteForCookiesDataView data,
         net::SiteForCookies* out) {
  std::string scheme, registrable_domain, first_party_url;
  if (!data.ReadScheme(&scheme)) {
    return false;
  }
  if (!data.ReadRegistrableDomain(&registrable_domain)) {
    return false;
  }

  if (!data.ReadFirstPartyUrl(&first_party_url)) {
    return false;
  }

  bool result = net::SiteForCookies::FromWire(scheme, registrable_domain,
                                              data.schemefully_same(),
                                              GURL(first_party_url), out);
  if (!result) {
    network::debug::SetDeserializationCrashKeyString("site_for_cookie");
  }
  return result;
}

}  // namespace mojo
