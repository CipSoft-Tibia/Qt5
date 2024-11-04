// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qmath.h>
#include "qabstract3daxis_p.h"
#include "qbar3dseries_p.h"
#include "qcategory3daxis_p.h"
#include "qquickgraphsbars_p.h"
#include "qvalue3daxis_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QBar3DSeries
 * \inmodule QtGraphs
 * \ingroup graphs_3D
 * \brief The QBar3DSeries class represents a data series in a 3D bar graph.
 *
 * This class manages the series specific visual elements, as well as the series
 * data (via a data proxy).
 *
 * If no data proxy is set explicitly for the series, the series creates a
 * default proxy. Setting another proxy will destroy the existing proxy and all
 * data added to it.
 *
 * QBar3DSeries supports the following format tags for QAbstract3DSeries::setItemLabelFormat():
 * \table
 *   \row
 *     \li @rowTitle      \li Title from row axis
 *   \row
 *     \li @colTitle      \li Title from column axis
 *   \row
 *     \li @valueTitle    \li Title from value axis
 *   \row
 *     \li @rowIdx        \li Visible row index. Localized using the graph locale.
 *   \row
 *     \li @colIdx        \li Visible column index. Localized using the graph locale.
 *   \row
 *     \li @rowLabel      \li Label from row axis
 *   \row
 *     \li @colLabel      \li Label from column axis
 *   \row
 *     \li @valueLabel    \li Item value formatted using the format of the value
 *                            axis attached to the graph. For more information,
 *                            see \l{QValue3DAxis::labelFormat}.
 *   \row
 *     \li @seriesName    \li Name of the series
 *   \row
 *     \li %<format spec> \li Item value in the specified format. Formatted
 *                            using the same rules as \l{QValue3DAxis::labelFormat}.
 * \endtable
 *
 * For example:
 * \snippet doc_src_qtgraphs.cpp labelformat
 *
 * \sa {Qt Graphs Data Handling with 3D}, QAbstract3DGraph::locale
 */

/*!
 * \qmltype Bar3DSeries
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml_3D
 * \instantiates QBar3DSeries
 * \inherits Abstract3DSeries
 * \brief Represents a data series in a 3D bar graph.
 *
 * This type manages the series specific visual elements, as well as the series
 * data (via a data proxy).
 *
 * For a more complete description, see QBar3DSeries.
 *
 * \sa {Qt Graphs Data Handling with 3D}
 */

/*!
 * \qmlproperty BarDataProxy Bar3DSeries::dataProxy
 *
 * The active data proxy. The series assumes ownership of any proxy set to
 * it and deletes any previously set proxy when a new one is added. The proxy
 * cannot be null or set to another series.
 */

/*!
 * \qmlproperty point Bar3DSeries::selectedBar
 *
 * The bar in the series that is selected.
 *
 * The position of the selected bar is specified as a row and column in the
 * data array of the series.
 *
 * Only one bar can be selected at a time.
 *
 * To clear selection from this series, set invalidSelectionPosition as the
 * position.
 *
 * If this series is added to a graph, the graph can adjust the selection
 * according to user interaction or if it becomes invalid. Selecting a bar on
 * another added series will also clear the selection.
 *
 * Removing rows from or inserting rows to the series before the row of the
 * selected bar will adjust the selection so that the same bar will stay
 * selected.
 *
 * \sa {AbstractGraph3D::clearSelection()}{AbstractGraph3D.clearSelection()}
 */

/*!
 * \qmlproperty point Bar3DSeries::invalidSelectionPosition
 * A constant property providing an invalid position for selection. This
 * position is set to the selectedBar property to clear the selection from this
 * series.
 *
 * \sa {AbstractGraph3D::clearSelection()}{AbstractGraph3D.clearSelection()}
 */

/*!
 * \qmlproperty real Bar3DSeries::meshAngle
 *
 * A convenience property for defining the series rotation angle in degrees.
 *
 * \note When reading this property, it is calculated from the
 * \l{Abstract3DSeries::meshRotation}{Abstract3DSeries.meshRotation} value
 * using floating point precision and always returns a value from zero to 360
 * degrees.
 *
 * \sa {Abstract3DSeries::meshRotation}{Abstract3DSeries.meshRotation}
 */

