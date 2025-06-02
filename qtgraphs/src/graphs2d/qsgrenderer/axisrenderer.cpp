// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/QBarCategoryAxis>
#include <QtGraphs/QGraphsTheme>
#include <private/axisrenderer_p.h>
#include <private/qabstractaxis_p.h>
#include <private/qbarseries_p.h>
#include <private/qdatetimeaxis_p.h>
#include <private/qgraphsview_p.h>
#include <private/qvalueaxis_p.h>

QT_BEGIN_NAMESPACE

AxisRenderer::AxisRenderer(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_graph = qobject_cast<QGraphsView *>(parent);
    setFlag(QQuickItem::ItemHasContents);
}

AxisRenderer::~AxisRenderer() {}

QGraphsTheme *AxisRenderer::theme() {
    return m_graph->m_theme;
}

void AxisRenderer::initialize() {
    if (m_initialized)
        return;
    if (!window())
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
        m_axisGrid = new AxisGrid(this);
        m_axisGrid->setZ(-1);
        m_axisGrid->setupShaders();
        m_axisGrid->setOrigo(0);
    }
    if (!m_axisLineVertical) {
        m_axisLineVertical = new AxisLine(this);
        m_axisLineVertical->setZ(-1);
        m_axisLineVertical->setupShaders();
    }
    if (!m_axisTickerVertical) {
        m_axisTickerVertical = new AxisTicker(this);
        m_axisTickerVertical->setZ(-2);
        m_axisTickerVertical->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerVertical->setSubTickLength(0.5);
        m_axisTickerVertical->setupShaders();
    }
    if (!m_axisLineHorizontal) {
        m_axisLineHorizontal = new AxisLine(this);
        m_axisLineHorizontal->setZ(-1);
        m_axisLineHorizontal->setIsHorizontal(true);
        m_axisLineHorizontal->setupShaders();
    }
    if (!m_axisTickerHorizontal) {
        m_axisTickerHorizontal = new AxisTicker(this);
        m_axisTickerHorizontal->setZ(-2);
        m_axisTickerHorizontal->setIsHorizontal(true);
        m_axisTickerHorizontal->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerHorizontal->setSubTickLength(0.2);
        m_axisTickerHorizontal->setupShaders();
    }

    // TODO: Create shadows only when needed
    if (!m_axisGridShadow) {
        m_axisGridShadow = new AxisGrid(this);
        m_axisGridShadow->setZ(-3);
        m_axisGridShadow->setupShaders();
        m_axisGridShadow->setOrigo(0);
    }
    if (!m_axisLineVerticalShadow) {
        m_axisLineVerticalShadow = new AxisLine(this);
        m_axisLineVerticalShadow->setZ(-3);
        m_axisLineVerticalShadow->setupShaders();
    }
    if (!m_axisTickerVerticalShadow) {
        m_axisTickerVerticalShadow = new AxisTicker(this);
        m_axisTickerVerticalShadow->setZ(-3);
        m_axisTickerVerticalShadow->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerVerticalShadow->setSubTickLength(m_axisTickerVertical->subTickLength());
        m_axisTickerVerticalShadow->setupShaders();
    }
    if (!m_axisLineHorizontalShadow) {
        m_axisLineHorizontalShadow = new AxisLine(this);
        m_axisLineHorizontalShadow->setZ(-3);
        m_axisLineHorizontalShadow->setupShaders();
    }
    if (!m_axisTickerHorizontalShadow) {
        m_axisTickerHorizontalShadow = new AxisTicker(this);
        m_axisTickerHorizontalShadow->setZ(-3);
        m_axisTickerHorizontalShadow->setIsHorizontal(true);
        m_axisTickerHorizontalShadow->setOrigo(0);
        // TODO: Configurable in theme or axis?
        m_axisTickerHorizontalShadow->setSubTickLength(m_axisTickerHorizontal->subTickLength());
        m_axisTickerHorizontalShadow->setupShaders();
    }

    updateAxis();
}

