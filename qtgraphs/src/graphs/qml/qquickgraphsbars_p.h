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

#ifndef QQUICKGRAPHSBARS_H
#define QQUICKGRAPHSBARS_H

#include "qbar3dseries.h"
#include "qquickgraphsitem_p.h"
#include "bars3dcontroller_p.h"
#include "barinstancing_p.h"

#include <QtQuick3D/private/qquick3dmaterial_p.h>

QT_BEGIN_NAMESPACE

class Q3DBars;

class QQuickGraphsBars : public QQuickGraphsItem
{
    Q_OBJECT
    Q_PROPERTY(QCategory3DAxis *rowAxis READ rowAxis WRITE setRowAxis NOTIFY rowAxisChanged)
    Q_PROPERTY(QValue3DAxis *valueAxis READ valueAxis WRITE setValueAxis NOTIFY valueAxisChanged)
    Q_PROPERTY(QCategory3DAxis *columnAxis READ columnAxis WRITE setColumnAxis NOTIFY columnAxisChanged)
    Q_PROPERTY(bool multiSeriesUniform READ isMultiSeriesUniform WRITE setMultiSeriesUniform NOTIFY multiSeriesUniformChanged)
    Q_PROPERTY(float barThickness READ barThickness WRITE setBarThickness NOTIFY barThicknessChanged)
    Q_PROPERTY(QSizeF barSpacing READ barSpacing WRITE setBarSpacing NOTIFY barSpacingChanged)
    Q_PROPERTY(bool barSpacingRelative READ isBarSpacingRelative WRITE setBarSpacingRelative NOTIFY barSpacingRelativeChanged)
    Q_PROPERTY(QSizeF barSeriesMargin READ barSeriesMargin WRITE setBarSeriesMargin NOTIFY barSeriesMarginChanged)
    Q_PROPERTY(QQmlListProperty<QBar3DSeries> seriesList READ seriesList CONSTANT)
    Q_PROPERTY(QBar3DSeries *selectedSeries READ selectedSeries NOTIFY selectedSeriesChanged)
    Q_PROPERTY(QBar3DSeries *primarySeries READ primarySeries WRITE setPrimarySeries NOTIFY primarySeriesChanged)
    Q_PROPERTY(float floorLevel READ floorLevel WRITE setFloorLevel NOTIFY floorLevelChanged)
    Q_CLASSINFO("DefaultProperty", "seriesList")

    QML_NAMED_ELEMENT(Bars3D)

public:
    explicit QQuickGraphsBars(QQuickItem *parent = 0);
    ~QQuickGraphsBars();

    QCategory3DAxis *rowAxis() const;
    void setRowAxis(QCategory3DAxis *axis);
    QValue3DAxis *valueAxis() const;
    void setValueAxis(QValue3DAxis *axis);
    QCategory3DAxis *columnAxis() const;
    void setColumnAxis(QCategory3DAxis *axis);

    void setMultiSeriesUniform(bool uniform);
    bool isMultiSeriesUniform() const;

    void setBarThickness(float thicknessRatio);
    float barThickness() const;

    void setBarSpacing(const QSizeF &spacing);
    QSizeF barSpacing() const;

    void setBarSpacingRelative(bool relative);
    bool isBarSpacingRelative() const;

    void setBarSeriesMargin(const QSizeF &margin);
    QSizeF barSeriesMargin() const;

    QQmlListProperty<QBar3DSeries> seriesList();
    static void appendSeriesFunc(QQmlListProperty<QBar3DSeries> *list, QBar3DSeries *series);
    static qsizetype countSeriesFunc(QQmlListProperty<QBar3DSeries> *list);
    static QBar3DSeries *atSeriesFunc(QQmlListProperty<QBar3DSeries> *list, qsizetype index);
    static void clearSeriesFunc(QQmlListProperty<QBar3DSeries> *list);
    Q_INVOKABLE void addSeries(QBar3DSeries *series);
    Q_INVOKABLE void removeSeries(QBar3DSeries *series);
    Q_INVOKABLE void insertSeries(int index, QBar3DSeries *series);

