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

#ifndef QQUICKGRAPHSSURFACE_P_H
#define QQUICKGRAPHSSURFACE_P_H

#include "qquickgraphsitem_p.h"
#include "qsurface3dseries.h"

#include <private/graphsglobal_p.h>

QT_BEGIN_NAMESPACE

class QValue3DAxis;
class QSurface3DSeries;
class QQuickGraphsSurface;
class SurfaceSelectionInstancing;
class Q3DSurface;

struct Surface3DChangeBitField
{
    bool selectedPointChanged : 1;
    bool rowsChanged : 1;
    bool itemChanged : 1;
    bool flipHorizontalGridChanged : 1;
    bool surfaceTextureChanged : 1;

    Surface3DChangeBitField()
        : selectedPointChanged(true)
        , rowsChanged(false)
        , itemChanged(false)
        , flipHorizontalGridChanged(true)
        , surfaceTextureChanged(true)
    {}
};

class QQuickGraphsSurface : public QQuickGraphsItem
{
    Q_OBJECT
    Q_PROPERTY(QValue3DAxis *axisX READ axisX WRITE setAxisX NOTIFY axisXChanged)
    Q_PROPERTY(QValue3DAxis *axisY READ axisY WRITE setAxisY NOTIFY axisYChanged)
    Q_PROPERTY(QValue3DAxis *axisZ READ axisZ WRITE setAxisZ NOTIFY axisZChanged)
    Q_PROPERTY(QSurface3DSeries *selectedSeries READ selectedSeries NOTIFY selectedSeriesChanged)
    Q_PROPERTY(QQmlListProperty<QSurface3DSeries> seriesList READ seriesList CONSTANT)
    Q_PROPERTY(bool flipHorizontalGrid READ flipHorizontalGrid WRITE setFlipHorizontalGrid NOTIFY
                   flipHorizontalGridChanged)
    Q_CLASSINFO("DefaultProperty", "seriesList")

    QML_NAMED_ELEMENT(Surface3D)

public:
    explicit QQuickGraphsSurface(QQuickItem *parent = 0);
    ~QQuickGraphsSurface() override;

    struct ChangeItem
    {
        QSurface3DSeries *series;
        QPoint point;
    };
    struct ChangeRow
    {
        QSurface3DSeries *series;
        int row;
    };
    enum DataDimension {
        BothAscending = 0,
        XDescending = 1,
        ZDescending = 2,
        BothDescending = XDescending | ZDescending
    };
    Q_DECLARE_FLAGS(DataDimensions, DataDimension)

    void setAxisX(QValue3DAxis *axis);
    QValue3DAxis *axisX() const;
    void setAxisY(QValue3DAxis *axis);
    QValue3DAxis *axisY() const;
    void setAxisZ(QValue3DAxis *axis);
    QValue3DAxis *axisZ() const;

    QQmlListProperty<QSurface3DSeries> seriesList();
    static void appendSeriesFunc(QQmlListProperty<QSurface3DSeries> *list, QSurface3DSeries *series);
    static qsizetype countSeriesFunc(QQmlListProperty<QSurface3DSeries> *list);
    static QSurface3DSeries *atSeriesFunc(QQmlListProperty<QSurface3DSeries> *list, qsizetype index);
    static void clearSeriesFunc(QQmlListProperty<QSurface3DSeries> *list);
    Q_INVOKABLE void addSeries(QSurface3DSeries *series);
    Q_INVOKABLE void removeSeries(QSurface3DSeries *series);
    Q_INVOKABLE void clearSelection() override;

    void setFlipHorizontalGrid(bool flip);
    bool flipHorizontalGrid() const;
    bool isFlatShadingSupported();
    QList<QSurface3DSeries *> surfaceSeriesList();
    void updateSurfaceTexture(QSurface3DSeries *series);

    static QPoint invalidSelectionPosition();
    void setSelectedPoint(const QPoint &position, QSurface3DSeries *series, bool enterSlice);

    inline QSurface3DSeries *selectedSeries() const { return m_selectedSeries; }
    void setSelectionMode(QAbstract3DGraph::SelectionFlags mode) override;

    void setDataDimensions(DataDimensions dimension) { m_dataDimensions = dimension; }
    DataDimensions dataDimensions() { return m_dataDimensions; }

    bool hasChangedSeriesList() const { return !m_changedSeriesList.isEmpty(); }
    bool isSeriesVisibilityDirty() const { return m_isSeriesVisibilityDirty; }
    void setSeriesVisibilityDirty(bool dirty) { m_isSeriesVisibilityDirty = dirty; }
    bool isDataDirty() const { return m_isDataDirty; }
    void setDataDirty(bool dirty) { m_isDataDirty = dirty; }
    bool isSeriesVisualsDirty() const { return m_isSeriesVisualsDirty; }
    void setSeriesVisualsDirty(bool dirty) { m_isSeriesVisualsDirty = dirty; }
    bool isIndexDirty() const { return m_isIndexDirty; }
    void setIndexDirty(bool dirty) { m_isIndexDirty = dirty; }

    QList<QAbstract3DSeries *> changedSeriesList() { return m_changedSeriesList; }

    bool isSelectedPointChanged() const { return m_changeTracker.selectedPointChanged; }
    void setSelectedPointChanged(bool changed) { m_changeTracker.selectedPointChanged = changed; }

    bool isFlipHorizontalGridChanged() const { return m_changeTracker.flipHorizontalGridChanged; }
    void setFlipHorizontalGridChanged(bool changed)
    {
        m_changeTracker.flipHorizontalGridChanged = changed;
    }

