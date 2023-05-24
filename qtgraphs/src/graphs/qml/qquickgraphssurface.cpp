// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "private/qquick3drepeater_p.h"
#include "qquickgraphssurface_p.h"
#include <QtCore/QMutexLocker>

#include "declarativescene_p.h"
#include "surface3dcontroller_p.h"
#include "surfaceselectioninstancing_p.h"
#include "qvalue3daxis_p.h"
#include "qcategory3daxis_p.h"
#include "quickgraphstexturedata_p.h"

#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>

QT_BEGIN_NAMESPACE

QQuickGraphsSurface::QQuickGraphsSurface(QQuickItem *parent)
    : QQuickGraphsItem(parent),
      m_surfaceController(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);

    // Create the shared component on the main GUI thread.
    m_surfaceController = new Surface3DController(boundingRect().toRect(), new Declarative3DScene);
    setSharedController(m_surfaceController);

    QObject::connect(m_surfaceController, &Surface3DController::selectedSeriesChanged,
                     this, &QQuickGraphsSurface::selectedSeriesChanged);
    QObject::connect(m_surfaceController, &Surface3DController::flipHorizontalGridChanged,
                     this, &QQuickGraphsSurface::handleFlipHorizontalGridChanged);
}

QQuickGraphsSurface::~QQuickGraphsSurface()
{
    QMutexLocker locker(m_nodeMutex.data());
    const QMutexLocker locker2(mutex());
    delete m_surfaceController;
    for (auto model : m_model)
        delete model;
    delete m_instancing;
    delete m_sliceInstancing;
}

QValue3DAxis *QQuickGraphsSurface::axisX() const
{
    return static_cast<QValue3DAxis *>(m_surfaceController->axisX());
}

void QQuickGraphsSurface::setAxisX(QValue3DAxis *axis)
{
    m_surfaceController->setAxisX(axis);
}

QValue3DAxis *QQuickGraphsSurface::axisY() const
{
    return static_cast<QValue3DAxis *>(m_surfaceController->axisY());
}

void QQuickGraphsSurface::setAxisY(QValue3DAxis *axis)
{
    m_surfaceController->setAxisY(axis);
}

QValue3DAxis *QQuickGraphsSurface::axisZ() const
{
    return static_cast<QValue3DAxis *>(m_surfaceController->axisZ());
}

void QQuickGraphsSurface::setAxisZ(QValue3DAxis *axis)
{
    m_surfaceController->setAxisZ(axis);
}

void QQuickGraphsSurface::handleFlatShadingEnabledChanged()
{
    auto series = static_cast<QSurface3DSeries *>(sender());
    for (auto model : m_model) {
        if (model->series == series) {
            updateModel(model);
            break;
        }
    }
}

void QQuickGraphsSurface::handleWireframeColorChanged()
{
    for (auto model : m_model) {
        QQmlListReference gridMaterialRef(model->gridModel, "materials");
        auto gridMaterial = gridMaterialRef.at(0);
        QColor gridColor = model->series->wireframeColor();
        gridMaterial->setProperty("gridColor", gridColor);

        if (sliceView()) {
            QQmlListReference gridMaterialRef(model->sliceGridModel, "materials");
            auto gridMaterial = static_cast<QQuick3DPrincipledMaterial *>(gridMaterialRef.at(0));
            gridMaterial->setBaseColor(gridColor);
        }
    }
}

void QQuickGraphsSurface::handleFlipHorizontalGridChanged(bool flip)
{
    if (!segmentLineRepeaterX()
            || !segmentLineRepeaterZ()) {
        return;
    }
    int gridLineCountX = segmentLineRepeaterX()->count();
    int subGridLineCountX = subsegmentLineRepeaterX()->count();
    int gridLineCountZ = segmentLineRepeaterZ()->count();
    int subGridLineCountZ = subsegmentLineRepeaterZ()->count();

    float factor = -1.0f;
    if (isGridUpdated())
        factor = flip ? -1.0f : 1.0f;

    for (int i = 0; i < subGridLineCountZ; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterZ()->objectAt(i));
        QVector3D pos = lineNode->position();
        pos.setY(pos.y() * factor);
        lineNode->setPosition(pos);
    }
    for (int i = 0; i < gridLineCountZ; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterZ()->objectAt(i));
        QVector3D pos = lineNode->position();
        pos.setY(pos.y() * factor);
        lineNode->setPosition(pos);
    }
    for (int i = 0; i < subGridLineCountX; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(subsegmentLineRepeaterX()->objectAt(i));
        QVector3D pos = lineNode->position();
        pos.setY(pos.y() * factor);
        lineNode->setPosition(pos);
    }
    for (int i = 0; i < gridLineCountX; i++) {
        QQuick3DNode *lineNode = static_cast<QQuick3DNode *>(segmentLineRepeaterX()->objectAt(i));
        QVector3D pos = lineNode->position();
        pos.setY(pos.y() * factor);
        lineNode->setPosition(pos);
    }

    for (int i = 0; i < repeaterX()->count(); i++) {
        QQuick3DNode *obj = static_cast<QQuick3DNode *>(repeaterX()->objectAt(i));
        QVector3D pos = obj->position();
        pos.setY(pos.y() * factor);
        obj->setPosition(pos);
    }

    for (int i = 0; i < repeaterZ()->count(); i++) {
        QQuick3DNode *obj = static_cast<QQuick3DNode *>(repeaterZ()->objectAt(i));
        QVector3D pos = obj->position();
        pos.setY(pos.y() * factor);
        obj->setPosition(pos);
    }

    QVector3D pos = titleLabelX()->position();
    pos.setY(pos.y() * factor);
    titleLabelX()->setPosition(pos);

    pos = titleLabelZ()->position();
    pos.setY(pos.y() * factor);
    titleLabelZ()->setPosition(pos);

    setGridUpdated(false);
    emit flipHorizontalGridChanged(flip);
    m_surfaceController->setFlipHorizontalGridChanged(false);
}

QSurface3DSeries *QQuickGraphsSurface::selectedSeries() const
{
    return m_surfaceController->selectedSeries();
}

void QQuickGraphsSurface::setFlipHorizontalGrid(bool flip)
{
    m_surfaceController->setFlipHorizontalGrid(flip);
}

bool QQuickGraphsSurface::flipHorizontalGrid() const
{
    return m_surfaceController->flipHorizontalGrid();
}

QQmlListProperty<QSurface3DSeries> QQuickGraphsSurface::seriesList()
{
    return QQmlListProperty<QSurface3DSeries>(this, this,
                                              &QQuickGraphsSurface::appendSeriesFunc,
                                              &QQuickGraphsSurface::countSeriesFunc,
                                              &QQuickGraphsSurface::atSeriesFunc,
                                              &QQuickGraphsSurface::clearSeriesFunc);
}

void QQuickGraphsSurface::appendSeriesFunc(QQmlListProperty<QSurface3DSeries> *list,
                                           QSurface3DSeries *series)
{
    reinterpret_cast<QQuickGraphsSurface *>(list->data)->addSeries(series);
}

qsizetype QQuickGraphsSurface::countSeriesFunc(QQmlListProperty<QSurface3DSeries> *list)
{
    return reinterpret_cast<QQuickGraphsSurface *>(list->data)->m_surfaceController->surfaceSeriesList().size();
}