    void setPrimarySeries(QBar3DSeries *series);
    QBar3DSeries *primarySeries() const;
    QBar3DSeries *selectedSeries() const;
    static inline QPoint invalidSelectionPosition(){ return QPoint(-1, -1); }

    void setFloorLevel(float level);
    float floorLevel() const;

protected:
    void componentComplete() override;
    void synchData() override;
    void updateParameters();
    void updateFloorLevel(float level);
    void updateGraph() override;
    void updateAxisRange(float min, float max) override;
    void updateAxisReversed(bool enable) override;
    void calculateSceneScalingFactors() override;
    QVector3D calculateCategoryLabelPosition(QAbstract3DAxis *axis, QVector3D labelPosition,
                                             int index) override;
    float calculateCategoryGridLinePosition(QAbstract3DAxis *axis, int index) override;
    bool handleMousePressedEvent(QMouseEvent *event) override;
    bool handleTouchEvent(QTouchEvent *event) override;
    void createSliceView() override;
    void updateSliceGraph() override;
    void handleLabelCountChanged(QQuick3DRepeater *repeater) override;
    void updateSelectionMode(QAbstract3DGraph::SelectionFlags mode) override;
    bool doPicking(const QPointF &position) override;

public Q_SLOTS:
    void handleAxisXChanged(QAbstract3DAxis *axis) override;
    void handleAxisYChanged(QAbstract3DAxis *axis) override;
    void handleAxisZChanged(QAbstract3DAxis *axis) override;
    void handleSeriesMeshChanged(QAbstract3DSeries::Mesh mesh);
    void handleMeshSmoothChanged(bool enable);
    void handleRowCountChanged();
    void handleColCountChanged();
    void handleRowColorsChanged();
    void handleCameraRotationChanged();

Q_SIGNALS:
    void rowAxisChanged(QCategory3DAxis *axis);
    void valueAxisChanged(QValue3DAxis *axis);
    void columnAxisChanged(QCategory3DAxis *axis);
    void multiSeriesUniformChanged(bool uniform);
    void barThicknessChanged(float thicknessRatio);
    void barSpacingChanged(const QSizeF &spacing);
    void barSpacingRelativeChanged(bool relative);
    void barSeriesMarginChanged(const QSizeF &margin);
    void meshFileNameChanged(const QString &filename);
    void primarySeriesChanged(QBar3DSeries *series);
    void selectedSeriesChanged(QBar3DSeries *series);
    void floorLevelChanged(float level);

private:
    Bars3DController *m_barsController;

    int m_cachedRowCount;
    int m_cachedColumnCount;
    int m_minRow;
    int m_maxRow;
    int m_minCol;
    int m_maxCol;
    int m_newRows;
    int m_newCols;

    float m_maxSceneSize;
    float m_rowWidth;
    float m_columnDepth;
    float m_maxDimension;
    float m_scaleFactor;
    float m_xScaleFactor;
    float m_zScaleFactor;

    QSizeF m_cachedBarSeriesMargin;
    QSizeF m_cachedBarThickness;
    QSizeF m_cachedBarSpacing;

    // Testing sketching
    AxisHelper m_helperAxisX;
    AxisHelper m_helperAxisY;
    AxisHelper m_helperAxisZ;

    float m_scaleXWithBackground = scaleWithBackground().x();
    float m_scaleYWithBackground = scaleWithBackground().y();
    float m_scaleZWithBackground = scaleWithBackground().z();
    float m_xScale = scale().x();
    float m_yScale = scale().y();
    float m_zScale = scale().z();

    float m_requestedMargin = -1.0f;
    float m_vBackgroundMargin = 0.1f;
    float m_hBackgroundMargin = 0.1f;

    bool m_hasNegativeValues;
    bool m_noZeroInRange;
    float m_actualFloorLevel;
    float m_heightNormalizer;
    float m_gradientFraction;
    float m_backgroundAdjustment;

