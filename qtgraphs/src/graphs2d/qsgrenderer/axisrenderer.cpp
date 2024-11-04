// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qbarcategoryaxis.h>
#include <QtGraphs/qgraphtheme.h>
#include <private/axisrenderer_p.h>
#include <private/qabstractaxis_p.h>
#include <private/qgraphsview_p.h>
#include <private/qvalueaxis_p.h>

QT_BEGIN_NAMESPACE

AxisRenderer::AxisRenderer(QQuickItem *parent) :
      QQuickItem(parent)
{
    m_graph = qobject_cast<QGraphsView*>(parent);
    setFlag(QQuickItem::ItemHasContents);
}

QGraphTheme *AxisRenderer::theme() {
    return m_graph->m_theme;
}

void AxisRenderer::initialize() {
    if (m_initialized)
        return;

    if (m_axisGrid)
        m_axisGrid->componentComplete();
    if (m_axisLineVertical)
        m_axisLineVertical->componentComplete();
    if (m_axisTickerVertical)
        m_axisTickerVertical->componentComplete();
    if (m_axisLineHorizontal)
        m_axisLineHorizontal->componentComplete();
    if (m_axisTickerHorizontal)
        m_axisTickerHorizontal->componentComplete();
    if (m_axisGridShadow)
        m_axisGridShadow->componentComplete();
    if (m_axisLineVerticalShadow)
        m_axisLineVerticalShadow->componentComplete();
    if (m_axisTickerVerticalShadow)
        m_axisTickerVerticalShadow->componentComplete();
    if (m_axisLineHorizontalShadow)
        m_axisLineHorizontalShadow->componentComplete();
    if (m_axisTickerHorizontalShadow)
        m_axisTickerHorizontalShadow->componentComplete();
    m_initialized = true;
}

void AxisRenderer::handlePolish()
{
    if (!m_axisGrid) {
        m_axisGrid = new AxisGrid();
        m_axisGrid->setParentItem(this);
        m_axisGrid->setZ(-1);
        m_axisGrid->setupShaders();
        m_axisGrid->setOrigo(0);
    }
    if (!m_axisLineVertical) {
        m_axisLineVertical = new AxisLine();
        m_axisLineVertical->setParentItem(this);
        m_axisLineVertical->setZ(-1);
        m_axisLineVertical->setupShaders();
    }
    if (!m_axisTickerVertical) {
        m_axisTickerVertical = new AxisTicker();
        m_axisTickerVertical->setParentItem(this);
        m_axisTickerVertical->setZ(-2);
        m_axisTickerVertical->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerVertical->setMinorBarsLength(0.5);
        m_axisTickerVertical->setupShaders();
    }
    if (!m_axisLineHorizontal) {
        m_axisLineHorizontal = new AxisLine();
        m_axisLineHorizontal->setParentItem(this);
        m_axisLineHorizontal->setZ(-1);
        m_axisLineHorizontal->setIsHorizontal(true);
        m_axisLineHorizontal->setupShaders();
    }
    if (!m_axisTickerHorizontal) {
        m_axisTickerHorizontal = new AxisTicker();
        m_axisTickerHorizontal->setParentItem(this);
        m_axisTickerHorizontal->setZ(-2);
        m_axisTickerHorizontal->setIsHorizontal(true);
        m_axisTickerHorizontal->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerHorizontal->setMinorBarsLength(0.2);
        m_axisTickerHorizontal->setupShaders();
    }

    // TODO: Create shadows only when needed
    if (!m_axisGridShadow) {
        m_axisGridShadow = new AxisGrid();
        m_axisGridShadow->setParentItem(this);
        m_axisGridShadow->setZ(-3);
        m_axisGridShadow->setupShaders();
        m_axisGridShadow->setOrigo(0);
    }
    if (!m_axisLineVerticalShadow) {
        m_axisLineVerticalShadow = new AxisLine();
        m_axisLineVerticalShadow->setParentItem(this);
        m_axisLineVerticalShadow->setZ(-3);
        m_axisLineVerticalShadow->setupShaders();
    }
    if (!m_axisTickerVerticalShadow) {
        m_axisTickerVerticalShadow = new AxisTicker();
        m_axisTickerVerticalShadow->setParentItem(this);
        m_axisTickerVerticalShadow->setZ(-3);
        m_axisTickerVerticalShadow->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerVerticalShadow->setMinorBarsLength(m_axisTickerVertical->minorBarsLength());
        m_axisTickerVerticalShadow->setupShaders();
    }
    if (!m_axisLineHorizontalShadow) {
        m_axisLineHorizontalShadow = new AxisLine();
        m_axisLineHorizontalShadow->setParentItem(this);
        m_axisLineHorizontalShadow->setZ(-3);
        m_axisLineHorizontalShadow->setupShaders();
    }
    if (!m_axisTickerHorizontalShadow) {
        m_axisTickerHorizontalShadow = new AxisTicker();
        m_axisTickerHorizontalShadow->setParentItem(this);
        m_axisTickerHorizontalShadow->setZ(-3);
        m_axisTickerHorizontalShadow->setIsHorizontal(true);
        m_axisTickerHorizontalShadow->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerHorizontalShadow->setMinorBarsLength(m_axisTickerHorizontal->minorBarsLength());
        m_axisTickerHorizontalShadow->setupShaders();
    }

    updateAxis();
}

