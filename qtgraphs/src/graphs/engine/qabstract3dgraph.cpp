// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qabstract3dgraph.h"
#include "q3dscene_p.h"
#include "qquickgraphsitem_p.h"
#ifdef Q_OS_DARWIN
#include <QtQuick3D/qquick3d.h>
#endif

QT_BEGIN_NAMESPACE

/*!
 * \class QAbstract3DGraph
 * \inmodule QtGraphs
 * \brief The QAbstract3DGraph class provides a window and render loop for graphs.
 *
 * This class subclasses a QWindow and provides render loop for graphs inheriting it.
 *
 * You should not need to use this class directly, but one of its subclasses instead.
 *
 * Anti-aliasing is turned on by default on C++, except in OpenGL ES2
 * environments, where anti-aliasing is not supported by Qt Graphs.
 * To specify non-default anti-aliasing for a graph, give a custom surface format as
 * a constructor parameter. You can use the convenience function \c qDefaultSurfaceFormat()
 * to create the surface format object.
 *
 * \note QAbstract3DGraph sets window flag \c Qt::FramelessWindowHint on by default. If you want to display
 * graph windows as standalone windows with regular window frame, clear this flag after constructing
 * the graph. For example:
 *
 * \code
 *  Q3DBars *graphWindow = new Q3DBars;
 *  graphWindow->setFlags(graphWindow->flags() ^ Qt::FramelessWindowHint);
 * \endcode
 *
 * \sa Q3DBars, Q3DScatter, Q3DSurface, {Qt Graphs C++ Classes}
 */

/*!
    \enum QAbstract3DGraph::SelectionFlag

    Item selection modes. Values of this enumeration can be combined with OR operator.

    \value SelectionNone
           Selection mode disabled.
    \value SelectionItem
           Selection highlights a single item.
    \value SelectionRow
           Selection highlights a single row.
    \value SelectionItemAndRow
           Combination flag for highlighting both item and row with different colors.
    \value SelectionColumn
           Selection highlights a single column.
    \value SelectionItemAndColumn
           Combination flag for highlighting both item and column with different colors.
    \value SelectionRowAndColumn
           Combination flag for highlighting both row and column.
    \value SelectionItemRowAndColumn
           Combination flag for highlighting item, row, and column.
    \value SelectionSlice
           Setting this mode flag indicates that the graph should take care of the slice view handling
           automatically. If you wish to control the slice view yourself via Q3DScene, do not set this
           flag. When setting this mode flag, either \c SelectionRow or \c SelectionColumn must also
           be set, but not both. Slicing is supported by Q3DBars and Q3DSurface only.
           When this flag is set, slice mode is entered in the following situations:
           \list
           \li When selection is changed explicitly via series API to a visible item
           \li When selection is changed by clicking on the graph
           \li When the selection mode changes and the selected item is visible
           \endlist
    \value SelectionMultiSeries
           Setting this mode means that items for all series at same position are highlighted, instead
           of just the selected item. The actual selection in the other series doesn't change.
           When setting this mode flag, one or more of the basic selection flags (\c {SelectionItem},
           \c {SelectionRow}, or \c SelectionColumn) must also be set.
           Multi-series selection is not supported for Q3DScatter.
*/

/*!
    \enum QAbstract3DGraph::ShadowQuality

    Quality of shadows.

    \value ShadowQualityNone
           Shadows are disabled.
    \value ShadowQualityLow
           Shadows are rendered in low quality.
    \value ShadowQualityMedium
           Shadows are rendered in medium quality.
    \value ShadowQualityHigh
           Shadows are rendered in high quality.
    \value ShadowQualitySoftLow
           Shadows are rendered in low quality with softened edges.
    \value ShadowQualitySoftMedium
           Shadows are rendered in medium quality with softened edges.
    \value ShadowQualitySoftHigh
           Shadows are rendered in high quality with softened edges.
*/

