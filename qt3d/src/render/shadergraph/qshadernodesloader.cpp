// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qshadernodesloader_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender
{
QShaderNodesLoader::QShaderNodesLoader() noexcept
    : m_status(Null),
      m_device(nullptr)
{
}

QShaderNodesLoader::Status QShaderNodesLoader::status() const noexcept
{
    return m_status;
}

QHash<QString, QShaderNode> QShaderNodesLoader::nodes() const noexcept
{
    return m_nodes;
}

QIODevice *QShaderNodesLoader::device() const noexcept
{
    return m_device;
}

void QShaderNodesLoader::setDevice(QIODevice *device) noexcept
{
    m_device = device;
    m_nodes.clear();
    m_status = !m_device ? Null
             : (m_device->openMode() & QIODevice::ReadOnly) ? Waiting
             : Error;
}

void QShaderNodesLoader::load()
{
    if (m_status == Error)
        return;

    auto error = QJsonParseError();
    const QJsonDocument document = QJsonDocument::fromJson(m_device->readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Invalid JSON document:" << error.errorString();
        m_status = Error;
        return;
    }

    if (document.isEmpty() || !document.isObject()) {
        qWarning() << "Invalid JSON document, root should be an object";
        m_status = Error;
        return;
    }

    const QJsonObject root = document.object();
    load(root);
}

void QShaderNodesLoader::load(const QJsonObject &prototypesObject)
{
    bool hasError = false;

    for (const QString &property : prototypesObject.keys()) {
        const QJsonValue nodeValue = prototypesObject.value(property);
        if (!nodeValue.isObject()) {
            qWarning() << "Invalid node found";
            hasError = true;
            break;
        }

        const QJsonObject nodeObject = nodeValue.toObject();

        auto node = QShaderNode();

        const QJsonValue inputsValue = nodeObject.value(QStringLiteral("inputs"));
        if (inputsValue.isArray()) {
            const QJsonArray inputsArray = inputsValue.toArray();
            for (const QJsonValue inputValue : inputsArray) {
                if (!inputValue.isString()) {
                    qWarning() << "Non-string value in inputs";
                    hasError = true;
                    break;
                }

                auto input = QShaderNodePort();
                input.direction = QShaderNodePort::Input;
                input.name = inputValue.toString();
                node.addPort(input);
            }
        }

        const QJsonValue outputsValue = nodeObject.value(QStringLiteral("outputs"));
        if (outputsValue.isArray()) {
            const QJsonArray outputsArray = outputsValue.toArray();
            for (const QJsonValue outputValue : outputsArray) {
                if (!outputValue.isString()) {
                    qWarning() << "Non-string value in outputs";
                    hasError = true;
                    break;
                }

                auto output = QShaderNodePort();
                output.direction = QShaderNodePort::Output;
                output.name = outputValue.toString();
                node.addPort(output);
            }
        }

        const QJsonValue parametersValue = nodeObject.value(QStringLiteral("parameters"));
        if (parametersValue.isObject()) {
            const QJsonObject parametersObject = parametersValue.toObject();
            for (const QString &parameterName : parametersObject.keys()) {
                const QJsonValue parameterValue = parametersObject.value(parameterName);
                if (parameterValue.isObject()) {
                    const QJsonObject parameterObject = parameterValue.toObject();
                    const QString type = parameterObject.value(QStringLiteral("type")).toString();
                    const QMetaType typeId = QMetaType::fromName(type.toUtf8());

                    const QString value = parameterObject.value(QStringLiteral("value")).toString();
                    auto variant = QVariant(value);

                    if (typeId.flags() & QMetaType::IsEnumeration) {
                        const QMetaObject *metaObject = typeId.metaObject();
                        const char *className = metaObject->className();
                        const QByteArray enumName = type.mid(static_cast<int>(qstrlen(className)) + 2).toUtf8();
                        const QMetaEnum metaEnum = metaObject->enumerator(metaObject->indexOfEnumerator(enumName));
                        const int enumValue = metaEnum.keyToValue(value.toUtf8());
                        variant = QVariant(enumValue);
                        variant.convert(typeId);
                    } else {
                        variant.convert(typeId);
                    }
                    node.setParameter(parameterName, variant);
                } else {
                    node.setParameter(parameterName, parameterValue.toVariant());
                }
            }
        }

        const QJsonValue rulesValue = nodeObject.value(QStringLiteral("rules"));
        if (rulesValue.isArray()) {
            const QJsonArray rulesArray = rulesValue.toArray();
            for (const QJsonValue ruleValue : rulesArray) {
                if (!ruleValue.isObject()) {
                    qWarning() << "Rules should be objects";
                    hasError = true;
                    break;
                }

                const QJsonObject ruleObject = ruleValue.toObject();

                const QJsonValue formatValue = ruleObject.value(QStringLiteral("format"));
                if (!formatValue.isObject()) {
                    qWarning() << "Format is mandatory in rules and should be an object";
                    hasError = true;
                    break;
                }

                const QJsonObject formatObject = formatValue.toObject();
                auto format = QShaderFormat();

                const QJsonValue apiValue = formatObject.value(QStringLiteral("api"));
                if (!apiValue.isString()) {
                    qWarning() << "Format API must be a string";
                    hasError = true;
                    break;
                }

                const QString api = apiValue.toString();
                format.setApi(api == QStringLiteral("OpenGLES") ? QShaderFormat::OpenGLES
                            : api == QStringLiteral("OpenGLNoProfile") ? QShaderFormat::OpenGLNoProfile
                            : api == QStringLiteral("OpenGLCoreProfile") ? QShaderFormat::OpenGLCoreProfile
                            : api == QStringLiteral("OpenGLCompatibilityProfile") ? QShaderFormat::OpenGLCompatibilityProfile
                            : api == QStringLiteral("VulkanFlavoredGLSL") ? QShaderFormat::VulkanFlavoredGLSL
                            : api == QStringLiteral("RHI") ? QShaderFormat::RHI
                            : QShaderFormat::NoApi);
                if (format.api() == QShaderFormat::NoApi) {
                    qWarning() << "Format API must be one of: OpenGLES, OpenGLNoProfile, OpenGLCoreProfile OpenGLCompatibilityProfile, VulkanFlavoredGLSL or RHI";
                    hasError = true;
                    break;
                }

                const QJsonValue majorValue = formatObject.value(QStringLiteral("major"));
                const QJsonValue minorValue = formatObject.value(QStringLiteral("minor"));
                if (!majorValue.isDouble() || !minorValue.isDouble()) {
                    qWarning() << "Format major and minor version must be values";
                    hasError = true;
                    break;
                }
                format.setVersion(QVersionNumber(majorValue.toInt(), minorValue.toInt()));

                const QJsonValue extensionsValue = formatObject.value(QStringLiteral("extensions"));
                const QJsonArray extensionsArray = extensionsValue.toArray();
                auto extensions = QStringList();
                std::transform(extensionsArray.constBegin(), extensionsArray.constEnd(),
                               std::back_inserter(extensions),
                               [] (const QJsonValue &extensionValue) { return extensionValue.toString(); });
                format.setExtensions(extensions);

                const QString vendor = formatObject.value(QStringLiteral("vendor")).toString();
                format.setVendor(vendor);

                const QJsonValue substitutionValue = ruleObject.value(QStringLiteral("substitution"));
                if (!substitutionValue.isString()) {
                    qWarning() << "Substitution needs to be a string";
                    hasError = true;
                    break;
                }

                // We default out to a Fragment ShaderType if nothing is specified
                // as that was the initial behavior we introduced
                const QString shaderType = formatObject.value(QStringLiteral("shaderType")).toString();
                format.setShaderType(shaderType == QStringLiteral("Fragment") ? QShaderFormat::Fragment
                                   : shaderType == QStringLiteral("Vertex") ? QShaderFormat::Vertex
                                   : shaderType == QStringLiteral("TessellationControl") ? QShaderFormat::TessellationControl
                                   : shaderType == QStringLiteral("TessellationEvaluation") ? QShaderFormat::TessellationEvaluation
                                   : shaderType == QStringLiteral("Geometry") ? QShaderFormat::Geometry
                                   : shaderType == QStringLiteral("Compute") ? QShaderFormat::Compute
                                   : QShaderFormat::Fragment);

                const QByteArray substitution = substitutionValue.toString().toUtf8();

                // WA for QTBUG-99019
                const auto wIt = (format.shaderType() == QShaderFormat::Fragment)
                                 ? ruleObject.constFind(QStringLiteral("headerSnippetsFrag"))
                                 : ruleObject.constEnd();
                const QJsonValue snippetsValue = (wIt != ruleObject.constEnd())
                                                 ? *wIt
                                                 : ruleObject.value(QStringLiteral("headerSnippets"));
                const QJsonArray snippetsArray = snippetsValue.toArray();
                auto snippets = QByteArrayList();
                std::transform(snippetsArray.constBegin(), snippetsArray.constEnd(),
                               std::back_inserter(snippets),
                               [] (const QJsonValue &snippetValue) { return snippetValue.toString().toUtf8(); });

                node.addRule(format, QShaderNode::Rule(substitution, snippets));
            }
        }

        m_nodes.insert(property, node);
    }

    if (hasError) {
        m_status = Error;
        m_nodes.clear();
    } else {
        m_status = Ready;
    }
}
}
QT_END_NAMESPACE