void AxisRenderer::updateAxis()
{
    if (!theme())
        return;

    // Update active axis
    QAbstractAxis *axisVertical = m_graph->m_axisY;
    QAbstractAxis *axisHorizontal = m_graph->m_axisX;

    // See if series is horizontal, so axis should also switch places.
    bool vertical = true;
    if (m_graph->orientation() == Qt::Orientation::Horizontal)
        vertical = false;
    if (vertical) {
        m_axisVertical = axisVertical;
        m_axisHorizontal = axisHorizontal;
    } else {
        m_axisVertical = axisHorizontal;
        m_axisHorizontal = axisVertical;
    }

    if (vertical != m_wasVertical) {
        // Orientation has changed, so clear possible custom elements
        for (auto &item : m_xAxisTextItems)
            item->deleteLater();
        m_xAxisTextItems.clear();

        for (auto &item : m_yAxisTextItems)
            item->deleteLater();
        m_yAxisTextItems.clear();

        m_wasVertical = vertical;
    }

    float axisWidth = m_graph->m_axisWidth;
    float axisHeight = m_graph->m_axisHeight;

    const bool gridVisible = theme()->isGridVisible();
    if (m_axisVertical) {
        m_gridVerticalLinesVisible = gridVisible && m_axisVertical->isGridVisible();
        m_gridVerticalSubLinesVisible = gridVisible && m_axisVertical->isSubGridVisible();
    }
    if (m_axisHorizontal) {
        m_gridHorizontalLinesVisible = gridVisible && m_axisHorizontal->isGridVisible();
        m_gridHorizontalSubLinesVisible = gridVisible && m_axisHorizontal->isSubGridVisible();
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
        int axisVerticalSubTickCount = vaxis->subTickCount();
        m_axisVerticalSubGridScale = axisVerticalSubTickCount > 0 ? 1.0 / (axisVerticalSubTickCount + 1) : 1.0;
        m_axisVerticalStepPx = (height() - m_graph->m_marginTop - m_graph->m_marginBottom - axisHeight) / (m_axisVerticalValueRange / m_axisVerticalValueStep);
        double axisVerticalValueDiff = m_axisVerticalMinLabel - m_axisVerticalMinValue;
        m_axisYDisplacement = -(axisVerticalValueDiff / m_axisVerticalValueStep) * m_axisVerticalStepPx;

        // Update value labels
        updateValueYAxisLabels(vaxis, m_graph->m_yAxisLabelsArea);
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
        int axisHorizontalSubTickCount = haxis->subTickCount();
        m_axisHorizontalSubGridScale = axisHorizontalSubTickCount > 0 ?
                1.0 / (axisHorizontalSubTickCount + 1) : 1.0;
        m_axisHorizontalStepPx = (width() - m_graph->m_marginLeft - m_graph->m_marginRight - axisWidth)
                / (m_axisHorizontalValueRange / m_axisHorizontalValueStep);
        double axisHorizontalValueDiff = m_axisHorizontalMinLabel - m_axisHorizontalMinValue;
        m_axisXDisplacement = -(axisHorizontalValueDiff / m_axisHorizontalValueStep) * m_axisHorizontalStepPx;

        // Update value labels
        updateValueXAxisLabels(haxis, m_graph->m_xAxisLabelsArea);
    }

    if (auto haxis = qobject_cast<QBarCategoryAxis *>(m_axisHorizontal)) {
        m_axisHorizontalMaxValue = haxis->categories().size();
        m_axisHorizontalMinValue = 0;
        m_axisHorizontalValueRange = m_axisHorizontalMaxValue - m_axisHorizontalMinValue;
        updateBarXAxisLabels(haxis, m_graph->m_xAxisLabelsArea);
    }
    if (auto vaxis = qobject_cast<QBarCategoryAxis *>(m_axisVertical)) {
        m_axisVerticalMaxValue = vaxis->categories().size();
        m_axisVerticalMinValue = 0;
        m_axisVerticalValueRange = m_axisVerticalMaxValue - m_axisVerticalMinValue;
        updateBarYAxisLabels(vaxis, m_graph->m_yAxisLabelsArea);
    }

    if (auto vaxis = qobject_cast<QDateTimeAxis *>(m_axisVertical)) {
        // Todo: make constant for all axis, or clamp in class? (QTBUG-124736)
        const double MAX_DIVS = 100.0;

        double interval = std::clamp<double>(vaxis->tickInterval(), 0.0, MAX_DIVS);
        m_axisVerticalMaxValue = vaxis->max().toMSecsSinceEpoch();
        m_axisVerticalMinValue = vaxis->min().toMSecsSinceEpoch();
        m_axisVerticalValueRange = std::abs(m_axisVerticalMaxValue - m_axisVerticalMinValue);

        // in ms
        double segment;
        if (interval <= 0) {
            segment = getValueStepsFromRange(m_axisVerticalValueRange);
            interval = m_axisVerticalValueRange / segment;
        } else {
            segment = m_axisVerticalValueRange / interval;
        }

        m_axisVerticalMinLabel = std::clamp(interval, 1.0, MAX_DIVS);

        m_axisVerticalValueStep = segment;
        int axisVerticalSubTickCount = vaxis->subTickCount();
        m_axisVerticalSubGridScale = axisVerticalSubTickCount > 0
                                           ? 1.0 / (axisVerticalSubTickCount + 1)
                                           : 1.0;
        m_axisVerticalStepPx = (height() - m_graph->m_marginTop - m_graph->m_marginBottom
                                - axisHeight)
                               / (qFuzzyCompare(segment, 0)
                                      ? interval
                                      : (m_axisVerticalValueRange / m_axisVerticalValueStep));

        updateDateTimeYAxisLabels(vaxis, m_graph->m_yAxisLabelsArea);
    }

    if (auto haxis = qobject_cast<QDateTimeAxis *>(m_axisHorizontal)) {
        const double MAX_DIVS = 100.0;

        double interval = std::clamp<double>(haxis->tickInterval(), 0.0, MAX_DIVS);
        m_axisHorizontalMaxValue = haxis->max().toMSecsSinceEpoch();
        m_axisHorizontalMinValue = haxis->min().toMSecsSinceEpoch();
        m_axisHorizontalValueRange = std::abs(m_axisHorizontalMaxValue - m_axisHorizontalMinValue);

        // in ms
        double segment;
        if (interval <= 0) {
            segment = getValueStepsFromRange(m_axisHorizontalValueRange);
            interval = m_axisHorizontalValueRange / segment;
        } else {
            segment = m_axisHorizontalValueRange / interval;
        }

        m_axisHorizontalMinLabel = std::clamp(interval, 1.0, MAX_DIVS);

        m_axisHorizontalValueStep = segment;
        int axisHorizontalSubTickCount = haxis->subTickCount();
        m_axisHorizontalSubGridScale = axisHorizontalSubTickCount > 0
                                             ? 1.0 / (axisHorizontalSubTickCount + 1)
                                             : 1.0;
        m_axisHorizontalStepPx = (width() - m_graph->m_marginLeft - m_graph->m_marginRight
                                  - axisWidth)
                                 / (qFuzzyCompare(segment, 0)
                                        ? interval
                                        : (m_axisHorizontalValueRange / m_axisHorizontalValueStep));
        updateDateTimeXAxisLabels(haxis, m_graph->m_xAxisLabelsArea);
    }

    updateAxisTickers();
    updateAxisTickersShadow();
    updateAxisGrid();
    updateAxisGridShadow();
    updateAxisTitles(m_graph->m_xAxisLabelsArea, m_graph->m_yAxisLabelsArea);
}

