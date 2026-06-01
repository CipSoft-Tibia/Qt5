// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottiefilleffect_p.h"

#include <QJsonObject>
#include <QJsonValue>

QT_BEGIN_NAMESPACE

QLottieFillEffect::QLottieFillEffect(const QLottieFillEffect &other)
    : QLottieBase(other)
{
    m_color = other.m_color;
    m_opacity = other.m_opacity;
}

QLottieBase *QLottieFillEffect::clone() const
{
    return new QLottieFillEffect(*this);
}

void QLottieFillEffect::construct(const QJsonObject &definition)
{
    m_type = LOTTIE_EFFECT_FILL;

    if (!definition.value(QLatin1String("hd")).toBool(true))
        return;

    QJsonArray properties = definition.value(QLatin1String("ef")).toArray();

    // TODO: Check are property positions really fixed in the effect?

    m_color.construct(properties.at(2).toObject().value(QLatin1String("v")).toObject());
    m_opacity.construct(properties.at(6).toObject().value(QLatin1String("v")).toObject());

    if (!qFuzzyCompare(properties.at(0).toObject().value(QLatin1String("v")).toObject().value(QLatin1String("k")).toDouble(), 0.0))
        qCInfo(lcLottieQtLottieParser)<< "QLottieFillEffect: Property 'Fill mask' not supported";

    if (!qFuzzyCompare(properties.at(1).toObject().value(QLatin1String("v")).toObject().value(QLatin1String("k")).toDouble(), 0.0))
        qCInfo(lcLottieQtLottieParser) << "QLottieFillEffect: Property 'All masks' not supported";

    if (!qFuzzyCompare(properties.at(3).toObject().value(QLatin1String("v")).toObject().value(QLatin1String("k")).toDouble(), 0.0))
        qCInfo(lcLottieQtLottieParser) << "QLottieFillEffect: Property 'Invert' not supported";

    if (!qFuzzyCompare(properties.at(4).toObject().value(QLatin1String("v")).toObject().value(QLatin1String("k")).toDouble(), 0.0))
        qCInfo(lcLottieQtLottieParser) << "QLottieFillEffect: Property 'Horizontal feather' not supported";

    if (!qFuzzyCompare(properties.at(5).toObject().value(QLatin1String("v")).toObject().value(QLatin1String("k")).toDouble(), 0.0))
        qCInfo(lcLottieQtLottieParser)
                << "QLottieFillEffect: Property 'Vertical feather' not supported";
}

void QLottieFillEffect::updateProperties(int frame)
{
    m_color.update(frame);
    m_opacity.update(frame);
}

void QLottieFillEffect::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QColor QLottieFillEffect::color() const
{
    QVector4D cVec = m_color.value();
    QColor color;
    qreal r = static_cast<qreal>(cVec.x());
    qreal g = static_cast<qreal>(cVec.y());
    qreal b = static_cast<qreal>(cVec.z());
    qreal a = static_cast<qreal>(cVec.w());
    color.setRgbF(r, g, b, a);
    return color;
}

qreal QLottieFillEffect::opacity() const
{
    return m_opacity.value();
}

QT_END_NAMESPACE
