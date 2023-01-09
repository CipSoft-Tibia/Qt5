/*
 * Copyright 2020 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/sksl/SkSLAnalysis.h"

#include "include/private/SkSLSampleUsage.h"
#include "src/sksl/SkSLErrorReporter.h"
#include "src/sksl/ir/SkSLExpression.h"
#include "src/sksl/ir/SkSLProgram.h"
#include "src/sksl/ir/SkSLProgramElement.h"
#include "src/sksl/ir/SkSLStatement.h"

// ProgramElements
#include "src/sksl/ir/SkSLEnum.h"
#include "src/sksl/ir/SkSLExtension.h"
#include "src/sksl/ir/SkSLFunctionDefinition.h"
#include "src/sksl/ir/SkSLInterfaceBlock.h"
#include "src/sksl/ir/SkSLModifiers.h"
#include "src/sksl/ir/SkSLSection.h"
#include "src/sksl/ir/SkSLVarDeclarations.h"

// Statements
#include "src/sksl/ir/SkSLBlock.h"
#include "src/sksl/ir/SkSLBreakStatement.h"
#include "src/sksl/ir/SkSLContinueStatement.h"
#include "src/sksl/ir/SkSLDiscardStatement.h"
#include "src/sksl/ir/SkSLDoStatement.h"
#include "src/sksl/ir/SkSLExpressionStatement.h"
#include "src/sksl/ir/SkSLForStatement.h"
#include "src/sksl/ir/SkSLIfStatement.h"
#include "src/sksl/ir/SkSLNop.h"
#include "src/sksl/ir/SkSLReturnStatement.h"
#include "src/sksl/ir/SkSLSwitchStatement.h"
#include "src/sksl/ir/SkSLVarDeclarationsStatement.h"
#include "src/sksl/ir/SkSLWhileStatement.h"

// Expressions
#include "src/sksl/ir/SkSLBinaryExpression.h"
#include "src/sksl/ir/SkSLBoolLiteral.h"
#include "src/sksl/ir/SkSLConstructor.h"
#include "src/sksl/ir/SkSLExternalFunctionCall.h"
#include "src/sksl/ir/SkSLExternalValueReference.h"
#include "src/sksl/ir/SkSLFieldAccess.h"
#include "src/sksl/ir/SkSLFloatLiteral.h"
#include "src/sksl/ir/SkSLFunctionCall.h"
#include "src/sksl/ir/SkSLFunctionReference.h"
#include "src/sksl/ir/SkSLIndexExpression.h"
#include "src/sksl/ir/SkSLInlineMarker.h"
#include "src/sksl/ir/SkSLIntLiteral.h"
#include "src/sksl/ir/SkSLNullLiteral.h"
#include "src/sksl/ir/SkSLPostfixExpression.h"
#include "src/sksl/ir/SkSLPrefixExpression.h"
#include "src/sksl/ir/SkSLSetting.h"
#include "src/sksl/ir/SkSLSwizzle.h"
#include "src/sksl/ir/SkSLTernaryExpression.h"
#include "src/sksl/ir/SkSLTypeReference.h"
#include "src/sksl/ir/SkSLVariableReference.h"

namespace SkSL {

namespace {

static bool is_sample_call_to_fp(const FunctionCall& fc, const Variable& fp) {
    const FunctionDeclaration& f = fc.fFunction;
    return f.fBuiltin && f.fName == "sample" && fc.fArguments.size() >= 1 &&
           fc.fArguments[0]->is<VariableReference>() &&
           fc.fArguments[0]->as<VariableReference>().fVariable == &fp;
}

// Visitor that determines the merged SampleUsage for a given child 'fp' in the program.
class MergeSampleUsageVisitor : public ProgramVisitor {
public:
    MergeSampleUsageVisitor(const Context& context, const Variable& fp)
            : fContext(context), fFP(fp) {}

    SampleUsage visit(const Program& program) {
        fUsage = SampleUsage(); // reset to none
        INHERITED::visit(program);
        return fUsage;
    }

protected:
    const Context& fContext;
    const Variable& fFP;
    SampleUsage fUsage;

    bool visitExpression(const Expression& e) override {
        // Looking for sample(fp, inColor?, ...)
        if (e.kind() == Expression::Kind::kFunctionCall) {
            const FunctionCall& fc = e.as<FunctionCall>();
            if (is_sample_call_to_fp(fc, fFP)) {
                // Determine the type of call at this site, and merge it with the accumulated state
                const Expression* lastArg = fc.fArguments.back().get();

                if (lastArg->type() == *fContext.fFloat2_Type) {
                    fUsage.merge(SampleUsage::Explicit());
                } else if (lastArg->type() == *fContext.fFloat3x3_Type) {
                    // Determine the type of matrix for this call site
                    if (lastArg->isConstantOrUniform()) {
                        if (lastArg->kind() == Expression::Kind::kVariableReference ||
                            lastArg->kind() == Expression::Kind::kConstructor) {
                            // FIXME if this is a constant, we should parse the float3x3 constructor
                            // and determine if the resulting matrix introduces perspective.
                            fUsage.merge(SampleUsage::UniformMatrix(lastArg->description()));
                        } else {
                            // FIXME this is really to workaround a restriction of the downstream
                            // code that relies on the SampleUsage's fExpression to identify uniform
                            // names. Once they are tracked separately, any uniform expression can
                            // work, but right now this avoids issues from '0.5 * matrix' that is
                            // both a constant AND a uniform.
                            fUsage.merge(SampleUsage::VariableMatrix());
                        }
                    } else {
                        fUsage.merge(SampleUsage::VariableMatrix());
                    }
                } else {
                    // The only other signatures do pass-through sampling
                    fUsage.merge(SampleUsage::PassThrough());
                }
                // NOTE: we don't return true here just because we found a sample call. We need to
                //  process the entire program and merge across all encountered calls.
            }
        }

        return INHERITED::visitExpression(e);
    }

    using INHERITED = ProgramVisitor;
};

// Visitor that searches through the program for references to a particular builtin variable
class BuiltinVariableVisitor : public ProgramVisitor {
public:
    BuiltinVariableVisitor(int builtin) : fBuiltin(builtin) {}

    bool visitExpression(const Expression& e) override {
        if (e.is<VariableReference>()) {
            const VariableReference& var = e.as<VariableReference>();
            return var.fVariable->fModifiers.fLayout.fBuiltin == fBuiltin;
        }
        return INHERITED::visitExpression(e);
    }

    int fBuiltin;

    using INHERITED = ProgramVisitor;
};

// Visitor that counts the number of nodes visited
class NodeCountVisitor : public ProgramVisitor {
public:
    int visit(const Statement& s) {
        fCount = 0;
        this->visitStatement(s);
        return fCount;
    }

    bool visitExpression(const Expression& e) override {
        ++fCount;
        return INHERITED::visitExpression(e);
    }

    bool visitProgramElement(const ProgramElement& p) override {
        ++fCount;
        return INHERITED::visitProgramElement(p);
    }

    bool visitStatement(const Statement& s) override {
        ++fCount;
        return INHERITED::visitStatement(s);
    }

private:
    int fCount;

    using INHERITED = ProgramVisitor;
};

class VariableWriteVisitor : public ProgramVisitor {
public:
    VariableWriteVisitor(const Variable* var)
        : fVar(var) {}

    bool visit(const Statement& s) {
        return this->visitStatement(s);
    }

    bool visitExpression(const Expression& e) override {
        if (e.is<VariableReference>()) {
            const VariableReference& ref = e.as<VariableReference>();
            if (ref.fVariable == fVar && (ref.fRefKind == VariableReference::kWrite_RefKind ||
                                          ref.fRefKind == VariableReference::kReadWrite_RefKind ||
                                          ref.fRefKind == VariableReference::kPointer_RefKind)) {
                return true;
            }
        }
        return INHERITED::visitExpression(e);
    }

private:
    const Variable* fVar;

    using INHERITED = ProgramVisitor;
};

// If a caller doesn't care about errors, we can use this trivial reporter that just counts up.
class TrivialErrorReporter : public ErrorReporter {
public:
    void error(int offset, String) override { ++fErrorCount; }
    int errorCount() override { return fErrorCount; }

private:
    int fErrorCount = 0;
};

// This isn't actually using ProgramVisitor, because it only considers a subset of the fields for
// any given expression kind. For instance, when indexing an array (e.g. `x[1]`), we only want to
// know if the base (`x`) is assignable; the index expression (`1`) doesn't need to be.
class IsAssignableVisitor {
public:
    IsAssignableVisitor(VariableReference** assignableVar, ErrorReporter* errors)
            : fAssignableVar(assignableVar), fErrors(errors) {
        if (fAssignableVar) {
            *fAssignableVar = nullptr;
        }
    }

    bool visit(Expression& expr) {
        this->visitExpression(expr);
        return fErrors->errorCount() == 0;
    }

    void visitExpression(Expression& expr) {
        switch (expr.kind()) {
            case Expression::Kind::kVariableReference: {
                VariableReference& varRef = expr.as<VariableReference>();
                const Variable* var = varRef.fVariable;
                if (var->fModifiers.fFlags & (Modifiers::kConst_Flag | Modifiers::kUniform_Flag |
                                              Modifiers::kVarying_Flag)) {
                    fErrors->error(expr.fOffset,
                                   "cannot modify immutable variable '" + var->fName + "'");
                } else if (fAssignableVar) {
                    SkASSERT(*fAssignableVar == nullptr);
                    *fAssignableVar = &varRef;
                }
                break;
            }
            case Expression::Kind::kFieldAccess:
                this->visitExpression(*expr.as<FieldAccess>().fBase);
                break;

            case Expression::Kind::kSwizzle: {
                const Swizzle& swizzle = expr.as<Swizzle>();
                this->checkSwizzleWrite(swizzle);
                this->visitExpression(*swizzle.fBase);
                break;
            }
            case Expression::Kind::kIndex:
                this->visitExpression(*expr.as<IndexExpression>().fBase);
                break;

            case Expression::Kind::kExternalValue: {
                const ExternalValue* var = expr.as<ExternalValueReference>().fValue;
                if (!var->canWrite()) {
                    fErrors->error(expr.fOffset,
                                   "cannot modify immutable external value '" + var->fName + "'");
                }
                break;
            }
            default:
                fErrors->error(expr.fOffset, "cannot assign to this expression");
                break;
        }
    }

private:
    void checkSwizzleWrite(const Swizzle& swizzle) {
        int bits = 0;
        for (int idx : swizzle.fComponents) {
            SkASSERT(idx <= 3);
            int bit = 1 << idx;
            if (bits & bit) {
                fErrors->error(swizzle.fOffset,
                               "cannot write to the same swizzle field more than once");
                break;
            }
            bits |= bit;
        }
    }

    VariableReference** fAssignableVar;
    ErrorReporter* fErrors;

    using INHERITED = ProgramVisitor;
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Analysis

SampleUsage Analysis::GetSampleUsage(const Program& program, const Variable& fp) {
    MergeSampleUsageVisitor visitor(*program.fContext, fp);
    return visitor.visit(program);
}

bool Analysis::ReferencesBuiltin(const Program& program, int builtin) {
    BuiltinVariableVisitor visitor(builtin);
    return visitor.visit(program);
}

bool Analysis::ReferencesSampleCoords(const Program& program) {
    return Analysis::ReferencesBuiltin(program, SK_MAIN_COORDS_BUILTIN);
}

bool Analysis::ReferencesFragCoords(const Program& program) {
    return Analysis::ReferencesBuiltin(program, SK_FRAGCOORD_BUILTIN);
}

int Analysis::NodeCount(const FunctionDefinition& function) {
    return NodeCountVisitor().visit(*function.fBody);
}

bool Analysis::StatementWritesToVariable(const Statement& stmt, const Variable& var) {
    return VariableWriteVisitor(&var).visit(stmt);
}

bool Analysis::IsAssignable(Expression& expr, VariableReference** assignableVar,
                            ErrorReporter* errors) {
    TrivialErrorReporter trivialErrors;
    return IsAssignableVisitor{assignableVar, errors ? errors : &trivialErrors}.visit(expr);
}

////////////////////////////////////////////////////////////////////////////////
// ProgramVisitor

template <typename PROG, typename EXPR, typename STMT, typename ELEM>
bool TProgramVisitor<PROG, EXPR, STMT, ELEM>::visit(PROG program) {
    for (ELEM pe : program) {
        if (this->visitProgramElement(pe)) {
            return true;
        }
    }
    return false;
}

template <typename PROG, typename EXPR, typename STMT, typename ELEM>
bool TProgramVisitor<PROG, EXPR, STMT, ELEM>::visitExpression(EXPR e) {
    switch (e.kind()) {
        case Expression::Kind::kBoolLiteral:
        case Expression::Kind::kDefined:
        case Expression::Kind::kExternalValue:
        case Expression::Kind::kFloatLiteral:
        case Expression::Kind::kFunctionReference:
        case Expression::Kind::kIntLiteral:
        case Expression::Kind::kNullLiteral:
        case Expression::Kind::kSetting:
        case Expression::Kind::kTypeReference:
        case Expression::Kind::kVariableReference:
            // Leaf expressions return false
            return false;

        case Expression::Kind::kBinary: {
            auto& b = e.template as<BinaryExpression>();
            return this->visitExpression(b.left()) || this->visitExpression(b.right());
        }
        case Expression::Kind::kConstructor: {
            auto& c = e.template as<Constructor>();
            for (auto& arg : c.arguments()) {
                if (this->visitExpression(*arg)) { return true; }
            }
            return false;
        }
        case Expression::Kind::kExternalFunctionCall: {
            auto& c = e.template as<ExternalFunctionCall>();
            for (auto& arg : c.arguments()) {
                if (this->visitExpression(*arg)) { return true; }
            }
            return false;
        }
        case Expression::Kind::kFieldAccess:
            return this->visitExpression(*e.template as<FieldAccess>().fBase);

        case Expression::Kind::kFunctionCall: {
            auto& c = e.template as<FunctionCall>();
            for (auto& arg : c.fArguments) {
                if (this->visitExpression(*arg)) { return true; }
            }
            return false;
        }
        case Expression::Kind::kIndex: {
            auto& i = e.template as<IndexExpression>();
            return this->visitExpression(*i.fBase) || this->visitExpression(*i.fIndex);
        }
        case Expression::Kind::kPostfix:
            return this->visitExpression(*e.template as<PostfixExpression>().fOperand);

        case Expression::Kind::kPrefix:
            return this->visitExpression(*e.template as<PrefixExpression>().fOperand);

        case Expression::Kind::kSwizzle:
            return this->visitExpression(*e.template as<Swizzle>().fBase);

        case Expression::Kind::kTernary: {
            auto& t = e.template as<TernaryExpression>();
            return this->visitExpression(*t.fTest) || this->visitExpression(*t.fIfTrue) ||
                   this->visitExpression(*t.fIfFalse);
        }
        default:
            SkUNREACHABLE;
    }
}

template <typename PROG, typename EXPR, typename STMT, typename ELEM>
bool TProgramVisitor<PROG, EXPR, STMT, ELEM>::visitStatement(STMT s) {
    switch (s.kind()) {
        case Statement::Kind::kBreak:
        case Statement::Kind::kContinue:
        case Statement::Kind::kDiscard:
        case Statement::Kind::kInlineMarker:
        case Statement::Kind::kNop:
            // Leaf statements just return false
            return false;

        case Statement::Kind::kBlock:
            for (auto& stmt : s.template as<Block>().children()) {
                if (this->visitStatement(*stmt)) {
                    return true;
                }
            }
            return false;

        case Statement::Kind::kDo: {
            auto& d = s.template as<DoStatement>();
            return this->visitExpression(*d.test()) || this->visitStatement(*d.statement());
        }
        case Statement::Kind::kExpression:
            return this->visitExpression(*s.template as<ExpressionStatement>().expression());

        case Statement::Kind::kFor: {
            auto& f = s.template as<ForStatement>();
            return (f.fInitializer && this->visitStatement(*f.fInitializer)) ||
                   (f.fTest && this->visitExpression(*f.fTest)) ||
                   (f.fNext && this->visitExpression(*f.fNext)) ||
                   this->visitStatement(*f.fStatement);
        }
        case Statement::Kind::kIf: {
            auto& i = s.template as<IfStatement>();
            return this->visitExpression(*i.fTest) ||
                   this->visitStatement(*i.fIfTrue) ||
                   (i.fIfFalse && this->visitStatement(*i.fIfFalse));
        }
        case Statement::Kind::kReturn: {
            auto& r = s.template as<ReturnStatement>();
            return r.fExpression && this->visitExpression(*r.fExpression);
        }
        case Statement::Kind::kSwitch: {
            auto& sw = s.template as<SwitchStatement>();
            if (this->visitExpression(*sw.fValue)) {
                return true;
            }
            for (auto& c : sw.fCases) {
                if (c->fValue && this->visitExpression(*c->fValue)) {
                    return true;
                }
                for (auto& st : c->fStatements) {
                    if (this->visitStatement(*st)) {
                        return true;
                    }
                }
            }
            return false;
        }
        case Statement::Kind::kVarDeclaration: {
            auto& v = s.template as<VarDeclaration>();
            for (auto& sizeExpr : v.fSizes) {
                if (sizeExpr && this->visitExpression(*sizeExpr)) {
                    return true;
                }
            }
            return v.fValue && this->visitExpression(*v.fValue);
        }
        case Statement::Kind::kVarDeclarations:
            return this->visitProgramElement(
                    *s.template as<VarDeclarationsStatement>().fDeclaration);

        case Statement::Kind::kWhile: {
            auto& w = s.template as<WhileStatement>();
            return this->visitExpression(*w.fTest) || this->visitStatement(*w.fStatement);
        }
        default:
            SkUNREACHABLE;
    }
}

template <typename PROG, typename EXPR, typename STMT, typename ELEM>
bool TProgramVisitor<PROG, EXPR, STMT, ELEM>::visitProgramElement(ELEM pe) {
    switch (pe.kind()) {
        case ProgramElement::Kind::kEnum:
        case ProgramElement::Kind::kExtension:
        case ProgramElement::Kind::kModifiers:
        case ProgramElement::Kind::kSection:
            // Leaf program elements just return false by default
            return false;

        case ProgramElement::Kind::kFunction:
            return this->visitStatement(*pe.template as<FunctionDefinition>().fBody);

        case ProgramElement::Kind::kInterfaceBlock:
            for (auto& e : pe.template as<InterfaceBlock>().fSizes) {
                if (this->visitExpression(*e)) {
                    return true;
                }
            }
            return false;

        case ProgramElement::Kind::kVar:
            for (auto& v : pe.template as<VarDeclarations>().fVars) {
                if (this->visitStatement(*v)) {
                    return true;
                }
            }
            return false;

        default:
            SkUNREACHABLE;
    }
}

template class TProgramVisitor<const Program&, const Expression&,
                               const Statement&, const ProgramElement&>;
template class TProgramVisitor<Program&, Expression&, Statement&, ProgramElement&>;

}  // namespace SkSL
