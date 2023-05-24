// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qml/qquickgraphsscatter_p.h"
#include "qml/declarativescene_p.h"
#include "data/qscatter3dseries_p.h"
#include "qvalue3daxis_p.h"
#include "qcategory3daxis_p.h"
#include "axis/qvalue3daxisformatter_p.h"
#include "engine/q3dcamera_p.h"
#include "quickgraphstexturedata_p.h"

#include <QtCore/QMutexLocker>
#include <QColor>
#include <QtQuick3D/private/qquick3drepeater_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>

QT_BEGIN_NAMESPACE

QQuickGraphsScatter::QQuickGraphsScatter(QQuickItem *parent)
    : QQuickGraphsItem(parent),
      m_scatterController(0)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents);
    // Create the shared component on the main GUI thread.
    m_scatterController = new Scatter3DController(boundingRect().toRect(), new Declarative3DScene);

    setSharedController(m_scatterController);

    QObject::connect(m_scatterController, &Scatter3DController::selectedSeriesChanged,
                     this, &QQuickGraphsScatter::selectedSeriesChanged);
}

QQuickGraphsScatter::~QQuickGraphsScatter()
{
    QMutexLocker locker(m_nodeMutex.data());
    const QMutexLocker locker2(mutex());

    for (auto &graphModel : m_scatterGraphs) {
        delete graphModel;
    }

    delete m_scatterController;
}

QValue3DAxis *QQuickGraphsScatter::axisX() const
{
    return static_cast<QValue3DAxis *>(m_scatterController->axisX());
}

void QQuickGraphsScatter::setAxisX(QValue3DAxis *axis)
{
    m_scatterController->setAxisX(axis);
}

QValue3DAxis *QQuickGraphsScatter::axisY() const
{
    return static_cast<QValue3DAxis *>(m_scatterController->axisY());
}

void QQuickGraphsScatter::setAxisY(QValue3DAxis *axis)
{
    m_scatterController->setAxisY(axis);
}

QValue3DAxis *QQuickGraphsScatter::axisZ() const
{
    return static_cast<QValue3DAxis *>(m_scatterController->axisZ());
}

void QQuickGraphsScatter::setAxisZ(QValue3DAxis *axis)
{
    m_scatterController->setAxisZ(axis);
}

void QQuickGraphsScatter::disconnectSeries(QScatter3DSeries *series)
{
    QObject::disconnect(series, 0, this, 0);
}

void QQuickGraphsScatter::generatePointsForScatterModel(ScatterModel *graphModel)
{
    QList<QQuick3DModel *> itemList;
    if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
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
        m_scatterController->markDataDirty();
    } else if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
        graphModel->instancingRootItem = createDataItem(graphModel->series);
        graphModel->instancingRootItem->setParent(graphModel->series);
        graphModel->instancingRootItem->setInstancing(graphModel->instancing);
        if (m_scatterController->selectionMode() != QAbstract3DGraph::SelectionNone) {
            graphModel->selectionIndicator = createDataItem(graphModel->series);
            graphModel->instancingRootItem->setPickable(true);
        }
    }
    m_scatterController->markSeriesVisualsDirty();
}

qsizetype QQuickGraphsScatter::getItemIndex(QQuick3DModel *item)
{
    Q_UNUSED(item);
    if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy)
        return 0;

    return -1;
}