void AxisRenderer::updateAxisTickers()
{
    if (m_axisVertical) {
        // Note: Fix before enabling, see QTBUG-121207 and QTBUG-121211
        //if (theme()->themeDirty()) {
            m_axisTickerVertical->setSubTickColor(theme()->axisY().subColor());
            m_axisTickerVertical->setTickColor(theme()->axisY().mainColor());
            m_axisTickerVertical->setTickLineWidth(theme()->axisY().mainWidth());
            m_axisTickerVertical->setSubTickLineWidth(theme()->axisY().subWidth());
            m_axisTickerVertical->setSmoothing(m_graph->axisYSmoothing());
        //}
        float topPadding = m_axisGrid->gridLineWidth() * 0.5;
        float bottomPadding = topPadding;
        // TODO Only when changed
        m_axisTickerVertical->setDisplacement(m_axisYDisplacement);
        QRectF rect = m_graph->m_yAxisTickersArea;
        m_axisTickerVertical->setX(rect.x());
        m_axisTickerVertical->setY(rect.y());
        m_axisTickerVertical->setWidth(rect.width());
        m_axisTickerVertical->setHeight(rect.height());
        m_axisTickerVertical->setFlipped(m_verticalAxisOnRight);

        m_axisTickerVertical->setSpacing((m_axisTickerVertical->height() - topPadding - bottomPadding)
                                         / (m_axisVerticalValueRange / m_axisVerticalValueStep));
        m_axisTickerVertical->setSubTicksVisible(!qFuzzyCompare(m_axisVerticalSubGridScale, 1.0));
        m_axisTickerVertical->setSubTickScale(m_axisVerticalSubGridScale);
        m_axisTickerVertical->setVisible(m_axisVertical->isVisible());
        // Axis line
        m_axisLineVertical->setColor(theme()->axisY().mainColor());
        m_axisLineVertical->setLineWidth(theme()->axisY().mainWidth());
        m_axisLineVertical->setSmoothing(m_graph->axisYSmoothing());

        float xMovement = 0.5 * (m_axisLineVertical->lineWidth() + m_axisLineVertical->smoothing());
        if (m_verticalAxisOnRight)
            m_axisLineVertical->setX(m_axisTickerVertical->x() - xMovement);
        else
            m_axisLineVertical->setX(m_axisTickerVertical->x() + m_axisTickerVertical->width() - xMovement);
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
        //if (theme()->themeDirty()) {
            m_axisTickerHorizontal->setSubTickColor(theme()->axisX().subColor());
            m_axisTickerHorizontal->setTickColor(theme()->axisX().mainColor());
            m_axisTickerHorizontal->setTickLineWidth(theme()->axisX().mainWidth());
            m_axisTickerHorizontal->setSubTickLineWidth(theme()->axisX().subWidth());
            m_axisTickerHorizontal->setSmoothing(m_graph->axisXSmoothing());
        //}
        float leftPadding = m_axisGrid->gridLineWidth() * 0.5;
        float rightPadding = leftPadding;
        // TODO Only when changed
        m_axisTickerHorizontal->setDisplacement(m_axisXDisplacement);
        QRectF rect = m_graph->m_xAxisTickersArea;
        m_axisTickerHorizontal->setX(rect.x());
        m_axisTickerHorizontal->setY(rect.y());
        m_axisTickerHorizontal->setWidth(rect.width());
        m_axisTickerHorizontal->setHeight(rect.height());
        m_axisTickerHorizontal->setFlipped(m_horizontalAxisOnTop);

        m_axisTickerHorizontal->setSpacing((m_axisTickerHorizontal->width() - leftPadding - rightPadding)
                                         / (m_axisHorizontalValueRange / m_axisHorizontalValueStep));
        m_axisTickerHorizontal->setSubTicksVisible(!qFuzzyCompare(m_axisHorizontalSubGridScale, 1.0));
        m_axisTickerHorizontal->setSubTickScale(m_axisHorizontalSubGridScale);
        m_axisTickerHorizontal->setVisible(m_axisHorizontal->isVisible());
        // Axis line
        m_axisLineHorizontal->setColor(theme()->axisX().mainColor());
        m_axisLineHorizontal->setLineWidth(theme()->axisX().mainWidth());
        m_axisLineHorizontal->setSmoothing(m_graph->axisXSmoothing());
        m_axisLineHorizontal->setX(m_axisTickerHorizontal->x());
        float yMovement = 0.5 * (m_axisLineHorizontal->lineWidth() + m_axisLineHorizontal->smoothing());
        if (m_horizontalAxisOnTop)
            m_axisLineHorizontal->setY(m_axisTickerHorizontal->y() + m_axisTickerHorizontal->height() - yMovement);
        else
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
    if (m_axisVertical && m_graph->isShadowVisible()) {
        m_axisTickerVerticalShadow->setSubTickColor(m_graph->shadowColor());
        m_axisTickerVerticalShadow->setTickColor(m_graph->shadowColor());
        m_axisTickerVerticalShadow->setSubTickLineWidth(m_axisTickerVertical->subTickLineWidth() + m_graph->shadowBarWidth());
        m_axisTickerVerticalShadow->setTickLineWidth(m_axisTickerVertical->tickLineWidth() + m_graph->shadowBarWidth());
        m_axisTickerVerticalShadow->setSmoothing(m_axisTickerVertical->smoothing() + m_graph->shadowSmoothing());

        // TODO Only when changed
        m_axisTickerVerticalShadow->setDisplacement(m_axisTickerVertical->displacement());
        m_axisTickerVerticalShadow->setX(m_axisTickerVertical->x() + m_graph->shadowXOffset());
        m_axisTickerVerticalShadow->setY(m_axisTickerVertical->y() + m_graph->shadowYOffset() + m_graph->shadowBarWidth() * 0.5);
        m_axisTickerVerticalShadow->setWidth(m_axisTickerVertical->width());
        m_axisTickerVerticalShadow->setHeight(m_axisTickerVertical->height());
        m_axisTickerVerticalShadow->setFlipped(m_axisTickerVertical->isFlipped());
        m_axisTickerVerticalShadow->setSpacing(m_axisTickerVertical->spacing());
        m_axisTickerVerticalShadow->setSubTicksVisible(m_axisTickerVertical->subTicksVisible());
        m_axisTickerVerticalShadow->setSubTickScale(m_axisTickerVertical->subTickScale());
        m_axisTickerVerticalShadow->setVisible(m_axisTickerVertical->isVisible());
        // Axis line
        m_axisLineVerticalShadow->setColor(m_graph->shadowColor());
        m_axisLineVerticalShadow->setLineWidth(m_axisLineVertical->lineWidth() + m_graph->shadowBarWidth());
        m_axisLineVerticalShadow->setSmoothing(m_axisLineVertical->smoothing() + m_graph->shadowSmoothing());
        m_axisLineVerticalShadow->setX(m_axisLineVertical->x() + m_graph->shadowXOffset());
        m_axisLineVerticalShadow->setY(m_axisLineVertical->y() + m_graph->shadowYOffset() + m_graph->shadowBarWidth() * 0.5);
        m_axisLineVerticalShadow->setWidth(m_axisLineVertical->width());
        m_axisLineVerticalShadow->setHeight(m_axisLineVertical->height());
        m_axisLineVerticalShadow->setVisible(m_axisLineVertical->isVisible());
    } else {
        // Hide all parts of vertical axis
        m_axisTickerVerticalShadow->setVisible(false);
        m_axisLineVerticalShadow->setVisible(false);
    }

    if (m_axisHorizontal && m_graph->isShadowVisible()) {
        m_axisTickerHorizontalShadow->setSubTickColor(m_graph->shadowColor());
        m_axisTickerHorizontalShadow->setTickColor(m_graph->shadowColor());
        m_axisTickerHorizontalShadow->setSubTickLineWidth(m_axisTickerHorizontal->subTickLineWidth() + m_graph->shadowBarWidth());
        m_axisTickerHorizontalShadow->setTickLineWidth(m_axisTickerHorizontal->tickLineWidth() + m_graph->shadowBarWidth());
        m_axisTickerHorizontalShadow->setSmoothing(m_axisTickerHorizontal->smoothing() + m_graph->shadowSmoothing());

        // TODO Only when changed
        m_axisTickerHorizontalShadow->setDisplacement(m_axisTickerHorizontal->displacement());
        m_axisTickerHorizontalShadow->setX(m_axisTickerHorizontal->x() + m_graph->shadowXOffset() - m_graph->shadowBarWidth() * 0.5);
        m_axisTickerHorizontalShadow->setY(m_axisTickerHorizontal->y() + m_graph->shadowYOffset());
        m_axisTickerHorizontalShadow->setWidth(m_axisTickerHorizontal->width());
        m_axisTickerHorizontalShadow->setHeight(m_axisTickerHorizontal->height());
        m_axisTickerHorizontalShadow->setFlipped(m_axisTickerHorizontal->isFlipped());
        m_axisTickerHorizontalShadow->setSpacing(m_axisTickerHorizontal->spacing());
        m_axisTickerHorizontalShadow->setSubTicksVisible(m_axisTickerHorizontal->subTicksVisible());
        m_axisTickerHorizontalShadow->setSubTickScale(m_axisTickerHorizontal->subTickScale());
        m_axisTickerHorizontalShadow->setVisible(m_axisTickerHorizontal->isVisible());
        // Axis line
        m_axisLineHorizontalShadow->setColor(m_graph->shadowColor());
        m_axisLineHorizontalShadow->setLineWidth(m_axisLineHorizontal->width() + m_graph->shadowBarWidth());
        m_axisLineHorizontalShadow->setSmoothing(m_axisLineHorizontal->smoothing() + m_graph->shadowSmoothing());
        m_axisLineHorizontalShadow->setX(m_axisLineHorizontal->x() + m_graph->shadowXOffset() - m_graph->shadowBarWidth() * 0.5);
        m_axisLineHorizontalShadow->setY(m_axisLineHorizontal->y() + m_graph->shadowYOffset());
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
    m_axisGrid->setGridColor(theme()->grid().mainColor());
    m_axisGrid->setSubGridColor(theme()->grid().subColor());
    m_axisGrid->setSubGridLineWidth(theme()->grid().subWidth());
    m_axisGrid->setGridLineWidth(theme()->grid().mainWidth());
    const double minimumSmoothing = 0.05;
    m_axisGrid->setSmoothing(m_graph->gridSmoothing() + minimumSmoothing);
    if (theme()->isPlotAreaBackgroundVisible())
        m_axisGrid->setPlotAreaBackgroundColor(theme()->plotAreaBackgroundColor());
    else
        m_axisGrid->setPlotAreaBackgroundColor(QColorConstants::Transparent);

    float topPadding = m_axisGrid->gridLineWidth() * 0.5;
    float bottomPadding = topPadding;
    float leftPadding = topPadding;
    float rightPadding = topPadding;
    // TODO Only when changed
    m_axisGrid->setGridMovement(QPointF(m_axisXDisplacement, m_axisYDisplacement));
    QRectF rect = m_graph->m_plotArea;
    m_axisGrid->setX(rect.x());
    m_axisGrid->setY(rect.y());
    m_axisGrid->setWidth(rect.width());
    m_axisGrid->setHeight(rect.height());

    m_axisGrid->setGridWidth((m_axisGrid->width() - leftPadding - rightPadding)
                             / (m_axisHorizontalValueRange / m_axisHorizontalValueStep));
    m_axisGrid->setGridHeight((m_axisGrid->height() - topPadding - bottomPadding)
                              / (m_axisVerticalValueRange / m_axisVerticalValueStep));
    m_axisGrid->setGridVisibility(QVector4D(m_gridHorizontalLinesVisible,
                                            m_gridVerticalLinesVisible,
                                            m_gridHorizontalSubLinesVisible,
                                            m_gridVerticalSubLinesVisible));
    m_axisGrid->setVerticalSubGridScale(m_axisVerticalSubGridScale);
    m_axisGrid->setHorizontalSubGridScale(m_axisHorizontalSubGridScale);
}

void AxisRenderer::updateAxisGridShadow()
{
    if (m_graph->isShadowVisible()) {
        m_axisGridShadow->setGridColor(m_graph->shadowColor());
        m_axisGridShadow->setSubGridColor(m_graph->shadowColor());
        m_axisGridShadow->setSubGridLineWidth(m_axisGrid->subGridLineWidth() + m_graph->shadowBarWidth());
        m_axisGridShadow->setGridLineWidth(m_axisGrid->gridLineWidth() + m_graph->shadowBarWidth());
        m_axisGridShadow->setSmoothing(m_axisGrid->smoothing() + m_graph->shadowSmoothing());

        // TODO Only when changed
        m_axisGridShadow->setGridMovement(m_axisGrid->gridMovement());
        m_axisGridShadow->setX(m_axisGrid->x() + m_graph->shadowXOffset() - m_graph->shadowBarWidth() * 0.5);
        m_axisGridShadow->setY(m_axisGrid->y() + m_graph->shadowYOffset() + m_graph->shadowBarWidth() * 0.5);
        m_axisGridShadow->setWidth(m_axisGrid->width());
        m_axisGridShadow->setHeight(m_axisGrid->height());
        m_axisGridShadow->setGridWidth(m_axisGrid->gridWidth());
        m_axisGridShadow->setGridHeight(m_axisGrid->gridHeight());
        m_axisGridShadow->setGridVisibility(m_axisGrid->gridVisibility());
        m_axisGridShadow->setVerticalSubGridScale(m_axisGrid->verticalSubGridScale());
        m_axisGridShadow->setHorizontalSubGridScale(m_axisGrid->horizontalSubGridScale());
        m_axisGridShadow->setVisible(true);
    } else {
        m_axisGridShadow->setVisible(false);
    }
}

void AxisRenderer::updateAxisTitles(const QRectF xAxisRect, const QRectF yAxisRect)
{
    if (!m_xAxisTitle) {
        m_xAxisTitle = new QQuickText(this);
        m_xAxisTitle->setVAlign(QQuickText::AlignBottom);
        m_xAxisTitle->setHAlign(QQuickText::AlignHCenter);
    }

    if (!m_yAxisTitle) {
        m_yAxisTitle = new QQuickText(this);
        m_yAxisTitle->setVAlign(QQuickText::AlignVCenter);
        m_yAxisTitle->setHAlign(QQuickText::AlignHCenter);
    }

    if (m_axisHorizontal && m_axisHorizontal->isTitleVisible()) {
        m_xAxisTitle->setText(m_axisHorizontal->titleText());
        m_xAxisTitle->setX((2 * xAxisRect.x() - m_xAxisTitle->contentWidth() + xAxisRect.width())
                           * 0.5);
        m_xAxisTitle->setY(xAxisRect.y() + xAxisRect.height());
        if (m_axisHorizontal->titleColor().isValid())
            m_xAxisTitle->setColor(m_axisHorizontal->titleColor());
        else
            m_xAxisTitle->setColor(theme()->labelTextColor());
        m_xAxisTitle->setFont(m_axisHorizontal->titleFont());
        m_xAxisTitle->setVisible(true);
    } else {
        m_xAxisTitle->setVisible(false);
    }

    if (m_axisVertical && m_axisVertical->isTitleVisible()) {
        m_yAxisTitle->setText(m_axisVertical->titleText());
        m_yAxisTitle->setX(0 + m_yAxisTitle->height() - m_yAxisTitle->contentWidth() * 0.5);
        m_yAxisTitle->setY((2 * yAxisRect.y() - m_yAxisTitle->contentHeight() + yAxisRect.height())
                           * 0.5);
        m_yAxisTitle->setRotation(-90);
        if (m_axisVertical->titleColor().isValid())
            m_yAxisTitle->setColor(m_axisVertical->titleColor());
        else
            m_yAxisTitle->setColor(theme()->labelTextColor());
        m_yAxisTitle->setFont(m_axisVertical->titleFont());
        m_yAxisTitle->setVisible(true);
    } else {
        m_yAxisTitle->setVisible(false);
    }
}

void AxisRenderer::updateAxisLabelItems(QList<QQuickItem *> &textItems,
                                        qsizetype neededSize, QQmlComponent *component)
{
    qsizetype currentTextItemsSize = textItems.size();
    if (currentTextItemsSize < neededSize) {
        for (qsizetype i = currentTextItemsSize; i <= neededSize; i++) {
            QQuickItem *item = nullptr;
            if (component) {
                item = qobject_cast<QQuickItem *>(
                    component->create(component->creationContext()));
            }
            if (!item)
                item = new QQuickText();
            item->setParent(this);
            item->setParentItem(this);
            textItems << item;
        }
    } else if (neededSize < currentTextItemsSize) {
        // Hide unused text items
        for (qsizetype i = neededSize;  i < currentTextItemsSize; i++) {
            auto textItem = textItems[i];
            textItem->setVisible(false);
        }
    }
}

void AxisRenderer::setLabelTextProperties(QQuickItem *item, const QString &text, bool xAxis,
                                          QQuickText::HAlignment hAlign, QQuickText::VAlignment vAlign)
{
    if (auto textItem = qobject_cast<QQuickText *>(item)) {
        // If the component is a Text item (default), then text
        // properties can be set directly.
        textItem->setText(text);
        textItem->setHeight(textItem->contentHeight()); // Default height
        textItem->setHAlign(hAlign);
        textItem->setVAlign(vAlign);
        if (xAxis) {
            textItem->setFont(theme()->axisXLabelFont());
            textItem->setColor(theme()->axisX().labelTextColor());
        } else {
            textItem->setFont(theme()->axisYLabelFont());
            textItem->setColor(theme()->axisY().labelTextColor());
        }
    } else {
        // Check for specific dynamic properties
        if (item->property("text").isValid())
            item->setProperty("text", text);
    }
}

void AxisRenderer::updateBarXAxisLabels(QBarCategoryAxis *axis, const QRectF rect)
{
    qsizetype categoriesCount = axis->categories().size();
    // See if we need more text items
    updateAxisLabelItems(m_xAxisTextItems, categoriesCount, axis->labelDelegate());

    int textIndex = 0;
    for (auto category : axis->categories()) {
        auto &textItem = m_xAxisTextItems[textIndex];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posX = rect.x() + ((float)textIndex / categoriesCount) *  rect.width();
            textItem->setX(posX);
            float posY = rect.y();
            textItem->setY(posY);
            textItem->setWidth(rect.width() / categoriesCount);
            textItem->setRotation(axis->labelsAngle());
            if (m_horizontalAxisOnTop) {
                setLabelTextProperties(textItem, category, true,
                                       QQuickText::HAlignment::AlignHCenter,
                                       QQuickText::VAlignment::AlignBottom);
            } else {
                setLabelTextProperties(textItem, category, true,
                                       QQuickText::HAlignment::AlignHCenter,
                                       QQuickText::VAlignment::AlignTop);
            }
            textItem->setHeight(rect.height());
            textItem->setVisible(true);
            theme()->dirtyBits()->axisXDirty = false;
        } else {
            textItem->setVisible(false);
        }
        textIndex++;
    }
}

void AxisRenderer::updateBarYAxisLabels(QBarCategoryAxis *axis, const QRectF rect)
{
    qsizetype categoriesCount = axis->categories().size();
    // See if we need more text items
    updateAxisLabelItems(m_yAxisTextItems, categoriesCount, axis->labelDelegate());

    int textIndex = 0;
    for (auto category : axis->categories()) {
        auto &textItem = m_yAxisTextItems[textIndex];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posX = rect.x();
            textItem->setX(posX);
            float posY = rect.y() + ((float)textIndex / categoriesCount) *  rect.height();
            textItem->setY(posY);
            textItem->setWidth(rect.width());
            textItem->setRotation(axis->labelsAngle());
            if (m_verticalAxisOnRight) {
                setLabelTextProperties(textItem, category, false,
                                       QQuickText::HAlignment::AlignRight,
                                       QQuickText::VAlignment::AlignVCenter);
            } else {
                setLabelTextProperties(textItem, category, false,
                                       QQuickText::HAlignment::AlignLeft,
                                       QQuickText::VAlignment::AlignVCenter);
            }
            textItem->setHeight(rect.height() / categoriesCount);
            textItem->setVisible(true);
            theme()->dirtyBits()->axisYDirty = false;
        } else {
            textItem->setVisible(false);
        }
        textIndex++;
    }
}

