// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSFUNCTIONINITIALIAZER_P_H
#define QQMLJSFUNCTIONINITIALIAZER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljscompilepass_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSFunctionInitializer
{
    Q_DISABLE_COPY_MOVE(QQmlJSFunctionInitializer)
public:
    QQmlJSFunctionInitializer(
            const QQmlJSTypeResolver *typeResolver,
            const QV4::CompiledData::Location &objectLocation,
            const QV4::CompiledData::Location &scopeLocation)
        : m_typeResolver(typeResolver)
        , m_scopeType(typeResolver->scopeForLocation(scopeLocation))
        , m_objectType(typeResolver->scopeForLocation(objectLocation))
    {}

    QQmlJSCompilePass::Function run(
            const QV4::Compiler::Context *context,
            const QString &propertyName, QQmlJS::AST::Node *astNode,
            const QmlIR::Binding &irBinding,
            QQmlJS::DiagnosticMessage *error);
    QQmlJSCompilePass::Function run(
            const QV4::Compiler::Context *context,
            const QString &functionName, QQmlJS::AST::Node *astNode,
            QQmlJS::DiagnosticMessage *error);

private:
    void populateSignature(
            const QV4::Compiler::Context *context, QQmlJS::AST::FunctionExpression *ast,
            QQmlJSCompilePass::Function *function, QQmlJS::DiagnosticMessage *error);

    const QQmlJSTypeResolver *m_typeResolver = nullptr;
    const QQmlJSScope::ConstPtr m_scopeType;
    const QQmlJSScope::ConstPtr m_objectType;
};

QT_END_NAMESPACE

#endif // QQMLJSFUNCTIONINITIALIZER_P_H
