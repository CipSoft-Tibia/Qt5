// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/public/base/session_binding_utils.h"

#include "base/base64url.h"
#include "base/containers/span.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "base/values.h"
#include "crypto/sha2.h"
#include "crypto/signature_verifier.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace signin {

namespace {

// Source: JSON Web Signature and Encryption Algorithms
// https://www.iana.org/assignments/jose/jose.xhtml
std::string SignatureAlgorithmToString(
    crypto::SignatureVerifier::SignatureAlgorithm algorithm) {
  switch (algorithm) {
    case crypto::SignatureVerifier::ECDSA_SHA256:
      return "ES256";
    case crypto::SignatureVerifier::RSA_PKCS1_SHA256:
      return "RS256";
    case crypto::SignatureVerifier::RSA_PSS_SHA256:
      return "PS256";
    case crypto::SignatureVerifier::RSA_PKCS1_SHA1:
      return "RS1";
  }
}

std::string Base64UrlEncode(base::StringPiece data) {
  std::string output;
  base::Base64UrlEncode(data, base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &output);
  return output;
}

std::string Base64UrlEncode(base::span<const uint8_t> data) {
  return Base64UrlEncode(base::StringPiece(
      reinterpret_cast<const char*>(data.data()), data.size()));
}

base::Value::Dict CreatePublicKeyInfo(base::span<const uint8_t> pubkey) {
  return base::Value::Dict()
      .Set("kty",
           "accounts.google.com/.well-known/kty/"
           "SubjectPublicKeyInfo")
      .Set("SubjectPublicKeyInfo", Base64UrlEncode(pubkey));
}

absl::optional<std::string> CreateHeaderAndPayloadWithCustomPayload(
    crypto::SignatureVerifier::SignatureAlgorithm algorithm,
    const base::Value::Dict& payload) {
  auto header = base::Value::Dict()
                    .Set("alg", SignatureAlgorithmToString(algorithm))
                    .Set("typ", "jwt");
  absl::optional<std::string> header_serialized = base::WriteJson(header);
  if (!header_serialized) {
    DVLOG(1) << "Unexpected JSONWriter error while serializing a registration "
                "token header";
    return absl::nullopt;
  }

  absl::optional<std::string> payload_serialized = base::WriteJsonWithOptions(
      payload, base::JSONWriter::OPTIONS_OMIT_DOUBLE_TYPE_PRESERVATION);
  if (!payload_serialized) {
    DVLOG(1) << "Unexpected JSONWriter error while serializing a registration "
                "token payload";
    return absl::nullopt;
  }

  return base::StrCat({Base64UrlEncode(*header_serialized), ".",
                       Base64UrlEncode(*payload_serialized)});
}

}  // namespace

absl::optional<std::string>
CreateKeyRegistrationHeaderAndPayloadForTokenBinding(
    base::StringPiece client_id,
    base::StringPiece auth_code,
    const GURL& registration_url,
    crypto::SignatureVerifier::SignatureAlgorithm algorithm,
    base::span<const uint8_t> pubkey,
    base::Time timestamp) {
  auto payload =
      base::Value::Dict()
          .Set("sub", client_id)
          .Set("aud", registration_url.spec())
          .Set("jti", Base64UrlEncode(crypto::SHA256HashString(auth_code)))
          // Write out int64_t variable as a double.
          // Note: this may discard some precision, but for `base::Value`
          // there's no other option.
          .Set("iat", static_cast<double>(
                          (timestamp - base::Time::UnixEpoch()).InSeconds()))
          .Set("key", CreatePublicKeyInfo(pubkey));
  return CreateHeaderAndPayloadWithCustomPayload(algorithm, payload);
}

absl::optional<std::string>
CreateKeyRegistrationHeaderAndPayloadForSessionBinding(
    base::StringPiece challenge,
    const GURL& registration_url,
    crypto::SignatureVerifier::SignatureAlgorithm algorithm,
    base::span<const uint8_t> pubkey,
    base::Time timestamp) {
  auto payload =
      base::Value::Dict()
          .Set("aud", registration_url.spec())
          .Set("jti", challenge)
          // Write out int64_t variable as a double.
          // Note: this may discard some precision, but for `base::Value`
          // there's no other option.
          .Set("iat", static_cast<double>(
                          (timestamp - base::Time::UnixEpoch()).InSeconds()))
          .Set("key", CreatePublicKeyInfo(pubkey));
  return CreateHeaderAndPayloadWithCustomPayload(algorithm, payload);
}

absl::optional<std::string> CreateKeyAssertionHeaderAndPayload(
    crypto::SignatureVerifier::SignatureAlgorithm algorithm,
    base::span<const uint8_t> pubkey,
    base::StringPiece client_id,
    base::StringPiece challenge,
    const GURL& destination_url) {
  auto payload = base::Value::Dict()
                     .Set("sub", client_id)
                     .Set("aud", destination_url.spec())
                     .Set("jti", challenge)
                     .Set("iss", Base64UrlEncode(crypto::SHA256Hash(pubkey)));
  return CreateHeaderAndPayloadWithCustomPayload(algorithm, payload);
}

std::string AppendSignatureToHeaderAndPayload(
    base::StringPiece header_and_payload,
    base::span<const uint8_t> signature) {
  return base::StrCat({header_and_payload, ".", Base64UrlEncode(signature)});
}

}  // namespace signin