void AxisRenderer::updateValueYAxisLabels(QValueAxis *axis, const QRectF rect)
{
    // Create label values in the range
    QList<double> yAxisLabelValues;
    const int MAX_LABELS_COUNT = 100;
    for (double i = m_axisVerticalMinLabel; i <= m_axisVerticalMaxValue; i += m_axisVerticalValueStep) {
        yAxisLabelValues << i;
        if (yAxisLabelValues.size() >= MAX_LABELS_COUNT)
            break;
    }
    qsizetype categoriesCount = yAxisLabelValues.size();

    // See if we need more text items
    updateAxisLabelItems(m_yAxisTextItems, categoriesCount, axis->labelDelegate());

    for (int i = 0;  i < categoriesCount; i++) {
        auto &textItem = m_yAxisTextItems[i];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posX = rect.x();
            textItem->setX(posX);
            float posY = rect.y() + rect.height() - (((float)i) * m_axisVerticalStepPx) + m_axisYDisplacement;
            const double titleMargin = 0.01;
            if ((posY - titleMargin) > (rect.height() + rect.y()) || (posY + titleMargin) < rect.y()) {
                // Hide text item which are outside the axis area
                textItem->setVisible(false);
                continue;
            }
            textItem->setY(posY);
            textItem->setWidth(rect.width());
            textItem->setRotation(axis->labelsAngle());
            double number = yAxisLabelValues.at(i);
            // Format the number
            int decimals = axis->labelDecimals();
            if (decimals < 0)
                decimals = getValueDecimalsFromRange(m_axisVerticalValueRange);
            const QString f = axis->labelFormat();
            QString label;
            if (f.length() <= 1) {
              char format = f.isEmpty() ? 'f' : f.front().toLatin1();
              label = QString::number(number, format, decimals);
            } else {
              QByteArray array = f.toLatin1();
              label = QString::asprintf(array.constData(), number);
            }
            if (m_verticalAxisOnRight) {
                setLabelTextProperties(textItem, label, false,
                                       QQuickText::HAlignment::AlignLeft,
                                       QQuickText::VAlignment::AlignVCenter);
            } else {
                setLabelTextProperties(textItem, label, false,
                                       QQuickText::HAlignment::AlignRight,
                                       QQuickText::VAlignment::AlignVCenter);
            }
            textItem->setHeight(0);
            textItem->setVisible(true);
            theme()->dirtyBits()->axisYDirty = false;
        } else {
            textItem->setVisible(false);
        }
    }
}

