// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmshapetransform_p.h"

#include <QJsonObject>
#include <QtMath>

#include "bmconstants_p.h"
#include "bmbasictransform_p.h"

BMShapeTransform::BMShapeTransform(const BMShapeTransform &other)
    : BMBasicTransform(other)
{
    m_skew = other.m_skew;
    m_skewAxis = other.m_skewAxis;
    m_shearX = other.m_shearX;
    m_shearY = other.m_shearY;
    m_shearAngle = other.m_shearAngle;
}

BMShapeTransform::BMShapeTransform(const QJsonObject &definition, const QVersionNumber &version,
                                   BMBase *parent)
{
    setParent(parent);
    construct(definition, version);
}

BMBase *BMShapeTransform::clone() const
{
    return new BMShapeTransform(*this);
}

void BMShapeTransform::construct(const QJsonObject &definition, const QVersionNumber &version)
{
    BMBasicTransform::construct(definition, version);

    qCDebug(lcLottieQtBodymovinParser) << "BMShapeTransform::construct():" << BMShape::name();

    QJsonObject skew = definition.value(QLatin1String("sk")).toObject();
    skew = resolveExpression(skew);
    m_skew.construct(skew, version);

    QJsonObject skewAxis = definition.value(QLatin1String("sa")).toObject();
    skewAxis = resolveExpression(skewAxis);
    m_skewAxis.construct(skewAxis, version);
}

void BMShapeTransform::updateProperties(int frame)
{
    BMBasicTransform::updateProperties(frame);

    m_skew.update(frame);
    m_skewAxis.update(frame);

    double rads = qDegreesToRadians(m_skewAxis.value());
    m_shearX = qCos(rads);
    m_shearY = qSin(rads);
    double tan = qDegreesToRadians(-m_skew.value());
    m_shearAngle = qTan(tan);
}

void BMShapeTransform::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

qreal BMShapeTransform::skew() const
{
    return m_skew.value();
}

qreal BMShapeTransform::skewAxis() const
{
    return m_skewAxis.value();
}

qreal BMShapeTransform::shearX() const
{
    return m_shearX;
}

qreal BMShapeTransform::shearY() const
{
    return m_shearY;
}

qreal BMShapeTransform::shearAngle() const
{
    return m_shearAngle;
}