void QQuickGraphsScatter::updateScatterGraphItemPositions(ScatterModel *graphModel)
{
    float itemSize = graphModel->series->itemSize() / m_itemScaler;
    QQuaternion meshRotation = graphModel->series->meshRotation();
    QScatterDataProxy *dataProxy = graphModel->series->dataProxy();
    QList<QQuick3DModel *> itemList = graphModel->dataItems;

    if (itemSize == 0.0f)
        itemSize = m_pointScale;

    if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
        if (dataProxy->itemCount() != itemList.size())
            qWarning() << __func__ << "Item count differs from itemList count";

        for (int i = 0; i < dataProxy->itemCount(); ++i) {
            const QScatterDataItem *item = dataProxy->itemAt(i);
            QQuick3DModel *dataPoint = itemList.at(i);

            QVector3D dotPos = item->position();
            if (isDotPositionInAxisRange(dotPos)) {
                dataPoint->setVisible(true);
                QQuaternion dotRot = item->rotation();
                float posX = axisX()->positionAt(dotPos.x()) * scale().x() + translate().x();
                float posY = axisY()->positionAt(dotPos.y()) * scale().y() + translate().y();
                float posZ = axisZ()->positionAt(dotPos.z()) * scale().z() + translate().z();
                dataPoint->setPosition(QVector3D(posX, posY, posZ));
                QQuaternion totalRotation;

                if (graphModel->series->mesh() != QAbstract3DSeries::MeshPoint)
                    totalRotation = dotRot * meshRotation;
                else
                    totalRotation = cameraTarget()->rotation();

                dataPoint->setRotation(totalRotation);
                dataPoint->setScale(QVector3D(itemSize, itemSize, itemSize));
            } else {
                dataPoint->setVisible(false);
            }
        }
    } else if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationDefault)  {
        int count = dataProxy->itemCount();
        QList<DataItemHolder> positions;

        for (int i = 0; i < count; i++) {
            auto item = dataProxy->itemAt(i);
            auto dotPos = item->position();

            if (isDotPositionInAxisRange(dotPos)) {
                auto posX = axisX()->positionAt(dotPos.x()) * scale().x() + translate().x();
                auto posY = axisY()->positionAt(dotPos.y()) * scale().y() + translate().y();
                auto posZ = axisZ()->positionAt(dotPos.z()) * scale().z() + translate().z();

                QQuaternion totalRotation;
                if (graphModel->series->mesh() != QAbstract3DSeries::MeshPoint)
                    totalRotation = item->rotation() * meshRotation;
                else
                    totalRotation = cameraTarget()->rotation();

                DataItemHolder dih;
                dih.position = {posX, posY, posZ};
                dih.rotation = totalRotation;
                dih.scale = {itemSize, itemSize, itemSize};

                positions.push_back(dih);
            }
        }
        graphModel->instancing->setDataArray(positions);
    }
}

