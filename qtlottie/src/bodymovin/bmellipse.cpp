// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmellipse_p.h"

#include <QJsonObject>
#include <QRectF>

#include "bmtrimpath_p.h"

QT_BEGIN_NAMESPACE

BMEllipse::BMEllipse(const BMEllipse &other)
    : BMShape(other)
{
    m_position = other.m_position;
    m_size = other.m_size;
}

BMEllipse::BMEllipse(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent)
{
    setParent(parent);
    construct(definition, version);
}

BMBase *BMEllipse::clone() const
{
    return new BMEllipse(*this);
}

void BMEllipse::construct(const QJsonObject &definition, const QVersionNumber &version)
{
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMEllipse::construct():" << m_name;

    QJsonObject position = definition.value(QLatin1String("p")).toObject();
    position = resolveExpression(position);
    m_position.construct(position, version);

    QJsonObject size = definition.value(QLatin1String("s")).toObject();
    size = resolveExpression(size);
    m_size.construct(size, version);

    m_direction = definition.value(QLatin1String("d")).toInt();
}

bool BMEllipse::acceptsTrim() const
{
    return true;
}

void BMEllipse::updateProperties(int frame)
{
    m_position.update(frame);
    m_size.update(frame);

    // AE uses center of a shape as it's position,
    // in Qt a translation is needed
    QPointF pos = QPointF(m_position.value().x() - m_size.value().width() / 2,
                             m_position.value().y() - m_size.value().height() / 2);

    m_path = QPainterPath();
    m_path.arcMoveTo(QRectF(pos, m_size.value()), 90);
    m_path.arcTo(QRectF(pos, m_size.value()), 90, 360);

    if (m_direction)
        m_path = m_path.toReversed();
}

void BMEllipse::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

QPointF BMEllipse::position() const
{
    return m_position.value();
}

QSizeF BMEllipse::size() const
{
    return m_size.value();
}

QT_END_NAMESPACE
