// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieprecomplayer_p.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>

#include "qlottiebasictransform_p.h"
#include "qlottierenderer_p.h"

QT_BEGIN_NAMESPACE

QLottiePrecompLayer::QLottiePrecompLayer(const QLottiePrecompLayer &other)
    : QLottieLayer(other)
{
}

QLottiePrecompLayer::QLottiePrecompLayer(const QJsonObject &definition, const QMap<QString, QJsonObject> &assets)
{
    m_type = LOTTIE_LAYER_PRECOMP_IX;

    QLottieLayer::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottiePrecompLayer::QLottiePrecompLayer()" << m_name;

    m_startTime = definition.value(QLatin1String("st")).toDouble(); // only relevant for precomps
    QString refId = definition.value(QLatin1String("refId")).toString();
    QJsonObject asset = assets.value(refId);
    QJsonArray jsonLayers = asset.value(QLatin1String("layers")).toArray();
    int numLayers = QLottieLayer::constructLayers(jsonLayers, this, assets);

    m_size = QSize(definition.value(QLatin1String("w")).toInt(-1), definition.value(QLatin1String("h")).toInt(-1));

    qCDebug(lcLottieQtLottieParser) << "QLottiePrecompLayer created" << numLayers << "layers";
}

QLottieBase *QLottiePrecompLayer::clone() const
{
    return new QLottiePrecompLayer(*this);
}

void QLottiePrecompLayer::render(QLottieRenderer &renderer) const
{
    if (!m_isActive)
        return;

    renderer.saveState();

    QLottieLayer::render(renderer);

    renderer.finish(*this);
    renderer.restoreState();
}

QT_END_NAMESPACE
