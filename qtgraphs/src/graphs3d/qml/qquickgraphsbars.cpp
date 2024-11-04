// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "q3dtheme_p.h"
#include "qbar3dseries_p.h"
#include "qbardataproxy_p.h"
#include "qcategory3daxis_p.h"
#include "qquickgraphsbars_p.h"
#include "qquickgraphstexturedata_p.h"
#include "qvalue3daxis_p.h"

#include <QColor>
#include <QtCore/QMutexLocker>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3drepeater_p.h>

QQuickGraphsBars::QQuickGraphsBars(QQuickItem *parent)
    : QQuickGraphsItem(parent)
{
    m_graphType = QAbstract3DSeries::SeriesType::Bar;
    setAxisX(0);
    setAxisY(0);
    setAxisZ(0);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlags(ItemHasContents);
    createInitialInputHandler();
    clearSelection();
}

QQuickGraphsBars::~QQuickGraphsBars()
{
    QMutexLocker locker(m_nodeMutex.data());
    const QMutexLocker locker2(mutex());
    removeBarModels();
    removeSlicedBarModels();
}

QCategory3DAxis *QQuickGraphsBars::rowAxis() const
{
    return static_cast<QCategory3DAxis *>(axisZ());
}

void QQuickGraphsBars::setRowAxis(QCategory3DAxis *axis)
{
    QQuickGraphsItem::setAxisZ(axis);
}

QValue3DAxis *QQuickGraphsBars::valueAxis() const
{
    return static_cast<QValue3DAxis *>(axisY());
}

void QQuickGraphsBars::setValueAxis(QValue3DAxis *axis)
{
    QQuickGraphsItem::setAxisY(axis);
}

QCategory3DAxis *QQuickGraphsBars::columnAxis() const
{
    return static_cast<QCategory3DAxis *>(axisX());
}

void QQuickGraphsBars::setColumnAxis(QCategory3DAxis *axis)
{
    QQuickGraphsItem::setAxisX(axis);
}

void QQuickGraphsBars::setMultiSeriesScaling(bool uniform)
{
    m_isMultiSeriesUniform = uniform;

    m_changeTracker.multiSeriesScalingChanged = true;
    emitNeedRender();
}

bool QQuickGraphsBars::multiSeriesScaling() const
{
    return m_isMultiSeriesUniform;
}

void QQuickGraphsBars::setMultiSeriesUniform(bool uniform)
{
    if (uniform != isMultiSeriesUniform()) {
        setMultiSeriesScaling(uniform);
        emit multiSeriesUniformChanged(uniform);
    }
}

bool QQuickGraphsBars::isMultiSeriesUniform() const
{
    return multiSeriesScaling();
}

void QQuickGraphsBars::setBarSpecs(float thicknessRatio, const QSizeF &spacing, bool relative)
{
    m_barThicknessRatio = thicknessRatio;
    m_barSpacing = spacing;
    m_isBarSpecRelative = relative;

    m_changeTracker.barSpecsChanged = true;
    emitNeedRender();
}

void QQuickGraphsBars::setBarThickness(float thicknessRatio)
{
    if (thicknessRatio != barThickness()) {
        setBarSpecs(thicknessRatio, barSpacing(), isBarSpacingRelative());
        emit barThicknessChanged(thicknessRatio);
    }
}

float QQuickGraphsBars::barThickness() const
{
    return m_barThicknessRatio;
}

void QQuickGraphsBars::setBarSpacing(const QSizeF &spacing)
{
    if (spacing != barSpacing()) {
        setBarSpecs(barThickness(), spacing, isBarSpacingRelative());
        emit barSpacingChanged(spacing);
    }
}

QSizeF QQuickGraphsBars::barSpacing() const
{
    return m_barSpacing;
}

void QQuickGraphsBars::setBarSpacingRelative(bool relative)
{
    if (relative != isBarSpacingRelative()) {
        setBarSpecs(barThickness(), barSpacing(), relative);
        emit barSpacingRelativeChanged(relative);
    }
}

bool QQuickGraphsBars::isBarSpacingRelative() const
{
    return m_isBarSpecRelative;
}

void QQuickGraphsBars::setBarSeriesMargin(const QSizeF &margin)
{
    if (margin != barSeriesMargin()) {
        m_barSeriesMargin = margin;
        m_changeTracker.barSeriesMarginChanged = true;
        emitNeedRender();
        emit barSeriesMarginChanged(barSeriesMargin());
    }
}

QSizeF QQuickGraphsBars::barSeriesMargin() const
{
    return m_barSeriesMargin;
}

QList<QBar3DSeries *> QQuickGraphsBars::barSeriesList()
{
    QList<QBar3DSeries *> barSeriesList;
    for (QAbstract3DSeries *abstractSeries : m_seriesList) {
        QBar3DSeries *barSeries = qobject_cast<QBar3DSeries *>(abstractSeries);
        if (barSeries)
            barSeriesList.append(barSeries);
    }

    return barSeriesList;
}

QQmlListProperty<QBar3DSeries> QQuickGraphsBars::seriesList()
{
    return QQmlListProperty<QBar3DSeries>(this,
                                          this,
                                          &QQuickGraphsBars::appendSeriesFunc,
                                          &QQuickGraphsBars::countSeriesFunc,
                                          &QQuickGraphsBars::atSeriesFunc,
                                          &QQuickGraphsBars::clearSeriesFunc);
}

void QQuickGraphsBars::appendSeriesFunc(QQmlListProperty<QBar3DSeries> *list, QBar3DSeries *series)
{
    reinterpret_cast<QQuickGraphsBars *>(list->data)->addSeries(series);
}

qsizetype QQuickGraphsBars::countSeriesFunc(QQmlListProperty<QBar3DSeries> *list)
{
    return reinterpret_cast<QQuickGraphsBars *>(list->data)->barSeriesList().size();
}

QBar3DSeries *QQuickGraphsBars::atSeriesFunc(QQmlListProperty<QBar3DSeries> *list, qsizetype index)
{
    return reinterpret_cast<QQuickGraphsBars *>(list->data)->barSeriesList().at(index);
}

void QQuickGraphsBars::clearSeriesFunc(QQmlListProperty<QBar3DSeries> *list)
{
    QQuickGraphsBars *declBars = reinterpret_cast<QQuickGraphsBars *>(list->data);
    QList<QBar3DSeries *> realList = declBars->barSeriesList();
    int count = realList.size();
    for (int i = 0; i < count; i++)
        declBars->removeSeries(realList.at(i));
}

void QQuickGraphsBars::addSeries(QBar3DSeries *series)
{
    insertSeries(m_seriesList.size(), series);
    connectSeries(series);
    if (series->selectedBar() != invalidSelectionPosition())
        updateSelectedBar();
}

void QQuickGraphsBars::removeSeries(QBar3DSeries *series)
{
    if (!series)
        return;

    bool wasVisible = (series && series->d_func()->m_graph == this && series->isVisible());

    QQuickGraphsItem::removeSeriesInternal(series);

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

    removeBarModels();
    if (m_selectedBarSeries == series)
        resetClickedStatus();
    series->setParent(this); // Reparent as removing will leave series parentless
    disconnectSeries(series);
}

