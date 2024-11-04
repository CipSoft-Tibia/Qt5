// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qgrpcgenerator.h"
#include "clientdeclarationprinter.h"
#include "clientdefinitionprinter.h"
#include "qmlclientdeclarationprinter.h"
#include "qmlclientdefinitionprinter.h"
#include "serverdeclarationprinter.h"

#include "grpctemplates.h"
#include "utils.h"
#include "options.h"

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>

using namespace ::QtGrpc;
using namespace ::qtprotoccommon;
using namespace ::google::protobuf;
using namespace ::google::protobuf::io;
using namespace ::google::protobuf::compiler;

static const std::set<std::string> externalQmlIncludes = {"QtQml/qqmlengine.h",
                                                          "QtQml/qjsengine.h",
                                                          "QtQml/qjsvalue.h"};

static const std::set<std::string> externalIncludes = {"QtGrpc/qabstractgrpcclient.h",
                                                       "QtGrpc/qgrpccallreply.h",
                                                       "QtGrpc/qgrpcstream.h"};

static std::string stringToUpper(std::string str)
{
    std::transform(str.begin(), str.end(),
                   str.begin(), utils::toAsciiUpper);
    return str;
}

QGrpcGenerator::QGrpcGenerator() : GeneratorBase()
{}

QGrpcGenerator::~QGrpcGenerator() = default;

bool QGrpcGenerator::Generate(const FileDescriptor *file,
                              [[maybe_unused]] const std::string &parameter,
                              GeneratorContext *generatorContext,
                              [[maybe_unused]] std::string *error) const
{
    assert(file != nullptr && generatorContext != nullptr);

    return GenerateClientServices(file, generatorContext);
}

std::set<std::string> QGrpcGenerator::GetInternalIncludes(const FileDescriptor *file)
{
    std::set<std::string> includes;
    assert(file != nullptr);
    for (int i = 0; i < file->service_count(); ++i) {
        const ServiceDescriptor *service = file->service(i);
        for (int i = 0; i < service->method_count(); ++i) {
            const MethodDescriptor *method = service->method(i);
            if (method->input_type()->file() != service->file()) {
                includes.insert(utils::removeFileSuffix(method->input_type()->file()->name())
                                + CommonTemplates::ProtoFileSuffix());
            }

            if (method->output_type()->file() != service->file()) {
                includes.insert(utils::removeFileSuffix(method->output_type()->file()->name())
                                + CommonTemplates::ProtoFileSuffix());
            }
        }
    }
    if (file->message_type_count() > 0) {
        includes.insert(generateBaseName(file, utils::extractFileBasename(file->name()))
                        + CommonTemplates::ProtoFileSuffix());
    }
    return includes;
}

template <typename ServicePrinterT>
void QGrpcGenerator::RunPrinter(const FileDescriptor *file, std::shared_ptr<Printer> printer) const
{
    assert(file != nullptr);
    OpenFileNamespaces(file, printer.get());
    for (int i = 0; i < file->service_count(); ++i) {
        const ServiceDescriptor *service = file->service(i);

        ServicePrinterT servicePrinter(service, printer);
        servicePrinter.run();
    }
    CloseFileNamespaces(file, printer.get());
}

void QGrpcGenerator::GenerateQmlClientServices(
        const ::google::protobuf::FileDescriptor *file,
        ::google::protobuf::compiler::GeneratorContext *generatorContext) const
{
    assert(file != nullptr);
    assert(generatorContext != nullptr);

    const std::string filename = utils::extractFileBasename(file->name());
    const std::string basename = generateBaseName(file, filename);
    const std::string clientFileName = basename
            + GrpcTemplates::GrpcClientFileSuffix() + CommonTemplates::ProtoFileSuffix();
    const std::string qmlPrefix = "qml";
    // QML registered client class
    std::unique_ptr<ZeroCopyOutputStream> clientQmlHeaderStream(
                generatorContext->Open(qmlPrefix + clientFileName + ".h"));
    std::unique_ptr<ZeroCopyOutputStream> clientQmlSourceStream(
                generatorContext->Open(qmlPrefix + clientFileName + ".cpp"));

    std::shared_ptr<Printer> qmlHeaderPrinter(new Printer(clientQmlHeaderStream.get(), '$'));
    std::shared_ptr<Printer> qmlSourcePrinter(new Printer(clientQmlSourceStream.get(), '$'));

    printDisclaimer(qmlHeaderPrinter.get());
    printDisclaimer(qmlSourcePrinter.get());
    std::string fileNameToUpper = stringToUpper(qmlPrefix + filename + "_client");
    qmlHeaderPrinter->Print({ { "filename", fileNameToUpper } },
                            CommonTemplates::PreambleTemplate());
    qmlHeaderPrinter->Print({ { "include", clientFileName } },
                            CommonTemplates::InternalIncludeTemplate());

    for (const auto &include : externalQmlIncludes) {
        qmlHeaderPrinter->Print({ { "include", include } },
                                CommonTemplates::ExternalIncludeTemplate());
    }
    qmlSourcePrinter->Print({ { "include", qmlPrefix + clientFileName } },
                            CommonTemplates::InternalIncludeTemplate());

    QGrpcGenerator::RunPrinter<QmlClientDeclarationPrinter>(file, qmlHeaderPrinter);
    QGrpcGenerator::RunPrinter<QmlClientDefinitionPrinter>(file, qmlSourcePrinter);
    qmlHeaderPrinter->Print({ { "filename", fileNameToUpper } },
                            CommonTemplates::FooterTemplate());
}

