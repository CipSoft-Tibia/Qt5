// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qabstractseries.h>
#include <private/qabstractseries_p.h>
#include <private/qgraphsview_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QAbstractSeries class is a base class for all Qt Graphs for 2D series.

    Usually, the series type specific inherited classes are used instead of the base class.

    \sa QLineSeries, QScatterSeries, QBarSeries, QAbstractBarSeries, QXYSeries
*/
/*!
    \qmltype AbstractSeries
    \instantiates QAbstractSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \brief Base type for all Qt Graph series types.

    This type cannot be instantiated directly. Instead, one of the following derived types
    should be used to create a series: LineSeries, BarSeries, or ScatterSeries.
*/

/*!
    \enum QAbstractSeries::SeriesType

    This enum describes the type of the series.

    \value SeriesTypeLine A line chart.
    \value SeriesTypeBar A vertical bar chart.
    \value SeriesTypeScatter A scatter chart.
*/

/*!
    \property QAbstractSeries::type
    \brief The type of the series.
*/
/*!
    \qmlproperty enumeration AbstractSeries::type

    The type of the series.

    \value AbstractSeries.SeriesTypeLine A line graph.
    \value AbstractSeries.SeriesTypeBar A vertical bar graph.
    \value AbstractSeries.SeriesTypeScatter A scatter graph.
*/

/*!
    \property QAbstractSeries::theme
    \brief The theme of the series.

    The theme used for this series.
*/
/*!
    \qmlproperty QSeriesTheme AbstractSeries::theme
    The theme used for this series.
*/

/*!
    \property QAbstractSeries::name
    \brief The name of the series.

    The name is displayed in the legend for the series and it supports HTML formatting.
*/
/*!
    \qmlproperty string AbstractSeries::name
    The name of the series. It is displayed in the legend for the series and it
    supports HTML formatting.
*/

/*!
    \property QAbstractSeries::visible
    \brief Visibility of the series.

    The visibility used for this series. By default, \a visible is set to \c true.
*/
/*!
    \qmlproperty bool AbstractSeries::visible
    The visibility used for this series. By default, \a visible is set to \c true.
*/

/*!
    \property QAbstractSeries::selectable
    \brief Controls if the series is selectable.

    Controls if the series can be selected with mouse/touch.
    By default, \a selectable is set to \c false.
*/
/*!
    \qmlproperty bool AbstractSeries::selectable
    Controls if the series can be selected with mouse/touch.
    By default, \a selectable is set to \c false.
*/

/*!
    \property QAbstractSeries::hoverable
    \brief Controls if the series is hoverable.

    Controls if the series can be hovered with mouse/touch.
    By default, \a hoverable is set to \c false.
*/
/*!
    \qmlproperty bool AbstractSeries::hoverable
    Controls if the series can be hovered with mouse/touch.
    By default, \a hoverable is set to \c false.
*/

/*!
    \property QAbstractSeries::opacity
    \brief The opacity of the series.

    By default, the opacity is 1.0. The valid values range from 0.0 (transparent) to 1.0 (opaque).
*/
/*!
    \qmlproperty real AbstractSeries::opacity
    The opacity of the series. By default, the opacity is 1.0.
    The valid values range from 0.0 (transparent) to 1.0 (opaque).
*/

/*!
    \property QAbstractSeries::valuesMultiplier
    \brief Controls the series values effective visible value.

    This variable can be used for animating the series values so they scale from 0 to actual value size.
    By default, the valuesMultiplier is 1.0. The valid values range from 0.0 (height 0) to 1.0 (full value).
*/
/*!
    \qmlproperty real AbstractSeries::valuesMultiplier
    This variable can be used for animating the series values so they scale from 0 to actual value size.
    By default, the valuesMultiplier is 1.0. The valid values range from 0.0 (height 0) to 1.0 (full value).
*/

/*!
    \fn void QAbstractSeries::themeChanged()
    This signal is emitted when the series theme changes.
*/

/*!
    \fn void QAbstractSeries::nameChanged()
    This signal is emitted when the series name changes.
*/

/*!
    \fn void QAbstractSeries::visibleChanged()
    This signal is emitted when the series visibility changes.
*/