void AxisRenderer::updateValueXAxisLabels(QValueAxis *axis, const QRectF rect)
{
    // Create label values in the range
    QList<double> axisLabelValues;
    const int MAX_LABELS_COUNT = 100;
    for (double i = m_axisHorizontalMinLabel; i <= m_axisHorizontalMaxValue; i += m_axisHorizontalValueStep) {
        axisLabelValues << i;
        if (axisLabelValues.size() >= MAX_LABELS_COUNT)
            break;
    }
    qsizetype categoriesCount = axisLabelValues.size();

    // See if we need more text items
    updateAxisLabelItems(m_xAxisTextItems, categoriesCount, axis->labelDelegate());

    for (int i = 0;  i < categoriesCount; i++) {
        auto &textItem = m_xAxisTextItems[i];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posY = rect.y();
            textItem->setY(posY);
            float textItemWidth = 20;
            float posX = rect.x() + (((float)i) * m_axisHorizontalStepPx) - m_axisXDisplacement;
            const double titleMargin = 0.01;
            if ((posX - titleMargin) > (rect.width() + rect.x()) || (posX + titleMargin) < rect.x()) {
                // Hide text item which are outside the axis area
                textItem->setVisible(false);
                continue;
            }
            // Take text size into account only after hiding
            posX -= 0.5 * textItemWidth;
            textItem->setX(posX);
            textItem->setWidth(textItemWidth);
            textItem->setRotation(axis->labelsAngle());
            double number = axisLabelValues.at(i);
            // Format the number
            int decimals = axis->labelDecimals();
            if (decimals < 0)
                decimals = getValueDecimalsFromRange(m_axisHorizontalValueRange);
            const QString f = axis->labelFormat();
            QString label;
            if (f.length() <= 1) {
              char format = f.isEmpty() ? 'f' : f.front().toLatin1();
              label = QString::number(number, format, decimals);
            } else {
              QByteArray array = f.toLatin1();
              label = QString::asprintf(array.constData(), number);
            }
            if (m_horizontalAxisOnTop) {
                setLabelTextProperties(textItem, label, true,
                                       QQuickText::HAlignment::AlignHCenter,
                                       QQuickText::VAlignment::AlignBottom);
            } else {
                setLabelTextProperties(textItem, label, true,
                                       QQuickText::HAlignment::AlignHCenter,
                                       QQuickText::VAlignment::AlignTop);
            }
            textItem->setHeight(rect.height());
            textItem->setVisible(true);
            theme()->dirtyBits()->axisXDirty = false;
        } else {
            textItem->setVisible(false);
        }
    }
}

