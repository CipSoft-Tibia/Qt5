// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientdefinitionprinter.h"

#include <google/protobuf/io/zero_copy_stream.h>

#include "generatorcommon.h"
#include "commontemplates.h"
#include "grpctemplates.h"

using namespace ::QtGrpc;
using namespace ::qtprotoccommon;
using namespace ::google::protobuf;
using namespace ::google::protobuf::compiler;

/*!
    \class ClientDefinitionPrinter
    \inmodule qtprotobufgen
    \private

    \brief Generates gRPC client class definition.
*/

ClientDefinitionPrinter::ClientDefinitionPrinter(
        const google::protobuf::ServiceDescriptor *service,
        const std::shared_ptr<::google::protobuf::io::Printer> &printer)
    : DescriptorPrinterBase<google::protobuf::ServiceDescriptor>(
          service, printer, common::produceClientTypeMap(service, nullptr))
{
}

void ClientDefinitionPrinter::printOpenNamespace()
{
    m_printer->Print({ { "scope_namespaces", m_typeMap["scope_type"] } },
                     CommonTemplates::NamespaceTemplate());
    m_printer->Print({ { "namespace", "Qt::StringLiterals" } }, CommonTemplates::UseNamespace());
}

void ClientDefinitionPrinter::printMethods()
{
    for (int i = 0; i < m_descriptor->method_count(); i++) {
        const MethodDescriptor *method = m_descriptor->method(i);
        printMethod(method);
    }
}

void ClientDefinitionPrinter::printMethod(const MethodDescriptor *method)
{
    MethodMap parameters = common::produceMethodMap(method, m_typeMap["classname"]);
    if (!method->client_streaming()) {
        if (method->server_streaming()) {
            m_printer->Print(parameters,
                             GrpcTemplates::ClientMethodServerStreamDefinitionTemplate());
        } else {
            m_printer->Print(parameters, GrpcTemplates::ClientMethodDefinitionSyncTemplate());
            m_printer->Print(parameters, GrpcTemplates::ClientMethodDefinitionAsyncTemplate());
            m_printer->Print(parameters, GrpcTemplates::ClientMethodDefinitionAsync2Template());
            if (Options::instance().hasQml())
                m_printer->Print(parameters, GrpcTemplates::ClientMethodDefinitionQmlTemplate());
        }
    }
}

void ClientDefinitionPrinter::printConstructor()
{
    m_printer->Print({ { "classname", m_typeMap["classname"] },
                       { "parent_class", m_typeMap["parent_class"] },
                       { "service_name", m_descriptor->full_name() } },
                     GrpcTemplates::ClientConstructorDefinitionTemplate());
}

void ClientDefinitionPrinter::printCloseNamespace()
{
    m_printer->Print({ { "scope_namespaces", m_typeMap["scope_type"] } },
                     CommonTemplates::NamespaceClosingTemplate());
}
