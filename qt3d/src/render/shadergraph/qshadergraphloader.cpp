// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qshadergraphloader_p.h"

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
void qt_register_ShaderLanguage_enums();

QShaderGraphLoader::QShaderGraphLoader() noexcept
    : m_status(Null),
      m_device(nullptr)
{
    qt_register_ShaderLanguage_enums();
}

QShaderGraphLoader::Status QShaderGraphLoader::status() const noexcept
{
    return m_status;
}

QShaderGraph QShaderGraphLoader::graph() const noexcept
{
    return m_graph;
}

QIODevice *QShaderGraphLoader::device() const noexcept
{
    return m_device;
}

void QShaderGraphLoader::setDevice(QIODevice *device) noexcept
{
    m_device = device;
    m_graph = QShaderGraph();
    m_status = !m_device ? Null
             : (m_device->openMode() & QIODevice::ReadOnly) ? Waiting
             : Error;
}

QHash<QString, QShaderNode> QShaderGraphLoader::prototypes() const noexcept
{
    return m_prototypes;
}

void QShaderGraphLoader::setPrototypes(const QHash<QString, QShaderNode> &prototypes) noexcept
{
    m_prototypes = prototypes;
}

void QShaderGraphLoader::load()
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

    const QJsonValue nodesValue = root.value(QStringLiteral("nodes"));
    if (!nodesValue.isArray()) {
        qWarning() << "Invalid nodes property, should be an array";
        m_status = Error;
        return;
    }

    const QJsonValue edgesValue = root.value(QStringLiteral("edges"));
    if (!edgesValue.isArray()) {
        qWarning() << "Invalid edges property, should be an array";
        m_status = Error;
        return;
    }

    bool hasError = false;

    const QJsonValue prototypesValue = root.value(QStringLiteral("prototypes"));
    if (!prototypesValue.isUndefined()) {
        if (prototypesValue.isObject()) {
            QShaderNodesLoader loader;
            loader.load(prototypesValue.toObject());
            m_prototypes.insert(loader.nodes());
        } else {
            qWarning() << "Invalid prototypes property, should be an object";
            m_status = Error;
            return;
        }
    }

    const QJsonArray nodes = nodesValue.toArray();
    for (const QJsonValue nodeValue : nodes) {
        if (!nodeValue.isObject()) {
            qWarning() << "Invalid node found";
            hasError = true;
            continue;
        }

        const QJsonObject nodeObject = nodeValue.toObject();

        const QString uuidString = nodeObject.value(QStringLiteral("uuid")).toString();
        const QUuid uuid = QUuid(uuidString);
        if (uuid.isNull()) {
            qWarning() << "Invalid UUID found in node:" << uuidString;
            hasError = true;
            continue;
        }

        const QString type = nodeObject.value(QStringLiteral("type")).toString();
        if (!m_prototypes.contains(type)) {
            qWarning() << "Unsupported node type found:" << type;
            hasError = true;
            continue;
        }

        const QJsonArray layersArray = nodeObject.value(QStringLiteral("layers")).toArray();
        auto layers = QStringList();
        for (const QJsonValue layerValue : layersArray)
            layers.append(layerValue.toString());

        QShaderNode node = m_prototypes.value(type);
        node.setUuid(uuid);
        node.setLayers(layers);

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

        m_graph.addNode(node);
    }

    const QJsonArray edges = edgesValue.toArray();
    for (const QJsonValue edgeValue : edges) {
        if (!edgeValue.isObject()) {
            qWarning() << "Invalid edge found";
            hasError = true;
            continue;
        }

        const QJsonObject edgeObject = edgeValue.toObject();

        const QString sourceUuidString = edgeObject.value(QStringLiteral("sourceUuid")).toString();
        const QUuid sourceUuid = QUuid(sourceUuidString);
        if (sourceUuid.isNull()) {
            qWarning() << "Invalid source UUID found in edge:" << sourceUuidString;
            hasError = true;
            continue;
        }

        const QString sourcePort = edgeObject.value(QStringLiteral("sourcePort")).toString();

        const QString targetUuidString = edgeObject.value(QStringLiteral("targetUuid")).toString();
        const QUuid targetUuid = QUuid(targetUuidString);
        if (targetUuid.isNull()) {
            qWarning() << "Invalid target UUID found in edge:" << targetUuidString;
            hasError = true;
            continue;
        }

        const QString targetPort = edgeObject.value(QStringLiteral("targetPort")).toString();

        const QJsonArray layersArray = edgeObject.value(QStringLiteral("layers")).toArray();
        auto layers = QStringList();
        for (const QJsonValue layerValue : layersArray)
            layers.append(layerValue.toString());

        auto edge = QShaderGraph::Edge();
        edge.sourceNodeUuid = sourceUuid;
        edge.sourcePortName = sourcePort;
        edge.targetNodeUuid = targetUuid;
        edge.targetPortName = targetPort;
        edge.layers = layers;
        m_graph.addEdge(edge);
    }

    if (hasError) {
        m_status = Error;
        m_graph = QShaderGraph();
    } else {
        m_status = Ready;
    }
}
}
QT_END_NAMESPACE
