// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACT3DGRAPH_H
#define QABSTRACT3DGRAPH_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtGraphs/q3dtheme.h>
#include <QtGraphs/q3dscene.h>
#include <QtGraphs/qabstract3dinputhandler.h>
#include <QtCore/qlocale.h>
#include <QtQuickWidgets/qquickwidget.h>

QT_BEGIN_NAMESPACE

class QCustom3DItem;
class QAbstract3DAxis;
class QAbstract3DSeries;
class QQuickGraphsItem;

class Q_GRAPHS_EXPORT QAbstract3DGraph : public QQuickWidget
{
    Q_OBJECT
    Q_FLAGS(SelectionFlag SelectionFlags)
    Q_FLAGS(OptimizationHint OptimizationHints)
    Q_PROPERTY(QAbstract3DInputHandler* activeInputHandler READ activeInputHandler WRITE setActiveInputHandler NOTIFY activeInputHandlerChanged)
    Q_PROPERTY(Q3DTheme* activeTheme READ activeTheme WRITE setActiveTheme NOTIFY activeThemeChanged)
    Q_PROPERTY(QAbstract3DGraph::SelectionFlags selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged)
    Q_PROPERTY(QAbstract3DGraph::ShadowQuality shadowQuality READ shadowQuality WRITE setShadowQuality NOTIFY shadowQualityChanged)
    Q_PROPERTY(Q3DScene* scene READ scene CONSTANT)
    Q_PROPERTY(bool measureFps READ measureFps WRITE setMeasureFps NOTIFY measureFpsChanged)
    Q_PROPERTY(int currentFps READ currentFps NOTIFY currentFpsChanged)
    Q_PROPERTY(bool orthoProjection READ isOrthoProjection WRITE setOrthoProjection NOTIFY orthoProjectionChanged)
    Q_PROPERTY(QAbstract3DGraph::ElementType selectedElement READ selectedElement NOTIFY selectedElementChanged)
    Q_PROPERTY(qreal aspectRatio READ aspectRatio WRITE setAspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(QAbstract3DGraph::OptimizationHints optimizationHints READ optimizationHints WRITE setOptimizationHints NOTIFY optimizationHintsChanged)
    Q_PROPERTY(bool polar READ isPolar WRITE setPolar NOTIFY polarChanged)
    Q_PROPERTY(float radialLabelOffset READ radialLabelOffset WRITE setRadialLabelOffset NOTIFY radialLabelOffsetChanged)
    Q_PROPERTY(qreal horizontalAspectRatio READ horizontalAspectRatio WRITE setHorizontalAspectRatio NOTIFY horizontalAspectRatioChanged)
    Q_PROPERTY(bool reflection READ isReflection WRITE setReflection NOTIFY reflectionChanged)
    Q_PROPERTY(qreal reflectivity READ reflectivity WRITE setReflectivity NOTIFY reflectivityChanged)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(QVector3D queriedGraphPosition READ queriedGraphPosition NOTIFY queriedGraphPositionChanged)
    Q_PROPERTY(qreal margin READ margin WRITE setMargin NOTIFY marginChanged)

    QML_NAMED_ELEMENT(AbstractGraph3D)
    QML_UNCREATABLE("Trying to create uncreatable: AbstractGraph3D.")

public:
     enum SelectionFlag {
        SelectionNone              = 0,
        SelectionItem              = 1,
        SelectionRow               = 2,
        SelectionItemAndRow        = SelectionItem | SelectionRow,
        SelectionColumn            = 4,
        SelectionItemAndColumn     = SelectionItem | SelectionColumn,
        SelectionRowAndColumn      = SelectionRow | SelectionColumn,
        SelectionItemRowAndColumn  = SelectionItem | SelectionRow | SelectionColumn,
        SelectionSlice             = 8,
        SelectionMultiSeries       = 16
    };
    Q_ENUM(SelectionFlag)
    Q_DECLARE_FLAGS(SelectionFlags, SelectionFlag)

    enum ShadowQuality {
        ShadowQualityNone = 0,
        ShadowQualityLow,
        ShadowQualityMedium,
        ShadowQualityHigh,
        ShadowQualitySoftLow,
        ShadowQualitySoftMedium,
        ShadowQualitySoftHigh
    };
    Q_ENUM(ShadowQuality)

    enum ElementType {
        ElementNone = 0,
        ElementSeries,
        ElementAxisXLabel,
        ElementAxisYLabel,
        ElementAxisZLabel,
        ElementCustomItem
    };
    Q_ENUM(ElementType)

    enum OptimizationHint {
        OptimizationDefault = 0,
        OptimizationStatic  = 1,
        OptimizationLegacy
    };
    Q_ENUM(OptimizationHint)
    Q_DECLARE_FLAGS(OptimizationHints, OptimizationHint)

    enum RenderingMode {
        RenderDirectToBackground = 0,
        RenderIndirect
    };
    Q_ENUM(RenderingMode)

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

    void setOptimizationHints(QAbstract3DGraph::OptimizationHints hints);
    QAbstract3DGraph::OptimizationHints optimizationHints() const;

    void setPolar(bool enable);
    bool isPolar() const;

    void setRadialLabelOffset(float offset);
    float radialLabelOffset() const;

    void setHorizontalAspectRatio(qreal ratio);
    qreal horizontalAspectRatio() const;

    void setReflection(bool enable);
    bool isReflection() const;

    void setReflectivity(qreal reflectivity);
    qreal reflectivity() const;

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

    QImage renderToImage(int msaaSamples = 0, const QSize &imageSize = QSize());

    virtual ~QAbstract3DGraph();

protected:
    QAbstract3DGraph();

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
    void optimizationHintsChanged(QAbstract3DGraph::OptimizationHints hints);
    void polarChanged(bool enabled);
    void radialLabelOffsetChanged(float offset);
    void horizontalAspectRatioChanged(qreal ratio);
    void reflectionChanged(bool enabled);
    void reflectivityChanged(qreal reflectivity);
    void localeChanged(const QLocale &locale);
    void queriedGraphPositionChanged(const QVector3D &data);
    void marginChanged(qreal margin);
private:
    Q_DISABLE_COPY(QAbstract3DGraph)
    QScopedPointer<QQuickGraphsItem> m_graphsItem;

    friend class Q3DBars;
    friend class Q3DScatter;
    friend class Q3DSurface;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstract3DGraph::SelectionFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstract3DGraph::OptimizationHints)

QT_END_NAMESPACE

#endif
