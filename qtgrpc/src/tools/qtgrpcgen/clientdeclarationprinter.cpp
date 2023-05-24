// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientdeclarationprinter.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>

#include <string>
#include <unordered_set>

#include "generatorcommon.h"
#include "commontemplates.h"
#include "grpctemplates.h"

using namespace ::QtGrpc;
using namespace ::qtprotoccommon;
using namespace ::google::protobuf;
using namespace ::google::protobuf::compiler;

/*!
   \class ClientDeclarationPrinter
   \inmodule qtprotobufgen
   \private

   \brief Generates gRPC client class declaration.
*/

ClientDeclarationPrinter::ClientDeclarationPrinter(
        const ::google::protobuf::ServiceDescriptor *service,
        const std::shared_ptr<::google::protobuf::io::Printer> &printer)
    : DescriptorPrinterBase<::google::protobuf::ServiceDescriptor>(
          service, printer, common::produceClientTypeMap(service, nullptr))

{
}

void ClientDeclarationPrinter::printOpenNamespace()
{
    m_printer->Print({ { "scope_namespaces", m_typeMap["scope_type"] } },
                     CommonTemplates::NamespaceTemplate());
}

void ClientDeclarationPrinter::printClientClass()
{
    m_printer->Print(m_typeMap, GrpcTemplates::ChildClassDeclarationTemplate());
    if (Options::instance().hasQml())
        m_printer->Print(m_typeMap, GrpcTemplates::ClientQmlDeclarationTemplate());
}

void ClientDeclarationPrinter::printConstructor()
{
    Indent();
    m_printer->Print(m_typeMap, CommonTemplates::QObjectConstructorMessageDeclarationTemplate());
    Outdent();
}

void ClientDeclarationPrinter::printClientMethodsDeclaration()
{
    Indent();
    for (int i = 0; i < m_descriptor->method_count(); i++) {
        const MethodDescriptor *method = m_descriptor->method(i);
        MethodMap parameters = common::produceMethodMap(method, m_typeMap["classname"]);

        if (!method->client_streaming()) {
            if (method->server_streaming()) {
                m_printer->Print(parameters,
                                 GrpcTemplates::ClientMethodServerStreamDeclarationTemplate());
            } else {
                m_printer->Print(parameters, GrpcTemplates::ClientMethodDeclarationSyncTemplate());
                m_printer->Print(parameters, GrpcTemplates::ClientMethodDeclarationAsyncTemplate());
                m_printer->Print(parameters,
                                 GrpcTemplates::ClientMethodDeclarationAsync2Template());
                if (Options::instance().hasQml())
                    m_printer->Print(parameters,
                                     GrpcTemplates::ClientMethodDeclarationQmlTemplate());
            }
            m_printer->Print("\n");
        }
    }
    Outdent();
    m_printer->Print("\n");
}

void ClientDeclarationPrinter::printCloseNamespace()
{
    m_printer->Print({ { "scope_namespaces", m_typeMap["scope_type"] } },
                     CommonTemplates::NamespaceClosingTemplate());
}
