// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/toolchain.h"

#include <stddef.h>
#include <string.h>
#include <utility>

#include "base/logging.h"
#include "tools/gn/target.h"
#include "tools/gn/value.h"

Toolchain::Toolchain(const Settings* settings,
                     const Label& label,
                     const std::set<SourceFile>& build_dependency_files)
    : Item(settings, label, build_dependency_files) {}

Toolchain::~Toolchain() = default;

Toolchain* Toolchain::AsToolchain() {
  return this;
}

const Toolchain* Toolchain::AsToolchain() const {
  return this;
}

Tool* Toolchain::GetTool(const char* name) {
  DCHECK(name != Tool::kToolNone);
  auto pair = tools_.find(name);
  if (pair != tools_.end()) {
    return pair->second.get();
  }
  return nullptr;
}

const Tool* Toolchain::GetTool(const char* name) const {
  DCHECK(name != Tool::kToolNone);
  auto pair = tools_.find(name);
  if (pair != tools_.end()) {
    return pair->second.get();
  }
  return nullptr;
}

GeneralTool* Toolchain::GetToolAsGeneral(const char* name) {
  if (Tool* tool = GetTool(name)) {
    return tool->AsGeneral();
  }
  return nullptr;
}

const GeneralTool* Toolchain::GetToolAsGeneral(const char* name) const {
  if (const Tool* tool = GetTool(name)) {
    return tool->AsGeneral();
  }
  return nullptr;
}

CTool* Toolchain::GetToolAsC(const char* name) {
  if (Tool* tool = GetTool(name)) {
    return tool->AsC();
  }
  return nullptr;
}

const CTool* Toolchain::GetToolAsC(const char* name) const {
  if (const Tool* tool = GetTool(name)) {
    return tool->AsC();
  }
  return nullptr;
}

RustTool* Toolchain::GetToolAsRust(const char* name) {
  if (Tool* tool = GetTool(name)) {
    return tool->AsRust();
  }
  return nullptr;
}

const RustTool* Toolchain::GetToolAsRust(const char* name) const {
  if (const Tool* tool = GetTool(name)) {
    return tool->AsRust();
  }
  return nullptr;
}

void Toolchain::SetTool(std::unique_ptr<Tool> t) {
  DCHECK(t->name() != Tool::kToolNone);
  DCHECK(tools_.find(t->name()) == tools_.end());
  t->SetComplete();
  tools_[t->name()] = std::move(t);
}

void Toolchain::ToolchainSetupComplete() {
  // Collect required bits from all tools.
  for (const auto& tool : tools_) {
    substitution_bits_.MergeFrom(tool.second->substitution_bits());
  }
  setup_complete_ = true;
}

const Tool* Toolchain::GetToolForSourceType(SourceFile::Type type) const {
  return GetTool(Tool::GetToolTypeForSourceType(type));
}

const CTool* Toolchain::GetToolForSourceTypeAsC(SourceFile::Type type) const {
  return GetToolAsC(Tool::GetToolTypeForSourceType(type));
}

const GeneralTool* Toolchain::GetToolForSourceTypeAsGeneral(
    SourceFile::Type type) const {
  return GetToolAsGeneral(Tool::GetToolTypeForSourceType(type));
}

const RustTool* Toolchain::GetToolForSourceTypeAsRust(
    SourceFile::Type type) const {
  return GetToolAsRust(Tool::GetToolTypeForSourceType(type));
}

const Tool* Toolchain::GetToolForTargetFinalOutput(const Target* target) const {
  return GetTool(Tool::GetToolTypeForTargetFinalOutput(target));
}

const CTool* Toolchain::GetToolForTargetFinalOutputAsC(
    const Target* target) const {
  return GetToolAsC(Tool::GetToolTypeForTargetFinalOutput(target));
}

const GeneralTool* Toolchain::GetToolForTargetFinalOutputAsGeneral(
    const Target* target) const {
  return GetToolAsGeneral(Tool::GetToolTypeForTargetFinalOutput(target));
}

const RustTool* Toolchain::GetToolForTargetFinalOutputAsRust(
    const Target* target) const {
  return GetToolAsRust(Tool::GetToolTypeForTargetFinalOutput(target));
}