void QQuickGraphsBars::insertSeries(int index, QBar3DSeries *series)
{
    Q_ASSERT(series && series->type() == QAbstract3DSeries::SeriesType::Bar);

    int oldSize = m_seriesList.size();

    QQuickGraphsItem::insertSeries(index, series);

    if (oldSize != m_seriesList.size()) {
        QBar3DSeries *barSeries = static_cast<QBar3DSeries *>(series);
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

void QQuickGraphsBars::clearSelection()
{
    setSelectedBar(invalidSelectionPosition(), 0, false);
}

void QQuickGraphsBars::setPrimarySeries(QBar3DSeries *series)
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

QBar3DSeries *QQuickGraphsBars::primarySeries() const
{
    return m_primarySeries;
}

QBar3DSeries *QQuickGraphsBars::selectedSeries() const
{
    return m_selectedBarSeries;
}

void QQuickGraphsBars::setSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    if (mode.testFlag(QAbstract3DGraph::SelectionSlice)
        && (mode.testFlag(QAbstract3DGraph::SelectionRow)
            == mode.testFlag(QAbstract3DGraph::SelectionColumn))) {
        qWarning("Must specify one of either row or column selection mode"
                 "in conjunction with slicing mode.");
    } else {
        QAbstract3DGraph::SelectionFlags oldMode = selectionMode();

        QQuickGraphsItem::setSelectionMode(mode);

        if (mode != oldMode) {
            // Refresh selection upon mode change to ensure slicing is correctly
            // updated according to series the visibility.
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

void QQuickGraphsBars::handleAxisAutoAdjustRangeChangedInOrientation(
    QAbstract3DAxis::AxisOrientation orientation, bool autoAdjust)
{
    Q_UNUSED(orientation);
    Q_UNUSED(autoAdjust);
    adjustAxisRanges();
}

void QQuickGraphsBars::handleSeriesVisibilityChangedBySender(QObject *sender)
{
    QQuickGraphsItem::handleSeriesVisibilityChangedBySender(sender);

    // Visibility changes may require disabling slicing,
    // so just reset selection to ensure everything is still valid.
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
}

void QQuickGraphsBars::handleAxisRangeChangedBySender(QObject *sender)
{
    // Data window changed
    if (sender == m_axisX || sender == m_axisZ) {
        if (sender == m_axisX)
            handleDataColumnLabelsChanged();
        if (sender == m_axisZ)
            handleDataRowLabelsChanged();
    }

    QQuickGraphsItem::handleAxisRangeChangedBySender(sender);

    setDataDirty(true);

    // Update selected bar - may be moved offscreen
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
}

void QQuickGraphsBars::adjustAxisRanges()
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
                const QBar3DSeries *barSeries = static_cast<QBar3DSeries *>(m_seriesList.at(series));
                if (barSeries->isVisible()) {
                    const QBarDataProxy *proxy = barSeries->dataProxy();

                    if (adjustZ && proxy) {
                        int rowCount = proxy->rowCount();
                        if (rowCount)
                            rowCount--;

                        maxRowCount = qMax(maxRowCount, rowCount);
                    }

                    if (adjustX && proxy) {
                        const QBarDataArray &array = proxy->array();
                        int columnCount = 0;
                        for (int i = 0; i < array.size(); i++) {
                            if (columnCount < array.at(i).size())
                                columnCount = array.at(i).size();
                        }
                        if (columnCount)
                            columnCount--;

                        maxColumnCount = qMax(maxColumnCount, columnCount);
                    }
                }
            }
            // Call private implementations of setRange to avoid unsetting auto adjust
            // flag
            if (adjustZ)
                categoryAxisZ->d_func()->setRange(0.0f, float(maxRowCount), true);
            if (adjustX)
                categoryAxisX->d_func()->setRange(0.0f, float(maxColumnCount), true);
        }

        // Now that we know the row and column ranges, figure out the value axis
        // range
        if (adjustY) {
            for (int series = 0; series < seriesCount; series++) {
                const QBar3DSeries *barSeries = static_cast<QBar3DSeries *>(m_seriesList.at(series));
                if (barSeries->isVisible()) {
                    const QBarDataProxy *proxy = barSeries->dataProxy();
                    if (adjustY && proxy) {
                        QPair<float, float> limits = proxy->d_func()
                                                         ->limitValues(categoryAxisZ->min(),
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

void QQuickGraphsBars::setFloorLevel(float level)
{
    if (level != floorLevel()) {
        m_floorLevel = level;
        setDataDirty(true);
        m_changeTracker.floorLevelChanged = true;
        emitNeedRender();
        emit floorLevelChanged(level);
    }
}

float QQuickGraphsBars::floorLevel() const
{
    return m_floorLevel;
}

void QQuickGraphsBars::componentComplete()
{
    QQuickGraphsItem::componentComplete();

    auto wallBackground = background();
    QUrl wallUrl = QUrl(QStringLiteral("defaultMeshes/backgroundNoFloorMesh"));
    wallBackground->setSource(wallUrl);
    setBackground(wallBackground);

    QUrl floorUrl = QUrl(QStringLiteral(":/defaultMeshes/barMeshFull"));
    m_floorBackground = new QQuick3DModel();
    m_floorBackgroundScale = new QQuick3DNode();
    m_floorBackgroundRotation = new QQuick3DNode();

    m_floorBackgroundScale->setParent(rootNode());
    m_floorBackgroundScale->setParentItem(rootNode());

    m_floorBackgroundRotation->setParent(m_floorBackgroundScale);
    m_floorBackgroundRotation->setParentItem(m_floorBackgroundScale);

    m_floorBackground->setObjectName("Floor Background");
    m_floorBackground->setParent(m_floorBackgroundRotation);
    m_floorBackground->setParentItem(m_floorBackgroundRotation);

    m_floorBackground->setSource(floorUrl);

    setFloorGridInRange(true);
    setVerticalSegmentLine(false);

    QObject::connect(cameraTarget(),
                     &QQuick3DNode::rotationChanged,
                     this,
                     &QQuickGraphsBars::handleCameraRotationChanged);
}

void QQuickGraphsBars::synchData()
{
    if (!m_noZeroInRange) {
        setMinCameraYRotation(-90.0f);
        setMaxCameraYRotation(90.0f);
    } else {
        if ((m_hasNegativeValues && !valueAxis()->reversed())
            || (!m_hasNegativeValues && valueAxis()->reversed())) {
            setMinCameraYRotation(-90.0f);
            setMaxCameraYRotation(0.0f);
        } else {
            setMinCameraYRotation(0.0f);
            setMaxCameraYRotation(90.0f);
        }
    }
    if (m_changeTracker.barSpecsChanged || !m_cachedBarThickness.isValid()) {
        updateBarSpecs(m_barThicknessRatio, m_barSpacing, m_isBarSpecRelative);
        m_changeTracker.barSpecsChanged = false;
    }

    // Floor level update requires data update, so do before qquickgraphicsitem sync
    if (m_changeTracker.floorLevelChanged) {
        updateFloorLevel(m_floorLevel);
        m_changeTracker.floorLevelChanged = false;
    }

    if (m_changeTracker.multiSeriesScalingChanged) {
        m_keepSeriesUniform = m_isMultiSeriesUniform;
        setSeriesVisualsDirty(true);
        m_changeTracker.multiSeriesScalingChanged = false;
    }

    // Do not clear dirty flag, we need to react to it in qquickgraphicsitem as well
    if (theme()->d_func()->m_dirtyBits.backgroundEnabledDirty) {
        m_floorBackground->setVisible(theme()->isBackgroundEnabled());
        setSeriesVisualsDirty(true);
        for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++)
            it.key()->d_func()->m_changeTracker.meshChanged = true;
    }

    if (m_changeTracker.barSeriesMarginChanged) {
        updateBarSeriesMargin(barSeriesMargin());
        m_changeTracker.barSeriesMarginChanged = false;
    }

    if (m_axisRangeChanged) {
        theme()->d_func()->resetDirtyBits();
        m_axisRangeChanged = false;
    }

    QQuickGraphsItem::synchData();

    // Draw floor
    m_floorBackground->setPickable(false);
    auto min = qMin(scaleWithBackground().x(), scaleWithBackground().z());
    m_floorBackgroundScale->setScale(
        QVector3D(scaleWithBackground().x(), min * gridOffset(), scaleWithBackground().z()));
    m_floorBackgroundScale->setPosition(QVector3D(0.0f, -m_backgroundAdjustment, 0.0f));

    QQuaternion m_xRightAngleRotation(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 90.0f));
    QQuaternion m_xRightAngleRotationNeg(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -90.0f));

    if (isYFlipped())
        m_floorBackgroundRotation->setRotation(m_xRightAngleRotation);
    else
        m_floorBackgroundRotation->setRotation(m_xRightAngleRotationNeg);

    QQmlListReference materialsRefF(m_floorBackground, "materials");
    QQuick3DPrincipledMaterial *bgMatFloor;
    if (!materialsRefF.size()) {
        bgMatFloor = new QQuick3DPrincipledMaterial();
        bgMatFloor->setParent(m_floorBackground);
        bgMatFloor->setMetalness(0.f);
        bgMatFloor->setRoughness(.3f);
        bgMatFloor->setEmissiveFactor(QVector3D(.001f, .001f, .001f));
        materialsRefF.append(bgMatFloor);
    } else {
        bgMatFloor = static_cast<QQuick3DPrincipledMaterial *>(materialsRefF.at(0));
    }
    bgMatFloor->setBaseColor(theme()->backgroundColor());

    if (m_selectedBarPos.isNull())
        itemLabel()->setVisible(false);
}

void QQuickGraphsBars::updateParameters()
{
    m_minRow = m_axisZ->min();
    m_maxRow = m_axisZ->max();
    m_minCol = m_axisX->min();
    m_maxCol = m_axisX->max();
    m_newRows = m_maxRow - m_minRow + 1;
    m_newCols = m_maxCol - m_minCol + 1;

    if (m_cachedRowCount != m_newRows || m_cachedColumnCount != m_newCols) {
        m_changeTracker.selectedBarChanged = true;
        m_cachedColumnCount = m_newCols;
        m_cachedRowCount = m_newRows;

        // Calculate max scene size
        float sceneRatio = qMin(float(m_newCols) / float(m_newRows),
                                float(m_newRows) / float(m_newCols));
        m_maxSceneSize = 2.0f * qSqrt(sceneRatio * m_newCols * m_newRows);

        if (m_cachedBarThickness.isValid())
            calculateSceneScalingFactors();
    }
    m_axisRangeChanged = true;
    setDataDirty(true);
}

void QQuickGraphsBars::updateFloorLevel(float level)
{
    setFloorLevel(level);
    calculateHeightAdjustment();
}

void QQuickGraphsBars::updateGraph()
{
    QList<QBar3DSeries *> barSeriesAsList = barSeriesList();
    calculateSceneScalingFactors();

    for (const auto &series : std::as_const(barSeriesAsList)) {
        QAbstract3DSeriesChangeBitField &changeTracker = series->d_func()->m_changeTracker;
        if (changeTracker.meshChanged || changeTracker.meshSmoothChanged) {
            changeTracker.meshChanged = false;
            changeTracker.meshSmoothChanged = false;
            setDataDirty(true);
        }
    }

    if (isDataDirty()) {
        removeBarModels();
        generateBars(barSeriesAsList);
    }

    if (isSeriesVisualsDirty()) {
        int visualIndex = 0;
        for (const auto &barSeries : std::as_const(barSeriesAsList)) {
            if (barSeries->isVisible()) {
                updateBarVisuality(barSeries, visualIndex);
                updateBarPositions(barSeries);
                updateBarVisuals(barSeries);
                ++visualIndex;
            } else {
                updateBarVisuality(barSeries, -1);
            }
        }
    }

    // Needs to be done after data is set, as it needs to know the visual array.
    if (m_changeTracker.selectedBarChanged) {
        updateSelectedBar();
        if (isSliceEnabled()) {
            createSliceView();
            for (const auto &series : std::as_const(barSeriesAsList)) {
                bool visible = (m_selectedBarSeries == series) && series->isVisible();
                if (sliceView()->isVisible()) {
                    if (visible) {
                        removeSlicedBarModels();
                        createSliceView();
                        setSliceActivatedChanged(false);
                        updateSliceGraph();
                        break;
                    } else {
                        setSliceActivatedChanged(true);
                    }
                } else {
                    if (visible) {
                        m_selectionDirty = true;
                        setSliceActivatedChanged(true);
                    }
                }
            }
        }
        m_changeTracker.selectedBarChanged = false;
    }

    setDataDirty(false);
    setSeriesVisualsDirty(false);
}

void QQuickGraphsBars::updateAxisRange(float min, float max)
{
    QQuickGraphsItem::updateAxisRange(min, max);
    calculateHeightAdjustment();
}

void QQuickGraphsBars::updateAxisReversed(bool enable)
{
    Q_UNUSED(enable);
    setSeriesVisualsDirty(true);
    calculateHeightAdjustment();
}

void QQuickGraphsBars::updateLightStrength()
{
    for (const auto list : std::as_const(m_barModelsMap)) {
        for (auto barModel : *list) {
            QQmlListReference materialRef(barModel->model, "materials");
            if (materialRef.size()) {
                QQuick3DCustomMaterial *material = qobject_cast<QQuick3DCustomMaterial *>(
                    materialRef.at(0));
                material->setProperty("specularBrightness", theme()->lightStrength() * 0.05);
            }
        }
    }
}

void QQuickGraphsBars::calculateSceneScalingFactors()
{
    m_rowWidth = (m_cachedColumnCount * m_cachedBarSpacing.width()) * 0.5f;
    m_columnDepth = (m_cachedRowCount * m_cachedBarSpacing.height()) * 0.5f;
    m_maxDimension = qMax(m_rowWidth, m_columnDepth);
    m_scaleFactor = qMin((m_cachedColumnCount * (m_maxDimension / m_maxSceneSize)),
                         (m_cachedRowCount * (m_maxDimension / m_maxSceneSize)));

    // Single bar scaling
    m_xScale = m_cachedBarThickness.width() / m_scaleFactor;
    m_zScale = m_cachedBarThickness.height() / m_scaleFactor;

    // Adjust scaling according to margin
    m_xScale = m_xScale - m_xScale * m_cachedBarSeriesMargin.width();
    m_zScale = m_zScale - m_zScale * m_cachedBarSeriesMargin.height();

    // Whole graph scale factors
    m_xScaleFactor = m_rowWidth / m_scaleFactor;
    m_zScaleFactor = m_columnDepth / m_scaleFactor;

    if (m_requestedMargin < 0.0f) {
        m_hBackgroundMargin = 0.0f;
        m_vBackgroundMargin = 0.0f;
    } else {
        m_hBackgroundMargin = m_requestedMargin;
        m_vBackgroundMargin = m_requestedMargin;
    }

    auto scale = QVector3D(m_xScaleFactor, 1.0f, m_zScaleFactor);
    setScaleWithBackground(scale);
    setBackgroundScaleMargin({m_hBackgroundMargin, m_vBackgroundMargin, m_hBackgroundMargin});
    setScale(scale);
}

void QQuickGraphsBars::calculateHeightAdjustment()
{
    float newAdjustment = 1.0f;
    m_actualFloorLevel = qBound(valueAxis()->min(), floorLevel(), valueAxis()->max());
    float maxAbs = qFabs(valueAxis()->max() - m_actualFloorLevel);

    // Check if we have negative values
    if (valueAxis()->min() < m_actualFloorLevel)
        m_hasNegativeValues = true;
    else if (valueAxis()->min() >= m_actualFloorLevel)
        m_hasNegativeValues = false;

    if (valueAxis()->max() < m_actualFloorLevel) {
        m_heightNormalizer = float(qFabs(valueAxis()->min()) - qFabs(valueAxis()->max()));
        maxAbs = qFabs(valueAxis()->max()) - qFabs(valueAxis()->min());
    } else {
        m_heightNormalizer = float(valueAxis()->max() - valueAxis()->min());
    }

    // Height fractions are used in gradient calculations and are therefore
    // doubled Note that if max or min is exactly zero, we still consider it
    // outside the range
    if (valueAxis()->max() <= m_actualFloorLevel || valueAxis()->min() >= m_actualFloorLevel)
        m_noZeroInRange = true;
    else
        m_noZeroInRange = false;

    // Calculate translation adjustment for background floor
    newAdjustment = (qBound(0.0f, (maxAbs / m_heightNormalizer), 1.0f) - 0.5f) * 2.0f;
    if (valueAxis()->reversed())
        newAdjustment = -newAdjustment;

    if (newAdjustment != m_backgroundAdjustment)
        m_backgroundAdjustment = newAdjustment;
}

void QQuickGraphsBars::calculateSeriesStartPosition()
{
    m_seriesStart = -((float(m_visibleSeriesCount) - 1.0f) * 0.5f)
                    * (m_seriesStep - (m_seriesStep * m_cachedBarSeriesMargin.width()));
}

QVector3D QQuickGraphsBars::calculateCategoryLabelPosition(QAbstract3DAxis *axis,
                                                           QVector3D labelPosition,
                                                           int index)
{
    QVector3D ret = labelPosition;
    if (axis->orientation() == QAbstract3DAxis::AxisOrientation::X) {
        float xPos = (index + 0.5f) * m_cachedBarSpacing.width();
        ret.setX((xPos - m_rowWidth) / m_scaleFactor);
    }
    if (axis->orientation() == QAbstract3DAxis::AxisOrientation::Z) {
        float zPos = (index + 0.5f) * m_cachedBarSpacing.height();
        ret.setZ((m_columnDepth - zPos) / m_scaleFactor);
    }
    ret.setY(-m_backgroundAdjustment);
    return ret;
}

float QQuickGraphsBars::calculateCategoryGridLinePosition(QAbstract3DAxis *axis, int index)
{
    float ret = 0.0f;
    if (axis->orientation() == QAbstract3DAxis::AxisOrientation::Z) {
        float colPos = index * -(m_cachedBarSpacing.height() / m_scaleFactor);
        ret = colPos + scale().z();
    }
    if (axis->orientation() == QAbstract3DAxis::AxisOrientation::X) {
        float rowPos = index * (m_cachedBarSpacing.width() / m_scaleFactor);
        ret = rowPos - scale().x();
    }
    if (axis->orientation() == QAbstract3DAxis::AxisOrientation::Y)
        ret = -m_backgroundAdjustment;
    return ret;
}

void QQuickGraphsBars::handleAxisXChanged(QAbstract3DAxis *axis)
{
    emit columnAxisChanged(static_cast<QCategory3DAxis *>(axis));
}

void QQuickGraphsBars::handleAxisYChanged(QAbstract3DAxis *axis)
{
    emit valueAxisChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsBars::handleAxisZChanged(QAbstract3DAxis *axis)
{
    emit rowAxisChanged(static_cast<QCategory3DAxis *>(axis));
}

void QQuickGraphsBars::handleSeriesMeshChanged(QAbstract3DSeries::Mesh mesh)
{
    m_meshType = mesh;
}

void QQuickGraphsBars::handleMeshSmoothChanged(bool enable)
{
    m_smooth = enable;
}

void QQuickGraphsBars::handleCameraRotationChanged()
{
    updateLabels();
}

void QQuickGraphsBars::handleArrayReset()
{
    QBar3DSeries *series;
    if (qobject_cast<QBarDataProxy *>(sender()))
        series = static_cast<QBarDataProxy *>(sender())->series();
    else
        series = static_cast<QBar3DSeries *>(sender());

    if (series->isVisible()) {
        adjustAxisRanges();
        setDataDirty(true);
        series->d_func()->markItemLabelDirty();
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);
    // Clear selection unless still valid
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
    series->d_func()->markItemLabelDirty();
    emitNeedRender();
}

void QQuickGraphsBars::handleRowsAdded(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    if (series->isVisible()) {
        adjustAxisRanges();
        setDataDirty(true);
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);
    emitNeedRender();
}

void QQuickGraphsBars::handleRowsChanged(int startIndex, int count)
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

void QQuickGraphsBars::handleRowsRemoved(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);

    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    if (series == m_selectedBarSeries) {
        // If rows removed from selected series before the selection, adjust the
        // selection
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
        setDataDirty(true);
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);

    emitNeedRender();
}

void QQuickGraphsBars::handleRowsInserted(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();
    if (series == m_selectedBarSeries) {
        // If rows inserted to selected series before the selection, adjust the
        // selection
        int selectedRow = m_selectedBar.x();
        if (startIndex <= selectedRow) {
            selectedRow += count;
            setSelectedBar(QPoint(selectedRow, m_selectedBar.y()), m_selectedBarSeries, false);
        }
    }

    if (series->isVisible()) {
        adjustAxisRanges();
        setDataDirty(true);
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);

    emitNeedRender();
}

void QQuickGraphsBars::handleItemChanged(int rowIndex, int columnIndex)
{
    QBar3DSeries *series = static_cast<QBarDataProxy *>(sender())->series();

    bool newItem = true;
    QPoint candidate(rowIndex, columnIndex);
    for (ChangeItem item : m_changedItems) {
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

void QQuickGraphsBars::handleDataRowLabelsChanged()
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

    if (repeaterZ()) {
        updateParameters();
        repeaterZ()->setModel(m_axisZ->labels().size());
    }
}

void QQuickGraphsBars::handleDataColumnLabelsChanged()
{
    if (m_axisX) {
        // Grab a sublist equal to data window (no need to have more labels in axis)
        int min = int(m_axisX->min());
        int count = int(m_axisX->max()) - min + 1;
        QStringList subList;
        if (m_primarySeries && m_primarySeries->dataProxy()) {
            subList = static_cast<QBarDataProxy *>(m_primarySeries->dataProxy())
                          ->columnLabels()
                          .mid(min, count);
        }
        static_cast<QCategory3DAxis *>(m_axisX)->d_func()->setDataLabels(subList);
    }

    if (repeaterX()) {
        updateParameters();
        repeaterX()->setModel(m_axisX->labels().size());
    }
}

void QQuickGraphsBars::handleRowColorsChanged()
{
    setSeriesVisualsDirty(true);
    emitNeedRender();
}

void QQuickGraphsBars::connectSeries(QBar3DSeries *series)
{
    m_meshType = series->mesh();
    m_smooth = series->isMeshSmooth();

    QObject::connect(series,
                     &QBar3DSeries::meshChanged,
                     this,
                     &QQuickGraphsBars::handleSeriesMeshChanged);
    QObject::connect(series,
                     &QBar3DSeries::meshSmoothChanged,
                     this,
                     &QQuickGraphsBars::handleMeshSmoothChanged);
    QObject::connect(series,
                     &QBar3DSeries::rowColorsChanged,
                     this,
                     &QQuickGraphsBars::handleRowColorsChanged);
}

void QQuickGraphsBars::disconnectSeries(QBar3DSeries *series)
{
    QObject::disconnect(series, 0, this, 0);
}

void QQuickGraphsBars::generateBars(QList<QBar3DSeries *> &barSeriesList)
{
    m_visibleSeriesCount = 0;
    for (const auto &barSeries : std::as_const(barSeriesList)) {
        QQuick3DTexture *texture = createTexture();
        texture->setParent(this);
        auto gradient = barSeries->baseGradient();
        auto textureData = static_cast<QQuickGraphsTextureData *>(texture->textureData());
        textureData->createGradient(gradient);

        bool visible = barSeries->isVisible();

        QList<BarModel *> *barList = m_barModelsMap.value(barSeries);

        if (!barList) {
            barList = new QList<BarModel *>;
            m_barModelsMap[barSeries] = barList;
        }

        if (barList->isEmpty()) {
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                QBarDataProxy *dataProxy = barSeries->dataProxy();
                int dataRowIndex = m_minRow;
                int newRowSize = qMin(dataProxy->rowCount() - dataRowIndex, m_newRows);

                for (int row = 0; row < newRowSize; ++row) {
                    const QBarDataRow &dataRow = dataProxy->rowAt(dataRowIndex);
                    if (!dataRow.isEmpty()) {
                        int dataColIndex = m_minCol;
                        int newColSize = qMin(dataRow.size() - dataColIndex, m_newCols);
                        for (int col = 0; col < newColSize; ++col) {
                            QBarDataItem &dataItem = const_cast<QBarDataItem &>(
                                dataRow.at(dataColIndex));
                            auto scene = QQuick3DViewport::scene();
                            QQuick3DModel *model = createDataItem(scene, barSeries);
                            model->setVisible(visible);

                            BarModel *barModel = new BarModel();
                            barModel->model = model;
                            barModel->barItem = &dataItem;
                            barModel->coord = QPoint(dataRowIndex, col);
                            barModel->texture = texture;

                            if (!barList->contains(barModel))
                                barList->append(barModel);

                            ++dataColIndex;
                        }
                        ++dataRowIndex;
                    }
                }
            } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
                auto scene = QQuick3DViewport::scene();
                BarModel *barInstancing = new BarModel();
                barInstancing->texture = texture;

                if (barInstancing->instancing == nullptr) {
                    barInstancing->instancing = new BarInstancing;
                    barInstancing->instancing->setParent(barSeries);
                    barInstancing->selectionInstancing = new BarInstancing;
                    barInstancing->selectionInstancing->setParent(barSeries);
                    barInstancing->multiSelectionInstancing = new BarInstancing;
                    barInstancing->multiSelectionInstancing->setParent(barSeries);
                }

                if (barInstancing->model == nullptr) {
                    barInstancing->model = createDataItem(scene, barSeries);
                    barInstancing->model->setInstancing(barInstancing->instancing);
                    barInstancing->model->setVisible(visible);
                    barInstancing->model->setPickable(true);
                    barInstancing->selectedModel = createDataItem(scene, barSeries);
                    barInstancing->selectedModel->setInstancing(barInstancing->selectionInstancing);
                    barInstancing->selectedModel->setVisible(visible);
                    barInstancing->selectedModel->setPickable(true);
                    barInstancing->multiSelectedModel = createDataItem(scene, barSeries);
                    barInstancing->multiSelectedModel->setInstancing(
                        barInstancing->multiSelectionInstancing);
                    barInstancing->multiSelectedModel->setVisible(visible);
                    barInstancing->multiSelectedModel->setPickable(true);
                }

                if (!barList->contains(barInstancing))
                    barList->append(barInstancing);
            }
        }

        if (barSeries->isVisible())
            m_visibleSeriesCount++;
    }
}

QQuick3DModel *QQuickGraphsBars::createDataItem(QQuick3DNode *scene, QAbstract3DSeries *series)
{
    auto model = new QQuick3DModel();
    model->setParent(scene);
    model->setParentItem(scene);
    model->setObjectName(QStringLiteral("BarModel"));
    QString fileName = getMeshFileName();
    if (fileName.isEmpty())
        fileName = series->userDefinedMesh();

    model->setSource(QUrl(fileName));
    return model;
}

QString QQuickGraphsBars::getMeshFileName()
{
    QString fileName = {};
    switch (m_meshType) {
    case QAbstract3DSeries::Mesh::Sphere:
        fileName = QStringLiteral("defaultMeshes/sphereMesh");
        break;
    case QAbstract3DSeries::Mesh::Bar:
    case QAbstract3DSeries::Mesh::Cube:
        fileName = QStringLiteral("defaultMeshes/barMesh");
        break;
    case QAbstract3DSeries::Mesh::Pyramid:
        fileName = QStringLiteral("defaultMeshes/pyramidMesh");
        break;
    case QAbstract3DSeries::Mesh::Cone:
        fileName = QStringLiteral("defaultMeshes/coneMesh");
        break;
    case QAbstract3DSeries::Mesh::Cylinder:
        fileName = QStringLiteral("defaultMeshes/cylinderMesh");
        break;
    case QAbstract3DSeries::Mesh::BevelBar:
    case QAbstract3DSeries::Mesh::BevelCube:
        fileName = QStringLiteral("defaultMeshes/bevelBarMesh");
        break;
    case QAbstract3DSeries::Mesh::UserDefined:
        break;
    default:
        fileName = QStringLiteral("defaultMeshes/sphereMesh");
    }

    fixMeshFileName(fileName, m_meshType);

    return fileName;
}

void QQuickGraphsBars::fixMeshFileName(QString &fileName, QAbstract3DSeries::Mesh meshType)
{
    // Should it be smooth?
    if (m_smooth && meshType != QAbstract3DSeries::Mesh::Point
        && meshType != QAbstract3DSeries::Mesh::UserDefined) {
        fileName += QStringLiteral("Smooth");
    }

    // Should it be filled?
    if (!theme()->isBackgroundEnabled() && meshType != QAbstract3DSeries::Mesh::Sphere
        && meshType != QAbstract3DSeries::Mesh::Point
        && meshType != QAbstract3DSeries::Mesh::UserDefined) {
        fileName.append(QStringLiteral("Full"));
    }
}

void QQuickGraphsBars::updateBarVisuality(QBar3DSeries *series, int visualIndex)
{
    QList<BarModel *> barList = *m_barModelsMap.value(series);
    for (int i = 0; i < barList.count(); i++) {
        barList.at(i)->visualIndex = visualIndex;
        barList.at(i)->model->setVisible(series->isVisible());
        if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
            barList.at(0)->selectedModel->setVisible(false);
            barList.at(0)->multiSelectedModel->setVisible(false);
        }
    }

    m_changeTracker.selectedBarChanged = true;
    itemLabel()->setVisible(false);
}

void QQuickGraphsBars::updateBarPositions(QBar3DSeries *series)
{
    QBarDataProxy *dataProxy = series->dataProxy();

    m_seriesScaleX = 1.0f / float(m_visibleSeriesCount);
    m_seriesStep = 1.0f / float(m_visibleSeriesCount);
    m_seriesStart = -((float(m_visibleSeriesCount) - 1.0f) * 0.5f)
                    * (m_seriesStep - (m_seriesStep * m_cachedBarSeriesMargin.width()));

    if (m_keepSeriesUniform)
        m_seriesScaleZ = m_seriesScaleX;
    else
        m_seriesScaleZ = 1.0f;

    m_zeroPosition = valueAxis()->positionAt(m_actualFloorLevel);

    QList<BarModel *> barList = *m_barModelsMap.value(series);

    int dataRowIndex = m_minRow;
    int newRowSize = qMin(dataProxy->rowCount() - dataRowIndex, m_newRows);
    int row = 0;
    int dataColIndex = m_minCol;
    int newColSize = qMin(dataProxy->colCount() - dataColIndex, m_newCols);
    int col = 0;
    for (int i = 0; i < barList.count(); i++) {
        float seriesPos = m_seriesStart + 0.5f
                          + (m_seriesStep
                             * (barList.at(i)->visualIndex
                                - (barList.at(i)->visualIndex * m_cachedBarSeriesMargin.width())));
        if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
            QBarDataItem *item = barList.at(i)->barItem;
            QQuick3DModel *model = barList.at(i)->model;
            float heightValue = updateBarHeightParameters(item);
            float angle = item->rotation();

            if (angle)
                model->setRotation(QQuaternion::fromAxisAndAngle(upVector, angle));
            else
                model->setRotation(QQuaternion());

            if (heightValue < 0.f) {
                const QVector3D rot = model->eulerRotation();
                model->setEulerRotation(QVector3D(-180.f, rot.y(), rot.z()));
            }

            float colPos = (col + seriesPos) * m_cachedBarSpacing.width();
            float xPos = (colPos - m_rowWidth) / m_scaleFactor;
            float rowPos = (row + 0.5f) * (m_cachedBarSpacing.height());
            float zPos = (m_columnDepth - rowPos) / m_scaleFactor;
            float yPos;

            if (heightValue < 0.f)
                yPos = heightValue - m_backgroundAdjustment - 0.015f;
            else
                yPos = heightValue - m_backgroundAdjustment + 0.015f;

            barList.at(i)->heightValue = heightValue;
            model->setPosition(QVector3D(xPos, yPos, zPos));

            if (heightValue == 0) {
                model->setScale(QVector3D(.0f, .0f, .0f));
                model->setPickable(false);
            } else {
                model->setScale(QVector3D(m_xScale * m_seriesScaleX,
                                          qAbs(heightValue),
                                          m_zScale * m_seriesScaleZ));
                model->setPickable(true);
            }

            if (col < newColSize - 1) {
                ++col;
            } else {
                col = 0;
                if (row < newRowSize - 1)
                    ++row;
                else
                    row = 0;
            }
        } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
            deleteBarItemHolders(barList.at(i)->instancing);
            QList<BarItemHolder *> positions;
            for (int row = 0; row < newRowSize; ++row) {
                const QBarDataRow &dataRow = dataProxy->rowAt(dataRowIndex);
                if (!dataRow.isEmpty()) {
                    dataColIndex = m_minCol;
                    newColSize = qMin(dataRow.size() - dataColIndex, m_newCols);
                    for (int col = 0; col < newColSize; col++) {
                        const QBarDataItem &item = dataRow.at(dataColIndex);
                        float heightValue = updateBarHeightParameters(&item);
                        BarItemHolder *bih = new BarItemHolder();

                        if (heightValue < 0.f) {
                            const QVector3D eulerRot = barList.at(i)->model->eulerRotation();
                            bih->eulerRotation = QVector3D(-180.f, eulerRot.y(), eulerRot.z());
                        }

                        float colPos = (col + seriesPos) * m_cachedBarSpacing.width();
                        float xPos = (colPos - m_rowWidth) / m_scaleFactor;
                        float rowPos = (row + 0.5f) * (m_cachedBarSpacing.height());
                        float zPos = (m_columnDepth - rowPos) / m_scaleFactor;
                        float yPos;

                        if (heightValue < 0.f)
                            yPos = heightValue - m_backgroundAdjustment - 0.015f;
                        else
                            yPos = heightValue - m_backgroundAdjustment + 0.015f;

                        bih->position = {xPos, yPos, zPos};
                        bih->coord = QPoint(row, col);

                        if (heightValue == 0) {
                            bih->scale = {.0f, .0f, .0f};
                        } else {
                            bih->scale = {m_xScale * m_seriesScaleX,
                                          qAbs(heightValue),
                                          m_zScale * m_seriesScaleZ};
                        }

                        bih->heightValue = heightValue;
                        bih->selectedBar = false;

                        bool colorStyleIsUniform = (series->colorStyle()
                                                    == Q3DTheme::ColorStyle::Uniform);
                        if (colorStyleIsUniform) {
                            QList<QColor> rowColors = series->rowColors();
                            if (rowColors.size() == 0) {
                                bih->color = series->baseColor();
                            } else {
                                int rowColorIndex = bih->coord.x() % rowColors.size();
                                bih->color = rowColors[rowColorIndex];
                            }
                        }

                        positions.push_back(bih);
                        dataColIndex++;
                    }
                }
                dataRowIndex++;
            }
            barList.at(i)->instancing->setDataArray(positions);
        }
    }
}

