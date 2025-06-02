// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qareaseries.h>
#include <private/qareaseries_p.h>
#include <private/qgraphsview_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAreaSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QAreaSeries class presents data in area graphs.

    An area graph is used to draw an area composed by points.
    The points are defined by two series: upperSeries and lowerSeries.
    The area between the series is drawn as a graph. If only the upperSeries
    is defined, the area is then between the bottom of the graph and the upper series.
*/
/*!
    \qmltype AreaSeries
    \nativetype QAreaSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits AbstractSeries

    \brief Presents data in area graphs.

    An area graph is used to draw an area composed by points.
    The points are defined by two series: upperSeries and lowerSeries.
    The area between the series is drawn as a graph. If only the upperSeries
    is defined, the area is then between the bottom of the graph and the upper series.

    \image graphs2d-area.png
*/

/*!
    \property QAreaSeries::color
    \brief The fill color of the area.
    The default value is \c Qt::transparent, meaning the color is defined by the theme.
*/
/*!
    \qmlproperty color AreaSeries::color
    The fill color of the area.
    The default value is \c transparent, meaning the color is defined by the theme.
*/

/*!
    \property QAreaSeries::selectedColor
    \brief The fill color of the area when selected.
    The default value is \c Qt::transparent, meaning the selected color is defined by the theme.
*/
/*!
    \qmlproperty color AreaSeries::selectedColor
    The fill color of the area when selected.
    The default value is \c transparent, meaning the selected color is defined by the theme.
*/

/*!
    \property QAreaSeries::borderColor
    \brief The border color of the area.
    The default value is \c Qt::transparent, meaning the border color is defined by the theme.
*/
/*!
    \qmlproperty color AreaSeries::borderColor
    The border color of the area.
    The default value is \c transparent, meaning the border color is defined by the theme.
*/

/*!
    \property QAreaSeries::selectedBorderColor
    \brief The border color of the area when selected.
    The default value is \c Qt::transparent, meaning the selected border color is defined by the theme.
*/
/*!
    \qmlproperty color AreaSeries::selectedBorderColor
    The border color of the area when selected.
    The default value is \c transparent, meaning the selected border color is defined by the theme.
*/

/*!
    \property QAreaSeries::borderWidth
    \brief The width of the line that encloses the area.
    The default value is \c -1, meaning the border width is defined by the theme.
*/
/*!
    \qmlproperty real AreaSeries::borderWidth
    The width of the line that encloses the area.
    The default value is \c -1, meaning the border width is defined by the theme.
*/

/*!
    \property QAreaSeries::selected
    \brief Sets this area as selected.
    The default value is \c false.
*/
/*!
    \qmlproperty bool AreaSeries::selected
    Sets this area as selected.
    The default value is \c false.
*/

/*!
    \property QAreaSeries::upperSeries
    \brief Sets the upper boundary of the area. No area is drawn if this is null.
*/
/*!
    \qmlproperty XYSeries AreaSeries::upperSeries
    Sets the upper boundary of the area. No area is drawn if this is null.
*/

/*!
    \property QAreaSeries::lowerSeries
    \brief Sets the lower boundary of the area. If this is null, the graph bottom
    is considered the lower bound.
*/
/*!
    \qmlproperty XYSeries AreaSeries::lowerSeries
    Sets the lower boundary of the area. If this is null, the graph bottom
    is considered the lower bound.
*/

/*!
    \qmlsignal AreaSeries::colorChanged(color newColor)
    This signal is emitted when the area \l color changes to \a newColor.
*/

/*!
    \qmlsignal AreaSeries::selectedColorChanged(color newSelectedColor)
    This signal is emitted when the color of a selected area changes to
    \a newSelectedColor.
*/

/*!
    \qmlsignal AreaSeries::borderColorChanged(color newBorderColor)
    This signal is emitted when the area border color changes to
    \a newBorderColor.
*/

/*!
    \qmlsignal AreaSeries::selectedBorderColorChanged(color newSelectedBorderColor);
    This signal is emitted when the border color of a selected area changes to
    \a newSelectedBorderColor.
*/

/*!
    \qmlsignal AreaSeries::borderWidthChanged();
    This signal is emitted when the width of the area border width changes.
*/

/*!
    \qmlsignal AreaSeries::selectedChanged();
    This signal is emitted when the current area is selected.
*/

/*!
    \qmlsignal AreaSeries::upperSeriesChanged();
    This signal is emitted when the upper series changes.
*/

