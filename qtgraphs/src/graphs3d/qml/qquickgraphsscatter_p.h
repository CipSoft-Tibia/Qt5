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

#include "qquickgraphsitem_p.h"
#include "qscatter3dseries.h"
#include "qvalue3daxis.h"
#include "scatterinstancing_p.h"

#include <private/graphsglobal_p.h>
#include <private/qqmldelegatemodel_p.h>

QT_BEGIN_NAMESPACE

class Q3DScatter;

struct Scatter3DChangeBitField
{
    bool selectedItemChanged : 1;
    bool itemChanged : 1;

    Scatter3DChangeBitField()
        : selectedItemChanged(true)
        , itemChanged(false)
    {}
};

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
    ~QQuickGraphsScatter() override;

    struct ChangeItem
    {
        QScatter3DSeries *series;
        int index;
    };

    void setAxisX(QValue3DAxis *axis);
    QValue3DAxis *axisX() const;
    void setAxisY(QValue3DAxis *axis);
    QValue3DAxis *axisY() const;
    void setAxisZ(QValue3DAxis *axis);
    QValue3DAxis *axisZ() const;

    QQmlListProperty<QScatter3DSeries> seriesList();
    static void appendSeriesFunc(QQmlListProperty<QScatter3DSeries> *list, QScatter3DSeries *series);
    static qsizetype countSeriesFunc(QQmlListProperty<QScatter3DSeries> *list);
    static QScatter3DSeries *atSeriesFunc(QQmlListProperty<QScatter3DSeries> *list, qsizetype index);
    static void clearSeriesFunc(QQmlListProperty<QScatter3DSeries> *list);
    Q_INVOKABLE void clearSelection() override;
    Q_INVOKABLE void addSeries(QScatter3DSeries *series);
    Q_INVOKABLE void removeSeries(QScatter3DSeries *series);
    QList<QScatter3DSeries *> scatterSeriesList();

    QScatter3DSeries *selectedSeries() const;
    void setSelectedItem(int index, QScatter3DSeries *series);
    static inline int invalidSelectionIndex() { return -1; }
    void setSelectionMode(QAbstract3DGraph::SelectionFlags mode) override;

    inline bool hasSelectedItemChanged() const { return m_changeTracker.selectedItemChanged; }
    inline void setSelectedItemChanged(bool changed)
    {
        m_changeTracker.selectedItemChanged = changed;
    }
    inline bool hasItemChanged() const { return m_changeTracker.itemChanged; }
    inline void setItemChanged(bool changed) { m_changeTracker.itemChanged = changed; }

    void handleAxisAutoAdjustRangeChangedInOrientation(QAbstract3DAxis::AxisOrientation orientation,
                                                       bool autoAdjust) override;
    void handleAxisRangeChangedBySender(QObject *sender) override;
    void adjustAxisRanges() override;
    inline bool hasChangedSeriesList() const { return !m_changedSeriesList.empty(); }
    inline bool isSeriesVisualsDirty() const { return m_isSeriesVisualsDirty; }
    inline void setSeriesVisualsDirty() { m_isSeriesVisualsDirty = true; }
    inline bool isDataDirty() const { return m_isDataDirty; }

public Q_SLOTS:
    void handleAxisXChanged(QAbstract3DAxis *axis) override;
    void handleAxisYChanged(QAbstract3DAxis *axis) override;
    void handleAxisZChanged(QAbstract3DAxis *axis) override;
    void handleSeriesMeshChanged();
    void handleMeshSmoothChanged(bool enable);

    void handleArrayReset();
    void handleItemsAdded(int startIndex, int count);
    void handleItemsChanged(int startIndex, int count);
    void handleItemsRemoved(int startIndex, int count);
    void handleItemsInserted(int startIndex, int count);

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
    void updateLightStrength() override;
    void startRecordingRemovesAndInserts() override;

