// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qgraphtheme.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGraphTheme
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QGraphTheme class handles theming of the graph.

    Graph theme handles grid, axis and shadow properties. Grid and axes are
    divided to major and minor bars and ticks. Major bars and ticks are located
    on the values for ValueAxis and between categories for CategoryAxis. Minor
    bars and ticks are located between the major ones.
*/
/*!
    \qmltype GraphTheme
    \instantiates QGraphTheme
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D

    \brief Handles theming of the graph.

    Graph theme handles grid, axis and shadow properties.

    \sa SeriesTheme
*/

/*!
    \qmlproperty ColorTheme GraphTheme::colorTheme
    List of premade themes that determine values for properties of this class.
*/

/*!
    \qmlproperty real GraphTheme::gridMajorBarsWidth
    Sets the width of major bars of the grid.
*/

/*!
    \qmlproperty real GraphTheme::gridMinorBarsWidth
    Sets the width of minor bars of the grid.
*/

/*!
    \qmlproperty real GraphTheme::gridSmoothing
    Sets the amount of smoothing for the bars of the grid.
    Use this to antialias thin lines.
*/

/*!
    \qmlproperty color GraphTheme::gridMajorBarsColor
    Sets the color of major bars of the grid.
*/

/*!
    \qmlproperty color GraphTheme::gridMinorBarsColor
    Sets the color of minor bars of the grid.
*/

/*!
    \qmlproperty color GraphTheme::axisYMajorColor
    Sets the color of major ticks of the y axis.
*/

/*!
    \qmlproperty color GraphTheme::axisYMinorColor
    Sets the color of minor ticks of the y axis.
*/

/*!
    \qmlproperty real GraphTheme::axisYMajorBarWidth
    Sets the width of major ticks of the y axis.
*/

/*!
    \qmlproperty real GraphTheme::axisYMinorBarWidth
    Sets the width of minor ticks of the y axis.
*/

/*!
    \qmlproperty real GraphTheme::axisYSmoothing
    Sets the amount of smoothing for the ticks of the y axis.
    Use this to antialias thin lines.
*/

/*!
    \qmlproperty color GraphTheme::axisYLabelsColor
    Sets the color of the label for the y axis.
*/

/*!
    \qmlproperty font GraphTheme::axisYLabelsFont
    Sets the font of the label for the y axis.
*/

/*!
    \qmlproperty color GraphTheme::axisXMajorColor
    Sets the color of major ticks of the x axis.
*/

/*!
    \qmlproperty color GraphTheme::axisXMinorColor
    Sets the color of minor ticks of the x axis.
*/

/*!
    \qmlproperty real GraphTheme::axisXMajorBarWidth
    Sets the width of major ticks of the x axis.
*/

/*!
    \qmlproperty real GraphTheme::axisXMinorBarWidth
    Sets the width of minor ticks of the x axis.
*/

/*!
    \qmlproperty real GraphTheme::axisXSmoothing
    Sets the amount of smoothing for the ticks of the x axis.
    Use this to antialias thin lines.
*/

/*!
    \qmlproperty color GraphTheme::axisXLabelsColor
    Sets the color of the label for the x axis.
*/

/*!
    \qmlproperty font GraphTheme::axisXLabelsFont
    Sets the font of the label for the x axis.
*/

/*!
    \qmlproperty bool GraphTheme::shadowEnable
    Sets shadows on and off for axex and grid.
*/

/*!
    \qmlproperty color GraphTheme::shadowColor
    Sets the color of shadows.
*/

/*!
    \qmlproperty real GraphTheme::shadowBarWidth
    Sets the width of shadow bars.
*/

/*!
    \qmlproperty real GraphTheme::shadowXOffset
    Sets the offset of shadows in x axis.
*/

/*!
    \qmlproperty real GraphTheme::shadowYOffset
    Sets the offset of shadows in y axis.
*/

/*!
    \qmlproperty real GraphTheme::shadowSmoothing
    Sets the amount of smoothing for the shadows.
    Use this to antialias thin lines.
*/

QGraphTheme::QGraphTheme(QObject *parent)
    : QObject{parent}
{
}

void QGraphTheme::classBegin()
{
}

void QGraphTheme::componentComplete()
{
    // Set initial theme if not one set already
    resetColorTheme();
    m_componentComplete = true;
}

void QGraphTheme::updateTheme()
{
    m_themeDirty = true;
    emit update();
}

void QGraphTheme::resetColorTheme()
{
    setColorTheme(QGraphTheme::ColorThemeDark);
}

QGraphTheme::ColorTheme QGraphTheme::colorTheme() const
{
    return m_colorTheme;
}