QSurface3DSeries *QQuickGraphsSurface::atSeriesFunc(QQmlListProperty<QSurface3DSeries> *list,
                                                    qsizetype index)
{
    return reinterpret_cast<QQuickGraphsSurface *>(list->data)->m_surfaceController->surfaceSeriesList().at(index);
}

void QQuickGraphsSurface::clearSeriesFunc(QQmlListProperty<QSurface3DSeries> *list)
{
    QQuickGraphsSurface *declSurface = reinterpret_cast<QQuickGraphsSurface *>(list->data);
    QList<QSurface3DSeries *> realList = declSurface->m_surfaceController->surfaceSeriesList();
    int count = realList.size();
    for (int i = 0; i < count; i++)
        declSurface->removeSeries(realList.at(i));
}

void QQuickGraphsSurface::addSeries(QSurface3DSeries *series)
{
    m_surfaceController->addSeries(series);
    if (isReady())
        addModel(series);
}

void QQuickGraphsSurface::removeSeries(QSurface3DSeries *series)
{
    m_surfaceController->removeSeries(series);
    series->setParent(this); // Reparent as removing will leave series parentless
    for (int i = 0; i < m_model.size();) {
        if (m_model[i]->series == series) {
            m_model[i]->model->deleteLater();
            m_model[i]->gridModel->deleteLater();
            m_model[i]->proxyModel->deleteLater();
            if (sliceView()) {
                m_model[i]->sliceModel->deleteLater();
                m_model[i]->sliceGridModel->deleteLater();
            }
            m_model.removeAt(i);
        } else {
            ++i;
        }
    }
}

