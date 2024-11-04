// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "axisgrid_p.h"

QT_BEGIN_NAMESPACE

AxisGrid::AxisGrid(QQuickItem *parent) :
      QQuickShaderEffect(parent)
{
}

void AxisGrid::componentComplete()
{
    QQuickShaderEffect::componentComplete();
    setupShaders();
}

void AxisGrid::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_iResolution = QVector3D(newGeometry.width(), newGeometry.height(), 1.0);
    emit iResolutionChanged();

    QQuickShaderEffect::geometryChange(newGeometry, oldGeometry);
}

void AxisGrid::setupShaders()
{
    setFragmentShader(QUrl(QStringLiteral("qrc:/shaders/gridshader.frag.qsb")));
    setVertexShader(QUrl(QStringLiteral("qrc:/shaders/gridshader.vert.qsb")));
}

QVector3D AxisGrid::iResolution() const
{
    return m_iResolution;
}

qreal AxisGrid::smoothing() const
{
    return m_smoothing;
}

void AxisGrid::setSmoothing(qreal newSmoothing)
{
    if (qFuzzyCompare(m_smoothing, newSmoothing))
        return;
    m_smoothing = newSmoothing;
    emit smoothingChanged();
}

int AxisGrid::origo() const
{
    return m_origo;
}

void AxisGrid::setOrigo(int newOrigo)
{
    if (m_origo == newOrigo)
        return;
    m_origo = newOrigo;
    emit origoChanged();
}

QVector4D AxisGrid::barsVisibility() const
{
    return m_barsVisibility;
}

void AxisGrid::setBarsVisibility(const QVector4D &newBarsVisibility)
{
    if (m_barsVisibility == newBarsVisibility)
        return;
    m_barsVisibility = newBarsVisibility;
    emit barsVisibilityChanged();
}

qreal AxisGrid::gridWidth() const
{
    return m_gridWidth;
}

void AxisGrid::setGridWidth(qreal newGridWidth)
{
    if (qFuzzyCompare(m_gridWidth, newGridWidth))
        return;
    m_gridWidth = newGridWidth;
    emit gridWidthChanged();
}

qreal AxisGrid::gridHeight() const
{
    return m_gridHeight;
}

void AxisGrid::setGridHeight(qreal newGridHeight)
{
    if (qFuzzyCompare(m_gridHeight, newGridHeight))
        return;
    m_gridHeight = newGridHeight;
    emit gridHeightChanged();
}

QPointF AxisGrid::gridMovement() const
{
    return m_gridMovement;
}

void AxisGrid::setGridMovement(QPointF newGridMovement)
{
    if (m_gridMovement == newGridMovement)
        return;
    m_gridMovement = newGridMovement;
    emit gridMovementChanged();
}

QColor AxisGrid::minorColor() const
{
    return m_minorColor;
}

void AxisGrid::setMinorColor(const QColor &newMinorColor)
{
    if (m_minorColor == newMinorColor)
        return;
    m_minorColor = newMinorColor;
    emit minorColorChanged();
}

QColor AxisGrid::majorColor() const
{
    return m_majorColor;
}

void AxisGrid::setMajorColor(const QColor &newMajorColor)
{
    if (m_majorColor == newMajorColor)
        return;
    m_majorColor = newMajorColor;
    emit majorColorChanged();
}

qreal AxisGrid::minorBarWidth() const
{
    return m_minorBarWidth;
}

void AxisGrid::setMinorBarWidth(qreal newMinorBarWidth)
{
    if (qFuzzyCompare(m_minorBarWidth, newMinorBarWidth))
        return;
    m_minorBarWidth = newMinorBarWidth;
    emit minorBarWidthChanged();
}

qreal AxisGrid::majorBarWidth() const
{
    return m_majorBarWidth;
}

void AxisGrid::setMajorBarWidth(qreal newMajorBarWidth)
{
    if (qFuzzyCompare(m_majorBarWidth, newMajorBarWidth))
        return;
    m_majorBarWidth = newMajorBarWidth;
    emit majorBarWidthChanged();
}

qreal AxisGrid::verticalMinorTickScale() const
{
    return m_verticalMinorTickScale;
}

void AxisGrid::setVerticalMinorTickScale(qreal newVerticalMinorTickScale)
{
    if (qFuzzyCompare(m_verticalMinorTickScale, newVerticalMinorTickScale))
        return;
    m_verticalMinorTickScale = newVerticalMinorTickScale;
    emit verticalMinorTickScaleChanged();
}

qreal AxisGrid::horizontalMinorTickScale() const
{
    return m_horizontalMinorTickScale;
}

void AxisGrid::setHorizontalMinorTickScale(qreal newHorizontalMinorTickScale)
{
    if (qFuzzyCompare(m_horizontalMinorTickScale, newHorizontalMinorTickScale))
        return;
    m_horizontalMinorTickScale = newHorizontalMinorTickScale;
    emit horizontalMinorTickScaleChanged();
}

QT_END_NAMESPACE
