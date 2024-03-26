// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QPROTOBUFGENERATOR_H
#define QPROTOBUFGENERATOR_H

#include "generatorbase.h"

#include <memory>
#include <set>

namespace google::protobuf {
class FileDescriptor;
class Descriptor;
namespace compiler {
class GeneratorContext;
} // namespace compiler
} // namespace google::protobuf

namespace QtProtobuf {
class QProtobufGenerator : public qtprotoccommon::GeneratorBase
{
public:
    QProtobufGenerator();
    ~QProtobufGenerator();
    bool GenerateAll(const std::vector<const ::google::protobuf::FileDescriptor *> &files,
                     const std::string &parameter,
                     ::google::protobuf::compiler::GeneratorContext *generatorContext,
                     std::string *error) const override;
    bool Generate(const ::google::protobuf::FileDescriptor *file,
                  const std::string &parameter,
                  ::google::protobuf::compiler::GeneratorContext *generatorContext,
                  std::string *error) const override;
private:
    bool GenerateMessages(const ::google::protobuf::FileDescriptor *file,
                          ::google::protobuf::compiler::GeneratorContext *generatorContext) const;
    void GenerateHeader(const ::google::protobuf::FileDescriptor *file,
                        ::google::protobuf::compiler::GeneratorContext *generatorContext) const;
    void GenerateSources(const ::google::protobuf::FileDescriptor *file,
                         ::google::protobuf::compiler::GeneratorContext *generatorContext) const;
    void GenerateQmlPluginSource(const ::google::protobuf::FileDescriptor *file,
                                 std::shared_ptr<google::protobuf::io::Printer> printer) const;
    void GenerateQmlPluginIntro(::google::protobuf::io::Printer *printer,
                                const std::set<std::string> &headersContainer,
                                const std::string &packageName) const;
};
} // namespace QtProtobuf

#endif // QPROTOBUFGENERATOR_H