void QQuickGraphsScatter::updateScatterGraphItemVisuals(ScatterModel *graphModel)
{
    bool useGradient = graphModel->series->d_func()->isUsingGradient();
    bool usePoint = graphModel->series->mesh() == QAbstract3DSeries::MeshPoint;
    int itemCount = graphModel->series->dataProxy()->itemCount();

    if (useGradient) {
        if (!graphModel->seriesTexture) {
            graphModel->seriesTexture = createTexture();
            graphModel->seriesTexture->setParent(graphModel->series);
        }

        QLinearGradient gradient = graphModel->series->baseGradient();
        auto textureData = static_cast<QuickGraphsTextureData *>(
                    graphModel->seriesTexture->textureData());
        textureData->createGradient(gradient);

        if (!graphModel->highlightTexture) {
            graphModel->highlightTexture = createTexture();
            graphModel->highlightTexture->setParent(graphModel->series);
        }

        QLinearGradient highlightGradient = graphModel->series->singleHighlightGradient();
        auto highlightTextureData = static_cast<QuickGraphsTextureData *>(
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

    bool rangeGradient = (useGradient && graphModel->series->d_func()->m_colorStyle
                          == Q3DTheme::ColorStyleRangeGradient) ? true : false;

    if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {

        if (itemCount != graphModel->dataItems.size())
            qWarning() << __func__ << "Item count differs from itemList count";

        if (!rangeGradient) {
            if (!usePoint) {
                for (const auto &obj : std::as_const(graphModel->dataItems)) {
                    updateItemMaterial(obj, useGradient, rangeGradient,
                                       QStringLiteral(":/materials/ObjectGradientMaterial"));

                    updatePrincipledMaterial(obj, graphModel->series->baseColor(),
                                             useGradient, graphModel->seriesTexture);
                }
                if (m_scatterController->m_selectedItem != invalidSelectionIndex()
                    && graphModel->series == m_scatterController->m_selectedItemSeries) {
                    QQuick3DModel *selectedItem = graphModel->dataItems.at(m_scatterController->m_selectedItem);
                    updatePrincipledMaterial(selectedItem, graphModel->series->singleHighlightColor(),
                                             useGradient, graphModel->highlightTexture);
                }
            } else {
                for (const auto &obj : std::as_const(graphModel->dataItems)) {
                    updatePointItemMaterial(obj, QStringLiteral(":/materials/PointMaterial"),
                                            QStringLiteral("pointmaterial"));
                    // Update point material
                    QQmlListReference materialsRef(obj, "materials");
                    auto pointMaterial = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
                    pointMaterial->setProperty("uColor", graphModel->series->baseColor());
                }
                if (m_scatterController->m_selectedItem != invalidSelectionIndex()
                    && graphModel->series == m_scatterController->m_selectedItemSeries) {
                    QQuick3DModel *selectedItem = graphModel->dataItems.at(m_scatterController->m_selectedItem);
                    QQmlListReference materialsRef(selectedItem, "materials");
                    auto pointMaterial = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
                    pointMaterial->setProperty("uColor", graphModel->series->singleHighlightColor());
                }
            }
        } else {
            if (!usePoint) {
                for (const auto &obj : std::as_const(graphModel->dataItems)) {
                    updateItemMaterial(obj, useGradient, rangeGradient,
                                       QStringLiteral(":/materials/RangeGradientScatterMaterial"));
                    updateCustomMaterial(obj, graphModel->seriesTexture);
                }

                if (m_scatterController->m_selectedItem != -1) {
                    QQuick3DModel *obj = graphModel->dataItems.at(m_scatterController->m_selectedItem);

                    updateCustomMaterial(obj, graphModel->highlightTexture);
                }
            } else {
                for (const auto &obj : std::as_const(graphModel->dataItems)) {
                    updatePointItemMaterial(obj,
                                            QStringLiteral(":/materials/PointRangeGradientMaterial"),
                                            QStringLiteral("pointrangegradientmaterial"));
                    // Update point material
                    updateCustomMaterial(obj, graphModel->seriesTexture);
                }

                if (m_scatterController->m_selectedItem != invalidSelectionIndex()
                    && graphModel->series == m_scatterController->selectedSeries()) {
                    QQuick3DModel *selectedItem = graphModel->dataItems.at(m_scatterController->m_selectedItem);
                    updateCustomMaterial(selectedItem, graphModel->highlightTexture);
                }
            }
        }
    } else if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
        graphModel->instancing->setRangeGradient(rangeGradient);
        if (!rangeGradient) {
            if (!usePoint) {
                updateItemMaterial(graphModel->instancingRootItem, useGradient, rangeGradient,
                                   QStringLiteral(":/materials/ObjectGradientMaterialInstancing"));
                updatePrincipledMaterial(graphModel->instancingRootItem, graphModel->series->baseColor(),
                                         useGradient, graphModel->seriesTexture);
            } else {
                QQuick3DModel *obj = graphModel->instancingRootItem;
                updateInstancedPointItemMaterial(obj,
                                                 QStringLiteral(":/materials/PointMaterialInstancing"),
                                                 QStringLiteral("pointmaterialinstancing"));
                QQmlListReference materialsRef(obj, "materials");
                QQuick3DMaterial *pointInstancingMaterial = qobject_cast<QQuick3DCustomMaterial *> (
                    materialsRef.at(0));
                pointInstancingMaterial->setProperty("uColor", graphModel->series->baseColor());
            }
        } else {
            if (!usePoint) {
                updateItemMaterial(graphModel->instancingRootItem, useGradient, rangeGradient,
                                   QStringLiteral(":/materials/RangeGradientMaterialInstancing"));
                float rangeGradientYScaler = m_rangeGradientYHelper / m_scaleY;

                updateInstancedCustomMaterial(graphModel, false, graphModel->seriesTexture);

                QList<float> customData;
                customData.resize(itemCount);

                QList<DataItemHolder> instancingData = graphModel->instancing->dataArray();
                for (int i = 0; i < instancingData.size(); i++) {
                    auto dih = instancingData.at(i);
                    float value = (dih.position.y() + m_scaleY) * rangeGradientYScaler;
                    customData[i] = value;
                }
                graphModel->instancing->setCustomData(customData);
            } else {
                QQuick3DModel *obj = graphModel->instancingRootItem;
                updateInstancedPointItemMaterial(obj,
                                                 QStringLiteral(":/materials/PointRangeGradientMaterialInstancing"),
                                                 QStringLiteral("pointrangegradientinstancingmaterial"));
                updateInstancedCustomMaterial(graphModel, false, graphModel->seriesTexture,
                                              graphModel->highlightTexture);

                float rangeGradientYScaler = m_rangeGradientYHelper / m_scaleY;

                QList<float> customData;
                customData.resize(itemCount);

                QList<DataItemHolder> instancingData = graphModel->instancing->dataArray();
                for (int i = 0; i < instancingData.size(); i++) {
                    auto dih = instancingData.at(i);
                    float value = (dih.position.y() + m_scaleY) * rangeGradientYScaler;
                    customData[i] = value;
                }
                graphModel->instancing->setCustomData(customData);
            }
        }

        if ((m_scatterController->m_selectedItem != -1
             && m_scatterController->m_selectedItemSeries == graphModel->series)
                && !m_selectionActive) {
            // Selection indicator
            if (!rangeGradient) {
                if (!usePoint) {
                    updateItemMaterial(graphModel->selectionIndicator, useGradient, rangeGradient,
                                       QStringLiteral(":/materials/ObjectGradientMaterial"));
                    updatePrincipledMaterial(graphModel->selectionIndicator,
                                             graphModel->series->singleHighlightColor(),
                                             useGradient, graphModel->highlightTexture);
                } else {
                    graphModel->selectionIndicator->setCastsShadows(false);
                    updatePointItemMaterial(graphModel->selectionIndicator,
                                            QStringLiteral(":/materials/PointMaterial"),
                                            QStringLiteral("pointmaterial"));
                    QQmlListReference materialsRef(graphModel->selectionIndicator, "materials");
                    auto pointMaterial = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
                    pointMaterial->setProperty("uColor", graphModel->series->singleHighlightColor());
                }
            } else {
                // Rangegradient
                if (!usePoint) {
                    updateItemMaterial(graphModel->selectionIndicator, useGradient, rangeGradient,
                                       QStringLiteral(":/materials/RangeGradientMaterial"));
                    updateInstancedCustomMaterial(graphModel, true, nullptr, graphModel->highlightTexture);
                } else {
                    graphModel->selectionIndicator->setCastsShadows(false);
                    updatePointItemMaterial(graphModel->selectionIndicator,
                                            QStringLiteral(":/materials/PointRangeGradientMaterial"),
                                            QStringLiteral("pointrangegradientmaterial"));
                    // Update point material
                    updateInstancedCustomMaterial(graphModel, true, nullptr, graphModel->highlightTexture);
                }
            }

            const DataItemHolder &dih = graphModel->instancing->dataArray().at(m_scatterController->m_selectedItem);

            graphModel->selectionIndicator->setPosition(dih.position);
            graphModel->selectionIndicator->setRotation(dih.rotation);
            graphModel->selectionIndicator->setScale(dih.scale);
            graphModel->selectionIndicator->setVisible(true);
            graphModel->instancing->hideDataItem(m_scatterController->m_selectedItem);
            updateItemLabel(graphModel->selectionIndicator->position());
            m_selectionActive = true;
            graphModel->instancing->markDataDirty();
        } else if ((m_scatterController->m_selectedItem == -1
                    || m_scatterController->m_selectedItemSeries != graphModel->series)
                   && graphModel->selectionIndicator) {
            graphModel->selectionIndicator->setVisible(false);
        }
    }
}

void QQuickGraphsScatter::updateItemMaterial(QQuick3DModel *item, bool useGradient,
                                             bool rangeGradient, const QString &materialName)
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
                QQuick3DCustomMaterial *customMaterial = createQmlCustomMaterial(
                            materialName);
                customMaterial->setParent(item);
                QObject *oldPrincipledMaterial = materialsRef.at(0);
                materialsRef.replace(0, customMaterial);
                delete oldPrincipledMaterial;
            }
        } else {
            // The item needs a range gradient material.
            QQuick3DCustomMaterial *customMaterial = createQmlCustomMaterial(
                materialName);
            customMaterial->setParent(item);
            materialsRef.append(customMaterial);
        }
    }
}