void QGraphTheme::setColorTheme(const ColorTheme &newColorTheme)
{
    if (m_componentComplete) {
        qDebug() << "reseting theme!";
        m_customFlags = QGraphTheme::CustomFlags();
        m_themeDirty = true;
    }

    if (m_colorTheme == newColorTheme && !m_themeDirty && m_componentComplete)
        return;
    m_colorTheme = newColorTheme;

    if (m_colorTheme == QGraphTheme::ColorThemeLight) {
        setColorThemeLight();
    } else {
        setColorThemeDark();
    }

    emit update();
    emit colorThemeChanged();
}

// Theme that it suitable on top of light backgrounds
void QGraphTheme::setColorThemeLight()
{
    if (!m_customFlags.gridMajorBarsColor)
        setGridMajorBarsColor(QColor(20, 20, 20));
    if (!m_customFlags.gridMinorBarsColor)
        setGridMinorBarsColor(QColor(50, 50, 50));
    if (!m_customFlags.axisYMajorColor)
        setAxisYMajorColor(QColor(20, 20, 20));
    if (!m_customFlags.axisYMinorColor)
        setAxisYMinorColor(QColor(50, 50, 50));
    if (!m_customFlags.axisYLabelsColor)
        setAxisYLabelsColor(QColor(20, 20, 20));
    if (!m_customFlags.axisXMajorColor)
        setAxisXMajorColor(QColor(20, 20, 20));
    if (!m_customFlags.axisXMinorColor)
        setAxisXMinorColor(QColor(50, 50, 50));
    if (!m_customFlags.axisXLabelsColor)
        setAxisXLabelsColor(QColor(20, 20, 20));
}

// Theme that it suitable on top of dark backgrounds
void QGraphTheme::setColorThemeDark()
{
    if (!m_customFlags.gridMajorBarsColor)
        setGridMajorBarsColor(QColor(250, 250, 250));
    if (!m_customFlags.gridMinorBarsColor)
        setGridMinorBarsColor(QColor(150, 150, 150));
    if (!m_customFlags.axisYMajorColor)
        setAxisYMajorColor(QColor(250, 250, 250));
    if (!m_customFlags.axisYMinorColor)
        setAxisYMinorColor(QColor(150, 150, 150));
    if (!m_customFlags.axisYLabelsColor)
        setAxisYLabelsColor(QColor(250, 250, 250));
    if (!m_customFlags.axisXMajorColor)
        setAxisXMajorColor(QColor(250, 250, 250));
    if (!m_customFlags.axisXMinorColor)
        setAxisXMinorColor(QColor(150, 150, 150));
    if (!m_customFlags.axisXLabelsColor)
        setAxisXLabelsColor(QColor(250, 250, 250));
}

qreal QGraphTheme::gridMajorBarsWidth() const
{
    return m_gridMajorBarsWidth;
}

void QGraphTheme::setGridMajorBarsWidth(qreal newGridMajorBarsWidth)
{
    if (qFuzzyCompare(m_gridMajorBarsWidth, newGridMajorBarsWidth))
        return;
    m_gridMajorBarsWidth = newGridMajorBarsWidth;
    updateTheme();
    emit gridMajorBarsWidthChanged();
}

qreal QGraphTheme::gridMinorBarsWidth() const
{
    return m_gridMinorBarsWidth;
}

void QGraphTheme::setGridMinorBarsWidth(qreal newGridMinorBarsWidth)
{
    if (qFuzzyCompare(m_gridMinorBarsWidth, newGridMinorBarsWidth))
        return;
    m_gridMinorBarsWidth = newGridMinorBarsWidth;
    updateTheme();
    emit gridMinorBarsWidthChanged();
}

qreal QGraphTheme::gridSmoothing() const
{
    return m_gridSmoothing;
}

void QGraphTheme::setGridSmoothing(qreal newGridSmoothing)
{
    if (qFuzzyCompare(m_gridSmoothing, newGridSmoothing))
        return;
    m_gridSmoothing = newGridSmoothing;
    updateTheme();
    emit gridSmoothingChanged();
}

QColor QGraphTheme::gridMajorBarsColor() const
{
    return m_gridMajorBarsColor;
}

void QGraphTheme::setGridMajorBarsColor(const QColor &newGridMajorBarsColor)
{
    if (m_gridMajorBarsColor == newGridMajorBarsColor)
        return;
    m_gridMajorBarsColor = newGridMajorBarsColor;
    m_customFlags.gridMajorBarsColor = true;
    updateTheme();
    emit gridMajorBarsColorChanged();
}

