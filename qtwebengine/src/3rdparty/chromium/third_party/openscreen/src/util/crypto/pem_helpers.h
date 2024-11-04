// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_PEM_HELPERS_H_
#define UTIL_CRYPTO_PEM_HELPERS_H_

#include <openssl/evp.h>

#include <string>
#include <string_view>
#include <vector>

namespace openscreen {

std::vector<std::string> ReadCertificatesFromPemFile(std::string_view filename);

bssl::UniquePtr<EVP_PKEY> ReadKeyFromPemFile(std::string_view filename);

}  // namespace openscreen

#endif  // UTIL_CRYPTO_PEM_HELPERS_H_