void QQuickGraphsScatter::updatePointItemMaterial(QQuick3DModel *item,
                                                  const QString &materialName,
                                                  const QString &objectName)
{
    QQmlListReference materialsRef(item, "materials");
    if (materialsRef.size()) {
        QObject *material = materialsRef.at(0);
        if (!material->objectName().contains(objectName)) {
            QQuick3DCustomMaterial *pointMaterial = createQmlCustomMaterial(materialName);
            pointMaterial->setParent(item);
            QObject *oldMaterial = materialsRef.at(0);
            materialsRef.replace(0, pointMaterial);
            pointMaterial->setObjectName(objectName);
            delete oldMaterial;
        }
    } else {
        QQuick3DCustomMaterial *pointMaterial = createQmlCustomMaterial(materialName);
        pointMaterial->setParent(item);
        pointMaterial->setObjectName(objectName);
        materialsRef.append(pointMaterial);
    }
}

void QQuickGraphsScatter::updateInstancedPointItemMaterial(QQuick3DModel *item,
                                                           const QString &materialName,
                                                           const QString &objectName)
{
    QQmlListReference materialsRef(item, "materials");
    if (materialsRef.size()) {
        QObject *material = materialsRef.at(0);
        if (!material->objectName().contains(objectName)) {
            QQuick3DCustomMaterial *pointMaterial = createQmlCustomMaterial(materialName);
            pointMaterial->setParent(item);
            QObject *oldMaterial = materialsRef.at(0);
            materialsRef.replace(0, pointMaterial);
            pointMaterial->setObjectName(objectName);
            delete oldMaterial;
        }
    } else {
        QQuick3DCustomMaterial *pointMaterial = createQmlCustomMaterial(materialName);
        pointMaterial->setParent(item);
        pointMaterial->setObjectName(objectName);
        materialsRef.append(pointMaterial);
    }
}

