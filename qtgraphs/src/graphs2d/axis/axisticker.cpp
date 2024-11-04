// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "axisticker_p.h"

QT_BEGIN_NAMESPACE

AxisTicker::AxisTicker(QQuickItem *parent) :
      QQuickShaderEffect(parent)
{
}

void AxisTicker::componentComplete()
{
    QQuickShaderEffect::componentComplete();
    setupShaders();
}

void AxisTicker::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_iResolution = QVector3D(newGeometry.width(), newGeometry.height(), 1.0);
    emit iResolutionChanged();

    QQuickShaderEffect::geometryChange(newGeometry, oldGeometry);
}

void AxisTicker::setupShaders()
{
    if (m_isHorizontal) {
        setFragmentShader(QUrl(QStringLiteral("qrc:/shaders/tickershaderhorizontal.frag.qsb")));
        setVertexShader(QUrl(QStringLiteral("qrc:/shaders/tickershaderhorizontal.vert.qsb")));
    } else {
        setFragmentShader(QUrl(QStringLiteral("qrc:/shaders/tickershader.frag.qsb")));
        setVertexShader(QUrl(QStringLiteral("qrc:/shaders/tickershader.vert.qsb")));
    }
}

QVector3D AxisTicker::iResolution() const
{
    return m_iResolution;
}

qreal AxisTicker::smoothing() const
{
    return m_smoothing;
}

void AxisTicker::setSmoothing(qreal newSmoothing)
{
    if (qFuzzyCompare(m_smoothing, newSmoothing))
        return;
    m_smoothing = newSmoothing;
    emit smoothingChanged();
}

int AxisTicker::origo() const
{
    return m_origo;
}

void AxisTicker::setOrigo(int newOrigo)
{
    if (m_origo == newOrigo)
        return;
    m_origo = newOrigo;
    emit origoChanged();
}

bool AxisTicker::minorBarsVisible() const
{
    return m_minorBarsVisible;
}

void AxisTicker::setMinorBarsVisible(bool newMinorBarsVisible)
{
    if (m_minorBarsVisible == newMinorBarsVisible)
        return;
    m_minorBarsVisible = newMinorBarsVisible;
    emit minorBarsVisibleChanged();
}

qreal AxisTicker::spacing() const
{
    return m_spacing;
}

void AxisTicker::setSpacing(qreal newSpacing)
{
    if (qFuzzyCompare(m_spacing, newSpacing))
        return;
    m_spacing = newSpacing;
    emit spacingChanged();
}

qreal AxisTicker::barsMovement() const
{
    return m_barsMovement;
}

void AxisTicker::setBarsMovement(qreal newBarsMovement)
{
    if (qFuzzyCompare(m_barsMovement, newBarsMovement))
        return;
    m_barsMovement = newBarsMovement;
    emit barsMovementChanged();
}

QColor AxisTicker::minorColor() const
{
    return m_minorColor;
}

void AxisTicker::setMinorColor(const QColor &newMinorColor)
{
    if (m_minorColor == newMinorColor)
        return;
    m_minorColor = newMinorColor;
    emit minorColorChanged();
}

QColor AxisTicker::majorColor() const
{
    return m_majorColor;
}

void AxisTicker::setMajorColor(const QColor &newMajorColor)
{
    if (m_majorColor == newMajorColor)
        return;
    m_majorColor = newMajorColor;
    emit majorColorChanged();
}

qreal AxisTicker::minorBarWidth() const
{
    return m_minorBarWidth;
}

void AxisTicker::setMinorBarWidth(qreal newMinorBarWidth)
{
    if (qFuzzyCompare(m_minorBarWidth, newMinorBarWidth))
        return;
    m_minorBarWidth = newMinorBarWidth;
    emit minorBarWidthChanged();
}

qreal AxisTicker::majorBarWidth() const
{
    return m_majorBarWidth;
}

void AxisTicker::setMajorBarWidth(qreal newMajorBarWidth)
{
    if (qFuzzyCompare(m_majorBarWidth, newMajorBarWidth))
        return;
    m_majorBarWidth = newMajorBarWidth;
    emit majorBarWidthChanged();
}

qreal AxisTicker::minorTickScale() const
{
    return m_minorTickScale;
}

void AxisTicker::setMinorTickScale(qreal newMinorTickScale)
{
    if (qFuzzyCompare(m_minorTickScale, newMinorTickScale))
        return;
    m_minorTickScale = newMinorTickScale;
    emit minorTickScaleChanged();
}

qreal AxisTicker::minorBarsLength() const
{
    return m_minorBarsLength;
}

void AxisTicker::setMinorBarsLength(qreal newMinorBarsLength)
{
    if (qFuzzyCompare(m_minorBarsLength, newMinorBarsLength))
        return;
    m_minorBarsLength = newMinorBarsLength;
    emit minorBarsLengthChanged();
}

bool AxisTicker::isHorizontal() const
{
    return m_isHorizontal;
}

void AxisTicker::setIsHorizontal(bool newIsHorizontal)
{
    if (m_isHorizontal == newIsHorizontal)
        return;
    m_isHorizontal = newIsHorizontal;
    setupShaders();
    emit isHorizontalChanged();
}

QT_END_NAMESPACE