private:
    Scatter3DChangeBitField m_changeTracker;
    QList<ChangeItem> m_changedItems;

    struct InsertRemoveRecord
    {
        bool m_isInsert;
        int m_startIndex;
        int m_count;
        QAbstract3DSeries *m_series;

        InsertRemoveRecord()
            : m_isInsert(false)
            , m_startIndex(0)
            , m_count(0)
            , m_series(0)
        {}

        InsertRemoveRecord(bool isInsert, int startIndex, int count, QAbstract3DSeries *series)
            : m_isInsert(isInsert)
            , m_startIndex(startIndex)
            , m_count(count)
            , m_series(series)
        {}
    };

    QList<InsertRemoveRecord> m_insertRemoveRecords;
    bool m_recordInsertsAndRemoves;

    struct ScatterModel
    {
        QList<QQuick3DModel *> dataItems;
        QQuick3DTexture *seriesTexture;
        QQuick3DTexture *highlightTexture;
        QScatter3DSeries *series;

        // For instanced, i.e. Default optimization
        ScatterInstancing *instancing = nullptr;
        QQuick3DModel *instancingRootItem = nullptr;
        QQuick3DModel *selectionIndicator = nullptr;
    };

    float m_maxItemSize = 0.0f;

    const float m_defaultMinSize = 0.01f;
    const float m_defaultMaxSize = 0.1f;
    const float m_itemScaler = 3.0f;
    float m_pointScale = 0;

    const float m_indicatorScaleAdjustment = 1.1f;
    const float m_rangeGradientYHelper = 0.5f;

    bool m_polarGraph = false;

    float m_selectedGradientPos = 0.0f;
    int m_selectedItem;
    QScatter3DSeries *m_selectedItemSeries; // Points to the series for which the bar is
                                            // selected in single series selection cases.
    QQuick3DModel *m_selected = nullptr;
    QQuick3DModel *m_previousSelected = nullptr;

    QList<ScatterModel *> m_scatterGraphs;

    bool m_optimizationChanged = false;

    void connectSeries(QScatter3DSeries *series);
    void disconnectSeries(QScatter3DSeries *series);
    qsizetype getItemIndex(QQuick3DModel *item);
    QVector3D selectedItemPosition();

    float m_dotSizedScale = 1.0f;

    void updateInstancedMaterialProperties(ScatterModel *graphModel,
                                           bool isHighlight = false,
                                           QQuick3DTexture *seriesTexture = nullptr,
                                           QQuick3DTexture *highlightTexture = nullptr);
    void updateItemMaterial(QQuick3DModel *item,
                            bool useGradient,
                            bool rangeGradient,
                            bool usePoint,
                            const QString &materialName);
    void updateMaterialProperties(QQuick3DModel *item,
                                  QQuick3DTexture *texture,
                                  const QColor &color = Qt::white);
    QQuick3DTexture *createTexture();
    QQuick3DModel *createDataItemModel(QAbstract3DSeries::Mesh meshType);
    QQuick3DNode *createSeriesRoot();
    QQuick3DModel *createDataItem(QAbstract3DSeries *series);
    void removeDataItems(ScatterModel *graphModel,
                         QAbstract3DGraph::OptimizationHint optimizationHint);
    void fixMeshFileName(QString &fileName, QAbstract3DSeries *series);
    QString getMeshFileName(QAbstract3DSeries *series);

    void deleteDataItem(QQuick3DModel *item);
    void removeDataItems(QList<QQuick3DModel *> &items, qsizetype count);
    void recreateDataItems();
    void recreateDataItems(const QList<ScatterModel *> &);
    void addPointsToScatterModel(ScatterModel *graphModel, qsizetype count);
    int sizeDifference(qsizetype size1, qsizetype size2);
    void handleSeriesChanged(QList<QAbstract3DSeries *> changedSeries);

    QColor m_selectedSeriesColor;
    bool selectedItemInSeries(const QScatter3DSeries *series);

    bool isDotPositionInAxisRange(const QVector3D &dotPos);

    QQmlComponent *createRepeaterDelegate(QAbstract3DSeries::Mesh MeshType);
    float calculatePointScaleSize();
    void updatePointScaleSize();
    void calculatePolarXZ(const float posX, const float posZ, float &x, float &z) const;

    void generatePointsForScatterModel(ScatterModel *series);
    void updateScatterGraphItemPositions(ScatterModel *graphModel);
    void updateScatterGraphItemVisuals(ScatterModel *graphModel);

    QQuick3DModel *selected() const;
    void setSelected(QQuick3DModel *newSelected);
    void setSelected(QQuick3DModel *root, qsizetype index);
    void clearSelectionModel();
    void clearAllSelectionInstanced();

    void optimizationChanged(QAbstract3DGraph::OptimizationHint toOptimization);

    void updateGraph() override;
    void synchData() override;
    void handleOptimizationHintChange(QAbstract3DGraph::OptimizationHint hint) override;

    bool selectedItemInRange(const ScatterModel *graphModel);

private slots:
    void cameraRotationChanged();

    friend class Q3DScatter;
};

QT_END_NAMESPACE
#endif // QQUICKGRAPHSSCATTER_P_H