void QQuickGraphsScatter::updateInstancedCustomMaterial(ScatterModel *graphModel, bool isHighlight,
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
    QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();

    if (isHighlight) {
        textureInput->setTexture(highlightTexture);

        if ((m_scatterController->m_selectedItem != -1
             && m_scatterController->m_selectedItemSeries == graphModel->series)
                && !m_selectionActive) {
            m_selectedGradientPos = graphModel->instancing->customData().at(
                        m_scatterController->m_selectedItem);
        }

        customMaterial->setProperty("gradientPos", m_selectedGradientPos);
    } else {
        textureInput->setTexture(seriesTexture);
    }
}

void QQuickGraphsScatter::updateCustomMaterial(QQuick3DModel *item, QQuick3DTexture *texture)
{
    QQmlListReference materialsRef(item, "materials");
    auto customMaterial = static_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
    QVariant textureInputAsVariant = customMaterial->property("custex");
    QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();

    textureInput->setTexture(texture);

    float rangeGradientYScaler = m_rangeGradientYHelper / m_scaleY;
    float value = (item->y() + m_scaleY) * rangeGradientYScaler;
    customMaterial->setProperty("gradientPos", value);
}

void QQuickGraphsScatter::updatePrincipledMaterial(QQuick3DModel *model, const QColor &color,
                                                   bool useGradient, QQuick3DTexture *texture)
{
    QQmlListReference materialsRef(model, "materials");

    if (useGradient) {
        auto objectGradientMaterial = qobject_cast<QQuick3DCustomMaterial *>(materialsRef.at(0));
        QVariant textureInputAsVariant = objectGradientMaterial->property("custex");
        QQuick3DShaderUtilsTextureInput *textureInput = textureInputAsVariant.value<QQuick3DShaderUtilsTextureInput *>();

        textureInput->setTexture(texture);
    } else {
        auto principledMaterial = static_cast<QQuick3DPrincipledMaterial *>(materialsRef.at(0));
        principledMaterial->setBaseColor(color);
    }
}

QQuick3DTexture *QQuickGraphsScatter::createTexture()
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
    QString fileName = getMeshFileName(series->mesh());
    if (fileName.isEmpty())
        fileName = series->userDefinedMesh();

    model->setSource(QUrl(fileName));
    return model;
}

void QQuickGraphsScatter::removeDataItems(ScatterModel *graphModel)
{
    if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
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

void QQuickGraphsScatter::recreateDataItems()
{
    if (!isComponentComplete())
        return;
    QList<QScatter3DSeries *> seriesList = m_scatterController->scatterSeriesList();
    for (auto series : seriesList) {
        for (const auto &model : std::as_const(m_scatterGraphs)) {
            if (model->series == series)
                removeDataItems(model);
        }
    }
    m_scatterController->markDataDirty();
}

void QQuickGraphsScatter::recreateDataItems(const QList<ScatterModel *> &graphs)
{
    if (!isComponentComplete())
        return;
    QList<QScatter3DSeries *> seriesList = m_scatterController->scatterSeriesList();
    for (auto series : seriesList) {
        for (const auto &model : graphs) {
            if (model->series == series)
                removeDataItems(model);
        }
    }
    m_scatterController->markDataDirty();
}

void QQuickGraphsScatter::addPointsToScatterModel(ScatterModel *graphModel, qsizetype count)
{
    for (int i = 0; i < count; i++) {
        QQuick3DModel *item = createDataItem(graphModel->series);
        item->setPickable(true);
        item->setParent(graphModel->series);
        graphModel->dataItems.push_back(item);
    }
    m_scatterController->setSeriesVisualsDirty();
}

int QQuickGraphsScatter::sizeDifference(qsizetype size1, qsizetype size2)
{
    return size2 - size1;
}

QVector3D QQuickGraphsScatter::selectedItemPosition()
{
    QVector3D position;
    if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationLegacy)
        position = {0.0f, 0.0f, 0.0f};
    else if (m_scatterController->optimizationHints() == QAbstract3DGraph::OptimizationDefault)
        position = {0.0f, 0.0f, 0.0f};

    return position;
}

