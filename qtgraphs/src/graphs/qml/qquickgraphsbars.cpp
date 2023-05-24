// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickgraphsbars_p.h"
#include "quickgraphstexturedata_p.h"
#include "bars3dcontroller_p.h"
#include "declarativescene_p.h"
#include "qbar3dseries_p.h"
#include "qvalue3daxis_p.h"
#include "qcategory3daxis_p.h"

#include "q3dcamera_p.h"
#include <QtCore/QMutexLocker>
#include <QColor>
#include "q3dtheme_p.h"

#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include "quickgraphstexturedata_p.h"
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>

QQuickGraphsBars::QQuickGraphsBars(QQuickItem *parent)
    : QQuickGraphsItem(parent),
      m_barsController(0),
      m_cachedRowCount(0),
      m_cachedColumnCount(0),
      m_minRow(0),
      m_maxRow(0),
      m_minCol(0),
      m_maxCol(0),
      m_newRows(0),
      m_newCols(0),
      m_maxSceneSize(40.0f),
      m_rowWidth(0),
      m_columnDepth(0),
      m_maxDimension(0),
      m_scaleFactor(0),
      m_xScaleFactor(1.0f),
      m_zScaleFactor(1.0f),
      m_cachedBarSeriesMargin(0.0f, 0.0f),
      m_hasNegativeValues(false),
      m_noZeroInRange(false),
      m_actualFloorLevel(0.0f),
      m_heightNormalizer(1.0f),
      m_backgroundAdjustment(0.0f),
      m_selectedBarSeries(0),
      m_selectedBarCoord(Bars3DController::invalidSelectionPosition()),
      m_selectedBarPos(0.0f, 0.0f, 0.0f),
      m_keepSeriesUniform(false),
      m_seriesScaleX(0.0f),
      m_seriesScaleZ(0.0f),
      m_seriesStep(0.0f),
      m_seriesStart(0.0f),
      m_zeroPosition(0.0f),
      m_visibleSeriesCount(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlags(ItemHasContents);
    // Create the shared component on the main GUI thread.
    m_barsController = new Bars3DController(boundingRect().toRect(), new Declarative3DScene);
    setSharedController(m_barsController);

    QObject::connect(m_barsController, &Bars3DController::primarySeriesChanged,
                     this, &QQuickGraphsBars::primarySeriesChanged);
    QObject::connect(m_barsController, &Bars3DController::selectedSeriesChanged,
                     this, &QQuickGraphsBars::selectedSeriesChanged);
}

QQuickGraphsBars::~QQuickGraphsBars()
{
    QMutexLocker locker(m_nodeMutex.data());
    const QMutexLocker locker2(mutex());
    removeBarModels();
    removeSelectedModels();
    removeSlicedBarModels();
    delete m_barsController;
}

QCategory3DAxis *QQuickGraphsBars::rowAxis() const
{
    return static_cast<QCategory3DAxis *>(m_barsController->axisZ());
}

void QQuickGraphsBars::setRowAxis(QCategory3DAxis *axis)
{
    m_barsController->setAxisZ(axis);
    // labelsChanged and rangeChanged signals are required to update the row and column numbers.
    // The same situation exists in the barscontroller. (see setAxisZ and setAxisHelper)
    // A better implementation may apply once controllers are removed
    QObject::connect(axis, &QAbstract3DAxis::labelsChanged, this,
                     &QQuickGraphsBars::handleRowCountChanged);
    QObject::connect(axis, &QAbstract3DAxis::rangeChanged,
                     this, &QQuickGraphsBars::handleRowCountChanged);
    handleRowCountChanged();
}

QValue3DAxis *QQuickGraphsBars::valueAxis() const
{
    return static_cast<QValue3DAxis *>(m_barsController->axisY());
}

void QQuickGraphsBars::setValueAxis(QValue3DAxis *axis)
{
    m_barsController->setAxisY(axis);
    if (segmentLineRepeaterY()) {
        int segmentCount = 0;
        int subSegmentCount = 0;
        int gridLineCount = 0;
        int subGridLineCount = 0;
        if (axis->type() & QAbstract3DAxis::AxisTypeValue) {
            QValue3DAxis *valueAxis = static_cast<QValue3DAxis *>(axis);
            segmentCount = valueAxis->segmentCount();
            subSegmentCount = valueAxis->subSegmentCount();
            gridLineCount = 2 * (segmentCount + 1);
            subGridLineCount = 2 * (segmentCount * (subSegmentCount - 1));
        } else if (axis->type() & QAbstract3DAxis::AxisTypeCategory) {
            gridLineCount = axis->labels().size();
        }
        segmentLineRepeaterY()->setModel(gridLineCount);
        subsegmentLineRepeaterY()->setModel(subGridLineCount);
        repeaterY()->setModel(2 * axis->labels().size());
    }
}

QCategory3DAxis *QQuickGraphsBars::columnAxis() const
{
    return static_cast<QCategory3DAxis *>(m_barsController->axisX());
}

void QQuickGraphsBars::setColumnAxis(QCategory3DAxis *axis)
{
    m_barsController->setAxisX(axis);
    QObject::connect(axis, &QAbstract3DAxis::labelsChanged, this,
                     &QQuickGraphsBars::handleColCountChanged);
    QObject::connect(axis, &QAbstract3DAxis::rangeChanged, this,
                     &QQuickGraphsBars::handleColCountChanged);
    handleColCountChanged();
}

void QQuickGraphsBars::setMultiSeriesUniform(bool uniform)
{
    if (uniform != isMultiSeriesUniform()) {
        m_barsController->setMultiSeriesScaling(uniform);
        emit multiSeriesUniformChanged(uniform);
    }
}

bool QQuickGraphsBars::isMultiSeriesUniform() const
{
    return m_barsController->multiSeriesScaling();
}

void QQuickGraphsBars::setBarThickness(float thicknessRatio)
{
    if (thicknessRatio != barThickness()) {
        m_barsController->setBarSpecs(thicknessRatio, barSpacing(),
                                      isBarSpacingRelative());
        emit barThicknessChanged(thicknessRatio);
    }
}

float QQuickGraphsBars::barThickness() const
{
    return m_barsController->barThickness();
}

void QQuickGraphsBars::setBarSpacing(const QSizeF &spacing)
{
    if (spacing != barSpacing()) {
        m_barsController->setBarSpecs(barThickness(), spacing, isBarSpacingRelative());
        emit barSpacingChanged(spacing);
    }
}

QSizeF QQuickGraphsBars::barSpacing() const
{
    return m_barsController->barSpacing();
}

void QQuickGraphsBars::setBarSpacingRelative(bool relative)
{
    if (relative != isBarSpacingRelative()) {
        m_barsController->setBarSpecs(barThickness(), barSpacing(), relative);
        emit barSpacingRelativeChanged(relative);
    }
}

bool QQuickGraphsBars::isBarSpacingRelative() const
{
    return m_barsController->isBarSpecRelative();
}

void QQuickGraphsBars::setBarSeriesMargin(const QSizeF &margin)
{
    if (margin != barSeriesMargin()) {
        m_barsController->setBarSeriesMargin(margin);
        emit barSeriesMarginChanged(barSeriesMargin());
    }
}

QSizeF QQuickGraphsBars::barSeriesMargin() const
{
    return m_barsController->barSeriesMargin();
}

