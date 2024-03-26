// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "enumdeclarationprinter.h"
#include "utils.h"
#include "generatorcommon.h"

using namespace ::QtProtobuf;
using namespace ::qtprotoccommon;
using namespace ::google::protobuf;
using namespace ::google::protobuf::io;

EnumDeclarationPrinter::EnumDeclarationPrinter(const EnumDescriptor *descriptor,
                                               std::shared_ptr<Printer> printer)
    : DescriptorPrinterBase<EnumDescriptor>(descriptor, std::move(printer),
                                            common::produceEnumTypeMap(descriptor, nullptr))
{
}

EnumDeclarationPrinter::~EnumDeclarationPrinter() = default;

void EnumDeclarationPrinter::startEnum()
{
    printEnumClass();
    if (!m_typeMap["export_macro"].empty())
        m_printer->Print(m_typeMap, CommonTemplates::EnumRegistrationDeclaration());
    else
        m_printer->Print(m_typeMap, CommonTemplates::EnumRegistrationDeclarationNoExport());
}

void EnumDeclarationPrinter::printEnum()
{
    auto typeMap = common::produceEnumTypeMap(m_descriptor, nullptr);

    m_printer->Print(typeMap, CommonTemplates::EnumDefinitionTemplate());

    Indent();
    int numValues = m_descriptor->value_count();
    for (int j = 0; j < numValues; ++j) {
        const EnumValueDescriptor *valueDescr = m_descriptor->value(j);
        m_printer->Print({ { "enumvalue", common::qualifiedCppName(valueDescr->name()) },
                           { "value", std::to_string(valueDescr->number()) } },
                         CommonTemplates::EnumFieldTemplate());
    }
    Outdent();
    m_printer->Print(CommonTemplates::SemicolonBlockEnclosureTemplate());
    m_printer->Print(typeMap, CommonTemplates::QEnumNSTemplate());
    m_printer->Print(typeMap, CommonTemplates::UsingRepeatedEnumTemplate());
}

void EnumDeclarationPrinter::printEnumClass()
{
    if (!m_typeMap["export_macro"].empty())
        m_printer->Print(m_typeMap, CommonTemplates::EnumDeclarationTemplate());
    else
        m_printer->Print(m_typeMap, CommonTemplates::EnumDeclarationNoExportTemplate());
}
