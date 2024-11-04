// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_BORINGSSL_UTIL_H_
#define CAST_COMMON_CERTIFICATE_BORINGSSL_UTIL_H_

#include <openssl/evp.h>
#include <openssl/x509v3.h>

#include "cast/common/public/certificate_types.h"
#include "platform/base/error.h"
#include "platform/base/span.h"

namespace openscreen::cast {

bool VerifySignedData(const EVP_MD* digest,
                      EVP_PKEY* public_key,
                      const ByteView& data,
                      const ByteView& signature);

ErrorOr<DateTime> GetNotBeforeTime(X509* cert);
ErrorOr<DateTime> GetNotAfterTime(X509* cert);

}  // namespace openscreen::cast

#endif  // CAST_COMMON_CERTIFICATE_BORINGSSL_UTIL_H_
