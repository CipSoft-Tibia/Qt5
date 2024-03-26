// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
#ifndef QQUICKGRAPHSSCATTER_P_H
#define QQUICKGRAPHSSCATTER_P_H

#include "qscatter3dseries.h"
#include "qquickgraphsitem_p.h"
#include "scatter3dcontroller_p.h"
#include "scatterinstancing_p.h"

#include <private/graphsglobal_p.h>
#include <private/qqmldelegatemodel_p.h>

QT_BEGIN_NAMESPACE

class Q3DScatter;

class QQuickGraphsScatter : public QQuickGraphsItem
{
    Q_OBJECT
    Q_PROPERTY(QValue3DAxis *axisX READ axisX WRITE setAxisX NOTIFY axisXChanged)
    Q_PROPERTY(QValue3DAxis *axisY READ axisY WRITE setAxisY NOTIFY axisYChanged)
    Q_PROPERTY(QValue3DAxis *axisZ READ axisZ WRITE setAxisZ NOTIFY axisZChanged)
    Q_PROPERTY(QScatter3DSeries *selectedSeries READ selectedSeries NOTIFY selectedSeriesChanged)
    Q_PROPERTY(QQmlListProperty<QScatter3DSeries> seriesList READ seriesList CONSTANT)
    Q_CLASSINFO("DefaultProperty", "seriesList")

    QML_NAMED_ELEMENT(Scatter3D)

public:
    explicit QQuickGraphsScatter(QQuickItem *parent = 0);
    ~QQuickGraphsScatter();

    QValue3DAxis *axisX() const;
    void setAxisX(QValue3DAxis *axis);
    QValue3DAxis *axisY() const;
    void setAxisY(QValue3DAxis *axis);
    QValue3DAxis *axisZ() const;
    void setAxisZ(QValue3DAxis *axis);

    QQmlListProperty<QScatter3DSeries> seriesList();
    static void appendSeriesFunc(QQmlListProperty<QScatter3DSeries> *list, QScatter3DSeries *series);
    static qsizetype countSeriesFunc(QQmlListProperty<QScatter3DSeries> *list);
    static QScatter3DSeries *atSeriesFunc(QQmlListProperty<QScatter3DSeries> *list, qsizetype index);
    static void clearSeriesFunc(QQmlListProperty<QScatter3DSeries> *list);
    Q_INVOKABLE void addSeries(QScatter3DSeries *series);
    Q_INVOKABLE void removeSeries(QScatter3DSeries *series);

    QScatter3DSeries *selectedSeries() const;
    void setSelectedItem(int index, QScatter3DSeries *series);
    static inline int invalidSelectionIndex() { return -1; }

public Q_SLOTS:
    void handleAxisXChanged(QAbstract3DAxis *axis) override;
    void handleAxisYChanged(QAbstract3DAxis *axis) override;
    void handleAxisZChanged(QAbstract3DAxis *axis) override;
    void handleSeriesMeshChanged();
    void handleMeshSmoothChanged(bool enable);

Q_SIGNALS:
    void axisXChanged(QValue3DAxis *axis);
    void axisYChanged(QValue3DAxis *axis);
    void axisZChanged(QValue3DAxis *axis);
    void selectedSeriesChanged(QScatter3DSeries *series);

protected:
    void calculateSceneScalingFactors() override;
    bool handleMousePressedEvent(QMouseEvent *event) override;
    void componentComplete() override;
    bool handleTouchEvent(QTouchEvent *event) override;
    bool doPicking(const QPointF &position) override;
    void updateShadowQuality(QAbstract3DGraph::ShadowQuality quality) override;

private:

    struct ScatterModel {
        QList <QQuick3DModel *> dataItems;
        QQuick3DTexture *seriesTexture;
        QQuick3DTexture *highlightTexture;
        QScatter3DSeries *series;

        // For Static optimization
        ScatterInstancing *instancing = nullptr;
        QQuick3DModel *instancingRootItem = nullptr;
        QQuick3DModel *selectionIndicator = nullptr;

    };

