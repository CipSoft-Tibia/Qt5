// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "q3dsurface.h"
#include "qquickgraphssurface_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class Q3DSurface
 * \inmodule QtGraphs
 * \brief The Q3DSurface class provides methods for rendering 3D surface plots.
 *
 * This class enables developers to render 3D surface plots and to view them by rotating the scene
 * freely. The visual properties of the surface such as draw mode and shading can be controlled
 * via QSurface3DSeries.
 *
 * The Q3DSurface supports selection by showing a highlighted ball on the data point where the user has clicked
 * with left mouse button (when default input handler is in use) or selected via QSurface3DSeries.
 * The selection pointer is accompanied with a label which in default case shows the value of the
 * data point and the coordinates of the point.
 *
 * The value range and the label format shown on the axis can be controlled through QValue3DAxis.
 *
 * To rotate the graph, hold down the right mouse button and move the mouse. Zooming is done using mouse
 * wheel. Both assume the default input handler is in use.
 *
 * If no axes are set explicitly to Q3DSurface, temporary default axes with no labels are created.
 * These default axes can be modified via axis accessors, but as soon any axis is set explicitly
 * for the orientation, the default axis for that orientation is destroyed.
 *
 * \section1 How to construct a minimal Q3DSurface graph
 *
 * First, construct Q3DSurface. Since we are running the graph as top level window
 * in this example, we need to clear the \c Qt::FramelessWindowHint flag, which gets set by
 * default:
 *
 * \snippet doc_src_q3dsurface_construction.cpp 0
 *
 * Now Q3DSurface is ready to receive data to be rendered. Create data elements to receive values:
 *
 * \snippet doc_src_q3dsurface_construction.cpp 1
 *
 * First feed the data to the row elements and then add their pointers to the data element:
 *
 * \snippet doc_src_q3dsurface_construction.cpp 2
 *
 * Create a new series and set data to it:
 *
 * \snippet doc_src_q3dsurface_construction.cpp 3
 *
 * Finally you will need to set it visible:
 *
 * \snippet doc_src_q3dsurface_construction.cpp 4
 *
 * The complete code needed to create and display this graph is:
 *
 * \snippet doc_src_q3dsurface_construction.cpp 5
 *
 * And this is what those few lines of code produce:
 *
 * \image q3dsurface-minimal.png
 *
 * The scene can be rotated, zoomed into, and a surface point can be selected to view its position,
 * but no other interaction is included in this minimal code example.
 * You can learn more by familiarizing yourself with the examples provided,
 * like the \l{Surface Graph Gallery}.
 *
 *
 * \sa Q3DBars, Q3DScatter, {Qt Graphs C++ Classes}
 */

/*!
 * Constructs a new 3D surface graph.
 */
Q3DSurface::Q3DSurface() : QAbstract3DGraph()
{
    QQmlComponent *component = new QQmlComponent(engine(), this);
    component->setData("import QtQuick; import QtGraphs; Surface3D { anchors.fill: parent; }",
                       QUrl());
    m_graphsItem.reset(qobject_cast<QQuickGraphsSurface *>(component->create()));
    setContent(component->url(), component, m_graphsItem.data());

    QObject::connect(m_graphsItem.data(), &QQuickGraphsItem::selectedElementChanged,
                     this, &QAbstract3DGraph::selectedElementChanged);
}

/*!
 *  Destroys the 3D surface graph.
 */
Q3DSurface::~Q3DSurface()
{
}

/*!
 * Adds the \a series to the graph.  A graph can contain multiple series, but has only one set of
 * axes. If the newly added series has specified a selected item, it will be highlighted and
 * any existing selection will be cleared. Only one added series can have an active selection.
 *
 * \sa  QAbstract3DGraph::hasSeries()
 */
void Q3DSurface::addSeries(QSurface3DSeries *series)
{
    dptr()->addSeries(series);
}

/*!
 * Removes the \a series from the graph.
 *
 * \sa QAbstract3DGraph::hasSeries()
 */
void Q3DSurface::removeSeries(QSurface3DSeries *series)
{
    dptr()->removeSeries(series);
}

/*!
 * Returns the list of series added to this graph.
 *
 * \sa QAbstract3DGraph::hasSeries()
 */
QList<QSurface3DSeries *> Q3DSurface::seriesList() const
{
    return dptrc()->m_surfaceController->surfaceSeriesList();
}

/*!
 * \property Q3DSurface::axisX
 *
 * \brief The active x-axis.
 *
 * Sets \a axis as the active x-axis. Implicitly calls addAxis() to transfer the
 * ownership of the axis to this graph.
 *
 * If \a axis is null, a temporary default axis with no labels and an
 * automatically adjusting range is created.
 *
 * This temporary axis is destroyed if another axis is set explicitly to the
 * same orientation.
 *
 * \sa addAxis(), releaseAxis()
 */
