// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/unique_vector.h"

#include <stddef.h>

#include <algorithm>

#include "util/test/test.h"

TEST(UniqueVector, PushBack) {
  UniqueVector<int> foo;
  EXPECT_TRUE(foo.push_back(1));
  EXPECT_FALSE(foo.push_back(1));
  EXPECT_TRUE(foo.push_back(2));
  EXPECT_TRUE(foo.push_back(0));
  EXPECT_FALSE(foo.push_back(2));
  EXPECT_FALSE(foo.push_back(1));

  EXPECT_EQ(3u, foo.size());
  EXPECT_EQ(1, foo[0]);
  EXPECT_EQ(2, foo[1]);
  EXPECT_EQ(0, foo[2]);

  // Verify those results with IndexOf as well.
  EXPECT_EQ(0u, foo.IndexOf(1));
  EXPECT_EQ(1u, foo.IndexOf(2));
  EXPECT_EQ(2u, foo.IndexOf(0));
  EXPECT_EQ(static_cast<size_t>(-1), foo.IndexOf(99));
}

TEST(UniqueVector, PushBackMove) {
  UniqueVector<std::string> vect;
  std::string a("a");
  EXPECT_TRUE(vect.push_back(std::move(a)));
  EXPECT_EQ("", a);

  a = "a";
  EXPECT_FALSE(vect.push_back(std::move(a)));
  EXPECT_EQ("a", a);

  EXPECT_EQ(0u, vect.IndexOf("a"));
}
