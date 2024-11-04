// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickgraphsscatter_p.h"
#include "qquickgraphstexturedata_p.h"
#include "qscatter3dseries_p.h"
#include "qscatterdataproxy_p.h"
#include "qvalue3daxis_p.h"

#include <QColor>
#include <QtCore/QMutexLocker>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3drepeater_p.h>

QT_BEGIN_NAMESPACE

static const int insertRemoveRecordReserveSize = 31;

QQuickGraphsScatter::QQuickGraphsScatter(QQuickItem *parent)
    : QQuickGraphsItem(parent)
{
    m_graphType = QAbstract3DSeries::SeriesType::Scatter;
    setAxisX(0);
    setAxisY(0);
    setAxisZ(0);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents);
    createInitialInputHandler();
    clearSelection();
}

QQuickGraphsScatter::~QQuickGraphsScatter()
{
    QMutexLocker locker(m_nodeMutex.data());
    const QMutexLocker locker2(mutex());

    for (auto &graphModel : m_scatterGraphs) {
        delete graphModel;
    }
}

void QQuickGraphsScatter::setAxisX(QValue3DAxis *axis)
{
    QQuickGraphsItem::setAxisX(axis);
}

QValue3DAxis *QQuickGraphsScatter::axisX() const
{
    return static_cast<QValue3DAxis *>(QQuickGraphsItem::axisX());
}

void QQuickGraphsScatter::setAxisY(QValue3DAxis *axis)
{
    QQuickGraphsItem::setAxisY(axis);
}

QValue3DAxis *QQuickGraphsScatter::axisY() const
{
    return static_cast<QValue3DAxis *>(QQuickGraphsItem::axisY());
}

void QQuickGraphsScatter::setAxisZ(QValue3DAxis *axis)
{
    QQuickGraphsItem::setAxisZ(axis);
}

QValue3DAxis *QQuickGraphsScatter::axisZ() const
{
    return static_cast<QValue3DAxis *>(QQuickGraphsItem::axisZ());
}

void QQuickGraphsScatter::disconnectSeries(QScatter3DSeries *series)
{
    QObject::disconnect(series, 0, this, 0);
}

void QQuickGraphsScatter::generatePointsForScatterModel(ScatterModel *graphModel)
{
    QList<QQuick3DModel *> itemList;
    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
        int itemCount = graphModel->series->dataProxy()->itemCount();
        if (graphModel->series->dataProxy()->itemCount() > 0)
            itemList.resize(itemCount);

        for (int i = 0; i < itemCount; i++) {
            QQuick3DModel *item = createDataItem(graphModel->series);
            item->setPickable(true);
            item->setParent(graphModel->series);
            itemList[i] = item;
        }
        graphModel->dataItems = itemList;
        markDataDirty();
    } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
        graphModel->instancingRootItem = createDataItem(graphModel->series);
        graphModel->instancingRootItem->setParent(graphModel->series);
        graphModel->instancingRootItem->setInstancing(graphModel->instancing);
        if (selectionMode() != QAbstract3DGraph::SelectionNone) {
            graphModel->selectionIndicator = createDataItem(graphModel->series);
            graphModel->instancingRootItem->setPickable(true);
        }
    }
    markSeriesVisualsDirty();
}

qsizetype QQuickGraphsScatter::getItemIndex(QQuick3DModel *item)
{
    Q_UNUSED(item);
    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy)
        return 0;

    return -1;
}

void QQuickGraphsScatter::clearSelection()
{
    setSelectedItem(invalidSelectionIndex(), 0);
}

void QQuickGraphsScatter::updateScatterGraphItemPositions(ScatterModel *graphModel)
{
    float itemSize = graphModel->series->itemSize() / m_itemScaler;
    QQuaternion meshRotation = graphModel->series->meshRotation();
    QScatterDataProxy *dataProxy = graphModel->series->dataProxy();
    QList<QQuick3DModel *> itemList = graphModel->dataItems;

    if (itemSize == 0.0f)
        itemSize = m_pointScale;

    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
        if (dataProxy->itemCount() != itemList.size())
            qWarning() << __func__ << "Item count differs from itemList count";

        for (int i = 0; i < dataProxy->itemCount(); ++i) {
            const QScatterDataItem item = dataProxy->itemAt(i);
            QQuick3DModel *dataPoint = itemList.at(i);

            QVector3D dotPos = item.position();
            if (isDotPositionInAxisRange(dotPos)) {
                dataPoint->setVisible(true);
                QQuaternion dotRot = item.rotation();
                float posX = static_cast<QValue3DAxis *>(axisX())->positionAt(dotPos.x())
                                 * scale().x() + translate().x();
                float posY = static_cast<QValue3DAxis *>(axisY())->positionAt(dotPos.y())
                                 * scale().y() + translate().y();
                float posZ = static_cast<QValue3DAxis *>(axisZ())->positionAt(dotPos.z())
                                 * scale().z() + translate().z();
                dataPoint->setPosition(QVector3D(posX, posY, posZ));
                QQuaternion totalRotation;

                if (graphModel->series->mesh() != QAbstract3DSeries::Mesh::Point)
                    totalRotation = dotRot * meshRotation;
                else
                    totalRotation = cameraTarget()->rotation();

                dataPoint->setRotation(totalRotation);
                dataPoint->setScale(QVector3D(itemSize, itemSize, itemSize));
            } else {
                dataPoint->setVisible(false);
            }
        }
    } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
        int count = dataProxy->itemCount();
        QList<DataItemHolder> positions;

        for (int i = 0; i < count; i++) {
            const QScatterDataItem &item = dataProxy->itemAt(i);
            QVector3D dotPos = item.position();

            if (isDotPositionInAxisRange(dotPos)) {
                auto posX = static_cast<QValue3DAxis *>(axisX())->positionAt(dotPos.x())
                                * scale().x()
                            + translate().x();
                auto posY = static_cast<QValue3DAxis *>(axisY())->positionAt(dotPos.y())
                                * scale().y()
                            + translate().y();
                auto posZ = static_cast<QValue3DAxis *>(axisZ())->positionAt(dotPos.z())
                                * scale().z()
                            + translate().z();

                QQuaternion totalRotation;

                if (graphModel->series->mesh() != QAbstract3DSeries::Mesh::Point)
                    totalRotation = item.rotation() * meshRotation;
                else
                    totalRotation = cameraTarget()->rotation();

                DataItemHolder dih;
                if (isPolar()) {
                    float x;
                    float z;
                    calculatePolarXZ(axisX()->positionAt(dotPos.x()),
                                     axisZ()->positionAt(dotPos.z()),
                                     x,
                                     z);
                    dih.position = {x, posY, z};
                } else {
                    dih.position = {posX, posY, posZ};
                }
                dih.rotation = totalRotation;
                dih.scale = {itemSize, itemSize, itemSize};

                positions.push_back(dih);
            } else {
                DataItemHolder dih;
                dih.hide = true;
                positions.push_back(dih);
            }
        }
        graphModel->instancing->setDataArray(positions);

        if (selectedItemInSeries(graphModel->series)) {
            QQuaternion totalRotation;

            if (graphModel->series->mesh() != QAbstract3DSeries::Mesh::Point) {
                totalRotation = graphModel->instancing->dataArray().at(m_selectedItem).rotation
                                * meshRotation;
            } else {
                totalRotation = cameraTarget()->rotation();
            }
            graphModel->selectionIndicator->setRotation(totalRotation);
            graphModel->instancing->hideDataItem(m_selectedItem);
        }
    }
}