/*!
    \fn void QAbstractSeries::selectableChanged()
    This signal is emitted when the series selectable changes.
*/

/*!
    \fn void QAbstractSeries::hoverableChanged()
    This signal is emitted when the series hoverable changes.
*/

/*!
    \fn void QAbstractSeries::opacityChanged()
    This signal is emitted when the opacity of the series changes.
*/

/*!
    \fn void QAbstractSeries::valuesMultiplierChanged()
    This signal is emitted when the valuesMultiplier of the series changes.
*/

/*!
    \fn void QAbstractSeries::hoverEnter(QString seriesName, QPointF position, QPointF value)
    This signal is emitted when the series hovering starts. The name of the series is in \a seriesName,
    the mouse/touch position in \a position, and the series value in \a value.
    \note This signal is only emitted when \l hoverable is set to true.
*/

/*!
    \fn void QAbstractSeries::hoverExit(QString seriesName, QPointF position)
    This signal is emitted when the series hovering ends. The name of the series is in \a seriesName,
    and the mouse/touch position in \a position.
    \note This signal is only emitted when \l hoverable is set to true.
*/

/*!
    \fn void QAbstractSeries::hover(QString seriesName, QPointF position, QPointF value)
    This signal is emitted when the series hovering changes. The name of the series is in \a seriesName,
    the mouse/touch position in \a position, and the series value in \a value.
    \note This signal is only emitted when \l hoverable is set to true.
*/

/*!
    \internal
    \brief Constructs QAbstractSeries object with \a parent.
*/
QAbstractSeries::QAbstractSeries(QAbstractSeriesPrivate &d, QObject *parent) :
    QObject(parent),
    d_ptr(&d)
{
}

/*!
    \brief Virtual destructor for the graph series.
*/
QAbstractSeries::~QAbstractSeries()
{
    //if (d_ptr->m_graph)
    //    qFatal("Series still bound to a graph when destroyed!");
    if (d_ptr->m_graph)
        d_ptr->m_graph->removeSeries(this);
}

QSeriesTheme *QAbstractSeries::theme() const
{
    return d_ptr->m_theme;
}

void QAbstractSeries::setTheme(QSeriesTheme *newTheme)
{
    if (d_ptr->m_theme == newTheme)
        return;
    d_ptr->m_theme = newTheme;
    update();
    emit themeChanged();
}

QString QAbstractSeries::name() const
{
    return d_ptr->m_name;
}

void QAbstractSeries::setName(const QString &name)
{
    if (name != d_ptr->m_name) {
        d_ptr->m_name = name;
        update();
        emit nameChanged();
    }
}

bool QAbstractSeries::isVisible() const
{
    return d_ptr->m_visible;
}

void QAbstractSeries::setVisible(bool visible)
{
    if (visible != d_ptr->m_visible) {
        d_ptr->m_visible = visible;
        update();
        emit visibleChanged();
    }
}

bool QAbstractSeries::selectable() const
{
    return d_ptr->m_selectable;
}

void QAbstractSeries::setSelectable(bool selectable)
{
    if (selectable != d_ptr->m_selectable) {
        d_ptr->m_selectable = selectable;
        update();
        emit selectableChanged();
    }
}

bool QAbstractSeries::hoverable() const
{
    return d_ptr->m_hoverable;
}

void QAbstractSeries::setHoverable(bool hoverable)
{
    if (hoverable != d_ptr->m_hoverable) {
        d_ptr->m_hoverable = hoverable;
        update();
        emit hoverableChanged();
    }
}

qreal QAbstractSeries::opacity() const
{
    return d_ptr->m_opacity;
}

void QAbstractSeries::setOpacity(qreal opacity)
{
    if (opacity != d_ptr->m_opacity) {
        d_ptr->m_opacity = opacity;
        update();
        emit opacityChanged();
    }
}

qreal QAbstractSeries::valuesMultiplier() const
{
    return d_ptr->m_valuesMultiplier;
}

void QAbstractSeries::setValuesMultiplier(qreal valuesMultiplier)
{
    valuesMultiplier = std::clamp(valuesMultiplier, 0.0, 1.0);
    if (valuesMultiplier != d_ptr->m_valuesMultiplier) {
        d_ptr->m_valuesMultiplier = valuesMultiplier;
        update();
        emit valuesMultiplierChanged();
    }
}

