// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "graphs2d/qabstractseries.h"
#include "graphs2d/qabstractseries_p.h"
#ifdef USE_AREAGRAPH
#include <QtGraphs/qareaseries.h>
#include <private/arearenderer_p.h>
#endif
#ifdef USE_BARGRAPH
#include <QtGraphs/qbarseries.h>
#include <private/barsrenderer_p.h>
#endif
#ifdef USE_PIEGRAPH
#include <QtGraphs/qpieseries.h>
#include <private/pierenderer_p.h>
#endif
#ifdef USE_LINEGRAPH
#include <QtGraphs/qlineseries.h>
#endif
#ifdef USE_SCATTERGRAPH
#include <QtGraphs/qscatterseries.h>
#endif
#ifdef USE_SPLINEGRAPH
#include <QtGraphs/qsplineseries.h>
#endif
#ifdef USE_POINTS
#include <private/pointrenderer_p.h>
#endif
#include <QTimer>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickpinchhandler_p.h>
#include <private/axisrenderer_p.h>
#include <private/qabstractaxis_p.h>
#include <private/qgraphsview_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype GraphsView
    \nativetype QGraphsView
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \brief Base type for all Qt Graphs views.

This class collects the series and theming together and draws the graphs.
You will need to import Qt Graphs module to use this type:

\snippet doc_src_qmlgraphs.cpp 0

After that you can use GraphsView in your qml files:

\snippet doc_src_qmlgraphs.cpp 10

\image graphsview-minimal.png

\sa BarSeries, LineSeries, BarCategoryAxis, ValueAxis, GraphsTheme
*/

QGraphsView::QGraphsView(QQuickItem *parent) :
    QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    m_defaultTheme = new QGraphsTheme(this);
    m_pinchHandler = new QQuickPinchHandler(this);
    m_pinchHandler->setTarget(nullptr);

    QObject::connect(m_pinchHandler,
                     &QQuickPinchHandler::scaleChanged,
                     this,
                     &QGraphsView::onPinchScaleChanged);
    QObject::connect(m_pinchHandler,
                     &QQuickPinchHandler::grabChanged,
                     this,
                     &QGraphsView::onPinchGrabChanged);
}

QGraphsView::~QGraphsView()
{
    const auto slist = m_seriesList;
    for (const auto &s : slist)
        removeSeries(s);
    if (m_axisX)
        m_axisX->d_func()->setGraph(nullptr);
    if (m_axisY)
        m_axisY->d_func()->setGraph(nullptr);
}

void QGraphsView::onPinchScaleChanged(qreal delta)
{
    if (m_axisRenderer)
        m_axisRenderer->handlePinchScale(delta);
}

void QGraphsView::onPinchGrabChanged(QPointingDevice::GrabTransition transition, QEventPoint point)
{
    if (m_axisRenderer)
        m_axisRenderer->handlePinchGrab(transition, point);
}

/*!
    \qmlmethod GraphsView::addSeries(AbstractSeries series)
    Appends a \a series into GraphsView.
    If the \a series is null, it will not be added. If the \a series already
    belongs to the graph, it will be moved into the end.
*/
/*!
    Appends a \a series into GraphsView.
    If the \a series is null, it will not be added. If the \a series already
    belongs to the graph, it will be moved into the end.
*/
void QGraphsView::addSeries(QObject *series)
{
    insertSeries(m_seriesList.size(), series);
}

/*!
    \qmlmethod GraphsView::insertSeries(int index, AbstractSeries series)
    Inserts a \a series at the position specified by \a index.
    If the \a series is null, it will not be inserted. If the \a series already
    belongs to the graph, it will be moved into \a index.
*/
/*!
    Inserts a \a series at the position specified by \a index.
    If the \a series is null, it will not be inserted. If the \a series already
    belongs to the graph, it will be moved into \a index.
*/
void QGraphsView::insertSeries(qsizetype index, QObject *object)
{
    if (auto series = qobject_cast<QAbstractSeries *>(object)) {
        series->setGraph(this);
        if (m_seriesList.contains(series)) {
            qsizetype oldIndex = m_seriesList.indexOf(series);
            if (index != oldIndex) {
                m_seriesList.removeOne(series);
                if (oldIndex < index)
                    index--;
                m_seriesList.insert(index, series);
            }
        } else {
            m_seriesList.insert(index, series);

            QObject::connect(series, &QAbstractSeries::update,
                             this, &QGraphsView::polishAndUpdate);
            QObject::connect(series, &QAbstractSeries::hoverEnter,
                             this, &QGraphsView::handleHoverEnter);
            QObject::connect(series, &QAbstractSeries::hoverExit,
                             this, &QGraphsView::handleHoverExit);
            QObject::connect(series, &QAbstractSeries::hover,
                             this, &QGraphsView::handleHover);

#ifdef USE_PIEGRAPH
            if (auto pie = qobject_cast<QPieSeries *>(series))
                connect(pie, &QPieSeries::removed, m_pieRenderer, &PieRenderer::markedDeleted);
#endif
        }
        polishAndUpdate();
    }
}

/*!
    \qmlmethod GraphsView::removeSeries(AbstractSeries series)
    Removes the \a series from the graph.
*/
/*!
    Removes the \a series from the graph.
*/
void QGraphsView::removeSeries(QObject *object)
{
    if (auto series = qobject_cast<QAbstractSeries *>(object)) {
        series->setGraph(nullptr);
        m_seriesList.removeAll(series);
        auto &cleanupSeriesList = m_cleanupSeriesList[getSeriesRendererIndex(series)];

#ifdef USE_PIEGRAPH
        if (auto pie = qobject_cast<QPieSeries *>(series))
            disconnect(pie, &QPieSeries::removed, m_pieRenderer, &PieRenderer::markedDeleted);
#endif

        cleanupSeriesList.append(series);
        polishAndUpdate();
    }
}