void AxisRenderer::updateAxis()
{
    // Update active axis
    QAbstractAxis *axisVertical = nullptr;
    QAbstractAxis *axisHorizontal = nullptr;
    for (auto a : m_graph->m_axis) {
        if (a->orientation() == Qt::Vertical)
            axisVertical = a;
        else
            axisHorizontal = a;
    }
    m_axisVertical = axisVertical;
    m_axisHorizontal = axisHorizontal;

    // Graph series area width & height
    QRectF seriesRect = m_graph->seriesRect();
    float w = seriesRect.width();
    float h = seriesRect.height();

    if (m_axisVertical) {
        m_gridVerticalMajorTicksVisible = m_axisVertical->isGridLineVisible();
        m_gridVerticalMinorTicksVisible = m_axisVertical->isMinorGridLineVisible();
    }
    if (m_axisHorizontal) {
        m_gridHorizontalMajorTicksVisible = m_axisHorizontal->isGridLineVisible();
        m_gridHorizontalMinorTicksVisible = m_axisHorizontal->isMinorGridLineVisible();
    }

    if (auto vaxis = qobject_cast<QValueAxis *>(m_axisVertical)) {
        m_axisVerticalMaxValue = vaxis->max();
        m_axisVerticalMinValue = vaxis->min();
        double step = vaxis->tickInterval();

        m_axisVerticalValueRange = m_axisVerticalMaxValue - m_axisVerticalMinValue;
        // If step is not manually defined (or it is invalid), calculate autostep
        if (step <= 0)
            step = getValueStepsFromRange(m_axisVerticalValueRange);

        // Get smallest tick label value
        double minLabel = vaxis->tickAnchor();
        while (minLabel < m_axisVerticalMinValue)
            minLabel += step;
        while (minLabel >= (m_axisVerticalMinValue + step))
            minLabel -= step;
        m_axisVerticalMinLabel = minLabel;

        m_axisVerticalValueStep = step;
        int axisVerticalMinorTickCount = vaxis->minorTickCount();
        m_axisVerticalMinorTickScale = axisVerticalMinorTickCount > 0 ? 1.0 / (axisVerticalMinorTickCount + 1) : 1.0;

        m_axisVerticalStepPx = (height() - m_graph->m_marginTop - m_graph->m_marginBottom - m_axisHeight) / (m_axisVerticalValueRange / m_axisVerticalValueStep);
        double axisVerticalValueDiff = m_axisVerticalMinLabel - m_axisVerticalMinValue;
        m_axisYMovement = -(axisVerticalValueDiff / m_axisVerticalValueStep) * m_axisVerticalStepPx;

        // Update value labels
        float rightMargin = 20;
        QRectF yAxisRect(m_graph->m_marginLeft, m_graph->m_marginTop, m_axisWidth - rightMargin, h);
        updateValueYAxisLabels(vaxis, yAxisRect);
    }

    if (auto haxis = qobject_cast<QValueAxis *>(m_axisHorizontal)) {
        m_axisHorizontalMaxValue = haxis->max();
        m_axisHorizontalMinValue = haxis->min();
        double step = haxis->tickInterval();

        m_axisHorizontalValueRange = m_axisHorizontalMaxValue - m_axisHorizontalMinValue;
        // If step is not manually defined (or it is invalid), calculate autostep
        if (step <= 0)
            step = getValueStepsFromRange(m_axisHorizontalValueRange);

        // Get smallest tick label value
        double minLabel = haxis->tickAnchor();
        while (minLabel < m_axisHorizontalMinValue)
            minLabel += step;
        while (minLabel >= (m_axisHorizontalMinValue + step))
            minLabel -= step;
        m_axisHorizontalMinLabel = minLabel;

        m_axisHorizontalValueStep = step;
        int axisHorizontalMinorTickCount = haxis->minorTickCount();
        m_axisHorizontalMinorTickScale = axisHorizontalMinorTickCount > 0 ?
                1.0 / (axisHorizontalMinorTickCount + 1) : 1.0;
        m_axisHorizontalStepPx = (width() - m_graph->m_marginLeft - m_graph->m_marginRight - m_axisWidth)
                / (m_axisHorizontalValueRange / m_axisHorizontalValueStep);
        double axisHorizontalValueDiff = m_axisHorizontalMinLabel - m_axisHorizontalMinValue;
        m_axisXMovement = -(axisHorizontalValueDiff / m_axisHorizontalValueStep) * m_axisHorizontalStepPx;

        // Update value labels
        float topMargin = 20;
        QRectF xAxisRect(m_graph->m_marginLeft + m_axisWidth,
                         m_graph->m_marginTop + h - m_graph->m_marginBottom + topMargin,
                         w,
                         m_axisHeight);
        updateValueXAxisLabels(haxis, xAxisRect);
    }

    if (auto haxis = qobject_cast<QBarCategoryAxis *>(m_axisHorizontal)) {
        m_axisHorizontalMaxValue = haxis->categories().size();
        m_axisHorizontalMinValue = 0;
        m_axisHorizontalValueRange = m_axisHorizontalMaxValue - m_axisHorizontalMinValue;
        QRectF xAxisRect(m_graph->m_marginLeft + m_axisWidth, m_graph->m_marginTop + h, w, m_axisHeight);
        updateBarXAxisLabels(haxis, xAxisRect);
    }

    updateAxisTickers();
    updateAxisTickersShadow();
    updateAxisGrid();
    updateAxisGridShadow();
}

