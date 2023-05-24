// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generatorbase.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>

#include "utils.h"
#include "commontemplates.h"
#include "generatorcommon.h"
#include "options.h"

#include <cassert>

using namespace ::google::protobuf;
using namespace ::google::protobuf::io;
using namespace ::google::protobuf::compiler;
using namespace ::qtprotoccommon;

bool GeneratorBase::GenerateAll(const std::vector<const FileDescriptor *> &files,
                                const std::string &parameter, GeneratorContext *generatorContext,
                                std::string *error) const
{
    Options::setFromString(parameter);
    return CodeGenerator::GenerateAll(files, parameter, generatorContext, error);
}

std::string GeneratorBase::generateBaseName(const FileDescriptor *file, const std::string &name)
{
    std::string outFileBasename;
    if (Options::instance().isFolder()) {
        outFileBasename = file->package();
        if (!outFileBasename.empty()) {
            outFileBasename = utils::replace(outFileBasename, ".", "/");
            outFileBasename += '/';
        }
    }
    outFileBasename += name;

    return outFileBasename;
}

void GeneratorBase::printDisclaimer(Printer *printer)
{
    printer->Print(CommonTemplates::DisclaimerTemplate());
}

void GeneratorBase::OpenFileNamespaces(
        const FileDescriptor *file,
        google::protobuf::io::Printer *printer) const
{
    assert(printer != nullptr);
    assert(file != nullptr);
    const bool hasQtNamespace = (Options::instance().extraNamespace() == "QT_NAMESPACE");
    const std::string scopeNamespaces = file->message_type_count() > 0
            ? common::getFullNamespace(file->message_type(0), "::")
            : common::getFullNamespace(file->enum_type(0), "::");
    printer->Print("\n");
    if (hasQtNamespace || file->package() == "QtCore" || file->package() == "QtGui")
        printer->PrintRaw("QT_BEGIN_NAMESPACE\n");
    if (!scopeNamespaces.empty()) {
        printer->Print({ { "scope_namespaces", scopeNamespaces } },
                       CommonTemplates::NamespaceTemplate());
    }
}

void GeneratorBase::CloseFileNamespaces(
        const ::google::protobuf::FileDescriptor *file,
        google::protobuf::io::Printer *printer) const
{
    assert(printer != nullptr);
    const bool hasQtNamespace = (Options::instance().extraNamespace() == "QT_NAMESPACE");
    const std::string scopeNamespaces = file->message_type_count() > 0
            ? common::getFullNamespace(file->message_type(0), "::")
            : common::getFullNamespace(file->enum_type(0), "::");
    if (!scopeNamespaces.empty()) {
        printer->Print({ { "scope_namespaces", scopeNamespaces } },
                       CommonTemplates::NamespaceClosingTemplate());
    }
    if (hasQtNamespace || file->package() == "QtCore" || file->package() == "QtGui")
        printer->PrintRaw("QT_END_NAMESPACE\n");
    printer->Print("\n");
}