QQmlListProperty<QBar3DSeries> QQuickGraphsBars::seriesList()
{
    return QQmlListProperty<QBar3DSeries>(this, this,
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
    return reinterpret_cast<QQuickGraphsBars *>(list->data)->m_barsController->barSeriesList().size();
}

QBar3DSeries *QQuickGraphsBars::atSeriesFunc(QQmlListProperty<QBar3DSeries> *list, qsizetype index)
{
    return reinterpret_cast<QQuickGraphsBars *>(list->data)->m_barsController->barSeriesList().at(index);
}

void QQuickGraphsBars::clearSeriesFunc(QQmlListProperty<QBar3DSeries> *list)
{
    QQuickGraphsBars *declBars = reinterpret_cast<QQuickGraphsBars *>(list->data);
    QList<QBar3DSeries *> realList = declBars->m_barsController->barSeriesList();
    int count = realList.size();
    for (int i = 0; i < count; i++)
        declBars->removeSeries(realList.at(i));
}

void QQuickGraphsBars::addSeries(QBar3DSeries *series)
{
    m_barsController->addSeries(series);
    connectSeries(series);
    if (series->selectedBar() != invalidSelectionPosition())
        updateSelectedBar();
}

void QQuickGraphsBars::removeSeries(QBar3DSeries *series)
{
    m_barsController->removeSeries(series);
    removeBarModels();
    series->setParent(this); // Reparent as removing will leave series parentless
    disconnectSeries(series);
    handleRowCountChanged();
    handleColCountChanged();
}

void QQuickGraphsBars::insertSeries(int index, QBar3DSeries *series)
{
    m_barsController->insertSeries(index, series);
    handleRowCountChanged();
    handleColCountChanged();
}

void QQuickGraphsBars::setPrimarySeries(QBar3DSeries *series)
{
    m_barsController->setPrimarySeries(series);
    handleRowCountChanged();
    handleColCountChanged();
}

QBar3DSeries *QQuickGraphsBars::primarySeries() const
{
    return m_barsController->primarySeries();
}

QBar3DSeries *QQuickGraphsBars::selectedSeries() const
{
    return m_barsController->selectedSeries();
}

void QQuickGraphsBars::setFloorLevel(float level)
{
    if (level != floorLevel()) {
        m_barsController->setFloorLevel(level);
        emit floorLevelChanged(level);
    }
}

float QQuickGraphsBars::floorLevel() const
{
    return m_barsController->floorLevel();
}

void QQuickGraphsBars::componentComplete()
{
    QQuickGraphsItem::componentComplete();

    auto wallBackground = background();
    QUrl wallUrl = QUrl(QStringLiteral("defaultMeshes/backgroundNoFloorMesh"));
    wallBackground->setSource(wallUrl);
    setBackground(wallBackground);

    QUrl floorUrl = QUrl(QStringLiteral(":/defaultMeshes/planeMesh"));
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

    QValue3DAxis *axisY = static_cast<QValue3DAxis *>(m_barsController->axisY());
    m_helperAxisY.setFormatter(axisY->formatter());

    setFloorGridInRange(true);
    setVerticalSegmentLine(false);

    QObject::connect(cameraTarget(), &QQuick3DNode::rotationChanged, this,
                     &QQuickGraphsBars::handleCameraRotationChanged);
}

void QQuickGraphsBars::synchData()
{
    Q3DCamera *camera = m_barsController->m_scene->activeCamera();
    Q3DTheme *theme = m_barsController->activeTheme();

    if (!m_noZeroInRange) {
        camera->d_func()->setMinYRotation(-90.0f);
        camera->d_func()->setMaxYRotation(90.0f);
    } else {
        if ((m_hasNegativeValues && !m_helperAxisY.isReversed())
                || (!m_hasNegativeValues && m_helperAxisY.isReversed())) {
            camera->d_func()->setMinYRotation(-90.0f);
            camera->d_func()->setMaxYRotation(0.0f);
        } else {
            camera->d_func()->setMinYRotation(0.0f);
            camera->d_func()->setMaxYRotation(90.0f);
        }
    }
    if (m_barsController->m_changeTracker.barSpecsChanged || !m_cachedBarThickness.isValid()) {
        updateBarSpecs(m_barsController->m_barThicknessRatio, m_barsController->m_barSpacing,
                       m_barsController->m_isBarSpecRelative);
        m_barsController->m_changeTracker.barSpecsChanged = false;
    }

    // Floor level update requires data update, so do before qquickgraphicsitem sync
    if (m_barsController->m_changeTracker.floorLevelChanged) {
        updateFloorLevel(m_barsController->m_floorLevel);
        m_barsController->m_changeTracker.floorLevelChanged = false;
    }

    // Do not clear dirty flag, we need to react to it in qquickgraphicsitem as well
    if (theme->d_func()->m_dirtyBits.backgroundEnabledDirty) {
        m_floorBackground->setVisible(theme->isBackgroundEnabled());
        m_barsController->setSeriesVisualsDirty(true);
        for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++)
            it.key()->d_func()->m_changeTracker.meshChanged = true;
    }

    if (m_barsController->m_changeTracker.barSeriesMarginChanged) {
        updateBarSeriesMargin(barSeriesMargin());
        m_barsController->m_changeTracker.barSeriesMarginChanged = false;
    }

    auto axisY = static_cast<QValue3DAxis *>(m_barsController->axisY());
    axisY->formatter()->d_func()->recalculate();
    m_helperAxisY.setFormatter(axisY->formatter());

    if (m_axisRangeChanged) {
        theme->d_func()->resetDirtyBits();
        updateGrid();
        updateLabels();
        m_axisRangeChanged = false;
    }

    QQuickGraphsItem::synchData();

    QMatrix4x4 modelMatrix;

    // Draw floor
    m_floorBackground->setPickable(false);
    m_floorBackgroundScale->setScale(scaleWithBackground());
    modelMatrix.scale(scaleWithBackground());
    m_floorBackgroundScale->setPosition(QVector3D(0.0f, -m_backgroundAdjustment, 0.0f));

    QQuaternion m_xRightAngleRotation(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 90.0f));
    QQuaternion m_xRightAngleRotationNeg(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -90.0f));

    if (isYFlipped()) {
        m_floorBackgroundRotation->setRotation(m_xRightAngleRotation);
        modelMatrix.rotate(m_xRightAngleRotation);
    } else {
        m_floorBackgroundRotation->setRotation(m_xRightAngleRotationNeg);
        modelMatrix.rotate(m_xRightAngleRotationNeg);
    }

    auto bgFloor = m_floorBackground;
    bgFloor->setPickable(false);
    QQmlListReference materialsRefF(bgFloor, "materials");
    QQuick3DPrincipledMaterial * bgMatFloor;

    if (!materialsRefF.size()) {
        bgMatFloor = new QQuick3DPrincipledMaterial();
        bgMatFloor->setParent(bgFloor);
        bgMatFloor->setMetalness(0.f);
        bgMatFloor->setRoughness(.3f);
        bgMatFloor->setEmissiveFactor(QVector3D(.001f, .001f, .001f));
        materialsRefF.append(bgMatFloor);
    } else {
        bgMatFloor = static_cast<QQuick3DPrincipledMaterial *>(materialsRefF.at(0));
    }
    bgMatFloor->setBaseColor(theme->backgroundColor());

    if (m_selectedBarPos.isNull())
        itemLabel()->setVisible(false);
}

void QQuickGraphsBars::updateParameters() {
    int cachedMinRow = m_minRow;
    int cachedMinCol = m_minCol;
    m_minRow = m_barsController->m_axisZ->min();
    m_maxRow = m_barsController->m_axisZ->max();
    m_minCol = m_barsController->m_axisX->min();
    m_maxCol = m_barsController->m_axisX->max();
    m_newRows = m_maxRow - m_minRow + 1;
    m_newCols = m_maxCol - m_minCol + 1;

    QList<QBar3DSeries *> barSeriesList = m_barsController->barSeriesList();
    if (m_cachedRowCount!= m_newRows || m_cachedColumnCount != m_newCols) {
        m_barsController->m_changeTracker.selectedBarChanged = true;
        m_cachedColumnCount = m_newCols;
        m_cachedRowCount = m_newRows;

        // Calculate max scene size
        float sceneRatio = qMin(float(m_newCols) / float(m_newRows),
                                float(m_newRows) / float(m_newCols));
        m_maxSceneSize = 2.0f * qSqrt(sceneRatio * m_newCols * m_newRows);

        if (m_cachedBarThickness.isValid())
            calculateSceneScalingFactors();

        removeBarModels();
        removeSelectedModels();
    }

    if (cachedMinRow != m_minRow || cachedMinCol != m_minCol)
        removeBarModels();

    m_axisRangeChanged = true;
    m_barsController->setDataDirty(true);
}

void QQuickGraphsBars::updateFloorLevel(float level)
{
    setFloorLevel(level);
    calculateHeightAdjustment();
}

void QQuickGraphsBars::updateGraph()
{
    QList<QBar3DSeries *> barSeriesList = m_barsController->barSeriesList();
    calculateSceneScalingFactors();

    bool isEmpty = m_barsController->m_changedSeriesList.isEmpty();

    for (const auto &series : std::as_const(barSeriesList)) {
        if (series->d_func()->m_changeTracker.meshChanged) {
            removeBarModels();
            removeSelectedModels();
            series->d_func()->m_changeTracker.meshChanged = false;
            m_barsController->setDataDirty(true);
        }
    }

    if (m_barsController->isDataDirty())
        generateBars(barSeriesList);

    if (m_barsController->isSeriesVisualsDirty()) {
        if (isSliceEnabled()) {
            removeSlicedBarModels();
            createSliceView();
            if (!isEmpty) {
                updateSliceGrid();
                updateSliceLabels();
            }
            if (!isSliceActivatedChanged() && m_selectionDirty) {
                setSliceActivatedChanged(true);
                m_selectionDirty = false;
            }
        }
        int visualIndex = 0;
        for (const auto &barSeries : std::as_const(barSeriesList)) {
            if (barSeries->isVisible()) {
                updateBarVisuality(barSeries, visualIndex);
                updateBarPositions(barSeries);
                updateBarVisuals(barSeries);
                ++visualIndex;
            } else {
                updateBarVisuality(barSeries, -1);
            }
        }

        // Needs to be done after data is set, as it needs to know the visual array.
        if (m_barsController->m_changeTracker.selectedBarChanged) {
            updateSelectedBar();
            m_barsController->m_changeTracker.selectedBarChanged = false;
        }
    }

    m_barsController->setDataDirty(false);
    m_barsController->setSeriesVisualsDirty(false);
}