void AxisRenderer::updateAxisTickers()
{
    if (m_axisVertical) {
        // Note: Fix before enabling, see QTBUG-121207 and QTBUG-121211
        //if (theme()->themeDirty()) {
            m_axisTickerVertical->setMinorColor(theme()->axisYMinorColor());
            m_axisTickerVertical->setMajorColor(theme()->axisYMajorColor());
            m_axisTickerVertical->setMajorBarWidth(theme()->axisYMajorBarWidth());
            m_axisTickerVertical->setMinorBarWidth(theme()->axisYMinorBarWidth());
            m_axisTickerVertical->setSmoothing(theme()->axisYSmoothing());
        //}
        float topPadding = m_axisGrid->majorBarWidth() * 0.5;
        float bottomPadding = topPadding;
        // TODO Only when changed
        m_axisTickerVertical->setBarsMovement(m_axisYMovement);
        m_axisTickerVertical->setX(m_axisWidth + m_graph->m_marginLeft - m_axisTickersWidth);
        m_axisTickerVertical->setY(m_graph->m_marginTop - topPadding);
        m_axisTickerVertical->setWidth(m_axisTickersWidth);
        m_axisTickerVertical->setHeight(height() - m_graph->m_marginTop - m_graph->m_marginBottom
                                        - m_axisHeight + topPadding + bottomPadding);
        m_axisTickerVertical->setSpacing((m_axisTickerVertical->height() - topPadding - bottomPadding)
                                         / (m_axisVerticalValueRange / m_axisVerticalValueStep));
        m_axisTickerVertical->setMinorBarsVisible(!qFuzzyCompare(m_axisVerticalMinorTickScale, 1.0));
        m_axisTickerVertical->setMinorTickScale(m_axisVerticalMinorTickScale);
        m_axisTickerVertical->setVisible(m_axisVertical->isVisible());
        // Axis line
        m_axisLineVertical->setColor(theme()->axisYMajorColor());
        m_axisLineVertical->setLineWidth(theme()->axisYMajorBarWidth());
        m_axisLineVertical->setSmoothing(theme()->axisYSmoothing());

        float xMovement = 0.5 * (m_axisLineVertical->lineWidth() + m_axisLineVertical->smoothing());
        m_axisLineVertical->setX(m_axisTickerVertical->x() + m_axisTickersWidth - xMovement);
        m_axisLineVertical->setY(m_axisTickerVertical->y());
        m_axisLineVertical->setWidth(m_axisLineVertical->lineWidth() + m_axisLineVertical->smoothing());
        m_axisLineVertical->setHeight(m_axisTickerVertical->height());
        m_axisLineVertical->setVisible(m_axisVertical->isLineVisible());
    } else {
        // Hide all parts of vertical axis
        m_axisTickerVertical->setVisible(false);
        m_axisLineVertical->setVisible(false);
        for (auto &textItem : m_yAxisTextItems)
            textItem->setVisible(false);
    }

    if (m_axisHorizontal) {
        if (theme()->themeDirty()) {
            m_axisTickerHorizontal->setMinorColor(theme()->axisXMinorColor());
            m_axisTickerHorizontal->setMajorColor(theme()->axisXMajorColor());
            m_axisTickerHorizontal->setMajorBarWidth(theme()->axisXMajorBarWidth());
            m_axisTickerHorizontal->setMinorBarWidth(theme()->axisXMinorBarWidth());
            m_axisTickerHorizontal->setSmoothing(theme()->axisXSmoothing());
        }
        float leftPadding = m_axisGrid->majorBarWidth() * 0.5;
        float rightPadding = leftPadding;
        // TODO Only when changed
        m_axisTickerHorizontal->setBarsMovement(m_axisXMovement);
        m_axisTickerHorizontal->setX(m_axisWidth + m_graph->m_marginLeft - leftPadding);
        m_axisTickerHorizontal->setY(height() - m_graph->m_marginBottom - m_axisHeight);
        m_axisTickerHorizontal->setWidth(width() - m_graph->m_marginLeft - m_graph->m_marginRight
                                         - m_axisWidth + leftPadding + rightPadding);
        m_axisTickerHorizontal->setHeight(m_axisTickersHeight);
        m_axisTickerHorizontal->setSpacing((m_axisTickerHorizontal->width() - leftPadding - rightPadding)
                                         / (m_axisHorizontalValueRange / m_axisHorizontalValueStep));
        m_axisTickerHorizontal->setMinorBarsVisible(!qFuzzyCompare(m_axisHorizontalMinorTickScale, 1.0));
        m_axisTickerHorizontal->setMinorTickScale(m_axisHorizontalMinorTickScale);
        m_axisTickerHorizontal->setVisible(m_axisHorizontal->isVisible());
        // Axis line
        m_axisLineHorizontal->setColor(theme()->axisXMajorColor());
        m_axisLineHorizontal->setLineWidth(theme()->axisXMajorBarWidth());
        m_axisLineHorizontal->setSmoothing(theme()->axisXSmoothing());
        m_axisLineHorizontal->setX(m_axisTickerHorizontal->x());
        float yMovement = 0.5 * (m_axisLineHorizontal->lineWidth() + m_axisLineHorizontal->smoothing());
        m_axisLineHorizontal->setY(m_axisTickerHorizontal->y() - yMovement);
        m_axisLineHorizontal->setWidth(m_axisTickerHorizontal->width());
        m_axisLineHorizontal->setHeight(m_axisLineHorizontal->lineWidth() + m_axisLineHorizontal->smoothing());
        m_axisLineHorizontal->setVisible(m_axisHorizontal->isLineVisible());
    } else {
        // Hide all parts of horizontal axis
        m_axisTickerHorizontal->setVisible(false);
        m_axisLineHorizontal->setVisible(false);
        for (auto &textItem : m_xAxisTextItems)
            textItem->setVisible(false);
    }
}

