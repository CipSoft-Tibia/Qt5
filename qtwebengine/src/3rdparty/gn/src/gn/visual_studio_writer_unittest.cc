// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/visual_studio_writer.h"

#include <memory>

#include "base/strings/string_util.h"
#include "gn/test_with_scope.h"
#include "gn/visual_studio_utils.h"
#include "util/test/test.h"

namespace {

class VisualStudioWriterTest : public testing::Test {
 protected:
  TestWithScope setup_;
};

std::string MakeTestPath(const std::string& path) {
#if defined(OS_WIN)
  return "C:" + path;
#else
  return path;
#endif
}

}  // namespace

TEST_F(VisualStudioWriterTest, ResolveSolutionFolders) {
  VisualStudioWriter writer(setup_.build_settings(), "Win32",
                            VisualStudioWriter::Version::Vs2015,
                            "10.0.17134.0");

  std::string path =
      MakeTestPath("/foo/chromium/src/out/Debug/obj/base/base.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "base", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/base"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/tools/gn/gn.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "gn", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/tools/gn"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/chrome/chrome.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "chrome", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/chrome"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/base/bar.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "bar", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/base"), "Win32"));

  writer.ResolveSolutionFolders();

  ASSERT_EQ(MakeTestPath("/foo/chromium/src"), writer.root_folder_path_);

  ASSERT_EQ(4u, writer.folders_.size());

  ASSERT_EQ("base", writer.folders_[0]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/base"), writer.folders_[0]->path);
  ASSERT_EQ(nullptr, writer.folders_[0]->parent_folder);

  ASSERT_EQ("chrome", writer.folders_[1]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/chrome"), writer.folders_[1]->path);
  ASSERT_EQ(nullptr, writer.folders_[1]->parent_folder);

  ASSERT_EQ("tools", writer.folders_[2]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools"), writer.folders_[2]->path);
  ASSERT_EQ(nullptr, writer.folders_[2]->parent_folder);

  ASSERT_EQ("gn", writer.folders_[3]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools/gn"),
            writer.folders_[3]->path);
  ASSERT_EQ(writer.folders_[2].get(), writer.folders_[3]->parent_folder);

  ASSERT_EQ(writer.folders_[0].get(), writer.projects_[0]->parent_folder);
  ASSERT_EQ(writer.folders_[3].get(), writer.projects_[1]->parent_folder);
  ASSERT_EQ(writer.folders_[1].get(), writer.projects_[2]->parent_folder);
  ASSERT_EQ(writer.folders_[0].get(), writer.projects_[3]->parent_folder);
}

TEST_F(VisualStudioWriterTest, ResolveSolutionFolders_AbsPath) {
  VisualStudioWriter writer(setup_.build_settings(), "Win32",
                            VisualStudioWriter::Version::Vs2015,
                            "10.0.17134.0");

  std::string path =
      MakeTestPath("/foo/chromium/src/out/Debug/obj/base/base.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "base", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/base"), "Win32"));

  path = MakeTestPath("/foo/chromium/src/out/Debug/obj/tools/gn/gn.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "gn", path, MakeGuid(path, "project"),
          MakeTestPath("/foo/chromium/src/tools/gn"), "Win32"));

  path = MakeTestPath(
      "/foo/chromium/src/out/Debug/obj/ABS_PATH/C/foo/bar/bar.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "bar", path, MakeGuid(path, "project"), MakeTestPath("/foo/bar"),
          "Win32"));

  std::string baz_label_dir_path = MakeTestPath("/foo/bar/baz");
#if defined(OS_WIN)
  // Make sure mixed lower and upper-case drive letters are handled properly.
  baz_label_dir_path[0] = base::ToLowerASCII(baz_label_dir_path[0]);
#endif
  path = MakeTestPath(
      "/foo/chromium/src/out/Debug/obj/ABS_PATH/C/foo/bar/baz/baz.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "baz", path, MakeGuid(path, "project"), baz_label_dir_path, "Win32"));

  writer.ResolveSolutionFolders();

  ASSERT_EQ(MakeTestPath("/foo"), writer.root_folder_path_);

  ASSERT_EQ(7u, writer.folders_.size());

  ASSERT_EQ("bar", writer.folders_[0]->name);
  ASSERT_EQ(MakeTestPath("/foo/bar"), writer.folders_[0]->path);
  ASSERT_EQ(nullptr, writer.folders_[0]->parent_folder);

  ASSERT_EQ("baz", writer.folders_[1]->name);
  ASSERT_EQ(MakeTestPath("/foo/bar/baz"), writer.folders_[1]->path);
  ASSERT_EQ(writer.folders_[0].get(), writer.folders_[1]->parent_folder);

  ASSERT_EQ("chromium", writer.folders_[2]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium"), writer.folders_[2]->path);
  ASSERT_EQ(nullptr, writer.folders_[2]->parent_folder);

  ASSERT_EQ("src", writer.folders_[3]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src"), writer.folders_[3]->path);
  ASSERT_EQ(writer.folders_[2].get(), writer.folders_[3]->parent_folder);

  ASSERT_EQ("base", writer.folders_[4]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/base"), writer.folders_[4]->path);
  ASSERT_EQ(writer.folders_[3].get(), writer.folders_[4]->parent_folder);

  ASSERT_EQ("tools", writer.folders_[5]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools"), writer.folders_[5]->path);
  ASSERT_EQ(writer.folders_[3].get(), writer.folders_[5]->parent_folder);

  ASSERT_EQ("gn", writer.folders_[6]->name);
  ASSERT_EQ(MakeTestPath("/foo/chromium/src/tools/gn"),
            writer.folders_[6]->path);
  ASSERT_EQ(writer.folders_[5].get(), writer.folders_[6]->parent_folder);

  ASSERT_EQ(writer.folders_[4].get(), writer.projects_[0]->parent_folder);
  ASSERT_EQ(writer.folders_[6].get(), writer.projects_[1]->parent_folder);
  ASSERT_EQ(writer.folders_[0].get(), writer.projects_[2]->parent_folder);
  ASSERT_EQ(writer.folders_[1].get(), writer.projects_[3]->parent_folder);
}

TEST_F(VisualStudioWriterTest, NoDotSlash) {
  VisualStudioWriter writer(setup_.build_settings(), "Win32",
                            VisualStudioWriter::Version::Vs2015,
                            "10.0.17134.0");

  std::string path = MakeTestPath("blah.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "base", path, MakeGuid(path, "project"), MakeTestPath("/foo"),
          "Win32"));

  std::unique_ptr<Tool> tool = Tool::CreateTool(CTool::kCToolAlink);
  tool->set_outputs(SubstitutionList::MakeForTest(
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}", ""));

  Toolchain toolchain(setup_.settings(), Label(SourceDir("//tc/"), "tc"));
  toolchain.SetTool(std::move(tool));

  Target target(setup_.settings(), Label(SourceDir("//baz/"), "baz"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.SetToolchain(&toolchain);

  Err err;
  ASSERT_TRUE(target.OnResolved(&err));

  VisualStudioWriter::SourceFileCompileTypePairs source_types;

  std::stringstream file_contents;
  writer.WriteProjectFileContents(file_contents, *writer.projects_.back(),
                                  &target, "", "", &source_types, &err);

  // Should find args of a ninja clean command, with no ./ before the file name.
  ASSERT_NE(file_contents.str().find("-tclean baz"), std::string::npos);
}

TEST_F(VisualStudioWriterTest, NinjaExecutable) {
  VisualStudioWriter writer(setup_.build_settings(), "Win32",
                            VisualStudioWriter::Version::Vs2015,
                            "10.0.17134.0");

  std::string path = MakeTestPath("blah.vcxproj");
  writer.projects_.push_back(
      std::make_unique<VisualStudioWriter::SolutionProject>(
          "base", path, MakeGuid(path, "project"), MakeTestPath("/foo"),
          "Win32"));

  std::unique_ptr<Tool> tool = Tool::CreateTool(CTool::kCToolAlink);
  tool->set_outputs(SubstitutionList::MakeForTest(
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}", ""));

  Toolchain toolchain(setup_.settings(), Label(SourceDir("//tc/"), "tc"));
  toolchain.SetTool(std::move(tool));

  Target target(setup_.settings(), Label(SourceDir("//baz/"), "baz"));
  target.set_output_type(Target::STATIC_LIBRARY);
  target.SetToolchain(&toolchain);

  Err err;
  ASSERT_TRUE(target.OnResolved(&err));

  VisualStudioWriter::SourceFileCompileTypePairs source_types;

  std::stringstream file_contents_without_flag;
  writer.WriteProjectFileContents(file_contents_without_flag,
                                  *writer.projects_.back(), &target, "", "",
                                  &source_types, &err);

  // Should default to ninja.exe if ninja_executable flag is not set.
  ASSERT_NE(file_contents_without_flag.str().find("call ninja.exe"),
            std::string::npos);

  std::stringstream file_contents_with_flag;
  writer.WriteProjectFileContents(file_contents_with_flag,
                                  *writer.projects_.back(), &target, "",
                                  "ninja_wrapper.exe", &source_types, &err);

  // Should use ninja_wrapper.exe because ninja_executable flag is set.
  ASSERT_NE(file_contents_with_flag.str().find("call ninja_wrapper.exe"),
            std::string::npos);
}
