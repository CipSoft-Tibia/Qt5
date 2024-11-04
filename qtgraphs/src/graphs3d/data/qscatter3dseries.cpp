// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcategory3daxis.h"
#include "qquickgraphsscatter_p.h"
#include "qscatter3dseries_p.h"
#include "qvalue3daxis.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QScatter3DSeries
 * \inmodule QtGraphs
 * \ingroup graphs_3D
 * \brief The QScatter3DSeries class represents a data series in a 3D scatter
 * graph.
 *
 * This class manages the series specific visual elements, as well as the series
 * data (via a data proxy).
 *
 * If no data proxy is set explicitly for the series, the series creates a
 * default proxy. Setting another proxy will destroy the existing proxy and all
 * data added to it.
 *
 * QScatter3DSeries supports the following format tags for QAbstract3DSeries::setItemLabelFormat():
 * \table
 *   \row
 *     \li @xTitle    \li Title from x-axis
 *   \row
 *     \li @yTitle    \li Title from y-axis
 *   \row
 *     \li @zTitle    \li Title from z-axis
 *   \row
 *     \li @xLabel    \li Item value formatted using the format of the x-axis.
 *                        For more information, see
 *                        \l{QValue3DAxis::setLabelFormat()}.
 *   \row
 *     \li @yLabel    \li Item value formatted using the format of the y-axis.
 *                        For more information, see
 *                        \l{QValue3DAxis::setLabelFormat()}.
 *   \row
 *     \li @zLabel    \li Item value formatted using the format of the z-axis.
 *                        For more information, see
 *                        \l{QValue3DAxis::setLabelFormat()}.
 *   \row
 *     \li @seriesName \li Name of the series
 * \endtable
 *
 * For example:
 * \snippet doc_src_qtgraphs.cpp labelformat
 *
 * \sa {Qt Graphs Data Handling with 3D}
 */

/*!
 * \qmltype Scatter3DSeries
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml_3D
 * \instantiates QScatter3DSeries
 * \inherits Abstract3DSeries
 * \brief Represents a data series in a 3D scatter graph.
 *
 * This type manages the series specific visual elements, as well as the series
 * data (via a data proxy).
 *
 * For a more complete description, see QScatter3DSeries.
 *
 * \sa {Qt Graphs Data Handling with 3D}
 */

/*!
 * \qmlproperty ScatterDataProxy Scatter3DSeries::dataProxy
 *
 * Sets the active data proxy. The series assumes ownership of any proxy set to
 * it and deletes any previously set proxy when a new one is added. The proxy
 * cannot be null or set to another series.
 */

/*!
 * \qmlproperty int Scatter3DSeries::selectedItem
 *
 * The item that is selected at the index in the data array of the series.
 * Only one item can be selected at a time.
 * To clear selection from this series, invalidSelectionIndex is set as the
 * index. If this series is added to a graph, the graph can adjust the selection
 * according to user interaction or if it becomes invalid. Selecting an item on
 * another added series will also clear the selection. Removing items from or
 * inserting items to the series before the selected item will adjust the
 * selection so that the same item will stay selected.
 *
 * \sa AbstractGraph3D::clearSelection()
 */

/*!
 * \qmlproperty float Scatter3DSeries::itemSize
 *
 * Sets the item size for the series. The size must be between \c 0.0 and
 * \c 1.0. Setting the size to \c 0.0 causes the item size to be automatically
 * scaled based on the total number of items in all the series for the graph.
 * The preset default is \c 0.0.
 */

/*!
 * \qmlproperty int Scatter3DSeries::invalidSelectionIndex
 * A constant property providing an invalid index for selection. This index is
 * set to the selectedItem property to clear the selection from this series.
 *
 * \sa AbstractGraph3D::clearSelection()
 */

/*!
 * Constructs a scatter 3D series with the parent \a parent.
 */
QScatter3DSeries::QScatter3DSeries(QObject *parent)
    : QAbstract3DSeries(*(new QScatter3DSeriesPrivate()), parent)
{
    Q_D(QScatter3DSeries);
    // Default proxy
    d->setDataProxy(new QScatterDataProxy);
}

/*!
 * Constructs a scatter 3D series with the data proxy \a dataProxy and the
 * parent \a parent.
 */
QScatter3DSeries::QScatter3DSeries(QScatterDataProxy *dataProxy, QObject *parent)
    : QAbstract3DSeries(*(new QScatter3DSeriesPrivate()), parent)
{
    Q_D(QScatter3DSeries);
    d->setDataProxy(dataProxy);
}