void AxisRenderer::updateAxisTickersShadow()
{
    if (m_axisVertical && theme()->shadowEnabled()) {
        m_axisTickerVerticalShadow->setMinorColor(theme()->shadowColor());
        m_axisTickerVerticalShadow->setMajorColor(theme()->shadowColor());
        m_axisTickerVerticalShadow->setMinorBarWidth(m_axisTickerVertical->minorBarWidth() + theme()->shadowBarWidth());
        m_axisTickerVerticalShadow->setMajorBarWidth(m_axisTickerVertical->majorBarWidth() + theme()->shadowBarWidth());
        m_axisTickerVerticalShadow->setSmoothing(m_axisTickerVertical->smoothing() + theme()->shadowSmoothing());

        // TODO Only when changed
        m_axisTickerVerticalShadow->setBarsMovement(m_axisTickerVertical->barsMovement());
        m_axisTickerVerticalShadow->setX(m_axisTickerVertical->x() + theme()->shadowXOffset());
        m_axisTickerVerticalShadow->setY(m_axisTickerVertical->y() + theme()->shadowYOffset());
        m_axisTickerVerticalShadow->setWidth(m_axisTickerVertical->width());
        m_axisTickerVerticalShadow->setHeight(m_axisTickerVertical->height());
        m_axisTickerVerticalShadow->setSpacing(m_axisTickerVertical->spacing());
        m_axisTickerVerticalShadow->setMinorBarsVisible(m_axisTickerVertical->minorBarsVisible());
        m_axisTickerVerticalShadow->setMinorTickScale(m_axisTickerVertical->minorTickScale());
        m_axisTickerVerticalShadow->setVisible(m_axisTickerVertical->isVisible());
        // Axis line
        m_axisLineVerticalShadow->setColor(theme()->shadowColor());
        m_axisLineVerticalShadow->setLineWidth(m_axisLineVertical->lineWidth() + theme()->shadowBarWidth());
        m_axisLineVerticalShadow->setSmoothing(m_axisLineVertical->smoothing() + theme()->shadowSmoothing());
        m_axisLineVerticalShadow->setX(m_axisLineVertical->x() + theme()->shadowXOffset());
        m_axisLineVerticalShadow->setY(m_axisLineVertical->y() + theme()->shadowYOffset());
        m_axisLineVerticalShadow->setWidth(m_axisLineVertical->width());
        m_axisLineVerticalShadow->setHeight(m_axisLineVertical->height());
        m_axisLineVerticalShadow->setVisible(m_axisLineVertical->isVisible());
    } else {
        // Hide all parts of vertical axis
        m_axisTickerVerticalShadow->setVisible(false);
        m_axisLineVerticalShadow->setVisible(false);
    }

    if (m_axisHorizontal && theme()->shadowEnabled()) {
        m_axisTickerHorizontalShadow->setMinorColor(theme()->shadowColor());
        m_axisTickerHorizontalShadow->setMajorColor(theme()->shadowColor());
        m_axisTickerHorizontalShadow->setMinorBarWidth(m_axisTickerHorizontal->minorBarWidth() + theme()->shadowBarWidth());
        m_axisTickerHorizontalShadow->setMajorBarWidth(m_axisTickerHorizontal->majorBarWidth() + theme()->shadowBarWidth());
        m_axisTickerHorizontalShadow->setSmoothing(m_axisTickerHorizontal->smoothing() + theme()->shadowSmoothing());

        // TODO Only when changed
        m_axisTickerHorizontalShadow->setBarsMovement(m_axisTickerHorizontal->barsMovement());
        m_axisTickerHorizontalShadow->setX(m_axisTickerHorizontal->x() + theme()->shadowXOffset());
        m_axisTickerHorizontalShadow->setY(m_axisTickerHorizontal->y() + theme()->shadowYOffset());
        m_axisTickerHorizontalShadow->setWidth(m_axisTickerHorizontal->width());
        m_axisTickerHorizontalShadow->setHeight(m_axisTickerHorizontal->height());
        m_axisTickerHorizontalShadow->setSpacing(m_axisTickerHorizontal->spacing());
        m_axisTickerHorizontalShadow->setMinorBarsVisible(m_axisTickerHorizontal->minorBarsVisible());
        m_axisTickerHorizontalShadow->setMinorTickScale(m_axisTickerHorizontal->minorTickScale());
        m_axisTickerHorizontalShadow->setVisible(m_axisTickerHorizontal->isVisible());
        // Axis line
        m_axisLineHorizontalShadow->setColor(theme()->shadowColor());
        m_axisLineHorizontalShadow->setLineWidth(m_axisLineHorizontal->width() + theme()->shadowBarWidth());
        m_axisLineHorizontalShadow->setSmoothing(m_axisLineHorizontal->smoothing() + theme()->shadowSmoothing());
        m_axisLineHorizontalShadow->setX(m_axisLineHorizontal->x() + theme()->shadowXOffset());
        m_axisLineHorizontalShadow->setY(m_axisLineHorizontal->y() + theme()->shadowYOffset());
        m_axisLineHorizontalShadow->setWidth(m_axisLineHorizontal->width());
        m_axisLineHorizontalShadow->setHeight(m_axisLineHorizontal->height());
        m_axisLineHorizontalShadow->setVisible(m_axisLineHorizontal->isVisible());
    } else {
        // Hide all parts of horizontal axis
        m_axisTickerHorizontalShadow->setVisible(false);
        m_axisLineHorizontalShadow->setVisible(false);
    }
}