void QQuickGraphsSurface::handleAxisXChanged(QAbstract3DAxis *axis)
{
    emit axisXChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsSurface::handleAxisYChanged(QAbstract3DAxis *axis)
{
    emit axisYChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsSurface::handleAxisZChanged(QAbstract3DAxis *axis)
{
    emit axisZChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsSurface::componentComplete()
{
    QQuickGraphsItem::componentComplete();

    for (auto series : m_surfaceController->surfaceSeriesList())
        addModel(series);

    QQuick3DNode *parent = rootNode();

    m_selectionPointer = new QQuick3DModel();
    m_selectionPointer->setParent(parent);
    m_selectionPointer->setParentItem(parent);
    m_selectionPointer->setSource(QUrl(QStringLiteral("#Sphere")));
    auto pointerMaterial = new QQuick3DPrincipledMaterial();
    pointerMaterial->setParent(this);
    pointerMaterial->setBaseColor(m_surfaceController->activeTheme()->singleHighlightColor());
    QQmlListReference materialRef(m_selectionPointer, "materials");
    materialRef.append(pointerMaterial);
    m_instancing = new SurfaceSelectionInstancing();
    m_instancing->setScale(QVector3D(0.001f, 0.001f, 0.001f));
    m_selectionPointer->setInstancing(m_instancing);
}

void QQuickGraphsSurface::synchData()
{
    QQuickGraphsItem::synchData();

    if (m_surfaceController->isSelectedPointChanged()) {
        if (m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionItem))
            updateSelectedPoint();
        m_surfaceController->setSelectedPointChanged(false);
    }

    if (isGridUpdated() || m_surfaceController->isFlipHorizontalGridChanged())
        handleFlipHorizontalGridChanged(m_surfaceController->flipHorizontalGrid());

    if (m_surfaceController->isSurfaceTextureChanged()) {
        if (!m_surfaceController->isChangedTexturesEmpty()) {
            for (auto model : m_model) {
                if (m_surfaceController->hasSeriesToChangeTexture(model->series))
                    updateMaterial(model);
            }
        }
        m_surfaceController->setSurfaceTextureChanged(false);
    }
}

void QQuickGraphsSurface::updateGraph()
{
    for (auto model : m_model) {
        bool seriesVisible = model->series->isVisible();
        if (m_surfaceController->isSeriesVisibilityDirty()) {
            bool graphVisible = (model->model->visible() || model->gridModel->visible());

            if (seriesVisible != graphVisible && m_surfaceController->isSlicingActive()) {
                setSliceActivatedChanged(true);
            }
            if (!seriesVisible) {
                model->model->setVisible(seriesVisible);
                model->gridModel->setVisible(seriesVisible);
                if (sliceView()) {
                    model->sliceModel->setVisible(seriesVisible);
                    model->sliceGridModel->setVisible(seriesVisible);
                }
                continue;
            }
        }

        if (model->gridModel->visible() != seriesVisible)
            model->gridModel->setVisible(seriesVisible);
        if (model->model->visible() != seriesVisible)
            model->model->setVisible(seriesVisible);
        model->gridModel->setVisible(model->series->drawMode().testFlag(QSurface3DSeries::DrawWireframe));
        if (model->series->drawMode().testFlag(QSurface3DSeries::DrawSurface))
            model->model->setLocalOpacity(1.f);
        else
            model->model->setLocalOpacity(.0f);

        if (sliceView() && sliceView()->isVisible()) {
            model->sliceGridModel->setVisible(model->series->drawMode().testFlag(QSurface3DSeries::DrawWireframe));
            if (model->series->drawMode().testFlag(QSurface3DSeries::DrawSurface))
                model->sliceModel->setLocalOpacity(1.f);
            else
                model->sliceModel->setLocalOpacity(.0f);
        }
        updateMaterial(model);
    }

    m_surfaceController->setSeriesVisibilityDirty(false);
    if (m_surfaceController->isDataDirty() ||
            m_surfaceController->isSeriesVisualsDirty()) {

        m_surfaceController->clearSelection();

        if (m_surfaceController->hasChangedSeriesList()) {
            handleChangedSeries();
        } else {
            for (auto model : m_model) {
                bool visible = model->series->isVisible();
                if (visible)
                    updateModel(model);
            }
        }

        if (sliceView() && sliceView()->isVisible())
            updateSliceGraph();

        m_surfaceController->setDataDirty(false);
        m_surfaceController->setSeriesVisualsDirty(false);
    }

    if (m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionItem))
        updateSelectedPoint();
}

void QQuickGraphsSurface::handleChangedSeries()
{
    auto changedSeries = m_surfaceController->changedSeriesList();
    for (auto series : changedSeries) {
        for (auto model : m_model) {
            if (model->series == series) {
                updateModel(model);
            }
        }
    }
}

inline static float getDataValue(const QSurfaceDataArray &array, bool searchRow, int index)
{
    if (searchRow)
        return array.at(0)->at(index).x();
    else
        return array.at(index)->at(0).z();
}

inline static int binarySearchArray(const QSurfaceDataArray &array, int maxIndex, float limitValue, bool searchRow, bool lowBound, bool ascending)
{
    int min = 0;
    int max = maxIndex;
    int mid = 0;
    int retVal;

    while (max >= min) {
        mid = (min + max) / 2;
        float arrayValue = getDataValue(array, searchRow, mid);
        if (arrayValue == limitValue)
            return mid;
        if (ascending) {
            if (arrayValue < limitValue)
                min = mid + 1;
            else
                max = mid -1;
        } else {
            if (arrayValue > limitValue)
                min = mid + 1;
            else
                max = mid - 1;
        }
    }

    if (lowBound == ascending) {
        if (mid > max)
            retVal = mid;
        else
            retVal = min;
    } else {
        if (mid > max)
            retVal = max;
        else
            retVal = mid;
    }

    if (retVal < 0 || retVal > maxIndex) {
        retVal = -1;
    } else if (lowBound) {
        if (getDataValue(array, searchRow, retVal) < limitValue)
            retVal = -1;
    } else {
        if (getDataValue(array, searchRow, retVal) > limitValue)
            retVal = -1;
    }
    return retVal;
}

QRect QQuickGraphsSurface::calculateSampleSpace(const QSurfaceDataArray &array)
{
    QRect sampleSpace;
    if (array.size() > 0) {
        if (array.size() >= 2 && array.at(0)->size() >= 2) {
            const int maxRow = array.size() - 1;
            const int maxColumn = array.at(0)->size() - 1;

            const bool ascendingX = array.at(0)->at(0).x() < array.at(0)->at(maxColumn).x();
            const bool ascendingZ = array.at(0)->at(0).z() < array.at(maxRow)->at(0).z();

            int idx = binarySearchArray(array, maxColumn, m_surfaceController->axisX()->min(), true, true, ascendingX);
            if (idx != -1) {
                if (ascendingX)
                    sampleSpace.setLeft(idx);
                else
                    sampleSpace.setRight(idx);
            } else {
                sampleSpace.setWidth(-1);
            }

            idx = binarySearchArray(array, maxColumn, m_surfaceController->axisX()->max(), true, false, ascendingX);
            if (idx != -1) {
                if (ascendingX)
                    sampleSpace.setRight(idx);
                else
                    sampleSpace.setLeft(idx);
            } else {
                sampleSpace.setWidth(-1); // to indicate nothing needs to be shown
            }

            idx = binarySearchArray(array, maxRow, m_surfaceController->axisZ()->min(), false, true, ascendingZ);
            if (idx != -1) {
                if (ascendingZ)
                    sampleSpace.setTop(idx);
                else
                    sampleSpace.setBottom(idx);
            } else {
                sampleSpace.setWidth(-1); // to indicate nothing needs to be shown
            }

            idx = binarySearchArray(array, maxRow, m_surfaceController->axisZ()->max(), false, false, ascendingZ);
            if (idx != -1) {
                if (ascendingZ)
                    sampleSpace.setBottom(idx);
                else
                    sampleSpace.setTop(idx);
            } else {
                sampleSpace.setWidth(-1); // to indicate nothing needs to be shown
            }
        }
    }
    return sampleSpace;
}

void QQuickGraphsSurface::updateModel(SurfaceModel *model)
{
    const QSurfaceDataArray &array = *(model->series->dataProxy())->array();
    if (!array.isEmpty()) {
        int rowCount = array.size();
        int columnCount = array.at(0)->size();

        const int maxSize = 4096; //maximum texture size
        columnCount = qMin(maxSize, columnCount);
        rowCount = qMin(maxSize, rowCount);

        if (model->rowCount != rowCount) {
            model->rowCount = rowCount;
            m_isIndexDirty = true;
        }
        if (model->columnCount != columnCount) {
            model->columnCount = columnCount;
            m_isIndexDirty = true;
        }

        bool isPolar = m_surfaceController->isPolar();
        bool polarChanged = false;
        if (model->polar != isPolar) {
            polarChanged = true;
            model->polar = isPolar;
        }

        bool dimensionsChanged = false;
        QRect sampleSpace = calculateSampleSpace(array);
        if (sampleSpace != model->sampleSpace) {
            dimensionsChanged = true;
            model->sampleSpace = sampleSpace;
        }
        int rowStart = sampleSpace.top();
        int columnStart = sampleSpace.left();
        int rowLimit = sampleSpace.bottom() + 1;
        int columnLimit = sampleSpace.right() + 1;

        QAbstract3DAxis *axisX = m_surfaceController->axisX();
        QAbstract3DAxis *axisZ = m_surfaceController->axisZ();

        QPoint selC = model->selectedVertex.coord;
        selC.setX(qMin(selC.x(), rowCount - 1));
        selC.setY(qMin(selC.y(), columnCount - 1));
        QVector3D selP = array.at(selC.x())->at(selC.y()).position();

        bool pickOutOfRange = false;
        if (selP.x() < axisX->min() || selP.x() > axisX->max() || selP.z() < axisZ->min()
            || selP.z() > axisZ->max()) {
            pickOutOfRange = true;
        }

        if (m_isIndexDirty || pickOutOfRange) {
            model->selectedVertex = SurfaceVertex();
            if (sliceView() && sliceView()->isVisible()) {
                m_surfaceController->setSlicingActive(false);
                setSliceActivatedChanged(true);
            }
        }
        int totalSize = rowCount * columnCount * 2;
        float uvX = 1.0f / float(columnCount - 1);
        float uvY = 1.0f / float(rowCount - 1);

        bool isFlatShadingEnabled = model->series->isFlatShadingEnabled();

        QVector3D boundsMin = model->boundsMin;
        QVector3D boundsMax = model->boundsMax;

        QVector<QVector4D> heights;
        heights.reserve(totalSize);

        QQmlListReference materialRef(model->model, "materials");
        auto material = materialRef.at(0);
        QVariant heightInputAsVariant = material->property("height");
        QQuick3DShaderUtilsTextureInput *heightInput
            = heightInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();
        QQuick3DTexture *heightMap = heightInput->texture();
        QQuick3DTextureData *heightMapData = nullptr;
        if (!heightMap) {
            heightMap = new QQuick3DTexture();
            heightMap->setParent(this);
            heightMap->setHorizontalTiling(QQuick3DTexture::ClampToEdge);
            heightMap->setVerticalTiling(QQuick3DTexture::ClampToEdge);
            heightMapData = new QQuick3DTextureData();
            heightMapData->setSize(QSize(sampleSpace.width(), sampleSpace.height()));
            heightMapData->setFormat(QQuick3DTextureData::RGBA32F);
            heightMapData->setParent(heightMap);
            heightMapData->setParentItem(heightMap);
        } else {
            heightMapData = heightMap->textureData();
            if (dimensionsChanged)
                heightMapData->setSize(QSize(sampleSpace.width(), sampleSpace.height()));
        }
        if (heightMapData->size().width() < 2 || heightMapData->size().height() < 2) {
            heightMapData->setTextureData(QByteArray());
            heightMap->setTextureData(heightMapData);
            heightInput->setTexture(heightMap);
            model->heightTexture = heightMap;
            return;
        }
        material->setProperty("xDiff", 1.0f / float(sampleSpace.width() - 1));
        material->setProperty("yDiff", 1.0f / float(sampleSpace.height() - 1));
        material->setProperty("flatShading", isFlatShadingEnabled);
        material->setProperty("rangeMin", QVector2D(columnStart, rowStart));
        material->setProperty("range", QVector2D(sampleSpace.width(), sampleSpace.height()));
        material->setProperty("vertices", QVector2D(columnCount, rowCount));

        model->vertices.clear();
        model->vertices.reserve(totalSize);

        for (int i = rowStart; i < rowLimit; i++) {
            const QSurfaceDataRow &row = *array.at(i);
            for (int j = columnStart; j < columnLimit; j++) {
                QVector3D pos = getNormalizedVertex(row.at(j), isPolar, false);
                heights.push_back(QVector4D(pos, .0f));
                SurfaceVertex vertex;
                vertex.position = pos;
                vertex.uv = QVector2D(j * uvX, i * uvY);
                vertex.coord = QPoint(i, j);
                model->vertices.push_back(vertex);
                if (boundsMin.isNull())
                    boundsMin = pos;
                else
                    boundsMin = QVector3D(qMin(boundsMin.x(), pos.x()),
                                          qMin(boundsMin.y(), pos.y()),
                                          qMin(boundsMin.z(), pos.z()));
                if (boundsMax.isNull())
                    boundsMax = pos;
                else
                    boundsMax = QVector3D(qMax(boundsMax.x(), pos.x()),
                                          qMax(boundsMax.y(), pos.y()),
                                          qMax(boundsMax.z(), pos.z()));
            }
        }
        model->boundsMin = boundsMin;
        model->boundsMax = boundsMax;

        QByteArray heightData = QByteArray(reinterpret_cast<char *>(heights.data()),
                                           heights.size() * sizeof(QVector4D));
        heightMapData->setTextureData(heightData);
        heightMap->setTextureData(heightMapData);
        heightInput->setTexture(heightMap);
        model->heightTexture = heightMap;

        if (m_isIndexDirty) {
            QVector<SurfaceVertex> vertices;
            for (int i = 0; i < rowCount; i++) {
                const QSurfaceDataRow &row = *array.at(i);
                for (int j = 0; j < columnCount; j++) {
                    SurfaceVertex vertex;
                    QVector3D pos = getNormalizedVertex(row.at(j), isPolar, false);
                    vertex.position = pos;
                    vertex.uv = QVector2D(j * uvX, i * uvY);
                    vertex.coord = QPoint(i, j);
                    vertices.push_back(vertex);
                }
            }
            createIndices(model, columnCount, rowCount);
            auto geometry = model->model->geometry();
            geometry->vertexData().clear();
            QByteArray vertexBuffer(reinterpret_cast<char *>(vertices.data()),
                                    vertices.size() * sizeof(SurfaceVertex));
            geometry->setVertexData(vertexBuffer);
            QByteArray indexBuffer(reinterpret_cast<char *>(model->indices.data()),
                                   model->indices.size() * sizeof(quint32));
            geometry->setIndexData(indexBuffer);
            geometry->setBounds(boundsMin, boundsMax);
            geometry->update();

            createGridlineIndices(model, 0, 0, columnCount, rowCount);
            auto gridGeometry = model->gridModel->geometry();
            gridGeometry->vertexData().clear();
            gridGeometry->setVertexData(vertexBuffer);
            QByteArray gridIndexBuffer(reinterpret_cast<char *>(model->gridIndices.data()),
                                       model->gridIndices.size() * sizeof(quint32));
            gridGeometry->setIndexData(gridIndexBuffer);
            gridGeometry->setBounds(boundsMin, boundsMax);
            gridGeometry->update();
            m_isIndexDirty = false;
        }
        QQmlListReference gridMaterialRef(model->gridModel, "materials");
        auto gridMaterial = gridMaterialRef.at(0);
        QVariant gridHeightInputAsVariant = gridMaterial->property("height");
        QQuick3DShaderUtilsTextureInput *gridHeightInput
            = gridHeightInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();
        gridHeightInput->setTexture(heightMap);
        QColor gridColor = model->series->wireframeColor();
        gridMaterial->setProperty("gridColor", gridColor);
        gridMaterial->setProperty("range", QVector2D(sampleSpace.width(), sampleSpace.height()));
        gridMaterial->setProperty("vertices", QVector2D(columnCount, rowCount));

        if (dimensionsChanged || polarChanged)
            updateProxyModel(model);
    }
    updateMaterial(model);
    updateSelectedPoint();
}

void QQuickGraphsSurface::updateProxyModel(SurfaceModel *model)
{
    if (!model->proxyModel)
        createProxyModel(model);

    const QSurfaceDataArray &array = *(model->series->dataProxy())->array();
    if (array.isEmpty())
        return;

    QRect sampleSpace = model->sampleSpace;
    int rowCount = sampleSpace.height();
    int columnCount = sampleSpace.width();
    int rowStart = sampleSpace.top();
    int columnStart = sampleSpace.left();
    int rowLimit = sampleSpace.bottom() + 1;
    int columnLimit = sampleSpace.right() + 1;
    if (rowCount == 0 || columnCount == 0)
        return;

    //calculate decimate factor based on the order of magnitude of total vertices
    float totalSize = rowCount * columnCount;
    int decimateFactor = qFloor(std::log10(qMax(10.0, totalSize)));

    int proxyColumnCount = 0;
    int proxyRowCount = 0;
    QVector<SurfaceVertex> proxyVerts;

    float uvY = 1.0f / float(rowCount - 1);
    float uvX = 1.0f / float(columnCount - 1);

    QVector3D boundsMin = model->boundsMin;
    QVector3D boundsMax = model->boundsMax;

    bool isPolar = m_surfaceController->isPolar();
    int i = rowStart;
    while (i < rowLimit) {
        const QSurfaceDataRow &row = *array.at(i);
        proxyRowCount++;
        int j = columnStart;
        while (j < columnLimit) {
            // getNormalizedVertex
            if (i == rowStart)
                proxyColumnCount++;
            QVector3D pos = getNormalizedVertex(row.at(j), isPolar, false);
            SurfaceVertex vertex;
            vertex.position = pos;
            vertex.uv = QVector2D(j * uvX, i * uvY);
            vertex.coord = QPoint(i, j);
            proxyVerts.push_back(vertex);

            boundsMin = QVector3D(qMin(boundsMin.x(), pos.x()),
                                  qMin(boundsMin.y(), pos.y()),
                                  qMin(boundsMin.z(), pos.z()));
            boundsMax = QVector3D(qMax(boundsMax.x(), pos.x()),
                                  qMax(boundsMax.y(), pos.y()),
                                  qMax(boundsMax.z(), pos.z()));

            if (j == columnLimit - 1)
                break;

            j += decimateFactor;
            if (j >= columnLimit)
                j = columnLimit - 1;
        }
        if (i == rowLimit - 1)
            break;

        i += decimateFactor;
        if (i >= rowLimit)
            i = rowLimit - 1;
    }

    model->boundsMin = boundsMin;
    model->boundsMax = boundsMax;
    int endX = proxyColumnCount - 1;
    int endY = proxyRowCount - 1;
    int indexCount = 6 * endX * endY;

    QVector<quint32> *proxyIndices = new QVector<quint32>();
    proxyIndices->resize(indexCount);

    const int maxRow = array.size() - 1;
    const int maxColumn = array.at(0)->size() - 1;
    const bool ascendingX = array.at(0)->at(0).x() < array.at(0)->at(maxColumn).x();
    const bool ascendingZ = array.at(0)->at(0).z() < array.at(maxRow)->at(0).z();

    int rowEnd = endY * proxyColumnCount;
    for (int row = 0; row < rowEnd; row += proxyColumnCount) {
        for (int j = 0; j < endX; j++) {
            if (ascendingX && ascendingZ) {
                proxyIndices->push_back(row + j + 1);
                proxyIndices->push_back(row + proxyColumnCount + j);
                proxyIndices->push_back(row + j);

                proxyIndices->push_back(row + proxyColumnCount + j + 1);
                proxyIndices->push_back(row + proxyColumnCount + j);
                proxyIndices->push_back(row + j + 1);
            } else if (!ascendingX) {
                proxyIndices->push_back(row + proxyColumnCount + j);
                proxyIndices->push_back(row + proxyColumnCount + j + 1);
                proxyIndices->push_back(row + j);

                proxyIndices->push_back(row + j);
                proxyIndices->push_back(row + proxyColumnCount + j + 1);
                proxyIndices->push_back(row + j + 1);
            } else {
                proxyIndices->push_back(row + proxyColumnCount + j);
                proxyIndices->push_back(row + proxyColumnCount + j + 1);
                proxyIndices->push_back(row + j + 1);

                proxyIndices->push_back(row + j);
                proxyIndices->push_back(row + proxyColumnCount + j);
                proxyIndices->push_back(row + j + 1);
            }
        }
    }

    auto geometry = model->proxyModel->geometry();
    geometry->vertexData().clear();
    QByteArray vertexBuffer(reinterpret_cast<char *>(proxyVerts.data()),
                            proxyVerts.size() * sizeof(SurfaceVertex));
    geometry->setVertexData(vertexBuffer);
    QByteArray indexBuffer(reinterpret_cast<char *>(proxyIndices->data()),
                           proxyIndices->size() * sizeof(quint32));
    geometry->setIndexData(indexBuffer);
    geometry->setBounds(boundsMin, boundsMax);
    geometry->update();
}

void QQuickGraphsSurface::createProxyModel(SurfaceModel *model)
{
    auto proxyModel = new QQuick3DModel();
    proxyModel->setParent(graphNode());
    proxyModel->setParentItem(model->model);
    proxyModel->setObjectName(QStringLiteral("ProxyModel"));
    proxyModel->setVisible(true);
    if (m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionNone))
        proxyModel->setPickable(false);
    else
        proxyModel->setPickable(true);

    auto geometry = new QQuick3DGeometry();
    geometry->setParent(proxyModel);
    geometry->setStride(sizeof(SurfaceVertex));
    geometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    geometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                           0,
                           QQuick3DGeometry::Attribute::F32Type);
    geometry->addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,
                           sizeof(QVector3D),
                           QQuick3DGeometry::Attribute::F32Type);
    geometry->addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                           0,
                           QQuick3DGeometry::Attribute::U32Type);
    proxyModel->setGeometry(geometry);


    QQmlListReference materialRef(proxyModel, "materials");
    QQuick3DPrincipledMaterial *material = new QQuick3DPrincipledMaterial();
    material->setParent(proxyModel);
    material->setBaseColor(Qt::white);
    material->setOpacity(0);
    material->setCullMode(QQuick3DMaterial::NoCulling);
    materialRef.append(material);

    model->proxyModel = proxyModel;
}

