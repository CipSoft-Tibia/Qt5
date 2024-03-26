// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bars3dcontroller_p.h"
#include "qvalue3daxis_p.h"
#include "qcategory3daxis_p.h"
#include "qbardataproxy_p.h"
#include "qbar3dseries_p.h"
#include "thememanager_p.h"
#include "q3dtheme_p.h"
#include <QtCore/QMutexLocker>

QT_BEGIN_NAMESPACE

Bars3DController::Bars3DController(QRect boundRect, Q3DScene *scene)
    : Abstract3DController(boundRect, scene),
      m_selectedBar(invalidSelectionPosition()),
      m_selectedBarSeries(0),
      m_primarySeries(0),
      m_isMultiSeriesUniform(false),
      m_isBarSpecRelative(true),
      m_barThicknessRatio(1.0f),
      m_barSpacing(QSizeF(1.0, 1.0)),
      m_floorLevel(0.0f),
      m_barSeriesMargin(0.0f, 0.0f)
{
    // Setting a null axis creates a new default axis according to orientation and graph type.
    // Note: these cannot be set in the Abstract3DController constructor, as they will call virtual
    //       functions implemented by subclasses.
    setAxisX(0);
    setAxisY(0);
    setAxisZ(0);
}

Bars3DController::~Bars3DController()
{
}

void Bars3DController::handleArrayReset()
{
    QBar3DSeries *series;
    if (qobject_cast<QBarDataProxy *>(sender()))
        series = static_cast<QBarDataProxy *>(sender())->series();
    else
        series = static_cast<QBar3DSeries *>(sender());

    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
        series->d_func()->markItemLabelDirty();
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);
    // Clear selection unless still valid
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
    series->d_func()->markItemLabelDirty();
    emitNeedRender();
}

void Bars3DController::handleRowsAdded(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);
    emitNeedRender();
}

void Bars3DController::handleRowsChanged(int startIndex, int count)
{
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    int oldChangeCount = m_changedRows.size();
    if (!oldChangeCount)
        m_changedRows.reserve(count);

    for (int i = 0; i < count; i++) {
        bool newItem = true;
        int candidate = startIndex + i;
        for (int j = 0; j < oldChangeCount; j++) {
            const ChangeRow &oldChangeItem = m_changedRows.at(j);
            if (oldChangeItem.row == candidate && series == oldChangeItem.series) {
                newItem = false;
                break;
            }
        }
        if (newItem) {
            ChangeRow newChangeItem = {series, candidate};
            m_changedRows.append(newChangeItem);
            if (series == m_selectedBarSeries && m_selectedBar.x() == candidate)
                series->d_func()->markItemLabelDirty();
        }
    }
    if (count) {
        m_changeTracker.rowsChanged = true;

        if (series->isVisible())
            adjustAxisRanges();

        // Clear selection unless still valid (row length might have changed)
        setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
        emitNeedRender();
    }
}

void Bars3DController::handleRowsRemoved(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);

    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    if (series == m_selectedBarSeries) {
        // If rows removed from selected series before the selection, adjust the selection
        int selectedRow = m_selectedBar.x();
        if (startIndex <= selectedRow) {
            if ((startIndex + count) > selectedRow)
                selectedRow = -1; // Selected row removed
            else
                selectedRow -= count; // Move selected row down by amount of rows removed

            setSelectedBar(QPoint(selectedRow, m_selectedBar.y()), m_selectedBarSeries, false);
        }
    }

    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);

    emitNeedRender();
}

void Bars3DController::handleRowsInserted(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    if (series == m_selectedBarSeries) {
        // If rows inserted to selected series before the selection, adjust the selection
        int selectedRow = m_selectedBar.x();
        if (startIndex <= selectedRow) {
            selectedRow += count;
            setSelectedBar(QPoint(selectedRow, m_selectedBar.y()), m_selectedBarSeries, false);
        }
    }

    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);

    emitNeedRender();
}