/*!
    \enum QAbstract3DGraph::ElementType

    Type of an element in the graph.

    \value ElementNone
           No defined element.
    \value ElementSeries
           An item in a series.
    \value ElementAxisXLabel
           The x-axis label.
    \value ElementAxisYLabel
           The y-axis label.
    \value ElementAxisZLabel
           The z-axis label.
    \value ElementCustomItem
           A custom item.
*/

/*!
    \enum QAbstract3DGraph::OptimizationHint

    The optimization hint for rendering.

    \value OptimizationDefault
           Provides the full feature set with instancing at a good performance.
    \value OptimizationStatic
           Optimizes the rendering of static data sets at the expense of some features.
           Usable only with Q3DScatter graphs.
    \value OptimizationLegacy
           Provides the full feature set at a reasonable performance. To be used if
           OptimizationDefault performs poorly or does not work.
*/

/*!
 * \internal
 */
QAbstract3DGraph::QAbstract3DGraph()
{
#ifdef Q_OS_DARWIN
    // Take care of widget users (or CI) wanting to use OpenGL backend on macOS
    if (QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGL)
        QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat(4));
#endif
}

/*!
 * Destroys QAbstract3DGraph.
 */
QAbstract3DGraph::~QAbstract3DGraph()
{
}

/*!
 * Adds the given \a inputHandler to the graph. The input handlers added via addInputHandler
 * are not taken in to use directly. Only the ownership of the \a inputHandler is given to the graph.
 * The \a inputHandler must not be null or already added to another graph.
 *
 * \sa releaseInputHandler(), setActiveInputHandler()
 */
void QAbstract3DGraph::addInputHandler(QAbstract3DInputHandler *inputHandler)
{
    m_graphsItem->addInputHandler(inputHandler);
}

/*!
 * Releases the ownership of the \a inputHandler back to the caller, if it was added to this graph.
 * If the released \a inputHandler is in use there will be no input handler active after this call.
 *
 * If the default input handler is released and added back later, it behaves as any other input handler would.
 *
 * \sa addInputHandler(), setActiveInputHandler()
 */
void QAbstract3DGraph::releaseInputHandler(QAbstract3DInputHandler *inputHandler)
{
    m_graphsItem->releaseInputHandler(inputHandler);
}

/*!
 * \property QAbstract3DGraph::activeInputHandler
 *
 * \brief The active input handler used in the graph.
 */

/*!
 * Sets \a inputHandler as the active input handler used in the graph.
 * Implicitly calls addInputHandler() to transfer ownership of \a inputHandler
 * to this graph.
 *
 * If \a inputHandler is null, no input handler will be active after this call.
 *
 * \sa addInputHandler(), releaseInputHandler()
 */
void QAbstract3DGraph::setActiveInputHandler(QAbstract3DInputHandler *inputHandler)
{
    m_graphsItem->setActiveInputHandler(inputHandler);
}

QAbstract3DInputHandler *QAbstract3DGraph::activeInputHandler() const
{
    return m_graphsItem->activeInputHandler();
}

/*!
 * Returns the list of all added input handlers.
 *
 * \sa addInputHandler()
 */
QList<QAbstract3DInputHandler *> QAbstract3DGraph::inputHandlers() const
{
    return m_graphsItem->inputHandlers();
}

/*!
 * Adds the given \a theme to the graph. The themes added via addTheme are not taken in to use
 * directly. Only the ownership of the theme is given to the graph.
 * The \a theme must not be null or already added to another graph.
 *
 * \sa releaseTheme(), setActiveTheme()
 */
void QAbstract3DGraph::addTheme(Q3DTheme *theme)
{
    m_graphsItem->addTheme(theme);
}

/*!
 * Releases the ownership of the \a theme back to the caller, if it was added to this graph.
 * If the released \a theme is in use, a new default theme will be created and set active.
 *
 * If the default theme is released and added back later, it behaves as any other theme would.
 *
 * \sa addTheme(), setActiveTheme()
 */
void QAbstract3DGraph::releaseTheme(Q3DTheme *theme)
{
    m_graphsItem->releaseTheme(theme);
}

