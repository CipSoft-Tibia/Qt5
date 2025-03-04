// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cassert>

#include "net/cert/pki/trust_store.h"

#include "net/cert/pki/string_util.h"

#include <assert.h>

namespace net {

namespace {

constexpr char kUnspecifiedStr[] = "UNSPECIFIED";
constexpr char kDistrustedStr[] = "DISTRUSTED";
constexpr char kTrustedAnchorStr[] = "TRUSTED_ANCHOR";
constexpr char kTrustedAnchorOrLeafStr[] = "TRUSTED_ANCHOR_OR_LEAF";
constexpr char kTrustedLeafStr[] = "TRUSTED_LEAF";

constexpr char kEnforceAnchorExpiry[] = "enforce_anchor_expiry";
constexpr char kEnforceAnchorConstraints[] = "enforce_anchor_constraints";
constexpr char kRequireAnchorBasicConstraints[] =
    "require_anchor_basic_constraints";
constexpr char kRequireLeafSelfsigned[] = "require_leaf_selfsigned";

}  // namespace

bool CertificateTrust::IsTrustAnchor() const {
  switch (type) {
    case CertificateTrustType::DISTRUSTED:
    case CertificateTrustType::UNSPECIFIED:
    case CertificateTrustType::TRUSTED_LEAF:
      return false;
    case CertificateTrustType::TRUSTED_ANCHOR:
    case CertificateTrustType::TRUSTED_ANCHOR_OR_LEAF:
      return true;
  }

  assert(0);  // NOTREACHED
  return false;
}

bool CertificateTrust::IsTrustLeaf() const {
  switch (type) {
    case CertificateTrustType::TRUSTED_LEAF:
    case CertificateTrustType::TRUSTED_ANCHOR_OR_LEAF:
      return true;
    case CertificateTrustType::DISTRUSTED:
    case CertificateTrustType::UNSPECIFIED:
    case CertificateTrustType::TRUSTED_ANCHOR:
      return false;
  }

  assert(0);  // NOTREACHED
  return false;
}

bool CertificateTrust::IsDistrusted() const {
  switch (type) {
    case CertificateTrustType::DISTRUSTED:
      return true;
    case CertificateTrustType::UNSPECIFIED:
    case CertificateTrustType::TRUSTED_ANCHOR:
    case CertificateTrustType::TRUSTED_ANCHOR_OR_LEAF:
    case CertificateTrustType::TRUSTED_LEAF:
      return false;
  }

  assert(0);  // NOTREACHED
  return false;
}

bool CertificateTrust::HasUnspecifiedTrust() const {
  switch (type) {
    case CertificateTrustType::UNSPECIFIED:
      return true;
    case CertificateTrustType::DISTRUSTED:
    case CertificateTrustType::TRUSTED_ANCHOR:
    case CertificateTrustType::TRUSTED_ANCHOR_OR_LEAF:
    case CertificateTrustType::TRUSTED_LEAF:
      return false;
  }

  assert(0);  // NOTREACHED
  return true;
}

std::string CertificateTrust::ToDebugString() const {
  std::string result;
  switch (type) {
    case CertificateTrustType::UNSPECIFIED:
      result = kUnspecifiedStr;
      break;
    case CertificateTrustType::DISTRUSTED:
      result = kDistrustedStr;
      break;
    case CertificateTrustType::TRUSTED_ANCHOR:
      result = kTrustedAnchorStr;
      break;
    case CertificateTrustType::TRUSTED_ANCHOR_OR_LEAF:
      result = kTrustedAnchorOrLeafStr;
      break;
    case CertificateTrustType::TRUSTED_LEAF:
      result = kTrustedLeafStr;
      break;
  }
  if (enforce_anchor_expiry) {
    result += '+';
    result += kEnforceAnchorExpiry;
  }
  if (enforce_anchor_constraints) {
    result += '+';
    result += kEnforceAnchorConstraints;
  }
  if (require_anchor_basic_constraints) {
    result += '+';
    result += kRequireAnchorBasicConstraints;
  }
  if (require_leaf_selfsigned) {
    result += '+';
    result += kRequireLeafSelfsigned;
  }
  return result;
}

// static
absl::optional<CertificateTrust> CertificateTrust::FromDebugString(
    const std::string& trust_string) {
  std::vector<std::string_view> split =
      string_util::SplitString(trust_string, '+');

  if (split.empty()) {
    return absl::nullopt;
  }

  CertificateTrust trust;

  if (string_util::IsEqualNoCase(split[0], kUnspecifiedStr)) {
    trust = CertificateTrust::ForUnspecified();
  } else if (string_util::IsEqualNoCase(split[0], kDistrustedStr)) {
    trust = CertificateTrust::ForDistrusted();
  } else if (string_util::IsEqualNoCase(split[0], kTrustedAnchorStr)) {
    trust = CertificateTrust::ForTrustAnchor();
  } else if (string_util::IsEqualNoCase(split[0], kTrustedAnchorOrLeafStr)) {
    trust = CertificateTrust::ForTrustAnchorOrLeaf();
  } else if (string_util::IsEqualNoCase(split[0], kTrustedLeafStr)) {
    trust = CertificateTrust::ForTrustedLeaf();
  } else {
    return absl::nullopt;
  }

  for (auto i = ++split.begin(); i != split.end(); ++i) {
    if (string_util::IsEqualNoCase(*i, kEnforceAnchorExpiry)) {
      trust = trust.WithEnforceAnchorExpiry();
    } else if (string_util::IsEqualNoCase(*i, kEnforceAnchorConstraints)) {
      trust = trust.WithEnforceAnchorConstraints();
    } else if (string_util::IsEqualNoCase(*i, kRequireAnchorBasicConstraints)) {
      trust = trust.WithRequireAnchorBasicConstraints();
    } else if (string_util::IsEqualNoCase(*i, kRequireLeafSelfsigned)) {
      trust = trust.WithRequireLeafSelfSigned();
    } else {
      return absl::nullopt;
    }
  }

  return trust;
}

TrustStore::TrustStore() = default;

void TrustStore::AsyncGetIssuersOf(const ParsedCertificate* cert,
                                   std::unique_ptr<Request>* out_req) {
  out_req->reset();
}

}  // namespace net