    float m_minHeight;
    float m_maxHeight;

    bool m_axisRangeChanged = false;

    QQuick3DModel *m_floorBackground = nullptr;
    QQuick3DNode *m_floorBackgroundScale = nullptr;
    QQuick3DNode *m_floorBackgroundRotation = nullptr;

    // Selected bar
    QBar3DSeries *m_selectedBarSeries;
    QPoint m_selectedBarCoord;
    QVector3D m_selectedBarPos;

    // Generate bars
    struct BarModel
    {
        QQuick3DModel *model;
        QBarDataItem *barItem;
        QPoint coord;
        int visualIndex;
        float heightValue;
        QQuick3DTexture *texture;

        //instancing
        BarInstancing *instancing = nullptr;
    };

    QHash<QBar3DSeries *, QList<BarModel *> *> m_barModelsMap;
    QAbstract3DSeries::Mesh m_meshType = QAbstract3DSeries::MeshSphere;
    bool m_smooth = false;
    bool m_keepSeriesUniform;
    bool m_hasHighlightTexture = false;
    bool m_selectionActive = false;
    float m_seriesScaleX;
    float m_seriesScaleZ;
    float m_seriesStep;
    float m_seriesStart;
    float m_zeroPosition;
    int m_visibleSeriesCount;
    QQuaternion m_meshRotation;
    QQuick3DTexture *m_highlightTexture = nullptr;
    QQuick3DTexture *m_multiHighlightTexture = nullptr;
    QHash<QBar3DSeries *, QList<QQuick3DModel *> *> m_slicedBarModels;
    bool m_selectionDirty = false;

    QHash<QBar3DSeries *, QList<QQuick3DModel *> *> m_selectedModels;
    QAbstract3DGraph::SelectionFlags m_selectionMode = QAbstract3DGraph::SelectionNone;

    void calculateHeightAdjustment();
    void calculateSeriesStartPosition();
    void connectSeries(QBar3DSeries *series);
    void disconnectSeries(QBar3DSeries *series);
    void generateBars(QList<QBar3DSeries *> &barSeriesList);
    QQuick3DModel *createDataItem(QQuick3DNode *scene, QAbstract3DSeries *series);
    QString getMeshFileName();
    void fixMeshFileName(QString &fileName, QAbstract3DSeries::Mesh meshType);
    void updateBarVisuality(QBar3DSeries *series, int visualIndex);
    void updateBarPositions(QBar3DSeries *series);
    float updateBarHeightParameters(const QBarDataItem *item);
    void updateBarVisuals(QBar3DSeries *series);
    void updateItemMaterial(QQuick3DModel *item, bool useGradient, bool rangeGradient,
                            const QString &materialName);
    void updateCustomMaterial(QQuick3DModel *item, bool isHighlight = false,
                              bool isMultiHighlight = false, QQuick3DTexture *texture = nullptr);
    void updatePrincipledMaterial(QQuick3DModel *model, const QColor &color, bool useGradient,
                                  bool isHighlight, QQuick3DTexture *texture);
    void removeBarModels();
    void deleteBarModels(BarModel *barModel);
    void deleteBarItemHolders();
    QQuick3DTexture *createTexture();
    void setSelectedBar(QBar3DSeries *series, const QPoint &coord);
    void createSelectedModels(QBar3DSeries *series);
    void updateSelectedBar();
    Abstract3DController::SelectionType isSelected(int row, int bar, QBar3DSeries *series);
    void resetClickedStatus();
    void removeSlicedBarModels();
    void removeSelectedModels();

    void updateBarSpecs(float thicknessRatio, const QSizeF &spacing, bool relative);
    void updateBarSeriesMargin(const QSizeF &margin);

    friend class Bars3DController;
    friend class Q3DBars;
};

QT_END_NAMESPACE
#endif // QQUICKGRAPHSBARS_H
