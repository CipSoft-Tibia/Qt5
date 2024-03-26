// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "gn/build_settings.h"
#include "gn/err.h"
#include "gn/loader.h"
#include "gn/parse_tree.h"
#include "gn/parser.h"
#include "gn/scheduler.h"
#include "gn/test_with_scheduler.h"
#include "gn/tokenizer.h"
#include "util/msg_loop.h"
#include "util/test/test.h"

namespace {

bool ItemContainsBuildDependencyFile(const Item* item,
                                     const SourceFile& source_file) {
  const auto& build_dependency_files = item->build_dependency_files();
  return build_dependency_files.end() !=
         build_dependency_files.find(source_file);
}

class MockBuilder {
 public:
  void OnItemDefined(std::unique_ptr<Item> item);
  std::vector<const Item*> GetAllItems() const;

 private:
  std::vector<std::unique_ptr<Item>> items_;
};

void MockBuilder::OnItemDefined(std::unique_ptr<Item> item) {
  items_.push_back(std::move(item));
}

std::vector<const Item*> MockBuilder::GetAllItems() const {
  std::vector<const Item*> result;
  for (const auto& item : items_) {
    result.push_back(item.get());
  }

  return result;
}

class MockInputFileManager {
 public:
  using Callback = std::function<void(const ParseNode*)>;

  MockInputFileManager();
  ~MockInputFileManager();

  LoaderImpl::AsyncLoadFileCallback GetAsyncCallback();

  // Sets a given response for a given source file.
  void AddCannedResponse(const SourceFile& source_file,
                         const std::string& source);

  // Returns true if there is/are pending load(s) matching the given file(s).
  bool HasOnePending(const SourceFile& f) const;
  bool HasTwoPending(const SourceFile& f1, const SourceFile& f2) const;

  void IssueAllPending();

 private:
  struct CannedResult {
    std::unique_ptr<InputFile> input_file;
    std::vector<Token> tokens;
    std::unique_ptr<ParseNode> root;
  };

  InputFileManager::SyncLoadFileCallback GetSyncCallback();

  bool AsyncLoadFile(const LocationRange& origin,
                     const BuildSettings* build_settings,
                     const SourceFile& file_name,
                     const Callback& callback,
                     Err* err) {
    pending_.push_back(std::make_pair(file_name, callback));
    return true;
  }

  using CannedResponseMap = std::map<SourceFile, std::unique_ptr<CannedResult>>;
  CannedResponseMap canned_responses_;

  std::vector<std::pair<SourceFile, Callback>> pending_;
};

MockInputFileManager::MockInputFileManager() {
  g_scheduler->input_file_manager()->set_load_file_callback(GetSyncCallback());
}

MockInputFileManager::~MockInputFileManager() {
  g_scheduler->input_file_manager()->set_load_file_callback(nullptr);
}

LoaderImpl::AsyncLoadFileCallback MockInputFileManager::GetAsyncCallback() {
  return
      [this](const LocationRange& origin, const BuildSettings* build_settings,
             const SourceFile& file_name, const Callback& callback, Err* err) {
        return AsyncLoadFile(origin, build_settings, file_name, callback, err);
      };
}

InputFileManager::SyncLoadFileCallback MockInputFileManager::GetSyncCallback() {
  return
      [this](const SourceFile& file_name, InputFile* file) {
        CannedResponseMap::const_iterator found = canned_responses_.find(file_name);
        if (found == canned_responses_.end())
          return false;
        file->SetContents(found->second->input_file->contents());
        return true;
      };
}

// Sets a given response for a given source file.
void MockInputFileManager::AddCannedResponse(const SourceFile& source_file,
                                             const std::string& source) {
  std::unique_ptr<CannedResult> canned = std::make_unique<CannedResult>();
  canned->input_file = std::make_unique<InputFile>(source_file);
  canned->input_file->SetContents(source);

  // Tokenize.
  Err err;
  canned->tokens = Tokenizer::Tokenize(canned->input_file.get(), &err);
  EXPECT_FALSE(err.has_error());

  // Parse.
  canned->root = Parser::Parse(canned->tokens, &err);
  if (err.has_error())
    err.PrintToStdout();
  EXPECT_FALSE(err.has_error());

  canned_responses_[source_file] = std::move(canned);
}

bool MockInputFileManager::HasOnePending(const SourceFile& f) const {
  return pending_.size() == 1u && pending_[0].first == f;
}

bool MockInputFileManager::HasTwoPending(const SourceFile& f1,
                                         const SourceFile& f2) const {
  if (pending_.size() != 2u)
    return false;
  return pending_[0].first == f1 && pending_[1].first == f2;
}

void MockInputFileManager::IssueAllPending() {
  BlockNode block(BlockNode::DISCARDS_RESULT);  // Default response.

  for (const auto& cur : pending_) {
    CannedResponseMap::const_iterator found = canned_responses_.find(cur.first);
    if (found == canned_responses_.end())
      cur.second(&block);
    else
      cur.second(found->second->root.get());
  }
  pending_.clear();
}

// LoaderTest ------------------------------------------------------------------

class LoaderTest : public TestWithScheduler {
 public:
  LoaderTest() { build_settings_.SetBuildDir(SourceDir("//out/Debug/")); }