void Bars3DController::handleItemChanged(int rowIndex, int columnIndex)
{
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();

    bool newItem = true;
    QPoint candidate(rowIndex, columnIndex);
    foreach (ChangeItem item, m_changedItems) {
        if (item.point == candidate && item.series == series) {
            newItem = false;
            break;
        }
    }

    if (newItem) {
        ChangeItem newItem = {series, candidate};
        m_changedItems.append(newItem);
        m_changeTracker.itemChanged = true;

        if (series == m_selectedBarSeries && m_selectedBar == candidate)
            series->d_func()->markItemLabelDirty();
        if (series->isVisible())
            adjustAxisRanges();
        emitNeedRender();
    }
}

void Bars3DController::handleDataRowLabelsChanged()
{
    if (m_axisZ) {
        // Grab a sublist equal to data window (no need to have more labels in axis)
        int min = int(m_axisZ->min());
        int count = int(m_axisZ->max()) - min + 1;
        QStringList subList;
        if (m_primarySeries && m_primarySeries->dataProxy())
            subList = m_primarySeries->dataProxy()->rowLabels().mid(min, count);
        static_cast<QCategory3DAxis *>(m_axisZ)->d_func()->setDataLabels(subList);
    }
}

void Bars3DController::handleDataColumnLabelsChanged()
{
    if (m_axisX) {
        // Grab a sublist equal to data window (no need to have more labels in axis)
        int min = int(m_axisX->min());
        int count = int(m_axisX->max()) - min + 1;
        QStringList subList;
        if (m_primarySeries && m_primarySeries->dataProxy()) {
            subList = static_cast<QBarDataProxy *>(m_primarySeries->dataProxy())
                    ->columnLabels().mid(min, count);
        }
        static_cast<QCategory3DAxis *>(m_axisX)->d_func()->setDataLabels(subList);
    }
}

void Bars3DController::handleRowColorsChanged()
{
    emitNeedRender();
}

void Bars3DController::handleAxisAutoAdjustRangeChangedInOrientation(
        QAbstract3DAxis::AxisOrientation orientation, bool autoAdjust)
{
    Q_UNUSED(orientation);
    Q_UNUSED(autoAdjust);
    adjustAxisRanges();
}

void Bars3DController::handleSeriesVisibilityChangedBySender(QObject *sender)
{
    Abstract3DController::handleSeriesVisibilityChangedBySender(sender);

    // Visibility changes may require disabling slicing,
    // so just reset selection to ensure everything is still valid.
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
}

QPoint Bars3DController::invalidSelectionPosition()
{
    static QPoint invalidSelectionPos(-1, -1);
    return invalidSelectionPos;
}

void Bars3DController::setAxisX(QAbstract3DAxis *axis)
{
    Abstract3DController::setAxisX(axis);
    handleDataColumnLabelsChanged();
}

void Bars3DController::setAxisZ(QAbstract3DAxis *axis)
{
    Abstract3DController::setAxisZ(axis);
    handleDataRowLabelsChanged();
}

void Bars3DController::setPrimarySeries(QBar3DSeries *series)
{
    if (!series) {
        if (m_seriesList.size())
            series = static_cast<QBar3DSeries *>(m_seriesList.at(0));
    } else if (!m_seriesList.contains(series)) {
        // Add nonexistent series.
        addSeries(series);
    }

    if (m_primarySeries != series) {
        m_primarySeries = series;
        handleDataRowLabelsChanged();
        handleDataColumnLabelsChanged();
        emit primarySeriesChanged(m_primarySeries);
    }
}

QBar3DSeries *Bars3DController::primarySeries() const
{
    return m_primarySeries;
}

void Bars3DController::addSeries(QAbstract3DSeries *series)
{
    insertSeries(m_seriesList.size(), series);
}

