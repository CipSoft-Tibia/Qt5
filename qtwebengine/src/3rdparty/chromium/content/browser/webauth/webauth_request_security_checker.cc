// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/webauth_request_security_checker.h"

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "device/fido/features.h"
#include "device/fido/fido_transport_protocol.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"
#include "third_party/blink/public/common/loader/network_utils.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy_feature.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_util.h"

namespace content {

namespace {

constexpr char kCryptotokenOrigin[] =
    "chrome-extension://kmendfapggjehodndflmmgagdbamhnfd";

// Returns AuthenticatorStatus::SUCCESS if the domain is valid and an error
// if it fails one of the criteria below.
// Reference https://url.spec.whatwg.org/#valid-domain-string and
// https://html.spec.whatwg.org/multipage/origin.html#concept-origin-effective-domain.
blink::mojom::AuthenticatorStatus ValidateEffectiveDomain(
    url::Origin caller_origin) {
  // For calls originating in the CryptoToken U2F extension, allow CryptoToken
  // to validate domain.
  if (WebAuthRequestSecurityChecker::OriginIsCryptoTokenExtension(
          caller_origin)) {
    return blink::mojom::AuthenticatorStatus::SUCCESS;
  }

  if (caller_origin.opaque()) {
    return blink::mojom::AuthenticatorStatus::OPAQUE_DOMAIN;
  }

  if (url::HostIsIPAddress(caller_origin.host()) ||
      !blink::network_utils::IsOriginSecure(caller_origin.GetURL())) {
    return blink::mojom::AuthenticatorStatus::INVALID_DOMAIN;
  }

  // Additionally, the scheme is required to be HTTP(S). Other schemes
  // may be supported in the future but the webauthn relying party is
  // just the domain of the origin so we would have to define how the
  // authority part of other schemes maps to a "domain" without
  // collisions. Given the |blink::network_utils::IsOriginSecure| check, just
  // above, HTTP is effectively restricted to just "localhost".
  if (caller_origin.scheme() != url::kHttpScheme &&
      caller_origin.scheme() != url::kHttpsScheme) {
    return blink::mojom::AuthenticatorStatus::INVALID_PROTOCOL;
  }

  return blink::mojom::AuthenticatorStatus::SUCCESS;
}

// Return the relying party ID to use for a request given the requested RP ID
// and the origin of the caller. It's valid for the requested RP ID to be a
// registrable domain suffix of, or be equal to, the origin's effective domain.
// Reference:
// https://html.spec.whatwg.org/multipage/origin.html#is-a-registrable-domain-suffix-of-or-is-equal-to.
base::Optional<std::string> GetRelyingPartyId(
    const std::string& claimed_relying_party_id,
    const url::Origin& caller_origin) {
  if (WebAuthRequestSecurityChecker::OriginIsCryptoTokenExtension(
          caller_origin)) {
    // This code trusts cryptotoken to handle the validation itself.
    return claimed_relying_party_id;
  }

  if (claimed_relying_party_id.empty()) {
    return base::nullopt;
  }

  if (caller_origin.host() == claimed_relying_party_id) {
    return claimed_relying_party_id;
  }

  if (!caller_origin.DomainIs(claimed_relying_party_id)) {
    return base::nullopt;
  }

  if (!net::registry_controlled_domains::HostHasRegistryControlledDomain(
          caller_origin.host(),
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ||
      !net::registry_controlled_domains::HostHasRegistryControlledDomain(
          claimed_relying_party_id,
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    // TODO(crbug.com/803414): Accept corner-case situations like the following
    // origin: "https://login.awesomecompany",
    // relying_party_id: "awesomecompany".
    return base::nullopt;
  }

  return claimed_relying_party_id;
}

}  // namespace

WebAuthRequestSecurityChecker::WebAuthRequestSecurityChecker(
    RenderFrameHost* host)
    : render_frame_host_(host) {}

WebAuthRequestSecurityChecker::~WebAuthRequestSecurityChecker() = default;

// static
void WebAuthRequestSecurityChecker::ReportSecurityCheckFailure(
    RelyingPartySecurityCheckFailure error) {
  UMA_HISTOGRAM_ENUMERATION(
      "WebAuthentication.RelyingPartySecurityCheckFailure", error);
}

// static
bool WebAuthRequestSecurityChecker::OriginIsCryptoTokenExtension(
    const url::Origin& origin) {
  auto cryptotoken_origin = url::Origin::Create(GURL(kCryptotokenOrigin));
  return cryptotoken_origin == origin;
}

bool WebAuthRequestSecurityChecker::IsSameOriginWithAncestors(
    const url::Origin& origin) {
  RenderFrameHost* parent = render_frame_host_->GetParent();
  while (parent) {
    if (!parent->GetLastCommittedOrigin().IsSameOriginWith(origin))
      return false;
    parent = parent->GetParent();
  }
  return true;
}

blink::mojom::AuthenticatorStatus
WebAuthRequestSecurityChecker::ValidateAncestorOrigins(
    const url::Origin& origin,
    RequestType type,
    bool* is_cross_origin) {
  *is_cross_origin = !IsSameOriginWithAncestors(origin);
  if ((type == RequestType::kMakeCredential ||
       !base::FeatureList::IsEnabled(
           device::kWebAuthGetAssertionFeaturePolicy) ||
       !static_cast<RenderFrameHostImpl*>(render_frame_host_)
            ->IsFeatureEnabled(blink::mojom::FeaturePolicyFeature::
                                   kPublicKeyCredentialsGet)) &&
      *is_cross_origin) {
    ReportSecurityCheckFailure(
        RelyingPartySecurityCheckFailure::kCrossOriginMismatch);
    return blink::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR;
  }
  return blink::mojom::AuthenticatorStatus::SUCCESS;
}

blink::mojom::AuthenticatorStatus
WebAuthRequestSecurityChecker::ValidateDomainAndRelyingPartyID(
    const url::Origin& caller_origin,
    const std::string& relying_party_id) {
  blink::mojom::AuthenticatorStatus domain_validation =
      ValidateEffectiveDomain(caller_origin);
  if (domain_validation != blink::mojom::AuthenticatorStatus::SUCCESS) {
    ReportSecurityCheckFailure(
        RelyingPartySecurityCheckFailure::kOpaqueOrNonSecureOrigin);
    return domain_validation;
  }

  base::Optional<std::string> valid_rp_id =
      GetRelyingPartyId(relying_party_id, caller_origin);
  if (!valid_rp_id) {
    ReportSecurityCheckFailure(
        RelyingPartySecurityCheckFailure::kRelyingPartyIdInvalid);
    return blink::mojom::AuthenticatorStatus::BAD_RELYING_PARTY_ID;
  }
  return blink::mojom::AuthenticatorStatus::SUCCESS;
}

blink::mojom::AuthenticatorStatus
WebAuthRequestSecurityChecker::ValidateAPrioriAuthenticatedUrl(
    const GURL& url) {
  if (url.is_empty())
    return blink::mojom::AuthenticatorStatus::SUCCESS;

  if (!url.is_valid()) {
    ReportSecurityCheckFailure(
        RelyingPartySecurityCheckFailure::kIconUrlInvalid);
    return blink::mojom::AuthenticatorStatus::INVALID_ICON_URL;
  }

  // https://www.w3.org/TR/mixed-content/#a-priori-authenticated-url
  if (!url.IsAboutSrcdoc() && !url.IsAboutBlank() &&
      !url.SchemeIs(url::kDataScheme) &&
      !blink::network_utils::IsOriginSecure(url)) {
    ReportSecurityCheckFailure(
        RelyingPartySecurityCheckFailure::kIconUrlInvalid);
    return blink::mojom::AuthenticatorStatus::INVALID_ICON_URL;
  }

  return blink::mojom::AuthenticatorStatus::SUCCESS;
}

bool WebAuthRequestSecurityChecker::
    DeduplicateCredentialDescriptorListAndValidateLength(
        std::vector<device::PublicKeyCredentialDescriptor>* list) {
  // Credential descriptor lists should not exceed 64 entries, which is enforced
  // by renderer code. Any duplicate entries they contain should be ignored.
  // This is to guard against sites trying to amplify small timing differences
  // in the processing of different types of credentials when sending probing
  // requests to physical security keys (https://crbug.com/1248862).
  if (list->size() > blink::mojom::kPublicKeyCredentialDescriptorListMaxSize) {
    return false;
  }
  auto credential_descriptor_compare_without_transport =
      [](const device::PublicKeyCredentialDescriptor& a,
         const device::PublicKeyCredentialDescriptor& b) {
        return a.credential_type() < b.credential_type() ||
               (a.credential_type() == b.credential_type() && a.id() < b.id());
      };
  std::set<device::PublicKeyCredentialDescriptor,
           decltype(credential_descriptor_compare_without_transport)>
      unique_credential_descriptors(
          credential_descriptor_compare_without_transport);
  for (const auto& credential_descriptor : *list) {
    auto it = unique_credential_descriptors.find(credential_descriptor);
    if (it == unique_credential_descriptors.end()) {
      unique_credential_descriptors.insert(credential_descriptor);
    } else {
      // Combine transport hints of descriptors with identical IDs. Empty
      // transport list means _any_ transport, so the union should still be
      // empty.
      base::flat_set<device::FidoTransportProtocol> merged_transports;
      if (!it->transports().empty() &&
          !credential_descriptor.transports().empty()) {
        std::set_union(
            base::ranges::begin(it->transports()),
            base::ranges::end(it->transports()),
            base::ranges::begin(credential_descriptor.transports()),
            base::ranges::end(credential_descriptor.transports()),
            std::inserter(merged_transports, merged_transports.begin()));
      }
      unique_credential_descriptors.erase(it);
      unique_credential_descriptors.insert(
          {credential_descriptor.credential_type(), credential_descriptor.id(),
           std::move(merged_transports)});
    }
  }
  *list = {unique_credential_descriptors.begin(),
           unique_credential_descriptors.end()};
  return true;
}

}  // namespace content
