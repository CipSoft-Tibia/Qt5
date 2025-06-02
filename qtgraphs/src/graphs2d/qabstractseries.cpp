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

    \sa QLineSeries, QSplineSeries, QScatterSeries, QBarSeries, QXYSeries
*/
/*!
    \qmltype AbstractSeries
    \nativetype QAbstractSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \brief Base type for all Qt Graph series types.

    This type cannot be instantiated directly. Instead, one of the following derived types
    should be used to create a series: LineSeries, SplineSeries, BarSeries, or ScatterSeries.
*/

/*!
    \class QLegendData
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QLegendData struct contains information to display on a sets
    legend marker.

    The information needed to make a visual association between a set and a
    marker include properties such as color, border color, and a name of a set.

    \sa QLineSeries, QSplineSeries, QScatterSeries, QBarSeries, QXYSeries
*/

/*!
    \qmltype LegendData
    \nativetype QLegendData
    \inqmlmodule QtGraphs
    \ingroup graphs_qml__2D
    \brief The LegendData struct contains information to display on a sets
    legend marker.

    The information needed to make a visual association between a set and a
    marker include properties such as color, border color, and a name of a set.
*/

/*!
    \property QLegendData::color
    \brief A legend marker's color.
*/
/*!
    \qmlproperty color LegendData::color
    \brief A legend marker's color.
*/

/*!
    \property QLegendData::borderColor
    \brief A border color of a legend marker.
*/
/*!
    \qmlproperty color LegendData::borderColor
    \brief A border color of a legend marker.
*/

/*!
    \property QLegendData::label
    \brief A name of a legend marker.
*/
/*!
    \qmlproperty string LegendData::label
    \brief A name of a legend marker.
*/

/*!
    \enum QAbstractSeries::SeriesType

    This enum describes the type of the series.

    \value Line A line graph.
    \value Bar A bar graph.
    \value Scatter A scatter graph.
    \value Pie A pie graph.
    \value Spline A spline graph.
    \value Area An area graph.
*/

/*!
    \property QAbstractSeries::type
    \brief The type of the series.
*/
/*!
    \qmlproperty enumeration AbstractSeries::type

    The type of the series.

    \value AbstractSeries.SeriesType.Line A line graph.
    \value AbstractSeries.SeriesType.Bar A bar graph.
    \value AbstractSeries.SeriesType.Scatter A scatter graph.
    \value AbstractSeries.SeriesType.Pie A pie graph.
    \value AbstractSeries.SeriesType.Spline A spline graph.
    \value AbstractSeries.SeriesType.Area An area graph.
*/

/*!
    \property QAbstractSeries::name
    \brief The name of the series.

    The name is displayed in the legend for the series and it supports HTML formatting.
*/
/*!
    \qmlproperty string AbstractSeries::name
    The name of the series. The name is displayed in the legend for the series and it
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
    \property QAbstractSeries::legendData
    \brief Contains information needed to create a legend marker for a data set in a graph.
    \sa QLegendData
 */
/*!
    \qmlproperty list<LegendData> AbstractSeries::legendData
    Contains information needed to create a legend marker for a data set in a graph.
*/

/*!
    \qmlsignal AbstractSeries::legendDataChanged()
    This signal is emitted when legend data changes.
*/

/*!
    \qmlsignal AbstractSeries::themeChanged()
    This signal is emitted when the series theme changes.
*/

/*!
    \qmlsignal AbstractSeries::nameChanged()
    This signal is emitted when the series \l name changes.
*/

/*!
    \qmlsignal AbstractSeries::visibleChanged()
    This signal is emitted when the series visibility changes.
*/

/*!
    \qmlsignal AbstractSeries::selectableChanged()
    This signal is emitted when the series \l selectable changes.
*/

/*!
    \qmlsignal AbstractSeries::hoverableChanged()
    This signal is emitted when the series \l hoverable changes.
*/

/*!
    \qmlsignal AbstractSeries::opacityChanged()
    This signal is emitted when the \l opacity of the series changes.
*/

/*!
    \qmlsignal AbstractSeries::valuesMultiplierChanged()
    This signal is emitted when the valuesMultiplier of the series changes.
*/

/*!
    \fn void QAbstractSeries::hoverEnter(const QString &seriesName, QPointF position, QPointF value)
    This signal is emitted when the series hovering starts. The name of the series is in \a seriesName,
    the mouse/touch position in \a position, and the series value in \a value.
    \note This signal is only emitted when \l hoverable is set to true.
*/

/*!
    \fn void QAbstractSeries::hoverExit(const QString &seriesName, QPointF position)
    This signal is emitted when the series hovering ends. The name of the series is in \a seriesName,
    and the mouse/touch position in \a position.
    \note This signal is only emitted when \l hoverable is set to true.
*/

/*!
    \fn void QAbstractSeries::hover(const QString &seriesName, QPointF position, QPointF value)
    This signal is emitted when the series hovering changes. The name of the series is in \a seriesName,
    the mouse/touch position in \a position, and the series value in \a value.
    \note This signal is only emitted when \l hoverable is set to true.
*/

