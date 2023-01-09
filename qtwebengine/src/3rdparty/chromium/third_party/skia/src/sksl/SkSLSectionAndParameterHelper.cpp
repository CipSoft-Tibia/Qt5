/*
 * Copyright 2019 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/sksl/SkSLSectionAndParameterHelper.h"
#include "src/sksl/ir/SkSLBinaryExpression.h"
#include "src/sksl/ir/SkSLConstructor.h"
#include "src/sksl/ir/SkSLDoStatement.h"
#include "src/sksl/ir/SkSLExpressionStatement.h"
#include "src/sksl/ir/SkSLFieldAccess.h"
#include "src/sksl/ir/SkSLForStatement.h"
#include "src/sksl/ir/SkSLFunctionCall.h"
#include "src/sksl/ir/SkSLIfStatement.h"
#include "src/sksl/ir/SkSLIndexExpression.h"
#include "src/sksl/ir/SkSLPostfixExpression.h"
#include "src/sksl/ir/SkSLPrefixExpression.h"
#include "src/sksl/ir/SkSLReturnStatement.h"
#include "src/sksl/ir/SkSLSwitchStatement.h"
#include "src/sksl/ir/SkSLSwizzle.h"
#include "src/sksl/ir/SkSLTernaryExpression.h"
#include "src/sksl/ir/SkSLVarDeclarationsStatement.h"
#include "src/sksl/ir/SkSLWhileStatement.h"

namespace SkSL {
#if 0
constexpr char kClassSection[];
constexpr char kCloneSection[];
constexpr char kConstructorSection[];
constexpr char kConstructorCodeSection[];
constexpr char kConstructorParamsSection[];
constexpr char kCppSection[];
constexpr char kCppEndSection[];
constexpr char kDumpInfoSection[];
constexpr char kEmitCodeSection[];
constexpr char kFieldsSection[];
constexpr char kHeaderSection[];
constexpr char kHeaderEndSection[];
constexpr char kInitializersSection[];
constexpr char kMakeSection[];
constexpr char kOptimizationFlagsSection[];
constexpr char kSamplerParamsSection[];
constexpr char kSetDataSection[];
constexpr char kTestCodeSection[];
#endif

SectionAndParameterHelper::SectionAndParameterHelper(const Program* program, ErrorReporter& errors)
    : fProgram(*program) {
    for (const auto& p : fProgram) {
        switch (p.kind()) {
            case ProgramElement::Kind::kVar: {
                const VarDeclarations& decls = (const VarDeclarations&) p;
                for (const auto& raw : decls.fVars) {
                    const VarDeclaration& decl = (VarDeclaration&) *raw;
                    if (IsParameter(*decl.fVar)) {
                        fParameters.push_back(decl.fVar);
                    }
                }
                break;
            }
            case ProgramElement::Kind::kSection: {
                const Section& s = (const Section&) p;
                if (IsSupportedSection(s.fName.c_str())) {
                    if (SectionRequiresArgument(s.fName.c_str()) && !s.fArgument.size()) {
                        errors.error(s.fOffset,
                                     ("section '@" + s.fName +
                                      "' requires one parameter").c_str());
                    }
                    if (!SectionAcceptsArgument(s.fName.c_str()) && s.fArgument.size()) {
                        errors.error(s.fOffset,
                                     ("section '@" + s.fName + "' has no parameters").c_str());
                    }
                } else {
                    errors.error(s.fOffset,
                                 ("unsupported section '@" + s.fName + "'").c_str());
                }
                if (!SectionPermitsDuplicates(s.fName.c_str()) &&
                        fSections.find(s.fName) != fSections.end()) {
                    errors.error(s.fOffset,
                                 ("duplicate section '@" + s.fName + "'").c_str());
                }
                fSections[s.fName].push_back(&s);
                break;
            }
            default:
                break;
        }
    }
}

}  // namespace SkSL