/*!
 * \qmlproperty list<Color> Bar3DSeries::rowColors
 * This property can be used to draw the rows of the series in different colors.
 * The \l{Theme3D::colorStyle}{Theme3D.colorStyle} must be set to
 * \c Uniform to use this property.
 * \note If the property is set and the theme is changed,
 * the rowColors list is not cleared automatically.
 *
 * \sa Q3DTheme::ColorStyle::Uniform
 */

/*!
 * Constructsa bar 3D series with the parent \a parent.
 */
QBar3DSeries::QBar3DSeries(QObject *parent)
    : QAbstract3DSeries(*(new QBar3DSeriesPrivate()), parent)
{
    Q_D(QBar3DSeries);
    // Default proxy
    d->setDataProxy(new QBarDataProxy);
    connectSignals();
}

/*!
 * Constructs a bar 3D series with the data proxy \a dataProxy and the parent
 * \a parent.
 */
QBar3DSeries::QBar3DSeries(QBarDataProxy *dataProxy, QObject *parent)
    : QAbstract3DSeries(*(new QBar3DSeriesPrivate()), parent)
{
    Q_D(QBar3DSeries);
    d->setDataProxy(dataProxy);
    connectSignals();
}

/*!
 * Deletes a bar 3D series.
 */
QBar3DSeries::~QBar3DSeries() {}

/*!
 * \property QBar3DSeries::dataProxy
 *
 * \brief The active data proxy.
 *
 * The series assumes ownership of any proxy set to it and deletes any
 * previously set proxy when a new one is added. The proxy cannot be null or
 * set to another series.
 */
void QBar3DSeries::setDataProxy(QBarDataProxy *proxy)
{
    Q_D(QBar3DSeries);
    d->setDataProxy(proxy);
}

QBarDataProxy *QBar3DSeries::dataProxy() const
{
    const Q_D(QBar3DSeries);
    return static_cast<QBarDataProxy *>(d->dataProxy());
}

/*!
 * \property QBar3DSeries::selectedBar
 *
 * \brief The bar in the series that is selected.
 *
 */

/*!
 * Selects the bar at the \a position position, specified as a row and column in
 * the data array of the series.
 *
 * Only one bar can be selected at a time.
 *
 * To clear selection from this series, invalidSelectionPosition() is set as
 * \a position.
 *
 * If this series is added to a graph, the graph can adjust the selection
 * according to user interaction or if it becomes invalid. Selecting a bar on
 * another added series will also clear the selection.
 *
 * Removing rows from or inserting rows to the series before the row of the
 * selected bar will adjust the selection so that the same bar will stay
 * selected.
 *
 * \sa QAbstract3DGraph::clearSelection()
 */
void QBar3DSeries::setSelectedBar(const QPoint &position)
{
    Q_D(QBar3DSeries);
    // Don't do this in private to avoid loops, as that is used for callback from
    // graph.
    if (d->m_graph)
        static_cast<QQuickGraphsBars *>(d->m_graph)->setSelectedBar(position, this, true);
    else
        d->setSelectedBar(position);
}

QPoint QBar3DSeries::selectedBar() const
{
    const Q_D(QBar3DSeries);
    return d->m_selectedBar;
}

/*!
 * Returns an invalid position for selection. This position is set to the
 * selectedBar property to clear the selection from this series.
 *
 * \sa QAbstract3DGraph::clearSelection()
 */
QPoint QBar3DSeries::invalidSelectionPosition()
{
    return QQuickGraphsBars::invalidSelectionPosition();
}

static inline float quaternionAngle(const QQuaternion &rotation)
{
    return qRadiansToDegrees(qAcos(rotation.scalar())) * 2.f;
}

