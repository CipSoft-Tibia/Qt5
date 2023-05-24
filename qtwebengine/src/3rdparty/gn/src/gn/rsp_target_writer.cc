/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rsp_target_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "gn/config_values_extractors.h"
#include "gn/deps_iterator.h"
#include "gn/ninja_c_binary_target_writer.h"
#include "gn/ninja_target_command_util.h"
#include "gn/output_file.h"
#include "gn/settings.h"
#include "gn/target.h"

RspTargetWriter::RspTargetWriter(const NinjaCBinaryTargetWriter* writer,
                                 const Target* target,
                                 Type type,
                                 std::ostream& out)
    : type_(type), target_(target), nwriter_(writer), out_(out) {}
RspTargetWriter::~RspTargetWriter() {}

// Based on similar function in qt_creator_writer.cc
static void CollectDeps(std::set<const Target*>& deps, const Target* target) {
  for (const auto& dep : target->GetDeps(Target::DEPS_ALL)) {
    const Target* dep_target = dep.ptr;
    if (deps.count(dep_target))
      continue;
    deps.insert(dep_target);
    CollectDeps(deps, dep_target);
  }
}

void RspTargetWriter::Run() {
  CHECK(target_->output_type() == Target::SHARED_LIBRARY ||
        target_->output_type() == Target::STATIC_LIBRARY)
      << "RspTargetWriter only supports libraries";

  std::vector<SourceFile> other_files;
  std::vector<OutputFile> tool_outputs;
  NinjaBinaryTargetWriter::ClassifiedDeps cdeps = nwriter_->GetClassifiedDeps();

  std::set<const Target*> deps;
  deps.insert(target_);
  CollectDeps(deps, target_);
  const Settings* settings = target_->settings();
  const CTool* tool =
      target_->toolchain()->GetToolForTargetFinalOutput(target_)->AsC();

  std::string prefix(
      settings->build_settings()->build_dir().SourceWithNoTrailingSlash());
  prefix.append("/");
#if defined(OS_WIN)
  prefix.erase(prefix.begin());
#endif
  switch (type_) {
    case NONE:
      return;
    case DEFINES: {
      for (const auto& target : deps) {
        for (ConfigValuesIterator it(target); !it.done(); it.Next()) {
          for (std::string define : it.cur().defines()) {
            out_ << define << " ";
          }
        }
      }
      out_.flush();
    } break;
    case OBJECTS: {
      PathOutput path_output(settings->build_settings()->build_dir(),
                             settings->build_settings()->root_path_utf8(),
                             ESCAPE_NONE);
      std::vector<SourceFile> object_files;
      object_files.reserve(target_->sources().size());

      for (const auto& source : target_->sources()) {
        const char* tool_type = nullptr;
        if (!target_->GetOutputFilesForSource(source, &tool_type,
                                              &tool_outputs)) {
          if (source.GetType() == SourceFile::SOURCE_DEF)
            other_files.push_back(source);
          continue;  // No output for this source.
        }
        object_files.push_back(
            tool_outputs[0].AsSourceFile(settings->build_settings()));
      }
      if (target_->config_values().has_precompiled_headers()) {
        const CTool* tool_cxx =
            target_->toolchain()->GetTool(CTool::kCToolCxx)->AsC();
        if (tool_cxx && tool_cxx->precompiled_header_type() == CTool::PCH_MSVC) {
          GetPCHOutputFiles(target_, CTool::kCToolCxx, &tool_outputs);
          if (!tool_outputs.empty())
            object_files.push_back(
                tool_outputs[0].AsSourceFile(settings->build_settings()));
        }
      }
      for (const auto& file : object_files) {
        out_ << "\"" << prefix;
        path_output.WriteFile(out_, file);
        out_ << "\"\n";
      }
      for (const auto& file : cdeps.extra_object_files) {
        out_ << "\"" << prefix;
        path_output.WriteFile(out_, file);
        out_ << "\"\n";
      }
      out_.flush();
    } break;
    case LFLAGS: {
      EscapeOptions opts;
      opts.mode = ESCAPE_COMMAND;
      // First the ldflags from the target and its config.
      RecursiveTargetConfigStringsToStream(kRecursiveWriterKeepDuplicates,
                                           target_, &ConfigValues::ldflags,
                                           opts, out_);
      out_.flush();
    } break;
    case LDIR: {
      // library dirs
      const UniqueVector<SourceDir> all_lib_dirs = target_->all_lib_dirs();

      if (!all_lib_dirs.empty()) {
        PathOutput lib_path_output(settings->build_settings()->build_dir(),
                                   settings->build_settings()->root_path_utf8(),
                                   ESCAPE_COMMAND);
        for (size_t i = 0; i < all_lib_dirs.size(); i++) {
          out_ << " -L\"";
          lib_path_output.WriteDir(out_, all_lib_dirs[i],
                                   PathOutput::DIR_NO_LAST_SLASH);
          out_ << "\"";
        }
        out_.flush();
      }
    } break;
    case ARCHIVES: {
      PathOutput path_output(settings->build_settings()->build_dir(),
                             settings->build_settings()->root_path_utf8(),
                             ESCAPE_NONE);
      std::vector<OutputFile> solibs;
      for (const Target* cur : cdeps.linkable_deps) {
        if (cur->dependency_output_file().value() !=
            cur->link_output_file().value()) {
          solibs.push_back(cur->link_output_file());
        } else {
          out_ << "\"" << prefix;
          path_output.WriteFile(out_, cur->link_output_file());
          out_ << "\"\n";
        }
      }
      out_.flush();

      CHECK(solibs.empty()) << "Unhandled solibs";
    } break;
    case LIBS: {
      EscapeOptions lib_escape_opts;
      lib_escape_opts.mode = ESCAPE_COMMAND;

      const UniqueVector<LibFile> all_libs = target_->all_libs();
      const std::string framework_ending(".framework");
      for (size_t i = 0; i < all_libs.size(); i++) {
        const LibFile& lib_file = all_libs[i];
        const std::string& lib_value = lib_file.value();
        if (lib_file.is_source_file()) {
          PathOutput lib_path_output(
              settings->build_settings()->build_dir(),
              settings->build_settings()->root_path_utf8(), ESCAPE_COMMAND);
          out_ << "\"";
          lib_path_output.WriteFile(out_, lib_file.source_file());
          out_ << "\"";
        } else if (base::EndsWith(lib_value, framework_ending,
                                  base::CompareCase::INSENSITIVE_ASCII)) {
          out_ << " -framework ";
          EscapeStringToStream(
              out_,
              lib_value.substr(0, lib_value.size() - framework_ending.size()),
              lib_escape_opts);
        } else {
          out_ << " " << tool->lib_switch();
          EscapeStringToStream(out_, lib_value, lib_escape_opts);
        }
      }
      const UniqueVector<std::string> all_frameworks = target_->all_frameworks();
      for (size_t i = 0; i < all_frameworks.size(); i++) {
        const std::string& lib_value = all_frameworks[i];
        out_ << " -framework ";
        EscapeStringToStream(
            out_,
            lib_value.substr(0, lib_value.size() - framework_ending.size()),
            lib_escape_opts);
      }
      const UniqueVector<std::string> weak_frameworks =
          target_->all_weak_frameworks();
      for (size_t i = 0; i < weak_frameworks.size(); i++) {
        const std::string& lib_value = weak_frameworks[i];
        out_ << " -weak_framework ";
        EscapeStringToStream(
            out_,
            lib_value.substr(0, lib_value.size() - framework_ending.size()),
            lib_escape_opts);
      }
      out_.flush();
    }
  }
}

RspTargetWriter::Type RspTargetWriter::strToType(const std::string& str) {
  static std::unordered_map<std::string, RspTargetWriter::Type> const types = {
      {"objects", RspTargetWriter::OBJECTS},
      {"archives", RspTargetWriter::ARCHIVES},
      {"defines", RspTargetWriter::DEFINES},
      {"lflags", RspTargetWriter::LFLAGS},
      {"libs", RspTargetWriter::LIBS},
      {"ldir", RspTargetWriter::LDIR}};
  auto it = types.find(str);
  if (it != types.end()) {
    return it->second;
  }
  return RspTargetWriter::NONE;
}