/*!
    Returns the graph that the series belongs to.

    Set automatically when the series is added to the graph,
    and unset when the series is removed from the graph.
*/
QGraphsView *QAbstractSeries::graph() const
{
    return d_ptr->m_graph;
}

void QAbstractSeries::setGraph(QGraphsView *graph)
{
    d_ptr->m_graph = graph;
    switch (type()) {
    case QAbstractSeries::SeriesTypeBar:
        graph->createBarsRenderer();
        graph->createAxisRenderer();
        break;
    case QAbstractSeries::SeriesTypeScatter:
    case QAbstractSeries::SeriesTypeLine:
        graph->createPointRenderer();
        graph->createAxisRenderer();
        break;
    default:
        break;
    }

    if (d_ptr->m_graph) {
        // Attach pending axes
        for (auto axis : d_ptr->m_axes) {
            d_ptr->m_graph->addAxis(axis);
            QObject::connect(axis, &QAbstractAxis::update, d_ptr->m_graph, &QQuickItem::update);
        }
    }
}

/*!
    Sets the visibility of the series to \c true.

    \sa setVisible(), isVisible()
*/
void QAbstractSeries::show()
{
    setVisible(true);
}

/*!
    Sets the visibility of the series to \c false.

    \sa setVisible(), isVisible()
*/
void QAbstractSeries::hide()
{
    setVisible(false);
}

/*!
    Attaches the axis specified by \a axis to the series.

    Returns \c true if the axis was attached successfully, \c false otherwise.

    \note If multiple axes of the same orientation are attached to the same series,
    they will have the same minimum and maximum values.
*/
bool QAbstractSeries::attachAxis(QAbstractAxis* axis)
{
    d_ptr->m_axes.append(axis);
    if (d_ptr->m_graph) {
        d_ptr->m_graph->addAxis(axis);
        QObject::connect(axis, &QAbstractAxis::update, d_ptr->m_graph, &QQuickItem::update);
        return true;
    }
    //qWarning("Series not in the graph. Please addSeries to graph first.");
    return false;
}

/*!
    Detaches the axis specified by \a axis from the series.

    Returns \c true if the axis was detached successfully, \c false otherwise.
*/
bool QAbstractSeries::detachAxis(QAbstractAxis* axis)
{
    d_ptr->m_axes.removeAll(axis);
    if (d_ptr->m_graph) {
        d_ptr->m_graph->removeAxis(axis);
        QObject::disconnect(axis, &QAbstractAxis::update, d_ptr->m_graph, &QQuickItem::update);
        return true;
    }
    //qWarning("Series not in the graph. Please addSeries to graph first.");
    return false;
}

/*!
    Returns the list of axes attached to the series. Usually, an x-axis and a y-axis
    are attached to a series, except for QPieSeries, which does not have any axes attached.
 \sa attachAxis(), detachAxis()
 */
QList<QAbstractAxis*> QAbstractSeries::attachedAxes()
{
    return d_ptr->m_axes;
}

QQmlListProperty<QObject> QAbstractSeries::seriesChildren()
{
    return QQmlListProperty<QObject>(this, 0, &QAbstractSeries::appendSeriesChildren ,0,0,0);
}

void QAbstractSeries::appendSeriesChildren(QQmlListProperty<QObject> *list, QObject *element)
{
    // Empty implementation; the children are parsed in componentComplete instead
    Q_UNUSED(list);
    Q_UNUSED(element);
}

void QAbstractSeries::classBegin()
{
}

void QAbstractSeries::componentComplete()
{
    if (!d_ptr->m_theme) {
        d_ptr->m_theme = new QSeriesTheme(this);
        d_ptr->m_theme->resetColorTheme();
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////

QAbstractSeriesPrivate::QAbstractSeriesPrivate(QAbstractSeries *q)
    : q_ptr(q),
      m_graph(nullptr),
      m_visible(true),
      m_opacity(1.0),
      m_valuesMultiplier(1.0)
{
}

QAbstractSeriesPrivate::~QAbstractSeriesPrivate()
{
}

QT_END_NAMESPACE

#include "moc_qabstractseries.cpp"
#include "moc_qabstractseries_p.cpp"