/*!
 * \property QAbstract3DGraph::activeTheme
 *
 * \brief The active theme of the graph.
 *
 * Sets \a activeTheme as the active theme to be used for the graph. Implicitly calls
 * addTheme() to transfer the ownership of the theme to this graph.
 *
 * If \a activeTheme is null, a temporary default theme is created. This temporary theme is destroyed
 * if any theme is explicitly set later.
 * Properties of the theme can be modified even after setting it, and the modifications take
 * effect immediately.
 */
Q3DTheme *QAbstract3DGraph::activeTheme() const
{
    return m_graphsItem->theme();
}

void QAbstract3DGraph::setActiveTheme(Q3DTheme *activeTheme)
{
    m_graphsItem->setTheme(activeTheme);
    emit activeThemeChanged(activeTheme);
}

/*!
 * Returns the list of all added themes.
 *
 * \sa addTheme()
 */
QList<Q3DTheme *> QAbstract3DGraph::themes() const
{
    return m_graphsItem->themes();
}

/*!
 * \property QAbstract3DGraph::selectionMode
 *
 * \brief Item selection mode.
 *
 * A combination of SelectionFlags. By default, \c SelectionItem.
 * Different graph types support different selection modes.
 *
 * \sa SelectionFlags
 */
QAbstract3DGraph::SelectionFlags QAbstract3DGraph::selectionMode() const
{
    return m_graphsItem->selectionMode();
}

void QAbstract3DGraph::setSelectionMode(const QAbstract3DGraph::SelectionFlags &selectionMode)
{
    m_graphsItem->setSelectionMode(selectionMode);
    emit selectionModeChanged(selectionMode);
}

/*!
 * \property QAbstract3DGraph::shadowQuality
 *
 * \brief The quality of the shadow.
 *
 * One of the ShadowQuality enum values. By default, \c ShadowQualityMedium.
 *
 * \note If setting the shadow quality to a certain level fails, the level is lowered
 * until it is successfully set. The \c shadowQualityChanged signal is emitted each time
 * a change is made.
 *
 * \sa ShadowQuality
 */
QAbstract3DGraph::ShadowQuality QAbstract3DGraph::shadowQuality() const
{
    return m_graphsItem->shadowQuality();
}

void QAbstract3DGraph::setShadowQuality(const QAbstract3DGraph::ShadowQuality &shadowQuality)
{
    m_graphsItem->setShadowQuality(shadowQuality);
    emit shadowQualityChanged(shadowQuality);
}

/*!
 * \property QAbstract3DGraph::scene
 *
 * \brief The Q3DScene pointer that can be used to manipulate the scene and
 * access the scene elements, such as the active camera.
 *
 * This property is read-only.
 */
Q3DScene *QAbstract3DGraph::scene() const
{
    return (Q3DScene *)m_graphsItem->scene();
}

/*!
 * Clears selection from all attached series.
 */
void QAbstract3DGraph::clearSelection()
{
    m_graphsItem->clearSelection();
}

/*!
 * Returns whether the \a series has already been added to the graph.
 */
bool QAbstract3DGraph::hasSeries(QAbstract3DSeries *series) const
{
    return m_graphsItem->hasSeries(series);
}

/*!
 * Adds a QCustom3DItem \a item to the graph. Graph takes ownership of the added item.
 *
 * Returns the index to the added item if the add operation was successful, -1
 * if trying to add a null item, and the index of the item if trying to add an
 * already added item.
 *
 * Items are rendered in the order they have been inserted. The rendering order needs to
 * be taken into account when having solid and transparent items.
 *
 * \sa removeCustomItems(), removeCustomItem(), removeCustomItemAt(), customItems()
 */
int QAbstract3DGraph::addCustomItem(QCustom3DItem *item)
{
    return m_graphsItem->addCustomItem(item);
}

/*!
 * Removes all custom items. Deletes the resources allocated to them.
 */
void QAbstract3DGraph::removeCustomItems()
{
    m_graphsItem->removeCustomItems();
}

/*!
 * Removes the custom \a {item}. Deletes the resources allocated to it.
 */
void QAbstract3DGraph::removeCustomItem(QCustom3DItem *item)
{
    m_graphsItem->removeCustomItem(item);
}