bool QGrpcGenerator::GenerateClientServices(const FileDescriptor *file,
                                            GeneratorContext *generatorContext) const
{
    assert(file != nullptr && generatorContext != nullptr);
    if (file->service_count() <= 0)
        return true;

    const std::string filename = utils::extractFileBasename(file->name());
    const std::string basename = generateBaseName(file, filename);
    const std::string clientFileName = basename
            + GrpcTemplates::GrpcClientFileSuffix() + CommonTemplates::ProtoFileSuffix();

    // Generate QML class
    if (Options::instance().hasQml())
        GenerateQmlClientServices(file, generatorContext);

    // CPP client class
    std::unique_ptr<ZeroCopyOutputStream> clientHeaderStream(
                generatorContext->Open(clientFileName + ".h"));
    std::unique_ptr<ZeroCopyOutputStream> clientSourceStream(
                generatorContext->Open(clientFileName + ".cpp"));

    std::shared_ptr<Printer> clientHeaderPrinter(new Printer(clientHeaderStream.get(), '$'));
    std::shared_ptr<Printer> clientSourcePrinter(new Printer(clientSourceStream.get(), '$'));

    printDisclaimer(clientHeaderPrinter.get());
    printDisclaimer(clientSourcePrinter.get());

    std::string fileNameToUpper = stringToUpper(filename + "_client");

    clientHeaderPrinter->Print({ { "filename", fileNameToUpper } },
                               CommonTemplates::PreambleTemplate());
    clientHeaderPrinter->Print(CommonTemplates::DefaultProtobufIncludesTemplate());
    clientSourcePrinter->Print({ { "include", clientFileName } },
                               CommonTemplates::InternalIncludeTemplate());

    for (const auto &include : externalIncludes) {
        clientHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    clientHeaderPrinter->Print(CommonTemplates::DefaultSystemIncludesTemplate());
    clientHeaderPrinter->Print("\n");

    std::set<std::string> internalIncludes = QGrpcGenerator::GetInternalIncludes(file);
    if (!Options::instance().exportMacroFilename().empty()) {
        std::string exportMacroFilename = Options::instance().exportMacroFilename();
        internalIncludes.insert(utils::removeFileSuffix(exportMacroFilename));
    }

    for (const auto &include : internalIncludes) {
        clientHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::InternalIncludeTemplate());
    }
    QGrpcGenerator::RunPrinter<ClientDeclarationPrinter>(file, clientHeaderPrinter);
    QGrpcGenerator::RunPrinter<ClientDefinitionPrinter>(file, clientSourcePrinter);
    clientHeaderPrinter->Print({ { "filename", fileNameToUpper } },
                               CommonTemplates::FooterTemplate());

    return true;
}

bool QGrpcGenerator::GenerateServerServices(const FileDescriptor *file,
                                            GeneratorContext *generatorContext) const
{
    assert(file != nullptr && generatorContext != nullptr);
    if (file->service_count() <= 0)
        return true;

    const std::string filename = utils::extractFileBasename(file->name());
    const std::string basename = generateBaseName(file, filename);
    std::unique_ptr<ZeroCopyOutputStream> serviceHeaderStream(
            generatorContext->Open(basename + GrpcTemplates::GrpcServiceFileSuffix()
                                   + CommonTemplates::ProtoFileSuffix() + ".h"));
    std::shared_ptr<Printer> serverHeaderPrinter(new Printer(serviceHeaderStream.get(), '$'));

    printDisclaimer(serverHeaderPrinter.get());
    serverHeaderPrinter->Print({ { "filename", filename + "_service" } },
                               CommonTemplates::PreambleTemplate());

    serverHeaderPrinter->Print(CommonTemplates::DefaultProtobufIncludesTemplate());
    if (Options::instance().hasQml())
        serverHeaderPrinter->Print(CommonTemplates::QmlProtobufIncludesTemplate());

    serverHeaderPrinter->Print(CommonTemplates::DefaultSystemIncludesTemplate());

    std::set<std::string> externalIncludes;
    for (const auto &include : externalIncludes) {
        serverHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    const std::string serviceIncludes("QAbstractGrpcService");
    serverHeaderPrinter->Print({ { "include", serviceIncludes } },
                               CommonTemplates::ExternalIncludeTemplate());

    std::set<std::string> internalIncludes = QGrpcGenerator::GetInternalIncludes(file);
    for (const auto &include : internalIncludes) {
        serverHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::InternalIncludeTemplate());
    }
    QGrpcGenerator::RunPrinter<ServerDeclarationPrinter>(file, serverHeaderPrinter);
    serverHeaderPrinter->Print({ { "filename", filename + "_service" } },
                               CommonTemplates::FooterTemplate());
    return true;
}

bool QGrpcGenerator::GenerateAll(const std::vector<const FileDescriptor *> &files,
                                 const std::string &parameter, GeneratorContext *generatorContext,
                                 std::string *error) const
{
    Options::setFromString(parameter, qtprotoccommon::Options::QtGrpcGen);
    return GeneratorBase::GenerateAll(files, parameter, generatorContext, error);
}
