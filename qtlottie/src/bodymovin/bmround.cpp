// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmround_p.h"

#include <QJsonObject>

#include "bmtrimpath_p.h"

QT_BEGIN_NAMESPACE

BMRound::BMRound(const BMRound &other)
    : BMShape(other)
{
    m_position = other.m_position;
    m_radius = other.m_radius;
}

BMRound::BMRound(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent)
{
    setParent(parent);
    construct(definition, version);
}

BMBase *BMRound::clone() const
{
    return new BMRound(*this);
}

void BMRound::construct(const QJsonObject &definition, const QVersionNumber &version)
{
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMRound::construct():" << m_name;

    QJsonObject position = definition.value(QLatin1String("p")).toObject();
    position = resolveExpression(position);
    m_position.construct(position, version);

    QJsonObject radius = definition.value(QLatin1String("r")).toObject();
    radius = resolveExpression(radius);
    m_radius.construct(radius, version);
}

void BMRound::updateProperties(int frame)
{
    m_position.update(frame);
    m_radius.update(frame);

    // AE uses center of a shape as it's position,
    // in Qt a translation is needed
    QPointF center = QPointF(m_position.value().x() - m_radius.value() / 2,
                             m_position.value().y() - m_radius.value() / 2);

    m_path = QPainterPath();
    m_path.arcMoveTo(QRectF(center,
                            QSizeF(m_radius.value(), m_radius.value())), 90);
    m_path.arcTo(QRectF(center,
                        QSizeF(m_radius.value(), m_radius.value())), 90, -360);

    if (m_direction)
        m_path = m_path.toReversed();
}

void BMRound::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

bool BMRound::acceptsTrim() const
{
    return true;
}

QPointF BMRound::position() const
{
    return m_position.value();
}

qreal BMRound::radius() const
{
    return m_radius.value();
}

QT_END_NAMESPACE
