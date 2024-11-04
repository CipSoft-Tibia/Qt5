// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_PROTOCOL_CASTV2_VALIDATION_H_
#define CAST_PROTOCOL_CASTV2_VALIDATION_H_

#include <vector>

#include "json/value.h"
#include "platform/base/error.h"

namespace openscreen::cast {

// Used to validate a JSON message against a JSON schema.
std::vector<Error> Validate(const Json::Value& document,
                            const Json::Value& schema_root);

// Used to validate streaming messages, such as OFFER or ANSWER.
std::vector<Error> ValidateStreamingMessage(const Json::Value& message);

// Used to validate receiver messages, such as LAUNCH or STOP.
std::vector<Error> ValidateReceiverMessage(const Json::Value& message);

}  // namespace openscreen::cast

#endif  // CAST_PROTOCOL_CASTV2_VALIDATION_H_