void AxisRenderer::updateAxisGrid()
{
    //if (theme()->themeDirty()) {
        m_axisGrid->setMajorColor(theme()->gridMajorBarsColor());
        m_axisGrid->setMinorColor(theme()->gridMinorBarsColor());
        m_axisGrid->setMinorBarWidth(theme()->gridMinorBarsWidth());
        m_axisGrid->setMajorBarWidth(theme()->gridMajorBarsWidth());
        const double minimumSmoothing = 0.05;
        m_axisGrid->setSmoothing(theme()->gridSmoothing() + minimumSmoothing);
    //}
    float topPadding = m_axisGrid->majorBarWidth() * 0.5;
    float bottomPadding = topPadding;
    float leftPadding = topPadding;
    float rightPadding = topPadding;
    // TODO Only when changed
    m_axisGrid->setGridMovement(QPointF(m_axisXMovement, m_axisYMovement));
    m_axisGrid->setX(m_axisWidth + m_graph->m_marginLeft - leftPadding);
    m_axisGrid->setY(m_graph->m_marginTop - topPadding);
    m_axisGrid->setWidth(width() - m_graph->m_marginLeft
                         - m_graph->m_marginRight - m_axisWidth
                         + leftPadding + rightPadding);
    m_axisGrid->setHeight(height() - m_graph->m_marginTop
                          - m_graph->m_marginBottom - m_axisHeight
                          + topPadding + bottomPadding);
    m_axisGrid->setGridWidth((m_axisGrid->width() - leftPadding - rightPadding)
                             / (m_axisHorizontalValueRange / m_axisHorizontalValueStep));
    m_axisGrid->setGridHeight((m_axisGrid->height() - topPadding - bottomPadding)
                              / (m_axisVerticalValueRange / m_axisVerticalValueStep));
    m_axisGrid->setBarsVisibility(QVector4D(m_gridHorizontalMajorTicksVisible,
                                            m_gridVerticalMajorTicksVisible,
                                            m_gridHorizontalMinorTicksVisible,
                                            m_gridVerticalMinorTicksVisible));
    m_axisGrid->setVerticalMinorTickScale(m_axisVerticalMinorTickScale);
    m_axisGrid->setHorizontalMinorTickScale(m_axisHorizontalMinorTickScale);
}

