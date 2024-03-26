// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/loader.h"

#include <memory>

#include "gn/build_settings.h"
#include "gn/err.h"
#include "gn/filesystem_utils.h"
#include "gn/input_file_manager.h"
#include "gn/parse_tree.h"
#include "gn/scheduler.h"
#include "gn/scope_per_file_provider.h"
#include "gn/settings.h"
#include "gn/source_dir.h"
#include "gn/source_file.h"
#include "gn/trace.h"

namespace {

struct SourceFileAndOrigin {
  SourceFileAndOrigin(const SourceFile& f, const LocationRange& o)
      : file(f), origin(o) {}

  SourceFile file;
  LocationRange origin;
};

}  // namespace

// Identifies one time a file is loaded in a given toolchain so we don't load
// it more than once.
struct LoaderImpl::LoadID {
  LoadID() = default;
  LoadID(const SourceFile& f, const Label& tc_name)
      : file(f), toolchain_name(tc_name) {}

  bool operator<(const LoadID& other) const {
    if (file == other.file)
      return toolchain_name < other.toolchain_name;
    return file < other.file;
  }

  SourceFile file;
  Label toolchain_name;
};

// Our tracking information for a toolchain.
struct LoaderImpl::ToolchainRecord {
  // The default toolchain label can be empty for the first time the default
  // toolchain is loaded, since we don't know it yet. This will be fixed up
  // later. It should be valid in all other cases.
  ToolchainRecord(const BuildSettings* build_settings,
                  const Label& toolchain_label,
                  const Label& default_toolchain_label)
      : settings(
            build_settings,
            GetOutputSubdirName(toolchain_label,
                                toolchain_label == default_toolchain_label)),
        is_toolchain_loaded(false),
        is_config_loaded(false) {
    settings.set_default_toolchain_label(default_toolchain_label);
    settings.set_toolchain_label(toolchain_label);
  }

  Settings settings;

  bool is_toolchain_loaded;
  bool is_config_loaded;

  std::vector<SourceFileAndOrigin> waiting_on_me;
};

// -----------------------------------------------------------------------------

const void* const Loader::kDefaultToolchainKey = &kDefaultToolchainKey;

Loader::Loader() = default;

Loader::~Loader() = default;

void Loader::Load(const Label& label, const LocationRange& origin) {
  Load(BuildFileForLabel(label), origin, label.GetToolchainLabel());
}

// -----------------------------------------------------------------------------

LoaderImpl::LoaderImpl(const BuildSettings* build_settings)
    : pending_loads_(0), build_settings_(build_settings) {
  // There may not be an active TaskRunner at this point. When that's the case,
  // the calling code is expected to call set_task_runner().
  task_runner_ = MsgLoop::Current();
}

LoaderImpl::~LoaderImpl() = default;

void LoaderImpl::Load(const SourceFile& file,
                      const LocationRange& origin,
                      const Label& in_toolchain_name) {
  const Label& toolchain_name = in_toolchain_name.is_null()
                                    ? default_toolchain_label_
                                    : in_toolchain_name;
  LoadID load_id(file, toolchain_name);
  if (!invocations_.insert(load_id).second)
    return;  // Already in set, so this file was already loaded or schedulerd.

  if (toolchain_records_.empty()) {
    // Nothing loaded, need to load the default build config. The initial load
    // should not specify a toolchain.
    DCHECK(toolchain_name.is_null());

    std::unique_ptr<ToolchainRecord> new_record =
        std::make_unique<ToolchainRecord>(build_settings_, Label(), Label());
    ToolchainRecord* record = new_record.get();
    Label empty_label;  // VS issues spurious warning using ...[Label()].
    toolchain_records_[empty_label] = std::move(new_record);

    // The default build config is no dependent on the toolchain definition,
    // since we need to load the build config before we know what the default
    // toolchain name is.
    record->is_toolchain_loaded = true;

    record->waiting_on_me.push_back(SourceFileAndOrigin(file, origin));
    ScheduleLoadBuildConfig(&record->settings, Scope::KeyValueMap());

    return;
  }

  ToolchainRecord* record;
  if (toolchain_name.is_null())
    record = toolchain_records_[default_toolchain_label_].get();
  else
    record = toolchain_records_[toolchain_name].get();

  if (!record) {
    DCHECK(!default_toolchain_label_.is_null());

    // No reference to this toolchain found yet, make one.
    std::unique_ptr<ToolchainRecord> new_record =
        std::make_unique<ToolchainRecord>(build_settings_, toolchain_name,
                                          default_toolchain_label_);
    record = new_record.get();
    toolchain_records_[toolchain_name] = std::move(new_record);

    // Schedule a load of the toolchain using the default one.
    Load(BuildFileForLabel(toolchain_name), origin, default_toolchain_label_);
  }

  if (record->is_config_loaded)
    ScheduleLoadFile(&record->settings, origin, file);
  else
    record->waiting_on_me.push_back(SourceFileAndOrigin(file, origin));
}

