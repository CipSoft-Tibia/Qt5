// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_PUBLIC_MDNS_DOMAIN_CONFIRMED_PROVIDER_H_
#define DISCOVERY_MDNS_PUBLIC_MDNS_DOMAIN_CONFIRMED_PROVIDER_H_

#include "discovery/mdns/public/mdns_records.h"

namespace openscreen::discovery {

class MdnsDomainConfirmedProvider {
 public:
  virtual ~MdnsDomainConfirmedProvider() = default;

  // Called once the probing phase has been completed, and a DomainName has
  // been confirmed. The callee is expected to register records for the
  // newly confirmed name in this callback. Note that the requested name and
  // the confirmed name may differ if conflict resolution has occurred.
  virtual void OnDomainFound(const DomainName& requested_name,
                             const DomainName& confirmed_name) = 0;
};

}  // namespace openscreen::discovery

#endif  // DISCOVERY_MDNS_PUBLIC_MDNS_DOMAIN_CONFIRMED_PROVIDER_H_
