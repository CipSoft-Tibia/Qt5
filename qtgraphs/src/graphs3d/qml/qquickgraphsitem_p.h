// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICKGRAPHSITEM_H
#define QQUICKGRAPHSITEM_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "../theme/thememanager_p.h"
#include "qabstract3daxis.h"
#include "qabstract3dgraph.h"
#include "qabstract3dseries.h"
#include "qcategory3daxis.h"
#include "qvalue3daxis.h"

#include <QtQuick3D/private/qquick3dviewport_p.h>

QT_BEGIN_NAMESPACE

class Q3DTheme;
class QAbstract3DAxis;
class QAbstract3DInputHandler;
class QAbstract3DSeries;
class QCustom3DItem;
class QCustom3DVolume;
class QCustom3DLabel;
class QQuick3DCustomMaterial;
class QQuick3DDirectionalLight;
class QQuick3DPrincipledMaterial;
class QQuick3DRepeater;
class QQuick3DPerspectiveCamera;
class QQuick3DOrthographicCamera;

struct Abstract3DChangeBitField
{
    bool themeChanged : 1;
    bool shadowQualityChanged : 1;
    bool selectionModeChanged : 1;
    bool optimizationHintChanged : 1;
    bool axisXTypeChanged : 1;
    bool axisYTypeChanged : 1;
    bool axisZTypeChanged : 1;
    bool axisXTitleChanged : 1;
    bool axisYTitleChanged : 1;
    bool axisZTitleChanged : 1;
    bool axisXLabelsChanged : 1;
    bool axisYLabelsChanged : 1;
    bool axisZLabelsChanged : 1;
    bool axisXRangeChanged : 1;
    bool axisYRangeChanged : 1;
    bool axisZRangeChanged : 1;
    bool axisXSegmentCountChanged : 1;
    bool axisYSegmentCountChanged : 1;
    bool axisZSegmentCountChanged : 1;
    bool axisXSubSegmentCountChanged : 1;
    bool axisYSubSegmentCountChanged : 1;
    bool axisZSubSegmentCountChanged : 1;
    bool axisXLabelFormatChanged : 1;
    bool axisYLabelFormatChanged : 1;
    bool axisZLabelFormatChanged : 1;
    bool axisXReversedChanged : 1;
    bool axisYReversedChanged : 1;
    bool axisZReversedChanged : 1;
    bool axisXFormatterChanged : 1;
    bool axisYFormatterChanged : 1;
    bool axisZFormatterChanged : 1;
    bool projectionChanged : 1;
    bool axisXLabelAutoRotationChanged : 1;
    bool axisYLabelAutoRotationChanged : 1;
    bool axisZLabelAutoRotationChanged : 1;
    bool aspectRatioChanged : 1;
    bool horizontalAspectRatioChanged : 1;
    bool axisXTitleVisibilityChanged : 1;
    bool axisYTitleVisibilityChanged : 1;
    bool axisZTitleVisibilityChanged : 1;
    bool axisXTitleFixedChanged : 1;
    bool axisYTitleFixedChanged : 1;
    bool axisZTitleFixedChanged : 1;
    bool polarChanged : 1;
    bool radialLabelOffsetChanged : 1;
    bool marginChanged : 1;

    Abstract3DChangeBitField()
        : themeChanged(true)
        , shadowQualityChanged(true)
        , selectionModeChanged(true)
        , optimizationHintChanged(true)
        , axisXTypeChanged(true)
        , axisYTypeChanged(true)
        , axisZTypeChanged(true)
        , axisXTitleChanged(true)
        , axisYTitleChanged(true)
        , axisZTitleChanged(true)
        , axisXLabelsChanged(true)
        , axisYLabelsChanged(true)
        , axisZLabelsChanged(true)
        , axisXRangeChanged(true)
        , axisYRangeChanged(true)
        , axisZRangeChanged(true)
        , axisXSegmentCountChanged(true)
        , axisYSegmentCountChanged(true)
        , axisZSegmentCountChanged(true)
        , axisXSubSegmentCountChanged(true)
        , axisYSubSegmentCountChanged(true)
        , axisZSubSegmentCountChanged(true)
        , axisXLabelFormatChanged(true)
        , axisYLabelFormatChanged(true)
        , axisZLabelFormatChanged(true)
        , axisXReversedChanged(true)
        , axisYReversedChanged(true)
        , axisZReversedChanged(true)
        , axisXFormatterChanged(true)
        , axisYFormatterChanged(true)
        , axisZFormatterChanged(true)
        , projectionChanged(true)
        , axisXLabelAutoRotationChanged(true)
        , axisYLabelAutoRotationChanged(true)
        , axisZLabelAutoRotationChanged(true)
        , aspectRatioChanged(true)
        , horizontalAspectRatioChanged(true)
        , axisXTitleVisibilityChanged(true)
        , axisYTitleVisibilityChanged(true)
        , axisZTitleVisibilityChanged(true)
        , axisXTitleFixedChanged(true)
        , axisYTitleFixedChanged(true)
        , axisZTitleFixedChanged(true)
        , polarChanged(true)
        , radialLabelOffsetChanged(true)
        , marginChanged(true)
    {}
};