void Bars3DController::removeSeries(QAbstract3DSeries *series)
{
    bool wasVisible = (series && series->d_func()->m_controller == this && series->isVisible());

    Abstract3DController::removeSeries(series);

    if (m_selectedBarSeries == series)
        setSelectedBar(invalidSelectionPosition(), 0, false);

    if (wasVisible)
        adjustAxisRanges();

    // If primary series is removed, reset it to default
    if (series == m_primarySeries) {
        if (m_seriesList.size())
            m_primarySeries = static_cast<QBar3DSeries *>(m_seriesList.at(0));
        else
            m_primarySeries = 0;

        handleDataRowLabelsChanged();
        handleDataColumnLabelsChanged();

        emit primarySeriesChanged(m_primarySeries);
    }
}

void Bars3DController::insertSeries(int index, QAbstract3DSeries *series)
{
    Q_ASSERT(series && series->type() == QAbstract3DSeries::SeriesTypeBar);

    int oldSize = m_seriesList.size();

    Abstract3DController::insertSeries(index, series);

    if (oldSize != m_seriesList.size())  {
        QBar3DSeries *barSeries =  static_cast<QBar3DSeries *>(series);
        if (!oldSize) {
            m_primarySeries = barSeries;
            handleDataRowLabelsChanged();
            handleDataColumnLabelsChanged();
        }

        if (barSeries->selectedBar() != invalidSelectionPosition())
            setSelectedBar(barSeries->selectedBar(), barSeries, false);

        if (!oldSize)
            emit primarySeriesChanged(m_primarySeries);
    }
}

QList<QBar3DSeries *> Bars3DController::barSeriesList()
{
    QList<QAbstract3DSeries *> abstractSeriesList = seriesList();
    QList<QBar3DSeries *> barSeriesList;
    foreach (QAbstract3DSeries *abstractSeries, abstractSeriesList) {
        QBar3DSeries *barSeries = qobject_cast<QBar3DSeries *>(abstractSeries);
        if (barSeries)
            barSeriesList.append(barSeries);
    }

    return barSeriesList;
}

void Bars3DController::handleAxisRangeChangedBySender(QObject *sender)
{
    // Data window changed
    if (sender == m_axisX || sender == m_axisZ) {
        if (sender == m_axisX)
            handleDataColumnLabelsChanged();
        if (sender == m_axisZ)
            handleDataRowLabelsChanged();
    }

    Abstract3DController::handleAxisRangeChangedBySender(sender);

    m_isDataDirty = true;

    // Update selected bar - may be moved offscreen
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
}

void Bars3DController::setMultiSeriesScaling(bool uniform)
{
    m_isMultiSeriesUniform = uniform;

    m_changeTracker.multiSeriesScalingChanged = true;
    emitNeedRender();
}

bool Bars3DController::multiSeriesScaling() const
{
    return m_isMultiSeriesUniform;
}

void Bars3DController::setBarSpecs(float thicknessRatio, const QSizeF &spacing, bool relative)
{
    m_barThicknessRatio = thicknessRatio;
    m_barSpacing        = spacing;
    m_isBarSpecRelative = relative;

    m_changeTracker.barSpecsChanged = true;
    emitNeedRender();
}

float Bars3DController::barThickness()
{
    return m_barThicknessRatio;
}

QSizeF Bars3DController::barSpacing()
{
    return m_barSpacing;
}

void Bars3DController::setBarSeriesMargin(const QSizeF &margin)
{
    m_barSeriesMargin = margin;
    m_changeTracker.barSeriesMarginChanged = true;
    emitNeedRender();
}

QSizeF Bars3DController::barSeriesMargin()
{
    return m_barSeriesMargin;
}

bool Bars3DController::isBarSpecRelative()
{
    return m_isBarSpecRelative;
}

void Bars3DController::setFloorLevel(float level)
{
    m_floorLevel = level;
    m_isDataDirty = true;
    m_changeTracker.floorLevelChanged = true;
    emitNeedRender();
}