void QQuickGraphsScatter::updateScatterGraphItemVisuals(ScatterModel *graphModel)
{
    bool useGradient = graphModel->series->d_func()->isUsingGradient();
    bool usePoint = graphModel->series->mesh() == QAbstract3DSeries::Mesh::Point;
    int itemCount = graphModel->series->dataProxy()->itemCount();

    if (useGradient) {
        if (!graphModel->seriesTexture) {
            graphModel->seriesTexture = createTexture();
            graphModel->seriesTexture->setParent(graphModel->series);
        }

        QLinearGradient gradient = graphModel->series->baseGradient();
        auto textureData = static_cast<QQuickGraphsTextureData *>(
            graphModel->seriesTexture->textureData());
        textureData->createGradient(gradient);

        if (!graphModel->highlightTexture) {
            graphModel->highlightTexture = createTexture();
            graphModel->highlightTexture->setParent(graphModel->series);
        }

        QLinearGradient highlightGradient = graphModel->series->singleHighlightGradient();
        auto highlightTextureData = static_cast<QQuickGraphsTextureData *>(
            graphModel->highlightTexture->textureData());
        highlightTextureData->createGradient(highlightGradient);
    } else {
        if (graphModel->seriesTexture) {
            graphModel->seriesTexture->deleteLater();
            graphModel->seriesTexture = nullptr;
        }

        if (graphModel->highlightTexture) {
            graphModel->highlightTexture->deleteLater();
            graphModel->highlightTexture = nullptr;
        }
    }

    bool rangeGradient = (useGradient
                          && graphModel->series->d_func()->m_colorStyle
                                 == Q3DTheme::ColorStyle::RangeGradient)
                             ? true
                             : false;

    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
        // Release resources that might not have been deleted even though deleteLater had been set
        window()->releaseResources();

        if (itemCount != graphModel->dataItems.size())
            qWarning() << __func__ << "Item count differs from itemList count";

        for (const auto &obj : std::as_const(graphModel->dataItems)) {
            updateItemMaterial(obj,
                               useGradient,
                               rangeGradient,
                               usePoint,
                               QStringLiteral(":/materials/ScatterMaterial"));
            updateMaterialProperties(obj,
                                     graphModel->seriesTexture,
                                     graphModel->series->baseColor());
        }

        if (m_selectedItem != invalidSelectionIndex() && graphModel->series == selectedSeries()) {
            QQuick3DModel *selectedItem = graphModel->dataItems.at(m_selectedItem);
            updateMaterialProperties(selectedItem,
                                     graphModel->highlightTexture,
                                     graphModel->series->singleHighlightColor());
        }
    } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
        graphModel->instancing->setRangeGradient(rangeGradient);
        if (!rangeGradient) {
            updateItemMaterial(graphModel->instancingRootItem,
                               useGradient,
                               rangeGradient,
                               usePoint,
                               QStringLiteral(":/materials/ScatterMaterialInstancing"));
            updateMaterialProperties(graphModel->instancingRootItem,
                                     graphModel->seriesTexture,
                                     graphModel->series->baseColor());
        } else {
            updateItemMaterial(graphModel->instancingRootItem,
                               useGradient,
                               rangeGradient,
                               usePoint,
                               QStringLiteral(":/materials/ScatterMaterialInstancing"));
            updateInstancedMaterialProperties(graphModel,
                                              false,
                                              graphModel->seriesTexture,
                                              graphModel->highlightTexture);

            const float scaleY = scaleWithBackground().y();
            float rangeGradientYScaler = m_rangeGradientYHelper / scaleY;

            QList<float> customData;
            customData.resize(itemCount);

            QList<DataItemHolder> instancingData = graphModel->instancing->dataArray();
            for (int i = 0; i < instancingData.size(); i++) {
                auto dih = instancingData.at(i);
                float value = (dih.position.y() + scaleY) * rangeGradientYScaler;
                customData[i] = value;
            }
            graphModel->instancing->setCustomData(customData);
        }

        if (selectedItemInSeries(graphModel->series)) {
            // Selection indicator
            if (!rangeGradient) {
                updateItemMaterial(graphModel->selectionIndicator,
                                   useGradient,
                                   rangeGradient,
                                   usePoint,
                                   QStringLiteral(":/materials/ScatterMaterial"));
                updateMaterialProperties(graphModel->selectionIndicator,
                                         graphModel->highlightTexture,
                                         graphModel->series->singleHighlightColor());
                graphModel->selectionIndicator->setCastsShadows(!usePoint);
            } else {
                // Rangegradient
                updateItemMaterial(graphModel->selectionIndicator,
                                   useGradient,
                                   rangeGradient,
                                   usePoint,
                                   QStringLiteral(":/materials/ScatterMaterial"));
                updateInstancedMaterialProperties(graphModel,
                                                  true,
                                                  nullptr,
                                                  graphModel->highlightTexture);
                graphModel->selectionIndicator->setCastsShadows(!usePoint);
            }

            const DataItemHolder &dih = graphModel->instancing->dataArray().at(m_selectedItem);

            graphModel->selectionIndicator->setPosition(dih.position);
            graphModel->selectionIndicator->setRotation(dih.rotation);
            graphModel->selectionIndicator->setScale(dih.scale);
            graphModel->selectionIndicator->setVisible(true);
            graphModel->instancing->hideDataItem(m_selectedItem);
            updateItemLabel(graphModel->selectionIndicator->position());
            graphModel->instancing->markDataDirty();
        } else if ((m_selectedItem == -1 || m_selectedItemSeries != graphModel->series)
                   && graphModel->selectionIndicator) {
            graphModel->selectionIndicator->setVisible(false);
        }
    }
}