class QQuickGraphsItem : public QQuick3DViewport
{
    Q_OBJECT
    Q_PROPERTY(QAbstract3DGraph::SelectionFlags selectionMode READ selectionMode WRITE
                   setSelectionMode NOTIFY selectionModeChanged)
    Q_PROPERTY(QAbstract3DGraph::ShadowQuality shadowQuality READ shadowQuality WRITE
                   setShadowQuality NOTIFY shadowQualityChanged)
    Q_PROPERTY(int msaaSamples READ msaaSamples WRITE setMsaaSamples NOTIFY msaaSamplesChanged)
    Q_PROPERTY(Q3DScene *scene READ scene NOTIFY sceneChanged)
    Q_PROPERTY(QAbstract3DInputHandler *inputHandler READ inputHandler WRITE setInputHandler NOTIFY
                   inputHandlerChanged)
    Q_PROPERTY(Q3DTheme *theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QAbstract3DGraph::RenderingMode renderingMode READ renderingMode WRITE
                   setRenderingMode NOTIFY renderingModeChanged)
    Q_PROPERTY(bool measureFps READ measureFps WRITE setMeasureFps NOTIFY measureFpsChanged)
    Q_PROPERTY(int currentFps READ currentFps NOTIFY currentFpsChanged)
    Q_PROPERTY(QQmlListProperty<QCustom3DItem> customItemList READ customItemList CONSTANT)
    Q_PROPERTY(bool orthoProjection READ isOrthoProjection WRITE setOrthoProjection NOTIFY
                   orthoProjectionChanged)
    Q_PROPERTY(QAbstract3DGraph::ElementType selectedElement READ selectedElement NOTIFY
                   selectedElementChanged)
    Q_PROPERTY(qreal aspectRatio READ aspectRatio WRITE setAspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(QAbstract3DGraph::OptimizationHint optimizationHint READ optimizationHint WRITE
                   setOptimizationHint NOTIFY optimizationHintChanged)
    Q_PROPERTY(bool polar READ isPolar WRITE setPolar NOTIFY polarChanged)
    Q_PROPERTY(float radialLabelOffset READ radialLabelOffset WRITE setRadialLabelOffset NOTIFY
                   radialLabelOffsetChanged)
    Q_PROPERTY(qreal horizontalAspectRatio READ horizontalAspectRatio WRITE setHorizontalAspectRatio
                   NOTIFY horizontalAspectRatioChanged)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(
        QVector3D queriedGraphPosition READ queriedGraphPosition NOTIFY queriedGraphPositionChanged)
    Q_PROPERTY(qreal margin READ margin WRITE setMargin NOTIFY marginChanged)
    Q_PROPERTY(float cameraXRotation READ cameraXRotation WRITE setCameraXRotation NOTIFY
                   cameraXRotationChanged)
    Q_PROPERTY(float cameraYRotation READ cameraYRotation WRITE setCameraYRotation NOTIFY
                   cameraYRotationChanged)
    Q_PROPERTY(float cameraZoomLevel READ cameraZoomLevel WRITE setCameraZoomLevel NOTIFY
                   cameraZoomLevelChanged)
    Q_PROPERTY(QAbstract3DGraph::CameraPreset cameraPreset READ cameraPreset WRITE setCameraPreset
                   NOTIFY cameraPresetChanged)
    Q_PROPERTY(QVector3D cameraTargetPosition READ cameraTargetPosition WRITE
                   setCameraTargetPosition NOTIFY cameraTargetPositionChanged)
    Q_PROPERTY(float minCameraZoomLevel READ minCameraZoomLevel WRITE setMinCameraZoomLevel NOTIFY
                   minCameraZoomLevelChanged)
    Q_PROPERTY(float maxCameraZoomLevel READ maxCameraZoomLevel WRITE setMaxCameraZoomLevel NOTIFY
                   maxCameraZoomLevelChanged)
    Q_PROPERTY(bool wrapCameraXRotation READ wrapCameraXRotation WRITE setWrapCameraXRotation NOTIFY
                   wrapCameraXRotationChanged)
    Q_PROPERTY(bool wrapCameraYRotation READ wrapCameraYRotation WRITE setWrapCameraYRotation NOTIFY
                   wrapCameraYRotationChanged)

    QML_NAMED_ELEMENT(GraphsItem3D)
    QML_UNCREATABLE("Trying to create uncreatable: GraphsItem3D.")

public:
    explicit QQuickGraphsItem(QQuickItem *parent = 0);
    ~QQuickGraphsItem() override;

    void markDataDirty();
    void markSeriesVisualsDirty();
    void markSeriesItemLabelsDirty();
    void emitNeedRender();

    void setQueriedGraphPosition(const QVector3D &position) { m_queriedGraphPosition = position; }

    virtual void handleAxisTitleChangedBySender(QObject *sender);
    virtual void handleAxisLabelsChangedBySender(QObject *sender);
    virtual void handleAxisRangeChangedBySender(QObject *sender);
    virtual void handleAxisSegmentCountChangedBySender(QObject *sender);
    virtual void handleAxisSubSegmentCountChangedBySender(QObject *sender);
    virtual void handleAxisAutoAdjustRangeChangedInOrientation(
        QAbstract3DAxis::AxisOrientation orientation, bool autoAdjust)
        = 0;
    virtual void handleAxisLabelFormatChangedBySender(QObject *sender);
    virtual void handleAxisReversedChangedBySender(QObject *sender);
    virtual void handleAxisFormatterDirtyBySender(QObject *sender);
    virtual void handleAxisLabelAutoRotationChangedBySender(QObject *sender);
    virtual void handleAxisTitleVisibilityChangedBySender(QObject *sender);
    virtual void handleAxisTitleFixedChangedBySender(QObject *sender);
    virtual void handleSeriesVisibilityChangedBySender(QObject *sender);
    virtual void adjustAxisRanges() = 0;

    bool graphPositionQueryPending() const { return m_graphPositionQueryPending; }
    void setGraphPositionQueryPending(const bool &pending)
    {
        m_graphPositionQueryPending = pending;
    }

    enum SelectionType { SelectionNone = 0, SelectionItem, SelectionRow, SelectionColumn };

    virtual void addSeriesInternal(QAbstract3DSeries *series);
    void insertSeries(int index, QAbstract3DSeries *series);
    virtual void removeSeriesInternal(QAbstract3DSeries *series);
    QList<QAbstract3DSeries *> seriesList();

    void setAxisX(QAbstract3DAxis *axis);
    QAbstract3DAxis *axisX() const;
    void setAxisY(QAbstract3DAxis *axis);
    QAbstract3DAxis *axisY() const;
    void setAxisZ(QAbstract3DAxis *axis);
    QAbstract3DAxis *axisZ() const;
    virtual void addAxis(QAbstract3DAxis *axis);
    virtual void releaseAxis(QAbstract3DAxis *axis);
    virtual QList<QAbstract3DAxis *> axes() const; // Omits default axes

    virtual void setRenderingMode(QAbstract3DGraph::RenderingMode mode);
    virtual QAbstract3DGraph::RenderingMode renderingMode() const;

    virtual void setSelectionMode(QAbstract3DGraph::SelectionFlags mode);
    virtual QAbstract3DGraph::SelectionFlags selectionMode() const;

    void doSetShadowQuality(QAbstract3DGraph::ShadowQuality quality);
    virtual void setShadowQuality(QAbstract3DGraph::ShadowQuality quality);
    virtual QAbstract3DGraph::ShadowQuality shadowQuality() const;

    virtual QAbstract3DGraph::ElementType selectedElement() const;

    virtual void setMsaaSamples(int samples);
    virtual int msaaSamples() const;

    virtual QAbstract3DInputHandler *inputHandler() const;
    virtual void setInputHandler(QAbstract3DInputHandler *inputHandler);

    virtual void addTheme(Q3DTheme *theme);
    virtual void releaseTheme(Q3DTheme *theme);
    virtual void setTheme(Q3DTheme *theme);
    virtual Q3DTheme *theme() const;
    virtual QList<Q3DTheme *> themes() const;

    bool isSlicingActive() const;
    void setSlicingActive(bool isSlicing);

    bool isCustomDataDirty() const { return m_isCustomDataDirty; }
    void setCustomDataDirty(bool dirty) { m_isCustomDataDirty = dirty; }
    bool isCustomItemDirty() const { return m_isCustomItemDirty; }
    void setCustomItemDirty(bool dirty) { m_isCustomItemDirty = dirty; }
    bool isCustomLabelItem(QCustom3DItem *item) const;
    bool isCustomVolumeItem(QCustom3DItem *item) const;
    QImage customTextureImage(QCustom3DItem *item);
    Q3DScene *scene();

    Q_INVOKABLE virtual bool hasSeries(QAbstract3DSeries *series);
    Q_INVOKABLE virtual void clearSelection() = 0;

    void deleteCustomItems();
    void deleteCustomItem(QCustom3DItem *item);
    void deleteCustomItem(const QVector3D &position);
    QList<QCustom3DItem *> customItems() const;

    Q_INVOKABLE virtual int addCustomItem(QCustom3DItem *item);
    Q_INVOKABLE virtual void removeCustomItems();
    Q_INVOKABLE virtual void removeCustomItem(QCustom3DItem *item);
    Q_INVOKABLE virtual void removeCustomItemAt(const QVector3D &position);
    Q_INVOKABLE virtual void releaseCustomItem(QCustom3DItem *item);

    Q_INVOKABLE virtual int selectedLabelIndex() const;
    Q_INVOKABLE virtual QAbstract3DAxis *selectedAxis() const;

    Q_INVOKABLE virtual int selectedCustomItemIndex() const;
    Q_INVOKABLE virtual QCustom3DItem *selectedCustomItem() const;

    QQmlListProperty<QCustom3DItem> customItemList();
    static void appendCustomItemFunc(QQmlListProperty<QCustom3DItem> *list, QCustom3DItem *item);
    static qsizetype countCustomItemFunc(QQmlListProperty<QCustom3DItem> *list);
    static QCustom3DItem *atCustomItemFunc(QQmlListProperty<QCustom3DItem> *list, qsizetype index);
    static void clearCustomItemFunc(QQmlListProperty<QCustom3DItem> *list);

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void checkWindowList(QQuickWindow *window);

    void setMeasureFps(bool enable);
    bool measureFps() const;
    int currentFps() const;

    void createInitialInputHandler();

    void setOrthoProjection(bool enable);
    bool isOrthoProjection() const;

    void setAspectRatio(qreal ratio);
    qreal aspectRatio() const;

    void setOptimizationHint(QAbstract3DGraph::OptimizationHint hint);
    QAbstract3DGraph::OptimizationHint optimizationHint() const;

    void setPolar(bool enable);
    bool isPolar() const;

    void setRadialLabelOffset(float offset);
    float radialLabelOffset() const;

    void setHorizontalAspectRatio(qreal ratio);
    qreal horizontalAspectRatio() const;

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    QVector3D queriedGraphPosition() const;

    void setMargin(qreal margin);
    qreal margin() const;

    QMutex *mutex() { return &m_mutex; }

    bool isReady() { return isComponentComplete(); }
    QQuick3DNode *rootNode() const;

    QQuick3DNode *cameraTarget() { return m_cameraTarget; }
    void setCameraTarget(QQuick3DNode *target) { m_cameraTarget = target; }

    QQuick3DModel *background() const { return m_background; }
    void setBackground(QQuick3DModel *newBackground) { m_background = newBackground; }
    inline QQuick3DModel *backgroundBB() const { return m_backgroundBB; }

    QQuick3DDirectionalLight *light() const;
    QQuick3DCustomMaterial *createQmlCustomMaterial(const QString &fileName);
    QQuick3DPrincipledMaterial *createPrincipledMaterial();

    QQuickItem *itemLabel() { return m_itemLabel; }
    QQuick3DNode *sliceItemLabel() { return m_sliceItemLabel; }

    QQuick3DModel *m_targetVisualizer;

    QQuick3DRepeater *repeaterX() const { return m_repeaterX; }
    QQuick3DRepeater *repeaterY() const { return m_repeaterY; }
    QQuick3DRepeater *repeaterZ() const { return m_repeaterZ; }

    QQuick3DNode *titleLabelX() const { return m_titleLabelX; }
    QQuick3DNode *titleLabelY() const { return m_titleLabelY; }
    QQuick3DNode *titleLabelZ() const { return m_titleLabelZ; }

    bool isXFlipped() const { return m_xFlipped; }
    void setXFlipped(bool xFlipped) { m_xFlipped = xFlipped; }
    bool isYFlipped() const { return m_yFlipped; }
    void setYFlipped(bool yFlipped) { m_yFlipped = yFlipped; }
    bool isZFlipped() const { return m_zFlipped; }
    void setZFlipped(bool zFlipped) { m_zFlipped = zFlipped; }
    QVector3D scaleWithBackground() const { return m_scaleWithBackground; }
    void setScaleWithBackground(const QVector3D &scale) { m_scaleWithBackground = scale; }
    void setBackgroundScaleMargin(const QVector3D &margin) { m_backgroundScaleMargin = margin; }
    QVector3D rotation() const { return m_rot; }
    void setRotation(const QVector3D &rotation) { m_rot = rotation; }
    QVector3D scale() const { return m_scale; }
    void setScale(const QVector3D &scale) { m_scale = scale; }
    QVector3D translate() const { return m_translate; }
    void setTranslate(const QVector3D &translate) { m_translate = translate; }

    float lineLengthScaleFactor() const { return m_lineLengthScaleFactor; }
    void setLineLengthScaleFactor(float scaleFactor) { m_lineLengthScaleFactor = scaleFactor; }
    float lineWidthScaleFactor() const { return m_lineWidthScaleFactor; }
    void setLineWidthScaleFactor(float scaleFactor) { m_lineWidthScaleFactor = scaleFactor; }
    float gridOffset() const { return m_gridOffset; }
    void setLabelMargin(float margin) { m_labelMargin = margin; }
    float labelMargin() const { return m_labelMargin; }

    QAbstract3DGraph::CameraPreset cameraPreset() const;
    void setCameraPreset(QAbstract3DGraph::CameraPreset preset);

    float cameraXRotation() const { return m_xRotation; }
    void setCameraXRotation(float rotation);
    float cameraYRotation() const { return m_yRotation; }
    void setCameraYRotation(float rotation);

    float minCameraXRotation() const { return m_minXRotation; }
    void setMinCameraXRotation(float rotation);
    float maxCameraXRotation() const { return m_maxXRotation; }
    void setMaxCameraXRotation(float rotation);

    float minCameraYRotation() const { return m_minYRotation; }
    void setMinCameraYRotation(float rotation);
    float maxCameraYRotation() const { return m_maxYRotation; }
    void setMaxCameraYRotation(float rotation);

    float cameraZoomLevel() const { return m_zoomLevel; }
    void setCameraZoomLevel(float level);

    float minCameraZoomLevel() const { return m_minZoomLevel; }
    void setMinCameraZoomLevel(float level);

    float maxCameraZoomLevel() const { return m_maxZoomLevel; }
    void setMaxCameraZoomLevel(float level);

    void setCameraTargetPosition(const QVector3D &target);
    QVector3D cameraTargetPosition() const { return m_requestedTarget; }

    bool wrapCameraXRotation() const { return m_wrapXRotation; }
    void setWrapCameraXRotation(bool wrap);

    bool wrapCameraYRotation() const { return m_wrapYRotation; }
    void setWrapCameraYRotation(bool wrap);

    void setCameraPosition(float horizontal, float vertical, float zoom = 100.0f);

    void changeLabelBackgroundColor(QQuick3DRepeater *repeater, const QColor &color);
    void changeLabelBackgroundEnabled(QQuick3DRepeater *repeater, const bool &enabled);
    void changeLabelBorderEnabled(QQuick3DRepeater *repeater, const bool &enabled);
    void changeLabelTextColor(QQuick3DRepeater *repeater, const QColor &color);
    void changeLabelFont(QQuick3DRepeater *repeater, const QFont &font);
    void changeLabelsEnabled(QQuick3DRepeater *repeater, const bool &enabled);
    void changeGridLineColor(QQuick3DRepeater *repeater, const QColor &color);
    void updateTitleLabels();
    virtual void updateSelectionMode(QAbstract3DGraph::SelectionFlags newMode);

    virtual bool doPicking(const QPointF &point);

    void minimizeMainGraph();

    int horizontalFlipFactor() const;
    void setHorizontalFlipFactor(int newHorizontalFlipFactor);

public Q_SLOTS:
    virtual void handleAxisXChanged(QAbstract3DAxis *axis) = 0;
    virtual void handleAxisYChanged(QAbstract3DAxis *axis) = 0;
    virtual void handleAxisZChanged(QAbstract3DAxis *axis) = 0;
    void handleFpsChanged();
    void windowDestroyed(QObject *obj);

    void handleAxisTitleChanged(const QString &title);
    void handleAxisLabelsChanged();
    void handleAxisRangeChanged(float min, float max);
    void handleAxisSegmentCountChanged(int count);
    void handleAxisSubSegmentCountChanged(int count);
    void handleAxisAutoAdjustRangeChanged(bool autoAdjust);
    void handleAxisLabelFormatChanged(const QString &format);
    void handleAxisReversedChanged(bool enable);
    void handleAxisFormatterDirty();
    void handleAxisLabelAutoRotationChanged(float angle);
    void handleAxisTitleVisibilityChanged(bool visible);
    void handleAxisTitleFixedChanged(bool fixed);
    void handleInputViewChanged(QAbstract3DInputHandler::InputView view);
    void handleInputPositionChanged(const QPoint &position);
    void handleSeriesVisibilityChanged(bool visible);

    void handleThemeColorStyleChanged(Q3DTheme::ColorStyle style);
    void handleThemeBaseColorsChanged(const QList<QColor> &color);
    void handleThemeBaseGradientsChanged(const QList<QLinearGradient> &gradient);
    void handleThemeSingleHighlightColorChanged(const QColor &color);
    void handleThemeSingleHighlightGradientChanged(const QLinearGradient &gradient);
    void handleThemeMultiHighlightColorChanged(const QColor &color);
    void handleThemeMultiHighlightGradientChanged(const QLinearGradient &gradient);
    void handleThemeTypeChanged(Q3DTheme::Theme theme);

    void handleRequestShadowQuality(QAbstract3DGraph::ShadowQuality quality);

    void updateCustomItem();

Q_SIGNALS:
    void selectionModeChanged(QAbstract3DGraph::SelectionFlags mode);
    void shadowQualityChanged(QAbstract3DGraph::ShadowQuality quality);
    void shadowsSupportedChanged(bool supported);
    void msaaSamplesChanged(int samples);
    void inputHandlerChanged(QAbstract3DInputHandler *inputHandler);
    void themeChanged(Q3DTheme *theme);
    void renderingModeChanged(QAbstract3DGraph::RenderingMode mode);
    void measureFpsChanged(bool enabled);
    void currentFpsChanged(int fps);
    void selectedElementChanged(QAbstract3DGraph::ElementType type);
    void orthoProjectionChanged(bool enabled);
    void aspectRatioChanged(qreal ratio);
    void optimizationHintChanged(QAbstract3DGraph::OptimizationHint hint);
    void polarChanged(bool enabled);
    void radialLabelOffsetChanged(float offset);
    void horizontalAspectRatioChanged(qreal ratio);
    void localeChanged(const QLocale &locale);
    void queriedGraphPositionChanged(const QVector3D &data);
    void marginChanged(qreal margin);
    void cameraPresetChanged(QAbstract3DGraph::CameraPreset preset);
    void cameraXRotationChanged(float rotation);
    void cameraYRotationChanged(float rotation);
    void cameraZoomLevelChanged(float zoomLevel);
    void cameraTargetPositionChanged(const QVector3D &target);
    void minCameraZoomLevelChanged(float zoomLevel);
    void maxCameraZoomLevelChanged(float zoomLevel);
    void minCameraXRotationChanged(float rotation);
    void minCameraYRotationChanged(float rotation);
    void maxCameraXRotationChanged(float rotation);
    void maxCameraYRotationChanged(float rotation);
    void wrapCameraXRotationChanged(bool wrap);
    void wrapCameraYRotationChanged(bool wrap);
    void needRender();
    void themeTypeChanged();
    void axisXChanged(QAbstract3DAxis *axis);
    void axisYChanged(QAbstract3DAxis *axis);
    void axisZChanged(QAbstract3DAxis *axis);
    void activeThemeChanged(Q3DTheme *activeTheme);

protected:
    bool event(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void touchEvent(QTouchEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    virtual void handleWindowChanged(/*QQuickWindow *win*/);
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    virtual void updateWindowParameters();
    virtual void handleSelectionModeChange(QAbstract3DGraph::SelectionFlags mode);
    virtual void handleShadowQualityChange(QAbstract3DGraph::ShadowQuality quality);
    virtual void handleSelectedElementChange(QAbstract3DGraph::ElementType type);
    virtual void handleOptimizationHintChange(QAbstract3DGraph::OptimizationHint hint);
    void keyPressEvent(QKeyEvent *ev) override;
    virtual bool handleMousePressedEvent(QMouseEvent *event);
    virtual bool handleTouchEvent(QTouchEvent *event);
    virtual void handleThemeTypeChange();
    virtual void handleParentWidthChange();
    virtual void handleParentHeightChange();
    void componentComplete() override;
    void checkSliceEnabled();

    virtual void createSliceView();

    void handleQueryPositionChanged(const QPoint &position);

    QQuick3DNode *graphNode() { return m_graphNode; }
    QQuick3DViewport *sliceView() { return m_sliceView; }

    QQmlComponent *createRepeaterDelegateComponent(const QString &fileName);
    QQuick3DRepeater *createRepeater(QQuick3DNode *parent = nullptr);
    QQuick3DNode *createTitleLabel(QQuick3DNode *parent = nullptr);
    void createItemLabel();
    QAbstract3DSeries::SeriesType m_graphType = QAbstract3DSeries::SeriesType::None;

    void updateXTitle(const QVector3D &labelRotation,
                      const QVector3D &labelTrans,
                      const QQuaternion &totalRotation,
                      float labelsMaxWidth,
                      const QVector3D &scale);
    void updateYTitle(const QVector3D &sideLabelRotation,
                      const QVector3D &backLabelRotation,
                      const QVector3D &sideLabelTrans,
                      const QVector3D &backLabelTrans,
                      const QQuaternion &totalSideRotation,
                      const QQuaternion &totalBackRotation,
                      float labelsMaxWidth,
                      const QVector3D &scale);
    void updateZTitle(const QVector3D &labelRotation,
                      const QVector3D &labelTrans,
                      const QQuaternion &totalRotation,
                      float labelsMaxWidth,
                      const QVector3D &scale);

    virtual void calculateSceneScalingFactors() = 0;
    void positionAndScaleLine(QQuick3DNode *lineNode, QVector3D scale, QVector3D position);
    int findLabelsMaxWidth(const QStringList &labels);
    virtual QVector3D calculateCategoryLabelPosition(QAbstract3DAxis *axis,
                                                     QVector3D labelPosition,
                                                     int index);
    virtual float calculateCategoryGridLinePosition(QAbstract3DAxis *axis, int index);
    float calculatePolarBackgroundMargin();
    void setFloorGridInRange(bool inRange) { m_isFloorGridInRange = inRange; }
    void setVerticalSegmentLine(bool hasVerticalLine)
    {
        m_hasVerticalSegmentLine = hasVerticalLine;
    }
    void updateGrid();
    void updateLabels();
    void updateSliceGrid();
    void updateSliceLabels();
    virtual void updateShadowQuality(QAbstract3DGraph::ShadowQuality quality);
    void updateItemLabel(const QVector3D &position);
    virtual void updateSliceItemLabel(QString label, const QVector3D &position);

    struct Volume
    {
        QQuick3DModel *model = nullptr;
        QQuick3DTexture *texture = nullptr;
        QQuick3DTextureData *textureData = nullptr;
        QQuick3DTexture *colorTexture = nullptr;
        QQuick3DTextureData *colorTextureData = nullptr;
        bool updateTextureData = false;
        bool updateColorTextureData = false;
        bool useHighDefShader = false;
        bool drawSlices = false;
        bool drawSliceFrames = false;
        QQuick3DModel *sliceFrameX = nullptr;
        QQuick3DModel *sliceFrameY = nullptr;
        QQuick3DModel *sliceFrameZ = nullptr;
        QQuick3DTexture *sliceFrameTexture = nullptr;
    };

    virtual void synchData();
    virtual void updateGraph() {}

    bool isSliceEnabled() const { return m_sliceEnabled; }
    void setSliceEnabled(bool enabled) { m_sliceEnabled = enabled; }
    bool isSliceActivatedChanged() const { return m_sliceActivatedChanged; }
    void setSliceActivatedChanged(bool changed) { m_sliceActivatedChanged = changed; }
    virtual void updateSliceGraph();
    void createSliceCamera();
    bool isSliceOrthoProjection() const { return m_sliceUseOrthoProjection; }
    void setSliceOrthoProjection(bool enable) { m_sliceUseOrthoProjection = enable; }

    virtual void updateAxisRange(float min, float max);
    virtual void updateAxisReversed(bool enable);
    virtual void updateSingleHighlightColor() {}
    virtual void updateLightStrength() {}

    virtual void handleLabelCountChanged(QQuick3DRepeater *repeater);

    bool isGridUpdated() { return m_gridUpdated; }
    void setGridUpdated(bool updated) { m_gridUpdated = updated; }

    void addInputHandler(QAbstract3DInputHandler *inputHandler);
    void releaseInputHandler(QAbstract3DInputHandler *inputHandler);
    void setActiveInputHandler(QAbstract3DInputHandler *inputHandler);
    QAbstract3DInputHandler *activeInputHandler() const { return m_activeInputHandler; };
    QList<QAbstract3DInputHandler *> inputHandlers() const { return m_inputHandlers; };

    virtual QAbstract3DAxis *createDefaultAxis(QAbstract3DAxis::AxisOrientation orientation);
    QValue3DAxis *createDefaultValueAxis();
    QCategory3DAxis *createDefaultCategoryAxis();
    void setAxisHelper(QAbstract3DAxis::AxisOrientation orientation,
                       QAbstract3DAxis *axis,
                       QAbstract3DAxis **axisPtr);
    virtual void startRecordingRemovesAndInserts();

    QSharedPointer<QMutex> m_nodeMutex;

    QMap<QCustom3DVolume *, Volume> m_customVolumes;

    Q3DScene *m_scene = nullptr;
    // Active axes
    QAbstract3DAxis *m_axisX = nullptr;
    QAbstract3DAxis *m_axisY = nullptr;
    QAbstract3DAxis *m_axisZ = nullptr;

    QList<QAbstract3DAxis *> m_axes; // List of all added axes
    bool m_isDataDirty = true;
    bool m_isCustomDataDirty = true;
    bool m_isCustomItemDirty = true;
    bool m_isSeriesVisualsDirty = true;
    bool m_renderPending = false;
    bool m_isPolar = false;
    float m_radialLabelOffset = 1.0f;
    float m_polarRadius = 2.0f;

    QList<QAbstract3DSeries *> m_seriesList;

    QList<QAbstract3DSeries *> m_changedSeriesList;

    QList<QCustom3DItem *> m_customItems;

    QAbstract3DGraph::ElementType m_clickedType = QAbstract3DGraph::ElementType::None;
    int m_selectedLabelIndex = -1;
    int m_selectedCustomItemIndex = -1;
    qreal m_margin = -1.0;

    QMutex m_renderMutex;
    QQuickGraphsItem *m_qml = nullptr;

private:
    // This is the same as the minimum bound of GridLine model.
    const float angularLineOffset = -49.98f;
    const float rotationOffset = 90.0f;

    QQuick3DModel *m_gridGeometryModel = nullptr;
    QQuick3DModel *m_sliceGridGeometryModel = nullptr;
    Abstract3DChangeBitField m_changeTracker;
    ThemeManager *m_themeManager = nullptr;
    QAbstract3DGraph::SelectionFlags m_selectionMode = QAbstract3DGraph::SelectionItem;
    QAbstract3DGraph::ShadowQuality m_shadowQuality = QAbstract3DGraph::ShadowQuality::Medium;
    bool m_useOrthoProjection = false;
    qreal m_aspectRatio = 2.0;
    qreal m_horizontalAspectRatio = 0.0;
    QAbstract3DGraph::OptimizationHint m_optimizationHint
        = QAbstract3DGraph::OptimizationHint::Default;
    QLocale m_locale;
    QVector3D m_queriedGraphPosition;
    bool m_graphPositionQueryPending = false;

    QQuick3DNode *m_graphNode = nullptr;
    QQuick3DModel *m_background = nullptr;
    QQuick3DModel *m_backgroundBB = nullptr;
    QQuick3DNode *m_backgroundScale = nullptr;
    QQuick3DNode *m_backgroundRotation = nullptr;

    QQuick3DRepeater *m_repeaterX = nullptr;
    QQuick3DRepeater *m_repeaterY = nullptr;
    QQuick3DRepeater *m_repeaterZ = nullptr;
    std::unique_ptr<QQmlComponent> m_delegateModelX;
    std::unique_ptr<QQmlComponent> m_delegateModelY;
    std::unique_ptr<QQmlComponent> m_delegateModelZ;

    QQuick3DNode *m_titleLabelX = nullptr;
    QQuick3DNode *m_titleLabelY = nullptr;
    QQuick3DNode *m_titleLabelZ = nullptr;

    QQuickItem *m_itemLabel = nullptr;
    QQuick3DNode *m_sliceItemLabel = nullptr;

    QQuick3DViewport *m_sliceView = nullptr;

    QQuick3DRepeater *m_sliceHorizontalLabelRepeater = nullptr;
    QQuick3DRepeater *m_sliceVerticalLabelRepeater = nullptr;
    std::unique_ptr<QQmlComponent> m_labelDelegate;

    QQuick3DNode *m_sliceHorizontalTitleLabel = nullptr;
    QQuick3DNode *m_sliceVerticalTitleLabel = nullptr;

    QQuick3DNode *m_cameraTarget = nullptr;
    QQuick3DDirectionalLight *m_light = nullptr;
    QQuick3DPerspectiveCamera *m_pCamera = nullptr;
    QQuick3DOrthographicCamera *m_oCamera = nullptr;
    QRectF m_cachedGeometry;
    QAbstract3DGraph::RenderingMode m_renderMode
        = QAbstract3DGraph::RenderingMode::DirectToBackground;
    int m_samples = 0;
    int m_windowSamples = 0;
    QSize m_initialisedSize = QSize(0, 0);
    bool m_runningInDesigner;
    QMutex m_mutex;

    bool m_xFlipped = false;
    bool m_yFlipped = false;
    bool m_zFlipped = false;

    bool m_flipScales;
    int m_horizontalFlipFactor = 1;

    bool m_isFloorGridInRange = false;
    bool m_hasVerticalSegmentLine = true;

    QVector3D m_scaleWithBackground = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D m_backgroundScaleMargin = QVector3D(0.0f, 0.0f, 0.0f);

    QVector3D m_rot = QVector3D(1.0f, 1.0f, 1.0f);

    QVector3D m_scale = QVector3D(1.0f, 1.0f, 1.0f);

    QVector3D m_translate = QVector3D(1.0f, 1.0f, 1.0f);

    QVector3D m_labelScale = QVector3D(0.01f, 0.01f, 0.0f);

    float m_gridOffset = 0.002f;
    float m_lineWidthScaleFactor = 0.0001f;
    float m_lineLengthScaleFactor = 0.02f;

    float m_labelMargin = .1f;

    bool m_sliceEnabled = false;
    bool m_sliceActivatedChanged = false;

    bool m_gridUpdated = false;

    bool m_validVolumeSlice = false;

    QVector3D m_labelPosition = QVector3D();
    QVector3D m_fontScaled = QVector3D();

    float m_initialZoomLevel = -1.0f;
    void setUpCamera();
    void setUpLight();
    void graphPositionAt(const QPoint &point);
    void updateCamera();
    void updateRadialLabelOffset();
    QVector3D calculateLabelRotation(float labelAutoAngle);
    void updateCustomData();
    void updateCustomLabelsRotation();
    float fontScaleFactor(float pointSize);
    float labelAdjustment(float width);
    void gridLineCountHelper(QAbstract3DAxis *axis, int &lineCount, int &sublineCount);
    QVector3D graphPosToAbsolute(const QVector3D &position);

    void createVolumeMaterial(QCustom3DVolume *volume, Volume &volumeItem);
    QQuick3DModel *createSliceFrame(Volume &volumeItem);
    void updateSliceFrameMaterials(QCustom3DVolume *volume, Volume &volumeItem);
    void updateCustomVolumes();

    bool m_sliceUseOrthoProjection = false;

    QHash<QQuickGraphsItem *, QQuickWindow *> m_graphWindowList = {};
    QHash<QCustom3DLabel *, QQuick3DNode *> m_customLabelList = {};
    QHash<QCustom3DItem *, QQuick3DModel *> m_customItemList = {};
    QList<QCustom3DItem *> m_pendingCustomItemList = {};

    int m_currentFps = -1;
    bool m_measureFps = false;

    QAbstract3DGraph::CameraPreset m_activePreset = QAbstract3DGraph::CameraPreset::NoPreset;
    float m_xRotation = 0.0f;
    float m_yRotation = 0.0f;
    float m_minXRotation = -180.0f;
    float m_maxXRotation = 180.0f;
    float m_minYRotation = 0.0f;
    float m_maxYRotation = 90.0f;
    bool m_wrapXRotation = true;
    bool m_wrapYRotation = false;

    float m_zoomLevel = 100.0f;
    float m_minZoomLevel = 10.0f;
    float m_maxZoomLevel = 500.0f;

    bool m_gridUpdate = false;

    QVector3D m_requestedTarget = QVector3D();

    QAbstract3DInputHandler *m_activeInputHandler = nullptr;
    QList<QAbstract3DInputHandler *> m_inputHandlers = {};

    friend class QAbstract3DGraph;
};

QT_END_NAMESPACE

#endif