/*!
    \internal
    \brief Constructs QAbstractSeries object with \a parent.
*/

QAbstractSeries::QAbstractSeries(QAbstractSeriesPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{}

/*!
    \brief Virtual destructor for the graph series.
*/
QAbstractSeries::~QAbstractSeries()
{
    Q_D(QAbstractSeries);
    if (d->m_graph)
        d->m_graph->removeSeries(this);
}

QString QAbstractSeries::name() const
{
    Q_D(const QAbstractSeries);
    return d->m_name;
}

void QAbstractSeries::setName(const QString &name)
{
    Q_D(QAbstractSeries);
    if (name != d->m_name) {
        d->m_name = name;
        update();
        emit nameChanged();
    }
}

bool QAbstractSeries::isVisible() const
{
    Q_D(const QAbstractSeries);
    return d->m_visible;
}

void QAbstractSeries::setVisible(bool visible)
{
    Q_D(QAbstractSeries);
    if (visible != d->m_visible) {
        d->m_visible = visible;
        update();
        emit visibleChanged();
    }
}

bool QAbstractSeries::isSelectable() const
{
    Q_D(const QAbstractSeries);
    return d->m_selectable;
}

void QAbstractSeries::setSelectable(bool selectable)
{
    Q_D(QAbstractSeries);
    if (selectable != d->m_selectable) {
        d->m_selectable = selectable;
        update();
        emit selectableChanged();
    }
}

bool QAbstractSeries::isHoverable() const
{
    Q_D(const QAbstractSeries);
    return d->m_hoverable;
}

void QAbstractSeries::setHoverable(bool hoverable)
{
    Q_D(QAbstractSeries);
    if (hoverable != d->m_hoverable) {
        d->m_hoverable = hoverable;
        update();
        emit hoverableChanged();
    }
}

bool QAbstractSeries::hasLoaded() const
{
    Q_D(const QAbstractSeries);
    return d->m_loaded;
}

qreal QAbstractSeries::opacity() const
{
    Q_D(const QAbstractSeries);
    return d->m_opacity;
}

void QAbstractSeries::setOpacity(qreal opacity)
{
    Q_D(QAbstractSeries);
    if (opacity != d->m_opacity) {
        d->m_opacity = opacity;
        update();
        emit opacityChanged();
    }
}

qreal QAbstractSeries::valuesMultiplier() const
{
    Q_D(const QAbstractSeries);
    return d->m_valuesMultiplier;
}

void QAbstractSeries::setValuesMultiplier(qreal valuesMultiplier)
{
    Q_D(QAbstractSeries);
    valuesMultiplier = std::clamp<qreal>(valuesMultiplier, 0.0, 1.0);
    if (valuesMultiplier != d->m_valuesMultiplier) {
        d->m_valuesMultiplier = valuesMultiplier;
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
    Q_D(const QAbstractSeries);
    return d->m_graph;
}

void QAbstractSeries::setGraph(QGraphsView *graph)
{
    Q_D(QAbstractSeries);
    d->m_graph = graph;
    if (graph) {
        switch (type()) {
        case SeriesType::Bar:
            graph->createBarsRenderer();
            break;
        case SeriesType::Scatter:
        case SeriesType::Line:
        case SeriesType::Spline:
            graph->createPointRenderer();
            break;
        case SeriesType::Pie:
            graph->createPieRenderer();
            break;
        case SeriesType::Area:
            graph->createAreaRenderer();
            break;
        default:
            break;
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

const QList<QLegendData> QAbstractSeries::legendData() const
{
    Q_D(const QAbstractSeries);
    return d->m_legendData;
}

QQmlListProperty<QObject> QAbstractSeries::seriesChildren()
{
    return QQmlListProperty<QObject>(this, 0, &QAbstractSeriesPrivate::appendSeriesChildren, 0, 0, 0);
}

void QAbstractSeries::classBegin()
{
}

void QAbstractSeries::componentComplete()
{
    Q_D(QAbstractSeries);
    d->m_loaded = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QAbstractSeriesPrivate::QAbstractSeriesPrivate()
{
}

QAbstractSeriesPrivate::~QAbstractSeriesPrivate()
{
}

void QAbstractSeriesPrivate::setLegendData(const QList<QLegendData> &legendData)
{
    if (legendData.data() != m_legendData.data()) {
        Q_Q(QAbstractSeries);
        m_legendData = legendData;
        emit q->legendDataChanged();
    }
}

void QAbstractSeriesPrivate::clearLegendData()
{
    if (!m_legendData.empty()) {
        Q_Q(QAbstractSeries);
        m_legendData.clear();
        emit q->legendDataChanged();
    }
}

void QAbstractSeriesPrivate::appendSeriesChildren(QQmlListProperty<QObject> *list, QObject *element)
{
    // Empty implementation; the children are parsed in componentComplete instead
    Q_UNUSED(list);
    Q_UNUSED(element);
}
QT_END_NAMESPACE

#include "moc_qabstractseries.cpp"
#include "moc_qabstractseries_p.cpp"