float QQuickGraphsBars::updateBarHeightParameters(const QBarDataItem *item)
{
    float value = item->value();
    float heightValue = valueAxis()->positionAt(value);

    if (m_noZeroInRange) {
        if (m_hasNegativeValues) {
            heightValue = -1.0f + heightValue;
            if (heightValue > 0.0f)
                heightValue = 0.0f;
        } else {
            if (heightValue < 0.0f)
                heightValue = 0.0f;
        }
    } else {
        heightValue -= m_zeroPosition;
    }

    if (valueAxis()->reversed())
        heightValue = -heightValue;

    return heightValue;
}

void QQuickGraphsBars::updateBarVisuals(QBar3DSeries *series)
{
    QList<BarModel *> barList = *m_barModelsMap.value(series);
    bool useGradient = series->d_func()->isUsingGradient();

    if (useGradient) {
        if (!m_hasHighlightTexture) {
            m_highlightTexture = createTexture();
            m_highlightTexture->setParent(this);
            m_multiHighlightTexture = createTexture();
            m_multiHighlightTexture->setParent(this);
            m_hasHighlightTexture = true;
        }
        auto highlightGradient = series->singleHighlightGradient();
        auto highlightTextureData = static_cast<QQuickGraphsTextureData *>(
            m_highlightTexture->textureData());
        highlightTextureData->createGradient(highlightGradient);
        auto multiHighlightGradient = series->multiHighlightGradient();
        auto multiHighlightTextureData = static_cast<QQuickGraphsTextureData *>(
            m_multiHighlightTexture->textureData());
        multiHighlightTextureData->createGradient(multiHighlightGradient);
    } else {
        if (m_hasHighlightTexture) {
            m_highlightTexture->deleteLater();
            m_multiHighlightTexture->deleteLater();
            m_hasHighlightTexture = false;
        }
    }

    bool rangeGradient = (useGradient
                          && series->d_func()->m_colorStyle == Q3DTheme::ColorStyle::RangeGradient);
    QColor baseColor = series->baseColor();
    QColor barColor;
    QLinearGradient gradient = series->baseGradient();

    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
        // Release resources that might not have been deleted even though deleteLater had been set
        window()->releaseResources();

        for (int i = 0; i < barList.count(); i++) {
            QQuick3DModel *model = barList.at(i)->model;
            auto textureData = static_cast<QQuickGraphsTextureData *>(
                barList.at(i)->texture->textureData());
            textureData->createGradient(series->baseGradient());
            updateItemMaterial(model,
                               useGradient,
                               rangeGradient,
                               QStringLiteral(":/materials/BarsMaterial"));
            if (useGradient) {
                updateMaterialProperties(model,
                                         false,
                                         false,
                                         barList.at(i)->texture,
                                         QColor(Qt::white));
            } else {
                QList<QColor> rowColors = series->rowColors();
                if (rowColors.size() == 0) {
                    barColor = baseColor;
                } else {
                    int rowColorIndex = barList.at(i)->coord.x() % rowColors.size();
                    barColor = rowColors[rowColorIndex];
                }
                updateMaterialProperties(model, false, false, barList.at(i)->texture, barColor);
            }
        }
    } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
        for (int i = 0; i < barList.count(); i++) {
            barList.at(i)->instancing->setRangeGradient(rangeGradient);
            auto textureData = static_cast<QQuickGraphsTextureData *>(
                barList.at(i)->texture->textureData());
            textureData->createGradient(series->baseGradient());
            updateItemMaterial(barList.at(i)->model,
                               useGradient,
                               rangeGradient,
                               QStringLiteral(":/materials/BarsMaterialInstancing"));
            updateMaterialProperties(barList.at(i)->model,
                                     false,
                                     false,
                                     barList.at(i)->texture,
                                     QColor(Qt::white));
        }
    }
}