void QQuickGraphsBars::updateAxisRange(float min, float max)
{
    QQuickGraphsItem::updateAxisRange(min, max);

    m_helperAxisY.setMin(min);
    m_helperAxisY.setMax(max);

    calculateHeightAdjustment();
}

void QQuickGraphsBars::updateAxisReversed(bool enable)
{
    m_barsController->setSeriesVisualsDirty(true);
    m_helperAxisY.setReversed(enable);
    calculateHeightAdjustment();
}

void QQuickGraphsBars::calculateSceneScalingFactors()
{
    m_rowWidth = (m_cachedColumnCount * m_cachedBarSpacing.width()) * 0.5f;
    m_columnDepth = (m_cachedRowCount * m_cachedBarSpacing.height()) * 0.5f;
    m_maxDimension = qMax(m_rowWidth, m_columnDepth);
    m_scaleFactor = qMin((m_cachedColumnCount *(m_maxDimension / m_maxSceneSize)),
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

    m_scaleXWithBackground = m_xScaleFactor + m_hBackgroundMargin;
    m_scaleYWithBackground = 1.0f + m_vBackgroundMargin;
    m_scaleZWithBackground = m_zScaleFactor + m_hBackgroundMargin;

    auto scale = QVector3D(m_xScaleFactor, 1.0f, m_zScaleFactor);
    setScaleWithBackground(scale);
    setBackgroundScaleMargin({m_hBackgroundMargin, m_vBackgroundMargin, m_hBackgroundMargin});
    setScale(scale);

    m_helperAxisX.setScale(m_scaleXWithBackground * 2);
    m_helperAxisY.setScale(m_yScale);
    m_helperAxisZ.setScale(-m_scaleZWithBackground * 2);
    m_helperAxisX.setTranslate(-m_xScale);
    m_helperAxisY.setTranslate(0.0f);
}

void QQuickGraphsBars::calculateHeightAdjustment()
{
    m_minHeight = m_helperAxisY.min();
    m_maxHeight = m_helperAxisY.max();
    float newAdjustment = 1.0f;
    m_actualFloorLevel = qBound(m_minHeight, floorLevel(), m_maxHeight);
    float maxAbs = qFabs(m_maxHeight - m_actualFloorLevel);

    // Check if we have negative values
    if (m_minHeight < m_actualFloorLevel)
        m_hasNegativeValues = true;
    else if (m_minHeight >= m_actualFloorLevel)
        m_hasNegativeValues = false;

    if (m_maxHeight < m_actualFloorLevel) {
        m_heightNormalizer = float(qFabs(m_minHeight) - qFabs(m_maxHeight));
        maxAbs = qFabs(m_maxHeight) - qFabs(m_minHeight);
    } else {
        m_heightNormalizer = float(m_maxHeight - m_minHeight);
    }

    // Height fractions are used in gradient calculations and are therefore doubled
    // Note that if max or min is exactly zero, we still consider it outside the range
    if (m_maxHeight <= m_actualFloorLevel || m_minHeight >= m_actualFloorLevel) {
        m_noZeroInRange = true;
        m_gradientFraction = 2.0f;
    } else {
        m_noZeroInRange = false;
        float minAbs = qFabs(m_minHeight - m_actualFloorLevel);
        m_gradientFraction = qMax(minAbs, maxAbs) / m_heightNormalizer * 2.0f;
    }

    // Calculate translation adjustment for background floor
    newAdjustment = (qBound(0.0f, (maxAbs / m_heightNormalizer), 1.0f) - 0.5f) * 2.0f;
    if (m_helperAxisY.isReversed())
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
                                                           QVector3D labelPosition, int index)
{
    QVector3D ret = labelPosition;
    if (axis->orientation() == QAbstract3DAxis::AxisOrientationX) {
        float xPos = (index + 0.5f) * m_cachedBarSpacing.width();
        ret.setX((xPos - m_rowWidth) / m_scaleFactor);
    }
    if (axis->orientation() == QAbstract3DAxis::AxisOrientationZ) {
        float zPos = (index + 0.5f) * m_cachedBarSpacing.height();
        ret.setZ((m_columnDepth - zPos) / m_scaleFactor);
    }
    ret.setY(-m_backgroundAdjustment);
    return ret;
}

float QQuickGraphsBars::calculateCategoryGridLinePosition(QAbstract3DAxis *axis, int index)
{
    float ret = 0.0f;
    if (axis->orientation() == QAbstract3DAxis::AxisOrientationZ) {
        float colPos = index * -(m_cachedBarSpacing.height() / m_scaleFactor);
        ret = colPos + scale().z();
    }
    if (axis->orientation() == QAbstract3DAxis::AxisOrientationX) {
        float rowPos = index * (m_cachedBarSpacing.width() / m_scaleFactor);
        ret = rowPos - scale().x();
    }
    if (axis->orientation() == QAbstract3DAxis::AxisOrientationY)
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
    removeBarModels();
}

void QQuickGraphsBars::handleMeshSmoothChanged(bool enable)
{
    m_smooth = enable;
    removeBarModels();
}

void QQuickGraphsBars::handleRowCountChanged()
{
    QCategory3DAxis *categoryAxisZ = static_cast<QCategory3DAxis *>(m_barsController->axisZ());
    if (repeaterZ()) {
        updateParameters();
        segmentLineRepeaterZ()->model().clear();
        segmentLineRepeaterZ()->setModel(m_cachedRowCount);
        repeaterZ()->model().clear();
        repeaterZ()->setModel(categoryAxisZ->labels().size());
    }
}

void QQuickGraphsBars::handleColCountChanged()
{
    QCategory3DAxis *categoryAxisX = static_cast<QCategory3DAxis *>(m_barsController->axisX());
    if (repeaterX()) {
        updateParameters();
        segmentLineRepeaterX()->model().clear();
        segmentLineRepeaterX()->setModel(m_cachedColumnCount);
        repeaterX()->model().clear();
        repeaterX()->setModel(categoryAxisX->labels().size());
    }
}

void QQuickGraphsBars::handleRowColorsChanged()
{
    m_barsController->setSeriesVisualsDirty(true);
}

void QQuickGraphsBars::handleCameraRotationChanged()
{
    updateLabels();
}

void QQuickGraphsBars::connectSeries(QBar3DSeries *series)
{
    m_meshType = series->mesh();
    m_smooth = series->isMeshSmooth();

    QObject::connect(series, &QBar3DSeries::meshChanged, this,
                     &QQuickGraphsBars::handleSeriesMeshChanged);
    QObject::connect(series, &QBar3DSeries::meshSmoothChanged, this,
                     &QQuickGraphsBars::handleMeshSmoothChanged);
    QObject::connect(series->dataProxy(), &QBarDataProxy::colCountChanged, this,
                     &QQuickGraphsBars::handleColCountChanged);
    QObject::connect(series->dataProxy(), &QBarDataProxy::rowCountChanged, this,
                     &QQuickGraphsBars::handleRowCountChanged);
    QObject::connect(series, &QBar3DSeries::rowColorsChanged, this,
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
        auto textureData = static_cast<QuickGraphsTextureData *>(texture->textureData());
        textureData->createGradient(gradient);

        bool visible = barSeries->isVisible();

        QList<BarModel *> *barList = m_barModelsMap.value(barSeries);

        if (!barList) {
            barList = new QList<BarModel *>;
            m_barModelsMap[barSeries] = barList;
        }

        if (barList->isEmpty()) {
            if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {

                QBarDataProxy *dataProxy = barSeries->dataProxy();
                int dataRowIndex = m_minRow;
                int newRowSize = qMin(dataProxy->rowCount() - dataRowIndex, m_newRows);

                for (int row = 0; row < newRowSize; ++row) {
                    const QBarDataRow *dataRow = dataProxy->rowAt(dataRowIndex);
                    if (dataRow) {
                        int dataColIndex = m_minCol;
                        int newColSize = qMin(dataRow->size() - dataColIndex, m_newCols);
                        for (int col = 0; col < newColSize; ++col) {
                            QBarDataItem *dataItem = const_cast<QBarDataItem *> (&(dataRow->at(dataColIndex)));
                            auto scene = QQuick3DViewport::scene();
                            QQuick3DModel *model = createDataItem(scene, barSeries);
                            model->setVisible(visible);

                            BarModel *barModel = new BarModel();
                            barModel->model = model;
                            barModel->barItem = dataItem;
                            barModel->coord = QPoint(dataRowIndex, col);
                            barModel->texture = texture;

                            if (!barList->contains(barModel)) {
                                barList->append(barModel);
                            } else {
                                delete barModel->model;
                                delete barModel;
                            }
                            ++dataColIndex;
                        }
                        ++dataRowIndex;
                    }
                }
            } else if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
                auto scene = QQuick3DViewport::scene();
                BarModel *barInstancing = new BarModel();
                barInstancing->texture = texture;

                if (barInstancing->instancing == nullptr) {
                    barInstancing->instancing = new BarInstancing;
                    barInstancing->instancing->setParent(barSeries);
                }

                if (barInstancing->model == nullptr) {
                    barInstancing->model = createDataItem(scene, barSeries);
                    barInstancing->model->setInstancing(barInstancing->instancing);
                    barInstancing->model->setVisible(visible);
                    barInstancing->model->setPickable(true);
                }

                if (!barList->contains(barInstancing)) {
                    barList->append(barInstancing);
                } else {
                    delete barInstancing->instancing;
                    delete barInstancing->model;
                    delete barInstancing;
                }
            }
            createSelectedModels(barSeries);
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
    case QAbstract3DSeries::MeshSphere:
        fileName = QStringLiteral("defaultMeshes/sphereMesh");
        break;
    case QAbstract3DSeries::MeshBar:
    case QAbstract3DSeries::MeshCube:
        fileName = QStringLiteral("defaultMeshes/barMesh");
        break;
    case QAbstract3DSeries::MeshPyramid:
        fileName = QStringLiteral("defaultMeshes/pyramidMesh");
        break;
    case QAbstract3DSeries::MeshCone:
        fileName = QStringLiteral("defaultMeshes/coneMesh");
        break;
    case QAbstract3DSeries::MeshCylinder:
        fileName = QStringLiteral("defaultMeshes/cylinderMesh");
        break;
    case QAbstract3DSeries::MeshBevelBar:
    case QAbstract3DSeries::MeshBevelCube:
        fileName = QStringLiteral("defaultMeshes/bevelBarMesh");
        break;
    case QAbstract3DSeries::MeshUserDefined:
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
    if (m_smooth && meshType != QAbstract3DSeries::MeshPoint
        && meshType != QAbstract3DSeries::MeshUserDefined) {
        fileName += QStringLiteral("Smooth");
    }

    // Should it be filled?
    if (!m_barsController->activeTheme()->isBackgroundEnabled()
            && meshType != QAbstract3DSeries::MeshSphere
            && meshType != QAbstract3DSeries::MeshPoint
            && meshType != QAbstract3DSeries::MeshUserDefined) {
        fileName.append(QStringLiteral("Full"));
    }
}

void QQuickGraphsBars::updateBarVisuality(QBar3DSeries *series, int visualIndex)
{
    QList<BarModel *> barList = *m_barModelsMap.value(series);
    for (int i = 0; i < barList.count(); i++) {
        m_barsController->m_changeTracker.selectedBarChanged = true;
        if (barList.at(i)->model->visible() != series->isVisible() && isSliceEnabled()) {
            if (m_selectedBarSeries == series && !series->isVisible()) {
                setSliceEnabled(false);
                setSliceActivatedChanged(true);
                m_selectionDirty = true;
            } else {
                setSliceActivatedChanged(true);
                m_selectionDirty = false;
            }
        }
        if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
            barList.at(i)->visualIndex = visualIndex;
            barList.at(i)->model->setVisible(series->isVisible());
        } else if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
            barList.at(i)->visualIndex = visualIndex;
            barList.at(i)->model->setVisible(series->isVisible());
            if (m_selectedBarSeries == series && !series->isVisible()) {
                for (const auto list : std::as_const(m_selectedModels)) {
                    for (auto selectedModel : *list)
                        selectedModel->setVisible(false);
                }
            }
        }
    }

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

    m_meshRotation = dataProxy->series()->meshRotation();
    m_zeroPosition = m_helperAxisY.itemPositionAt(m_actualFloorLevel);

    QList<BarModel *> barList = *m_barModelsMap.value(series);

    int dataRowIndex = m_minRow;
    int newRowSize = qMin(dataProxy->rowCount() - dataRowIndex, m_newRows);
    int row = 0;
    int dataColIndex = m_minCol;
    int newColSize = qMin(dataProxy->colCount() - dataColIndex, m_newCols);
    int col = 0;
    for (int i = 0; i < barList.count(); i++) {
        if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
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

            float seriesPos = m_seriesStart + m_seriesStep * (barList.at(i)->visualIndex
                                                              - (barList.at(i)->visualIndex
                                                                 * m_cachedBarSeriesMargin.width())) + 0.5f;


            float colPos = (col + seriesPos) * m_cachedBarSpacing.width();
            float xPos = (colPos - m_rowWidth) / m_scaleFactor;
            float rowPos = (row + 0.5f) * (m_cachedBarSpacing.height());
            float zPos = (m_columnDepth - rowPos) / m_scaleFactor;

            barList.at(i)->heightValue = heightValue;
            model->setPosition(QVector3D(xPos, heightValue - m_backgroundAdjustment, zPos));
            model->setScale(QVector3D(m_xScale * m_seriesScaleX, qAbs(heightValue),
                                      m_zScale * m_seriesScaleZ));

            if (heightValue == 0)
                model->setPickable(false);
            else
                model->setPickable(true);

            if (col < newColSize - 1) {
                ++col;
            } else {
                col = 0;
                if (row < newRowSize - 1)
                    ++row;
                else
                    row = 0;
            }
        } else if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
            auto barItemList = barList.at(i)->instancing->dataArray();
            for (auto bih : barItemList)
                delete bih;
            barList.at(i)->instancing->clearDataArray();

            QList<BarItemHolder *> positions;
            for (int row = 0; row < newRowSize; ++row) {
                const QBarDataRow *dataRow = dataProxy->rowAt(dataRowIndex);
                if (dataRow) {
                    dataColIndex = m_minCol;
                    for (int col = 0; col < newColSize; col++) {
                        const QBarDataItem *item = const_cast<QBarDataItem *> (&(dataRow->at(dataColIndex)));
                        float heightValue = updateBarHeightParameters(item);

                        float angle = item->rotation();
                        BarItemHolder *bih = new BarItemHolder();
                        if (angle)
                            bih->rotation = QQuaternion::fromAxisAndAngle(upVector, angle);
                        else
                            bih->rotation = QQuaternion();

                        if (heightValue < 0.f) {
                            const QVector3D eulerRot = barList.at(i)->model->eulerRotation();
                            bih->eulerRotation = QVector3D(-180.f, eulerRot.y(), eulerRot.z());
                        }

                        float seriesPos = m_seriesStart + m_seriesStep * (barList.at(i)->visualIndex
                                                                          - (barList.at(i)->visualIndex
                                                                             * m_cachedBarSeriesMargin.width())) + 0.5f;


                        float colPos = (col + seriesPos) * m_cachedBarSpacing.width();
                        float xPos = (colPos - m_rowWidth) / m_scaleFactor;
                        float rowPos = (row + 0.5f) * (m_cachedBarSpacing.height());
                        float zPos = (m_columnDepth - rowPos) / m_scaleFactor;

                        bih->position = {xPos, (heightValue - m_backgroundAdjustment), zPos};
                        bih->coord = QPoint(row, col);

                        if (heightValue == 0) {
                            bih->scale = {.0f, .0f, .0f};
                        } else {
                            bih->scale = {m_xScale * m_seriesScaleX, qAbs(heightValue),
                                          m_zScale * m_seriesScaleZ};
                        }

                        bih->heightValue = heightValue;
                        bih->selectedBar = false;

                        bool colorStyleIsUniform = (series->colorStyle() == Q3DTheme::ColorStyleUniform);
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
    float heightValue = m_helperAxisY.itemPositionAt(value);

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

    if (m_helperAxisY.isReversed())
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
        auto highlightTextureData
            = static_cast<QuickGraphsTextureData *>(m_highlightTexture->textureData());
        highlightTextureData->createGradient(highlightGradient);
        auto multiHighlightGradient = series->multiHighlightGradient();
        auto multiHighlightTextureData
            = static_cast<QuickGraphsTextureData *>(m_multiHighlightTexture->textureData());
        multiHighlightTextureData->createGradient(multiHighlightGradient);
    } else {
        if (m_hasHighlightTexture) {
            m_highlightTexture->deleteLater();
            m_multiHighlightTexture->deleteLater();
            m_hasHighlightTexture = false;
        }
    }

    bool rangeGradient = (useGradient && series->d_func()->m_colorStyle
                                             == Q3DTheme::ColorStyleRangeGradient);
    QColor baseColor = series->baseColor();
    QColor barColor;

    if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
        if (!rangeGradient) {
            for (int i = 0; i < barList.count(); i++) {
                QQuick3DModel *model = barList.at(i)->model;
                updateItemMaterial(model, useGradient, rangeGradient,
                                   QStringLiteral(":/materials/ObjectGradientMaterial"));
                if (useGradient) {
                    updateCustomMaterial(model, false, false, barList.at(i)->texture);
                } else {
                    QList<QColor> rowColors = series->rowColors();
                    if (rowColors.size() == 0) {
                        barColor = baseColor;
                    } else {
                        int rowColorIndex = barList.at(i)->coord.x() % rowColors.size();
                        barColor = rowColors[rowColorIndex];
                    }
                    updatePrincipledMaterial(model, barColor, useGradient, false,
                                             barList.at(i)->texture);
                }
            }
        } else {
            for (int i = 0; i < barList.count(); i++) {
                QQuick3DModel *model = barList.at(i)->model;
                updateItemMaterial(model, useGradient, rangeGradient,
                                   QStringLiteral(":/materials/RangeGradientMaterial"));
                if (useGradient) {
                    updateCustomMaterial(model, false, false, barList.at(i)->texture);
                } else {
                    updatePrincipledMaterial(model, baseColor, useGradient, false,
                                             barList.at(i)->texture);
                }
            }
        }
    } else if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
        for (int i = 0; i < barList.count(); i++) {
            barList.at(i)->instancing->setRangeGradient(rangeGradient);
            if (!rangeGradient) {
                updateItemMaterial(barList.at(i)->model, useGradient, rangeGradient,
                                   QStringLiteral(":/materials/ObjectGradientMaterialInstancing"));
                if (useGradient) {
                    updateCustomMaterial(barList.at(i)->model, false, false, barList.at(i)->texture);
                } else {
                    updatePrincipledMaterial(barList.at(i)->model, QColor(Qt::white),
                                             useGradient, false, barList.at(i)->texture);
                }
            } else {
                updateItemMaterial(barList.at(i)->model, useGradient, rangeGradient,
                                   QStringLiteral(":/materials/RangeGradientMaterialInstancing"));
                if (useGradient) {
                    updateCustomMaterial(barList.at(i)->model, false, false, barList.at(i)->texture);
                } else {
                    updatePrincipledMaterial(barList.at(i)->model, baseColor,
                                             useGradient, false, barList.at(i)->texture);
                }
            }
        }
    }
}