void AxisRenderer::updateAxisGridShadow()
{
    if (theme()->shadowEnabled()) {
        m_axisGridShadow->setMajorColor(theme()->shadowColor());
        m_axisGridShadow->setMinorColor(theme()->shadowColor());
        m_axisGridShadow->setMinorBarWidth(m_axisGrid->minorBarWidth() + theme()->shadowBarWidth());
        m_axisGridShadow->setMajorBarWidth(m_axisGrid->majorBarWidth() + theme()->shadowBarWidth());
        m_axisGridShadow->setSmoothing(m_axisGrid->smoothing() + theme()->shadowSmoothing());

        // TODO Only when changed
        m_axisGridShadow->setGridMovement(m_axisGrid->gridMovement());
        m_axisGridShadow->setX(m_axisGrid->x() + theme()->shadowXOffset());
        m_axisGridShadow->setY(m_axisGrid->y() + theme()->shadowYOffset());
        m_axisGridShadow->setWidth(m_axisGrid->width());
        m_axisGridShadow->setHeight(m_axisGrid->height());
        m_axisGridShadow->setGridWidth(m_axisGrid->gridWidth());
        m_axisGridShadow->setGridHeight(m_axisGrid->gridHeight());
        m_axisGridShadow->setBarsVisibility(m_axisGrid->barsVisibility());
        m_axisGridShadow->setVerticalMinorTickScale(m_axisGrid->verticalMinorTickScale());
        m_axisGridShadow->setHorizontalMinorTickScale(m_axisGrid->horizontalMinorTickScale());
        m_axisGridShadow->setVisible(true);
    } else {
        m_axisGridShadow->setVisible(false);
    }
}