void QQuickGraphsBars::updateItemMaterial(QQuick3DModel *item,
                                          bool useGradient,
                                          bool rangeGradient,
                                          const QString &materialName)
{
    QQmlListReference materialsRef(item, "materials");

    bool needNewMaterial = false;
    if (!materialsRef.size())
        needNewMaterial = true;
    else if (materialsRef.at(0)->objectName().contains(QStringLiteral("Instancing"))
             == materialName.contains(QStringLiteral("Instancing"))) {
        needNewMaterial = true;
    }

    if (needNewMaterial) {
        materialsRef.clear();
        auto *material = createQmlCustomMaterial(materialName);
        material->setObjectName(materialName);
        material->setParent(item);
        materialsRef.append(material);
    }
    int colorStyle;
    if (!useGradient)
        colorStyle = 0; // style is uniform
    else if (!rangeGradient)
        colorStyle = 1; // style is objectGradient
    else
        colorStyle = 2; // style is rangeGradient

    auto *material = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
    material->setProperty("colorStyle", colorStyle);
}

void QQuickGraphsBars::updateMaterialProperties(QQuick3DModel *item,
                                                bool isHighlight,
                                                bool isMultiHighlight,
                                                QQuick3DTexture *texture,
                                                const QColor &color)
{
    QQmlListReference materialsRef(item, "materials");
    auto customMaterial = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
    if (!customMaterial)
        return;
    QVariant textureInputAsVariant = customMaterial->property("custex");
    QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant
                                                        .value<QQuick3DShaderUtilsTextureInput *>();

    int colorStyle = customMaterial->property("colorStyle").value<int>();
    if (colorStyle == 0) {
        customMaterial->setProperty("uniformColor", color);
    } else {
        if (!isHighlight && !isMultiHighlight)
            textureInput->setTexture(texture);
        else
            textureInput->setTexture(isHighlight ? m_highlightTexture : m_multiHighlightTexture);
        customMaterial->setProperty("isHighlight", isHighlight || isMultiHighlight);
    }
    customMaterial->setProperty("specularBrightness", theme()->lightStrength() * 0.05);
}

