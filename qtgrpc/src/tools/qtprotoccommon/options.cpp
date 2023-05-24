// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "options.h"

#include "utils.h"

using namespace ::qtprotoccommon;
static const char QmlPluginOption[] = "QML";
static const char QmlPluginUriOption[] = "QML_URI";
static const char CommentsGenerationOption[] = "COPY_COMMENTS";
static const char FolderGenerationOption[] = "GENERATE_PACKAGE_SUBFOLDERS";
static const char FieldEnumGenerationOption[] = "FIELD_ENUM";
static const char ExtraNamespaceGenerationOption[] = "EXTRA_NAMESPACE";
static const char ExportMacroGenerationOption[] = "EXPORT_MACRO";

Options::Options()
    : m_generateComments(false), m_isFolder(false), m_generateFieldEnum(true), m_qml(false)
{
}

Options::~Options() = default;

Options &Options::mutableInstance()
{
    static Options _instance;
    return _instance;
}

const Options &Options::instance()
{
    return mutableInstance();
}

std::string extractCompositeOptionValue(const std::string &option)
{
    std::vector<std::string> compositeOption = utils::split(option, "=");
    if (compositeOption.size() != 2)
        return {};
    const std::string &optionValue = compositeOption.back();
    if (utils::startsWith(optionValue, '\"') && utils::endsWith(optionValue, '\"')) {
        return optionValue.substr(1, optionValue.size() - 2);
    }
    return optionValue;
}

void Options::setFromString(const std::string &options, GeneratorType type)
{
    Options &instance = mutableInstance();
    for (const auto &option : utils::split(options, ";")) {
        QT_PROTOBUF_DEBUG("option: " << option);
        if (option == CommentsGenerationOption) {
            QT_PROTOBUF_DEBUG("set m_generateComments: true");
            instance.m_generateComments = true;
        } else if (option == FolderGenerationOption) {
            QT_PROTOBUF_DEBUG("set m_isFolder: true");
            instance.m_isFolder = true;
        } else if (option == FieldEnumGenerationOption) {
            // TODO: Opt in this option by default since it's required for
            // Oneof fields.
            // QT_PROTOBUF_DEBUG("set m_generateFieldEnum: true");
            // instance.m_generateFieldEnum = true;
        } else if (option.find(ExtraNamespaceGenerationOption) == 0) {
            instance.m_extraNamespace = extractCompositeOptionValue(option);
            QT_PROTOBUF_DEBUG("set m_extraNamespace: " << instance.m_extraNamespace);
        } else if (option.find(ExportMacroGenerationOption) == 0) {
            instance.m_exportMacro = extractCompositeOptionValue(option);
            QT_PROTOBUF_DEBUG("set m_exportMacro: " << instance.m_exportMacro);
        } else if (option.find(QmlPluginUriOption) == 0 && type != QtGrpcGen) {
            instance.m_qmlUri = extractCompositeOptionValue(option);
            QT_PROTOBUF_DEBUG("set m_qmlUri: " << instance.m_qmlUri);
        } else if (option == QmlPluginOption && type == QtGrpcGen) {
            instance.m_qml = true;
            QT_PROTOBUF_DEBUG("set m_qml: true");
        }
    }
}