void QQuickGraphsBars::updateItemMaterial(QQuick3DModel *item, bool useGradient, bool rangeGradient, const QString &materialName)
{
    QQmlListReference materialsRef(item, "materials");
    if (!rangeGradient) {
        if (materialsRef.size()) {
            QObject *material = materialsRef.at(0);
            if (useGradient && !material->objectName().contains(QStringLiteral("objectgradient"))) {
                // The item has an existing material which is principled or range gradient.
                // The item needs an object gradient material.
                QQuick3DCustomMaterial *objectGradientMaterial = createQmlCustomMaterial(
                    materialName);
                objectGradientMaterial->setParent(item);
                QObject *oldMaterial = materialsRef.at(0);
                materialsRef.replace(0, objectGradientMaterial);
                objectGradientMaterial->setObjectName("objectgradient");
                delete oldMaterial;
            } else if (!useGradient && !qobject_cast<QQuick3DPrincipledMaterial *>(material)) {
                // The item has an existing material which is object gradient or range gradient.
                // The item needs a principled material for uniform color.
                auto principledMaterial = new QQuick3DPrincipledMaterial();
                principledMaterial->setParent(item);
                QObject *oldCustomMaterial = materialsRef.at(0);
                materialsRef.replace(0, principledMaterial);
                delete oldCustomMaterial;
            }
        } else {
            if (useGradient) {
                // The item needs object gradient material.
                QQuick3DCustomMaterial *objectGradientMaterial = createQmlCustomMaterial(
                    materialName);
                objectGradientMaterial->setParent(item);
                materialsRef.append(objectGradientMaterial);
                objectGradientMaterial->setObjectName("objectgradient");
            } else {
                // The item needs a principled material.
                auto principledMaterial = new QQuick3DPrincipledMaterial();
                principledMaterial->setParent(item);
                materialsRef.append(principledMaterial);
            }
        }
    } else {
        if (materialsRef.size()) {
            QObject *material = materialsRef.at(0);
            if (!qobject_cast<QQuick3DCustomMaterial *>(material)
                || material->objectName().contains(QStringLiteral("objectgradient"))) {
                // The item has an existing material which is principled or object gradient.
                // The item needs a range gradient material.
                if (useGradient) {
                    QQuick3DCustomMaterial *customMaterial = createQmlCustomMaterial(
                        materialName);
                    customMaterial->setParent(item);
                    QObject *oldPrincipledMaterial = materialsRef.at(0);
                    materialsRef.replace(0, customMaterial);
                    delete oldPrincipledMaterial;
                } else  {
                    // The item needs a principled material for uniform color.
                    auto principledMaterial = new QQuick3DPrincipledMaterial();
                    principledMaterial->setParent(item);
                    QObject *oldCustomMaterial = materialsRef.at(0);
                    materialsRef.replace(0, principledMaterial);
                    delete oldCustomMaterial;
                }
            }
        } else {
            if (useGradient) {
                // The item needs a range gradient material.
                QQuick3DCustomMaterial *customMaterial = createQmlCustomMaterial(
                    materialName);
                customMaterial->setParent(item);
                materialsRef.append(customMaterial);
            } else {
                // The item needs a principled material for uniform color.
                auto principledMaterial = new QQuick3DPrincipledMaterial();
                principledMaterial->setParent(item);
                materialsRef.append(principledMaterial);
            }
        }
    }
}