    Scatter3DController *m_scatterController;
    float m_maxItemSize = 0.0f;

    const float m_defaultMinSize = 0.01f;
    const float m_defaultMaxSize = 0.1f;
    const float m_itemScaler = 3.0f;
    float m_pointScale = 0;

    const float m_indicatorScaleAdjustment = 1.1f;
    const float m_rangeGradientYHelper = 0.5f;

    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    float m_scaleZ = 1.0f;

    float m_requestedMargin = -1.0f;
    float m_vBackgroundMargin = 0.1f;
    float m_hBackgroundMargin = 0.1f;

    bool m_polarGraph = false;

    bool m_selectionActive = false;
    float m_selectedGradientPos = 0.0f;
    int m_selectedItem;
    QScatter3DSeries *m_selectedItemSeries; // Points to the series for which the bar is selected
                                            // in single series selection cases.
    QQuick3DModel *m_selected = nullptr;
    QQuick3DModel *m_previousSelected = nullptr;

    QList<ScatterModel *> m_scatterGraphs;

    void connectSeries(QScatter3DSeries *series);
    void disconnectSeries(QScatter3DSeries *series);
    qsizetype getItemIndex(QQuick3DModel *item);
//    void clearSelection();
    QVector3D selectedItemPosition();

    bool m_smooth = false;

    float m_dotSizedScale = 1.0f;

    void updateInstancedCustomMaterial(ScatterModel *graphModel, bool isHighlight = false,
                                       QQuick3DTexture *seriesTexture = nullptr,
                                       QQuick3DTexture *highlightTexture = nullptr);

    void updateItemMaterial(QQuick3DModel *item, bool useGradient, bool rangeGradient,
                            const QString &materialName);
    void updatePointItemMaterial(QQuick3DModel *item,
                            const QString &materialName, const QString &objectName);
    void updateInstancedPointItemMaterial(QQuick3DModel *item,
                                 const QString &materialName, const QString &objectName);
    void updateCustomMaterial(QQuick3DModel *item, QQuick3DTexture *texture);
    void updatePrincipledMaterial(QQuick3DModel *model, const QColor &color,
                                  bool useGradient, QQuick3DTexture *texture = nullptr);

    QQuick3DTexture *createTexture();
    QQuick3DModel *createDataItemModel(QAbstract3DSeries::Mesh meshType);
    QQuick3DNode *createSeriesRoot();
    QQuick3DModel *createDataItem(QAbstract3DSeries *series);
    void removeDataItems(ScatterModel *graphModel);
    void fixMeshFileName(QString &fileName, QAbstract3DSeries::Mesh meshType);
    QString getMeshFileName(QAbstract3DSeries::Mesh meshType);

    void deleteDataItem(QQuick3DModel *item);
    void removeDataItems(QList<QQuick3DModel *> &items, qsizetype count);
    void recreateDataItems();
    void recreateDataItems(const QList<ScatterModel *> &);
    void addPointsToScatterModel(ScatterModel *graphModel, qsizetype count);
    int sizeDifference(qsizetype size1, qsizetype size2);
    void handleSeriesChanged(QList<QAbstract3DSeries *> changedSeries);

    QColor m_selectedSeriesColor;

    bool isDotPositionInAxisRange(const QVector3D &dotPos);

    QQmlComponent *createRepeaterDelegate(QAbstract3DSeries::Mesh MeshType);
    float calculatePointScaleSize();
    void updatePointScaleSize();

    void generatePointsForScatterModel(ScatterModel *series);
    void updateScatterGraphItemPositions(ScatterModel *graphModel);
    void updateScatterGraphItemVisuals(ScatterModel *graphModel);

    QQuick3DModel *selected() const;
    void setSelected(QQuick3DModel *newSelected);
    void setSelected(QQuick3DModel *root, qsizetype index);
    void clearSelectionModel();
    void clearAllSelectionInstanced();

    void updateGraph() override;
    void synchData() override;

private slots:
    void cameraRotationChanged();

    friend class Q3DScatter;
};

QT_END_NAMESPACE
#endif // QQUICKGRAPHSSCATTER_P_H