/*!
 * \internal
 */
QScatter3DSeries::QScatter3DSeries(QScatter3DSeriesPrivate &d, QObject *parent)
    : QAbstract3DSeries(d, parent)
{}

/*!
 * Deletes the scatter 3D series.
 */
QScatter3DSeries::~QScatter3DSeries() {}

/*!
 * \property QScatter3DSeries::dataProxy
 *
 * \brief The active data proxy.
 */

/*!
 * Sets the active data proxy for the series to \a proxy. The series assumes
 * ownership of any proxy set to it and deletes any previously set proxy when
 * a new one is added. The \a proxy argument cannot be null or set to another
 * series.
 */
void QScatter3DSeries::setDataProxy(QScatterDataProxy *proxy)
{
    Q_D(QScatter3DSeries);
    d->setDataProxy(proxy);
}

QScatterDataProxy *QScatter3DSeries::dataProxy() const
{
    const Q_D(QScatter3DSeries);
    return static_cast<QScatterDataProxy *>(d->dataProxy());
}

/*!
 * \property QScatter3DSeries::selectedItem
 *
 * \brief The item that is selected in the series.
 */

/*!
 * Selects the item at the index \a index in the data array of the series.
 * Only one item can be selected at a time.
 *
 * To clear selection from this series, invalidSelectionIndex() is set as \a
 * index. If this series is added to a graph, the graph can adjust the selection
 * according to user interaction or if it becomes invalid. Selecting an item on
 * another added series will also clear the selection.
 *
 * Removing items from or inserting items to the series before the selected item
 * will adjust the selection so that the same item will stay selected.
 *
 * \sa QAbstract3DGraph::clearSelection()
 */
void QScatter3DSeries::setSelectedItem(int index)
{
    Q_D(QScatter3DSeries);
    // Don't do this in private to avoid loops, as that is used for callback from
    // graph.
    if (d->m_graph)
        static_cast<QQuickGraphsScatter *>(d->m_graph)->setSelectedItem(index, this);
    else
        d->setSelectedItem(index);
}

int QScatter3DSeries::selectedItem() const
{
    const Q_D(QScatter3DSeries);
    return d->m_selectedItem;
}

/*!
 * \property QScatter3DSeries::itemSize
 *
 * \brief Item size for the series.
 *
 * The size must be between \c 0.0f and \c 1.0f. Setting the size to \c 0.0f
 * causes the item size to be automatically scaled based on the total number of
 * items in all the series for the graph.
 *
 * The preset default is \c 0.0f.
 */
void QScatter3DSeries::setItemSize(float size)
{
    Q_D(QScatter3DSeries);
    if (size < 0.0f || size > 1.0f) {
        qWarning("Invalid size. Valid range for itemSize is 0.0f...1.0f");
    } else if (size != d->m_itemSize) {
        d->setItemSize(size);
        emit itemSizeChanged(size);
    }
}

float QScatter3DSeries::itemSize() const
{
    const Q_D(QScatter3DSeries);
    return d->m_itemSize;
}

/*!
 * Returns an invalid index for selection. This index is set to the selectedItem
 * property to clear the selection from this series.
 *
 * \sa QAbstract3DGraph::clearSelection()
 */
int QScatter3DSeries::invalidSelectionIndex()
{
    return QQuickGraphsScatter::invalidSelectionIndex();
}

// QScatter3DSeriesPrivate

QScatter3DSeriesPrivate::QScatter3DSeriesPrivate()
    : QAbstract3DSeriesPrivate(QAbstract3DSeries::SeriesType::Scatter)
    , m_selectedItem(QQuickGraphsScatter::invalidSelectionIndex())
    , m_itemSize(0.0f)
{
    m_itemLabelFormat = QStringLiteral("@xLabel, @yLabel, @zLabel");
    m_mesh = QAbstract3DSeries::Mesh::Sphere;
}

QScatter3DSeriesPrivate::~QScatter3DSeriesPrivate() {}

void QScatter3DSeriesPrivate::setDataProxy(QAbstractDataProxy *proxy)
{
    Q_Q(QScatter3DSeries);
    Q_ASSERT(proxy->type() == QAbstractDataProxy::DataType::Scatter);

    QAbstract3DSeriesPrivate::setDataProxy(proxy);

    emit q->dataProxyChanged(static_cast<QScatterDataProxy *>(proxy));
}

