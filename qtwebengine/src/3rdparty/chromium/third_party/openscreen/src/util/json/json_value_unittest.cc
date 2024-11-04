// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/json/json_value.h"

#include "gtest/gtest.h"
#include "platform/base/error.h"
#include "util/json/json_serialization.h"

namespace openscreen {

TEST(JsonValueTest, GetInt) {
  std::string_view obj(R"!({"key1": 17, "key2": 32.3, "key3": "asdf"})!");
  ErrorOr<Json::Value> value_or_error = json::Parse(obj);
  ASSERT_TRUE(value_or_error);
  Json::Value& value = value_or_error.value();
  std::optional<int> result1 =
      MaybeGetInt(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key1"));
  std::optional<int> result2 =
      MaybeGetInt(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key2"));
  std::optional<int> result3 =
      MaybeGetInt(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key42"));
  EXPECT_FALSE(result2);
  EXPECT_FALSE(result3);

  ASSERT_TRUE(result1);
  EXPECT_EQ(result1.value(), 17);
}

TEST(JsonValueTest, GetString) {
  std::string_view obj(
      R"!({"key1": 17, "key2": 32.3, "key3": "asdf", "key4": ""})!");
  ErrorOr<Json::Value> value_or_error = json::Parse(obj);
  ASSERT_TRUE(value_or_error);
  Json::Value& value = value_or_error.value();
  std::optional<std::string_view> result1 =
      MaybeGetString(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key3"));
  std::optional<std::string_view> result2 =
      MaybeGetString(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key2"));
  std::optional<std::string_view> result3 =
      MaybeGetString(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key42"));
  std::optional<std::string_view> result4 =
      MaybeGetString(value, JSON_EXPAND_FIND_CONSTANT_ARGS("key4"));

  EXPECT_FALSE(result2);
  EXPECT_FALSE(result3);

  ASSERT_TRUE(result1);
  EXPECT_EQ(result1.value(), "asdf");
  ASSERT_TRUE(result4);
  EXPECT_EQ(result4.value(), "");
}

}  // namespace openscreen