void QQuickGraphsSurface::updateMaterial(SurfaceModel *model)
{
    QQmlListReference materialRef(model->model, "materials");

    QQuick3DCustomMaterial *material = qobject_cast<QQuick3DCustomMaterial *>(materialRef.at(0));

    if (!material) {
        material = createQmlCustomMaterial(QStringLiteral(":/materials/SurfaceMaterial"));
        model->customMaterial = material;
    }

    bool textured = !(model->series->texture().isNull() && model->series->textureFile().isEmpty());

    if (m_surfaceController->isSeriesVisualsDirty()
        || !textured) {

        float minY = model->boundsMin.y();
        float maxY = model->boundsMax.y();
        float range = maxY - minY;

        switch (model->series->colorStyle()) {
        case(Q3DTheme::ColorStyleObjectGradient):
            material->setProperty("colorStyle", 0);
            material->setProperty("gradientMin", -(minY / range));
            material->setProperty("gradientHeight", 1.0f / range);
            break;
        case(Q3DTheme::ColorStyleRangeGradient):
            material->setProperty("colorStyle", 1);
            break;
        case(Q3DTheme::ColorStyleUniform):
            material->setProperty("colorStyle", 2);
            material->setProperty("uniformColor",model->series->baseColor());
            break;
        }

        QVariant textureInputAsVariant = material->property("custex");
        QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();
        auto textureData = static_cast<QuickGraphsTextureData *>(model->texture->textureData());
        textureData->createGradient(model->series->baseGradient());
        textureInput->setTexture(model->texture);

        QVariant heightInputAsVariant = material->property("height");
        QQuick3DShaderUtilsTextureInput *heightInput = heightInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();
        heightInput->setTexture(model->heightTexture);
        material->setParent(model->model);
        material->setParentItem(model->model);
        material->setCullMode(QQuick3DMaterial::NoCulling);
        material->setProperty("flatShading", model->series->isFlatShadingEnabled());
    }

    if (textured) {
        material->setProperty("colorStyle", 3);
        QQuick3DShaderUtilsTextureInput *texInput = material->property("baseColor").value<QQuick3DShaderUtilsTextureInput *>();
        if (!texInput->texture()) {
            QQuick3DTexture *texture = new QQuick3DTexture();
            texture->setParent(material);
            texture->setParentItem(material);
            texInput->setTexture(texture);
        }
        if (!model->series->textureFile().isEmpty()) {
            texInput->texture()->setSource(QUrl::fromLocalFile(model->series->textureFile()));
        } else if (!model->series->texture().isNull()) {
            QImage image = model->series->texture();
            image.convertTo(QImage::Format_RGBA32FPx4);
            auto textureData = static_cast<QuickGraphsTextureData *>(model->texture->textureData());
            textureData->setFormat(QQuick3DTextureData::RGBA32F);
            textureData->setSize(image.size());
            textureData->setTextureData(QByteArray(reinterpret_cast<const char*>(image.bits()),
                                                   image.sizeInBytes()));
            texInput->texture()->setTextureData(textureData);
            texInput->texture()->setVerticalTiling(QQuick3DTexture::ClampToEdge);
            texInput->texture()->setHorizontalTiling(QQuick3DTexture::ClampToEdge);
        } else {
            texInput->texture()->setSource(QUrl());
        }
    }
    material->update();
}

