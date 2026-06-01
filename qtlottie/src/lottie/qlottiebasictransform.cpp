// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottiebasictransform_p.h"

#include <QJsonObject>

#include "qlottieconstants_p.h"

QT_BEGIN_NAMESPACE

QLottieBasicTransform::QLottieBasicTransform(const QLottieBasicTransform &other)
    : QLottieShape(other)
{
    m_direction = other.m_direction;
    m_anchorPoint = other.m_anchorPoint;
    m_splitPosition = other.m_splitPosition;
    m_position = other.m_position;
    m_xPos = other.m_xPos;
    m_yPos = other.m_yPos;
    m_scale = other.m_scale;
    m_rotation = other.m_rotation;
    m_opacity = other.m_opacity;
}

QLottieBasicTransform::QLottieBasicTransform(const QJsonObject &definition,
                                   QLottieBase *parent)
{
    setParent(parent);
    construct(definition);
}

QLottieBase *QLottieBasicTransform::clone() const
{
    return new QLottieBasicTransform(*this);
}

void QLottieBasicTransform::construct(const QJsonObject &definition)
{
    QLottieBase::parse(definition);

    qCDebug(lcLottieQtLottieParser)
            << "QLottieBasicTransform::construct():" << m_name;

    QJsonObject anchors = definition.value(QLatin1String("a")).toObject();
    anchors = resolveExpression(anchors);
    m_anchorPoint.construct(anchors);

    if (definition.value(QLatin1String("p")).toObject().contains(QLatin1String("s"))) {
        QJsonObject posX = definition.value(QLatin1String("p")).toObject().value(QLatin1String("x")).toObject();
        posX = resolveExpression(posX);
        m_xPos.construct(posX);

        QJsonObject posY = definition.value(QLatin1String("p")).toObject().value(QLatin1String("y")).toObject();
        posY = resolveExpression(posY);
        m_yPos.construct(posY);

        m_splitPosition = true;
    } else {
        QJsonObject position = definition.value(QLatin1String("p")).toObject();
        position = resolveExpression(position);
        m_position.construct(position);
    }

    if (definition.contains(QLatin1String("s"))) {
        QJsonObject scale = definition.value(QLatin1String("s")).toObject();
        scale = resolveExpression(scale);
        m_scale.construct(scale);
    } else {
        m_scale.setValue(QPointF(100, 100));
    }

    QJsonObject rotation = definition.value(QLatin1String("r")).toObject();
    rotation = resolveExpression(rotation);
    m_rotation.construct(rotation);

    // If this is the base class for QLottieRepeaterTransform,
    // opacity is not present
    if (definition.contains(QLatin1String("o"))) {
        QJsonObject opacity = definition.value(QLatin1String("o")).toObject();
        opacity = resolveExpression(opacity);
        m_opacity.construct(opacity);
    } else {
        m_opacity.setValue(100);
    }
}

void QLottieBasicTransform::updateProperties(int frame)
{
    if (m_splitPosition) {
        m_xPos.update(frame);
        m_yPos.update(frame);
    } else
        m_position.update(frame);
    m_anchorPoint.update(frame);
    m_scale.update(frame);
    m_rotation.update(frame);
    m_opacity.update(frame);
}

void QLottieBasicTransform::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QPointF QLottieBasicTransform::anchorPoint() const
{
    return m_anchorPoint.value();
}

QPointF QLottieBasicTransform::position() const
{
    if (m_splitPosition)
        return QPointF(m_xPos.value(), m_yPos.value());
    else
        return m_position.value();
}

QPointF QLottieBasicTransform::scale() const
{
    // Scale the value to 0..1 to be suitable for Qt
    return m_scale.value() / 100.0;
}

qreal QLottieBasicTransform::rotation() const
{
    return m_rotation.value();
}

qreal QLottieBasicTransform::opacity() const
{
    // Scale the value to 0..1 to be suitable for Qt
    return m_opacity.value() / 100.0;
}

QT_END_NAMESPACE
