// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/ninja_binary_target_writer.h"

#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"
#include "util/test/test.h"

using NinjaBinaryTargetWriterTest = TestWithScheduler;

TEST_F(NinjaBinaryTargetWriterTest, CSources) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  // Also test object files, which should be just passed through to the
  // dependents to link.
  target.sources().push_back(SourceFile("//foo/input3.o"));
  target.sources().push_back(SourceFile("//foo/input4.obj"));
  target.source_types_used().Set(SourceFile::SOURCE_CPP);
  target.source_types_used().Set(SourceFile::SOURCE_O);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = bar\n"
      "\n"
      "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc\n"
      "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc\n"
      "\n"
      "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
      "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

TEST_F(NinjaBinaryTargetWriterTest, NoSourcesSourceSet) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.visibility().SetPublic();
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = bar\n"
      "\n"
      "\n"
      "build obj/foo/bar.stamp: stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

TEST_F(NinjaBinaryTargetWriterTest, NoSourcesStaticLib) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.visibility().SetPublic();
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "\n"
      "build obj/foo/libbar.a: alink\n"
      "  arflags =\n"
      "  output_extension = \n"
      "  output_dir = \n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}