/*!
    \qmlsignal AreaSeries::lowerSeriesChanged();
    This signal is emitted when the lower series changes.
*/

QAreaSeries::QAreaSeries(QObject *parent)
    : QAbstractSeries(*(new QAreaSeriesPrivate()), parent)
{}

QAreaSeries::~QAreaSeries() {}

QAreaSeries::QAreaSeries(QAreaSeriesPrivate &dd, QObject *parent)
    : QAbstractSeries(dd, parent)
{}

QAbstractSeries::SeriesType QAreaSeries::type() const
{
    return QAbstractSeries::SeriesType::Area;
}

QColor QAreaSeries::color() const
{
    Q_D(const QAreaSeries);
    return d->m_color;
}

void QAreaSeries::setColor(QColor newColor)
{
    Q_D(QAreaSeries);
    if (color() != newColor) {
        d->m_color = newColor;
        emit colorChanged(newColor);
    }
}

QColor QAreaSeries::selectedColor() const
{
    Q_D(const QAreaSeries);
    return d->m_selectedColor;
}

void QAreaSeries::setSelectedColor(QColor newSelectedColor)
{
    Q_D(QAreaSeries);
    if (selectedColor() != newSelectedColor) {
        d->m_selectedColor = newSelectedColor;
        emit selectedColorChanged(newSelectedColor);
    }
}

QColor QAreaSeries::borderColor() const
{
    Q_D(const QAreaSeries);
    return d->m_borderColor;
}

void QAreaSeries::setBorderColor(QColor newBorderColor)
{
    Q_D(QAreaSeries);
    if (d->m_borderColor == newBorderColor)
        return;
    d->m_borderColor = newBorderColor;
    emit borderColorChanged(newBorderColor);
}

QColor QAreaSeries::selectedBorderColor() const
{
    Q_D(const QAreaSeries);
    return d->m_selectedBorderColor;
}

void QAreaSeries::setSelectedBorderColor(QColor newSelectedBorderColor)
{
    Q_D(QAreaSeries);
    if (d->m_selectedBorderColor == newSelectedBorderColor)
        return;
    d->m_selectedBorderColor = newSelectedBorderColor;
    emit selectedBorderColorChanged(newSelectedBorderColor);
}

qreal QAreaSeries::borderWidth() const
{
    Q_D(const QAreaSeries);
    return d->m_borderWidth;
}

void QAreaSeries::setBorderWidth(qreal newBorderWidth)
{
    Q_D(QAreaSeries);
    if (qFuzzyCompare(d->m_borderWidth, newBorderWidth))
        return;
    d->m_borderWidth = newBorderWidth;
    emit borderWidthChanged();
}

bool QAreaSeries::isSelected() const
{
    Q_D(const QAreaSeries);
    return d->m_selected;
}

void QAreaSeries::setSelected(bool newSelected)
{
    Q_D(QAreaSeries);
    if (d->m_selected == newSelected)
        return;
    d->m_selected = newSelected;
    emit selectedChanged();
}

QXYSeries *QAreaSeries::upperSeries() const
{
    Q_D(const QAreaSeries);
    return d->m_upperSeries;
}

void QAreaSeries::setUpperSeries(QXYSeries *newUpperSeries)
{
    Q_D(QAreaSeries);
    if (d->m_upperSeries == newUpperSeries)
        return;

    if (d->m_upperSeries)
        disconnect(newUpperSeries, &QXYSeries::update, this, &QAreaSeries::update);

    d->m_upperSeries = newUpperSeries;

    connect(newUpperSeries, &QXYSeries::update, this, &QAreaSeries::update);

    emit upperSeriesChanged();
}

QXYSeries *QAreaSeries::lowerSeries() const
{
    Q_D(const QAreaSeries);
    return d->m_lowerSeries;
}

void QAreaSeries::setLowerSeries(QXYSeries *newLowerSeries)
{
    Q_D(QAreaSeries);
    if (d->m_lowerSeries == newLowerSeries)
        return;

    if (d->m_lowerSeries)
        disconnect(newLowerSeries, &QXYSeries::update, this, &QAreaSeries::update);

    d->m_lowerSeries = newLowerSeries;

    connect(newLowerSeries, &QXYSeries::update, this, &QAreaSeries::update);

    emit lowerSeriesChanged();
}

QAreaSeriesPrivate::QAreaSeriesPrivate() {}

QT_END_NAMESPACE
