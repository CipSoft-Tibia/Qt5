// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmrect_p.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

#include <QDebug>

#include "bmtrimpath_p.h"

QT_BEGIN_NAMESPACE

BMRect::BMRect(const BMRect &other)
    : BMShape(other)
{
    m_position = other.m_position;
    m_size = other.m_size;
    m_roundness = other.m_roundness;
}

BMRect::BMRect(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent)
{
    setParent(parent);
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMRect::BMRect():" << m_name;

    QJsonObject position = definition.value(QLatin1String("p")).toObject();
    position = resolveExpression(position);
    m_position.construct(position, version);

    QJsonObject size = definition.value(QLatin1String("s")).toObject();
    size = resolveExpression(size);
    m_size.construct(size, version);

    QJsonObject roundness = definition.value(QLatin1String("r")).toObject();
    roundness = resolveExpression(roundness);
    m_roundness.construct(roundness, version);

    m_direction = definition.value(QLatin1String("d")).toInt();
}


BMBase *BMRect::clone() const
{
    return new BMRect(*this);
}

bool BMRect::setProperty(BMLiteral::PropertyType propertyType, QVariant value)
{
    switch (propertyType) {
    case BMLiteral::RectPosition:
        qCDebug(lcLottieQtBodymovinParser) << "Set position" << value.toPointF();
        m_position.setValue(value.toPointF());
        break;
    default:
        return false;
    }
    return true;
}

void BMRect::updateProperties(int frame)
{
    m_size.update(frame);
    m_position.update(frame);
    m_roundness.update(frame);

    // AE uses center of a shape as it's position,
    // in Qt a translation is needed
    QPointF pos = QPointF(m_position.value().x() - m_size.value().width() / 2,
                             m_position.value().y() - m_size.value().height() / 2);

    m_path = QPainterPath();
    m_path.addRoundedRect(QRectF(pos, m_size.value()),
                               m_roundness.value(), m_roundness.value());

    if (m_direction)
        m_path = m_path.toReversed();
}

void BMRect::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

bool BMRect::acceptsTrim() const
{
    return true;
}

QPointF BMRect::position() const
{
    return m_position.value();
}

QSizeF BMRect::size() const
{
    return m_size.value();
}

qreal BMRect::roundness() const
{
    return m_roundness.value();
}

QT_END_NAMESPACE
