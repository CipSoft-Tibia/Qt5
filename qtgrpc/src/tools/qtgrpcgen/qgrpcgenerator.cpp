// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qgrpcgenerator.h"
#include "clientdeclarationprinter.h"
#include "clientdefinitionprinter.h"
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

QGrpcGenerator::QGrpcGenerator() : GeneratorBase()
{}

QGrpcGenerator::~QGrpcGenerator() = default;

bool QGrpcGenerator::Generate(const FileDescriptor *file,
                              [[maybe_unused]] const std::string &parameter,
                              GeneratorContext *generatorContext,
                              [[maybe_unused]] std::string *error) const
{
    assert(file != nullptr && generatorContext != nullptr);

    // Check if .proto files contain client side or bidirectional streaming
    // methods which are not supported.
    bool hasClientStreaming = false;
    for (int i = 0; i < file->service_count() && !hasClientStreaming; ++i) {
        auto service = file->service(i);
        assert(service != nullptr);

        for (int j = 0; j < service->method_count(); ++j) {
            if (service->method(j)->client_streaming()) {
                hasClientStreaming = true;
                break;
            }
        }
    }

    if (hasClientStreaming)
        std::cerr << "Client-side streaming is not supported by this QtGRPC version.";

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
void QGrpcGenerator::RunPrinter(const FileDescriptor *file, std::shared_ptr<Printer> printer)
{
    assert(file != nullptr);
    for (int i = 0; i < file->service_count(); ++i) {
        const ServiceDescriptor *service = file->service(i);

        ServicePrinterT servicePrinter(service, printer);
        servicePrinter.run();
    }
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
    std::unique_ptr<ZeroCopyOutputStream> clientHeaderStream(
                generatorContext->Open(clientFileName + ".h"));
    std::unique_ptr<ZeroCopyOutputStream> clientSourceStream(
                generatorContext->Open(clientFileName + ".cpp"));

    std::shared_ptr<Printer> clientHeaderPrinter(new Printer(clientHeaderStream.get(), '$'));
    std::shared_ptr<Printer> clientSourcePrinter(new Printer(clientSourceStream.get(), '$'));

    printDisclaimer(clientHeaderPrinter.get());

    std::string fileNameToUpper = filename + "_client";
    std::transform(fileNameToUpper.begin(), fileNameToUpper.end(),
                   fileNameToUpper.begin(), utils::toAsciiUpper);

    clientHeaderPrinter->Print({ { "filename", fileNameToUpper } },
                               CommonTemplates::PreambleTemplate());

    clientHeaderPrinter->Print(CommonTemplates::DefaultProtobufIncludesTemplate());
    if (Options::instance().hasQml()) {
        clientHeaderPrinter->Print({ { "include", "QtQml/qjsvalue.h" } },
                                   CommonTemplates::ExternalIncludeTemplate());
        clientHeaderPrinter->Print(CommonTemplates::QmlProtobufIncludesTemplate());
    }

    printDisclaimer(clientSourcePrinter.get());
    clientSourcePrinter->Print({ { "include", clientFileName } },
                               CommonTemplates::InternalIncludeTemplate());

    std::set<std::string> externalIncludes = {"QtGrpc/qabstractgrpcclient.h",
                                                     "QtGrpc/qgrpccallreply.h",
                                                     "QtGrpc/qgrpcstream.h"};
    for (const auto &include : externalIncludes) {
        clientHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    clientHeaderPrinter->Print(CommonTemplates::DefaultSystemIncludesTemplate());
    clientHeaderPrinter->Print("\n");

    std::set<std::string> internalIncludes = QGrpcGenerator::GetInternalIncludes(file);
    for (const auto &include : internalIncludes) {
        clientHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::InternalIncludeTemplate());
    }

    if (Options::instance().hasQml()) {
        clientSourcePrinter->Print({ { "include", "QtQml/qqmlengine.h" } },
                                   CommonTemplates::ExternalIncludeTemplate());
        clientSourcePrinter->Print({ { "include", "QtQml/qjsengine.h" } },
                                   CommonTemplates::ExternalIncludeTemplate());
        clientSourcePrinter->Print({ { "include", "QtQml/qjsvalue.h" } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    clientHeaderPrinter->PrintRaw("\n");
    if (!Options::instance().exportMacro().empty()) {
        clientHeaderPrinter->Print({ { "export_macro", Options::instance().exportMacro() } },
                                   CommonTemplates::ExportMacroTemplate());
    }

    OpenFileNamespaces(file, clientHeaderPrinter.get());
    OpenFileNamespaces(file, clientSourcePrinter.get());

    QGrpcGenerator::RunPrinter<ClientDeclarationPrinter>(file, clientHeaderPrinter);
    QGrpcGenerator::RunPrinter<ClientDefinitionPrinter>(file, clientSourcePrinter);

    CloseFileNamespaces(file, clientHeaderPrinter.get());
    CloseFileNamespaces(file, clientSourcePrinter.get());

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

    serverHeaderPrinter->PrintRaw("\n");
    if (!Options::instance().exportMacro().empty()) {
        serverHeaderPrinter->Print({ { "export_macro", Options::instance().exportMacro() } },
                                   CommonTemplates::ExportMacroTemplate());
    }
    OpenFileNamespaces(file, serverHeaderPrinter.get());

    QGrpcGenerator::RunPrinter<ServerDeclarationPrinter>(file, serverHeaderPrinter);

    CloseFileNamespaces(file, serverHeaderPrinter.get());

    serverHeaderPrinter->Print({ { "filename", filename + "_service" } },
                               CommonTemplates::FooterTemplate());
    return true;
}

bool QGrpcGenerator::GenerateAll(const std::vector<const FileDescriptor *> &files,
                                 const std::string &parameter, GeneratorContext *generatorContext,
                                 std::string *error) const
{
    Options::setFromString(parameter, qtprotoccommon::Options::QtGrpcGen);
    return CodeGenerator::GenerateAll(files, parameter, generatorContext, error);
}
