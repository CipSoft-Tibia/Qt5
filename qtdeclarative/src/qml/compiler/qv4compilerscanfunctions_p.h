// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4COMPILERSCANFUNCTIONS_P_H
#define QV4COMPILERSCANFUNCTIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtqmlcompilerglobal_p.h>
#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qv4util_p.h>
#include <QtCore/QStringList>
#include <QStack>
#include <QScopedValueRollback>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Moth {
struct Instruction;
}

namespace CompiledData {
struct CompilationUnit;
}

namespace Compiler {

class Codegen;

class ScanFunctions: protected QQmlJS::AST::Visitor
{
    typedef QScopedValueRollback<bool> TemporaryBoolAssignment;
public:
    ScanFunctions(Codegen *cg, const QString &sourceCode, ContextType defaultProgramType);
    void operator()(QQmlJS::AST::Node *node);

    // see comment at its call site in generateJSCodeForFunctionsAndBindings
    // for why this function is necessary
    void handleTopLevelFunctionFormals(QQmlJS::AST::FunctionExpression *node) {
        if (node && node->formals)
            node->formals->accept(this);
    }

    void enterGlobalEnvironment(ContextType compilationMode);
    void enterEnvironment(QQmlJS::AST::Node *node, ContextType compilationMode,
                          const QString &name);
    void leaveEnvironment();

    void enterQmlFunction(QQmlJS::AST::FunctionExpression *ast)
    { enterFunction(ast, FunctionNameContext::None); }

protected:
    // Function declarations add their name to the outer scope, but not the
    // inner scope. Function expressions add their name to the inner scope,
    // unless the name is actually picked from the outer scope rather than
    // given after the function token. QML functions don't add their name
    // anywhere because the name is already recorded in the QML element.
    // This enum is used to control the behavior of enterFunction().
    enum class FunctionNameContext {
        None, Inner, Outer
    };

    using Visitor::visit;
    using Visitor::endVisit;

    void checkDirectivePrologue(QQmlJS::AST::StatementList *ast);

    void checkName(QStringView name, const QQmlJS::SourceLocation &loc);

    bool visit(QQmlJS::AST::Program *ast) override;
    void endVisit(QQmlJS::AST::Program *) override;

    bool visit(QQmlJS::AST::ESModule *ast) override;
    void endVisit(QQmlJS::AST::ESModule *) override;

    bool visit(QQmlJS::AST::ExportDeclaration *declaration) override;
    bool visit(QQmlJS::AST::ImportDeclaration *declaration) override;

    bool visit(QQmlJS::AST::CallExpression *ast) override;
    bool visit(QQmlJS::AST::PatternElement *ast) override;
    bool visit(QQmlJS::AST::IdentifierExpression *ast) override;
    bool visit(QQmlJS::AST::ExpressionStatement *ast) override;
    bool visit(QQmlJS::AST::FunctionExpression *ast) override;
    bool visit(QQmlJS::AST::TemplateLiteral *ast) override;
    bool visit(QQmlJS::AST::SuperLiteral *) override;
    bool visit(QQmlJS::AST::FieldMemberExpression *) override;
    bool visit(QQmlJS::AST::ArrayPattern *) override;

    bool enterFunction(QQmlJS::AST::FunctionExpression *ast,
                       FunctionNameContext nameContext);

    void endVisit(QQmlJS::AST::FunctionExpression *) override;

    bool visit(QQmlJS::AST::ObjectPattern *ast) override;

    bool visit(QQmlJS::AST::PatternProperty *ast) override;
    void endVisit(QQmlJS::AST::PatternProperty *) override;

    bool visit(QQmlJS::AST::FunctionDeclaration *ast) override;
    void endVisit(QQmlJS::AST::FunctionDeclaration *) override;

    bool visit(QQmlJS::AST::ClassExpression *ast) override;
    void endVisit(QQmlJS::AST::ClassExpression *) override;

    bool visit(QQmlJS::AST::ClassDeclaration *ast) override;
    void endVisit(QQmlJS::AST::ClassDeclaration *) override;

    bool visit(QQmlJS::AST::DoWhileStatement *ast) override;
    bool visit(QQmlJS::AST::ForStatement *ast) override;
    void endVisit(QQmlJS::AST::ForStatement *) override;
    bool visit(QQmlJS::AST::ForEachStatement *ast) override;
    void endVisit(QQmlJS::AST::ForEachStatement *) override;

    bool visit(QQmlJS::AST::ThisExpression *ast) override;

    bool visit(QQmlJS::AST::Block *ast) override;
    void endVisit(QQmlJS::AST::Block *ast) override;

    bool visit(QQmlJS::AST::CaseBlock *ast) override;
    void endVisit(QQmlJS::AST::CaseBlock *ast) override;

    bool visit(QQmlJS::AST::Catch *ast) override;
    void endVisit(QQmlJS::AST::Catch *ast) override;

    bool visit(QQmlJS::AST::WithStatement *ast) override;
    void endVisit(QQmlJS::AST::WithStatement *ast) override;

    void throwRecursionDepthError() override;

protected:
    bool enterFunction(QQmlJS::AST::Node *ast, const QString &name,
                       QQmlJS::AST::FormalParameterList *formals,
                       QQmlJS::AST::StatementList *body, FunctionNameContext nameContext);

    void calcEscapingVariables();
// fields:
    Codegen *_cg;
    const QString _sourceCode;
    Context *_context;
    QStack<Context *> _contextStack;

    bool _allowFuncDecls;
    ContextType defaultProgramType;

private:
    static constexpr QQmlJS::AST::Node *astNodeForGlobalEnvironment = nullptr;
};

}

}

QT_END_NAMESPACE

#endif // QV4CODEGEN_P_H