/*!
 * Removes all custom items at \a {position}. Deletes the resources allocated to them.
 */
void QAbstract3DGraph::removeCustomItemAt(const QVector3D &position)
{
    m_graphsItem->removeCustomItemAt(position);
}

/*!
 * Gets ownership of given \a item back and removes the \a item from the graph.
 *
 * \note If the same item is added back to the graph, the texture or the texture file needs to be
 * re-set.
 *
 * \sa QCustom3DItem::setTextureImage(), QCustom3DItem::setTextureFile()
 */
void QAbstract3DGraph::releaseCustomItem(QCustom3DItem *item)
{
    return m_graphsItem->releaseCustomItem(item);
}

/*!
 * Returns the list of all added custom items.
 * \sa addCustomItem()
 */
QList<QCustom3DItem *> QAbstract3DGraph::customItems() const
{
    // TODO: API missing in QQuickGraphsItem (QTBUG-99844)
    return {};
//    return m_graphsItem->customItems();
}

/*!
 * Can be used to query the index of the selected label after receiving \c selectedElementChanged
 * signal with any label type. Selection is valid until the next \c selectedElementChanged signal.
 *
 * Returns the index of the selected label, or -1.
 *
 * \sa selectedElement
 */
int QAbstract3DGraph::selectedLabelIndex() const
{
    return m_graphsItem->selectedLabelIndex();
}

/*!
 * Can be used to get the selected axis after receiving \c selectedElementChanged signal with any label
 * type. Selection is valid until the next \c selectedElementChanged signal.
 *
 * Returns the pointer to the selected axis, or null.
 *
 * \sa selectedElement
 */
QAbstract3DAxis *QAbstract3DGraph::selectedAxis() const
{
    return m_graphsItem->selectedAxis();
}

/*!
 * Can be used to query the index of the selected custom item after receiving \c selectedElementChanged
 * signal with QAbstract3DGraph::ElementCustomItem type. Selection is valid until the next
 * \c selectedElementChanged signal.
 *
 * Returns the index of the selected custom item, or -1.
 *
 * \sa selectedElement
 */
int QAbstract3DGraph::selectedCustomItemIndex() const
{
    return m_graphsItem->selectedCustomItemIndex();
}

/*!
 * Can be used to get the selected custom item after receiving \c selectedElementChanged signal with
 * QAbstract3DGraph::ElementCustomItem type. Ownership of the item remains with the graph.
 * Selection is valid until the next \c selectedElementChanged signal.
 *
 * Returns the pointer to the selected custom item, or null.
 *
 * \sa selectedElement
 */
QCustom3DItem *QAbstract3DGraph::selectedCustomItem() const
{
    return m_graphsItem->selectedCustomItem();
}

/*!
 * \property QAbstract3DGraph::selectedElement
 *
 * \brief The element selected in the graph.
 *
 * This property can be used to query the selected element type. The type is
 * valid until a new selection is made in the graph and the
 * \c selectedElementChanged signal is emitted.
 *
 * The signal can be used for example for implementing custom input handlers, as
 * demonstrated in the \l {Graph Gallery} example under \uicontrol {Scatter Graph} tab.
 *
 * \sa selectedLabelIndex(), selectedAxis(), selectedCustomItemIndex(), selectedCustomItem(),
 * Q3DBars::selectedSeries(), Q3DScatter::selectedSeries(), Q3DSurface::selectedSeries(),
 * Q3DScene::setSelectionQueryPosition()
 */
QAbstract3DGraph::ElementType QAbstract3DGraph::selectedElement() const
{
    return m_graphsItem->selectedElement();
}

/*!
 * Renders current frame to an image of \a imageSize. Default size is the window size. Image is
 * rendered with antialiasing level given in \a msaaSamples. Default level is \c{0}.
 *
 * Returns the rendered image.
 *
 * \note OpenGL ES2 does not support anitialiasing, so \a msaaSamples is always forced to \c{0}.
 */