/*!
    \qmlmethod GraphsView::removeSeries(int index)
    Removes the series specified by \a index from the graph.
*/
/*!
    Removes the series specified by \a index from the graph.
*/
void QGraphsView::removeSeries(qsizetype index)
{
    if (index >= 0 && index < m_seriesList.size())
        removeSeries(m_seriesList[index]);
}

/*!
    \qmlmethod bool GraphsView::hasSeries(AbstractSeries series)
    Returns \c true if the \a series is in the graph.
*/
/*!
    Returns \c true if the \a series is in the graph.
*/
bool QGraphsView::hasSeries(QObject *series)
{
    return m_seriesList.contains(series);
}

void QGraphsView::addAxis(QAbstractAxis *axis)
{
    if (axis) {
        axis->d_func()->setGraph(this);
        // Ensure AxisRenderer exists
        createAxisRenderer();
        polishAndUpdate();
        QObject::connect(axis, &QAbstractAxis::update, this, &QGraphsView::polishAndUpdate);
        QObject::connect(axis,
                         &QAbstractAxis::visibleChanged,
                         this,
                         &QGraphsView::updateComponentSizes);
    }
}

void QGraphsView::removeAxis(QAbstractAxis *axis)
{
    if (m_axisX == axis)
        m_axisX = nullptr;
    if (m_axisY == axis)
        m_axisY = nullptr;
}

qsizetype QGraphsView::graphSeriesCount() const
{
    return m_graphSeriesCount;
}

void QGraphsView::setGraphSeriesCount(qsizetype count)
{
    if (count > m_graphSeriesCount)
        m_graphSeriesCount = count;
}

#ifdef USE_BARGRAPH
void QGraphsView::createBarsRenderer()
{
    if (!m_barsRenderer) {
        m_barsRenderer = new BarsRenderer(this);
        updateComponentSizes();
    }
}
#endif

void QGraphsView::createAxisRenderer()
{
    if (!m_axisRenderer) {
        m_axisRenderer = new AxisRenderer(this);
        m_axisRenderer->setZ(-1);
        updateComponentSizes();
    }
}

#ifdef USE_POINTS
void QGraphsView::createPointRenderer()
{
    if (!m_pointRenderer) {
        m_pointRenderer = new PointRenderer(this);
        updateComponentSizes();
    }
}
#endif

#ifdef USE_PIEGRAPH
void QGraphsView::createPieRenderer()
{
    if (!m_pieRenderer) {
        m_pieRenderer = new PieRenderer(this);
        updateComponentSizes();
    }
}
#endif

#ifdef USE_AREAGRAPH
void QGraphsView::createAreaRenderer()
{
    if (!m_areaRenderer) {
        m_areaRenderer = new AreaRenderer(this);
        updateComponentSizes();
    }
}
#endif

/*!
    \property QGraphsView::axisXSmoothing
    \brief Controls the graph X axis smoothing (antialiasing) amount.
    By default, the smoothing is \c 1.0.
*/
/*!
    \qmlproperty real GraphsView::axisXSmoothing
    Controls the graph X axis smoothing (antialiasing) amount.
    By default, the smoothing is \c 1.0.
*/
qreal QGraphsView::axisXSmoothing() const
{
    return m_axisXSmoothing;
}