void QQuickGraphsScatter::updateItemMaterial(QQuick3DModel *item,
                                             bool useGradient,
                                             bool rangeGradient,
                                             bool usePoint,
                                             const QString &materialName)
{
    QQmlListReference materialsRef(item, "materials");
    bool needNewMat = false;
    if (!materialsRef.size()) {
        needNewMat = true;
    } else if (materialsRef.at(0)->objectName().contains(QStringLiteral("Instancing"))
               == materialName.contains(QStringLiteral("Instancing"))) {
        needNewMat = true;
    }

    if (needNewMat) {
        materialsRef.clear();
        auto newMaterial = createQmlCustomMaterial(materialName);
        newMaterial->setObjectName(materialName);
        newMaterial->setParent(item);
        materialsRef.append(newMaterial);
    }

    auto material = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
    if (!useGradient)
        material->setProperty("colorStyle", 0);
    else if (!rangeGradient)
        material->setProperty("colorStyle", 1);
    else
        material->setProperty("colorStyle", 2);

    material->setProperty("usePoint", usePoint);
}

void QQuickGraphsScatter::updateInstancedMaterialProperties(ScatterModel *graphModel,
                                                            bool isHighlight,
                                                            QQuick3DTexture *seriesTexture,
                                                            QQuick3DTexture *highlightTexture)
{
    QQuick3DModel *model = nullptr;
    if (isHighlight)
        model = graphModel->selectionIndicator;
    else
        model = graphModel->instancingRootItem;

    QQmlListReference materialsRef(model, "materials");

    auto customMaterial = static_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));

    QVariant textureInputAsVariant = customMaterial->property("custex");
    QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant
                                                        .value<QQuick3DShaderUtilsTextureInput *>();

    if (isHighlight) {
        textureInput->setTexture(highlightTexture);

        if (selectedItemInSeries(graphModel->series)) {
            m_selectedGradientPos = graphModel->instancing->customData().at(m_selectedItem);
        }

        customMaterial->setProperty("gradientPos", m_selectedGradientPos);
    } else {
        textureInput->setTexture(seriesTexture);
    }
}

void QQuickGraphsScatter::updateMaterialProperties(QQuick3DModel *item,
                                                   QQuick3DTexture *texture,
                                                   const QColor &color)
{
    QQmlListReference materialsRef(item, "materials");
    auto customMaterial = static_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));

    int style = customMaterial->property("colorStyle").value<int>();
    if (style == 0)
        customMaterial->setProperty("uColor", color);
    else {
        QVariant textureInputAsVariant = customMaterial->property("custex");
        QQuick3DShaderUtilsTextureInput *textureInput
            = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();

        textureInput->setTexture(texture);

        const float scaleY = scaleWithBackground().y();
        float rangeGradientYScaler = m_rangeGradientYHelper / scaleY;
        float value = (item->y() + scaleY) * rangeGradientYScaler;
        customMaterial->setProperty("gradientPos", value);
    }
}

QQuick3DTexture *QQuickGraphsScatter::createTexture()
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

QQuick3DNode *QQuickGraphsScatter::createSeriesRoot()
{
    auto model = new QQuick3DNode();

    model->setParentItem(QQuick3DViewport::scene());
    return model;
}

QQuick3DModel *QQuickGraphsScatter::createDataItem(QAbstract3DSeries *series)
{
    auto model = new QQuick3DModel();
    model->setParent(this);
    model->setParentItem(QQuick3DViewport::scene());
    QString fileName = getMeshFileName(series);
    if (fileName.isEmpty())
        fileName = series->userDefinedMesh();

    model->setSource(QUrl(fileName));
    return model;
}

void QQuickGraphsScatter::removeDataItems(ScatterModel *graphModel,
                                          QAbstract3DGraph::OptimizationHint optimizationHint)
{
    if (optimizationHint == QAbstract3DGraph::OptimizationHint::Default) {
        delete graphModel->instancing;
        graphModel->instancing = nullptr;
        deleteDataItem(graphModel->instancingRootItem);
        deleteDataItem(graphModel->selectionIndicator);

        graphModel->instancingRootItem = nullptr;
        graphModel->selectionIndicator = nullptr;
    } else {
        QList<QQuick3DModel *> &items = graphModel->dataItems;
        removeDataItems(items, items.count());
    }
}

void QQuickGraphsScatter::removeDataItems(QList<QQuick3DModel *> &items, qsizetype count)
{
    for (int i = 0; i < count; ++i) {
        QQuick3DModel *item = items.takeLast();
        QQmlListReference materialsRef(item, "materials");
        if (materialsRef.size()) {
            QObject *material = materialsRef.at(0);
            delete material;
        }
        item->deleteLater();
    }
}

QList<QScatter3DSeries *> QQuickGraphsScatter::scatterSeriesList()
{
    QList<QScatter3DSeries *> scatterSeriesList;
    for (QAbstract3DSeries *abstractSeries : m_seriesList) {
        QScatter3DSeries *scatterSeries = qobject_cast<QScatter3DSeries *>(abstractSeries);
        if (scatterSeries)
            scatterSeriesList.append(scatterSeries);
    }

    return scatterSeriesList;
}

void QQuickGraphsScatter::recreateDataItems()
{
    if (!isComponentComplete())
        return;
    QList<QScatter3DSeries *> seriesList = scatterSeriesList();
    for (auto series : seriesList) {
        for (const auto &model : std::as_const(m_scatterGraphs)) {
            if (model->series == series)
                removeDataItems(model, optimizationHint());
        }
    }
    markDataDirty();
}

void QQuickGraphsScatter::recreateDataItems(const QList<ScatterModel *> &graphs)
{
    if (!isComponentComplete())
        return;
    QList<QScatter3DSeries *> seriesList = scatterSeriesList();
    for (auto series : seriesList) {
        for (const auto &model : graphs) {
            if (model->series == series)
                removeDataItems(model, optimizationHint());
        }
    }
    markDataDirty();
}