/*!
  \property QBar3DSeries::meshAngle

  \brief The series rotation angle in degrees.

  Setting this property is equivalent to the following call:

  \code
  setMeshRotation(QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, angle))
  \endcode

  \note When reading this property, it is calculated from the
        QAbstract3DSeries::meshRotation value using floating point precision
        and always returns a value from zero to 360 degrees.

  \sa QAbstract3DSeries::meshRotation
 */
void QBar3DSeries::setMeshAngle(float angle)
{
    setMeshRotation(QQuaternion::fromAxisAndAngle(upVector, angle));
}

float QBar3DSeries::meshAngle() const
{
    QQuaternion rotation = meshRotation();

    if (rotation.isIdentity() || rotation.x() != 0.0f || rotation.z() != 0.0f)
        return 0.0f;
    else
        return quaternionAngle(rotation);
}

/*!
 * \property QBar3DSeries::rowColors
 *
 * \brief The list of row colors in the series.
 *
 * This property can be used to color
 * the rows of the series in different colors.
 * The Q3DTheme::ColorStyle must be set to
 * Q3DTheme::ColorStyle::Uniform to use this property.
 *
 * \sa Q3DTheme::ColorStyle::Uniform
 */
void QBar3DSeries::setRowColors(const QList<QColor> &colors)
{
    Q_D(QBar3DSeries);
    d->setRowColors(colors);
}
QList<QColor> QBar3DSeries::rowColors() const
{
    const Q_D(QBar3DSeries);
    return d->m_rowColors;
}

/*!
 * \internal
 */
void QBar3DSeries::connectSignals()
{
    QObject::connect(this,
                     &QAbstract3DSeries::meshRotationChanged,
                     this,
                     &QBar3DSeries::handleMeshRotationChanged);
}

/*!
 * \internal
 */
void QBar3DSeries::handleMeshRotationChanged(const QQuaternion &rotation)
{
    emit meshAngleChanged(quaternionAngle(rotation));
}

// QBar3DSeriesPrivate

QBar3DSeriesPrivate::QBar3DSeriesPrivate()
    : QAbstract3DSeriesPrivate(QAbstract3DSeries::SeriesType::Bar)
    , m_selectedBar(QQuickGraphsBars::invalidSelectionPosition())
{
    m_itemLabelFormat = QStringLiteral("@valueLabel");
    m_mesh = QAbstract3DSeries::Mesh::BevelBar;
}

QBar3DSeriesPrivate::~QBar3DSeriesPrivate() {}

void QBar3DSeriesPrivate::setDataProxy(QAbstractDataProxy *proxy)
{
    Q_ASSERT(proxy->type() == QAbstractDataProxy::DataType::Bar);
    Q_Q(QBar3DSeries);

    QAbstract3DSeriesPrivate::setDataProxy(proxy);

    emit q->dataProxyChanged(static_cast<QBarDataProxy *>(proxy));
}

void QBar3DSeriesPrivate::connectGraphAndProxy(QQuickGraphsItem *newGraph)
{
    Q_Q(QBar3DSeries);
    QBarDataProxy *barDataProxy = static_cast<QBarDataProxy *>(m_dataProxy);

    if (m_graph && barDataProxy) {
        // Disconnect old graph/old proxy
        QObject::disconnect(barDataProxy, 0, m_graph, 0);
        QObject::disconnect(q, 0, m_graph, 0);
    }

    if (newGraph && barDataProxy) {
        QQuickGraphsBars *graph = static_cast<QQuickGraphsBars *>(newGraph);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::arrayReset,
                         graph,
                         &QQuickGraphsBars::handleArrayReset);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::rowsAdded,
                         graph,
                         &QQuickGraphsBars::handleRowsAdded);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::rowsChanged,
                         graph,
                         &QQuickGraphsBars::handleRowsChanged);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::rowsRemoved,
                         graph,
                         &QQuickGraphsBars::handleRowsRemoved);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::rowsInserted,
                         graph,
                         &QQuickGraphsBars::handleRowsInserted);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::itemChanged,
                         graph,
                         &QQuickGraphsBars::handleItemChanged);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::rowLabelsChanged,
                         graph,
                         &QQuickGraphsBars::handleDataRowLabelsChanged);
        QObject::connect(barDataProxy,
                         &QBarDataProxy::columnLabelsChanged,
                         graph,
                         &QQuickGraphsBars::handleDataColumnLabelsChanged);
        QObject::connect(q,
                         &QBar3DSeries::dataProxyChanged,
                         graph,
                         &QQuickGraphsBars::handleArrayReset);
        QObject::connect(q,
                         &QBar3DSeries::rowColorsChanged,
                         graph,
                         &QQuickGraphsBars::handleRowColorsChanged);
    }
}