QImage QAbstract3DGraph::renderToImage(int msaaSamples, const QSize &imageSize)
{
    // TODO: API missing in QQuickGraphsItem (QTBUG-111712)
    Q_UNUSED(msaaSamples)
    QSize renderSize = imageSize;
    if (renderSize.isEmpty())
        renderSize = size();
    // TODO: API missing in QQuickGraphsItem (QTBUG-111712)
    return {};//m_graphsItem->renderToImage(msaaSamples, renderSize);
}

/*!
 * \property QAbstract3DGraph::measureFps
 *
 * \brief Whether rendering is done continuously instead of on demand.
 *
 * If \c {true}, rendering is continuous and the value of the currentFps
 * property is updated. Defaults to \c{false}.
 *
 * \sa currentFps
 */
void QAbstract3DGraph::setMeasureFps(bool enable)
{
    m_graphsItem->setMeasureFps(enable);
    if (enable) {
        QObject::connect(m_graphsItem.data(), &QQuickGraphsItem::currentFpsChanged,
                         this, &QAbstract3DGraph::currentFpsChanged);
    } else {
        QObject::disconnect(m_graphsItem.data(), &QQuickGraphsItem::currentFpsChanged,
                            this, &QAbstract3DGraph::currentFpsChanged);
    }
}

bool QAbstract3DGraph::measureFps() const
{
    return m_graphsItem->measureFps();
}

/*!
 * \property QAbstract3DGraph::currentFps
 *
 * \brief The rendering results for the last second.
 *
 * The results are stored in this read-only property when FPS measuring is
 * enabled. It takes at least a second before this value is updated after
 * measuring is activated.
 *
 * \sa measureFps
 */
int QAbstract3DGraph::currentFps() const
{
    return m_graphsItem->currentFps();
}

/*!
 * \property QAbstract3DGraph::orthoProjection
 *
 * \brief Whether orthographic projection is used for displaying the graph.
 *
 * Defaults to \c{false}.
 * \note Shadows will be disabled when set to \c{true}.
 *
 * \sa QAbstract3DAxis::labelAutoRotation, Q3DCamera::cameraPreset
 */
void QAbstract3DGraph::setOrthoProjection(bool enable)
{
    m_graphsItem->setOrthoProjection(enable);
}

bool QAbstract3DGraph::isOrthoProjection() const
{
    return m_graphsItem->isOrthoProjection();
}

/*!
 * \property QAbstract3DGraph::aspectRatio
 *
 * \brief The ratio of the graph scaling between the longest axis on the
 * horizontal plane and the y-axis.
 *
 * Defaults to \c{2.0}.
 *
 * \note Has no effect on Q3DBars.
 *
 * \sa horizontalAspectRatio
 */
void QAbstract3DGraph::setAspectRatio(qreal ratio)
{
    m_graphsItem->setAspectRatio(ratio);
}

qreal QAbstract3DGraph::aspectRatio() const
{
    return m_graphsItem->aspectRatio();
}

/*!
 * \property QAbstract3DGraph::optimizationHints
 *
 * \brief Whether the default, static, or legacy mode is used for rendering optimization.
 *
 * The default mode uses instanced rendering, and provides the full feature set at the best level of
 * performance on most systems. The static mode optimizes graph rendering and is ideal for
 * large non-changing data sets. It is slower with dynamic data changes and item rotations.
 * Selection is not optimized, so using the static mode with massive data sets is not advisable.
 * Static optimization works only on scatter graphs.
 * Legacy mode renders all items in th graph individually, without instancing. It should be used
 * only if default mode does not work, i.e. if the target system does not support instancing.
 * Defaults to \l{OptimizationDefault}.
 *
 * \note On some environments, large graphs using static optimization may not render, because
 * all of the items are rendered using a single draw call, and different graphics drivers
 * support different maximum vertice counts per call.
 * This is mostly an issue on 32bit and OpenGL ES2 platforms.
 * To work around this issue, choose an item mesh with a low vertex count or use
 * the point mesh.
 *
 * \sa QAbstract3DSeries::mesh
 */
void QAbstract3DGraph::setOptimizationHints(QAbstract3DGraph::OptimizationHints hints)
{
    m_graphsItem->setOptimizationHints(hints);
}