void QQuickGraphsScatter::addPointsToScatterModel(ScatterModel *graphModel, qsizetype count)
{
    for (int i = 0; i < count; i++) {
        QQuick3DModel *item = createDataItem(graphModel->series);
        item->setPickable(true);
        item->setParent(graphModel->series);
        graphModel->dataItems.push_back(item);
    }
    setSeriesVisualsDirty();
}

int QQuickGraphsScatter::sizeDifference(qsizetype size1, qsizetype size2)
{
    return size2 - size1;
}

QVector3D QQuickGraphsScatter::selectedItemPosition()
{
    QVector3D position;
    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy)
        position = {0.0f, 0.0f, 0.0f};
    else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default)
        position = {0.0f, 0.0f, 0.0f};

    return position;
}

void QQuickGraphsScatter::fixMeshFileName(QString &fileName, QAbstract3DSeries *series)
{
    auto meshType = series->mesh();
    // Should it be smooth?
    if (series->isMeshSmooth() && meshType != QAbstract3DSeries::Mesh::Point
        && meshType != QAbstract3DSeries::Mesh::UserDefined) {
        fileName += QStringLiteral("Smooth");
    }

    // Should it be filled?
    if (meshType != QAbstract3DSeries::Mesh::Sphere && meshType != QAbstract3DSeries::Mesh::Arrow
        && meshType != QAbstract3DSeries::Mesh::Minimal
        && meshType != QAbstract3DSeries::Mesh::Point
        && meshType != QAbstract3DSeries::Mesh::UserDefined) {
        fileName.append(QStringLiteral("Full"));
    }
}

QString QQuickGraphsScatter::getMeshFileName(QAbstract3DSeries *series)
{
    QString fileName = {};
    switch (series->mesh()) {
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
    case QAbstract3DSeries::Mesh::Minimal:
        fileName = QStringLiteral("defaultMeshes/minimalMesh");
        break;
    case QAbstract3DSeries::Mesh::Arrow:
        fileName = QStringLiteral("defaultMeshes/arrowMesh");
        break;
    case QAbstract3DSeries::Mesh::Point:
        fileName = shadowQuality() == QAbstract3DGraph::ShadowQuality::None
                       ? QStringLiteral("defaultMeshes/planeMesh")
                       : QStringLiteral("defaultMeshes/octagonMesh");
        break;
    case QAbstract3DSeries::Mesh::UserDefined:
        break;
    default:
        fileName = QStringLiteral("defaultMeshes/sphereMesh");
    }

    fixMeshFileName(fileName, series);

    return fileName;
}

void QQuickGraphsScatter::deleteDataItem(QQuick3DModel *item)
{
    QQmlListReference materialsRef(item, "materials");
    if (item) {
        if (materialsRef.size()) {
            QObject *material = materialsRef.at(0);
            delete material;
        }
        item->deleteLater();
        item = nullptr;
    }
}

void QQuickGraphsScatter::handleSeriesChanged(QList<QAbstract3DSeries *> changedSeries)
{
    Q_UNUSED(changedSeries)
    // TODO: generate items and remove old items
}

bool QQuickGraphsScatter::selectedItemInSeries(const QScatter3DSeries *series)
{
    return (m_selectedItem != -1 && m_selectedItemSeries == series);
}

bool QQuickGraphsScatter::isDotPositionInAxisRange(const QVector3D &dotPos)
{
    return ((dotPos.x() >= axisX()->min() && dotPos.x() <= axisX()->max())
            && (dotPos.y() >= axisY()->min() && dotPos.y() <= axisY()->max())
            && (dotPos.z() >= axisZ()->min() && dotPos.z() <= axisZ()->max()));
}

QScatter3DSeries *QQuickGraphsScatter::selectedSeries() const
{
    return m_selectedItemSeries;
}

void QQuickGraphsScatter::setSelectedItem(int index, QScatter3DSeries *series)
{
    const QScatterDataProxy *proxy = 0;

    // Series may already have been removed, so check it before setting the selection.
    if (!m_seriesList.contains(series))
        series = nullptr;

    if (series)
        proxy = series->dataProxy();

    if (!proxy || index < 0 || index >= proxy->itemCount())
        index = invalidSelectionIndex();

    if (index != m_selectedItem || series != m_selectedItemSeries) {
        bool seriesChanged = (series != m_selectedItemSeries);
        m_selectedItem = index;
        m_selectedItemSeries = series;
        m_changeTracker.selectedItemChanged = true;

        // Clear selection from other series and finally set new selection to the
        // specified series
        for (QAbstract3DSeries *otherSeries : m_seriesList) {
            QScatter3DSeries *scatterSeries = static_cast<QScatter3DSeries *>(otherSeries);
            if (scatterSeries != m_selectedItemSeries)
                scatterSeries->d_func()->setSelectedItem(invalidSelectionIndex());
        }
        if (m_selectedItemSeries)
            m_selectedItemSeries->d_func()->setSelectedItem(m_selectedItem);

        if (seriesChanged)
            emit selectedSeriesChanged(m_selectedItemSeries);

        emitNeedRender();
    }

    if (index != invalidSelectionIndex())
        itemLabel()->setVisible(true);
}

void QQuickGraphsScatter::setSelectionMode(QAbstract3DGraph::SelectionFlags mode)
{
    // We only support single item selection mode and no selection mode
    if (mode != QAbstract3DGraph::SelectionItem && mode != QAbstract3DGraph::SelectionNone) {
        qWarning("Unsupported selection mode - only none and item selection modes "
                 "are supported.");
        return;
    }

    QQuickGraphsItem::setSelectionMode(mode);
}

void QQuickGraphsScatter::handleAxisAutoAdjustRangeChangedInOrientation(
    QAbstract3DAxis::AxisOrientation orientation, bool autoAdjust)
{
    Q_UNUSED(orientation);
    Q_UNUSED(autoAdjust);
    adjustAxisRanges();
}

void QQuickGraphsScatter::handleAxisRangeChangedBySender(QObject *sender)
{
    QQuickGraphsItem::handleAxisRangeChangedBySender(sender);

    m_isDataDirty = true;

    // Update selected index - may be moved offscreen
    setSelectedItem(m_selectedItem, m_selectedItemSeries);
}