void QQuickGraphsBars::removeBarModels()
{
    for (const auto list : std::as_const(m_barModelsMap)) {
        for (auto barModel : *list) {
            deleteBarModels(barModel->model);
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
                deleteBarItemHolders(barModel->instancing);
                deleteBarItemHolders(barModel->selectionInstancing);
                deleteBarItemHolders(barModel->multiSelectionInstancing);
                deleteBarModels(barModel->selectedModel);
                deleteBarModels(barModel->multiSelectedModel);
            }
            delete barModel;
        }
        delete list;
    }

    m_barModelsMap.clear();
    setSelectedBar(m_selectedBar, m_selectedBarSeries, false);
}

void QQuickGraphsBars::deleteBarModels(QQuick3DModel *model)
{
    model->setPickable(false);
    model->setVisible(false);
    QQmlListReference materialsRef(model, "materials");
    if (materialsRef.size()) {
        auto material = materialsRef.at(0);
        delete material;
    }
    delete model;
}

void QQuickGraphsBars::deleteBarItemHolders(BarInstancing *instancing)
{
    const QList<BarItemHolder *> barItemList = instancing->dataArray();
    for (const auto bih : barItemList)
        delete bih;
    instancing->clearDataArray();
}

QQuick3DTexture *QQuickGraphsBars::createTexture()
{
    QQuick3DTexture *texture = new QQuick3DTexture();
    texture->setParent(this);
    texture->setRotationUV(-90.0f);
    texture->setHorizontalTiling(QQuick3DTexture::ClampToEdge);
    texture->setVerticalTiling(QQuick3DTexture::ClampToEdge);
    QQuickGraphsTextureData *textureData = new QQuickGraphsTextureData();
    textureData->setParent(texture);
    textureData->setParentItem(texture);
    texture->setTextureData(textureData);

    return texture;
}

