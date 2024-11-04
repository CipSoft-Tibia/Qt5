// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_CONSTANTS_H_
#define DISCOVERY_DNSSD_IMPL_CONSTANTS_H_

#include <string>
#include <utility>

#include "discovery/mdns/public/mdns_constants.h"
#include "discovery/mdns/public/mdns_records.h"

namespace openscreen::discovery {

// This is the DNS Information required to start a new query.
struct DnsQueryInfo {
  DomainName name;
  DnsType dns_type;
  DnsClass dns_class;
};

}  // namespace openscreen::discovery

#endif  // DISCOVERY_DNSSD_IMPL_CONSTANTS_H_
