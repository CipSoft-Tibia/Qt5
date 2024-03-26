// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MAC_MAC_LOGGING_H_
#define BASE_MAC_MAC_LOGGING_H_

#include "base/logging.h"
#include "util/build_config.h"

#if defined(OS_IOS)
#include <MacTypes.h>
#else
#include <libkern/OSTypes.h>
#endif

// Use the OSSTATUS_LOG family to log messages related to errors in Mac OS X
// system routines that report status via an OSStatus or OSErr value. It is
// similar to the PLOG family which operates on errno, but because there is no
// global (or thread-local) OSStatus or OSErr value, the specific error must
// be supplied as an argument to the OSSTATUS_LOG macro. The message logged
// will contain the symbolic constant name corresponding to the status value,
// along with the value itself.
//
// OSErr is just an older 16-bit form of the newer 32-bit OSStatus. Despite
// the name, OSSTATUS_LOG can be used equally well for OSStatus and OSErr.

namespace logging {

// Returns a UTF8 description from an OS X Status error.
std::string DescriptionFromOSStatus(OSStatus err);

class OSStatusLogMessage : public logging::LogMessage {
 public:
  OSStatusLogMessage(const char* file_path,
                     int line,
                     LogSeverity severity,
                     OSStatus status);
  ~OSStatusLogMessage();

 private:
  OSStatus status_;

  OSStatusLogMessage(const OSStatusLogMessage&) = delete;
  OSStatusLogMessage& operator=(const OSStatusLogMessage&) = delete;
};

}  // namespace logging

#define OSSTATUS_LOG_STREAM(severity, status) \
  COMPACT_GOOGLE_LOG_EX_##severity(OSStatusLogMessage, status).stream()

#define OSSTATUS_LOG(severity, status) \
  LAZY_STREAM(OSSTATUS_LOG_STREAM(severity, status), LOG_IS_ON(severity))
#define OSSTATUS_LOG_IF(severity, condition, status) \
  LAZY_STREAM(OSSTATUS_LOG_STREAM(severity, status), \
              LOG_IS_ON(severity) && (condition))

#define OSSTATUS_CHECK(condition, status)                       \
  LAZY_STREAM(OSSTATUS_LOG_STREAM(FATAL, status), !(condition)) \
      << "Check failed: " #condition << ". "

#define OSSTATUS_DLOG(severity, status) \
  LAZY_STREAM(OSSTATUS_LOG_STREAM(severity, status), DLOG_IS_ON(severity))
#define OSSTATUS_DLOG_IF(severity, condition, status) \
  LAZY_STREAM(OSSTATUS_LOG_STREAM(severity, status),  \
              DLOG_IS_ON(severity) && (condition))

#define OSSTATUS_DCHECK(condition, status)        \
  LAZY_STREAM(OSSTATUS_LOG_STREAM(FATAL, status), \
              DCHECK_IS_ON() && !(condition))     \
      << "Check failed: " #condition << ". "

#endif  // BASE_MAC_MAC_LOGGING_H_
