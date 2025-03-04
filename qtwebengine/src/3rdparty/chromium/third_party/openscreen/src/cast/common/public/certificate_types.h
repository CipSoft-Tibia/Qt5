// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_PUBLIC_CERTIFICATE_TYPES_H_
#define CAST_COMMON_PUBLIC_CERTIFICATE_TYPES_H_

#include <stdint.h>

namespace openscreen::cast {

// This must be a UTC time.
struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

}  // namespace openscreen::cast

#endif  // CAST_COMMON_PUBLIC_CERTIFICATE_TYPES_H_
