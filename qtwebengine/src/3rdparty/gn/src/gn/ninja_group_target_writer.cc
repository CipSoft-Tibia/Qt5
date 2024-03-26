// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/ninja_group_target_writer.h"

#include "base/strings/string_util.h"
#include "gn/deps_iterator.h"
#include "gn/output_file.h"
#include "gn/string_utils.h"
#include "gn/target.h"

NinjaGroupTargetWriter::NinjaGroupTargetWriter(const Target* target,
                                               std::ostream& out)
    : NinjaTargetWriter(target, out) {}

NinjaGroupTargetWriter::~NinjaGroupTargetWriter() = default;

void NinjaGroupTargetWriter::Run() {
  // A group rule just generates a stamp file with dependencies on each of
  // the deps and data_deps in the group.
  std::vector<OutputFile> output_files;
  std::vector<OutputFile> data_output_files;
  for (const auto& pair : target_->GetDeps(Target::DEPS_LINKED)) {
    if (pair.ptr->IsDataOnly()) {
      data_output_files.push_back(pair.ptr->dependency_output_file());
    } else {
      output_files.push_back(pair.ptr->dependency_output_file());
    }
  }

  const LabelTargetVector& data_deps = target_->data_deps();
  for (const auto& pair : data_deps)
    data_output_files.push_back(pair.ptr->dependency_output_file());

  WriteStampForTarget(output_files, data_output_files);
}
