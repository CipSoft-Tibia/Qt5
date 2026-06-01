// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieflatlayers_p.h"

#include <QJsonObject>
#include <QJsonArray>


#include "qlottiebase_p.h"
#include "qlottieimage_p.h"
#include "qlottieshape_p.h"
#include "qlottietrimpath_p.h"
#include "qlottiebasictransform_p.h"
#include "qlottierenderer_p.h"

QT_BEGIN_NAMESPACE

QLottieNullLayer::QLottieNullLayer(const QLottieNullLayer &other)
    : QLottieLayer(other)
{
}

QLottieNullLayer::QLottieNullLayer(const QJsonObject &definition)
{
    m_type = LOTTIE_LAYER_NULL_IX;

    QLottieLayer::parse(definition);

    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieNullLayer::QLottieNullLayer()" << m_name;
}

QLottieBase *QLottieNullLayer::clone() const
{
    return new QLottieNullLayer(*this);
}

void QLottieNullLayer::render(QLottieRenderer &renderer) const
{
    if (!m_isActive)
        return;

    renderer.saveState();

    applyLayerTransform(renderer);

    renderer.render(*this);

    renderer.finish(*this);
    renderer.restoreState();
}

QLottieSolidLayer::QLottieSolidLayer(const QLottieSolidLayer &other)
    : QLottieLayer(other)
{
    m_color = other.m_color;
}

QLottieSolidLayer::QLottieSolidLayer(const QJsonObject &definition)
{
    m_type = LOTTIE_LAYER_SOLID_IX;

    QLottieLayer::parse(definition);

    if (m_hidden)
        return;

    m_size = QSize(definition.value(QLatin1String("sw")).toInt(-1), definition.value(QLatin1String("sh")).toInt(-1));
    m_color = QColor(definition.value(QLatin1String("sc")).toString());

    qCDebug(lcLottieQtLottieParser) << "QLottieSolidLayer::QLottieSolidLayer()" << m_name;
}

QLottieBase *QLottieSolidLayer::clone() const
{
    return new QLottieSolidLayer(*this);
}

void QLottieSolidLayer::render(QLottieRenderer &renderer) const
{
    if (!m_isActive)
        return;

    renderer.saveState();

    applyLayerTransform(renderer);

    renderer.render(*this);

    renderer.finish(*this);
    renderer.restoreState();
}

QColor QLottieSolidLayer::color() const
{
    return m_color;
}

QLottieImageLayer::QLottieImageLayer(const QLottieImageLayer &other)
    : QLottieLayer(other)
{
}

QLottieImageLayer::QLottieImageLayer(const QJsonObject &definition)
{
    m_type = LOTTIE_LAYER_IMAGE_IX;

    QLottieLayer::parse(definition);

    QLottieImage *image = new QLottieImage(definition, this);
    appendChild(image);

    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieImageLayer::QLottieImageLayer()" << m_name;
}

QLottieBase *QLottieImageLayer::clone() const
{
    return new QLottieImageLayer(*this);
}

void QLottieImageLayer::render(QLottieRenderer &renderer) const
{
    if (!m_isActive)
        return;

    renderer.saveState();

    QLottieLayer::render(renderer);

    renderer.finish(*this);
    renderer.restoreState();
}

QT_END_NAMESPACE