QVector3D QQuickGraphsSurface::getNormalizedVertex(const QSurfaceDataItem &data, bool polar, bool flipXZ)
{
    Q_UNUSED(flipXZ);

    QValue3DAxis* axisX = static_cast<QValue3DAxis *>(m_surfaceController->axisX());
    QValue3DAxis* axisY = static_cast<QValue3DAxis *>(m_surfaceController->axisY());
    QValue3DAxis* axisZ = static_cast<QValue3DAxis *>(m_surfaceController->axisZ());

    float normalizedX = axisX->positionAt(data.x());
    float normalizedY;
    float normalizedZ = axisZ->positionAt(data.z());
    // TODO : Need to handle, flipXZ

    float scale, translate;
    if (polar) {
        float angle = normalizedX * M_PI * 2.0f;
        float radius = normalizedZ;
        normalizedX = radius * qSin(angle) * 1.0f;
        normalizedZ = -(radius * qCos(angle)) * 1.0f;
    } else {
        scale = translate = this->scaleWithBackground().x();
        normalizedX = normalizedX * scale * 2.0f - translate;
        scale = translate = this->scaleWithBackground().z();
        normalizedZ = normalizedZ * -scale * 2.0f + translate;
    }
    scale = translate = this->scale().y();
    normalizedY = axisY->positionAt(data.y()) * scale * 2.0f - translate;
    return QVector3D(normalizedX, normalizedY, normalizedZ);
}