void AxisRenderer::updateBarXAxisLabels(QBarCategoryAxis *axis, const QRectF &rect)
{
    int categoriesCount =  axis->categories().size();
    // See if we need more text items
    int currentTextItemsSize = m_xAxisTextItems.size();
    if (currentTextItemsSize < categoriesCount) {
        for (int i = currentTextItemsSize; i <= categoriesCount; i++) {
            auto bi = new QQuickText();
            bi->setParentItem(this);
            m_xAxisTextItems << bi;
        }
    }
    int textIndex = 0;
    for (auto category : axis->categories()) {
        auto &textItem = m_xAxisTextItems[textIndex];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posX = rect.x() + ((float)textIndex / categoriesCount) *  rect.width();
            textItem->setX(posX);
            float posY = rect.y();
            textItem->setY(posY);
            textItem->setHAlign(QQuickText::HAlignment::AlignHCenter);
            textItem->setVAlign(QQuickText::VAlignment::AlignVCenter);
            textItem->setWidth(rect.width() / categoriesCount);
            textItem->setHeight(rect.height());
            textItem->setFont(theme()->axisXLabelsFont());
            textItem->setColor(theme()->axisXLabelsColor());
            textItem->setRotation(axis->labelsAngle());
            textItem->setText(category);
            textItem->setVisible(true);
        } else {
            textItem->setVisible(false);
        }
        textIndex++;
    }
}

void AxisRenderer::updateValueYAxisLabels(QValueAxis *axis, const QRectF &rect)
{
    // Create label values in the range
    QList<double> yAxisLabelValues;
    const int MAX_LABELS_COUNT = 100;
    for (double i = m_axisVerticalMinLabel; i <= m_axisVerticalMaxValue; i += m_axisVerticalValueStep) {
        yAxisLabelValues << i;
        if (yAxisLabelValues.size() >= MAX_LABELS_COUNT)
            break;
    }
    int categoriesCount = yAxisLabelValues.size();

    // See if we need more text items
    int currentTextItemsSize = m_yAxisTextItems.size();
    if (currentTextItemsSize < categoriesCount) {
        for (int i = currentTextItemsSize; i <= categoriesCount; i++) {
            auto bi = new QQuickText();
            bi->setParentItem(this);
            m_yAxisTextItems << bi;
        }
    } else if (categoriesCount < currentTextItemsSize) {
        // Hide unused text items
        for (int i = categoriesCount;  i < currentTextItemsSize; i++) {
            auto &textItem = m_yAxisTextItems[i];
            textItem->setVisible(false);
        }
    }
    for (int i = 0;  i < categoriesCount; i++) {
        auto &textItem = m_yAxisTextItems[i];
        if (axis->isVisible() && axis->labelsVisible()) {
            // TODO: Not general, fix vertical align to work in all cases
            float fontSize = theme()->axisYLabelsFont().pixelSize() < 0 ? theme()->axisYLabelsFont().pointSize() : theme()->axisYLabelsFont().pixelSize();
            float posX = rect.x();
            textItem->setX(posX);
            float posY = rect.y() + rect.height() - (((float)i) * m_axisVerticalStepPx) + m_axisYMovement;
            const double titleMargin = 0.01;
            if ((posY - titleMargin) > (rect.height() + rect.y()) || (posY + titleMargin) < rect.y()) {
                // Hide text item which are outside the axis area
                textItem->setVisible(false);
                continue;
            }
            // Take font size into account only after hiding
            posY -= fontSize;
            textItem->setY(posY);
            textItem->setHAlign(QQuickText::HAlignment::AlignRight);
            textItem->setVAlign(QQuickText::VAlignment::AlignBottom);
            textItem->setWidth(rect.width());
            textItem->setFont(theme()->axisYLabelsFont());
            textItem->setColor(theme()->axisYLabelsColor());
            textItem->setRotation(axis->labelsAngle());
            double number = yAxisLabelValues.at(i);
            // Format the number
            int decimals = axis->labelDecimals();
            if (decimals < 0)
                decimals = getValueDecimalsFromRange(m_axisVerticalValueRange);
            const QString f = axis->labelFormat();
            char format = f.isEmpty() ? 'f' : f.front().toLatin1();
            textItem->setText(QString::number(number, format, decimals));
            textItem->setVisible(true);
        } else {
            textItem->setVisible(false);
        }
    }
}