void QQuickGraphsScatter::fixMeshFileName(QString &fileName, QAbstract3DSeries::Mesh meshType)
{
    // Should it be smooth?
    if (m_smooth && meshType != QAbstract3DSeries::MeshPoint
        && meshType != QAbstract3DSeries::MeshUserDefined) {
        fileName += QStringLiteral("Smooth");
    }

    // Should it be filled?
    if (meshType != QAbstract3DSeries::MeshSphere && meshType != QAbstract3DSeries::MeshArrow
            && meshType != QAbstract3DSeries::MeshMinimal
            && meshType != QAbstract3DSeries::MeshPoint
            && meshType != QAbstract3DSeries::MeshUserDefined) {
        fileName.append(QStringLiteral("Full"));
    }
}

QString QQuickGraphsScatter::getMeshFileName(QAbstract3DSeries::Mesh meshType)
{
    QString fileName = {};
    switch (meshType) {
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
    case QAbstract3DSeries::MeshMinimal:
        fileName = QStringLiteral("defaultMeshes/minimalMesh");
        break;
    case QAbstract3DSeries::MeshArrow:
        fileName = QStringLiteral("defaultMeshes/arrowMesh");
        break;
    case QAbstract3DSeries::MeshPoint:
        fileName = shadowQuality() == QAbstract3DGraph::ShadowQualityNone
                       ? QStringLiteral("defaultMeshes/planeMesh")
                       : QStringLiteral("defaultMeshes/octagonMesh");
        break;
    case QAbstract3DSeries::MeshUserDefined:
        break;
    default:
        fileName = QStringLiteral("defaultMeshes/sphereMesh");
    }

    fixMeshFileName(fileName, meshType);

    return fileName;
}

void QQuickGraphsScatter::deleteDataItem(QQuick3DModel *item)
{
    QQmlListReference materialsRef(item, "materials");
    if (materialsRef.size()) {
        QObject *material = materialsRef.at(0);
        delete material;
    }
    item->deleteLater();
    item = nullptr;
}

void QQuickGraphsScatter::handleSeriesChanged(QList<QAbstract3DSeries *> changedSeries)
{
    Q_UNUSED(changedSeries)
    // TODO: generate items and remove old items
}

bool QQuickGraphsScatter::isDotPositionInAxisRange(const QVector3D &dotPos)
{
    return ((dotPos.x() >= axisX()->min() && dotPos.x() <= axisX()->max())
            && (dotPos.y() >= axisY()->min() && dotPos.y() <= axisY()->max())
            && (dotPos.z() >= axisZ()->min() && dotPos.z() <= axisZ()->max()));
}

QScatter3DSeries *QQuickGraphsScatter::selectedSeries() const
{
    return m_scatterController->selectedSeries();
}

void QQuickGraphsScatter::setSelectedItem(int index, QScatter3DSeries *series)
{
    m_scatterController->setSelectedItem(index, series);

    if (index != invalidSelectionIndex())
        itemLabel()->setVisible(true);
}

