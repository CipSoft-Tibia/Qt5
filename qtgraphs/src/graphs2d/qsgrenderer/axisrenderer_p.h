// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef AXISRENDERER_H
#define AXISRENDERER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QQuickItem>
#include <QRectF>
#include <QList>
#include <QList>
#include <QtQuick/private/qquicktext_p.h>
#include <private/axisgrid_p.h>
#include <private/axisticker_p.h>
#include <private/axisline_p.h>

QT_BEGIN_NAMESPACE

class QAbstractAxis;
class QGraphsView;
class QBarCategoryAxis;
class QValueAxis;
class QGraphTheme;

class AxisRenderer : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    AxisRenderer(QQuickItem *parent = nullptr);

    void handlePolish();
    void updateAxis();
    void updateAxisTickers();
    void updateAxisTickersShadow();
    void updateAxisGrid();
    void updateAxisGridShadow();
    void updateBarXAxisLabels(QBarCategoryAxis *axis, const QRectF &rect);
    void updateValueYAxisLabels(QValueAxis *axis, const QRectF &rect);
    void updateValueXAxisLabels(QValueAxis *axis, const QRectF &rect);
    void initialize();

Q_SIGNALS:

private:
    friend class QGraphsView;
    friend class BarsRenderer;
    friend class LinesRenderer;
    friend class PointRenderer;

    double getValueStepsFromRange(double range);
    int getValueDecimalsFromRange(double range);

    QGraphsView *m_graph = nullptr;
    QGraphTheme *theme();
    bool m_initialized = false;

    QAbstractAxis *m_axisVertical = nullptr;
    QAbstractAxis *m_axisHorizontal = nullptr;
    QList<QQuickText *> m_xAxisTextItems;
    QList<QQuickText *> m_yAxisTextItems;
    AxisGrid *m_axisGrid = nullptr;
    AxisTicker *m_axisTickerVertical = nullptr;
    AxisTicker *m_axisTickerHorizontal = nullptr;
    AxisLine *m_axisLineVertical = nullptr;
    AxisLine *m_axisLineHorizontal = nullptr;

    // Shadow
    AxisGrid *m_axisGridShadow = nullptr;
    AxisTicker *m_axisTickerVerticalShadow = nullptr;
    AxisTicker *m_axisTickerHorizontalShadow = nullptr;
    AxisLine *m_axisLineVerticalShadow = nullptr;
    AxisLine *m_axisLineHorizontalShadow = nullptr;

    // Vertical axis
    // Max value
    double m_axisVerticalMaxValue = 20;
    // Min value
    double m_axisVerticalMinValue = 0;
    // Values range, so m_axisVerticalMaxValue - m_axisVerticalMinValue
    double m_axisVerticalValueRange = 0;
    // How much each major value step is
    double m_axisVerticalValueStep = 1.0;
    // px between major ticks
    double m_axisVerticalStepPx = 0;
    // Ticks movement, between -m_axisHorizontalStepPx .. m_axisHorizontalStepPx.
    double m_axisYMovement = 0;
    // The value of smallest label
    double m_axisVerticalMinLabel = 0;


    // Horizontal axis
    // Max value
    double m_axisHorizontalMaxValue = 20;
    // Min value
    double m_axisHorizontalMinValue = 0;
    // Values range, so m_axisHorizontalMaxValue - m_axisHorizontalMinValue
    double m_axisHorizontalValueRange = 0;
    // How much each major value step is
    double m_axisHorizontalValueStep = 1.0;
    // px between major ticks
    double m_axisHorizontalStepPx = 0;
    // Ticks movement, between -m_axisHorizontalStepPx .. m_axisHorizontalStepPx.
    double m_axisXMovement = 0;
    // The value of smallest label
    double m_axisHorizontalMinLabel = 0;

    double m_axisVerticalMinorTickScale = 0.5;
    double m_axisHorizontalMinorTickScale = 0.5;
    bool m_gridHorizontalMajorTicksVisible = true;
    bool m_gridVerticalMajorTicksVisible = true;
    bool m_gridHorizontalMinorTicksVisible = false;
    bool m_gridVerticalMinorTicksVisible = false;

    // Sizes required of axis labels
    // TODO: Should these come from QAbstactAxis?
    qreal m_axisWidth = 40;
    qreal m_axisHeight = 40;
    qreal m_axisTickersWidth = 15;
    qreal m_axisTickersHeight = 15;
};

QT_END_NAMESPACE

#endif // AXISRENDERER_H