void QQuickGraphsBars::updateCustomMaterial(QQuick3DModel *item, bool isHighlight,
                                            bool isMultiHighlight, QQuick3DTexture *texture)
{
    QQmlListReference materialsRef(item, "materials");
    auto customMaterial = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
    if (!customMaterial)
        return;
    QVariant textureInputAsVariant = customMaterial->property("custex");
    QQuick3DShaderUtilsTextureInput *textureInput
        = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();

    if (!isHighlight && !isMultiHighlight)
        textureInput->setTexture(texture);
    else
        textureInput->setTexture(isHighlight ? m_highlightTexture : m_multiHighlightTexture);
}

void QQuickGraphsBars::updatePrincipledMaterial(QQuick3DModel *model, const QColor &color,
                                                bool useGradient, bool isHighlight,
                                                QQuick3DTexture *texture)
{
    QQmlListReference materialsRef(model, "materials");
    auto principledMaterial = qobject_cast<QQuick3DPrincipledMaterial *>(materialsRef.at(0));
    if (!principledMaterial)
        return;
    principledMaterial->setParent(this);

    if (useGradient) {
        principledMaterial->setBaseColor(QColor(Qt::white));
        if (!isHighlight)
            principledMaterial->setBaseColorMap(texture);
        else
            principledMaterial->setBaseColorMap(m_highlightTexture);
    } else {
        principledMaterial->setBaseColor(color);
    }
}

void QQuickGraphsBars::removeBarModels()
{
    deleteBarItemHolders();
    for (const auto list : std::as_const(m_barModelsMap)) {
        for (auto barModel : *list) {
            deleteBarModels(barModel);
        }
        delete list;
    }

    m_barModelsMap.clear();
}

void QQuickGraphsBars::deleteBarModels(BarModel *barModel)
{
    barModel->model->setPickable(false);
    barModel->model->setVisible(false);
    QQmlListReference materialsRef(barModel->model, "materials");
    if (materialsRef.size()) {
        auto material = materialsRef.at(0);
        delete material;
    }
    delete barModel->model;
    delete barModel;
}