void Q3DSurface::setAxisX(QValue3DAxis *axis)
{
    dptr()->setAxisX(axis);
}

QValue3DAxis *Q3DSurface::axisX() const
{
    return static_cast<QValue3DAxis *>(dptrc()->axisX());
}

/*!
 * \property Q3DSurface::axisY
 *
 * \brief The active y-axis.
 *
 * Sets \a axis as the active y-axis. Implicitly calls addAxis() to transfer the
 * ownership of the axis to this graph.
 *
 * If \a axis is null, a temporary default axis with no labels and an
 * automatically adjusting range is created.
 *
 * This temporary axis is destroyed if another axis is set explicitly to the
 * same orientation.
 *
 * \sa addAxis(), releaseAxis()
 */
void Q3DSurface::setAxisY(QValue3DAxis *axis)
{
    dptr()->setAxisY(axis);
}

QValue3DAxis *Q3DSurface::axisY() const
{
    return static_cast<QValue3DAxis *>(dptrc()->axisY());
}

/*!
 * \property Q3DSurface::axisZ
 *
 * \brief The active z-axis.
 *
 * Sets \a axis as the active z-axis. Implicitly calls addAxis() to transfer the
 * ownership of the axis to this graph.
 *
 * If \a axis is null, a temporary default axis with no labels and an
 * automatically adjusting range is created.
 *
 * This temporary axis is destroyed if another axis is set explicitly to the
 * same orientation.
 *
 * \sa addAxis(), releaseAxis()
 */
void Q3DSurface::setAxisZ(QValue3DAxis *axis)
{
    dptr()->setAxisZ(axis);
}

QValue3DAxis *Q3DSurface::axisZ() const
{
    return static_cast<QValue3DAxis *>(dptrc()->axisZ());
}

/*!
 * \property Q3DSurface::selectedSeries
 *
 * \brief The selected series or null.
 *
 * If selectionMode has \c SelectionMultiSeries set, this
 * property holds the series which owns the selected point.
 */
QSurface3DSeries *Q3DSurface::selectedSeries() const
{
    return dptrc()->selectedSeries();
}

/*!
 * \property Q3DSurface::flipHorizontalGrid
 *
 * \brief Whether the horizontal axis grid is displayed on top of the graph
 * rather than on the bottom.
 *
 * In some use cases the horizontal axis grid is mostly covered by the surface, so it can be more
 * useful to display the horizontal axis grid on top of the graph rather than on the bottom.
 * A typical use case for this is showing 2D spectrograms using orthoGraphic projection with
 * a top-down viewpoint.
 *
 * If \c{false}, the horizontal axis grid and labels are drawn on the horizontal background
 * of the graph.
 * If \c{true}, the horizontal axis grid and labels are drawn on the opposite side of the graph
 * from the horizontal background.
 * Defaults to \c{false}.
 */
void Q3DSurface::setFlipHorizontalGrid(bool flip)
{
    dptr()->setFlipHorizontalGrid(flip);
}

bool Q3DSurface::flipHorizontalGrid() const
{
    return dptrc()->flipHorizontalGrid();
}

/*!
 * Adds \a axis to the graph. The axes added via addAxis are not yet taken to use,
 * addAxis is simply used to give the ownership of the \a axis to the graph.
 * The \a axis must not be null or added to another graph.
 *
 * \sa releaseAxis(), setAxisX(), setAxisY(), setAxisZ()
 */
void Q3DSurface::addAxis(QValue3DAxis *axis)
{
    return dptrc()->m_surfaceController->addAxis(axis);
}

/*!
 * Releases the ownership of the \a axis back to the caller, if it is added to this graph.
 * If the released \a axis is in use, a new default axis will be created and set active.
 *
 * If the default axis is released and added back later, it behaves as any other axis would.
 *
 * \sa addAxis(), setAxisX(), setAxisY(), setAxisZ()
 */
void Q3DSurface::releaseAxis(QValue3DAxis *axis)
{
    return dptrc()->m_surfaceController->releaseAxis(axis);
}

/*!
 * Returns the list of all added axes.
 *
 * \sa addAxis()
 */
QList<QValue3DAxis *> Q3DSurface::axes() const
{
    QList<QAbstract3DAxis *> abstractAxes = dptrc()->m_surfaceController->axes();
    QList<QValue3DAxis *> retList;
    for (QAbstract3DAxis *axis : abstractAxes)
        retList.append(static_cast<QValue3DAxis *>(axis));

    return retList;
}

/*!
 * \internal
 */
QQuickGraphsSurface *Q3DSurface::dptr()
{
    return static_cast<QQuickGraphsSurface *>(m_graphsItem.data());
}

/*!
 * \internal
 */
const QQuickGraphsSurface *Q3DSurface::dptrc() const
{
    return static_cast<const QQuickGraphsSurface *>(m_graphsItem.data());
}

QT_END_NAMESPACE