QColor QGraphTheme::gridMinorBarsColor() const
{
    return m_gridMinorBarsColor;
}

void QGraphTheme::setGridMinorBarsColor(const QColor &newGridMinorBarsColor)
{
    if (m_gridMinorBarsColor == newGridMinorBarsColor)
        return;
    m_gridMinorBarsColor = newGridMinorBarsColor;
    m_customFlags.gridMinorBarsColor = true;
    updateTheme();
    emit gridMinorBarsColorChanged();
}

QColor QGraphTheme::axisYMajorColor() const
{
    return m_axisYMajorColor;
}

void QGraphTheme::setAxisYMajorColor(const QColor &newAxisYMajorColor)
{
    if (m_axisYMajorColor == newAxisYMajorColor)
        return;
    m_axisYMajorColor = newAxisYMajorColor;
    m_customFlags.axisYMajorColor = true;
    updateTheme();
    emit axisYMajorColorChanged();
}

QColor QGraphTheme::axisYMinorColor() const
{
    return m_axisYMinorColor;
}

void QGraphTheme::setAxisYMinorColor(const QColor &newAxisYMinorColor)
{
    if (m_axisYMinorColor == newAxisYMinorColor)
        return;
    m_axisYMinorColor = newAxisYMinorColor;
    m_customFlags.axisYMinorColor = true;
    updateTheme();
    emit axisYMinorColorChanged();
}

qreal QGraphTheme::axisYMajorBarWidth() const
{
    return m_axisYMajorBarWidth;
}

void QGraphTheme::setAxisYMajorBarWidth(qreal newAxisYMajorBarWidth)
{
    if (qFuzzyCompare(m_axisYMajorBarWidth, newAxisYMajorBarWidth))
        return;
    m_axisYMajorBarWidth = newAxisYMajorBarWidth;
    updateTheme();
    emit axisYMajorBarWidthChanged();
}

qreal QGraphTheme::axisYMinorBarWidth() const
{
    return m_axisYMinorBarWidth;
}

void QGraphTheme::setAxisYMinorBarWidth(qreal newAxisYMinorBarWidth)
{
    if (qFuzzyCompare(m_axisYMinorBarWidth, newAxisYMinorBarWidth))
        return;
    m_axisYMinorBarWidth = newAxisYMinorBarWidth;
    updateTheme();
    emit axisYMinorBarWidthChanged();
}

qreal QGraphTheme::axisYSmoothing() const
{
    return m_axisYSmoothing;
}

void QGraphTheme::setAxisYSmoothing(qreal newAxisYSmoothing)
{
    if (qFuzzyCompare(m_axisYSmoothing, newAxisYSmoothing))
        return;
    m_axisYSmoothing = newAxisYSmoothing;
    updateTheme();
    emit axisYSmoothingChanged();
}

QColor QGraphTheme::axisYLabelsColor() const
{
    return m_axisYLabelsColor;
}

void QGraphTheme::setAxisYLabelsColor(const QColor &newAxisYLabelsColor)
{
    if (m_axisYLabelsColor == newAxisYLabelsColor)
        return;
    m_axisYLabelsColor = newAxisYLabelsColor;
    m_customFlags.axisYLabelsColor = true;
    updateTheme();
    emit axisYLabelsColorChanged();
}

QFont QGraphTheme::axisYLabelsFont() const
{
    return m_axisYLabelsFont;
}

void QGraphTheme::setAxisYLabelsFont(const QFont &newAxisYLabelsFont)
{
    if (m_axisYLabelsFont == newAxisYLabelsFont)
        return;
    m_axisYLabelsFont = newAxisYLabelsFont;
    updateTheme();
    emit axisYLabelsFontChanged();
}

QColor QGraphTheme::axisXMajorColor() const
{
    return m_axisXMajorColor;
}

void QGraphTheme::setAxisXMajorColor(const QColor &newAxisXMajorColor)
{
    if (m_axisXMajorColor == newAxisXMajorColor)
        return;
    m_axisXMajorColor = newAxisXMajorColor;
    m_customFlags.axisXMajorColor = true;
    updateTheme();
    emit axisXMajorColorChanged();
}

QColor QGraphTheme::axisXMinorColor() const
{
    return m_axisXMinorColor;
}

void QGraphTheme::setAxisXMinorColor(const QColor &newAxisXMinorColor)
{
    if (m_axisXMinorColor == newAxisXMinorColor)
        return;
    m_axisXMinorColor = newAxisXMinorColor;
    m_customFlags.axisXMinorColor = true;
    updateTheme();
    emit axisXMinorColorChanged();
}

