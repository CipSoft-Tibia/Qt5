// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACT3DGRAPH_H
#define QABSTRACT3DGRAPH_H

#if 0
#  pragma qt_class(QAbstract3DGraph)
#endif

#include <QtCore/qlocale.h>
#include <QtGraphs/q3dscene.h>
#include <QtGraphs/q3dtheme.h>
#include <QtGraphs/qabstract3dinputhandler.h>
#include <QtGraphs/qgraphsglobal.h>
#include <QtQuickWidgets/qquickwidget.h>
#include <QtQuick/qquickitemgrabresult.h>

QT_BEGIN_NAMESPACE

class QCustom3DItem;
class QAbstract3DAxis;
class QAbstract3DSeries;
class QQuickGraphsItem;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QAbstract3DGraph : public QQuickWidget
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(QAbstract3DInputHandler *activeInputHandler READ activeInputHandler WRITE
                   setActiveInputHandler NOTIFY activeInputHandlerChanged)
    Q_PROPERTY(Q3DTheme *activeTheme READ activeTheme WRITE setActiveTheme NOTIFY activeThemeChanged)
    Q_PROPERTY(QAbstract3DGraph::SelectionFlags selectionMode READ selectionMode WRITE
                   setSelectionMode NOTIFY selectionModeChanged)
    Q_PROPERTY(QAbstract3DGraph::ShadowQuality shadowQuality READ shadowQuality WRITE
                   setShadowQuality NOTIFY shadowQualityChanged)
    Q_PROPERTY(Q3DScene *scene READ scene CONSTANT)
    Q_PROPERTY(bool measureFps READ measureFps WRITE setMeasureFps NOTIFY measureFpsChanged)
    Q_PROPERTY(int currentFps READ currentFps NOTIFY currentFpsChanged)
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
    Q_PROPERTY(QAbstract3DGraph::CameraPreset cameraPreset READ cameraPreset WRITE setCameraPreset
                   NOTIFY cameraPresetChanged)
    Q_PROPERTY(float cameraXRotation READ cameraXRotation WRITE setCameraXRotation NOTIFY
                   cameraXRotationChanged)
    Q_PROPERTY(float cameraYRotation READ cameraYRotation WRITE setCameraYRotation NOTIFY
                   cameraYRotationChanged)
    Q_PROPERTY(float cameraZoomLevel READ cameraZoomLevel WRITE setCameraZoomLevel NOTIFY
                   cameraZoomLevelChanged)
    Q_PROPERTY(float minCameraZoomLevel READ minCameraZoomLevel WRITE setMinCameraZoomLevel NOTIFY
                   minCameraZoomLevelChanged)
    Q_PROPERTY(float maxCameraZoomLevel READ maxCameraZoomLevel WRITE setMaxCameraZoomLevel NOTIFY
                   maxCameraZoomLevelChanged)
    Q_PROPERTY(bool wrapCameraXRotation READ wrapCameraXRotation WRITE setWrapCameraXRotation NOTIFY
                   wrapCameraXRotationChanged)
    Q_PROPERTY(bool wrapCameraYRotation READ wrapCameraYRotation WRITE setWrapCameraYRotation NOTIFY
                   wrapCameraYRotationChanged)
    Q_PROPERTY(QVector3D cameraTargetPosition READ cameraTargetPosition WRITE
                   setCameraTargetPosition NOTIFY cameraTargetPositionChanged)
    Q_PROPERTY(int msaaSamples READ msaaSamples WRITE setMsaaSamples NOTIFY msaaSamplesChanged)

    QML_NAMED_ELEMENT(AbstractGraph3D)
    QML_UNCREATABLE("Trying to create uncreatable: AbstractGraph3D.")