QQmlListProperty<QScatter3DSeries> QQuickGraphsScatter::seriesList()
{
    return QQmlListProperty<QScatter3DSeries>(this,
                                              this,
                                              &QQuickGraphsScatter::appendSeriesFunc,
                                              &QQuickGraphsScatter::countSeriesFunc,
                                              &QQuickGraphsScatter::atSeriesFunc,
                                              &QQuickGraphsScatter::clearSeriesFunc);
}

void QQuickGraphsScatter::appendSeriesFunc(QQmlListProperty<QScatter3DSeries> *list,
                                           QScatter3DSeries *series)
{
    reinterpret_cast<QQuickGraphsScatter *>(list->data)->addSeries(series);
}

qsizetype QQuickGraphsScatter::countSeriesFunc(QQmlListProperty<QScatter3DSeries> *list)
{
    return reinterpret_cast<QQuickGraphsScatter *>(list->data)->scatterSeriesList().size();
}

QScatter3DSeries *QQuickGraphsScatter::atSeriesFunc(QQmlListProperty<QScatter3DSeries> *list,
                                                    qsizetype index)
{
    return reinterpret_cast<QQuickGraphsScatter *>(list->data)->scatterSeriesList().at(index);
}

void QQuickGraphsScatter::clearSeriesFunc(QQmlListProperty<QScatter3DSeries> *list)
{
    QQuickGraphsScatter *declScatter = reinterpret_cast<QQuickGraphsScatter *>(list->data);
    QList<QScatter3DSeries *> realList = declScatter->scatterSeriesList();
    int count = realList.size();
    for (int i = 0; i < count; i++)
        declScatter->removeSeries(realList.at(i));
}

void QQuickGraphsScatter::addSeries(QScatter3DSeries *series)
{
    Q_ASSERT(series && series->type() == QAbstract3DSeries::SeriesType::Scatter);

    QQuickGraphsItem::addSeriesInternal(series);

    QScatter3DSeries *scatterSeries = static_cast<QScatter3DSeries *>(series);
    if (scatterSeries->selectedItem() != invalidSelectionIndex())
        setSelectedItem(scatterSeries->selectedItem(), scatterSeries);

    auto graphModel = new ScatterModel;
    graphModel->series = series;
    graphModel->seriesTexture = nullptr;
    graphModel->highlightTexture = nullptr;
    m_scatterGraphs.push_back(graphModel);

    connectSeries(series);

    if (series->selectedItem() != invalidSelectionIndex())
        setSelectedItem(series->selectedItem(), series);
}

void QQuickGraphsScatter::removeSeries(QScatter3DSeries *series)
{
    bool wasVisible = (series && series->d_func()->m_graph == this && series->isVisible());

    QQuickGraphsItem::removeSeriesInternal(series);

    if (m_selectedItemSeries == series)
        setSelectedItem(invalidSelectionIndex(), 0);

    if (wasVisible)
        adjustAxisRanges();

    series->setParent(this); // Reparent as removing will leave series parentless

    // Find scattergraph model
    for (QList<ScatterModel *>::ConstIterator it = m_scatterGraphs.cbegin();
         it != m_scatterGraphs.cend();) {
        if ((*it)->series == series) {
            removeDataItems(*it, optimizationHint());

            if ((*it)->seriesTexture)
                delete (*it)->seriesTexture;
            if ((*it)->highlightTexture)
                delete (*it)->highlightTexture;

            delete *it;
            it = m_scatterGraphs.erase(it);
        } else {
            ++it;
        }
    }

    disconnectSeries(series);
}

void QQuickGraphsScatter::handleAxisXChanged(QAbstract3DAxis *axis)
{
    emit axisXChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsScatter::handleAxisYChanged(QAbstract3DAxis *axis)
{
    emit axisYChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsScatter::handleAxisZChanged(QAbstract3DAxis *axis)
{
    emit axisZChanged(static_cast<QValue3DAxis *>(axis));
}

void QQuickGraphsScatter::handleSeriesMeshChanged()
{
    recreateDataItems();
}

void QQuickGraphsScatter::handleMeshSmoothChanged(bool enable)
{
    Q_UNUSED(enable);
    QScatter3DSeries *series = qobject_cast<QScatter3DSeries *>(sender());
    for (auto &model : std::as_const(m_scatterGraphs)) {
        if (model->series == series)
            removeDataItems(model, optimizationHint());
    }
    markDataDirty();
}

void QQuickGraphsScatter::handleArrayReset()
{
    QScatter3DSeries *series;
    if (qobject_cast<QScatterDataProxy *>(sender()))
        series = static_cast<QScatterDataProxy *>(sender())->series();
    else
        series = static_cast<QScatter3DSeries *>(sender());

    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);
    setSelectedItem(m_selectedItem, m_selectedItemSeries);
    series->d_func()->markItemLabelDirty();
    emitNeedRender();
}

void QQuickGraphsScatter::handleItemsAdded(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QScatter3DSeries *series = static_cast<QScatterDataProxy *>(sender())->series();
    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);
    emitNeedRender();
}

void QQuickGraphsScatter::handleItemsChanged(int startIndex, int count)
{
    QScatter3DSeries *series = static_cast<QScatterDataProxy *>(sender())->series();
    int oldChangeCount = m_changedItems.size();
    if (!oldChangeCount)
        m_changedItems.reserve(count);

    for (int i = 0; i < count; i++) {
        bool newItem = true;
        int candidate = startIndex + i;
        for (int j = 0; j < oldChangeCount; j++) {
            const ChangeItem &oldChangeItem = m_changedItems.at(j);
            if (oldChangeItem.index == candidate && series == oldChangeItem.series) {
                newItem = false;
                break;
            }
        }
        if (newItem) {
            ChangeItem newChangeItem = {series, candidate};
            m_changedItems.append(newChangeItem);
            if (series == m_selectedItemSeries && m_selectedItem == candidate)
                series->d_func()->markItemLabelDirty();
        }
    }

    if (count) {
        m_changeTracker.itemChanged = true;
        if (series->isVisible())
            adjustAxisRanges();
        emitNeedRender();
    }
}

