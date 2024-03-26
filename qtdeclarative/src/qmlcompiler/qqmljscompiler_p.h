// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSCOMPILER_P_H
#define QQMLJSCOMPILER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qtqmlcompilerexports_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qloggingcategory.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljscompilepass_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qv4compileddata_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

Q_QMLCOMPILER_PRIVATE_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcAotCompiler);

struct Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSCompileError
{
    QString message;
    void print();
    QQmlJSCompileError augment(const QString &contextErrorMessage) const;
    void appendDiagnostics(const QString &inputFileName,
                           const QList<QQmlJS::DiagnosticMessage> &diagnostics);
    void appendDiagnostic(const QString &inputFileName,
                          const QQmlJS::DiagnosticMessage &diagnostic);
};

struct Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSAotFunction
{
    QStringList includes;
    QStringList argumentTypes;
    QString code;
    QString returnType;
};

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSAotCompiler
{
public:
    QQmlJSAotCompiler(QQmlJSImporter *importer, const QString &resourcePath,
                      const QStringList &qmldirFiles, QQmlJSLogger *logger);

    virtual ~QQmlJSAotCompiler() = default;

    virtual void setDocument(const QmlIR::JSCodeGen *codegen, const QmlIR::Document *document);
    virtual void setScope(const QmlIR::Object *object, const QmlIR::Object *scope);
    virtual std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> compileBinding(
            const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding,
            QQmlJS::AST::Node *astNode);
    virtual std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage> compileFunction(
            const QV4::Compiler::Context *context, const QString &name, QQmlJS::AST::Node *astNode);

    virtual QQmlJSAotFunction globalCode() const;

protected:
    virtual QQmlJS::DiagnosticMessage diagnose(
            const QString &message, QtMsgType type, const QQmlJS::SourceLocation &location) const;

    QQmlJSTypeResolver m_typeResolver;

    const QString m_resourcePath;
    const QStringList m_qmldirFiles;

    const QmlIR::Document *m_document = nullptr;
    const QmlIR::Object *m_currentObject = nullptr;
    const QmlIR::Object *m_currentScope = nullptr;
    const QV4::Compiler::JSUnitGenerator *m_unitGenerator = nullptr;

    QQmlJSImporter *m_importer = nullptr;
    QQmlJSLogger *m_logger = nullptr;

private:
    QQmlJSAotFunction doCompile(
            const QV4::Compiler::Context *context, QQmlJSCompilePass::Function *function,
            QQmlJS::DiagnosticMessage *error);
};


using QQmlJSAotFunctionMap = QMap<int, QQmlJSAotFunction>;
using QQmlJSSaveFunction
    = std::function<bool(const QV4::CompiledData::SaveableUnitPointer &,
                         const QQmlJSAotFunctionMap &, QString *)>;

bool Q_QMLCOMPILER_PRIVATE_EXPORT qCompileQmlFile(const QString &inputFileName,
                                          QQmlJSSaveFunction saveFunction,
                                          QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error,
                                          bool storeSourceLocation = false,
                                          QV4::Compiler::CodegenWarningInterface *interface =
                                                  QV4::Compiler::defaultCodegenWarningInterface(),
                                          const QString *fileContents = nullptr);
bool Q_QMLCOMPILER_PRIVATE_EXPORT qCompileQmlFile(QmlIR::Document &irDocument, const QString &inputFileName,
                                          QQmlJSSaveFunction saveFunction,
                                          QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error,
                                          bool storeSourceLocation = false,
                                          QV4::Compiler::CodegenWarningInterface *interface =
                                                  QV4::Compiler::defaultCodegenWarningInterface(),
                                          const QString *fileContents = nullptr);
bool Q_QMLCOMPILER_PRIVATE_EXPORT qCompileJSFile(const QString &inputFileName, const QString &inputFileUrl,
                                         QQmlJSSaveFunction saveFunction,
                                         QQmlJSCompileError *error);

bool Q_QMLCOMPILER_PRIVATE_EXPORT qSaveQmlJSUnitAsCpp(const QString &inputFileName,
                                              const QString &outputFileName,
                                              const QV4::CompiledData::SaveableUnitPointer &unit,
                                              const QQmlJSAotFunctionMap &aotFunctions,
                                              QString *errorString);

QT_END_NAMESPACE

#endif // QQMLJSCOMPILER_P_H