void QQuickGraphsBars::deleteBarItemHolders()
{
    for (const auto list : std::as_const(m_barModelsMap)) {
        for (auto barModel : *list) {
            QList<BarItemHolder *> barItemList = barModel->instancing->dataArray();
            for (auto bih : barItemList)
                delete bih;
        }
    }
}

QQuick3DTexture *QQuickGraphsBars::createTexture()
{
    QQuick3DTexture *texture = new QQuick3DTexture();
    texture->setParent(this);
    texture->setRotationUV(-90.0f);
    texture->setHorizontalTiling(QQuick3DTexture::ClampToEdge);
    texture->setVerticalTiling(QQuick3DTexture::ClampToEdge);
    QuickGraphsTextureData *textureData = new QuickGraphsTextureData();
    textureData->setParent(texture);
    textureData->setParentItem(texture);
    texture->setTextureData(textureData);

    return texture;
}

bool QQuickGraphsBars::handleMousePressedEvent(QMouseEvent *event)
{
    m_selectionDirty = true;

    if (!QQuickGraphsItem::handleMousePressedEvent(event))
        return true;

    createSliceView();

    if (Qt::LeftButton == event->button())
        doPicking(event->pos());

    return true;
}

bool QQuickGraphsBars::handleTouchEvent(QTouchEvent *event)
{
    m_selectionDirty = true;

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

    QList<QQuick3DPickResult> pickResults = pickAll(position.x(), position.y());
    QQuick3DModel *selectedModel = nullptr;
    int instanceInd = 0;
    if (!m_selectionMode.testFlag(QAbstract3DGraph::SelectionNone)) {
        for (const auto &picked : std::as_const(pickResults)) {
            if (picked.objectHit()->visible()) {
                if (picked.objectHit() == backgroundBB() || picked.objectHit() == background()) {
                    resetClickedStatus();
                    continue;
                } else if (picked.objectHit()->objectName().contains(QStringLiteral("BarModel"))) {
                    if (optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
                        selectedModel = picked.objectHit();
                        break;
                    } else if (optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
                        selectedModel = picked.objectHit();
                        // Prevents to select bars with a height of 0 which affect picking.
                        if (selectedModel->instancing()->instancePosition(picked.instanceIndex()).y()
                            != 0) {
                            instanceInd = picked.instanceIndex();
                            break;
                        }
                    }
                }
            }
        }

        if (selectedModel) {
            QBar3DSeries *series = 0;
            QPoint coord = m_barsController->invalidSelectionPosition();
            for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++) {
                QList<BarModel *> barList = *it.value();
                if (!it.key()->isVisible())
                    continue;
                for (int i = 0; i < barList.size(); i++) {
                    if (optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
                        QQuick3DModel *model = barList.at(i)->model;
                        if (model == selectedModel) {
                            series = it.key();
                            coord = barList.at(i)->coord;
                        }
                    } else if (optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
                        QList<BarItemHolder *> barItemList = barList.at(i)->instancing->dataArray();
                        auto itemPos = barItemList.at(instanceInd)->position;
                        auto selected = selectedModel->instancing()->instancePosition(instanceInd);
                        if (itemPos == selected) {
                            series = it.key();
                            coord = barItemList.at(instanceInd)->coord;
                            m_selectedBarPos = barItemList.at(instanceInd)->position;
                        }
                    }
                }
            }
            setSelectedBar(series, coord);
        } else {
            resetClickedStatus();
        }
    }
    return true;
}

void QQuickGraphsBars::setSelectedBar(QBar3DSeries *series, const QPoint &coord)
{
    if (!m_barModelsMap.contains(series))
        series = 0;

    if (coord != m_selectedBarCoord || series != m_selectedBarSeries) {
        m_selectedBarSeries = series;
        m_selectedBarCoord = coord;
        if (isSliceEnabled())
            setSliceActivatedChanged(true);

        // Clear selection from other series and finally set new selection to the specified series
        for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++) {
            if (it.key() != m_selectedBarSeries)
                it.key()->d_func()->setSelectedBar(invalidSelectionPosition());
        }
        if (m_selectedBarSeries) {
            m_selectedBarSeries->d_func()->setSelectedBar(m_selectedBarCoord);
            m_barsController->setSelectedBar(m_selectedBarCoord, m_selectedBarSeries, false);
        }
        m_barsController->setSeriesVisualsDirty(true);
    }
}