void QQuickGraphsSurface::updateSliceGraph()
{
    if (!sliceView())
        createSliceView();

    QQuickGraphsItem::updateSliceGraph();

    m_surfaceController->setSelectedPointChanged(true);

    if (!sliceView()->isVisible())
        return;

    auto selectionMode = m_surfaceController->selectionMode();
    for (auto model : m_model) {
        bool visible = model->series->isVisible();

        model->sliceModel->setVisible(visible);
        model->sliceGridModel->setVisible(visible);

        if (!selectionMode.testFlag(QAbstract3DGraph::SelectionMultiSeries)
                && !model->picked) {
            model->sliceModel->setVisible(false);
            model->sliceGridModel->setVisible(false);
            continue;
        } else {
            model->sliceGridModel->setVisible(model->series->drawMode().testFlag(QSurface3DSeries::DrawWireframe));
            if (model->series->drawMode().testFlag(QSurface3DSeries::DrawSurface))
                model->sliceModel->setLocalOpacity(1.f);
            else
                model->sliceModel->setLocalOpacity(.0f);
        }

        QVector<SurfaceVertex> selectedSeries;

        QRect sampleSpace = model->sampleSpace;
        int rowStart = sampleSpace.top();
        int columnStart = sampleSpace.left();
        int rowCount = sampleSpace.height();
        int columnCount = sampleSpace.width();

        int indexCount = 0;
        if (selectionMode.testFlag(QAbstract3DGraph::SelectionRow)) {
            int selectedRow = (model->selectedVertex.coord.x() - rowStart) * columnCount;
            selectedSeries.reserve(columnCount * 2);
            QVector<SurfaceVertex> list;
            for (int i = 0; i < columnCount; i++) {
                SurfaceVertex vertex = model->vertices.at(selectedRow + i);
                vertex.position.setY(vertex.position.y() - .025f);
                vertex.position.setZ(.0f);
                selectedSeries.append(vertex);
                vertex.position.setY(vertex.position.y() + .05f);
                list.append(vertex);
            }
            selectedSeries.append(list);
            indexCount = columnCount - 1;
        }

        if (selectionMode.testFlag(QAbstract3DGraph::SelectionColumn)) {
            int selectedColumn = model->selectedVertex.coord.y() - columnStart;
            selectedSeries.reserve(rowCount * 2);
            QVector<SurfaceVertex> list;
            for (int i = 0; i < rowCount; i++) {
                SurfaceVertex vertex = model->vertices.at((i * columnCount) + selectedColumn);
                vertex.position.setX(-vertex.position.z());
                vertex.position.setY(vertex.position.y() - .025f);
                vertex.position.setZ(0);
                selectedSeries.append(vertex);
                vertex.position.setY(vertex.position.y() + .05f);
                list.append(vertex);
            }
            selectedSeries.append(list);
            indexCount = rowCount - 1;

            QQmlListReference materialRef(model->sliceModel, "materials");
            auto material = materialRef.at(0);
            material->setProperty("isColumn", true);
        }

        QVector<quint32> indices;
        indices.reserve(indexCount * 6);
        for (int i = 0; i < indexCount; i++) {
            indices.push_back(i + 1);
            indices.push_back(i + indexCount + 1);
            indices.push_back(i);
            indices.push_back(i + indexCount + 2);
            indices.push_back(i + indexCount + 1);
            indices.push_back(i + 1);
        }

        auto geometry = model->sliceModel->geometry();
        geometry->vertexData().clear();
        geometry->indexData().clear();
        QByteArray vertexBuffer(reinterpret_cast<char *>(selectedSeries.data()),
                                selectedSeries.size() * sizeof(SurfaceVertex));
        geometry->setVertexData(vertexBuffer);
        QByteArray indexBuffer(reinterpret_cast<char *>(indices.data()),
                               indices.size() * sizeof(quint32));
        geometry->setIndexData(indexBuffer);
        geometry->update();

        geometry = model->sliceGridModel->geometry();
        geometry->vertexData().clear();
        geometry->indexData().clear();
        geometry->setVertexData(vertexBuffer);

        QVector<quint32> gridIndices;
        gridIndices.reserve(indexCount * 4);
        for (int i = 0; i < indexCount; i++) {
            gridIndices.push_back(i);
            gridIndices.push_back(i + indexCount + 1);

            gridIndices.push_back(i);
            gridIndices.push_back(i + 1);
        }
        geometry->indexData().clear();
        QByteArray gridIndexBuffer(reinterpret_cast<char *>(gridIndices.data()),
                                   gridIndices.size() * sizeof(quint32));
        geometry->setIndexData(gridIndexBuffer);
        geometry->update();

        QQmlListReference gridMaterialRef(model->sliceGridModel, "materials");
        auto gridMaterial = static_cast<QQuick3DPrincipledMaterial *>(gridMaterialRef.at(0));
        QColor gridColor = model->series->wireframeColor();
        gridMaterial->setBaseColor(gridColor);

        updateSelectedPoint();
    }
}


void QQuickGraphsSurface::createIndices(SurfaceModel *model, int columnCount, int rowCount)
{
    Surface3DController::DataDimensions dataDimensions = m_surfaceController->dataDimensions();

    int endX = columnCount - 1;
    int endY = rowCount -1;

    int indexCount = 6 * endX * endY;
    QVector<quint32> *indices = &model->indices;

    indices->clear();
    indices->resize(indexCount);

    int rowEnd = endY * columnCount;
    for (int row = 0; row < rowEnd ; row += columnCount) {
        for (int j = 0 ; j < endX ; j++) {
            if (dataDimensions == Surface3DController::BothAscending
                    || dataDimensions == Surface3DController::BothDescending) {
                indices->push_back(row + j + 1);
                indices->push_back(row + columnCount + j);
                indices->push_back(row + j);

                indices->push_back(row + columnCount + j + 1);
                indices->push_back(row + columnCount + j);
                indices->push_back(row + j + 1);
            } else if (dataDimensions == Surface3DController::XDescending) {
                indices->push_back(row + columnCount + j);
                indices->push_back(row + columnCount + j + 1);
                indices->push_back(row + j);

                indices->push_back(row + j);
                indices->push_back(row + columnCount + j + 1);
                indices->push_back(row + j + 1);
            } else {
                indices->push_back(row + columnCount + j);
                indices->push_back(row + columnCount + j + 1);
                indices->push_back(row + j + 1);

                indices->push_back(row + j);
                indices->push_back(row + columnCount + j);
                indices->push_back(row + j + 1);
            }
        }
    }
}
void QQuickGraphsSurface::createGridlineIndices(SurfaceModel *model, int x, int y, int endX, int endY)
{
    int columnCount = model->columnCount;
    int rowCount = model->rowCount;

    if (endX >= columnCount)
        endX = columnCount - 1;
    if (endY >= rowCount)
        endY = rowCount - 1;
    if (x > endX)
        x = endX - 1;
    if (y > endY)
        y = endY - 1;

    int nColumns = endX - x + 1;
    int nRows = endY - y + 1;

    int gridIndexCount = 2 * nColumns * (nRows - 1) + 2 * nRows * (nColumns - 1);
    model->gridIndices.clear();
    model->gridIndices.resize(gridIndexCount);

    for (int i = y, row = columnCount * y ; i <= endY ; i++, row += columnCount) {
        for (int j = x ; j < endX ; j++) {
            model->gridIndices.push_back(row + j);
            model->gridIndices.push_back(row + j + 1);
        }
    }
    for (int i = y, row = columnCount * y ; i < endY ; i++, row += columnCount) {
        for (int j = x ; j <= endX ; j++) {
            model->gridIndices.push_back(row + j);
            model->gridIndices.push_back(row + j + columnCount);
        }
    }
}
bool QQuickGraphsSurface::handleMousePressedEvent(QMouseEvent *event)
{
    if (!QQuickGraphsItem::handleMousePressedEvent(event))
        return true;

    if (Qt::LeftButton == event->button())
        doPicking(event->pos());

    return true;
}

