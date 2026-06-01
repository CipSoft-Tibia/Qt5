// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottiefill_p.h"

QT_BEGIN_NAMESPACE

QLottieFill::QLottieFill(const QLottieFill &other)
    : QLottieShape(other)
{
    m_color = other.m_color;
    m_opacity = other.m_opacity;
}

QLottieFill::QLottieFill(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);
    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieFill::construct():" << m_name;

    QJsonObject color = definition.value(QLatin1String("c")).toObject();
    m_color.construct(color);

    QJsonObject opacity = definition.value(QLatin1String("o")).toObject();
    opacity = resolveExpression(opacity);
    m_opacity.construct(opacity);
}

QLottieBase *QLottieFill::clone() const
{
    return new QLottieFill(*this);
}

void QLottieFill::updateProperties(int frame)
{
    m_color.update(frame);
    m_opacity.update(frame);
}

void QLottieFill::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QColor QLottieFill::color() const
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

qreal QLottieFill::opacity() const
{
    return m_opacity.value();
}

QT_END_NAMESPACE
