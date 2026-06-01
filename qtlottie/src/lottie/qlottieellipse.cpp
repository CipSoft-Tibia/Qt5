// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieellipse_p.h"

#include <QJsonObject>
#include <QRectF>

#include "qlottietrimpath_p.h"

QT_BEGIN_NAMESPACE

QLottieEllipse::QLottieEllipse(const QLottieEllipse &other)
    : QLottieShape(other)
{
    m_position = other.m_position;
    m_size = other.m_size;
}

QLottieEllipse::QLottieEllipse(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);
    construct(definition);
}

QLottieBase *QLottieEllipse::clone() const
{
    return new QLottieEllipse(*this);
}

void QLottieEllipse::construct(const QJsonObject &definition)
{
    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieEllipse::construct():" << m_name;

    QJsonObject position = definition.value(QLatin1String("p")).toObject();
    position = resolveExpression(position);
    m_position.construct(position);

    QJsonObject size = definition.value(QLatin1String("s")).toObject();
    size = resolveExpression(size);
    m_size.construct(size);

    m_direction = definition.value(QLatin1String("d")).toInt();
}

bool QLottieEllipse::acceptsTrim() const
{
    return true;
}

void QLottieEllipse::updateProperties(int frame)
{
    m_position.update(frame);
    m_size.update(frame);

    // AE uses center of a shape as it's position,
    // in Qt a translation is needed
    QPointF pos = QPointF(m_position.value().x() - m_size.value().width() / 2,
                             m_position.value().y() - m_size.value().height() / 2);

    m_path.clear();
    m_path.arcMoveTo(QRectF(pos, m_size.value()), 90);
    m_path.arcTo(QRectF(pos, m_size.value()), 90, -360);

    if (hasReversedDirection())
        m_path = m_path.toReversed();
}

void QLottieEllipse::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QPointF QLottieEllipse::position() const
{
    return m_position.value();
}

QSizeF QLottieEllipse::size() const
{
    return m_size.value();
}

QT_END_NAMESPACE