void AxisRenderer::updateDateTimeYAxisLabels(QDateTimeAxis *axis, const QRectF rect)
{
    auto maxDate = axis->max();
    auto minDate = axis->min();
    int dateTimeSize = m_axisVerticalMinLabel + 1;
    auto segment = (maxDate.toMSecsSinceEpoch() - minDate.toMSecsSinceEpoch())
                   / m_axisVerticalMinLabel;

    // See if we need more text items
    updateAxisLabelItems(m_yAxisTextItems, dateTimeSize, axis->labelDelegate());

    for (auto i = 0; i < dateTimeSize; ++i) {
        auto &textItem = m_yAxisTextItems[i];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posX = rect.x();
            textItem->setX(posX);
            float posY = rect.y() + rect.height() - (((float) i) * m_axisVerticalStepPx);
            const double titleMargin = 0.01;
            if ((posY - titleMargin) > (rect.height() + rect.y())
                || (posY + titleMargin) < rect.y()) {
                // Hide text item which are outside the axis area
                textItem->setVisible(false);
                continue;
            }
            textItem->setY(posY);
            textItem->setWidth(rect.width());
            textItem->setRotation(axis->labelsAngle());
            QString label = minDate.addMSecs(segment * i).toString(axis->labelFormat());
            if (m_verticalAxisOnRight) {
                setLabelTextProperties(textItem, label, false,
                                       QQuickText::HAlignment::AlignLeft,
                                       QQuickText::VAlignment::AlignVCenter);
            } else {
                setLabelTextProperties(textItem, label, false,
                                       QQuickText::HAlignment::AlignRight,
                                       QQuickText::VAlignment::AlignVCenter);
            }
            textItem->setHeight(0);
            textItem->setVisible(true);
        } else {
            textItem->setVisible(false);
        }
    }
}

