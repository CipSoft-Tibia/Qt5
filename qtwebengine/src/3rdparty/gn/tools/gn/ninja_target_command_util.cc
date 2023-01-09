// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/ninja_target_command_util.h"

#include <string.h>

#include "tools/gn/c_tool.h"
#include "tools/gn/substitution_writer.h"

namespace {

// Returns the language-specific suffix for precompiled header files.
const char* GetPCHLangSuffixForToolType(const char* name) {
  if (name == CTool::kCToolCc)
    return "c";
  if (name == CTool::kCToolCxx)
    return "cc";
  if (name == CTool::kCToolObjC)
    return "m";
  if (name == CTool::kCToolObjCxx)
    return "mm";
  NOTREACHED() << "Not a valid PCH tool type: " << name;
  return "";
}

}  // namespace

// Returns the computed name of the Windows .pch file for the given
// tool type. The tool must support precompiled headers.
OutputFile GetWindowsPCHFile(const Target* target, const char* tool_name) {
  // Use "obj/{dir}/{target_name}_{lang}.pch" which ends up
  // looking like "obj/chrome/browser/browser_cc.pch"
  OutputFile ret = GetBuildDirForTargetAsOutputFile(target, BuildDirType::OBJ);
  ret.value().append(target->label().name());
  ret.value().push_back('_');
  ret.value().append(GetPCHLangSuffixForToolType(tool_name));
  ret.value().append(".pch");

  return ret;
}

void WriteOneFlag(const Target* target,
                  const Substitution* subst_enum,
                  bool has_precompiled_headers,
                  const char* tool_name,
                  const std::vector<std::string>& (ConfigValues::*getter)()
                      const,
                  EscapeOptions flag_escape_options,
                  PathOutput& path_output,
                  std::ostream& out,
                  bool write_substitution) {
  if (!target->toolchain()->substitution_bits().used.count(subst_enum))
    return;

  if (write_substitution)
    out << subst_enum->ninja_name << " =";

  if (has_precompiled_headers) {
    const CTool* tool = target->toolchain()->GetToolAsC(tool_name);
    if (tool && tool->precompiled_header_type() == CTool::PCH_MSVC) {
      // Name the .pch file.
      out << " /Fp";
      path_output.WriteFile(out, GetWindowsPCHFile(target, tool_name));

      // Enables precompiled headers and names the .h file. It's a string
      // rather than a file name (so no need to rebase or use path_output).
      out << " /Yu" << target->config_values().precompiled_header();
      RecursiveTargetConfigStringsToStream(target, getter, flag_escape_options,
                                           out);
    } else if (tool && tool->precompiled_header_type() == CTool::PCH_GCC) {
      // The targets to build the .gch files should omit the -include flag
      // below. To accomplish this, each substitution flag is overwritten in
      // the target rule and these values are repeated. The -include flag is
      // omitted in place of the required -x <header lang> flag for .gch
      // targets.
      RecursiveTargetConfigStringsToStream(target, getter, flag_escape_options,
                                           out);

      // Compute the gch file (it will be language-specific).
      std::vector<OutputFile> outputs;
      GetPCHOutputFiles(target, tool_name, &outputs);
      if (!outputs.empty()) {
        // Trim the .gch suffix for the -include flag.
        // e.g. for gch file foo/bar/target.precompiled.h.gch:
        //          -include foo/bar/target.precompiled.h
        std::string pch_file = outputs[0].value();
        pch_file.erase(pch_file.length() - 4);
        out << " -include " << pch_file;
      }
    } else {
      RecursiveTargetConfigStringsToStream(target, getter, flag_escape_options,
                                           out);
    }
  } else {
    RecursiveTargetConfigStringsToStream(target, getter, flag_escape_options,
                                         out);
  }

  if (write_substitution)
    out << std::endl;
}

void GetPCHOutputFiles(const Target* target,
                       const char* tool_name,
                       std::vector<OutputFile>* outputs) {
  outputs->clear();

  // Compute the tool. This must use the tool type passed in rather than the
  // detected file type of the precompiled source file since the same
  // precompiled source file will be used for separate C/C++ compiles.
  const CTool* tool = target->toolchain()->GetToolAsC(tool_name);
  if (!tool)
    return;
  SubstitutionWriter::ApplyListToCompilerAsOutputFile(
      target, target->config_values().precompiled_source(), tool->outputs(),
      outputs);

  if (outputs->empty())
    return;
  if (outputs->size() > 1)
    outputs->resize(1);  // Only link the first output from the compiler tool.

  std::string& output_value = (*outputs)[0].value();
  size_t extension_offset = FindExtensionOffset(output_value);
  if (extension_offset == std::string::npos) {
    // No extension found.
    return;
  }
  DCHECK(extension_offset >= 1);
  DCHECK(output_value[extension_offset - 1] == '.');

  std::string output_extension;
  CTool::PrecompiledHeaderType header_type = tool->precompiled_header_type();
  switch (header_type) {
    case CTool::PCH_MSVC:
      output_extension = GetWindowsPCHObjectExtension(
          tool_name, output_value.substr(extension_offset - 1));
      break;
    case CTool::PCH_GCC:
      output_extension = GetGCCPCHOutputExtension(tool_name);
      break;
    case CTool::PCH_NONE:
      NOTREACHED() << "No outputs for no PCH type.";
      break;
  }
  output_value.replace(extension_offset - 1, std::string::npos,
                       output_extension);
}

std::string GetGCCPCHOutputExtension(const char* tool_name) {
  const char* lang_suffix = GetPCHLangSuffixForToolType(tool_name);
  std::string result = ".";
  // For GCC, the output name must have a .gch suffix and be annotated with
  // the language type. For example:
  //   obj/foo/target_name.header.h ->
  //   obj/foo/target_name.header.h-cc.gch
  // In order for the compiler to pick it up, the output name (minus the .gch
  // suffix MUST match whatever is passed to the -include flag).
  result += "h-";
  result += lang_suffix;
  result += ".gch";
  return result;
}

std::string GetWindowsPCHObjectExtension(const char* tool_name,
                                         const std::string& obj_extension) {
  const char* lang_suffix = GetPCHLangSuffixForToolType(tool_name);
  std::string result = ".";
  // For MSVC, annotate the obj files with the language type. For example:
  //   obj/foo/target_name.precompile.obj ->
  //   obj/foo/target_name.precompile.cc.obj
  result += lang_suffix;
  result += obj_extension;
  return result;
}