void QQuickGraphsScatter::handleItemsRemoved(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QScatter3DSeries *series = static_cast<QScatterDataProxy *>(sender())->series();
    if (series == m_selectedItemSeries) {
        // If items removed from selected series before the selection, adjust the selection
        int selectedItem = m_selectedItem;
        if (startIndex <= selectedItem) {
            if ((startIndex + count) > selectedItem)
                selectedItem = -1; // Selected item removed
            else
                selectedItem -= count; // Move selected item down by amount of item removed

            setSelectedItem(selectedItem, m_selectedItemSeries);
        }
    }

    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);

    if (m_recordInsertsAndRemoves) {
        InsertRemoveRecord record(false, startIndex, count, series);
        m_insertRemoveRecords.append(record);
    }

    emitNeedRender();
}

void QQuickGraphsScatter::adjustAxisRanges()
{
    QValue3DAxis *valueAxisX = static_cast<QValue3DAxis *>(m_axisX);
    QValue3DAxis *valueAxisY = static_cast<QValue3DAxis *>(m_axisY);
    QValue3DAxis *valueAxisZ = static_cast<QValue3DAxis *>(m_axisZ);
    bool adjustX = (valueAxisX && valueAxisX->isAutoAdjustRange());
    bool adjustY = (valueAxisY && valueAxisY->isAutoAdjustRange());
    bool adjustZ = (valueAxisZ && valueAxisZ->isAutoAdjustRange());

    if (adjustX || adjustY || adjustZ) {
        float minValueX = 0.0f;
        float maxValueX = 0.0f;
        float minValueY = 0.0f;
        float maxValueY = 0.0f;
        float minValueZ = 0.0f;
        float maxValueZ = 0.0f;
        int seriesCount = m_seriesList.size();
        for (int series = 0; series < seriesCount; series++) {
            const QScatter3DSeries *scatterSeries = static_cast<QScatter3DSeries *>(
                m_seriesList.at(series));
            const QScatterDataProxy *proxy = scatterSeries->dataProxy();
            if (scatterSeries->isVisible() && proxy) {
                QVector3D minLimits;
                QVector3D maxLimits;
                proxy->d_func()->limitValues(minLimits,
                                             maxLimits,
                                             valueAxisX,
                                             valueAxisY,
                                             valueAxisZ);
                if (adjustX) {
                    if (!series) {
                        // First series initializes the values
                        minValueX = minLimits.x();
                        maxValueX = maxLimits.x();
                    } else {
                        minValueX = qMin(minValueX, minLimits.x());
                        maxValueX = qMax(maxValueX, maxLimits.x());
                    }
                }
                if (adjustY) {
                    if (!series) {
                        // First series initializes the values
                        minValueY = minLimits.y();
                        maxValueY = maxLimits.y();
                    } else {
                        minValueY = qMin(minValueY, minLimits.y());
                        maxValueY = qMax(maxValueY, maxLimits.y());
                    }
                }
                if (adjustZ) {
                    if (!series) {
                        // First series initializes the values
                        minValueZ = minLimits.z();
                        maxValueZ = maxLimits.z();
                    } else {
                        minValueZ = qMin(minValueZ, minLimits.z());
                        maxValueZ = qMax(maxValueZ, maxLimits.z());
                    }
                }
            }
        }

        static const float adjustmentRatio = 20.0f;
        static const float defaultAdjustment = 1.0f;

        if (adjustX) {
            // If all points at same coordinate, need to default to some valid range
            float adjustment = 0.0f;
            if (minValueX == maxValueX) {
                if (adjustZ) {
                    // X and Z are linked to have similar unit size, so choose the valid
                    // range based on it
                    if (minValueZ == maxValueZ)
                        adjustment = defaultAdjustment;
                    else
                        adjustment = qAbs(maxValueZ - minValueZ) / adjustmentRatio;
                } else {
                    if (valueAxisZ)
                        adjustment = qAbs(valueAxisZ->max() - valueAxisZ->min()) / adjustmentRatio;
                    else
                        adjustment = defaultAdjustment;
                }
            }
            valueAxisX->d_func()->setRange(minValueX - adjustment, maxValueX + adjustment, true);
        }
        if (adjustY) {
            // If all points at same coordinate, need to default to some valid range
            // Y-axis unit is not dependent on other axes, so simply adjust +-1.0f
            float adjustment = 0.0f;
            if (minValueY == maxValueY)
                adjustment = defaultAdjustment;
            valueAxisY->d_func()->setRange(minValueY - adjustment, maxValueY + adjustment, true);
        }
        if (adjustZ) {
            // If all points at same coordinate, need to default to some valid range
            float adjustment = 0.0f;
            if (minValueZ == maxValueZ) {
                if (adjustX) {
                    // X and Z are linked to have similar unit size, so choose the valid
                    // range based on it
                    if (minValueX == maxValueX)
                        adjustment = defaultAdjustment;
                    else
                        adjustment = qAbs(maxValueX - minValueX) / adjustmentRatio;
                } else {
                    if (valueAxisX)
                        adjustment = qAbs(valueAxisX->max() - valueAxisX->min()) / adjustmentRatio;
                    else
                        adjustment = defaultAdjustment;
                }
            }
            valueAxisZ->d_func()->setRange(minValueZ - adjustment, maxValueZ + adjustment, true);
        }
    }
}

void QQuickGraphsScatter::handleItemsInserted(int startIndex, int count)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(count);
    QScatter3DSeries *series = static_cast<QScatterDataProxy *>(sender())->series();
    if (series == m_selectedItemSeries) {
        // If items inserted to selected series before the selection, adjust the selection
        int selectedItem = m_selectedItem;
        if (startIndex <= selectedItem) {
            selectedItem += count;
            setSelectedItem(selectedItem, m_selectedItemSeries);
        }
    }

    if (series->isVisible()) {
        adjustAxisRanges();
        m_isDataDirty = true;
    }
    if (!m_changedSeriesList.contains(series))
        m_changedSeriesList.append(series);

    if (m_recordInsertsAndRemoves) {
        InsertRemoveRecord record(true, startIndex, count, series);
        m_insertRemoveRecords.append(record);
    }

    emitNeedRender();
}

bool QQuickGraphsScatter::handleMousePressedEvent(QMouseEvent *event)
{
    if (Qt::LeftButton == event->button())
        doPicking(event->pos());

    return true;
}

bool QQuickGraphsScatter::handleTouchEvent(QTouchEvent *event)
{
    if (scene()->selectionQueryPosition() != scene()->invalidSelectionPoint()
        && !event->isUpdateEvent()) {
        doPicking(event->point(0).position());
        scene()->setSelectionQueryPosition(scene()->invalidSelectionPoint());
    }

    return true;
}

