// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill_assistant/browser/actions/action_test_utils.h"

#include "components/autofill_assistant/browser/actions/mock_action_delegate.h"
#include "components/autofill_assistant/browser/client_status.h"
#include "components/autofill_assistant/browser/selector.h"
#include "components/autofill_assistant/browser/web/element_finder.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace autofill_assistant {
namespace test_util {

using ::testing::_;
using ::testing::WithArgs;

ElementFinder::Result MockFindElement(MockActionDelegate& delegate,
                                      const Selector& selector) {
  EXPECT_CALL(delegate, FindElement(selector, _))
      .WillOnce(WithArgs<1>([&selector](auto&& callback) {
        auto element_result = std::make_unique<ElementFinder::Result>();
        element_result->object_id = selector.proto.filters(0).css_selector();
        std::move(callback).Run(OkClientStatus(), std::move(element_result));
      }));

  ElementFinder::Result expected_result;
  expected_result.object_id = selector.proto.filters(0).css_selector();
  return expected_result;
}

}  // namespace test_util
}  // namespace autofill_assistant
