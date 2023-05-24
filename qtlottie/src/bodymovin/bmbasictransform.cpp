// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmbasictransform_p.h"

#include <QJsonObject>

#include "bmconstants_p.h"

QT_BEGIN_NAMESPACE

BMBasicTransform::BMBasicTransform(const BMBasicTransform &other)
    : BMShape(other)
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

BMBasicTransform::BMBasicTransform(const QJsonObject &definition, const QVersionNumber &version,
                                   BMBase *parent)
{
    setParent(parent);
    construct(definition, version);
}

BMBase *BMBasicTransform::clone() const
{
    return new BMBasicTransform(*this);
}

void BMBasicTransform::construct(const QJsonObject &definition, const QVersionNumber &version)
{
    BMBase::parse(definition);

    qCDebug(lcLottieQtBodymovinParser)
            << "BMBasicTransform::construct():" << m_name;

    QJsonObject anchors = definition.value(QLatin1String("a")).toObject();
    anchors = resolveExpression(anchors);
    m_anchorPoint.construct(anchors, version);

    if (definition.value(QLatin1String("p")).toObject().contains(QLatin1String("s"))) {
        QJsonObject posX = definition.value(QLatin1String("p")).toObject().value(QLatin1String("x")).toObject();
        posX = resolveExpression(posX);
        m_xPos.construct(posX, version);

        QJsonObject posY = definition.value(QLatin1String("p")).toObject().value(QLatin1String("y")).toObject();
        posY = resolveExpression(posY);
        m_yPos.construct(posY, version);

        m_splitPosition = true;
    } else {
        QJsonObject position = definition.value(QLatin1String("p")).toObject();
        position = resolveExpression(position);
        m_position.construct(position, version);
    }

    QJsonObject scale = definition.value(QLatin1String("s")).toObject();
    scale = resolveExpression(scale);
    m_scale.construct(scale, version);

    QJsonObject rotation = definition.value(QLatin1String("r")).toObject();
    rotation = resolveExpression(rotation);
    m_rotation.construct(rotation, version);

    // If this is the base class for BMRepeaterTransform,
    // opacity is not present
    if (definition.contains(QLatin1String("o"))) {
        QJsonObject opacity = definition.value(QLatin1String("o")).toObject();
        opacity = resolveExpression(opacity);
        m_opacity.construct(opacity, version);
    }
}

void BMBasicTransform::updateProperties(int frame)
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

void BMBasicTransform::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

QPointF BMBasicTransform::anchorPoint() const
{
    return m_anchorPoint.value();
}

QPointF BMBasicTransform::position() const
{
    if (m_splitPosition)
        return QPointF(m_xPos.value(), m_yPos.value());
    else
        return m_position.value();
}

QPointF BMBasicTransform::scale() const
{
    // Scale the value to 0..1 to be suitable for Qt
    return m_scale.value() / 100.0;
}

qreal BMBasicTransform::rotation() const
{
    return m_rotation.value();
}

qreal BMBasicTransform::opacity() const
{
    // Scale the value to 0..1 to be suitable for Qt
    return m_opacity.value() / 100.0;
}

QT_END_NAMESPACE
