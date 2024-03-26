// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/scope_per_file_provider.h"
#include "gn/build_settings.h"
#include "gn/settings.h"
#include "gn/test_with_scope.h"
#include "gn/toolchain.h"
#include "gn/variables.h"
#include "util/test/test.h"

TEST(ScopePerFileProvider, Expected) {
  TestWithScope test;

// Prevent horrible wrapping of calls below.
#define GPV(val) provider.GetProgrammaticValue(val)->string_value()

  // Test the default toolchain.
  {
    Scope scope(test.settings());
    scope.set_source_dir(SourceDir("//source/"));
    ScopePerFileProvider provider(&scope, true);

    EXPECT_EQ("//toolchain:default", GPV(variables::kCurrentToolchain));
    // TODO(brettw) this test harness does not set up the Toolchain manager
    // which is the source of this value, so we can't test this yet.
    // EXPECT_EQ("//toolchain:default",    GPV(variables::kDefaultToolchain));
    EXPECT_EQ("//out/Debug", GPV(variables::kRootBuildDir));
    EXPECT_EQ("//out/Debug/gen", GPV(variables::kRootGenDir));
    EXPECT_EQ("//out/Debug", GPV(variables::kRootOutDir));
    EXPECT_EQ("//out/Debug/gen/source", GPV(variables::kTargetGenDir));
    EXPECT_EQ("//out/Debug/obj/source", GPV(variables::kTargetOutDir));

    EXPECT_GE(provider.GetProgrammaticValue(variables::kGnVersion)->int_value(),
              0);
  }

  // Test some with an alternate toolchain.
  {
    Settings settings(test.build_settings(), "tc/");
    Toolchain toolchain(&settings, Label(SourceDir("//toolchain/"), "tc"));
    settings.set_toolchain_label(toolchain.label());

    Scope scope(&settings);
    scope.set_source_dir(SourceDir("//source/"));
    ScopePerFileProvider provider(&scope, true);

    EXPECT_EQ("//toolchain:tc", GPV(variables::kCurrentToolchain));
    // See above.
    // EXPECT_EQ("//toolchain:default", GPV(variables::kDefaultToolchain));
    EXPECT_EQ("//out/Debug", GPV(variables::kRootBuildDir));
    EXPECT_EQ("//out/Debug/tc/gen", GPV(variables::kRootGenDir));
    EXPECT_EQ("//out/Debug/tc", GPV(variables::kRootOutDir));
    EXPECT_EQ("//out/Debug/tc/gen/source", GPV(variables::kTargetGenDir));
    EXPECT_EQ("//out/Debug/tc/obj/source", GPV(variables::kTargetOutDir));
  }
}