QQmlListProperty<QScatter3DSeries> QQuickGraphsScatter::seriesList()
{
    return QQmlListProperty<QScatter3DSeries>(this, this,
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
    return reinterpret_cast<QQuickGraphsScatter *>(list->data)->m_scatterController->scatterSeriesList().size();
}

QScatter3DSeries *QQuickGraphsScatter::atSeriesFunc(QQmlListProperty<QScatter3DSeries> *list,
                                                    qsizetype index)
{
    return reinterpret_cast<QQuickGraphsScatter *>(list->data)->m_scatterController->scatterSeriesList().at(index);
}

void QQuickGraphsScatter::clearSeriesFunc(QQmlListProperty<QScatter3DSeries> *list)
{
    QQuickGraphsScatter *declScatter = reinterpret_cast<QQuickGraphsScatter *>(list->data);
    QList<QScatter3DSeries *> realList = declScatter->m_scatterController->scatterSeriesList();
    int count = realList.size();
    for (int i = 0; i < count; i++)
        declScatter->removeSeries(realList.at(i));
}

void QQuickGraphsScatter::addSeries(QScatter3DSeries *series)
{
    m_scatterController->addSeries(series);

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
    m_scatterController->removeSeries(series);
    series->setParent(this); // Reparent as removing will leave series parentless

    // Find scattergraph model
    for (QList<ScatterModel *>::ConstIterator it = m_scatterGraphs.cbegin();
         it != m_scatterGraphs.cend();) {
        if ((*it)->series == series) {
            removeDataItems(*it);

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
    m_smooth = enable;
    recreateDataItems();
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
                    if (optimizationHints() == QAbstract3DGraph::OptimizationLegacy) {
                        setSelected(hit);
                        break;
                    } else if (optimizationHints() == QAbstract3DGraph::OptimizationDefault) {
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
    m_scatterController->setSeriesVisualsDirty();

    if (prevShadowsEnabled != light()->castsShadow()) {
        // Need to change mesh for series using point type
        QList<ScatterModel *> graphs;
        for (const auto &graph : std::as_const(m_scatterGraphs)) {
            if (graph->series->mesh() == QAbstract3DSeries::MeshPoint)
                graphs.append(graph);
        }
        recreateDataItems(graphs);
    }
}

void QQuickGraphsScatter::componentComplete()
{
    QQuickGraphsItem::componentComplete();
    QObject::connect(cameraTarget(), &QQuick3DNode::rotationChanged,
                     this, &QQuickGraphsScatter::cameraRotationChanged);
}

void QQuickGraphsScatter::connectSeries(QScatter3DSeries *series)
{
    m_smooth = series->isMeshSmooth();

    QObject::connect(series, &QScatter3DSeries::meshChanged, this,
                     &QQuickGraphsScatter::handleSeriesMeshChanged);
    QObject::connect(series, &QScatter3DSeries::meshSmoothChanged, this,
                     &QQuickGraphsScatter::handleMeshSmoothChanged);
}

void QQuickGraphsScatter::calculateSceneScalingFactors()
{
    if (m_requestedMargin < 0.0f) {
        if (m_maxItemSize > m_defaultMaxSize)
            m_hBackgroundMargin = m_maxItemSize / m_itemScaler;
        else
            m_hBackgroundMargin = m_defaultMaxSize;
        m_vBackgroundMargin = m_hBackgroundMargin;
    } else {
        m_hBackgroundMargin = m_requestedMargin;
        m_vBackgroundMargin = m_requestedMargin;
    }

    float hAspectRatio = horizontalAspectRatio();

    QSizeF areaSize;
    auto *axisX = static_cast<QValue3DAxis *>(m_scatterController->axisX());
    auto *axisZ = static_cast<QValue3DAxis *>(m_scatterController->axisZ());

    if (qFuzzyIsNull(hAspectRatio)) {
        areaSize.setHeight(axisZ->max() - axisZ->min());
        areaSize.setWidth(axisX->max() - axisX->min());
    } else {
        areaSize.setHeight(1.0f);
        areaSize.setWidth(hAspectRatio);
    }

    float horizontalMaxDimension;
    float graphAspectRatio = aspectRatio();

    if (graphAspectRatio > 2.0f) {
        horizontalMaxDimension = 2.0f;
        m_scaleY = 2.0f / graphAspectRatio;
    } else {
        horizontalMaxDimension = graphAspectRatio;
        m_scaleY = 1.0f;
    }
    float scaleFactor = qMax(areaSize.width(), areaSize.height());
    m_scaleX = horizontalMaxDimension * areaSize.width() / scaleFactor;
    m_scaleZ = horizontalMaxDimension * areaSize.height() / scaleFactor;

    setBackgroundScaleMargin({m_hBackgroundMargin, m_vBackgroundMargin, m_hBackgroundMargin});

    setLineLengthScaleFactor(0.02f);
    setScaleWithBackground({m_scaleX, m_scaleY, m_scaleZ});
    setScale({m_scaleX * 2.0f, m_scaleY * 2.0f, m_scaleZ * -2.0f});
    setTranslate({-m_scaleX, -m_scaleY, m_scaleZ});
}

float QQuickGraphsScatter::calculatePointScaleSize()
{
    QList<QScatter3DSeries *> series = m_scatterController->scatterSeriesList();
    int totalDataSize = 0;
    for (const auto &scatterSeries : std::as_const(series)) {
        if (scatterSeries->isVisible())
            totalDataSize += scatterSeries->dataProxy()->array()->size();
    }

    return qBound(m_defaultMinSize, 2.0f / float(qSqrt(qreal(totalDataSize))), m_defaultMaxSize);
}

void QQuickGraphsScatter::updatePointScaleSize()
{
    m_pointScale = calculatePointScaleSize();
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
            m_selectionActive = false;
            m_scatterController->setSeriesVisualsDirty();
            m_scatterController->setSelectedItemChanged(true);
        }
    }
}

void QQuickGraphsScatter::setSelected(QQuick3DModel *root, qsizetype index)
{
    if (index != m_scatterController->m_selectedItem) {
        auto series = static_cast<QScatter3DSeries *>(root->parent());

        m_scatterController->setSeriesVisualsDirty();
        setSelectedItem(index, series);
        m_scatterController->setSelectedItemChanged(true);
        m_selectionActive = false;
    }
}

void QQuickGraphsScatter::clearSelectionModel()
{
    if (optimizationHints() == QAbstract3DGraph::OptimizationDefault)
        clearAllSelectionInstanced();

    setSelectedItem(invalidSelectionIndex(), nullptr);

    itemLabel()->setVisible(false);
    m_scatterController->setSeriesVisualsDirty();
    m_selected = nullptr;
    m_previousSelected = nullptr;
}

void QQuickGraphsScatter::clearAllSelectionInstanced()
{
    for (const auto &graph : m_scatterGraphs)
        graph->instancing->resetVisibilty();
}

void QQuickGraphsScatter::updateGraph()
{
    updatePointScaleSize();
    for (auto graphModel : std::as_const(m_scatterGraphs)) {
        if (m_scatterController->isDataDirty()) {
            if (optimizationHints() == QAbstract3DGraph::OptimizationHint::OptimizationLegacy) {
                if (graphModel->dataItems.count() != graphModel->series->dataProxy()->itemCount()) {
                    int sizeDiff = sizeDifference(graphModel->dataItems.count(),
                                                  graphModel->series->dataProxy()->itemCount());

                    if (sizeDiff > 0)
                        addPointsToScatterModel(graphModel, sizeDiff);
                    else
                        removeDataItems(graphModel->dataItems, qAbs(sizeDiff));
                }
            } else  {
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

        if (m_scatterController->isSeriesVisualsDirty())
            updateScatterGraphItemVisuals(graphModel);

        if (m_scatterController->m_selectedItemSeries == graphModel->series
                && m_scatterController->m_selectedItem != invalidSelectionIndex()) {
            QVector3D selectionPosition = {0.0f, 0.0f, 0.0f};
            if (optimizationHints() == QAbstract3DGraph::OptimizationHint::OptimizationLegacy) {
                QQuick3DModel *selectedModel = graphModel->dataItems.at(
                            m_scatterController->m_selectedItem);

                selectionPosition = selectedModel->position();
            } else {
                selectionPosition = graphModel->instancing->dataArray().at(
                            m_scatterController->m_selectedItem).position;
            }
            updateItemLabel(selectionPosition);
            QString label = m_scatterController->m_selectedItemSeries->itemLabel();
            itemLabel()->setProperty("labelText", label);
        }
    }

    if (m_scatterController->m_selectedItem == invalidSelectionIndex()) {
        itemLabel()->setVisible(false);
    }
}

void QQuickGraphsScatter::synchData()
{
    QList<QScatter3DSeries *> seriesList = m_scatterController->scatterSeriesList();

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
    scene()->activeCamera()->d_func()->setMinYRotation(-90.0f);

    m_pointScale = calculatePointScaleSize();

    if (m_scatterController->hasSelectedItemChanged()) {
        if (m_scatterController->m_selectedItem != m_scatterController->invalidSelectionIndex()) {
            QString itemLabelText = m_scatterController->m_selectedItemSeries->itemLabel();
            itemLabel()->setProperty("labelText", itemLabelText);
        }
        m_scatterController->setSelectedItemChanged(false);
    }
}

void QQuickGraphsScatter::cameraRotationChanged()
{
    m_scatterController->m_isDataDirty = true;
}
QT_END_NAMESPACE