bool QQuickGraphsScatter::doPicking(const QPointF &position)
{
    if (!QQuickGraphsItem::doPicking(position))
        return false;

    if (selectionMode() == QAbstract3DGraph::SelectionItem) {
        QList<QQuick3DPickResult> results = pickAll(position.x(), position.y());
        if (!results.empty()) {
            for (const auto &result : std::as_const(results)) {
                if (const auto &hit = result.objectHit()) {
                    if (hit == backgroundBB() || hit == background()) {
                        clearSelectionModel();
                        continue;
                    }
                    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                        setSelected(hit);
                        break;
                    } else if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default) {
                        setSelected(hit, result.instanceIndex());
                        break;
                    }
                }
            }
        } else {
            clearSelectionModel();
        }
    }
    return true;
}

void QQuickGraphsScatter::updateShadowQuality(QAbstract3DGraph::ShadowQuality quality)
{
    // Were shadows enabled before?
    bool prevShadowsEnabled = light()->castsShadow();
    QQuickGraphsItem::updateShadowQuality(quality);
    setSeriesVisualsDirty();

    if (prevShadowsEnabled != light()->castsShadow()) {
        // Need to change mesh for series using point type
        QList<ScatterModel *> graphs;
        for (const auto &graph : std::as_const(m_scatterGraphs)) {
            if (graph->series->mesh() == QAbstract3DSeries::Mesh::Point)
                graphs.append(graph);
        }
        recreateDataItems(graphs);
    }
}

void QQuickGraphsScatter::updateLightStrength()
{
    for (auto graphModel : m_scatterGraphs) {
        for (const auto &obj : std::as_const(graphModel->dataItems)) {
            QQmlListReference materialsRef(obj, "materials");
            auto material = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
            material->setProperty("specularBrightness", theme()->lightStrength() * 0.05);
        }
    }
}

void QQuickGraphsScatter::startRecordingRemovesAndInserts()
{
    m_recordInsertsAndRemoves = false;

    if (m_scene->selectionQueryPosition() != m_scene->invalidSelectionPoint()) {
        m_recordInsertsAndRemoves = true;
        if (m_insertRemoveRecords.size()) {
            m_insertRemoveRecords.clear();
            // Reserve some space for remove/insert records to avoid unnecessary reallocations.
            m_insertRemoveRecords.reserve(insertRemoveRecordReserveSize);
        }
    }
}

void QQuickGraphsScatter::componentComplete()
{
    QQuickGraphsItem::componentComplete();
    QObject::connect(cameraTarget(),
                     &QQuick3DNode::rotationChanged,
                     this,
                     &QQuickGraphsScatter::cameraRotationChanged);
}

void QQuickGraphsScatter::connectSeries(QScatter3DSeries *series)
{
    QObject::connect(series,
                     &QScatter3DSeries::meshChanged,
                     this,
                     &QQuickGraphsScatter::handleSeriesMeshChanged);
    QObject::connect(series,
                     &QScatter3DSeries::meshSmoothChanged,
                     this,
                     &QQuickGraphsScatter::handleMeshSmoothChanged);
}

void QQuickGraphsScatter::calculateSceneScalingFactors()
{
    float marginV = 0.0f;
    float marginH = 0.0f;
    if (margin() < 0.0f) {
        if (m_maxItemSize > m_defaultMaxSize)
            marginH = m_maxItemSize / m_itemScaler;
        else
            marginH = m_defaultMaxSize;
        marginV = marginH;
    } else {
        marginH = margin();
        marginV = margin();
    }
    if (isPolar()) {
        float polarMargin = calculatePolarBackgroundMargin();
        marginH = qMax(marginH, polarMargin);
    }

    float tHorizontalAspectRatio;
    if (isPolar())
        tHorizontalAspectRatio = 1.0f;
    else
        tHorizontalAspectRatio = horizontalAspectRatio();

    QSizeF areaSize;
    if (qFuzzyIsNull(tHorizontalAspectRatio)) {
        areaSize.setHeight(axisZ()->max() - axisZ()->min());
        areaSize.setWidth(axisX()->max() - axisX()->min());
    } else {
        areaSize.setHeight(1.0f);
        areaSize.setWidth(tHorizontalAspectRatio);
    }

    float horizontalMaxDimension;
    float scaleY = 0.0f;
    if (aspectRatio() > 2.0f) {
        horizontalMaxDimension = 2.0f;
        scaleY = 2.0f / aspectRatio();
    } else {
        horizontalMaxDimension = aspectRatio();
        scaleY = 1.0f;
    }

    if (isPolar())
        m_polarRadius = horizontalMaxDimension;

    float scaleFactor = qMax(areaSize.width(), areaSize.height());
    float scaleX = horizontalMaxDimension * areaSize.width() / scaleFactor;
    float scaleZ = horizontalMaxDimension * areaSize.height() / scaleFactor;

    setBackgroundScaleMargin({marginH, marginV, marginH});

    setScaleWithBackground({scaleX, scaleY, scaleZ});
    setScale({scaleX * 2.0f, scaleY * 2.0f, scaleZ * -2.0f});
    setTranslate({-scaleX, -scaleY, scaleZ});
}

float QQuickGraphsScatter::calculatePointScaleSize()
{
    QList<QScatter3DSeries *> series = scatterSeriesList();
    int totalDataSize = 0;
    for (const auto &scatterSeries : std::as_const(series)) {
        if (scatterSeries->isVisible())
            totalDataSize += scatterSeries->dataProxy()->array().size();
    }

    return qBound(m_defaultMinSize, 2.0f / float(qSqrt(qreal(totalDataSize))), m_defaultMaxSize);
}

void QQuickGraphsScatter::updatePointScaleSize()
{
    m_pointScale = calculatePointScaleSize();
}

void QQuickGraphsScatter::calculatePolarXZ(const float posX,
                                           const float posZ,
                                           float &x,
                                           float &z) const
{
    const qreal angle = posX * (M_PI * 2.0f);
    const qreal radius = posZ;

    x = static_cast<float>(radius * qSin(angle)) * m_polarRadius;
    z = -static_cast<float>(radius * qCos(angle)) * m_polarRadius;
}