void QQuickGraphsBars::updateSelectedBar()
{
    bool visible = false;
    for (auto it = m_barModelsMap.begin(); it != m_barModelsMap.end(); it++) {
        if (m_selectedBarSeries && it.key()->isVisible()) {
            bool useGradient = m_selectedBarSeries->d_func()->isUsingGradient();
            QString label = m_selectedBarSeries->itemLabel();
            if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
                for (auto barList : *it.value()) {
                    Bars3DController::SelectionType selectionType =
                        isSelected(barList->coord.x(), barList->coord.y(), it.key());
                    switch (selectionType) {
                    case Bars3DController::SelectionItem: {
                        if (useGradient) {
                            updateCustomMaterial(barList->model, true, false,
                                                 barList->texture);
                        } else {
                            updatePrincipledMaterial(barList->model,
                                                     it.key()->singleHighlightColor(), useGradient,
                                                     true, barList->texture);
                        }

                        m_selectedBarPos = barList->model->position();
                        visible = m_selectedBarSeries->isVisible() && !m_selectedBarPos.isNull();
                        QString label = (m_selectedBarSeries->d_func()->itemLabel());

                        if (barList->heightValue >= 0.0f) {
                            m_selectedBarPos.setY(m_selectedBarPos.y() + barList->heightValue
                                                  + 0.2f);
                        } else {
                            m_selectedBarPos.setY(m_selectedBarPos.y() + barList->heightValue
                                                  - 0.2f);
                        }

                        updateItemLabel(m_selectedBarPos);
                        itemLabel()->setProperty("labelText", label);

                        if (isSliceEnabled()) {
                            QFontMetrics fm(m_barsController->activeTheme()->font());
                            float textPadding = m_barsController->activeTheme()->font().pointSizeF() * .5f;
                            float labelHeight = fm.height() + textPadding;
                            float labelWidth = fm.horizontalAdvance(label) + textPadding;
                            QVector3D scale = sliceItemLabel()->scale();
                            scale.setX(scale.y() * labelWidth / labelHeight);
                            sliceItemLabel()->setProperty("labelWidth", labelWidth);
                            sliceItemLabel()->setProperty("labelHeight", labelHeight);
                            QVector3D slicePos = barList->model->position();
                            if (m_barsController->selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
                                slicePos.setX(slicePos.z() - .1f);
                            else if (m_barsController->selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
                                slicePos.setX(slicePos.x() - .1f);
                            slicePos.setZ(.0f);
                            slicePos.setY(slicePos.y() + 1.5f);
                            sliceItemLabel()->setPosition(slicePos);
                            sliceItemLabel()->setProperty("labelText", label);
                            sliceItemLabel()->setEulerRotation(QVector3D(0.0f, 0.0f, 90.0f));
                            sliceItemLabel()->setVisible(true);
                        }
                        break;
                    }
                    case Bars3DController::SelectionRow:
                    case Bars3DController::SelectionColumn: {
                        if (useGradient) {
                            updateCustomMaterial(barList->model, false, true,
                                                 barList->texture);
                        } else {
                            updatePrincipledMaterial(barList->model,
                                                     it.key()->multiHighlightColor(), useGradient,
                                                     true, barList->texture);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            } else if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
                bool rangeGradient = (useGradient && it.key()->d_func()->m_colorStyle
                                                         == Q3DTheme::ColorStyleRangeGradient);
                int index = 0;
                QList<BarModel *> barList = *m_barModelsMap.value(it.key());
                QList<BarItemHolder *> barItemList = barList.at(0)->instancing->dataArray();
                for (auto bih : barItemList) {
                    Bars3DController::SelectionType selectionType =
                        isSelected(bih->coord.x(), bih->coord.y(), it.key());
                    switch (selectionType) {
                    case Bars3DController::SelectionItem: {
                        if (index <= m_selectedModels.value(it.key())->size()) {
                            visible = m_selectedBarSeries->isVisible() && !m_selectedBarPos.isNull();
                            bih->selectedBar = true;
                            QQuick3DModel *selectedModel = m_selectedModels.value(it.key())->at(index);
                            selectedModel->setVisible(true);
                            selectedModel->setPosition(bih->position);
                            selectedModel->setScale(bih->scale);

                            if (!rangeGradient) {
                                updateItemMaterial(selectedModel, useGradient, rangeGradient,
                                                   QStringLiteral(":/materials/ObjectGradientMaterial"));
                                if (useGradient) {
                                    updateCustomMaterial(selectedModel, true, false,
                                                         barList.at(0)->texture);
                                } else {
                                    updatePrincipledMaterial(selectedModel,
                                                             it.key()->singleHighlightColor(), useGradient,
                                                             true, barList.at(0)->texture);
                                }
                            } else {
                                updateItemMaterial(selectedModel, useGradient, rangeGradient,
                                                   QStringLiteral(":/materials/RangeGradientMaterial"));
                                if (useGradient) {
                                    updateCustomMaterial(selectedModel, true, false,
                                                         barList.at(0)->texture);
                                } else {
                                    updatePrincipledMaterial(selectedModel,
                                                             it.key()->singleHighlightColor(), useGradient,
                                                             true, barList.at(0)->texture);
                                }
                            }

                            m_selectedBarPos = bih->position;
                            QString label = (m_selectedBarSeries->d_func()->itemLabel());

                            if (bih->heightValue >= 0.0f)
                                m_selectedBarPos.setY(m_selectedBarPos.y() + bih->heightValue + 0.2f);
                            else
                                m_selectedBarPos.setY(m_selectedBarPos.y() + bih->heightValue - 0.2f);

                            updateItemLabel(m_selectedBarPos);
                            itemLabel()->setProperty("labelText", label);

                            if (isSliceEnabled()) {
                                QFontMetrics fm(m_barsController->activeTheme()->font());
                                float textPadding = m_barsController->activeTheme()->font().pointSizeF() * .5f;
                                float labelHeight = fm.height() + textPadding;
                                float labelWidth = fm.horizontalAdvance(label) + textPadding;
                                QVector3D scale = sliceItemLabel()->scale();
                                scale.setX(scale.y() * labelWidth / labelHeight);
                                sliceItemLabel()->setProperty("labelWidth", labelWidth);
                                sliceItemLabel()->setProperty("labelHeight", labelHeight);
                                QVector3D slicePos = bih->position;
                                if (m_barsController->selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
                                    slicePos.setX(slicePos.z() - .1f);
                                else if (m_barsController->selectionMode().testFlag(QAbstract3DGraph::SelectionRow))
                                    slicePos.setX(slicePos.x() - .1f);
                                slicePos.setZ(.0f);
                                slicePos.setY(slicePos.y() + 1.5f);
                                sliceItemLabel()->setPosition(slicePos);
                                sliceItemLabel()->setProperty("labelText", label);
                                sliceItemLabel()->setEulerRotation(QVector3D(0.0f, 0.0f, 90.0f));
                                sliceItemLabel()->setVisible(true);
                            }
                            ++index;
                        }
                        break;
                    }
                    case Bars3DController::SelectionRow:
                    case Bars3DController::SelectionColumn: {
                        if (index <= m_selectedModels.value(it.key())->size()) {
                            bih->selectedBar = true;
                            QQuick3DModel *selectedModel = m_selectedModels.value(it.key())->at(index);
                            selectedModel->setVisible(true);
                            selectedModel->setPosition(bih->position);
                            selectedModel->setScale(bih->scale);

                            if (!rangeGradient) {
                                updateItemMaterial(selectedModel, useGradient, rangeGradient,
                                                   QStringLiteral(":/materials/ObjectGradientMaterial"));
                                if (useGradient) {
                                    updateCustomMaterial(selectedModel, false, true,
                                                         barList.at(0)->texture);
                                } else {
                                    updatePrincipledMaterial(selectedModel,
                                                             it.key()->multiHighlightColor(), useGradient,
                                                             true, barList.at(0)->texture);
                                }
                            } else {
                                updateItemMaterial(selectedModel, useGradient, rangeGradient,
                                                   QStringLiteral(":/materials/RangeGradientMaterial"));
                                if (useGradient) {
                                    updateCustomMaterial(selectedModel, false, true,
                                                         barList.at(0)->texture);
                                } else {
                                    updatePrincipledMaterial(selectedModel,
                                                             it.key()->multiHighlightColor(), useGradient,
                                                             true, barList.at(0)->texture);
                                }
                            }
                            ++index;
                        }
                        break;
                    }
                    default:
                        break;
                    }

                }
            }
        }
    }
    itemLabel()->setVisible(visible);
}

void QQuickGraphsBars::createSelectedModels(QBar3DSeries *series)
{
    QList<QQuick3DModel *> *selectedModelsList = m_selectedModels.value(series);
    if (!selectedModelsList) {
        selectedModelsList = new QList<QQuick3DModel *>;
        m_selectedModels[series] = selectedModelsList;
    }
    int selectedModelsListSize = 1;
    int rowCount = series->dataProxy()->rowCount();
    int colCount = series->dataProxy()->colCount();
    if (m_selectionMode.testFlag(QAbstract3DGraph::SelectionRow))
        selectedModelsListSize = colCount;
    else if (m_selectionMode.testFlag(QAbstract3DGraph::SelectionColumn))
        selectedModelsListSize = rowCount;

    if (m_selectionMode.testFlag(QAbstract3DGraph::SelectionRow) &&
        m_selectionMode.testFlag(QAbstract3DGraph::SelectionColumn)) {
        selectedModelsListSize = rowCount + colCount - 1;
    }

    for (int ind = 0; ind < selectedModelsListSize; ++ind) {
        QQuick3DModel *model = createDataItem(QQuick3DViewport::scene(), series);
        model->setVisible(false);

        if (!selectedModelsList->contains(model))
            selectedModelsList->append(model);
    }
}

Abstract3DController::SelectionType QQuickGraphsBars::isSelected(int row, int bar,
                                                                 QBar3DSeries *series)
{
    Bars3DController::SelectionType isSelectedType = Bars3DController::SelectionNone;
    if ((m_selectionMode.testFlag(QAbstract3DGraph::SelectionMultiSeries)
         && m_selectedBarSeries) || series == m_selectedBarSeries) {
        if (row == m_selectedBarCoord.x() && bar == m_selectedBarCoord.y()
                && (m_selectionMode.testFlag(QAbstract3DGraph::SelectionItem))) {
            isSelectedType = Bars3DController::SelectionItem;
        } else if (row == m_selectedBarCoord.x()
                   && (m_selectionMode.testFlag(QAbstract3DGraph::SelectionRow))) {
            isSelectedType = Bars3DController::SelectionRow;
        } else if (bar == m_selectedBarCoord.y()
                   && (m_selectionMode.testFlag(QAbstract3DGraph::SelectionColumn))) {
            isSelectedType = Bars3DController::SelectionColumn;
        }
    }

    return isSelectedType;
}

void QQuickGraphsBars::resetClickedStatus()
{
    m_barsController->setSeriesVisualsDirty(true);
    m_selectedBarPos = QVector3D(0.0f, 0.0f, 0.0f);
    m_selectedBarCoord = Bars3DController::invalidSelectionPosition();
    m_selectedBarSeries = 0;
    m_barsController->clearSelection();

    if (optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
        for (const auto list : std::as_const(m_selectedModels)) {
            for (auto selectedModel : *list)
                selectedModel->setVisible(false);
        }
        for (const auto list : std::as_const(m_barModelsMap)) {
            QList<BarItemHolder *> barItemList = list->at(0)->instancing->dataArray();
            for (auto bih : barItemList)
                bih->selectedBar = false;
        }
    }
    m_barsController->setSeriesVisualsDirty(true);
}

void QQuickGraphsBars::createSliceView()
{
    QQuickGraphsItem::createSliceView();

    QQuick3DViewport *sliceParent = sliceView();

    QList<QBar3DSeries *> barSeriesList = m_barsController->barSeriesList();
    for (const auto &barSeries : std::as_const(barSeriesList)) {
        bool useGradient = barSeries->d_func()->isUsingGradient();
        bool rangeGradient = (useGradient && barSeries->d_func()->m_colorStyle
                                                 == Q3DTheme::ColorStyleRangeGradient);
        QList<QQuick3DModel *> *slicedBarList = m_slicedBarModels.value(barSeries);
        if (!slicedBarList) {
            slicedBarList = new QList<QQuick3DModel *>;
            m_slicedBarModels[barSeries] = slicedBarList;
        }
        if (slicedBarList->isEmpty()) {
            int slicedBarListSize = 0;
            if (m_selectionMode.testFlag(QAbstract3DGraph::SelectionRow))
                slicedBarListSize = barSeries->dataProxy()->colCount();
            else if (m_selectionMode.testFlag(QAbstract3DGraph::SelectionColumn))
                slicedBarListSize = barSeries->dataProxy()->rowCount();

            for (int ind = 0; ind < slicedBarListSize; ++ind) {
                QQuick3DModel *model = createDataItem(sliceParent->scene(), barSeries);
                model->setVisible(false);
                if (!rangeGradient) {
                    updateItemMaterial(model, useGradient, rangeGradient,
                                       QStringLiteral(":/materials/ObjectGradientMaterial"));
                } else {
                    updateItemMaterial(model, useGradient, rangeGradient,
                                       QStringLiteral(":/materials/RangeGradientMaterial"));
                }

                if (!slicedBarList->contains(model))
                    slicedBarList->append(model);
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
        m_barsController->m_changeTracker.selectedBarChanged = false;
        return;
    }

    int index = 0;
    bool rowMode = m_selectionMode.testFlag(QAbstract3DGraph::SelectionRow);
    for (auto it = m_slicedBarModels.begin(); it != m_slicedBarModels.end(); it++) {
        QList<BarModel *> barList = *m_barModelsMap.value(it.key());
        bool useGradient = it.key()->d_func()->isUsingGradient();
        if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
            for (int ind = 0; ind < it.value()->count(); ++ind) {
                if (rowMode)
                    index = (m_selectedBarCoord.x() * it.key()->dataProxy()->colCount()) + ind;
                else
                    index = m_selectedBarCoord.y() + (ind * it.key()->dataProxy()->colCount());
                bool visible = ((m_selectedBarSeries == it.key()
                                 || m_selectionMode.testFlag(QAbstract3DGraph::SelectionMultiSeries))
                                && it.key()->isVisible());

                if (index < barList.size() && m_selectedBarCoord != invalidSelectionPosition()) {
                    QQuick3DModel *sliceBarModel = it.value()->at(ind);
                    BarModel *barModel = barList.at(index);

                    sliceBarModel->setVisible(visible);
                    if (rowMode) {
                        sliceBarModel->setPosition(QVector3D(barModel->model->x(),
                                                                    barModel->model->y(), 0.0f));
                    } else {
                        sliceBarModel->setX(barModel->model->z()
                                                   + (barModel->visualIndex * .2f));
                        sliceBarModel->setY(barModel->model->y());
                        sliceBarModel->setZ(0.0f);
                    }
                    sliceBarModel->setScale(barModel->model->scale());
                    bool highlightBar = (ind == (rowMode ? m_selectedBarCoord.y()
                                                         : m_selectedBarCoord.x()));
                    if (useGradient) {
                        updateCustomMaterial(sliceBarModel, highlightBar, false, barList.at(index)->texture);
                    } else {
                        updatePrincipledMaterial(sliceBarModel,
                                                 highlightBar ? m_selectedBarSeries->singleHighlightColor()
                                                              : m_selectedBarSeries->baseColor(),
                                                 useGradient, highlightBar, barList.at(index)->texture);
                    }
                } else {
                    setSliceEnabled(false);
                    QQuickGraphsItem::updateSliceGraph();
                    return;
                }
            }
        } else if (m_barsController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
            QList<BarItemHolder *> barItemList = barList.at(0)->instancing->dataArray();
            if (!barItemList.isEmpty()) {
                for (int ind = 0; ind < it.value()->size(); ++ind) {
                    if (rowMode)
                        index = (m_selectedBarCoord.x() * it.key()->dataProxy()->colCount()) + ind;
                    else
                        index = m_selectedBarCoord.y() + (ind * it.key()->dataProxy()->colCount());
                    bool visible = ((m_selectedBarSeries == it.key()
                                     || m_selectionMode.testFlag(QAbstract3DGraph::SelectionMultiSeries))
                                    && it.key()->isVisible());

                    if (index < barItemList.size() && m_selectedBarCoord != invalidSelectionPosition()) {
                        QQuick3DModel *sliceBarModel = it.value()->at(ind);
                        BarItemHolder *bih = barItemList.at(index);

                        sliceBarModel->setVisible(visible);

                        if (rowMode) {
                            sliceBarModel->setPosition(QVector3D(bih->position.x(),
                                                                 bih->position.y(), 0.0f));
                        } else {
                            sliceBarModel->setX(bih->position.z()
                                                + (barList.at(0)->visualIndex * .2f));
                            sliceBarModel->setY(bih->position.y());
                            sliceBarModel->setZ(0.0f);
                        }
                        sliceBarModel->setScale(bih->scale);
                        bool highlightBar = (ind == (rowMode ? m_selectedBarCoord.y()
                                                             : m_selectedBarCoord.x()));
                        if (useGradient) {
                            updateCustomMaterial(sliceBarModel, highlightBar, false, barList.at(0)->texture);
                        } else {
                            updatePrincipledMaterial(sliceBarModel,
                                                     highlightBar ? m_selectedBarSeries->singleHighlightColor()
                                                                  : m_selectedBarSeries->baseColor(),
                                                     useGradient, highlightBar, barList.at(0)->texture);
                        }
                    } else {
                        setSliceEnabled(false);
                        QQuickGraphsItem::updateSliceGraph();
                        return;
                    }
                }
            }
        }
    }
}

void QQuickGraphsBars::handleLabelCountChanged(QQuick3DRepeater *repeater)
{
    QQuickGraphsItem::handleLabelCountChanged(repeater);

    if (repeater == repeaterX())
        handleColCountChanged();
    if (repeater == repeaterZ())
        handleRowCountChanged();
}

void QQuickGraphsBars::removeSlicedBarModels()
{
    for (const auto list : std::as_const(m_slicedBarModels)) {
        for (auto model : *list) {
            model->setPickable(false);
            model->setVisible(false);
            QQmlListReference materialsRef(model, "materials");
            if (materialsRef.size()) {
                auto material = materialsRef.at(0);
                delete material;
            }
            delete model;
        }
        delete list;
    }
    m_slicedBarModels.clear();
}

void QQuickGraphsBars::removeSelectedModels()
{
    for (const auto list : std::as_const(m_selectedModels)) {
        for (auto selectedModel : *list) {
            selectedModel->setPickable(false);
            selectedModel->setVisible(false);
            QQmlListReference materialsRef(selectedModel, "materials");
            if (materialsRef.size()) {
                auto material = materialsRef.at(0);
                delete material;
            }
            delete selectedModel;
        }
        delete list;
    }
    m_selectedModels.clear();
    m_barsController->setSelectedBar(m_selectedBarCoord, m_selectedBarSeries, false);
}

void QQuickGraphsBars::updateSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    if (mode.testFlag(QAbstract3DGraph::SelectionSlice) && m_selectedBarSeries) {
        setSliceActivatedChanged(true);
        if (sliceView() && isSliceEnabled()) {
            m_selectionDirty = false;
        } else {
            setSliceEnabled(true);
            m_selectionDirty = true;
        }
    }

    m_selectionMode = mode;

    if (optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
        for (const auto list : std::as_const(m_barModelsMap)) {
            QList<BarItemHolder *> barItemList = list->at(0)->instancing->dataArray();
            for (auto bih : barItemList)
                bih->selectedBar = false;
        }
    }

    removeSelectedModels();
    QList<QBar3DSeries *> barSeriesList = m_barsController->barSeriesList();
    for (const auto &series : std::as_const(barSeriesList)) {
        if (m_barModelsMap.contains(series))
            createSelectedModels(series);
    }

    m_barsController->setSeriesVisualsDirty(true);
    itemLabel()->setVisible(false);
}

void QQuickGraphsBars::updateBarSpecs(float thicknessRatio, const QSizeF &spacing, bool relative)
{
    // Convert ratio to QSizeF, as we need it in that format for autoscaling calculations
    m_cachedBarThickness.setWidth(1.0);
    m_cachedBarThickness.setHeight(1.0f / thicknessRatio);

    if (relative) {
        m_cachedBarSpacing.setWidth((m_cachedBarThickness.width() * 2)
                                    * (spacing.width() + 1.0f));
        m_cachedBarSpacing.setHeight((m_cachedBarThickness.height() * 2)
                                     * (spacing.height() + 1.0f));
    } else {
        m_cachedBarSpacing = m_cachedBarThickness * 2 + spacing * 2;
    }

    m_axisRangeChanged = true;
    m_barsController->m_changeTracker.selectedBarChanged = true;

    // Calculate here and at setting sample space
    calculateSceneScalingFactors();
}

void QQuickGraphsBars::updateBarSeriesMargin(const QSizeF &margin)
{
    m_cachedBarSeriesMargin = margin;
    calculateSeriesStartPosition();
    calculateSceneScalingFactors();
    m_barsController->setSeriesVisualsDirty(true);
}
