// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_COMMON_TESTING_MOCK_REPORTING_CLIENT_H_
#define DISCOVERY_COMMON_TESTING_MOCK_REPORTING_CLIENT_H_

#include "discovery/common/reporting_client.h"
#include "gmock/gmock.h"

namespace openscreen::discovery {

class MockReportingClient : public ReportingClient {
 public:
  MOCK_METHOD1(OnFatalError, void(Error error));
  MOCK_METHOD1(OnRecoverableError, void(Error error));
};

}  // namespace openscreen::discovery

#endif  // DISCOVERY_COMMON_TESTING_MOCK_REPORTING_CLIENT_H_