 protected:
  BuildSettings build_settings_;
  MockBuilder mock_builder_;
  MockInputFileManager mock_ifm_;
};

}  // namespace

// -----------------------------------------------------------------------------

TEST_F(LoaderTest, Foo) {
  SourceFile build_config("//build/config/BUILDCONFIG.gn");
  build_settings_.set_build_config_file(build_config);

  scoped_refptr<LoaderImpl> loader(new LoaderImpl(&build_settings_));

  // The default toolchain needs to be set by the build config file.
  mock_ifm_.AddCannedResponse(build_config,
                              "set_default_toolchain(\"//tc:tc\")");

  loader->set_async_load_file(mock_ifm_.GetAsyncCallback());

  // Request the root build file be loaded. This should kick off the default
  // build config loading.
  SourceFile root_build("//BUILD.gn");
  loader->Load(root_build, LocationRange(), Label());
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Completing the build config load should kick off the root build file load.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();
  EXPECT_TRUE(mock_ifm_.HasOnePending(root_build));

  // Load the root build file.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();

  // Schedule some other file to load in another toolchain.
  Label second_tc(SourceDir("//tc2/"), "tc2");
  SourceFile second_file("//foo/BUILD.gn");
  loader->Load(second_file, LocationRange(), second_tc);
  EXPECT_TRUE(mock_ifm_.HasOnePending(SourceFile("//tc2/BUILD.gn")));

  // Running the toolchain file should schedule the build config file to load
  // for that toolchain.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();

  // We have to tell it we have a toolchain definition now (normally the
  // builder would do this).
  const Settings* default_settings = loader->GetToolchainSettings(Label());
  Toolchain second_tc_object(default_settings, second_tc);
  loader->ToolchainLoaded(&second_tc_object);
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Scheduling a second file to load in that toolchain should not make it
  // pending yet (it's waiting for the build config).
  SourceFile third_file("//bar/BUILD.gn");
  loader->Load(third_file, LocationRange(), second_tc);
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Running the build config file should make our third file pending.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();
  EXPECT_TRUE(mock_ifm_.HasTwoPending(second_file, third_file));

  EXPECT_FALSE(scheduler().is_failed());
}

TEST_F(LoaderTest, BuildDependencyFilesAreCollected) {
  SourceFile build_config("//build/config/BUILDCONFIG.gn");
  SourceFile root_build("//BUILD.gn");
  build_settings_.set_build_config_file(build_config);
  build_settings_.set_item_defined_callback(
      [builder = &mock_builder_](std::unique_ptr<Item> item) {
        builder->OnItemDefined(std::move(item));
      });

  scoped_refptr<LoaderImpl> loader(new LoaderImpl(&build_settings_));
  mock_ifm_.AddCannedResponse(build_config,
                              "set_default_toolchain(\"//tc:tc\")");
  std::string root_build_content =
      "executable(\"a\") { sources = [ \"a.cc\" ] }\n"
      "config(\"b\") { configs = [\"//t:t\"] }\n"
      "toolchain(\"c\") {}\n"
      "pool(\"d\") { depth = 1 }";
  mock_ifm_.AddCannedResponse(root_build, root_build_content);

  loader->set_async_load_file(mock_ifm_.GetAsyncCallback());

  // Request the root build file be loaded. This should kick off the default
  // build config loading.
  loader->Load(root_build, LocationRange(), Label());
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Completing the build config load should kick off the root build file load.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();
  EXPECT_TRUE(mock_ifm_.HasOnePending(root_build));

  // Completing the root build file should define a target which must have
  // set of source files hashes.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();

  std::vector<const Item*> items = mock_builder_.GetAllItems();
  EXPECT_TRUE(items[0]->AsTarget());
  EXPECT_TRUE(ItemContainsBuildDependencyFile(items[0], root_build));
  EXPECT_TRUE(items[1]->AsConfig());
  EXPECT_TRUE(ItemContainsBuildDependencyFile(items[1], root_build));
  EXPECT_TRUE(items[2]->AsToolchain());
  EXPECT_TRUE(ItemContainsBuildDependencyFile(items[2], root_build));
  EXPECT_TRUE(items[3]->AsPool());
  EXPECT_TRUE(ItemContainsBuildDependencyFile(items[3], root_build));

  EXPECT_FALSE(scheduler().is_failed());
}

