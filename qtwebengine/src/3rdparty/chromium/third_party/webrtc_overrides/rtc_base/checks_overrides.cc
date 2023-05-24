// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/webrtc/rtc_base/checks.h"

#include "base/logging.h"

namespace rtc::webrtc_checks_impl {

RTC_NORETURN void WriteFatalLog(absl::string_view output) {
  LOG(FATAL) << output;
#if !defined(_MSC_VER) || defined(__clang__)
  __builtin_unreachable();
#else
  __assume(0);
#endif
}

RTC_NORETURN void WriteFatalLog(const char* file,
                                int line,
                                absl::string_view output) {
  {
    logging::LogMessage msg(file, line, logging::LOG_FATAL);
    msg.stream() << output;
  }
#if !defined(_MSC_VER) || defined(__clang__)
  __builtin_unreachable();
#else
  __assume(0);
#endif
}

}  // namespace rtc::webrtc_checks_impl