QAbstract3DGraph::OptimizationHints QAbstract3DGraph::optimizationHints() const
{
    return m_graphsItem->optimizationHints();
}

/*!
 * \property QAbstract3DGraph::polar
 *
 * \brief Whether horizontal axes are changed into polar axes.
 *
 * If \c {true}, the x-axis becomes the angular axis and the z-axis becomes the
 * radial axis.
 * Polar mode is not available for bar graphs.
 *
 * Defaults to \c{false}.
 *
 * \sa orthoProjection, radialLabelOffset
 */
void QAbstract3DGraph::setPolar(bool enable)
{
    m_graphsItem->setPolar(enable);
}

bool QAbstract3DGraph::isPolar() const
{
    return m_graphsItem->isPolar();
}

/*!
 * \property QAbstract3DGraph::radialLabelOffset
 *
 * \brief The normalized horizontal offset for the axis labels of the radial
 * polar axis.
 *
 * The value \c 0.0 indicates that the labels should be drawn next to the 0-angle
 * angular axis grid line. The value \c 1.0 indicates that the labels are drawn
 * in their usual place at the edge of the graph background. Defaults to \c 1.0.
 *
 * This property is ignored if the \l polar property value is \c{false}.
 *
 * \sa polar
 */
void QAbstract3DGraph::setRadialLabelOffset(float offset)
{
    m_graphsItem->setRadialLabelOffset(offset);
}

float QAbstract3DGraph::radialLabelOffset() const
{
    return m_graphsItem->radialLabelOffset();
}

/*!
 * \property QAbstract3DGraph::horizontalAspectRatio
 *
 * \brief The ratio of the graph scaling between the x-axis and z-axis.
 *
 * The value of \c 0.0 indicates automatic scaling according to axis ranges.
 * Defaults to \c{0.0}.
 *
 * Has no effect on Q3DBars, which handles scaling on the horizontal plane via
 * the \l{Q3DBars::barThickness}{barThickness} and \l{Q3DBars::barSpacing}{barSpacing} properties.
 * Polar graphs also ignore this property.
 *
 * \sa aspectRatio, polar, Q3DBars::barThickness, Q3DBars::barSpacing
 */
void QAbstract3DGraph::setHorizontalAspectRatio(qreal ratio)
{
    m_graphsItem->setHorizontalAspectRatio(ratio);
}

qreal QAbstract3DGraph::horizontalAspectRatio() const
{
    return m_graphsItem->horizontalAspectRatio();
}

/*!
 * \property QAbstract3DGraph::reflection
 *
 * \brief Whether floor reflections are on or off.
 *
 * Defaults to \c{false}.
 *
 * Affects only Q3DBars. However, in Q3DBars graphs holding both positive and
 * negative values, reflections are not supported for custom items that
 * intersect the floor plane. In that case, reflections should be turned off
 * to avoid incorrect rendering.
 *
 * If using a custom surface format, the stencil buffer needs to be defined
 * (QSurfaceFormat::setStencilBufferSize()) for reflections to work.
 *
 * \sa reflectivity
 */
void QAbstract3DGraph::setReflection(bool enable)
{
    m_graphsItem->setReflection(enable);
}

bool QAbstract3DGraph::isReflection() const
{
    // TODO: API missing in QQuickGraphsItem (QTBUG-99816)
    return false;
//    return m_graphsItem->reflection();
}

/*!
 * \property QAbstract3DGraph::reflectivity
 *
 * \brief Floor reflectivity.
 *
 * Larger numbers make the floor more reflective. The valid range is \c{[0...1]}.
 * Defaults to \c{0.5}.
 *
 * \note Affects only Q3DBars.
 *
 * \sa reflection
 */
void QAbstract3DGraph::setReflectivity(qreal reflectivity)
{
    m_graphsItem->setReflectivity(reflectivity);
}

qreal QAbstract3DGraph::reflectivity() const
{
    return m_graphsItem->reflectivity();
}

/*!
 * \property QAbstract3DGraph::locale
 *
 * \brief The locale used for formatting various numeric labels.
 *
 * Defaults to the \c{"C"} locale.
 *
 * \sa QValue3DAxis::labelFormat
 */
