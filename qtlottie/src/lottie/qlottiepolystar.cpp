// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottiepolystar_p.h"

#include <QJsonObject>
#include <QRectF>

#include "qlottietrimpath_p.h"

QT_BEGIN_NAMESPACE

QLottiePolyStar::QLottiePolyStar(const QLottiePolyStar &other)
    : QLottieShape(other)
{
    m_position = other.m_position;
    m_pointCount = other.m_pointCount;
    m_outerRadius = other.m_outerRadius;
    m_innerRadius = other.m_innerRadius;
    m_startAngle = other.m_startAngle;
    m_polygonMode = other.m_polygonMode;
}

QLottiePolyStar::QLottiePolyStar(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);
    construct(definition);
}

QLottieBase *QLottiePolyStar::clone() const
{
    return new QLottiePolyStar(*this);
}

void QLottiePolyStar::construct(const QJsonObject &definition)
{
    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottiePolyStar::construct():" << m_name;

    QJsonObject position = definition.value(QLatin1String("p")).toObject();
    position = resolveExpression(position);
    m_position.construct(position);

    QJsonObject outerRadius = definition.value(QLatin1String("or")).toObject();
    outerRadius = resolveExpression(outerRadius);
    m_outerRadius.construct(outerRadius);

    QJsonObject innerRadius = definition.value(QLatin1String("ir")).toObject();
    innerRadius = resolveExpression(innerRadius);
    m_innerRadius.construct(innerRadius);

    QJsonObject startAngle = definition.value(QLatin1String("r")).toObject();
    startAngle = resolveExpression(startAngle);
    m_startAngle.construct(startAngle);

    QJsonObject pointCount = definition.value(QLatin1String("pt")).toObject();
    pointCount = resolveExpression(pointCount);
    m_pointCount.construct(pointCount);

    m_polygonMode = (definition.value(QLatin1String("sy")).toInt() == 2);

    m_direction = definition.value(QLatin1String("d")).toInt();
}

bool QLottiePolyStar::acceptsTrim() const
{
    return true;
}

void QLottiePolyStar::updateProperties(int frame)
{
    m_position.update(frame);
    m_outerRadius.update(frame);
    m_innerRadius.update(frame);
    m_startAngle.update(frame);
    m_pointCount.update(frame);

    m_path.clear();

    const int numPoints = m_pointCount.value();
    if (numPoints < 1)
        return;
    const qreal angleStep = -360.0 / numPoints;
    const qreal halfAngleStep = angleStep / 2;
    qreal angle = 90 - m_startAngle.value();

    const QPointF pos = m_position.value();
    QLineF outLine(pos, QPointF(pos.x(), pos.y() - m_outerRadius.value()));
    QLineF inLine(pos, QPointF(pos.x(), pos.y() - m_innerRadius.value()));

    outLine.setAngle(angle);
    const QPointF startPt = outLine.p2();
    m_path.moveTo(startPt);
    for (int i = 0; i < numPoints; i++) {
        if (!m_polygonMode) {
            inLine.setAngle(angle + halfAngleStep);
            m_path.lineTo(inLine.p2());
        }
        angle += angleStep;
        outLine.setAngle(angle);
        m_path.lineTo(outLine.p2());
    }
    // Fix potential accuracy errors, ensuring path is closed:
    m_path.setElementPositionAt(m_path.elementCount() - 1, startPt.x(), startPt.y());

    if (hasReversedDirection())
        m_path = m_path.toReversed();
}

void QLottiePolyStar::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QPointF QLottiePolyStar::position() const
{
    return m_position.value();
}

qreal QLottiePolyStar::outerRadius() const
{
    return m_outerRadius.value();
}

qreal QLottiePolyStar::innerRadius() const
{
    return m_innerRadius.value();
}

qreal QLottiePolyStar::startAngle() const
{
    return m_startAngle.value();
}

int QLottiePolyStar::pointCount() const
{
    return m_pointCount.value();
}

bool QLottiePolyStar::isPolygonModeEnabled() const
{
    return m_polygonMode;
}

QT_END_NAMESPACE