void AxisRenderer::updateDateTimeXAxisLabels(QDateTimeAxis *axis, const QRectF rect)
{
    auto maxDate = axis->max();
    auto minDate = axis->min();
    int dateTimeSize = m_axisHorizontalMinLabel + 1;
    auto segment = (maxDate.toMSecsSinceEpoch() - minDate.toMSecsSinceEpoch())
                   / m_axisHorizontalMinLabel;

    // See if we need more text items
    updateAxisLabelItems(m_xAxisTextItems, dateTimeSize, axis->labelDelegate());

    for (auto i = 0; i < dateTimeSize; ++i) {
        auto &textItem = m_xAxisTextItems[i];
        if (axis->isVisible() && axis->labelsVisible()) {
            float posY = rect.y();
            textItem->setY(posY);
            float textItemWidth = 20;
            float posX = rect.x() + (((float) i) * m_axisHorizontalStepPx);
            const double titleMargin = 0.01;
            if ((posX - titleMargin) > (rect.width() + rect.x())
                || (posX + titleMargin) < rect.x()) {
                // Hide text item which are outside the axis area
                textItem->setVisible(false);
                continue;
            }
            // Take text size into account only after hiding
            posX -= 0.5 * textItemWidth;
            textItem->setX(posX);
            textItem->setWidth(textItemWidth);
            textItem->setRotation(axis->labelsAngle());
            QString label = minDate.addMSecs(segment * i).toString(axis->labelFormat());
            if (m_horizontalAxisOnTop) {
                setLabelTextProperties(textItem, label, true,
                                       QQuickText::HAlignment::AlignHCenter,
                                       QQuickText::VAlignment::AlignBottom);
            } else {
                setLabelTextProperties(textItem, label, true,
                                       QQuickText::HAlignment::AlignHCenter,
                                       QQuickText::VAlignment::AlignTop);
            }
            textItem->setHeight(rect.height());
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
