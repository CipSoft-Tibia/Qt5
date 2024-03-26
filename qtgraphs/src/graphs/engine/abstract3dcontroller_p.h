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

#ifndef ABSTRACT3DCONTROLLER_P_H
#define ABSTRACT3DCONTROLLER_P_H

#include <private/graphsglobal_p.h>

#include "qabstract3daxis.h"
#include "qabstract3dinputhandler.h"
#include "qabstract3dgraph.h"
#include "qcustom3ditem.h"
#include <QtGui/QLinearGradient>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLocale>
#include <QtCore/QMutex>

QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)

QT_BEGIN_NAMESPACE

class QValue3DAxis;
class QCategory3DAxis;
class AbstractDeclarative;
class QAbstract3DSeries;
class ThemeManager;

struct Abstract3DChangeBitField {
    bool themeChanged                  : 1;
    bool shadowQualityChanged          : 1;
    bool selectionModeChanged          : 1;
    bool optimizationHintChanged       : 1;
    bool axisXTypeChanged              : 1;
    bool axisYTypeChanged              : 1;
    bool axisZTypeChanged              : 1;
    bool axisXTitleChanged             : 1;
    bool axisYTitleChanged             : 1;
    bool axisZTitleChanged             : 1;
    bool axisXLabelsChanged            : 1;
    bool axisYLabelsChanged            : 1;
    bool axisZLabelsChanged            : 1;
    bool axisXRangeChanged             : 1;
    bool axisYRangeChanged             : 1;
    bool axisZRangeChanged             : 1;
    bool axisXSegmentCountChanged      : 1;
    bool axisYSegmentCountChanged      : 1;
    bool axisZSegmentCountChanged      : 1;
    bool axisXSubSegmentCountChanged   : 1;
    bool axisYSubSegmentCountChanged   : 1;
    bool axisZSubSegmentCountChanged   : 1;
    bool axisXLabelFormatChanged       : 1;
    bool axisYLabelFormatChanged       : 1;
    bool axisZLabelFormatChanged       : 1;
    bool axisXReversedChanged          : 1;
    bool axisYReversedChanged          : 1;
    bool axisZReversedChanged          : 1;
    bool axisXFormatterChanged         : 1;
    bool axisYFormatterChanged         : 1;
    bool axisZFormatterChanged         : 1;
    bool projectionChanged             : 1;
    bool axisXLabelAutoRotationChanged : 1;
    bool axisYLabelAutoRotationChanged : 1;
    bool axisZLabelAutoRotationChanged : 1;
    bool aspectRatioChanged            : 1;
    bool horizontalAspectRatioChanged  : 1;
    bool axisXTitleVisibilityChanged   : 1;
    bool axisYTitleVisibilityChanged   : 1;
    bool axisZTitleVisibilityChanged   : 1;
    bool axisXTitleFixedChanged        : 1;
    bool axisYTitleFixedChanged        : 1;
    bool axisZTitleFixedChanged        : 1;
    bool polarChanged                  : 1;
    bool radialLabelOffsetChanged      : 1;
    bool reflectionChanged             : 1;
    bool reflectivityChanged           : 1;
    bool marginChanged                 : 1;

    Abstract3DChangeBitField() :
        themeChanged(true),
        shadowQualityChanged(true),
        selectionModeChanged(true),
        optimizationHintChanged(true),
        axisXTypeChanged(true),
        axisYTypeChanged(true),
        axisZTypeChanged(true),
        axisXTitleChanged(true),
        axisYTitleChanged(true),
        axisZTitleChanged(true),
        axisXLabelsChanged(true),
        axisYLabelsChanged(true),
        axisZLabelsChanged(true),
        axisXRangeChanged(true),
        axisYRangeChanged(true),
        axisZRangeChanged(true),
        axisXSegmentCountChanged(true),
        axisYSegmentCountChanged(true),
        axisZSegmentCountChanged(true),
        axisXSubSegmentCountChanged(true),
        axisYSubSegmentCountChanged(true),
        axisZSubSegmentCountChanged(true),
        axisXLabelFormatChanged(true),
        axisYLabelFormatChanged(true),
        axisZLabelFormatChanged(true),
        axisXReversedChanged(true),
        axisYReversedChanged(true),
        axisZReversedChanged(true),
        axisXFormatterChanged(true),
        axisYFormatterChanged(true),
        axisZFormatterChanged(true),
        projectionChanged(true),
        axisXLabelAutoRotationChanged(true),
        axisYLabelAutoRotationChanged(true),
        axisZLabelAutoRotationChanged(true),
        aspectRatioChanged(true),
        horizontalAspectRatioChanged(true),
        axisXTitleVisibilityChanged(true),
        axisYTitleVisibilityChanged(true),
        axisZTitleVisibilityChanged(true),
        axisXTitleFixedChanged(true),
        axisYTitleFixedChanged(true),
        axisZTitleFixedChanged(true),
        polarChanged(true),
        radialLabelOffsetChanged(true),
        reflectionChanged(true),
        reflectivityChanged(true),
        marginChanged(true)
    {
    }
};