QQuick3DModel *QQuickGraphsScatter::selected() const
{
    return m_selected;
}

void QQuickGraphsScatter::setSelected(QQuick3DModel *newSelected)
{
    if (newSelected != m_selected) {
        m_previousSelected = m_selected;
        m_selected = newSelected;

        auto series = static_cast<QScatter3DSeries *>(m_selected->parent());

        // Find scattermodel
        ScatterModel *graphModel = nullptr;

        for (const auto &model : std::as_const(m_scatterGraphs)) {
            if (model->series == series) {
                graphModel = model;
                break;
            }
        }

        if (graphModel) {
            qsizetype index = graphModel->dataItems.indexOf(m_selected);
            setSelectedItem(index, series);
            setSeriesVisualsDirty();
            setSelectedItemChanged(true);
        }
    }
}

void QQuickGraphsScatter::setSelected(QQuick3DModel *root, qsizetype index)
{
    if (index != m_selectedItem) {
        auto series = static_cast<QScatter3DSeries *>(root->parent());

        setSeriesVisualsDirty();
        setSelectedItem(index, series);
        setSelectedItemChanged(true);
    }
}

void QQuickGraphsScatter::clearSelectionModel()
{
    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default)
        clearAllSelectionInstanced();

    setSelectedItem(invalidSelectionIndex(), nullptr);

    itemLabel()->setVisible(false);
    setSeriesVisualsDirty();
    m_selected = nullptr;
    m_previousSelected = nullptr;
}

void QQuickGraphsScatter::clearAllSelectionInstanced()
{
    for (const auto &graph : m_scatterGraphs)
        graph->instancing->resetVisibilty();
}

void QQuickGraphsScatter::optimizationChanged(QAbstract3DGraph::OptimizationHint toOptimization)
{
    if (toOptimization == QAbstract3DGraph::OptimizationHint::Default) {
        for (const auto &graph : std::as_const(m_scatterGraphs))
            removeDataItems(graph, QAbstract3DGraph::OptimizationHint::Legacy);
    } else {
        for (const auto &graph : std::as_const(m_scatterGraphs))
            removeDataItems(graph, QAbstract3DGraph::OptimizationHint::Default);
    }
    setSeriesVisualsDirty();
}

void QQuickGraphsScatter::updateGraph()
{
    updatePointScaleSize();
    if (m_optimizationChanged) {
        optimizationChanged(optimizationHint());
        m_optimizationChanged = false;
    }

    for (auto graphModel : std::as_const(m_scatterGraphs)) {
        if (isDataDirty()) {
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                if (graphModel->dataItems.count() != graphModel->series->dataProxy()->itemCount()) {
                    int sizeDiff = sizeDifference(graphModel->dataItems.count(),
                                                  graphModel->series->dataProxy()->itemCount());

                    if (sizeDiff > 0)
                        addPointsToScatterModel(graphModel, sizeDiff);
                    else
                        removeDataItems(graphModel->dataItems, qAbs(sizeDiff));
                }
            } else {
                if (graphModel->instancing == nullptr) {
                    graphModel->instancing = new ScatterInstancing;
                    graphModel->instancing->setParent(graphModel->series);
                }
                if (graphModel->instancingRootItem == nullptr) {
                    graphModel->instancingRootItem = createDataItem(graphModel->series);
                    graphModel->instancingRootItem->setParent(graphModel->series);
                    graphModel->instancingRootItem->setInstancing(graphModel->instancing);
                    if (selectionMode() != QAbstract3DGraph::SelectionNone) {
                        graphModel->instancingRootItem->setPickable(true);
                        graphModel->selectionIndicator = createDataItem(graphModel->series);
                        graphModel->selectionIndicator->setVisible(false);
                    }
                }
            }

            updateScatterGraphItemPositions(graphModel);
        }

        if (isSeriesVisualsDirty() || (graphModel->instancing && graphModel->instancing->isDirty()))
            updateScatterGraphItemVisuals(graphModel);

        const bool validSelection = (m_selectedItemSeries == graphModel->series
                                     && m_selectedItem != invalidSelectionIndex())
                                    && selectedItemInRange(graphModel);

        if (validSelection) {
            QVector3D selectionPosition = {0.0f, 0.0f, 0.0f};
            if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Legacy) {
                QQuick3DModel *selectedModel = graphModel->dataItems.at(m_selectedItem);

                selectionPosition = selectedModel->position();
            } else {
                selectionPosition = graphModel->instancing->dataArray().at(m_selectedItem).position;
            }
            updateItemLabel(selectionPosition);
            QString label = m_selectedItemSeries->itemLabel();
            itemLabel()->setProperty("labelText", label);
        }
    }

    if (m_selectedItem == invalidSelectionIndex()) {
        itemLabel()->setVisible(false);
    }
}

void QQuickGraphsScatter::synchData()
{
    QList<QScatter3DSeries *> seriesList = scatterSeriesList();

    float maxItemSize = 0.0f;
    for (const auto &series : std::as_const(seriesList)) {
        if (series->isVisible()) {
            float itemSize = series->itemSize();
            if (itemSize > maxItemSize)
                maxItemSize = itemSize;
        }
    }

    m_maxItemSize = maxItemSize;

    updatePointScaleSize();
    QQuickGraphsItem::synchData();
    setMinCameraYRotation(-90.0f);

    m_pointScale = calculatePointScaleSize();

    if (hasSelectedItemChanged()) {
        if (m_selectedItem != invalidSelectionIndex()) {
            QString itemLabelText = m_selectedItemSeries->itemLabel();
            itemLabel()->setProperty("labelText", itemLabelText);
        }
        setSelectedItemChanged(false);
    }
}

void QQuickGraphsScatter::cameraRotationChanged()
{
    m_isDataDirty = true;
}

void QQuickGraphsScatter::handleOptimizationHintChange(QAbstract3DGraph::OptimizationHint hint)
{
    Q_UNUSED(hint)
    m_optimizationChanged = true;
}

bool QQuickGraphsScatter::selectedItemInRange(const ScatterModel *graphModel)
{
    qsizetype itemCount;
    if (optimizationHint() == QAbstract3DGraph::OptimizationHint::Default)
        itemCount = graphModel->instancing->dataArray().count();
    else
        itemCount = graphModel->dataItems.count();

    return m_selectedItem >= 0 && m_selectedItem < itemCount;
}
QT_END_NAMESPACE
