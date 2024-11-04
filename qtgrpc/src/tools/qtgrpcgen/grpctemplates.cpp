// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "grpctemplates.h"

using namespace ::QtGrpc;
const char *GrpcTemplates::ChildClassDeclarationTemplate()
{
    return "\nclass $export_macro$ $classname$ : public $parent_class$\n"
           "{\n"
           "    Q_OBJECT\n";
}

const char *GrpcTemplates::ClientQmlDeclarationTemplate()
{
    return "    QML_NAMED_ELEMENT($service_name$Client)\n";
}

const char *GrpcTemplates::ClientMethodDeclarationAsyncTemplate()
{
    return "std::shared_ptr<QGrpcCallReply> $method_name$(const $param_type$ &$param_name$, const "
           "QGrpcCallOptions &options = {});\n";
}

const char *GrpcTemplates::ClientMethodDeclarationAsync2Template()
{
    return "Q_INVOKABLE void $method_name$(const $param_type$ &$param_name$, const QObject "
           "*context, "
           "const std::function<void(std::shared_ptr<QGrpcCallReply>)> &callback, const "
           "QGrpcCallOptions &options = {});\n";
}

const char *GrpcTemplates::ClientMethodDeclarationQmlTemplate()
{
    return "Q_INVOKABLE void $method_name$(const $param_type$ &$param_name$, "
           "const QJSValue &callback, "
           "const QJSValue &errorCallback, "
           "const QGrpcCallOptions &options = {});\n";
}

const char *GrpcTemplates::ServerMethodDeclarationTemplate()
{
    return "Q_INVOKABLE virtual $return_type$ $method_name$(const $param_type$ &$param_name$) = "
           "0;\n";
}

const char *GrpcTemplates::ClientConstructorDefinitionTemplate()
{
    return "\n$classname$::$classname$(QObject *parent)\n"
           "    : $parent_class$(\"$service_name$\"_L1, "
           "parent)\n"
           "{\n"
           "}\n\n";
}

const char *GrpcTemplates::ClientQmlConstructorDefinitionTemplate()
{
    return "\n$classname$::$classname$(QObject *parent)\n"
           "    : $parent_class$(parent)\n"
           "{\n"
           "}\n\n";
}

const char *GrpcTemplates::ClientMethodDefinitionAsyncTemplate()
{
    return "\nstd::shared_ptr<QGrpcCallReply> $classname$::$method_name$(const $param_type$ "
           "&$param_name$, const QGrpcCallOptions &options)\n"
           "{\n"
           "    return call<$param_type$>(\"$method_name$\"_L1, $param_name$, options);\n"
           "}\n";
}

const char *GrpcTemplates::ClientMethodDefinitionAsync2Template()
{
    return "\nvoid $classname$::$method_name$(const $param_type$ &$param_name$, const QObject "
           "*context, const std::function<void(std::shared_ptr<QGrpcCallReply>)> &callback, const "
           "QGrpcCallOptions &options)\n"
           "{\n"
           "    std::shared_ptr<QGrpcCallReply> reply = call<$param_type$>(\"$method_name$\"_L1, "
           "$param_name$, options);\n"
           "    QObject::connect(reply.get(), &QGrpcCallReply::finished, context, [reply, "
           "callback]() "
           "{\n"
           "        callback(reply);\n"
           "    }, Qt::SingleShotConnection);\n"
           "}\n\n";
}

const char *GrpcTemplates::ClientMethodDefinitionQmlTemplate()
{
    return "\nvoid $classname$::$method_name$(const $param_type$ &$param_name$, const QJSValue "
           "&callback, "
           "const QJSValue &errorCallback,"
           "const QGrpcCallOptions &options)\n"
           "{\n"
           "    if (!callback.isCallable()) {\n"
           "        qWarning() << \"Unable to call $classname$::$method_name$, callback is not "
           "callable\";\n"
           "        return;\n"
           "    }\n\n"
           "    QJSEngine *jsEngine = qjsEngine(this);\n"
           "    if (jsEngine == nullptr) {\n"
           "        qWarning() << \"Unable to call $classname$::$method_name$, it's only callable "
           "from JS engine context\";\n"
           "        return;\n"
           "    }\n\n"
           "    std::shared_ptr<QGrpcCallReply> reply = call<$param_type$>(\"$method_name$\"_L1, "
           "$param_name$, options);\n"
           "    reply->subscribe(jsEngine, [reply, callback, jsEngine]() {\n"
           "        auto result = reply->read<$return_type$>();\n"
           "        callback.call(QJSValueList{jsEngine->toScriptValue(result)});\n"
           "    }, [errorCallback, jsEngine](const QGrpcStatus &status) {\n"
           "        errorCallback.call(QJSValueList{jsEngine->toScriptValue(status)});\n"
           "    });\n"
           "}\n";
}

const char *GrpcTemplates::ClientMethodStreamDeclarationTemplate()
{
    return "std::shared_ptr<$stream_type$> stream$method_name_upper$(const $param_type$ "
           "&$param_name$, const QGrpcCallOptions &options = {});\n";
}

const char *GrpcTemplates::ClientMethodStreamDefinitionTemplate()
{
    return "std::shared_ptr<$stream_type$> $classname$::stream$method_name_upper$("
           "const $param_type$ &$param_name$, const QGrpcCallOptions &options)\n"
           "{\n"
           "    return startStream<$param_type$, $stream_type$>(\"$method_name$\"_L1, "
           "$param_name$, options);\n"
           "}\n\n";
}

const char *GrpcTemplates::GrpcClientFileSuffix()
{
    return "_client.grpc";
}

const char *GrpcTemplates::GrpcServiceFileSuffix()
{
    return "_service.grpc";
}