float Bars3DController::floorLevel() const
{
    return m_floorLevel;
}

void Bars3DController::setSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    if (mode.testFlag(QAbstract3DGraph::SelectionSlice)
            && (mode.testFlag(QAbstract3DGraph::SelectionRow)
                == mode.testFlag(QAbstract3DGraph::SelectionColumn))) {
        qWarning("Must specify one of either row or column selection mode in conjunction with slicing mode.");
    } else {
        QAbstract3DGraph::SelectionFlags oldMode = selectionMode();

        Abstract3DController::setSelectionMode(mode);

        if (mode != oldMode) {
            // Refresh selection upon mode change to ensure slicing is correctly updated
            // according to series the visibility.
            setSelectedBar(m_selectedBar, m_selectedBarSeries, true);

            // Special case: Always deactivate slicing when changing away from slice
            // automanagement, as this can't be handled in setSelectedBar.
            if (!mode.testFlag(QAbstract3DGraph::SelectionSlice)
                    && oldMode.testFlag(QAbstract3DGraph::SelectionSlice)) {
                scene()->setSlicingActive(false);
            }
        }
    }
}

void Bars3DController::setSelectedBar(const QPoint &position, QBar3DSeries *series, bool enterSlice)
{
    // If the selection targets non-existent bar, clear selection instead.
    QPoint pos = position;

    // Series may already have been removed, so check it before setting the selection.
    if (!m_seriesList.contains(series))
        series = nullptr;

    adjustSelectionPosition(pos, series);

    if (series && selectionMode().testFlag(QAbstract3DGraph::SelectionSlice)) {
        // If the selected bar is outside data window, or there is no visible selected bar,
        // disable slicing.
        if (pos.x() < m_axisZ->min() || pos.x() > m_axisZ->max()
                || pos.y() < m_axisX->min() || pos.y() > m_axisX->max()
                || !series->isVisible()) {
            scene()->setSlicingActive(false);
        } else if (enterSlice) {
            scene()->setSlicingActive(true);
        }
        emitNeedRender();
    }

    if (pos != m_selectedBar || series != m_selectedBarSeries) {
        bool seriesChanged = (series != m_selectedBarSeries);
        m_selectedBar = pos;
        m_selectedBarSeries = series;
        m_changeTracker.selectedBarChanged = true;

        // Clear selection from other series and finally set new selection to the specified series
        foreach (QAbstract3DSeries *otherSeries, m_seriesList) {
            QBar3DSeries *barSeries = static_cast<QBar3DSeries *>(otherSeries);
            if (barSeries != m_selectedBarSeries)
                barSeries->d_func()->setSelectedBar(invalidSelectionPosition());
        }
        if (m_selectedBarSeries)
            m_selectedBarSeries->d_func()->setSelectedBar(m_selectedBar);

        if (seriesChanged)
            emit selectedSeriesChanged(m_selectedBarSeries);

        emitNeedRender();
    }
}

void Bars3DController::clearSelection()
{
    setSelectedBar(invalidSelectionPosition(), 0, false);
}

