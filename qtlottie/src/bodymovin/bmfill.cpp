// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmfill_p.h"

QT_BEGIN_NAMESPACE

BMFill::BMFill(const BMFill &other)
    : BMShape(other)
{
    m_color = other.m_color;
    m_opacity = other.m_opacity;
}

BMFill::BMFill(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent)
{
    setParent(parent);
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMFill::construct():" << m_name;

    QJsonObject color = definition.value(QLatin1String("c")).toObject();
    m_color.construct(color, version);

    QJsonObject opacity = definition.value(QLatin1String("o")).toObject();
    opacity = resolveExpression(opacity);
    m_opacity.construct(opacity, version);
}

BMBase *BMFill::clone() const
{
    return new BMFill(*this);
}

void BMFill::updateProperties(int frame)
{
    m_color.update(frame);
    m_opacity.update(frame);
}

void BMFill::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

QColor BMFill::color() const
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

qreal BMFill::opacity() const
{
    return m_opacity.value();
}

QT_END_NAMESPACE