bool QQuickGraphsBars::handleMousePressedEvent(QMouseEvent *event)
{
    if (!QQuickGraphsItem::handleMousePressedEvent(event))
        return true;

    createSliceView();

    if (Qt::LeftButton == event->button())
        doPicking(event->pos());

    return true;
}

bool QQuickGraphsBars::handleTouchEvent(QTouchEvent *event)
{
    if (!QQuickGraphsItem::handleTouchEvent(event))
        return true;

    createSliceView();

    if (scene()->selectionQueryPosition() != scene()->invalidSelectionPoint()
        && !event->isUpdateEvent()) {
        doPicking(event->point(0).position());
        scene()->setSelectionQueryPosition(scene()->invalidSelectionPoint());
    }
    return true;
}

bool QQuickGraphsBars::doPicking(const QPointF &position)
{
    if (!QQuickGraphsItem::doPicking(position))
        return false;

    m_selectionDirty = true;
    QList<QQuick3DPickResult> pickResults = pickAll(position.x(), position.y());
    QQuick3DModel *selectedModel = nullptr;
    QVector3D instancePos = {.0f, .0f, .0f};
    if (!selectionMode().testFlag(QAbstract3DGraph::SelectionNone)) {
        if (!pickResults.isEmpty()) {
            for (const auto &picked : std::as_const(pickResults)) {
                if (const auto &hit = picked.objectHit()) {
                    if (hit == backgroundBB() || hit == background()) {
                        resetClickedStatus();
                        continue;
                    } else if (hit->objectName().contains(QStringLiteral("BarModel"))) {
                        if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                            selectedModel = hit;
                            for (const auto barlist : std::as_const(m_barModelsMap)) {
                                for (const auto barModel : *barlist) {
                                    if (barModel->model == selectedModel) {
                                        setSelectedBar(barModel->coord,
                                                       m_barModelsMap.key(barlist),
                                                       false);
                                    }
                                }
                            }
                            break;
                        } else if (optimizationHint()
                                   == QAbstract3DGraph::OptimizationHint::Default) {
                            BarInstancing *barIns = static_cast<BarInstancing *>(hit->instancing());
                            // Prevents to select bars with a height of 0 which affect picking.
                            if (!barIns->dataArray().isEmpty()
                                && barIns->dataArray().at(picked.instanceIndex())->heightValue
                                       != 0) {
                                selectedModel = hit;
                                instancePos = selectedModel->instancing()->instancePosition(
                                    picked.instanceIndex());
                                for (const auto barlist : std::as_const(m_barModelsMap)) {
                                    for (const auto barModel : *barlist) {
                                        QList<BarItemHolder *> barItemList = barModel->instancing
                                                                                 ->dataArray();
                                        for (const auto bih : barItemList) {
                                            if (bih->position == instancePos) {
                                                setSelectedBar(bih->coord,
                                                               m_barModelsMap.key(barlist),
                                                               false);
                                                if (isSliceEnabled())
                                                    setSliceActivatedChanged(true);
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    } else if (hit->objectName().contains(QStringLiteral("ElementAxis"))) {
                        QPoint coord = invalidSelectionPosition();
                        if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn)
                            && selectedAxis() == axisX()) {
                            // Use row from previous selection in case of row + column mode
                            int previousRow = qMax(0, m_selectedBar.x());
                            coord = QPoint(previousRow, selectedLabelIndex());
                        } else if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow)
                                   && selectedAxis() == axisZ()) {
                            // Use column from previous selection in case of row + column mode
                            int previousCol = qMax(0, m_selectedBar.y());
                            coord = QPoint(selectedLabelIndex(), previousCol);
                        }
                        for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++) {
                            if (it.key()->isVisible())
                                setSelectedBar(coord, it.key(), false);
                        }
                        break;
                    }
                }
            }
        } else {
            resetClickedStatus();
        }
    }
    return true;
}

QAbstract3DAxis *QQuickGraphsBars::createDefaultAxis(QAbstract3DAxis::AxisOrientation orientation)
{
    QAbstract3DAxis *defaultAxis = 0;

    if (orientation == QAbstract3DAxis::AxisOrientation::Y)
        defaultAxis = createDefaultValueAxis();
    else
        defaultAxis = createDefaultCategoryAxis();

    return defaultAxis;
}

void QQuickGraphsBars::adjustSelectionPosition(QPoint &pos, const QBar3DSeries *series)
{
    const QBarDataProxy *proxy = 0;
    if (series)
        proxy = series->dataProxy();

    if (!proxy)
        pos = invalidSelectionPosition();

    if (pos != invalidSelectionPosition()) {
        int maxRow = proxy->rowCount() - 1;
        int maxCol = (pos.x() <= maxRow && pos.x() >= 0 && !proxy->rowAt(pos.x()).isEmpty())
                         ? proxy->rowAt(pos.x()).size() - 1
                         : -1;

        if (pos.x() < 0 || pos.x() > maxRow || pos.y() < 0 || pos.y() > maxCol)
            pos = invalidSelectionPosition();
    }
}

void QQuickGraphsBars::setSelectedBar(const QPoint &coord, QBar3DSeries *series, bool enterSlice)
{
    // If the selection targets non-existent bar, clear selection instead.
    QPoint pos = coord;

    // Series may already have been removed, so check it before setting the
    // selection.
    if (!m_seriesList.contains(series))
        series = nullptr;

    adjustSelectionPosition(pos, series);

    if (series && selectionMode().testFlag(QAbstract3DGraph::SelectionSlice)) {
        // If the selected bar is outside data window, or there is no visible
        // selected bar, disable slicing.
        if (pos.x() < m_axisZ->min() || pos.x() > m_axisZ->max() || pos.y() < m_axisX->min()
            || pos.y() > m_axisX->max() || !series->isVisible()) {
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
        checkSliceEnabled();

        // Clear selection from other series and finally set new selection to the
        // specified series
        for (QAbstract3DSeries *otherSeries : m_seriesList) {
            QBar3DSeries *barSeries = static_cast<QBar3DSeries *>(otherSeries);
            if (barSeries != m_selectedBarSeries)
                barSeries->d_func()->setSelectedBar(invalidSelectionPosition());
        }
        if (m_selectedBarSeries)
            m_selectedBarSeries->d_func()->setSelectedBar(m_selectedBar);

        if (seriesChanged)
            emit selectedSeriesChanged(m_selectedBarSeries);

        if (pos == invalidSelectionPosition())
            resetClickedStatus();

        setSeriesVisualsDirty(true);
        emitNeedRender();
    }
}

void QQuickGraphsBars::updateSelectedBar()
{
    for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++) {
        if (m_selectedBarSeries && it.key()->isVisible()) {
            QString label = m_selectedBarSeries->itemLabel();
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                for (auto barList : *it.value()) {
                    QQuickGraphsBars::SelectionType selectionType = isSelected(barList->coord.x(),
                                                                               barList->coord.y(),
                                                                               it.key());
                    switch (selectionType) {
                    case QQuickGraphsBars::SelectionItem: {
                        updateMaterialProperties(barList->model,
                                                 true,
                                                 false,
                                                 barList->texture,
                                                 it.key()->singleHighlightColor());

                        m_selectedBarPos = barList->model->position();
                        QString label = (m_selectedBarSeries->d_func()->itemLabel());

                        if (barList->heightValue >= 0.0f) {
                            m_selectedBarPos.setY(m_selectedBarPos.y() + barList->heightValue
                                                  + 0.2f);
                        } else {
                            m_selectedBarPos.setY(m_selectedBarPos.y() + barList->heightValue
                                                  - 0.2f);
                        }

                        updateItemLabel(m_selectedBarPos);
                        itemLabel()->setVisible(theme()->isLabelsEnabled());
                        itemLabel()->setProperty("labelText", label);
                        if (isSliceEnabled())
                            updateSliceItemLabel(label, m_selectedBarPos);
                        break;
                    }
                    case QQuickGraphsBars::SelectionRow:
                    case QQuickGraphsBars::SelectionColumn: {
                        updateMaterialProperties(barList->model,
                                                 false,
                                                 true,
                                                 barList->texture,
                                                 it.key()->multiHighlightColor());
                        break;
                    }
                    default:
                        break;
                    }
                }
            } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
                QList<BarModel *> barList = *m_barModelsMap.value(it.key());
                deleteBarItemHolders(barList.at(0)->selectionInstancing);
                deleteBarItemHolders(barList.at(0)->multiSelectionInstancing);
                createBarItemHolders(it.key(), barList, false);
            }
        }
    }
}