public:
    enum SelectionFlag {
        SelectionNone = 0,
        SelectionItem = 1,
        SelectionRow = 2,
        SelectionItemAndRow = SelectionItem | SelectionRow,
        SelectionColumn = 4,
        SelectionItemAndColumn = SelectionItem | SelectionColumn,
        SelectionRowAndColumn = SelectionRow | SelectionColumn,
        SelectionItemRowAndColumn = SelectionItem | SelectionRow | SelectionColumn,
        SelectionSlice = 8,
        SelectionMultiSeries = 16
    };
    Q_FLAG(SelectionFlag)
    Q_DECLARE_FLAGS(SelectionFlags, SelectionFlag)

    enum class ShadowQuality { None, Low, Medium, High, SoftLow, SoftMedium, SoftHigh };
    Q_ENUM(ShadowQuality)

    enum class ElementType { None, Series, AxisXLabel, AxisYLabel, AxisZLabel, CustomItem };
    Q_ENUM(ElementType)

    enum class OptimizationHint { Default, Legacy };
    Q_ENUM(OptimizationHint)

    enum class RenderingMode { DirectToBackground, Indirect };
    Q_ENUM(RenderingMode)

    enum class CameraPreset {
        NoPreset,
        FrontLow,
        Front,
        FrontHigh,
        LeftLow,
        Left,
        LeftHigh,
        RightLow,
        Right,
        RightHigh,
        BehindLow,
        Behind,
        BehindHigh,
        IsometricLeft,
        IsometricLeftHigh,
        IsometricRight,
        IsometricRightHigh,
        DirectlyAbove,
        DirectlyAboveCW45,
        DirectlyAboveCCW45,
        FrontBelow,
        LeftBelow,
        RightBelow,
        BehindBelow,
        DirectlyBelow
    };
    Q_ENUM(CameraPreset)

    void addInputHandler(QAbstract3DInputHandler *inputHandler);
    void releaseInputHandler(QAbstract3DInputHandler *inputHandler);
    void setActiveInputHandler(QAbstract3DInputHandler *inputHandler);
    QAbstract3DInputHandler *activeInputHandler() const;
    QList<QAbstract3DInputHandler *> inputHandlers() const;

    void addTheme(Q3DTheme *theme);
    void releaseTheme(Q3DTheme *theme);
    Q3DTheme *activeTheme() const;
    void setActiveTheme(Q3DTheme *activeTheme);
    QList<Q3DTheme *> themes() const;

    QAbstract3DGraph::ShadowQuality shadowQuality() const;
    void setShadowQuality(const QAbstract3DGraph::ShadowQuality &shadowQuality);

    QAbstract3DGraph::SelectionFlags selectionMode() const;
    void setSelectionMode(const QAbstract3DGraph::SelectionFlags &selectionMode);

    Q3DScene *scene() const;

    void setMeasureFps(bool enable);
    bool measureFps() const;
    int currentFps() const;

    void setOrthoProjection(bool enable);
    bool isOrthoProjection() const;

    QAbstract3DGraph::ElementType selectedElement() const;

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

    void clearSelection();

    bool hasSeries(QAbstract3DSeries *series) const;

    int addCustomItem(QCustom3DItem *item);
    void removeCustomItems();
    void removeCustomItem(QCustom3DItem *item);
    void removeCustomItemAt(const QVector3D &position);
    void releaseCustomItem(QCustom3DItem *item);
    QList<QCustom3DItem *> customItems() const;

    int selectedLabelIndex() const;
    QAbstract3DAxis *selectedAxis() const;

    int selectedCustomItemIndex() const;
    QCustom3DItem *selectedCustomItem() const;

    QSharedPointer<QQuickItemGrabResult> renderToImage(const QSize &imageSize = QSize());

    QAbstract3DGraph::CameraPreset cameraPreset() const;
    void setCameraPreset(QAbstract3DGraph::CameraPreset preset);

    float cameraXRotation();
    void setCameraXRotation(float rotation);
    float cameraYRotation();
    void setCameraYRotation(float rotation);

    float minCameraXRotation();
    void setMinCameraXRotation(float rotation);
    float maxCameraXRotation();
    void setMaxCameraXRotation(float rotation);

    float minCameraYRotation();
    void setMinCameraYRotation(float rotation);
    float maxCameraYRotation();
    void setMaxCameraYRotation(float rotation);

    float cameraZoomLevel();
    void setCameraZoomLevel(float level);

    float minCameraZoomLevel();
    void setMinCameraZoomLevel(float level);

    float maxCameraZoomLevel();
    void setMaxCameraZoomLevel(float level);

    QVector3D cameraTargetPosition();
    void setCameraTargetPosition(const QVector3D &target);

    bool wrapCameraXRotation();
    void setWrapCameraXRotation(bool wrap);

    bool wrapCameraYRotation();
    void setWrapCameraYRotation(bool wrap);

    void setCameraPosition(float horizontal, float vertical, float zoom = 100.0f);

    int msaaSamples() const;
    void setMsaaSamples(int samples);

    ~QAbstract3DGraph() override;

protected:
    QAbstract3DGraph(const QString &graphType);

    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif

Q_SIGNALS:
    void activeInputHandlerChanged(QAbstract3DInputHandler *inputHandler);
    void activeThemeChanged(Q3DTheme *activeTheme);
    void shadowQualityChanged(QAbstract3DGraph::ShadowQuality quality);
    void selectionModeChanged(const QAbstract3DGraph::SelectionFlags selectionMode);
    void selectedElementChanged(QAbstract3DGraph::ElementType type);
    void measureFpsChanged(bool enabled);
    void currentFpsChanged(int fps);
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
    void msaaSamplesChanged(int samples);

private:
    Q_DISABLE_COPY(QAbstract3DGraph)
    QScopedPointer<QQuickGraphsItem> m_graphsItem;

    friend class Q3DBars;
    friend class Q3DScatter;
    friend class Q3DSurface;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstract3DGraph::SelectionFlags)

QT_END_NAMESPACE

#endif