void QBar3DSeriesPrivate::createItemLabel()
{
    Q_Q(QBar3DSeries);
    static const QString rowIndexTag(QStringLiteral("@rowIdx"));
    static const QString rowLabelTag(QStringLiteral("@rowLabel"));
    static const QString rowTitleTag(QStringLiteral("@rowTitle"));
    static const QString colIndexTag(QStringLiteral("@colIdx"));
    static const QString colLabelTag(QStringLiteral("@colLabel"));
    static const QString colTitleTag(QStringLiteral("@colTitle"));
    static const QString valueTitleTag(QStringLiteral("@valueTitle"));
    static const QString valueLabelTag(QStringLiteral("@valueLabel"));
    static const QString seriesNameTag(QStringLiteral("@seriesName"));

    if (m_selectedBar == QBar3DSeries::invalidSelectionPosition()) {
        m_itemLabel = QString();
        return;
    }

    QLocale locale(QLocale::c());
    if (m_graph)
        locale = m_graph->locale();
    else
        return;

    QCategory3DAxis *categoryAxisZ = static_cast<QCategory3DAxis *>(m_graph->axisZ());
    QCategory3DAxis *categoryAxisX = static_cast<QCategory3DAxis *>(m_graph->axisX());
    QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(m_graph->axisY());
    qreal selectedBarValue = qreal(q->dataProxy()->itemAt(m_selectedBar).value());

    // Custom format expects printf format specifier. There is no tag for it.
    m_itemLabel = valueAxis->formatter()->stringForValue(selectedBarValue, m_itemLabelFormat);

    int selBarPosRow = m_selectedBar.x();
    int selBarPosCol = m_selectedBar.y();
    m_itemLabel.replace(rowIndexTag, locale.toString(selBarPosRow));
    if (categoryAxisZ->labels().size() > selBarPosRow)
        m_itemLabel.replace(rowLabelTag, categoryAxisZ->labels().at(selBarPosRow));
    else
        m_itemLabel.replace(rowLabelTag, QString());
    m_itemLabel.replace(rowTitleTag, categoryAxisZ->title());
    m_itemLabel.replace(colIndexTag, locale.toString(selBarPosCol));
    if (categoryAxisX->labels().size() > selBarPosCol)
        m_itemLabel.replace(colLabelTag, categoryAxisX->labels().at(selBarPosCol));
    else
        m_itemLabel.replace(colLabelTag, QString());
    m_itemLabel.replace(colTitleTag, categoryAxisX->title());
    m_itemLabel.replace(valueTitleTag, valueAxis->title());

    if (m_itemLabel.contains(valueLabelTag)) {
        QString valueLabelText = valueAxis->formatter()->stringForValue(selectedBarValue,
                                                                        valueAxis->labelFormat());
        m_itemLabel.replace(valueLabelTag, valueLabelText);
    }

    m_itemLabel.replace(seriesNameTag, m_name);
}

void QBar3DSeriesPrivate::setSelectedBar(const QPoint &position)
{
    Q_Q(QBar3DSeries);
    if (position != m_selectedBar) {
        markItemLabelDirty();
        m_selectedBar = position;
        emit q->selectedBarChanged(m_selectedBar);
    }
}

void QBar3DSeriesPrivate::setRowColors(const QList<QColor> &colors)
{
    Q_Q(QBar3DSeries);
    if (m_rowColors != colors) {
        m_rowColors = colors;
        emit q->rowColorsChanged(m_rowColors);
    }
}

QT_END_NAMESPACE