bool QQuickGraphsSurface::handleTouchEvent(QTouchEvent *event)
{
    if (!QQuickGraphsItem::handleTouchEvent(event))
        return true;

    if (scene()->selectionQueryPosition() != scene()->invalidSelectionPoint()
        && !event->isUpdateEvent()) {
        doPicking(event->point(0).position());
        scene()->setSelectionQueryPosition(scene()->invalidSelectionPoint());
    }

    return true;
}

bool QQuickGraphsSurface::doPicking(const QPointF &position)
{
    if (!QQuickGraphsItem::doPicking(position))
        return false;

    auto pickResult = pickAll(position.x(), position.y());
    QVector3D pickedPos(0.0f, 0.0f, 0.0f);
    QQuick3DModel *pickedModel = nullptr;

    auto selectionMode = m_surfaceController->selectionMode();
    if (!selectionMode.testFlag(QAbstract3DGraph::SelectionNone)) {
        if (!sliceView() && selectionMode.testFlag(QAbstract3DGraph::SelectionSlice))
            createSliceView();

        for (auto picked : pickResult) {
            if (picked.objectHit()
                    && picked.objectHit()->objectName().contains(QStringLiteral("ProxyModel"))) {
                pickedPos = picked.position();
                pickedModel = qobject_cast<QQuick3DModel *>(picked.objectHit()->parentItem());
                bool visible = false;
                for (auto model : m_model) {
                    if (model->model == pickedModel)
                        visible = model->series->isVisible();
                }
                if (!pickedPos.isNull() && visible)
                    break;
            }
        }

        bool inRange = qAbs(pickedPos.x()) < scaleWithBackground().x()
                       && qAbs(pickedPos.z()) < scaleWithBackground().z();

        if (!pickedPos.isNull() && inRange) {
            float min = -1.0f;

            for (auto model : m_model) {
                if (!model->series->isVisible())
                    continue;

                model->picked = (model->model == pickedModel);

                SurfaceVertex selectedVertex;
                for (auto vertex : model->vertices) {
                    QVector3D pos = vertex.position;
                    float dist = pickedPos.distanceToPoint(pos);
                    if (selectedVertex.position.isNull() || dist < min) {
                        min = dist;
                        selectedVertex = vertex;
                    }
                }
                model->selectedVertex = selectedVertex;
                if (!selectedVertex.position.isNull()
                    && model->picked) {
                    model->series->setSelectedPoint(selectedVertex.coord);
                    m_surfaceController->setSlicingActive(false);
                    if (isSliceEnabled())
                        setSliceActivatedChanged(true);
                }
            }
        }
    }
    return true;
}

void QQuickGraphsSurface::updateSelectedPoint()
{
    bool labelVisible = false;
    m_instancing->resetPositions();
    if (sliceView() && sliceView()->isVisible())
        m_sliceInstancing->resetPositions();
    for (auto model : m_model) {
        if ((!m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionMultiSeries) &&
                !model->picked)|| model->selectedVertex.position.isNull())
            continue;
        QPoint selectedCoord = model->selectedVertex.coord;
        int coordX = selectedCoord.x() - model->sampleSpace.top();
        int coordY = selectedCoord.y() - model->sampleSpace.left();
        int index = coordX * model->sampleSpace.width() + coordY;
        SurfaceVertex selectedVertex = model->vertices.at(index);
        if (model->series->isVisible() &&
                !selectedVertex.position.isNull() &&
                m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionItem)) {
            m_instancing->addPosition(selectedVertex.position);
            QVector3D slicePosition = selectedVertex.position;
            if (sliceView() && sliceView()->isVisible()) {
                if (m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionColumn))
                    slicePosition.setX(-slicePosition.z());
                slicePosition.setZ(.0f);
                m_sliceInstancing->addPosition(slicePosition);
            }
            if (model->picked) {
                QVector3D labelPosition = selectedVertex.position;
                QString label = model->series->itemLabel();
                m_surfaceController->setSelectedPoint(selectedVertex.coord, model->series, false);

                updateItemLabel(labelPosition);
                itemLabel()->setProperty("labelText", label);
                labelVisible = true;

                if (sliceView() && sliceView()->isVisible()) {
                    QFontMetrics fm(m_surfaceController->activeTheme()->font());
                    float textPadding = 12.0f;
                    float labelHeight = fm.height() + textPadding;
                    float labelWidth = fm.horizontalAdvance(label) + textPadding;
                    QVector3D scale = sliceItemLabel()->scale();
                    scale.setX(scale.y() * labelWidth / labelHeight);
                    sliceItemLabel()->setProperty("labelWidth", labelWidth);
                    sliceItemLabel()->setProperty("labelHeight", labelHeight);
                    sliceItemLabel()->setScale(scale);
                    labelPosition = slicePosition;
                    labelPosition.setZ(.1f);
                    labelPosition.setY(labelPosition.y() + .05f);
                    sliceItemLabel()->setPosition(labelPosition);
                    sliceItemLabel()->setProperty("labelText", label);
                }
            }
        }
    }
    itemLabel()->setVisible(labelVisible);
    if (sliceView() && sliceView()->isVisible())
        sliceItemLabel()->setVisible(labelVisible);
}