class Q_GRAPHS_EXPORT Abstract3DController : public QObject
{
    Q_OBJECT

public:
    enum SelectionType {
        SelectionNone = 0,
        SelectionItem,
        SelectionRow,
        SelectionColumn
    };

private:
    Abstract3DChangeBitField m_changeTracker;
    ThemeManager *m_themeManager;
    QAbstract3DGraph::SelectionFlags m_selectionMode;
    QAbstract3DGraph::ShadowQuality m_shadowQuality;
    bool m_useOrthoProjection;
    qreal m_aspectRatio;
    qreal m_horizontalAspectRatio;
    QAbstract3DGraph::OptimizationHints m_optimizationHints;
    bool m_reflectionEnabled;
    qreal m_reflectivity;
    QLocale m_locale;
    QVector3D m_queriedGraphPosition;
    bool m_graphPositionQueryPending = false;

protected:
    Q3DScene *m_scene;
    // Active axes
    QAbstract3DAxis *m_axisX;
    QAbstract3DAxis *m_axisY;
    QAbstract3DAxis *m_axisZ;

    QList<QAbstract3DAxis *> m_axes; // List of all added axes
    bool m_isDataDirty;
    bool m_isCustomDataDirty;
    bool m_isCustomItemDirty;
    bool m_isSeriesVisualsDirty;
    bool m_renderPending;
    bool m_isPolar;
    float m_radialLabelOffset;

    QList<QAbstract3DSeries *> m_seriesList;

    QList<QAbstract3DSeries *> m_changedSeriesList;

    QList<QCustom3DItem *> m_customItems;

    QAbstract3DGraph::ElementType m_clickedType;
    int m_selectedLabelIndex;
    int m_selectedCustomItemIndex;
    qreal m_margin;

    QMutex m_renderMutex;
    QQuickGraphsItem *m_qml = nullptr;

    explicit Abstract3DController(QRect initialViewport, Q3DScene *scene, QObject *parent = 0);

public:
    virtual ~Abstract3DController();

    virtual void addSeries(QAbstract3DSeries *series);
    virtual void insertSeries(int index, QAbstract3DSeries *series);
    virtual void removeSeries(QAbstract3DSeries *series);
    virtual bool hasSeries(QAbstract3DSeries *series);
    QList<QAbstract3DSeries *> seriesList();

    virtual void setAxisX(QAbstract3DAxis *axis);
    virtual QAbstract3DAxis *axisX() const;
    virtual void setAxisY(QAbstract3DAxis *axis);
    virtual QAbstract3DAxis *axisY() const;
    virtual void setAxisZ(QAbstract3DAxis *axis);
    virtual QAbstract3DAxis *axisZ() const;
    virtual void addAxis(QAbstract3DAxis *axis);
    virtual void releaseAxis(QAbstract3DAxis *axis);
    virtual QList<QAbstract3DAxis *> axes() const; // Omits default axes

    virtual void addTheme(Q3DTheme *theme);
    virtual void releaseTheme(Q3DTheme *theme);
    virtual void setActiveTheme(Q3DTheme *theme, bool force = true);
    virtual Q3DTheme *activeTheme() const;
    virtual QList<Q3DTheme *> themes() const;

    virtual void setSelectionMode(QAbstract3DGraph::SelectionFlags mode);
    virtual QAbstract3DGraph::SelectionFlags selectionMode() const;

    virtual void setShadowQuality(QAbstract3DGraph::ShadowQuality quality);
    virtual void doSetShadowQuality(QAbstract3DGraph::ShadowQuality quality);
    virtual QAbstract3DGraph::ShadowQuality shadowQuality() const;

    void setOptimizationHints(QAbstract3DGraph::OptimizationHints hints);
    QAbstract3DGraph::OptimizationHints optimizationHints() const;

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

    void markDataDirty();
    void markSeriesVisualsDirty();

    void requestRender(QOpenGLFramebufferObject *fbo);

    int addCustomItem(QCustom3DItem *item);
    void deleteCustomItems();
    void deleteCustomItem(QCustom3DItem *item);
    void deleteCustomItem(const QVector3D &position);
    void releaseCustomItem(QCustom3DItem *item);
    QList<QCustom3DItem *> customItems() const;