void QScatter3DSeriesPrivate::connectGraphAndProxy(QQuickGraphsItem *newGraph)
{
    Q_Q(QScatter3DSeries);
    QScatterDataProxy *scatterDataProxy = static_cast<QScatterDataProxy *>(m_dataProxy);

    if (m_graph && scatterDataProxy) {
        // Disconnect old graph/old proxy
        QObject::disconnect(scatterDataProxy, 0, m_graph, 0);
        QObject::disconnect(q, 0, m_graph, 0);
    }

    if (newGraph && scatterDataProxy) {
        QQuickGraphsScatter *graph = static_cast<QQuickGraphsScatter *>(newGraph);
        QObject::connect(scatterDataProxy,
                         &QScatterDataProxy::arrayReset,
                         graph,
                         &QQuickGraphsScatter::handleArrayReset);
        QObject::connect(scatterDataProxy,
                         &QScatterDataProxy::itemsAdded,
                         graph,
                         &QQuickGraphsScatter::handleItemsAdded);
        QObject::connect(scatterDataProxy,
                         &QScatterDataProxy::itemsChanged,
                         graph,
                         &QQuickGraphsScatter::handleItemsChanged);
        QObject::connect(scatterDataProxy,
                         &QScatterDataProxy::itemsRemoved,
                         graph,
                         &QQuickGraphsScatter::handleItemsRemoved);
        QObject::connect(scatterDataProxy,
                         &QScatterDataProxy::itemsInserted,
                         graph,
                         &QQuickGraphsScatter::handleItemsInserted);
        QObject::connect(q,
                         &QScatter3DSeries::dataProxyChanged,
                         graph,
                         &QQuickGraphsScatter::handleArrayReset);
    }
}

void QScatter3DSeriesPrivate::createItemLabel()
{
    Q_Q(QScatter3DSeries);
    static const QString xTitleTag(QStringLiteral("@xTitle"));
    static const QString yTitleTag(QStringLiteral("@yTitle"));
    static const QString zTitleTag(QStringLiteral("@zTitle"));
    static const QString xLabelTag(QStringLiteral("@xLabel"));
    static const QString yLabelTag(QStringLiteral("@yLabel"));
    static const QString zLabelTag(QStringLiteral("@zLabel"));
    static const QString seriesNameTag(QStringLiteral("@seriesName"));

    if (m_selectedItem == QScatter3DSeries::invalidSelectionIndex()) {
        m_itemLabel = QString();
        return;
    }

    QValue3DAxis *axisX = static_cast<QValue3DAxis *>(m_graph->axisX());
    QValue3DAxis *axisY = static_cast<QValue3DAxis *>(m_graph->axisY());
    QValue3DAxis *axisZ = static_cast<QValue3DAxis *>(m_graph->axisZ());
    QVector3D selectedPosition = q->dataProxy()->itemAt(m_selectedItem).position();

    m_itemLabel = m_itemLabelFormat;

    m_itemLabel.replace(xTitleTag, axisX->title());
    m_itemLabel.replace(yTitleTag, axisY->title());
    m_itemLabel.replace(zTitleTag, axisZ->title());

    if (m_itemLabel.contains(xLabelTag)) {
        QString valueLabelText = axisX->formatter()->stringForValue(qreal(selectedPosition.x()),
                                                                    axisX->labelFormat());
        m_itemLabel.replace(xLabelTag, valueLabelText);
    }
    if (m_itemLabel.contains(yLabelTag)) {
        QString valueLabelText = axisY->formatter()->stringForValue(qreal(selectedPosition.y()),
                                                                    axisY->labelFormat());
        m_itemLabel.replace(yLabelTag, valueLabelText);
    }
    if (m_itemLabel.contains(zLabelTag)) {
        QString valueLabelText = axisZ->formatter()->stringForValue(qreal(selectedPosition.z()),
                                                                    axisZ->labelFormat());
        m_itemLabel.replace(zLabelTag, valueLabelText);
    }
    m_itemLabel.replace(seriesNameTag, m_name);
}

void QScatter3DSeriesPrivate::setSelectedItem(int index)
{
    Q_Q(QScatter3DSeries);
    if (index != m_selectedItem) {
        markItemLabelDirty();
        m_selectedItem = index;
        emit q->selectedItemChanged(m_selectedItem);
    }
}

void QScatter3DSeriesPrivate::setItemSize(float size)
{
    m_itemSize = size;
    if (m_graph)
        m_graph->markSeriesVisualsDirty();
}

QT_END_NAMESPACE