void QQuickGraphsSurface::addModel(QSurface3DSeries *series)
{
    auto parent = graphNode();
    bool visible = series->isVisible();

    auto model = new QQuick3DModel();
    model->setParent(parent);
    model->setParentItem(parent);
    model->setObjectName(QStringLiteral("SurfaceModel"));
    model->setVisible(visible);
    if (m_surfaceController->selectionMode().testFlag(QAbstract3DGraph::SelectionNone))
        model->setPickable(false);
    else
        model->setPickable(true);

    auto geometry = new QQuick3DGeometry();
    geometry->setParent(model);
    geometry->setStride(sizeof(SurfaceVertex));
    geometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    geometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                           0,
                           QQuick3DGeometry::Attribute::F32Type);
    geometry->addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,
                           sizeof(QVector3D),
                           QQuick3DGeometry::Attribute::F32Type);
    geometry->addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                           0,
                           QQuick3DGeometry::Attribute::U32Type);
    model->setGeometry(geometry);


    QQuick3DTexture *texture = new QQuick3DTexture();
    texture->setHorizontalTiling(QQuick3DTexture::ClampToEdge);
    texture->setVerticalTiling(QQuick3DTexture::ClampToEdge);
    QuickGraphsTextureData *textureData = new QuickGraphsTextureData();
    textureData->setParent(texture);
    textureData->setParentItem(texture);
    texture->setTextureData(textureData);

    QQmlListReference materialRef(model, "materials");

    QQuick3DCustomMaterial *customMaterial = createQmlCustomMaterial(QStringLiteral(":/materials/SurfaceMaterial"));

    customMaterial->setParent(model);
    customMaterial->setParentItem(model);
    customMaterial->setCullMode(QQuick3DMaterial::NoCulling);
    QVariant textureInputAsVariant = customMaterial->property("custex");
    QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();
    textureInput->setTexture(texture);

    texture->setParent(customMaterial);

    materialRef.append(customMaterial);

    auto gridModel = new QQuick3DModel();
    gridModel->setParent(parent);
    gridModel->setParentItem(parent);
    gridModel->setObjectName(QStringLiteral("SurfaceModel"));
    gridModel->setVisible(visible);
    gridModel->setDepthBias(1.0f);
    auto gridGeometry = new QQuick3DGeometry();
    gridGeometry->setParent(this);
    gridGeometry->setStride(sizeof(SurfaceVertex));
    gridGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);
    gridGeometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                               0,
                               QQuick3DGeometry::Attribute::F32Type);
    gridGeometry->addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,
                               sizeof(QVector3D),
                               QQuick3DGeometry::Attribute::F32Type);
    gridGeometry->addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                               0,
                               QQuick3DGeometry::Attribute::U32Type);
    gridModel->setGeometry(gridGeometry);
    QQmlListReference gridMaterialRef(gridModel, "materials");
    auto gridMaterial = createQmlCustomMaterial(QStringLiteral(":/materials/GridSurfaceMaterial"));
    gridMaterial->setParent(gridModel);
    gridMaterial->setParentItem(gridModel);
    gridMaterialRef.append(gridMaterial);

    SurfaceModel *surfaceModel = new SurfaceModel();
    surfaceModel->model = model;
    surfaceModel->gridModel = gridModel;
    surfaceModel->series = series;
    surfaceModel->texture = texture;
    surfaceModel->customMaterial = customMaterial;

    m_model.push_back(surfaceModel);

    connect(series,
            &QSurface3DSeries::flatShadingEnabledChanged,
            this,
            &QQuickGraphsSurface::handleFlatShadingEnabledChanged);
    connect(series,
            &QSurface3DSeries::wireframeColorChanged,
            this,
            &QQuickGraphsSurface::handleWireframeColorChanged);

    if (sliceView())
        addSliceModel(surfaceModel);
}

void QQuickGraphsSurface::createSliceView()
{
    QQuickGraphsItem::createSliceView();

    for (auto surfaceModel : m_model)
        addSliceModel(surfaceModel);

    QQuick3DViewport *sliceParent = sliceView();

    m_sliceSelectionPointer = new QQuick3DModel();
    m_sliceSelectionPointer->setParent(sliceParent->scene());
    m_sliceSelectionPointer->setParentItem(sliceParent->scene());
    m_sliceSelectionPointer->setSource(QUrl(QStringLiteral("#Sphere")));
    QQuick3DPrincipledMaterial *pointerMaterial = new QQuick3DPrincipledMaterial();
    pointerMaterial->setParent(m_sliceSelectionPointer);
    pointerMaterial->setBaseColor(m_surfaceController->activeTheme()->singleHighlightColor());
    QQmlListReference sliceMaterialRef(m_sliceSelectionPointer, "materials");
    sliceMaterialRef.append(pointerMaterial);
    m_sliceInstancing = new SurfaceSelectionInstancing();
    m_sliceInstancing->setScale(QVector3D(0.001f, 0.001f, 0.001f));
    m_sliceSelectionPointer->setInstancing(m_sliceInstancing);
    m_sliceInstancing->setColor(m_surfaceController->activeTheme()->singleHighlightColor());
}

void QQuickGraphsSurface::addSliceModel(SurfaceModel *model)
{
    QQuick3DViewport *sliceParent = sliceView();

    auto surfaceModel = new QQuick3DModel();
    surfaceModel->setParent(sliceParent->scene());
    surfaceModel->setParentItem(sliceParent->scene());
    surfaceModel->setVisible(model->series->isVisible());

    auto geometry = new QQuick3DGeometry();
    geometry->setParent(surfaceModel);
    geometry->setParentItem(surfaceModel);
    geometry->setStride(sizeof(SurfaceVertex));
    geometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    geometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                           0,
                           QQuick3DGeometry::Attribute::F32Type);
    geometry->addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,
                           sizeof(QVector3D),
                           QQuick3DGeometry::Attribute::F32Type);
    geometry->addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                           0,
                           QQuick3DGeometry::Attribute::U32Type);
    surfaceModel->setGeometry(geometry);

    QQmlListReference materialRef(surfaceModel, "materials");
    auto material = createQmlCustomMaterial(QStringLiteral(":/materials/SurfaceSliceMaterial"));
    material->setCullMode(QQuick3DMaterial::NoCulling);
    QVariant textureInputAsVariant = material->property("custex");
    QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();
    QQuick3DTexture *texture = model->texture;
    textureInput->setTexture(texture);
    materialRef.append(material);

    model->sliceModel = surfaceModel;

    QQuick3DModel *gridModel = new QQuick3DModel();
    gridModel->setParent(sliceParent->scene());
    gridModel->setParentItem(sliceParent->scene());
    gridModel->setVisible(model->series->isVisible());
    gridModel->setDepthBias(1.0f);
    QQuick3DGeometry *gridGeometry = new QQuick3DGeometry();
    gridGeometry->setParent(gridModel);
    gridGeometry->setStride(sizeof(SurfaceVertex));
    gridGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);
    gridGeometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                               0,
                               QQuick3DGeometry::Attribute::F32Type);
    gridGeometry->addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                               0,
                               QQuick3DGeometry::Attribute::U32Type);
    gridModel->setGeometry(gridGeometry);
    QQmlListReference gridMaterialRef(gridModel, "materials");
    QQuick3DPrincipledMaterial *gridMaterial = new QQuick3DPrincipledMaterial();
    gridMaterial->setParent(gridModel);
    gridMaterial->setLighting(QQuick3DPrincipledMaterial::NoLighting);
    gridMaterial->setParent(gridModel);
    gridMaterialRef.append(gridMaterial);

    model->sliceGridModel = gridModel;
}

void QQuickGraphsSurface::updateSingleHighlightColor()
{
    m_instancing->setColor(m_surfaceController->activeTheme()->singleHighlightColor());
    if (sliceView())
        m_sliceInstancing->setColor(m_surfaceController->activeTheme()->singleHighlightColor());
}

void QQuickGraphsSurface::updateLightStrength()
{
    for (auto model : m_model) {
        QQmlListReference materialRef(model->model, "materials");
        QQuick3DCustomMaterial *material = qobject_cast<QQuick3DCustomMaterial *>(materialRef.at(0));
        material->setProperty("specularBrightness",
                              m_surfaceController->activeTheme()->lightStrength() * 0.05);
    }
}

void QQuickGraphsSurface::handleThemeTypeChange()
{
    for (auto model : m_model)
        updateMaterial(model);
}

QT_END_NAMESPACE