void QGraphsView::setAxisXSmoothing(qreal smoothing)
{
    if (qFuzzyCompare(m_axisXSmoothing, smoothing))
        return;
    m_axisXSmoothing = smoothing;
    emit axisXSmoothingChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::axisYSmoothing
    \brief Controls the graph Y axis smoothing (antialiasing) amount.
    By default, the smoothing is \c 1.0.
*/
/*!
    \qmlproperty real GraphsView::axisYSmoothing
    Controls the graph Y axis smoothing (antialiasing) amount.
    By default, the smoothing is \c 1.0.
*/
qreal QGraphsView::axisYSmoothing() const
{
    return m_axisYSmoothing;
}

void QGraphsView::setAxisYSmoothing(qreal smoothing)
{
    if (qFuzzyCompare(m_axisYSmoothing, smoothing))
        return;
    m_axisYSmoothing = smoothing;
    emit axisYSmoothingChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::gridSmoothing
    \brief Controls the graph grid smoothing (antialiasing) amount.
    By default, the smoothing is \c 1.0.
*/
/*!
    \qmlproperty real GraphsView::gridSmoothing
    Controls the graph grid smoothing (antialiasing) amount.
    By default, the smoothing is \c 1.0.
*/
qreal QGraphsView::gridSmoothing() const
{
    return m_gridSmoothing;
}

void QGraphsView::setGridSmoothing(qreal smoothing)
{
    if (qFuzzyCompare(m_gridSmoothing, smoothing))
        return;
    m_gridSmoothing = smoothing;
    emit gridSmoothingChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::shadowVisible
    \brief Controls if the graph grid shadow is visible.
    By default, shadow visibility is set to \c false.
*/
/*!
    \qmlproperty bool GraphsView::shadowVisible
    Controls if the graph grid shadow is visible.
    By default, shadow visibility is set to \c false.
*/
bool QGraphsView::isShadowVisible() const
{
    return m_isShadowVisible;
}

void QGraphsView::setShadowVisible(bool newShadowVisibility)
{
    if (m_isShadowVisible == newShadowVisibility)
        return;
    m_isShadowVisible = newShadowVisibility;
    emit shadowVisibleChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::shadowColor
    \brief Controls the graph grid shadow color.
    By default, shadow color is set to \c black.
*/
/*!
    \qmlproperty color GraphsView::shadowColor
    Controls the graph grid shadow color.
    By default, shadow color is set to \c black.
*/
QColor QGraphsView::shadowColor() const
{
    return m_shadowColor;
}

void QGraphsView::setShadowColor(QColor newShadowColor)
{
    if (m_shadowColor == newShadowColor)
        return;
    m_shadowColor = newShadowColor;
    emit shadowColorChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::shadowBarWidth
    \brief Controls the graph grid shadow width.
    By default, shadow width is set to \c 2.0.
*/
/*!
    \qmlproperty real GraphsView::shadowBarWidth
    Controls the graph grid shadow width.
    By default, shadow width is set to \c 2.0.
*/
qreal QGraphsView::shadowBarWidth() const
{
    return m_shadowBarWidth;
}

void QGraphsView::setShadowBarWidth(qreal newShadowBarWidth)
{
    if (qFuzzyCompare(m_shadowBarWidth, newShadowBarWidth))
        return;
    m_shadowBarWidth = newShadowBarWidth;
    emit shadowBarWidthChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::shadowXOffset
    \brief Controls the graph grid shadow X offset.
    By default, shadow X offset is set to \c 0.0.
*/
/*!
    \qmlproperty real GraphsView::shadowXOffset
    Controls the graph grid shadow X offset.
    By default, shadow X offset is set to \c 0.0.
*/
qreal QGraphsView::shadowXOffset() const
{
    return m_shadowXOffset;
}

void QGraphsView::setShadowXOffset(qreal newShadowXOffset)
{
    if (qFuzzyCompare(m_shadowXOffset, newShadowXOffset))
        return;
    m_shadowXOffset = newShadowXOffset;
    emit shadowXOffsetChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::shadowYOffset
    \brief Controls the graph grid shadow Y offset.
    By default, shadow Y offset is set to \c 0.0.
*/
/*!
    \qmlproperty real GraphsView::shadowYOffset
    Controls the graph grid shadow Y offset.
    By default, shadow Y offset is set to \c 0.0.
*/
qreal QGraphsView::shadowYOffset() const
{
    return m_shadowYOffset;
}

void QGraphsView::setShadowYOffset(qreal newShadowYOffset)
{
    if (qFuzzyCompare(m_shadowYOffset, newShadowYOffset))
        return;
    m_shadowYOffset = newShadowYOffset;
    emit shadowYOffsetChanged();
    polishAndUpdate();
}

/*!
    \property QGraphsView::shadowSmoothing
    \brief Controls the graph grid shadow smoothing (antialiasing) amount.
    By default, shadow smoothing is set to \c 4.0.
*/
/*!
    \qmlproperty real GraphsView::shadowSmoothing
    Controls the graph grid shadow smoothing (antialiasing) amount.
    By default, shadow smoothing is set to \c 4.0.
*/
qreal QGraphsView::shadowSmoothing() const
{
    return m_shadowSmoothing;
}

void QGraphsView::setShadowSmoothing(qreal smoothing)
{
    if (qFuzzyCompare(m_shadowSmoothing, smoothing))
        return;
    m_shadowSmoothing = smoothing;
    emit shadowSmoothingChanged();
    polishAndUpdate();
}

void QGraphsView::handleHoverEnter(const QString &seriesName, QPointF position, QPointF value)
{
    if (m_hoverCount == 0)
        emit hoverEnter(seriesName, position, value);
    m_hoverCount++;
}

void QGraphsView::handleHoverExit(const QString &seriesName, QPointF position)
{
    m_hoverCount--;
    if (m_hoverCount == 0)
        emit hoverExit(seriesName, position);
}

void QGraphsView::handleHover(const QString &seriesName, QPointF position, QPointF value)
{
    emit hover(seriesName, position, value);
}

void QGraphsView::updateComponentSizes()
{
    updateAxisAreas();
    updatePlotArea();

    if (m_axisRenderer)
        m_axisRenderer->setSize(size());

#ifdef USE_BARGRAPH
    if (m_barsRenderer) {
        m_barsRenderer->setX(m_plotArea.x());
        m_barsRenderer->setY(m_plotArea.y());
        m_barsRenderer->setSize(m_plotArea.size());
    }
#endif
#ifdef USE_POINTS
    if (m_pointRenderer) {
        m_pointRenderer->setX(m_plotArea.x());
        m_pointRenderer->setY(m_plotArea.y());
        m_pointRenderer->setSize(m_plotArea.size());
    }
#endif
#ifdef USE_PIEGRAPH
    if (m_pieRenderer) {
        m_pieRenderer->setX(m_plotArea.x());
        m_pieRenderer->setY(m_plotArea.y());
        m_pieRenderer->setSize(m_plotArea.size());
    }
#endif
#ifdef USE_AREAGRAPH
    if (m_areaRenderer) {
        m_areaRenderer->setX(m_plotArea.x());
        m_areaRenderer->setY(m_plotArea.y());
        m_areaRenderer->setSize(m_plotArea.size());
    }
#endif
}

void QGraphsView::componentComplete()
{
    if (!m_zoomAreaDelegate && !m_zoomAreaItem) {
        const QString qmlData = QLatin1StringView(R"QML(
            import QtQuick;
            Rectangle {
                color: "#8888aaff"
                border.width: 1
                border.color: "#4466aa"
            }
        )QML");

        QQmlComponent *tempZoomAreaDelegate = new QQmlComponent(qmlEngine(this), this);
        tempZoomAreaDelegate->setData(qmlData.toUtf8(), QUrl());

        m_zoomAreaItem = qobject_cast<QQuickItem *>(
            tempZoomAreaDelegate->create(tempZoomAreaDelegate->creationContext()));
        m_zoomAreaItem->setParent(this);
        m_zoomAreaItem->setParentItem(this);
        m_zoomAreaItem->setVisible(false);
    }

    if (!m_theme) {
        m_theme = m_defaultTheme;
        QObject::connect(m_theme, &QGraphsTheme::update, this, &QQuickItem::update);
        m_theme->resetColorTheme();
    }
    QQuickItem::componentComplete();

    ensurePolished();
}

void QGraphsView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    updateComponentSizes();

    ensurePolished();
}

void QGraphsView::hoverMoveEvent(QHoverEvent *event)
{
    bool handled = false;

    // Adjust event position to renderers position
    QPointF localPos = event->position() - m_plotArea.topLeft();
    QHoverEvent mappedEvent(event->type(), localPos,event->globalPosition(),
                            event->oldPosF(), event->modifiers());
    mappedEvent.setAccepted(false);

#ifdef USE_BARGRAPH
    if (m_barsRenderer)
        handled |= m_barsRenderer->handleHoverMove(&mappedEvent);
#endif

#ifdef USE_POINTS
    if (m_pointRenderer)
        handled |= m_pointRenderer->handleHoverMove(&mappedEvent);
#endif

#ifdef USE_PIEGRAPH
    if (m_pieRenderer)
        handled |= m_pieRenderer->handleHoverMove(&mappedEvent);
#endif

#ifdef USE_AREAGRAPH
    if (m_areaRenderer)
        handled |= m_areaRenderer->handleHoverMove(&mappedEvent);
#endif

    if (!handled)
        event->ignore();
}

void QGraphsView::wheelEvent(QWheelEvent *event)
{
    bool handled = false;

    // Adjust event position to renderers position
    QPointF localPos = event->position() - m_plotArea.topLeft();
    QWheelEvent mappedEvent(localPos,
                            event->globalPosition(),
                            event->pixelDelta(),
                            event->angleDelta(),
                            event->buttons(),
                            event->modifiers(),
                            event->phase(),
                            event->inverted(),
                            event->source());
    mappedEvent.setAccepted(false);

    if (m_axisRenderer)
        handled |= m_axisRenderer->handleWheel(&mappedEvent);

    if (!handled)
        event->ignore();
    else
        polishAndUpdate();
}

QSGNode *QGraphsView::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);

    for (auto series : std::as_const(m_seriesList)) {
#ifdef USE_BARGRAPH
        if (m_barsRenderer) {
            if (auto barSeries = qobject_cast<QBarSeries *>(series))
                m_barsRenderer->updateSeries(barSeries);
        }
#endif

#ifdef USE_POINTS
        if (m_pointRenderer) {
#ifdef USE_LINEGRAPH
            if (auto lineSeries = qobject_cast<QLineSeries *>(series))
                m_pointRenderer->updateSeries(lineSeries);
#endif
#ifdef USE_SCATTERGRAPH
            if (auto scatterSeries = qobject_cast<QScatterSeries *>(series))
                m_pointRenderer->updateSeries(scatterSeries);
#endif
#ifdef USE_SPLINEGRAPH
            if (auto splineSeries = qobject_cast<QSplineSeries *>(series))
                m_pointRenderer->updateSeries(splineSeries);
#endif
        }
#endif

#ifdef USE_PIEGRAPH
        if (m_pieRenderer) {
            if (auto pieSeries = qobject_cast<QPieSeries *>(series))
                m_pieRenderer->updateSeries(pieSeries);
        }
#endif

#ifdef USE_AREAGRAPH
        if (m_areaRenderer) {
            if (auto areaSeries = qobject_cast<QAreaSeries *>(series))
                m_areaRenderer->updateSeries(areaSeries);
        }
#endif
    }

#ifdef USE_BARGRAPH
    if (m_barsRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[0];
        m_barsRenderer->afterUpdate(cleanupSeriesList);
        cleanupSeriesList.clear();
    }
#endif

#ifdef USE_POINTS
    if (m_pointRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[1];
        m_pointRenderer->afterUpdate(cleanupSeriesList);
        cleanupSeriesList.clear();
    }
#endif

#ifdef USE_AREAGRAPH
    if (m_areaRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[2];
        m_areaRenderer->afterUpdate(cleanupSeriesList);
    }
#endif

#ifdef USE_PIEGRAPH
    if (m_pieRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[3];
        m_pieRenderer->afterUpdate(cleanupSeriesList);
        cleanupSeriesList.clear();
    }
#endif

    // Now possibly dirty theme has been taken into use
    m_theme->resetThemeDirty();

    return oldNode;
}

void QGraphsView::updatePolish()
{
    if (m_axisRenderer) {
        m_axisRenderer->handlePolish();
        // Initialize shaders after system's event queue
        QTimer::singleShot(0, m_axisRenderer, &AxisRenderer::initialize);
    }
    if (m_theme && m_theme->isBackgroundVisible()) {
        if (!m_backgroundRectangle) {
            // Create m_backgroundRectangle only when it is needed
            m_backgroundRectangle = new QQuickRectangle(this);
            m_backgroundRectangle->setZ(-2);
        }
        m_backgroundRectangle->setColor(m_theme->backgroundColor());
        m_backgroundRectangle->setWidth(width());
        m_backgroundRectangle->setHeight(height());
        m_backgroundRectangle->setVisible(true);
    } else if (m_backgroundRectangle) {
        // Hide and delete the m_backgroundRectangle
        m_backgroundRectangle->setVisible(false);
        m_backgroundRectangle->deleteLater();
        m_backgroundRectangle = nullptr;
    }

    // Polish for all series
    for (auto series : std::as_const(m_seriesList)) {
#ifdef USE_BARGRAPH
        if (m_barsRenderer) {
            if (auto barSeries = qobject_cast<QBarSeries*>(series))
                m_barsRenderer->handlePolish(barSeries);
        }
#endif

#ifdef USE_POINTS
        if (m_pointRenderer) {
#ifdef USE_LINEGRAPH
            if (auto lineSeries = qobject_cast<QLineSeries *>(series))
                m_pointRenderer->handlePolish(lineSeries);
#endif

#ifdef USE_SCATTERGRAPH
            if (auto scatterSeries = qobject_cast<QScatterSeries *>(series))
                m_pointRenderer->handlePolish(scatterSeries);
#endif

#ifdef USE_SPLINEGRAPH
            if (auto splineSeries = qobject_cast<QSplineSeries *>(series))
                m_pointRenderer->handlePolish(splineSeries);
#endif
        }
#endif

#ifdef USE_PIEGRAPH
        if (m_pieRenderer) {
            if (auto pieSeries = qobject_cast<QPieSeries *>(series))
                m_pieRenderer->handlePolish(pieSeries);
        }
#endif

#ifdef USE_AREAGRAPH
        if (m_areaRenderer) {
            if (auto areaSeries = qobject_cast<QAreaSeries *>(series))
                m_areaRenderer->handlePolish(areaSeries);
        }
#endif
    }

#ifdef USE_BARGRAPH
    if (m_barsRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[0];
        m_barsRenderer->afterPolish(cleanupSeriesList);
    }
#endif
#ifdef USE_POINTS
    if (m_pointRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[1];
        m_pointRenderer->afterPolish(cleanupSeriesList);
    }
#endif
#ifdef USE_AREAGRAPH
    if (m_areaRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[2];
        m_areaRenderer->afterPolish(cleanupSeriesList);
    }
#endif
#ifdef USE_PIEGRAPH
    if (m_pieRenderer) {
        auto &cleanupSeriesList = m_cleanupSeriesList[3];
        m_pieRenderer->afterPolish(cleanupSeriesList);
    }
#endif
}

void QGraphsView::polishAndUpdate()
{
    polish();
    update();
}

// ***** Static QQmlListProperty methods *****

/*!
    \qmlproperty list GraphsView::seriesList

    List of series that are rendered by the GraphsView. Filled automatically
    with the series type children of the GraphsView.

    This is the default property, so child elements are automatically added
    into the series list.
    \sa BarSeries, LineSeries, ScatterSeries
*/
QQmlListProperty<QObject> QGraphsView::seriesList()
{
    return QQmlListProperty<QObject>(this, this,
                                          &QGraphsView::appendSeriesFunc,
                                          &QGraphsView::countSeriesFunc,
                                          &QGraphsView::atSeriesFunc,
                                          &QGraphsView::clearSeriesFunc);
}

void QGraphsView::appendSeriesFunc(QQmlListProperty<QObject> *list, QObject *series)
{
    reinterpret_cast<QGraphsView *>(list->data)->addSeries(series);
}

qsizetype QGraphsView::countSeriesFunc(QQmlListProperty<QObject> *list)
{
    return reinterpret_cast<QGraphsView *>(list->data)->getSeriesList().size();
}

QObject *QGraphsView::atSeriesFunc(QQmlListProperty<QObject> *list, qsizetype index)
{
    return reinterpret_cast<QGraphsView *>(list->data)->getSeriesList().at(index);
}

void QGraphsView::clearSeriesFunc(QQmlListProperty<QObject> *list)
{
    QGraphsView *declItems = reinterpret_cast<QGraphsView *>(list->data);
    QList<QObject *> realList = declItems->getSeriesList();
    qsizetype count = realList.size();
    for (int i = 0; i < count; i++)
        declItems->removeSeries(realList.at(i));
}

/*!
    \qmlproperty GraphsTheme GraphsView::theme
    The theme used by the graph. Determines coloring,
    axis lines, fonts etc. If theme has not been set,
    the default theme is used.
*/
QGraphsTheme *QGraphsView::theme() const
{
    return m_theme;
}

void QGraphsView::setTheme(QGraphsTheme *newTheme)
{
    if (m_theme == newTheme)
        return;

    if (m_theme)
        QObject::disconnect(m_theme, nullptr, this, nullptr);

    m_theme = newTheme;

    if (!m_theme) {
        m_theme = m_defaultTheme;
        m_theme->resetColorTheme();
    }

    QObject::connect(m_theme, &QGraphsTheme::update, this, &QGraphsView::polishAndUpdate);
    emit themeChanged();
    polishAndUpdate();
}

/*!
    \qmlproperty real GraphsView::marginTop
    The amount of empty space on the top of the graph.
    By default, the margin is 20.
*/
qreal QGraphsView::marginTop() const
{
    return m_marginTop;
}

void QGraphsView::setMarginTop(qreal newMarginTop)
{
    if (qFuzzyCompare(m_marginTop, newMarginTop))
        return;
    m_marginTop = newMarginTop;
    updateComponentSizes();
    polishAndUpdate();
    emit marginTopChanged();
}

/*!
    \qmlproperty real GraphsView::marginBottom
    The amount of empty space on the bottom of the graph.
    By default, the margin is 20.
*/
qreal QGraphsView::marginBottom() const
{
    return m_marginBottom;
}

void QGraphsView::setMarginBottom(qreal newMarginBottom)
{
    if (qFuzzyCompare(m_marginBottom, newMarginBottom))
        return;
    m_marginBottom = newMarginBottom;
    updateComponentSizes();
    polishAndUpdate();
    emit marginBottomChanged();
}

/*!
    \qmlproperty real GraphsView::marginLeft
    The amount of empty space on the left of the graph.
    By default, the margin is 20.
*/
qreal QGraphsView::marginLeft() const
{
    return m_marginLeft;
}

void QGraphsView::setMarginLeft(qreal newMarginLeft)
{
    if (qFuzzyCompare(m_marginLeft, newMarginLeft))
        return;
    m_marginLeft = newMarginLeft;
    updateComponentSizes();
    polishAndUpdate();
    emit marginLeftChanged();
}

/*!
    \qmlproperty real GraphsView::marginRight
    The amount of empty space on the right of the graph.
    By default, the margin is 20.
*/
qreal QGraphsView::marginRight() const
{
    return m_marginRight;
}

void QGraphsView::setMarginRight(qreal newMarginRight)
{
    if (qFuzzyCompare(m_marginRight, newMarginRight))
        return;
    m_marginRight = newMarginRight;
    updateComponentSizes();
    polishAndUpdate();
    emit marginRightChanged();
}

/*!
    \property QGraphsView::plotArea
    \since 6.9
    \brief The rectangle within which the graph is drawn.

    This is the QGraphsView area minus axis areas and margins.
    \sa marginTop, marginBottom, marginLeft, marginRight
*/
/*!
    \qmlproperty rect GraphsView::plotArea
    \since 6.9
    The rectangle within which the graph is drawn.
    This is the GraphsView area minus axis areas and margins.
    \sa marginTop, marginBottom, marginLeft, marginRight
*/
QRectF QGraphsView::plotArea() const
{
    return m_plotArea;
}

void QGraphsView::updateAxisAreas()
{
    if (m_axisX && !m_axisX->isVisible()) {
        m_axisXLabelsMargin = 0;
        m_axisTickersHeight = 0;
        m_axisLabelsHeight = 0;
    } else {
        m_axisTickersHeight = m_defaultAxisTickersHeight;
        m_axisLabelsHeight = m_defaultAxisLabelsHeight;
        m_axisXLabelsMargin = m_defaultAxisXLabelsMargin;
    }

    if (m_axisY && !m_axisY->isVisible()) {
        m_axisTickersWidth = 0;
        m_axisLabelsWidth = 0;
        m_axisYLabelsMargin = 0;
    } else {
        m_axisLabelsWidth = m_defaultAxisLabelsWidth;
        m_axisTickersWidth = m_defaultAxisTickersWidth;
        m_axisYLabelsMargin = m_defaultAxisYLabelsMargin;
    }

    QRectF r = { m_marginLeft,
                 m_marginTop,
                 width() - m_marginLeft - m_marginRight,
                 height() - m_marginTop - m_marginBottom };
    m_axisHeight = m_axisLabelsHeight + m_axisXLabelsMargin + m_axisTickersHeight;
    m_axisWidth = m_axisLabelsWidth + m_axisYLabelsMargin + m_axisTickersWidth;
    float leftPadding = (m_axisY && m_axisY->alignment() == Qt::AlignLeft) ? m_axisWidth : 0;
    float topPadding = (m_axisX && m_axisX->alignment() == Qt::AlignTop) ? m_axisHeight : 0;
    if (m_axisX && m_axisX->alignment() == Qt::AlignTop) {
        m_xAxisArea = { r.x() + leftPadding, r.y(), r.width() - m_axisWidth, m_axisHeight};
        m_xAxisLabelsArea = { m_xAxisArea.x(),
                              m_xAxisArea.y(),
                              m_xAxisArea.width(),
                              m_axisLabelsHeight };
        m_xAxisTickersArea = { m_xAxisArea.x(),
                               m_xAxisArea.y() + m_axisLabelsHeight + m_axisXLabelsMargin,
                               m_xAxisArea.width(),
                               m_axisTickersHeight };
    } else {
        m_xAxisArea = { r.x() + leftPadding, r.y() + r.height() - m_axisHeight,
                        r.width() - m_axisWidth, m_axisHeight };
        m_xAxisLabelsArea = { m_xAxisArea.x(),
                              m_xAxisArea.y() + m_axisTickersHeight + m_axisXLabelsMargin,
                              m_xAxisArea.width(),
                              m_axisTickersHeight };
        m_xAxisTickersArea = { m_xAxisArea.x(),
                               m_xAxisArea.y(),
                               m_xAxisArea.width(),
                               m_axisTickersHeight };
    }
    if (m_axisY && m_axisY->alignment() == Qt::AlignLeft) {
        m_yAxisArea = { r.x(), r.y() + topPadding, m_axisWidth, r.height() - m_axisHeight };
        m_yAxisLabelsArea = { m_yAxisArea.x(),
                              m_yAxisArea.y(),
                              m_axisLabelsWidth,
                              m_yAxisArea.height() };
        m_yAxisTickersArea = { m_yAxisArea.x() + m_axisLabelsWidth + m_axisYLabelsMargin,
                               m_yAxisArea.y(),
                               m_axisTickersWidth,
                               m_yAxisArea.height() };
    } else {
        m_yAxisArea = { r.x() + r.width() - m_axisWidth, r.y() + topPadding,
                        m_axisWidth, r.height() - m_axisHeight };
        m_yAxisLabelsArea = { m_yAxisArea.x() + m_axisTickersWidth + m_axisYLabelsMargin,
                              m_yAxisArea.y(),
                              m_axisLabelsWidth,
                              m_yAxisArea.height() };
        m_yAxisTickersArea = { m_yAxisArea.x(),
                               m_yAxisArea.y(),
                               m_axisTickersWidth,
                               m_yAxisArea.height() };
    }
}

void QGraphsView::updatePlotArea()
{
    // When axis are in left & bottom
    qreal x = m_marginLeft;
    qreal y = m_marginTop;
    qreal w = width() - x - m_marginRight;
    qreal h = height() - y - m_marginBottom;
    if (m_axisX && m_axisX->alignment() == Qt::AlignTop)
        y += m_axisHeight;
    if (m_axisY && m_axisY->alignment() != Qt::AlignRight)
        x += m_axisWidth;
    if (m_axisX)
        h -= m_axisHeight;
    if (m_axisY)
        w -= m_axisWidth;
    w = qMax(w, 0.0);
    h = qMax(h, 0.0);
    QRectF plotArea = QRectF(x, y, w, h);
    if (plotArea != m_plotArea) {
        m_plotArea = plotArea;
        emit plotAreaChanged();
    }
}

/*!
    \property QGraphsView::axisX
    \brief X-axis of this view.

    The x-axis used for the series inside this view.

    \note Setting the same axis to multiple QGraphsViews is not supported.
*/
/*!
    \qmlproperty AbstractAxis GraphsView::axisX
    The x-axis used for the series inside this view.
    \sa axisY

    \note Setting the same axis to multiple GraphsViews is not supported.
*/

QAbstractAxis *QGraphsView::axisX() const
{
    return m_axisX;
}

void QGraphsView::setAxisX(QAbstractAxis *axis)
{
    if (m_axisX == axis)
        return;
    removeAxis(m_axisX);
    m_axisX = axis;
    if (axis) {
        if (axis->alignment() != Qt::AlignBottom && axis->alignment() != Qt::AlignTop)
            axis->setAlignment(Qt::AlignBottom);
        addAxis(axis);
    }
    emit axisXChanged();
    emit update();
}

/*!
    \property QGraphsView::axisY
    \brief Y-axis of this view.

    The y-axis used for the series inside this view.

    \note Setting the same axis to multiple QGraphsViews is not supported.
*/
/*!
    \qmlproperty AbstractAxis GraphsView::axisY
    The y-axis used for the series inside this view.
    \sa axisX

    \note Setting the same axis to multiple GraphsViews is not supported.
*/

QAbstractAxis *QGraphsView::axisY() const
{
    return m_axisY;
}

void QGraphsView::setAxisY(QAbstractAxis *axis)
{
    if (m_axisY == axis)
        return;
    removeAxis(m_axisY);
    m_axisY = axis;
    if (axis) {
        if (axis->alignment() != Qt::AlignLeft && axis->alignment() != Qt::AlignRight)
            axis->setAlignment(Qt::AlignLeft);
        addAxis(axis);
    }
    emit axisYChanged();
    emit update();
}

/*!
    \property QGraphsView::orientation
    \brief Orientation of the GraphsView.

    Determines the orientation of the QGraphsView. When the orientation is
    \l {Qt::Horizontal}{Qt::Horizontal}, \l axisX and \l axisY will switch the
    positions so that \l axisX is rendered vertically and \l axisY horizontally.
    This property is currently used by the \l QBarSeries.
    The default value is \l {Qt::Vertical}{Qt::Vertical}.
*/
/*!
    \qmlproperty Qt.Orientation GraphsView::orientation
    Determines the orientation of the GraphsView. When the orientation is
    \l {Qt::Horizontal}{Qt.Horizontal}, \l axisX and \l axisY will switch the
    positions so that \l axisX is rendered vertically and \l axisY horizontally.
    This property is currently used by the \l BarSeries.
    The default value is \l {Qt::Vertical}{Qt.Vertical}.
*/
Qt::Orientation QGraphsView::orientation() const
{
    return m_orientation;
}

void QGraphsView::setOrientation(Qt::Orientation newOrientation)
{
    if (m_orientation == newOrientation)
        return;
    m_orientation = newOrientation;
    emit orientationChanged();
    emit update();
}

/*!
    \enum QGraphsView::ZoomStyle
    This enum value describes the zoom style of the graph:

    \value None
        Zooming is disabled.
    \value Center
        Pinch zoom and mouse wheel zoom towards the center of the graph view.
*/

/*!
    \property QGraphsView::zoomStyle
    \brief Zoom style of the GraphsView.

    Determines the zoom style of the QGraphsView. Zooming works by
    manipulating the QValueAxis zoom property. The default value
    is \c {QGraphsView::ZoomStyle::None}.
*/
/*!
    \qmlproperty enumeration GraphsView::zoomStyle
    Determines the zoom style of the GraphsView. Zooming works by
    manipulating the ValueAxis zoom property. The default value
    is \c {GraphsView.ZoomStyle.None}.

    \value GraphsView.ZoomStyle.None
        Zooming is disabled.
    \value GraphsView.ZoomStyle.Center
        Pinch zoom and mouse wheel zoom towards the center of the graph view.

*/
QGraphsView::ZoomStyle QGraphsView::zoomStyle() const
{
    return m_zoomStyle;
}

void QGraphsView::setZoomStyle(ZoomStyle newZoomStyle)
{
    if (m_zoomStyle == newZoomStyle)
        return;
    m_zoomStyle = newZoomStyle;
    emit zoomStyleChanged();
}

/*!
    \enum QGraphsView::PanStyle
    This enum value describes the pan style of the graph:

    \value None
        Panning is disabled.
    \value Drag
        Mouse and touch drag pan the view around.
*/

/*!
    \property QGraphsView::panStyle
    \brief Pan style of the GraphsView.

    Determines the pan style of the QGraphsView. Panning works by
    manipulating the pan property of a QValueAxis.
    The default value is \c {QGraphsView::PanStyle::None}.
*/
/*!
    \qmlproperty enumeration GraphsView::panStyle
    Determines the pan style of the GraphsView. Panning works by
    manipulating the pan property of a ValueAxis.
    The default value is \c {GraphsView.PanStyle.None}.

    \value GraphsView.PanStyle.None
        Panning is disabled.
    \value GraphsView.PanStyle.Drag
        Mouse and touch drag pan the view around.
*/
QGraphsView::PanStyle QGraphsView::panStyle() const
{
    return m_panStyle;
}

void QGraphsView::setPanStyle(PanStyle newPanStyle)
{
    if (m_panStyle == newPanStyle)
        return;
    m_panStyle = newPanStyle;
    emit panStyleChanged();
}

/*!
    \property QGraphsView::zoomAreaEnabled
    \brief Enables zoom area

    Zoom area changes mouse and touch dragging to draw a box determined
    by \c zoomAreaDelegate. Upon release the graph QValueAxis zoom and pan
    properties are changed so that the view covers only the area intersected
    by the drawn box.
    \sa zoomAreaDelegate
*/
/*!
    \qmlproperty bool GraphsView::zoomAreaEnabled
    Zoom area changes mouse and touch dragging to draw a box determined
    by \c zoomAreaDelegate. Upon release the graph ValueAxis zoom and pan
    properties are changed so that the view covers only the area intersected
    by the drawn box.
    \sa zoomAreaDelegate
*/
bool QGraphsView::zoomAreaEnabled() const
{
    return m_zoomAreaEnabled;
}

void QGraphsView::setZoomAreaEnabled(bool newZoomAreaEnabled)
{
    if (m_zoomAreaEnabled == newZoomAreaEnabled)
        return;
    m_zoomAreaEnabled = newZoomAreaEnabled;
    emit zoomAreaEnabledChanged();
}

/*!
    \property QGraphsView::zoomAreaDelegate
    \brief Zoom area visual delegate

    Determines the QML element that is drawn when the user performs a drag
    motion to zoom in to an area.
*/
/*!
    \qmlproperty Component GraphsView::zoomAreaDelegate
    Determines the QML element that is drawn when the user performs a drag
    motion to zoom in to an area.
*/
QQmlComponent *QGraphsView::zoomAreaDelegate() const
{
    return m_zoomAreaDelegate;
}

void QGraphsView::setZoomAreaDelegate(QQmlComponent *newZoomAreaDelegate)
{
    if (m_zoomAreaDelegate == newZoomAreaDelegate)
        return;
    m_zoomAreaDelegate = newZoomAreaDelegate;

    if (m_zoomAreaDelegate) {
        m_zoomAreaItem = qobject_cast<QQuickItem *>(
            m_zoomAreaDelegate->create(m_zoomAreaDelegate->creationContext()));
        m_zoomAreaItem->setParent(this);
        m_zoomAreaItem->setParentItem(this);
        m_zoomAreaItem->setVisible(false);
    }

    emit zoomAreaDelegateChanged();
}

/*!
    \property QGraphsView::zoomSensitivity
    \brief Zoom value change sensitivity

    Determines how fast zoom value changes while zooming.
*/
/*!
    \qmlproperty real GraphsView::zoomSensitivity
    Determines how fast zoom value changes while zooming.
*/
qreal QGraphsView::zoomSensitivity() const
{
    return m_zoomSensitivity;
}

void QGraphsView::setZoomSensitivity(qreal newZoomSensitivity)
{
    if (qFuzzyCompare(m_zoomSensitivity, newZoomSensitivity))
        return;
    m_zoomSensitivity = newZoomSensitivity;
    emit zoomSensitivityChanged();
}

int QGraphsView::getSeriesRendererIndex(QAbstractSeries *series)
{
    int index = 0;
    if (series) {
        switch (QAbstractSeriesPrivate::get(series)->type()) {
        case QAbstractSeries::SeriesType::Bar:
            index = 0;
            break;
        case QAbstractSeries::SeriesType::Scatter:
        case QAbstractSeries::SeriesType::Line:
        case QAbstractSeries::SeriesType::Spline:
            index = 1;
            break;
        case QAbstractSeries::SeriesType::Area:
            index = 2;
            break;
        case QAbstractSeries::SeriesType::Pie:
            index = 3;
            break;
        }
    }
    return index;
}

QT_END_NAMESPACE
