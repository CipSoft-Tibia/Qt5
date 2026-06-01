// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieroot_p.h"
#include "qlottielayer_p.h"

#include <QJsonDocument>
#include <QJsonArray>

QT_BEGIN_NAMESPACE

QLottieRoot::QLottieRoot(const QLottieRoot &other)
    : QLottieBase(other)
{
}

QLottieBase *QLottieRoot::clone() const
{
    return new QLottieRoot(*this);
}

int QLottieRoot::parseSource(const QByteArray &jsonSource, const QUrl &fileSource)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonSource);
    QJsonObject rootObj = doc.object();
    m_definition = rootObj;

    if (rootObj.empty())
        return -1;

    QMap<QString, QJsonObject> assets;
    QJsonArray jsonLayers = rootObj.value(QLatin1String("layers")).toArray();
    QJsonArray jsonAssets = rootObj.value(QLatin1String("assets")).toArray();
    m_frameRate = rootObj.value(QLatin1String("fr")).toVariant().toInt();
    m_startFrame = rootObj.value(QLatin1String("ip")).toVariant().toInt();
    m_endFrame = rootObj.value(QLatin1String("op")).toVariant().toInt();
    QJsonArray::const_iterator jsonAssetsIt = jsonAssets.constBegin();
    while (jsonAssetsIt != jsonAssets.constEnd()) {
        QJsonObject jsonAsset = (*jsonAssetsIt).toObject();

        jsonAsset.insert(QLatin1String("fileSource"), QJsonValue::fromVariant(fileSource));
        QString id = jsonAsset.value(QLatin1String("id")).toString();
        assets.insert(id, jsonAsset);
        jsonAssetsIt++;
    }

    QLottieLayer::constructLayers(jsonLayers, this, assets);

    return 0;
}

void QLottieRoot::setStructureDumping(bool enabled)
{
    m_structureDumping = enabled ? 1 : 0;
}

QT_END_NAMESPACE
