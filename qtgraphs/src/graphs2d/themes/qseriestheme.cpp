// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qseriestheme.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSeriesTheme
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QSeriesTheme class handles theming of series.

    Series theme is used to handle color, border color and border width.
*/
/*!
    \qmltype SeriesTheme
    \instantiates QSeriesTheme
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D

    \brief Handles theming of series.

    Series theme is used to handle color, border color and border width.

    \sa GraphTheme
*/

/*!
    \qmlproperty SeriesColorTheme SeriesTheme::colorTheme
    List of premade themes that determine values for properties of this class.
*/

/*!
    \qmlproperty list SeriesTheme::colors
    List of colors used to pick color of the series or set.
*/

/*!
    \qmlproperty list SeriesTheme::borderColors
    List of colors used to pick border color of the series or set.
*/

/*!
    \qmlproperty real SeriesTheme::borderWidth
    Sets the width of the border of the series.
*/

QSeriesTheme::QSeriesTheme(QObject *parent)
    : QObject{parent}
{
}

void QSeriesTheme::classBegin()
{
}

void QSeriesTheme::componentComplete()
{
    // Set initial theme if not one set already
    if (!m_useCustomColors)
        setColorTheme(m_colorTheme);
    m_componentComplete = true;
}

void QSeriesTheme::resetColorTheme()
{
    setColorTheme(SeriesColorTheme::SeriesTheme1);
}

QSeriesTheme::SeriesColorTheme QSeriesTheme::colorTheme() const
{
    return m_colorTheme;
}

void QSeriesTheme::setColorTheme(const QSeriesTheme::SeriesColorTheme &newColorTheme)
{
    if (m_componentComplete)
        m_themeDirty = true;

    if (m_colorTheme == newColorTheme && !m_themeDirty && m_componentComplete)
        return;

    m_colorTheme = newColorTheme;

    if (m_colorTheme == QSeriesTheme::SeriesTheme1) {
        setColorTheme1();
    } else {
        setColorTheme2();
    }

    emit update();
    emit colorThemeChanged();
}

void QSeriesTheme::setColorTheme1()
{
    m_colors = { "#3d9c73", "#63b179", "#88c580", "#aed987", "#d6ec91", "#ffff9d",
                       "#fee17e", "#fcc267", "#f7a258", "#ef8250", "#e4604e", "#d43d51" };
    // TODO
    m_borderColors = { "#ffffff" };
}

void QSeriesTheme::setColorTheme2()
{
    m_colors = { "#00429d", "#485ba8", "#6c77b3", "#8a94be", "#a4b2ca", "#b9d4d6",
                       "#ffd3bf", "#ffa59e", "#f4777f", "#dd4c65", "#be214d", "#93003a" };
    // TODO
    m_borderColors = { "#ffffff" };
}

QColor QSeriesTheme::indexColorFrom(const QList<QColor> &colors, int index) const
{
    // Select colors from theme with as much separation as possible. So:
    // - if we need 2 series from 12 color palette, select indexes [0, 11]
    // - If we need 3 series from 12 color palette, select indexes [0, 5, 11]
    if (colors.isEmpty())
        return QColor();
    if (m_seriesCount <= 1) {
        if (!colors.isEmpty())
            return colors.first();
        else
            return QColor();
    }
    int ci = (float)index * ((float)(colors.size() - 1) / (m_seriesCount - 1));
    ci = std::min(ci, (int)colors.size() - 1);
    return colors.at(ci);
}

int QSeriesTheme::graphSeriesCount() const
{
    return m_seriesCount;
}
void QSeriesTheme::setGraphSeriesCount(int count)
{
    m_seriesCount = count;
}


QColor QSeriesTheme::graphSeriesColor(int index) const
{
    return indexColorFrom(m_colors, index);
}

QColor QSeriesTheme::graphSeriesBorderColor(int index) const
{
    return indexColorFrom(m_borderColors, index);
}

QList<QColor> QSeriesTheme::colors() const
{
    return m_colors;
}

void QSeriesTheme::setColors(const QList<QColor> &newColors)
{
    if (m_colors == newColors)
        return;
    m_colors = newColors;
    m_useCustomColors = !m_colors.isEmpty();
    emit colorsChanged();
}

QList<QColor> QSeriesTheme::borderColors() const
{
    return m_borderColors;
}

void QSeriesTheme::setBorderColors(const QList<QColor> &newBorderColors)
{
    if (m_borderColors == newBorderColors)
        return;
    m_borderColors = newBorderColors;
    emit borderColorsChanged();
}

qreal QSeriesTheme::borderWidth() const
{
    return m_borderWidth;
}

void QSeriesTheme::setBorderWidth(qreal newBorderWidth)
{
    if (qFuzzyCompare(m_borderWidth, newBorderWidth))
        return;
    m_borderWidth = newBorderWidth;
    emit borderWidthChanged();
}

QT_END_NAMESPACE
