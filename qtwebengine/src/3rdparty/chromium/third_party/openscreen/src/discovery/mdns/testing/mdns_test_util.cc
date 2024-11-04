// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/testing/mdns_test_util.h"

#include <string>
#include <utility>
#include <vector>

namespace openscreen::discovery {

TxtRecordRdata MakeTxtRecord(std::initializer_list<std::string_view> strings) {
  return TxtRecordRdata(MakeTxtEntries(strings));
}

std::vector<TxtRecordRdata::Entry> MakeTxtEntries(
    std::initializer_list<std::string_view> strings) {
  std::vector<TxtRecordRdata::Entry> texts;
  for (const auto& string : strings) {
    texts.push_back(TxtRecordRdata::Entry(string.begin(), string.end()));
  }
  return texts;
}

MdnsRecord GetFakePtrRecord(const DomainName& target,
                            std::chrono::seconds ttl) {
  DomainName name(++target.labels().begin(), target.labels().end());
  PtrRecordRdata rdata(target);
  return MdnsRecord(std::move(name), DnsType::kPTR, DnsClass::kIN,
                    RecordType::kShared, ttl, std::move(rdata));
}

MdnsRecord GetFakeSrvRecord(const DomainName& name, std::chrono::seconds ttl) {
  return GetFakeSrvRecord(name, name, ttl);
}

MdnsRecord GetFakeSrvRecord(const DomainName& name,
                            const DomainName& target,
                            std::chrono::seconds ttl) {
  SrvRecordRdata rdata(0, 0, kFakeSrvRecordPort, target);
  return MdnsRecord(name, DnsType::kSRV, DnsClass::kIN, RecordType::kUnique,
                    ttl, std::move(rdata));
}

MdnsRecord GetFakeTxtRecord(const DomainName& name, std::chrono::seconds ttl) {
  TxtRecordRdata rdata;
  return MdnsRecord(name, DnsType::kTXT, DnsClass::kIN, RecordType::kUnique,
                    ttl, std::move(rdata));
}

MdnsRecord GetFakeARecord(const DomainName& name, std::chrono::seconds ttl) {
  ARecordRdata rdata(kFakeARecordAddress);
  return MdnsRecord(name, DnsType::kA, DnsClass::kIN, RecordType::kUnique, ttl,
                    std::move(rdata));
}

MdnsRecord GetFakeAAAARecord(const DomainName& name, std::chrono::seconds ttl) {
  AAAARecordRdata rdata(kFakeAAAARecordAddress);
  return MdnsRecord(name, DnsType::kAAAA, DnsClass::kIN, RecordType::kUnique,
                    ttl, std::move(rdata));
}

}  // namespace openscreen::discovery