    bool isSurfaceTextureChanged() const { return m_changeTracker.surfaceTextureChanged; }
    void setSurfaceTextureChanged(bool changed) { m_changeTracker.surfaceTextureChanged = changed; }
    bool isChangedTexturesEmpty() const { return m_changedTextures.empty(); }
    bool hasSeriesToChangeTexture(QSurface3DSeries *series) const
    {
        return m_changedTextures.contains(series);
    }

    void handleAxisAutoAdjustRangeChangedInOrientation(QAbstract3DAxis::AxisOrientation orientation,
                                                       bool autoAdjust) override;
    void handleAxisRangeChangedBySender(QObject *sender) override;
    void handleSeriesVisibilityChangedBySender(QObject *sender) override;
    void adjustAxisRanges() override;

protected:
    void componentComplete() override;
    void synchData() override;
    void updateGraph() override;
    void calculateSceneScalingFactors() override;
    void updateSliceGraph() override;
    bool handleMousePressedEvent(QMouseEvent *event) override;
    void updateSingleHighlightColor() override;
    void updateLightStrength() override;
    void handleThemeTypeChange() override;
    bool handleTouchEvent(QTouchEvent *event) override;
    bool doPicking(const QPointF &position) override;

    void createSliceView() override;
    void updateSliceItemLabel(QString label, const QVector3D &position) override;
    void updateSelectionMode(QAbstract3DGraph::SelectionFlags mode) override;

public Q_SLOTS:
    void handleAxisXChanged(QAbstract3DAxis *axis) override;
    void handleAxisYChanged(QAbstract3DAxis *axis) override;
    void handleAxisZChanged(QAbstract3DAxis *axis) override;

    void handleFlatShadingEnabledChanged();
    void handleWireframeColorChanged();
    void handleFlipHorizontalGridChanged(bool flip);

    void handleArrayReset();
    void handleRowsAdded(int startIndex, int count);
    void handleRowsChanged(int startIndex, int count);
    void handleRowsRemoved(int startIndex, int count);
    void handleRowsInserted(int startIndex, int count);
    void handleItemChanged(int rowIndex, int columnIndex);

    void handleFlatShadingSupportedChange(bool supported);

Q_SIGNALS:
    void axisXChanged(QValue3DAxis *axis);
    void axisYChanged(QValue3DAxis *axis);
    void axisZChanged(QValue3DAxis *axis);
    void selectedSeriesChanged(QSurface3DSeries *series);
    void flipHorizontalGridChanged(bool flip);

private:
    struct SurfaceVertex
    {
        QVector3D position;
        QVector2D uv;
        QPoint coord;
    };

    struct SurfaceModel
    {
        QQuick3DModel *model;
        QQuick3DModel *gridModel;
        QQuick3DModel *sliceModel;
        QQuick3DModel *sliceGridModel;
        QQuick3DModel *proxyModel;
        QVector<SurfaceVertex> vertices;
        QVector<quint32> indices;
        QVector<quint32> gridIndices;
        QSurface3DSeries *series;
        QQuick3DTexture *texture;
        QQuick3DTexture *heightTexture;
        QQuick3DCustomMaterial *customMaterial;
        int columnCount;
        int rowCount;
        SurfaceVertex selectedVertex;
        bool picked = false;
        bool polar;
        QVector3D boundsMin;
        QVector3D boundsMax;
        QRect sampleSpace;
    };

    QVector3D getNormalizedVertex(const QSurfaceDataItem &data, bool polar, bool flipXZ);
    QRect calculateSampleSpace(const QSurfaceDataArray &array);
    QPointF mapCoordsToWorldSpace(SurfaceModel *model, const QPointF &coords);
    QPoint mapCoordsToSampleSpace(SurfaceModel *model, const QPointF &coords);
    void createIndices(SurfaceModel *model, int columnCount, int rowCount);
    void createGridlineIndices(SurfaceModel *model, int x, int y, int endX, int endY);
    void handleChangedSeries();
    void updateModel(SurfaceModel *model);
    void createProxyModel(SurfaceModel *parentModel);
    void updateProxyModel(SurfaceModel *model);
    void updateMaterial(SurfaceModel *model);
    void updateSelectedPoint();
    void addModel(QSurface3DSeries *series);
    void addSliceModel(SurfaceModel *model);
    void delete3DModel(QQuick3DModel *model);

    QVector<SurfaceModel *> m_model;
    QQuick3DModel *m_selectionPointer = nullptr;
    SurfaceSelectionInstancing *m_instancing = nullptr;
    QQuick3DModel *m_sliceSelectionPointer = nullptr;
    SurfaceSelectionInstancing *m_sliceInstancing = nullptr;

    bool m_isIndexDirty = true;
    bool m_selectionDirty = false;

    Surface3DChangeBitField m_changeTracker;
    QPoint m_selectedPoint;
    QSurface3DSeries *m_selectedSeries
        = nullptr; // Points to the series for which the point is selected in
                   // single series selection cases.
    bool m_flatShadingSupported = true;
    QList<ChangeItem> m_changedItems;
    QList<ChangeRow> m_changedRows;
    bool m_flipHorizontalGrid = false;
    QList<QSurface3DSeries *> m_changedTextures;
    bool m_isSeriesVisibilityDirty = false;

    DataDimensions m_dataDimensions;

    friend class Q3DSurface;
};

QT_END_NAMESPACE

#endif // QQUICKGRAPHSSURFACE_P_H