QQuickGraphsItem::SelectionType QQuickGraphsBars::isSelected(int row, int bar, QBar3DSeries *series)
{
    QQuickGraphsBars::SelectionType isSelectedType = QQuickGraphsBars::SelectionNone;
    if ((selectionMode().testFlag(QAbstract3DGraph::SelectionMultiSeries) && m_selectedBarSeries)
        || series == m_selectedBarSeries) {
        if (row == m_selectedBar.x() && bar == m_selectedBar.y()
            && (selectionMode().testFlag(QAbstract3DGraph::SelectionItem))) {
            isSelectedType = QQuickGraphsBars::SelectionItem;
        } else if (row == m_selectedBar.x()
                   && (selectionMode().testFlag(QAbstract3DGraph::SelectionRow))) {
            isSelectedType = QQuickGraphsBars::SelectionRow;
        } else if (bar == m_selectedBar.y()
                   && (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))) {
            isSelectedType = QQuickGraphsBars::SelectionColumn;
        }
    }

    return isSelectedType;
}

void QQuickGraphsBars::updateSliceItemLabel(QString label, const QVector3D &position)
{
    QQuickGraphsItem::updateSliceItemLabel(label, position);

    QFontMetrics fm(theme()->font());
    float textPadding = theme()->font().pointSizeF() * .7f;
    float labelHeight = fm.height() + textPadding;
    float labelWidth = fm.horizontalAdvance(label) + textPadding;
    sliceItemLabel()->setProperty("labelWidth", labelWidth);
    sliceItemLabel()->setProperty("labelHeight", labelHeight);
    QVector3D slicePos = position;
    if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
        slicePos.setX(slicePos.z());
    else if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
        slicePos.setX(slicePos.x());
    QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axisY());
    if (valueAxis->reversed())
        slicePos.setY(slicePos.y() - (textPadding * .06f));
    else
        slicePos.setY(slicePos.y() + (textPadding * .06f));
    slicePos.setZ(.1f);
    sliceItemLabel()->setPosition(slicePos);
    sliceItemLabel()->setProperty("labelText", label);
    sliceItemLabel()->setEulerRotation(QVector3D(0.0f, 0.0f, 90.0f));
    sliceItemLabel()->setVisible(theme()->isLabelsEnabled());
}

void QQuickGraphsBars::resetClickedStatus()
{
    m_selectedBarPos = QVector3D(0.0f, 0.0f, 0.0f);
    m_selectedBar = QQuickGraphsBars::invalidSelectionPosition();
    m_selectedBarSeries = 0;
    clearSelection();

    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
        for (const auto barList : std::as_const(m_barModelsMap)) {
            QList<BarItemHolder *> barItemList = barList->at(0)->instancing->dataArray();
            for (auto bih : barItemList)
                bih->selectedBar = false;
        }
    }

    if (sliceView() && sliceView()->isVisible()) {
        setSliceActivatedChanged(true);
        m_selectionDirty = true;
    }
    setSeriesVisualsDirty(true);
}

void QQuickGraphsBars::createSliceView()
{
    setSliceOrthoProjection(false);
    QQuickGraphsItem::createSliceView();

    QList<QBar3DSeries *> barSeries = barSeriesList();
    for (const auto &barSeries : std::as_const(barSeries)) {
        QList<BarModel *> *slicedBarList = m_slicedBarModels.value(barSeries);
        if (!slicedBarList) {
            slicedBarList = new QList<BarModel *>;
            m_slicedBarModels[barSeries] = slicedBarList;
        }
        if (slicedBarList->isEmpty()) {
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                int dataRowIndex = m_minRow;
                int newRowSize = qMin(barSeries->dataProxy()->rowCount() - dataRowIndex, m_newRows);
                int newColSize = 0;
                if (newRowSize) {
                    const QBarDataRow *dataRow = &barSeries->dataProxy()->rowAt(dataRowIndex);
                    if (dataRow) {
                        int dataColIndex = m_minCol;
                        newColSize = qMin(dataRow->size() - dataColIndex, m_newCols);
                    }
                }
                int slicedBarListSize = 0;

                if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
                    slicedBarListSize = newColSize;
                else if (selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
                    slicedBarListSize = newRowSize;

                for (int ind = 0; ind < slicedBarListSize; ++ind) {
                    QQuick3DModel *model = createDataItem(sliceView()->scene(), barSeries);
                    model->setVisible(false);
                    BarModel *barModel = new BarModel();
                    barModel->model = model;

                    if (!slicedBarList->contains(barModel))
                        slicedBarList->append(barModel);
                }
            } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
                BarModel *barInstancing = new BarModel();

                if (barInstancing->selectionInstancing == nullptr) {
                    barInstancing->selectionInstancing = new BarInstancing;
                    barInstancing->selectionInstancing->setParent(barSeries);
                    barInstancing->multiSelectionInstancing = new BarInstancing;
                    barInstancing->multiSelectionInstancing->setParent(barSeries);
                }

                if (barInstancing->selectedModel == nullptr) {
                    barInstancing->selectedModel = createDataItem(sliceView()->scene(), barSeries);
                    barInstancing->selectedModel->setInstancing(barInstancing->selectionInstancing);
                    barInstancing->selectedModel->setVisible(false);
                    barInstancing->multiSelectedModel = createDataItem(sliceView()->scene(),
                                                                       barSeries);
                    barInstancing->multiSelectedModel->setInstancing(
                        barInstancing->multiSelectionInstancing);
                    barInstancing->multiSelectedModel->setVisible(false);
                }

                if (!slicedBarList->contains(barInstancing))
                    slicedBarList->append(barInstancing);
            }
        }
    }
}

void QQuickGraphsBars::updateSliceGraph()
{
    if (m_selectionDirty)
        QQuickGraphsItem::updateSliceGraph();

    if (!sliceView()->isVisible()) {
        removeSlicedBarModels();
        m_changeTracker.selectedBarChanged = false;
        return;
    }

    int index = 0;
    bool rowMode = selectionMode().testFlag(QAbstract3DGraph::SelectionRow);
    for (auto it = m_slicedBarModels.begin(); it != m_slicedBarModels.end(); it++) {
        bool useGradient = it.key()->d_func()->isUsingGradient();
        bool rangeGradient = (useGradient
                              && it.key()->d_func()->m_colorStyle
                                     == Q3DTheme::ColorStyle::RangeGradient);
        QList<BarModel *> barList = *m_barModelsMap.value(it.key());
        QList<BarModel *> *sliceList = it.value();
        if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
            for (int ind = 0; ind < it.value()->count(); ++ind) {
                if (rowMode)
                    index = (m_selectedBar.x() * it.key()->dataProxy()->colCount()) + ind;
                else
                    index = m_selectedBar.y() + (ind * it.key()->dataProxy()->colCount());
                bool visible = ((m_selectedBarSeries == it.key()
                                 || selectionMode().testFlag(QAbstract3DGraph::SelectionMultiSeries))
                                && it.key()->isVisible());

                if (index < barList.size() && m_selectedBar != invalidSelectionPosition()) {
                    BarModel *sliceBarModel = sliceList->at(ind);
                    BarModel *barModel = barList.at(index);

                    sliceBarModel->model->setVisible(visible);
                    if (rowMode) {
                        sliceBarModel->model->setPosition(
                            QVector3D(barModel->model->x(), barModel->model->y(), 0.0f));
                    } else {
                        sliceBarModel->model->setX(barModel->model->z()
                                                   - (barList.at(0)->visualIndex * .1f));
                        sliceBarModel->model->setY(barModel->model->y());
                        sliceBarModel->model->setZ(0.0f);
                    }
                    sliceBarModel->model->setScale(barModel->model->scale());

                    updateItemMaterial(sliceBarModel->model,
                                       useGradient,
                                       rangeGradient,
                                       QStringLiteral(":/materials/BarsMaterial"));

                    if (barModel->coord == m_selectedBar
                        && selectionMode().testFlag(QAbstract3DGraph::SelectionItem)) {
                        updateMaterialProperties(sliceBarModel->model,
                                                 true,
                                                 false,
                                                 barList.at(index)->texture,
                                                 it.key()->singleHighlightColor());
                        updateSliceItemLabel(m_selectedBarSeries->itemLabel(), m_selectedBarPos);
                    } else {
                        updateMaterialProperties(sliceBarModel->model,
                                                 false,
                                                 false,
                                                 barList.at(index)->texture,
                                                 it.key()->baseColor());
                    }
                } else {
                    setSliceEnabled(false);
                    QQuickGraphsItem::updateSliceGraph();
                    return;
                }
            }
            setSliceActivatedChanged(false);
        } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
            deleteBarItemHolders(sliceList->at(0)->selectionInstancing);
            deleteBarItemHolders(sliceList->at(0)->multiSelectionInstancing);
            createBarItemHolders(it.key(), barList, true);
            setSliceActivatedChanged(false);
        }
    }
}