TEST_F(LoaderTest, TemplateBuildDependencyFilesAreCollected) {
  SourceFile build_config("//build/config/BUILDCONFIG.gn");
  SourceFile root_build("//BUILD.gn");
  build_settings_.set_build_config_file(build_config);
  build_settings_.set_item_defined_callback(
      [builder = &mock_builder_](std::unique_ptr<Item> item) {
        builder->OnItemDefined(std::move(item));
      });

  scoped_refptr<LoaderImpl> loader(new LoaderImpl(&build_settings_));
  mock_ifm_.AddCannedResponse(build_config,
                              "set_default_toolchain(\"//tc:tc\")");
  mock_ifm_.AddCannedResponse(
      SourceFile("//test.gni"),
      "template(\"tmpl\") {\n"
      "  executable(target_name) { sources = invoker.sources }\n"
      "}\n");
  mock_ifm_.AddCannedResponse(root_build,
                              "import(\"//test.gni\")\n"
                              "tmpl(\"a\") {sources = [ \"a.cc\" ]}\n");

  loader->set_async_load_file(mock_ifm_.GetAsyncCallback());

  // Request the root build file be loaded. This should kick off the default
  // build config loading.
  loader->Load(root_build, LocationRange(), Label());
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Completing the build config load should kick off the root build file load.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();
  EXPECT_TRUE(mock_ifm_.HasOnePending(root_build));

  // Completing the root build file should define a target which must have
  // set of source files hashes.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();

  std::vector<const Item*> items = mock_builder_.GetAllItems();
  EXPECT_TRUE(items[0]->AsTarget());
  // Ensure the target as a dep on BUILD.gn even though it was defined via
  // a template.
  EXPECT_TRUE(ItemContainsBuildDependencyFile(items[0], root_build));

  EXPECT_FALSE(scheduler().is_failed());
}

TEST_F(LoaderTest, NonDefaultBuildFileName) {
  std::string new_name = "BUILD.more.gn";

  SourceFile build_config("//build/config/BUILDCONFIG.gn");
  build_settings_.set_build_config_file(build_config);

  scoped_refptr<LoaderImpl> loader(new LoaderImpl(&build_settings_));
  loader->set_build_file_extension("more");

  // The default toolchain needs to be set by the build config file.
  mock_ifm_.AddCannedResponse(build_config,
                              "set_default_toolchain(\"//tc:tc\")");

  loader->set_async_load_file(mock_ifm_.GetAsyncCallback());

  // Request the root build file be loaded. This should kick off the default
  // build config loading.
  SourceFile root_build("//" + new_name);
  loader->Load(root_build, LocationRange(), Label());
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Completing the build config load should kick off the root build file load.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();
  EXPECT_TRUE(mock_ifm_.HasOnePending(root_build));

  // Load the root build file.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();

  // Schedule some other file to load in another toolchain.
  Label second_tc(SourceDir("//tc2/"), "tc2");
  SourceFile second_file("//foo/" + new_name);
  loader->Load(second_file, LocationRange(), second_tc);
  EXPECT_TRUE(mock_ifm_.HasOnePending(SourceFile("//tc2/" + new_name)));

  // Running the toolchain file should schedule the build config file to load
  // for that toolchain.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();

  // We have to tell it we have a toolchain definition now (normally the
  // builder would do this).
  const Settings* default_settings = loader->GetToolchainSettings(Label());
  Toolchain second_tc_object(default_settings, second_tc);
  loader->ToolchainLoaded(&second_tc_object);
  EXPECT_TRUE(mock_ifm_.HasOnePending(build_config));

  // Running the build config file should make our second file pending.
  mock_ifm_.IssueAllPending();
  MsgLoop::Current()->RunUntilIdleForTesting();
  EXPECT_TRUE(mock_ifm_.HasOnePending(second_file));

  EXPECT_FALSE(scheduler().is_failed());
}