void LoaderImpl::ToolchainLoaded(const Toolchain* toolchain) {
  ToolchainRecord* record = toolchain_records_[toolchain->label()].get();
  if (!record) {
    DCHECK(!default_toolchain_label_.is_null());
    std::unique_ptr<ToolchainRecord> new_record =
        std::make_unique<ToolchainRecord>(build_settings_, toolchain->label(),
                                          default_toolchain_label_);
    record = new_record.get();
    toolchain_records_[toolchain->label()] = std::move(new_record);
  }
  record->is_toolchain_loaded = true;

  // The default build config is loaded first, then its toolchain. Secondary
  // ones are loaded in the opposite order so we can pass toolchain parameters
  // to the build config. So we may or may not have a config at this point.
  if (!record->is_config_loaded) {
    ScheduleLoadBuildConfig(&record->settings, toolchain->args());
  } else {
    // There should be nobody waiting on this if the build config is already
    // loaded.
    DCHECK(record->waiting_on_me.empty());
  }
}

Label LoaderImpl::GetDefaultToolchain() const {
  return default_toolchain_label_;
}

const Settings* LoaderImpl::GetToolchainSettings(const Label& label) const {
  ToolchainRecordMap::const_iterator found_toolchain;
  if (label.is_null()) {
    if (default_toolchain_label_.is_null())
      return nullptr;
    found_toolchain = toolchain_records_.find(default_toolchain_label_);
  } else {
    found_toolchain = toolchain_records_.find(label);
  }

  if (found_toolchain == toolchain_records_.end())
    return nullptr;
  return &found_toolchain->second->settings;
}

SourceFile LoaderImpl::BuildFileForLabel(const Label& label) const {
  return SourceFile(
      label.dir().value() + "BUILD" + build_file_extension_ + ".gn");
}

void LoaderImpl::ScheduleLoadFile(const Settings* settings,
                                  const LocationRange& origin,
                                  const SourceFile& file) {
  Err err;
  pending_loads_++;
  if (!AsyncLoadFile(
          origin, settings->build_settings(), file,
          [this, settings, file, origin](const ParseNode* parse_node) {
            BackgroundLoadFile(settings, file, origin, parse_node);
          },
          &err)) {
    g_scheduler->FailWithError(err);
    DecrementPendingLoads();
  }
}

void LoaderImpl::ScheduleLoadBuildConfig(
    Settings* settings,
    const Scope::KeyValueMap& toolchain_overrides) {
  Err err;
  pending_loads_++;
  if (!AsyncLoadFile(
          LocationRange(), settings->build_settings(),
          settings->build_settings()->build_config_file(),
          [this, settings, toolchain_overrides](const ParseNode* root) {
            BackgroundLoadBuildConfig(settings, toolchain_overrides, root);
          },
          &err)) {
    g_scheduler->FailWithError(err);
    DecrementPendingLoads();
  }
}

void LoaderImpl::BackgroundLoadFile(const Settings* settings,
                                    const SourceFile& file_name,
                                    const LocationRange& origin,
                                    const ParseNode* root) {
  if (!root) {
    task_runner_->PostTask([this]() { DecrementPendingLoads(); });
    return;
  }

  if (g_scheduler->verbose_logging()) {
    g_scheduler->Log("Running",
                     file_name.value() + " with toolchain " +
                         settings->toolchain_label().GetUserVisibleName(false));
  }

  Scope our_scope(settings->base_config());
  ScopePerFileProvider per_file_provider(&our_scope, true);
  our_scope.set_source_dir(file_name.GetDir());
  our_scope.AddBuildDependencyFile(file_name);

  // Targets, etc. generated as part of running this file will end up here.
  Scope::ItemVector collected_items;
  our_scope.set_item_collector(&collected_items);

  ScopedTrace trace(TraceItem::TRACE_FILE_EXECUTE, file_name.value());
  trace.SetToolchain(settings->toolchain_label());

  Err err;
  root->Execute(&our_scope, &err);
  if (!err.has_error())
    our_scope.CheckForUnusedVars(&err);

  if (err.has_error()) {
    if (!origin.is_null())
      err.AppendSubErr(Err(origin, "which caused the file to be included."));

    if (!settings->is_default())
      err.set_toolchain_label(settings->toolchain_label());

    g_scheduler->FailWithError(err);
  }

  // Pass all of the items that were defined off to the builder.
  for (auto& item : collected_items)
    settings->build_settings()->ItemDefined(std::move(item));

  trace.Done();

  task_runner_->PostTask([this]() { DidLoadFile(); });
}