qreal QGraphTheme::axisXMajorBarWidth() const
{
    return m_axisXMajorBarWidth;
}

void QGraphTheme::setAxisXMajorBarWidth(qreal newAxisXMajorBarWidth)
{
    if (qFuzzyCompare(m_axisXMajorBarWidth, newAxisXMajorBarWidth))
        return;
    m_axisXMajorBarWidth = newAxisXMajorBarWidth;
    updateTheme();
    emit axisXMajorBarWidthChanged();
}

qreal QGraphTheme::axisXMinorBarWidth() const
{
    return m_axisXMinorBarWidth;
}

void QGraphTheme::setAxisXMinorBarWidth(qreal newAxisXMinorBarWidth)
{
    if (qFuzzyCompare(m_axisXMinorBarWidth, newAxisXMinorBarWidth))
        return;
    m_axisXMinorBarWidth = newAxisXMinorBarWidth;
    updateTheme();
    emit axisXMinorBarWidthChanged();
}

qreal QGraphTheme::axisXSmoothing() const
{
    return m_axisXSmoothing;
}

void QGraphTheme::setAxisXSmoothing(qreal newAxisXSmoothing)
{
    if (qFuzzyCompare(m_axisXSmoothing, newAxisXSmoothing))
        return;
    m_axisXSmoothing = newAxisXSmoothing;
    updateTheme();
    emit axisXSmoothingChanged();
}

QColor QGraphTheme::axisXLabelsColor() const
{
    return m_axisXLabelsColor;
}

void QGraphTheme::setAxisXLabelsColor(const QColor &newAxisXLabelsColor)
{
    if (m_axisXLabelsColor == newAxisXLabelsColor)
        return;
    m_axisXLabelsColor = newAxisXLabelsColor;
    m_customFlags.axisXLabelsColor = true;
    updateTheme();
    emit axisXLabelsColorChanged();
}

QFont QGraphTheme::axisXLabelsFont() const
{
    return m_axisXLabelsFont;
}

void QGraphTheme::setAxisXLabelsFont(const QFont &newAxisXLabelsFont)
{
    if (m_axisXLabelsFont == newAxisXLabelsFont)
        return;
    m_axisXLabelsFont = newAxisXLabelsFont;
    updateTheme();
    emit axisXLabelsFontChanged();
}

bool QGraphTheme::shadowEnabled() const
{
    return m_shadowEnabled;
}

void QGraphTheme::setShadowEnabled(bool newShadowEnabled)
{
    if (m_shadowEnabled == newShadowEnabled)
        return;
    m_shadowEnabled = newShadowEnabled;
    updateTheme();
    emit shadowEnabledChanged();
}


QColor QGraphTheme::shadowColor() const
{
    return m_shadowColor;
}

void QGraphTheme::setShadowColor(const QColor &newShadowColor)
{
    if (m_shadowColor == newShadowColor)
        return;
    m_shadowColor = newShadowColor;
    updateTheme();
    emit shadowColorChanged();
}

qreal QGraphTheme::shadowBarWidth() const
{
    return m_shadowBarWidth;
}

void QGraphTheme::setShadowBarWidth(qreal newShadowBarWidth)
{
    if (qFuzzyCompare(m_shadowBarWidth, newShadowBarWidth))
        return;
    m_shadowBarWidth = newShadowBarWidth;
    updateTheme();
    emit shadowBarWidthChanged();
}

qreal QGraphTheme::shadowXOffset() const
{
    return m_shadowXOffset;
}

void QGraphTheme::setShadowXOffset(qreal newShadowXOffset)
{
    if (qFuzzyCompare(m_shadowXOffset, newShadowXOffset))
        return;
    m_shadowXOffset = newShadowXOffset;
    updateTheme();
    emit shadowXOffsetChanged();
}

qreal QGraphTheme::shadowYOffset() const
{
    return m_shadowYOffset;
}

void QGraphTheme::setShadowYOffset(qreal newShadowYOffset)
{
    if (qFuzzyCompare(m_shadowYOffset, newShadowYOffset))
        return;
    m_shadowYOffset = newShadowYOffset;
    updateTheme();
    emit shadowYOffsetChanged();
}

qreal QGraphTheme::shadowSmoothing() const
{
    return m_shadowSmoothing;
}

void QGraphTheme::setShadowSmoothing(qreal newShadowSmoothing)
{
    if (qFuzzyCompare(m_shadowSmoothing, newShadowSmoothing))
        return;
    m_shadowSmoothing = newShadowSmoothing;
    updateTheme();
    emit shadowSmoothingChanged();
}

QT_END_NAMESPACE