void QQuickGraphsBars::handleLabelCountChanged(QQuick3DRepeater *repeater)
{
    QQuickGraphsItem::handleLabelCountChanged(repeater);

    if (repeater == repeaterX())
        handleDataColumnLabelsChanged();
    if (repeater == repeaterZ())
        handleDataRowLabelsChanged();
}

void QQuickGraphsBars::removeSlicedBarModels()
{
    for (const auto list : std::as_const(m_slicedBarModels)) {
        for (auto barModel : *list) {
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                deleteBarModels(barModel->model);
            } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
                deleteBarItemHolders(barModel->selectionInstancing);
                deleteBarItemHolders(barModel->multiSelectionInstancing);
                deleteBarModels(barModel->selectedModel);
                deleteBarModels(barModel->multiSelectedModel);
            }
            delete barModel;
        }
        delete list;
    }

    m_slicedBarModels.clear();
}

void QQuickGraphsBars::createBarItemHolders(QBar3DSeries *series,
                                            QList<BarModel *> barList,
                                            bool slice)
{
    bool useGradient = series->d_func()->isUsingGradient();
    bool rangeGradient = (useGradient
                          && series->d_func()->m_colorStyle == Q3DTheme::ColorStyle::RangeGradient);
    bool visible = ((m_selectedBarSeries == series
                     || selectionMode().testFlag(QAbstract3DGraph::SelectionMultiSeries))
                    && series->isVisible());
    QList<BarItemHolder *> barItemList = *new QList<BarItemHolder *>;
    QList<BarItemHolder *> multiBarItemList = *new QList<BarItemHolder *>;
    QQuick3DModel *selectedModel = nullptr;
    QQuick3DModel *multiSelectedModel = nullptr;
    QList<BarItemHolder *> selectedItem = *new QList<BarItemHolder *>;
    QList<BarItemHolder *> multiSelectedItems = *new QList<BarItemHolder *>;
    BarInstancing *instancing;
    BarInstancing *multiInstancing;

    if (slice) {
        QList<BarModel *> sliceBarList = *m_slicedBarModels.value(series);
        barItemList = barList.at(0)->selectionInstancing->dataArray();
        multiBarItemList = barList.at(0)->multiSelectionInstancing->dataArray();
        selectedModel = sliceBarList.at(0)->selectedModel;
        multiSelectedModel = sliceBarList.at(0)->multiSelectedModel;
        instancing = sliceBarList.at(0)->selectionInstancing;
        multiInstancing = sliceBarList.at(0)->multiSelectionInstancing;
    } else {
        barItemList = barList.at(0)->instancing->dataArray();
        selectedModel = barList.at(0)->selectedModel;
        multiSelectedModel = barList.at(0)->multiSelectedModel;
        instancing = barList.at(0)->selectionInstancing;
        multiInstancing = barList.at(0)->multiSelectionInstancing;
    }

    for (const auto bih : std::as_const(barItemList)) {
        QQuickGraphsBars::SelectionType selectionType = isSelected(bih->coord.x(),
                                                                   bih->coord.y(),
                                                                   series);
        switch (selectionType) {
        case QQuickGraphsBars::SelectionItem: {
            updateItemMaterial(selectedModel,
                               useGradient,
                               rangeGradient,
                               QStringLiteral(":/materials/BarsMaterialInstancing"));
            updateMaterialProperties(selectedModel,
                                     true,
                                     false,
                                     barList.at(0)->texture,
                                     QColor(Qt::white));
            if (!slice)
                bih->selectedBar = true;
            selectedModel->setVisible(visible);
            BarItemHolder *selectedBih = new BarItemHolder();
            selectedBih->selectedBar = false;
            selectedBih->color = series->singleHighlightColor();
            selectedBih->coord = bih->coord;
            selectedBih->eulerRotation = bih->eulerRotation;
            selectedBih->heightValue = bih->heightValue;
            selectedBih->position = bih->position;
            selectedBih->scale = bih->scale;

            QString label = m_selectedBarSeries->itemLabel();
            if (slice) {
                if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow)) {
                    selectedBih->position.setZ(.0f);
                } else {
                    selectedBih->position.setX(selectedBih->position.z()
                                               - (barList.at(0)->visualIndex * .1f));
                    selectedBih->position.setZ(.0f);
                }
                updateSliceItemLabel(label, m_selectedBarPos);
            }

            selectedItem.push_back(selectedBih);
            instancing->setDataArray(selectedItem);

            m_selectedBarPos = bih->position;

            if (bih->heightValue >= 0.0f)
                m_selectedBarPos.setY(m_selectedBarPos.y() + bih->heightValue + 0.2f);
            else
                m_selectedBarPos.setY(m_selectedBarPos.y() + bih->heightValue - 0.2f);

            updateItemLabel(m_selectedBarPos);
            itemLabel()->setVisible(theme()->isLabelsEnabled());
            itemLabel()->setProperty("labelText", label);
            if (isSliceEnabled())
                updateSliceItemLabel(label, m_selectedBarPos);
            break;
        }
        case QQuickGraphsBars::SelectionRow:
        case QQuickGraphsBars::SelectionColumn: {
            updateItemMaterial(multiSelectedModel,
                               useGradient,
                               rangeGradient,
                               QStringLiteral(":/materials/BarsMaterialInstancing"));
            updateMaterialProperties(multiSelectedModel,
                                     false,
                                     true,
                                     barList.at(0)->texture,
                                     QColor(Qt::white));
            if (!slice)
                bih->selectedBar = true;
            multiSelectedModel->setVisible(visible);
            BarItemHolder *selectedBih = new BarItemHolder();
            selectedBih->selectedBar = false;
            selectedBih->color = series->multiHighlightColor();
            selectedBih->coord = bih->coord;
            selectedBih->eulerRotation = bih->eulerRotation;
            selectedBih->heightValue = bih->heightValue;
            selectedBih->position = bih->position;
            selectedBih->scale = bih->scale;

            multiSelectedItems.push_back(selectedBih);
            multiInstancing->setDataArray(multiSelectedItems);
            break;
        }
        default:
            break;
        }
    }

    if (slice) {
        for (const auto bih : std::as_const(multiBarItemList)) {
            updateItemMaterial(multiSelectedModel,
                               useGradient,
                               rangeGradient,
                               QStringLiteral(":/materials/BarsMaterialInstancing"));
            updateMaterialProperties(multiSelectedModel,
                                     false,
                                     false,
                                     barList.at(0)->texture,
                                     QColor(Qt::white));

            multiSelectedModel->setVisible(visible);
            BarItemHolder *selectedBih = new BarItemHolder();
            selectedBih->selectedBar = false;
            selectedBih->color = series->baseColor();
            selectedBih->coord = bih->coord;
            selectedBih->eulerRotation = bih->eulerRotation;
            selectedBih->heightValue = bih->heightValue;
            selectedBih->position = bih->position;
            selectedBih->scale = bih->scale;

            if (selectionMode().testFlag(QAbstract3DGraph::SelectionRow)) {
                selectedBih->position.setZ(.0f);
            } else {
                selectedBih->position.setX(selectedBih->position.z()
                                           - (barList.at(0)->visualIndex * .1f));
                selectedBih->position.setZ(.0f);
            }

            multiSelectedItems.push_back(selectedBih);
            multiInstancing->setDataArray(multiSelectedItems);
        }
    }
}

void QQuickGraphsBars::updateSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    checkSliceEnabled();
    if (!sliceView())
        createSliceView();

    bool validSlice = mode.testFlag(QAbstract3DGraph::SelectionSlice) && m_selectedBarSeries;
    if (sliceView() && sliceView()->isVisible()) {
        if (validSlice) {
            removeSlicedBarModels();
            createSliceView();
            updateSliceGraph();
        } else {
            m_selectionDirty = true;
            setSliceActivatedChanged(true);
        }
    } else if (validSlice) {
        m_selectionDirty = true;
        setSliceActivatedChanged(true);
    }

    setSeriesVisualsDirty(true);
    itemLabel()->setVisible(false);
    if (sliceView() && sliceView()->isVisible())
        sliceItemLabel()->setVisible(false);
}

void QQuickGraphsBars::updateBarSpecs(float thicknessRatio, const QSizeF &spacing, bool relative)
{
    // Convert ratio to QSizeF, as we need it in that format for autoscaling
    // calculations
    m_cachedBarThickness.setWidth(1.0);
    m_cachedBarThickness.setHeight(1.0f / thicknessRatio);

    if (relative) {
        m_cachedBarSpacing.setWidth((m_cachedBarThickness.width() * 2) * (spacing.width() + 1.0f));
        m_cachedBarSpacing.setHeight((m_cachedBarThickness.height() * 2)
                                     * (spacing.height() + 1.0f));
    } else {
        m_cachedBarSpacing = m_cachedBarThickness * 2 + spacing * 2;
    }

    m_axisRangeChanged = true;
    m_changeTracker.selectedBarChanged = true;

    // Calculate here and at setting sample space
    calculateSceneScalingFactors();
}

void QQuickGraphsBars::updateBarSeriesMargin(const QSizeF &margin)
{
    m_cachedBarSeriesMargin = margin;
    calculateSeriesStartPosition();
    calculateSceneScalingFactors();
    setSeriesVisualsDirty(true);
}