void QAbstract3DGraph::setLocale(const QLocale &locale)
{
    m_graphsItem->setLocale(locale);
}

QLocale QAbstract3DGraph::locale() const
{
    return m_graphsItem->locale();
}

/*!
 * \property QAbstract3DGraph::queriedGraphPosition
 *
 * \brief The latest queried graph position values along each axis.
 *
 * This read-only property contains the results from
 * Q3DScene::graphPositionQuery. The values are normalized to the range \c{[-1, 1]}.
 * If the queried position was outside the graph bounds, the values
 * will not reflect the real position, but will instead indicate an undefined position outside
 * the range \c{[-1, 1]}. The value will be undefined until a query is made.
 *
 * There is no single correct 3D coordinate to match a particular screen position, so to be
 * consistent, the queries are always done against the inner sides of an invisible box surrounding
 * the graph.
 *
 * \note Bar graphs only allow querying graph position at the graph floor level,
 * so the y-value is always zero for bar graphs and the valid queries can be only made at
 * screen positions that contain the floor of the graph.
 *
 * \sa Q3DScene::graphPositionQuery
 */
QVector3D QAbstract3DGraph::queriedGraphPosition() const
{
    return m_graphsItem->queriedGraphPosition();
}

/*!
 * \property QAbstract3DGraph::margin
 *
 * \brief The absolute value used for the space left between the edge of the
 * plottable graph area and the edge of the graph background.
 *
 * If the margin value is negative, the margins are determined automatically and can vary according
 * to the size of the items in the series and the type of the graph.
 * The value is interpreted as a fraction of the y-axis range if the graph
 * aspect ratios have not beed changed from the default values.
 * Defaults to \c{-1.0}.
 *
 * \note Setting a smaller margin for a scatter graph than the automatically
 * determined margin can cause the scatter items at the edges of the graph to
 * overlap with the graph background.
 *
 * \note On scatter and surface graphs, if the margin is small in comparison to the axis label
 * size, the positions of the edge labels of the axes are adjusted to avoid overlap with
 * the edge labels of the neighboring axes.
 */
void QAbstract3DGraph::setMargin(qreal margin)
{
    m_graphsItem->setMargin(margin);
}

qreal QAbstract3DGraph::margin() const
{
    return m_graphsItem->margin();
}

/*!
 * \internal
 */
bool QAbstract3DGraph::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchCancel:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        m_graphsItem->touchEvent(static_cast<QTouchEvent *>(event));
        return true;
    default:
        return QWidget::event(event);
    }
}

/*!
 * \internal
 */
void QAbstract3DGraph::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    if (m_graphsItem) {
        QQuickWidget::resizeEvent(event);

        Q3DScene *scene = (Q3DScene *)m_graphsItem->scene();
        scene->d_func()->setWindowSize(QSize(width(), height()));
        scene->d_func()->setViewport(QRect(0, 0, width(), height()));
        if (m_graphsItem->sliceView() && m_graphsItem->sliceView()->isVisible())
            m_graphsItem->minimizeMainGraph();
    }
}

/*!
 * \internal
 */
void QAbstract3DGraph::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_graphsItem->mouseDoubleClickEvent(event);
}

/*!
 * \internal
 */
void QAbstract3DGraph::mousePressEvent(QMouseEvent *event)
{
    m_graphsItem->mousePressEvent(event);
}

/*!
 * \internal
 */
void QAbstract3DGraph::mouseReleaseEvent(QMouseEvent *event)
{
    m_graphsItem->mouseReleaseEvent(event);
}

/*!
 * \internal
 */
void QAbstract3DGraph::mouseMoveEvent(QMouseEvent *event)
{
    m_graphsItem->mouseMoveEvent(event);
}

#if QT_CONFIG(wheelevent)
/*!
 * \internal
 */
void QAbstract3DGraph::wheelEvent(QWheelEvent *event)
{
    m_graphsItem->wheelEvent(event);
}
#endif

QT_END_NAMESPACE