void AxisRenderer::updateValueXAxisLabels(QValueAxis *axis, const QRectF &rect)
{
    // Create label values in the range
    QList<double> axisLabelValues;
    const int MAX_LABELS_COUNT = 100;
    for (double i = m_axisHorizontalMinLabel; i <= m_axisHorizontalMaxValue; i += m_axisHorizontalValueStep) {
        axisLabelValues << i;
        if (axisLabelValues.size() >= MAX_LABELS_COUNT)
            break;
    }
    int categoriesCount = axisLabelValues.size();

    // See if we need more text items
    int currentTextItemsSize = m_xAxisTextItems.size();
    if (currentTextItemsSize < categoriesCount) {
        for (int i = currentTextItemsSize; i <= categoriesCount; i++) {
            auto bi = new QQuickText();
            bi->setParentItem(this);
            m_xAxisTextItems << bi;
        }
    } else if (categoriesCount < currentTextItemsSize) {
        // Hide unused text items
        for (int i = categoriesCount;  i < currentTextItemsSize; i++) {
            auto &textItem = m_xAxisTextItems[i];
            textItem->setVisible(false);
        }
    }
    for (int i = 0;  i < categoriesCount; i++) {
        auto &textItem = m_xAxisTextItems[i];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posY = rect.y();
            textItem->setY(posY);
            float textItemWidth = 20;
            float posX = rect.x() + (((float)i) * m_axisHorizontalStepPx) - m_axisXMovement;
            const double titleMargin = 0.01;
            if ((posX - titleMargin) > (rect.width() + rect.x()) || (posX + titleMargin) < rect.x()) {
                // Hide text item which are outside the axis area
                textItem->setVisible(false);
                continue;
            }
            // Take text size into account only after hiding
            posX -= 0.5 * textItemWidth;
            textItem->setX(posX);
            textItem->setHAlign(QQuickText::HAlignment::AlignHCenter);
            textItem->setVAlign(QQuickText::VAlignment::AlignBottom);
            textItem->setWidth(textItemWidth);
            textItem->setHeight(rect.height());
            textItem->setFont(theme()->axisYLabelsFont());
            textItem->setColor(theme()->axisYLabelsColor());
            textItem->setRotation(axis->labelsAngle());
            double number = axisLabelValues.at(i);
            // Format the number
            int decimals = axis->labelDecimals();
            if (decimals < 0)
                decimals = getValueDecimalsFromRange(m_axisHorizontalValueRange);
            const QString f = axis->labelFormat();
            char format = f.isEmpty() ? 'f' : f.front().toLatin1();
            textItem->setText(QString::number(number, format, decimals));
            textItem->setVisible(true);
        } else {
            textItem->setVisible(false);
        }
    }
}

// Calculate suitable major step based on range
double AxisRenderer::getValueStepsFromRange(double range)
{
    int digits = std::ceil(std::log10(range));
    double r = std::pow(10.0, -digits);
    r *= 10.0;
    double v = std::ceil(range * r) / r;
    double step = v * 0.1;
    // Step must always be bigger than 0
    step = qMax(0.0001, step);
    return step;
}

// Calculate suitable decimals amount based on range
int AxisRenderer::getValueDecimalsFromRange(double range)
{
    if (range <= 0)
        return 0;
    int decimals = std::ceil(std::log10(10.0 / range));
    // Decimals must always be at least 0
    decimals = qMax(0, decimals);
    return decimals;
}

QT_END_NAMESPACE