    int selectedLabelIndex() const;
    QAbstract3DAxis *selectedAxis() const;
    int selectedCustomItemIndex() const;
    QCustom3DItem *selectedCustomItem() const;

    void setOrthoProjection(bool enable);
    bool isOrthoProjection() const;

    QAbstract3DGraph::ElementType selectedElement() const;

    void setAspectRatio(qreal ratio);
    qreal aspectRatio();
    void setHorizontalAspectRatio(qreal ratio);
    qreal horizontalAspectRatio() const;

    void setReflection(bool enable);
    bool reflection() const;
    void setReflectivity(qreal reflectivity);
    qreal reflectivity() const;

    void setPolar(bool enable);
    bool isPolar() const;
    void setRadialLabelOffset(float offset);
    float radialLabelOffset() const;

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    QVector3D queriedGraphPosition() const;
    void setQueriedGraphPosition(const QVector3D &position) { m_queriedGraphPosition = position; }

    void setMargin(qreal margin);
    qreal margin() const;

    void emitNeedRender();

    virtual void clearSelection() = 0;

    virtual void handleAxisTitleChangedBySender(QObject *sender);
    virtual void handleAxisLabelsChangedBySender(QObject *sender);
    virtual void handleAxisRangeChangedBySender(QObject *sender);
    virtual void handleAxisSegmentCountChangedBySender(QObject *sender);
    virtual void handleAxisSubSegmentCountChangedBySender(QObject *sender);
    virtual void handleAxisAutoAdjustRangeChangedInOrientation(
            QAbstract3DAxis::AxisOrientation orientation, bool autoAdjust) = 0;
    virtual void handleAxisLabelFormatChangedBySender(QObject *sender);
    virtual void handleAxisReversedChangedBySender(QObject *sender);
    virtual void handleAxisFormatterDirtyBySender(QObject *sender);
    virtual void handleAxisLabelAutoRotationChangedBySender(QObject *sender);
    virtual void handleAxisTitleVisibilityChangedBySender(QObject *sender);
    virtual void handleAxisTitleFixedChangedBySender(QObject *sender);
    virtual void handleSeriesVisibilityChangedBySender(QObject *sender);
    virtual void adjustAxisRanges() = 0;

    void markSeriesItemLabelsDirty();

    bool graphPositionQueryPending() const { return m_graphPositionQueryPending; }
    void setGraphPositionQueryPending(const bool &pending) { m_graphPositionQueryPending = pending; }

public Q_SLOTS:
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

    // Renderer callback handlers
    void handleRequestShadowQuality(QAbstract3DGraph::ShadowQuality quality);

    void updateCustomItem();

Q_SIGNALS:
    void shadowQualityChanged(QAbstract3DGraph::ShadowQuality quality);
    void activeThemeChanged(Q3DTheme *activeTheme);
    void selectionModeChanged(QAbstract3DGraph::SelectionFlags mode);
    void needRender();
    void axisXChanged(QAbstract3DAxis *axis);
    void axisYChanged(QAbstract3DAxis *axis);
    void axisZChanged(QAbstract3DAxis *axis);
    void elementSelected(QAbstract3DGraph::ElementType type);
    void orthoProjectionChanged(bool enabled);
    void aspectRatioChanged(qreal ratio);
    void horizontalAspectRatioChanged(qreal ratio);
    void optimizationHintsChanged(QAbstract3DGraph::OptimizationHints hints);
    void polarChanged(bool enabled);
    void radialLabelOffsetChanged(float offset);
    void reflectionChanged(bool enabled);
    void reflectivityChanged(qreal reflectivity);
    void localeChanged(const QLocale &locale);
    void queriedGraphPositionChanged(const QVector3D &data);
    void marginChanged(qreal margin);
    void themeTypeChanged();

protected:
    virtual QAbstract3DAxis *createDefaultAxis(QAbstract3DAxis::AxisOrientation orientation);
    QValue3DAxis *createDefaultValueAxis();
    QCategory3DAxis *createDefaultCategoryAxis();
    virtual void startRecordingRemovesAndInserts();

private:
    void setAxisHelper(QAbstract3DAxis::AxisOrientation orientation, QAbstract3DAxis *axis,
                       QAbstract3DAxis **axisPtr);

    friend class AbstractDeclarative;
    friend class QQuickGraphsItem;
    friend class Bars3DController;
    friend class QAbstract3DGraphPrivate;
};

QT_END_NAMESPACE

#endif