void LoaderImpl::BackgroundLoadBuildConfig(
    Settings* settings,
    const Scope::KeyValueMap& toolchain_overrides,
    const ParseNode* root) {
  if (!root) {
    task_runner_->PostTask([this]() { DecrementPendingLoads(); });
    return;
  }

  Scope* base_config = settings->base_config();
  base_config->set_source_dir(SourceDir("//"));
  base_config->AddBuildDependencyFile(
      settings->build_settings()->build_config_file());

  settings->build_settings()->build_args().SetupRootScope(base_config,
                                                          toolchain_overrides);

  base_config->SetProcessingBuildConfig();

  // See kDefaultToolchainKey in the header.
  Label default_toolchain_label;
  if (settings->is_default())
    base_config->SetProperty(kDefaultToolchainKey, &default_toolchain_label);

  ScopedTrace trace(TraceItem::TRACE_FILE_EXECUTE,
                    settings->build_settings()->build_config_file().value());
  trace.SetToolchain(settings->toolchain_label());

  // Run the BUILDCONFIG with its directory as the current one. We want
  // BUILDCONFIG to modify the base_config so can't make a copy or a nested one.
  base_config->set_source_dir(
      settings->build_settings()->build_config_file().GetDir());

  Err err;
  root->Execute(base_config, &err);

  // Put back the root as the default source dir. This probably isn't necessary
  // as other scopes will set their directories to their own path, but it's a
  // better default than the build config's directory.
  base_config->set_source_dir(SourceDir("//"));

  // Clear all private variables left in the scope. We want the root build
  // config to be like a .gni file in that variables beginning with an
  // underscore aren't exported.
  base_config->RemovePrivateIdentifiers();

  trace.Done();

  if (err.has_error()) {
    if (!settings->is_default())
      err.set_toolchain_label(settings->toolchain_label());

    g_scheduler->FailWithError(err);
  }

  base_config->ClearProcessingBuildConfig();
  if (settings->is_default()) {
    // The default toolchain must have been set in the default build config
    // file.
    if (default_toolchain_label.is_null()) {
      g_scheduler->FailWithError(Err(
          Location(),
          "The default build config file did not call set_default_toolchain()",
          "If you don't call this, I can't figure out what toolchain to use\n"
          "for all of this code."));
    } else {
      DCHECK(settings->toolchain_label().is_null());
      settings->set_toolchain_label(default_toolchain_label);
    }
  }

  task_runner_->PostTask(
      [this, toolchain_label = settings->toolchain_label()]() {
        DidLoadBuildConfig(toolchain_label);
      });
}

void LoaderImpl::DidLoadFile() {
  DecrementPendingLoads();
}

void LoaderImpl::DidLoadBuildConfig(const Label& label) {
  // Do not return early, we must call DecrementPendingLoads() at the bottom.

  ToolchainRecordMap::iterator found_toolchain = toolchain_records_.find(label);
  ToolchainRecord* record = nullptr;
  if (found_toolchain == toolchain_records_.end()) {
    // When loading the default build config, we'll insert it into the record
    // map with an empty label since we don't yet know what to call it.
    //
    // In this case, we should have exactly one entry in the map with an empty
    // label. We now need to fix up the naming so it refers to the "real" one.
    CHECK_EQ(1U, toolchain_records_.size());
    ToolchainRecordMap::iterator empty_label = toolchain_records_.find(Label());
    CHECK(empty_label != toolchain_records_.end());

    // Fix up the toolchain record.
    std::unique_ptr<ToolchainRecord> moved_record =
        std::move(empty_label->second);
    record = moved_record.get();
    toolchain_records_[label] = std::move(moved_record);
    toolchain_records_.erase(empty_label);

    // Save the default toolchain label.
    default_toolchain_label_ = label;
    DCHECK(record->settings.default_toolchain_label().is_null());
    record->settings.set_default_toolchain_label(label);

    // The settings object should have the toolchain label already set.
    DCHECK(!record->settings.toolchain_label().is_null());

    // Update any stored invocations that refer to the empty toolchain label.
    // This will normally only be one, for the root build file, so brute-force
    // is OK.
    LoadIDSet old_loads;
    invocations_.swap(old_loads);
    for (const auto& load : old_loads) {
      if (load.toolchain_name.is_null()) {
        // Fix up toolchain label
        invocations_.emplace(load.file, label);
      } else {
        // Can keep the old one.
        invocations_.insert(load);
      }
    }
  } else {
    record = found_toolchain->second.get();
  }

  DCHECK(!record->is_config_loaded);
  DCHECK(record->is_toolchain_loaded);
  record->is_config_loaded = true;

  // Schedule all waiting file loads.
  for (const auto& waiting : record->waiting_on_me)
    ScheduleLoadFile(&record->settings, waiting.origin, waiting.file);
  record->waiting_on_me.clear();

  DecrementPendingLoads();
}

void LoaderImpl::DecrementPendingLoads() {
  DCHECK_GT(pending_loads_, 0);
  pending_loads_--;
  if (pending_loads_ == 0 && complete_callback_)
    complete_callback_();
}

bool LoaderImpl::AsyncLoadFile(const LocationRange& origin,
                               const BuildSettings* build_settings,
                               const SourceFile& file_name,
                               std::function<void(const ParseNode*)> callback,
                               Err* err) {
  if (async_load_file_) {
    return async_load_file_(origin, build_settings, file_name,
                            std::move(callback), err);
  }
  return g_scheduler->input_file_manager()->AsyncLoadFile(
      origin, build_settings, file_name, std::move(callback), err);
}