void Bars3DController::adjustAxisRanges()
{
    QCategory3DAxis *categoryAxisZ = static_cast<QCategory3DAxis *>(m_axisZ);
    QCategory3DAxis *categoryAxisX = static_cast<QCategory3DAxis *>(m_axisX);
    QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(m_axisY);

    bool adjustZ = (categoryAxisZ && categoryAxisZ->isAutoAdjustRange());
    bool adjustX = (categoryAxisX && categoryAxisX->isAutoAdjustRange());
    bool adjustY = (valueAxis && categoryAxisX && categoryAxisZ && valueAxis->isAutoAdjustRange());

    if (adjustZ || adjustX || adjustY) {
        int maxRowCount = 0;
        int maxColumnCount = 0;
        float minValue = 0.0f;
        float maxValue = 0.0f;

        // First figure out row and column counts
        int seriesCount = m_seriesList.size();
        if (adjustZ || adjustX) {
            for (int series = 0; series < seriesCount; series++) {
                const QBar3DSeries *barSeries =
                        static_cast<QBar3DSeries *>(m_seriesList.at(series));
                if (barSeries->isVisible()) {
                    const QBarDataProxy *proxy = barSeries->dataProxy();

                    if (adjustZ && proxy) {
                        int rowCount = proxy->rowCount();
                        if (rowCount)
                            rowCount--;

                        maxRowCount = qMax(maxRowCount, rowCount);
                    }

                    if (adjustX && proxy) {
                        const QBarDataArray *array = proxy->array();
                        int columnCount = 0;
                        for (int i = 0; i < array->size(); i++) {
                            if (columnCount < array->at(i)->size())
                                columnCount = array->at(i)->size();
                        }
                        if (columnCount)
                            columnCount--;

                        maxColumnCount = qMax(maxColumnCount, columnCount);
                    }
                }
            }
            // Call private implementations of setRange to avoid unsetting auto adjust flag
            if (adjustZ)
                categoryAxisZ->d_func()->setRange(0.0f, float(maxRowCount), true);
            if (adjustX)
                categoryAxisX->d_func()->setRange(0.0f, float(maxColumnCount), true);
        }

        // Now that we know the row and column ranges, figure out the value axis range
        if (adjustY) {
            for (int series = 0; series < seriesCount; series++) {
                const QBar3DSeries *barSeries =
                        static_cast<QBar3DSeries *>(m_seriesList.at(series));
                if (barSeries->isVisible()) {
                    const QBarDataProxy *proxy = barSeries->dataProxy();
                    if (adjustY && proxy) {
                        QPair<float, float> limits =
                            proxy->d_func()->limitValues(categoryAxisZ->min(),
                                                         categoryAxisZ->max(),
                                                         categoryAxisX->min(),
                                                         categoryAxisX->max());
                        if (!series) {
                            // First series initializes the values
                            minValue = limits.first;
                            maxValue = limits.second;
                        } else {
                            minValue = qMin(minValue, limits.first);
                            maxValue = qMax(maxValue, limits.second);
                        }
                    }
                }
            }

            if (maxValue < 0.0f)
                maxValue = 0.0f;
            if (minValue > 0.0f)
                minValue = 0.0f;
            if (minValue == 0.0f && maxValue == 0.0f) {
                // Only zero value values in data set, set range to something.
                minValue = 0.0f;
                maxValue = 1.0f;
            }
            valueAxis->d_func()->setRange(minValue, maxValue, true);
        }
    }
}

// Invalidate selection position if outside data for the series
void Bars3DController::adjustSelectionPosition(QPoint &pos, const QBar3DSeries *series)
{
    const QBarDataProxy *proxy = 0;
    if (series)
        proxy = series->dataProxy();

    if (!proxy)
        pos = invalidSelectionPosition();

    if (pos != invalidSelectionPosition()) {
        int maxRow = proxy->rowCount() - 1;
        int maxCol = (pos.x() <= maxRow && pos.x() >= 0 && proxy->rowAt(pos.x()))
                ? proxy->rowAt(pos.x())->size() - 1 : -1;

        if (pos.x() < 0 || pos.x() > maxRow || pos.y() < 0 || pos.y() > maxCol)
            pos = invalidSelectionPosition();
    }
}

QAbstract3DAxis *Bars3DController::createDefaultAxis(QAbstract3DAxis::AxisOrientation orientation)
{
    QAbstract3DAxis *defaultAxis = 0;

    if (orientation == QAbstract3DAxis::AxisOrientationY)
        defaultAxis = createDefaultValueAxis();
    else
        defaultAxis = createDefaultCategoryAxis();

    return defaultAxis;
}

QT_END_NAMESPACE
