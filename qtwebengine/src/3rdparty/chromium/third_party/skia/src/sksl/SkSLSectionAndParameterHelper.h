/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_SECTIONANDPARAMETERHELPER
#define SKSL_SECTIONANDPARAMETERHELPER

#include "src/sksl/SkSLErrorReporter.h"
#include "src/sksl/ir/SkSLProgram.h"
#include "src/sksl/ir/SkSLSection.h"
#include "src/sksl/ir/SkSLVarDeclarations.h"
#include <unordered_map>
#include <vector>

namespace SkSL {

constexpr char kClassSection[] =              "class";
constexpr char kCloneSection[] =              "clone";
constexpr char kConstructorSection[] =        "constructor";
constexpr char kConstructorCodeSection[] =    "constructorCode";
constexpr char kConstructorParamsSection[] =  "constructorParams";
constexpr char kCppSection[] =                "cpp";
constexpr char kCppEndSection[] =             "cppEnd";
constexpr char kDumpInfoSection[] =           "dumpInfo";
constexpr char kEmitCodeSection[] =           "emitCode";
constexpr char kFieldsSection[] =             "fields";
constexpr char kHeaderSection[] =             "header";
constexpr char kHeaderEndSection[] =          "headerEnd";
constexpr char kInitializersSection[] =       "initializers";
constexpr char kMakeSection[] =               "make";
constexpr char kOptimizationFlagsSection[] =  "optimizationFlags";
constexpr char kSamplerParamsSection[] =      "samplerParams";
constexpr char kSetDataSection[] =            "setData";
constexpr char kTestCodeSection[] =           "test";

class SectionAndParameterHelper {
public:
    SectionAndParameterHelper(const Program* program, ErrorReporter& errors);

    const Section* getSection(const char* name) {
        SkASSERT(!SectionPermitsDuplicates(name));
        auto found = fSections.find(name);
        if (found == fSections.end()) {
            return nullptr;
        }
        SkASSERT(found->second.size() == 1);
        return found->second[0];
    }

    std::vector<const Section*> getSections(const char* name) {
        auto found = fSections.find(name);
        if (found == fSections.end()) {
            return std::vector<const Section*>();
        }
        return found->second;
    }

    const std::vector<const Variable*>& getParameters() {
        return fParameters;
    }

    static bool IsParameter(const Variable& var) {
        return (var.fModifiers.fFlags & Modifiers::kIn_Flag) &&
               -1 == var.fModifiers.fLayout.fBuiltin;
    }

    static bool IsSupportedSection(const char* name) {
        return !strcmp(name, kClassSection) ||
               !strcmp(name, kCloneSection) ||
               !strcmp(name, kConstructorSection) ||
               !strcmp(name, kConstructorCodeSection) ||
               !strcmp(name, kConstructorParamsSection) ||
               !strcmp(name, kCppSection) ||
               !strcmp(name, kCppEndSection) ||
               !strcmp(name, kDumpInfoSection) ||
               !strcmp(name, kEmitCodeSection) ||
               !strcmp(name, kFieldsSection) ||
               !strcmp(name, kHeaderSection) ||
               !strcmp(name, kHeaderEndSection) ||
               !strcmp(name, kInitializersSection) ||
               !strcmp(name, kMakeSection) ||
               !strcmp(name, kOptimizationFlagsSection) ||
               !strcmp(name, kSamplerParamsSection) ||
               !strcmp(name, kSetDataSection) ||
               !strcmp(name, kTestCodeSection);
    }

    static bool SectionAcceptsArgument(const char* name) {
        return !strcmp(name, kSamplerParamsSection) ||
               !strcmp(name, kSetDataSection) ||
               !strcmp(name, kTestCodeSection);
    }

    static bool SectionRequiresArgument(const char* name) {
        return !strcmp(name, kSamplerParamsSection) ||
               !strcmp(name, kSetDataSection) ||
               !strcmp(name, kTestCodeSection);
    }

    static bool SectionPermitsDuplicates(const char* name) {
        return !strcmp(name, kSamplerParamsSection);
    }

private:
    const Program& fProgram;
    std::vector<const Variable*> fParameters;
    std::unordered_map<String, std::vector<const Section*>> fSections;
};

} // namespace SkSL

#endif
