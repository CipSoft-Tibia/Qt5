// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qlineseries.h>
#include <QtGraphs/qscatterseries.h>
#include <QtGraphs/qsplineseries.h>
#include <private/pointrenderer_p.h>
#include <private/axisrenderer_p.h>
#include <private/qabstractseries_p.h>
#include <private/qgraphsview_p.h>
#include <private/qxyseries_p.h>

QT_BEGIN_NAMESPACE

static const char *TAG_POINT_COLOR = "pointColor";
static const char *TAG_POINT_BORDER_COLOR = "pointBorderColor";
static const char *TAG_POINT_BORDER_WIDTH = "pointBorderWidth";
static const char *TAG_POINT_SELECTED_COLOR = "pointSelectedColor";
static const char *TAG_POINT_SELECTED = "pointSelected";
static const char *TAG_POINT_VALUE_X = "pointValueX";
static const char *TAG_POINT_VALUE_Y = "pointValueY";

PointRenderer::PointRenderer(QGraphsView *graph)
    : QQuickItem(graph)
    , m_graph(graph)
{
    setFlag(QQuickItem::ItemHasContents);
    setClip(true);
    m_shape.setParentItem(this);
    m_shape.setPreferredRendererType(QQuickShape::CurveRenderer);

    const QString qmlData = QLatin1StringView(R"QML(
        import QtQuick;

        Rectangle {
            property bool pointSelected
            property color pointColor
            property color pointBorderColor
            property color pointSelectedColor
            property real pointBorderWidth
            color: pointSelected ? pointSelectedColor : pointColor
            border.color: pointBorderColor
            border.width: pointBorderWidth
            width: %1
            height: %1
        }
    )QML").arg(QString::number((int) defaultSize()));
    m_tempMarker = new QQmlComponent(qmlEngine(m_graph), this);
    m_tempMarker->setData(qmlData.toUtf8(), QUrl());
}

PointRenderer::~PointRenderer()
{
    qDeleteAll(m_groups);
}

qreal PointRenderer::defaultSize(QXYSeries *series)
{
    qreal size = 16.0;
    if (series != nullptr) {
        if (auto line = qobject_cast<QLineSeries *>(series))
            size = qMax(size, line->width());
        else if (auto spline = qobject_cast<QSplineSeries *>(series))
            size = qMax(size, spline->width());
    }
    return size;
}

void PointRenderer::calculateRenderCoordinates(
    AxisRenderer *axisRenderer, qreal origX, qreal origY, qreal *renderX, qreal *renderY)
{
    auto flipX = axisRenderer->m_axisHorizontalMaxValue < axisRenderer->m_axisHorizontalMinValue
                     ? -1
                     : 1;
    auto flipY = axisRenderer->m_axisVerticalMaxValue < axisRenderer->m_axisVerticalMinValue ? -1
                                                                                             : 1;

    *renderX = m_areaWidth * flipX * origX * m_maxHorizontal - m_horizontalOffset;
    *renderY = m_areaHeight - m_areaHeight * flipY * origY * m_maxVertical
               + m_verticalOffset;
}

void PointRenderer::reverseRenderCoordinates(
    AxisRenderer *axisRenderer, qreal renderX, qreal renderY, qreal *origX, qreal *origY)
{
    auto flipX = axisRenderer->m_axisHorizontalMaxValue < axisRenderer->m_axisHorizontalMinValue
                     ? -1
                     : 1;
    auto flipY = axisRenderer->m_axisVerticalMaxValue < axisRenderer->m_axisVerticalMinValue ? -1
                                                                                             : 1;

    *origX = (renderX + m_horizontalOffset) / (m_areaWidth * flipX * m_maxHorizontal);
    *origY = (renderY - m_areaHeight - m_verticalOffset)
             / (-1 * m_areaHeight * flipY * m_maxVertical);
}

PointRenderer::SeriesStyle PointRenderer::getSeriesStyle(PointGroup *group)
{
    auto theme = m_graph->theme();

    const auto &seriesColors = theme->seriesColors();
    const auto &borderColors = theme->borderColors();

    qsizetype index = group->colorIndex % seriesColors.size();
    QColor color = group->series->color().alpha() != 0 ? group->series->color() : seriesColors.at(index);

    QColor selectedColor = group->series->selectedColor().alpha() != 0
                               ? group->series->selectedColor()
                               : m_graph->theme()->singleHighlightColor();

    index = group->colorIndex % borderColors.size();
    QColor borderColor = borderColors.at(index);
    qreal borderWidth = theme->borderWidth();

    return {
        color,
        selectedColor,
        borderColor,
        borderWidth
    };
}

void PointRenderer::updatePointDelegate(
    QXYSeries *series, PointGroup *group, qsizetype pointIndex, qreal x, qreal y)
{
    const auto style = getSeriesStyle(group);

    auto marker = group->markers[pointIndex];
    auto &rect = group->rects[pointIndex];

    if (marker->property(TAG_POINT_SELECTED).isValid())
        marker->setProperty(TAG_POINT_SELECTED, series->isPointSelected(pointIndex));
    if (marker->property(TAG_POINT_COLOR).isValid())
        marker->setProperty(TAG_POINT_COLOR, style.color);
    if (marker->property(TAG_POINT_BORDER_COLOR).isValid())
        marker->setProperty(TAG_POINT_BORDER_COLOR, style.borderColor);
    if (marker->property(TAG_POINT_BORDER_WIDTH).isValid())
        marker->setProperty(TAG_POINT_BORDER_WIDTH, style.borderWidth);
    if (marker->property(TAG_POINT_SELECTED_COLOR).isValid())
        marker->setProperty(TAG_POINT_SELECTED_COLOR, style.selectedColor);
    const auto point = series->points().at(pointIndex);
    if (marker->property(TAG_POINT_VALUE_X).isValid())
        marker->setProperty(TAG_POINT_VALUE_X, point.x());
    if (marker->property(TAG_POINT_VALUE_Y).isValid())
        marker->setProperty(TAG_POINT_VALUE_Y, point.y());

    marker->setX(x - marker->width() / 2.0);
    marker->setY(y - marker->height() / 2.0);
    marker->setVisible(true);

    rect = QRectF(x - marker->width() / 2.0,
                  y - marker->height() / 2.0,
                  marker->width(),
                  marker->height());
}

void PointRenderer::hidePointDelegates(QXYSeries *series)
{
    auto *group = m_groups.value(series);
    if (group->currentMarker) {
        for (int i = 0; i < group->markers.size(); ++i) {
            auto *marker = group->markers[i];
            marker->setVisible(false);
        }
    }
    group->rects.clear();
}

void PointRenderer::updateLegendData(QXYSeries *series, QLegendData &legendData)
{
    QList<QLegendData> legendDataList = {legendData};
    series->d_func()->setLegendData(legendDataList);
}

void PointRenderer::updateScatterSeries(QScatterSeries *series, QLegendData &legendData)
{
    auto group = m_groups.value(series);
    const auto style = getSeriesStyle(group);

    if (series->isVisible()) {
        auto &&points = series->points();
        group->rects.resize(points.size());
        for (int i = 0; i < points.size(); ++i) {
            qreal x, y;
            calculateRenderCoordinates(m_graph->m_axisRenderer, points[i].x(), points[i].y(), &x, &y);
            y *= series->valuesMultiplier();
            if (group->currentMarker) {
                updatePointDelegate(series, group, i, x, y);
            } else {
                auto &rect = group->rects[i];
                qreal size = defaultSize(series);
                rect = QRectF(x - size / 2.0, y - size / 2.0, size, size);
            }
        }
    } else {
        hidePointDelegates(series);
    }

    legendData = { style.color, style.borderColor, series->name() };
}

void PointRenderer::updateLineSeries(QLineSeries *series, QLegendData &legendData)
{
    auto group = m_groups.value(series);
    const auto style = getSeriesStyle(group);

    group->shapePath->setStrokeColor(style.color);
    group->shapePath->setStrokeWidth(series->width());
    group->shapePath->setFillColor(QColorConstants::Transparent);

    Qt::PenCapStyle capStyle = series->capStyle();
    if (capStyle == Qt::PenCapStyle::SquareCap)
        group->shapePath->setCapStyle(QQuickShapePath::CapStyle::SquareCap);
    else if (capStyle == Qt::PenCapStyle::FlatCap)
        group->shapePath->setCapStyle(QQuickShapePath::CapStyle::FlatCap);
    else if (capStyle == Qt::PenCapStyle::RoundCap)
        group->shapePath->setCapStyle(QQuickShapePath::CapStyle::RoundCap);

    auto &painterPath = group->painterPath;
    painterPath.clear();

    if (series->isVisible()) {
        auto &&points = series->points();
        group->rects.resize(points.size());
        for (int i = 0; i < points.size(); ++i) {
            qreal x, y;
            calculateRenderCoordinates(m_graph->m_axisRenderer, points[i].x(), points[i].y(), &x, &y);
            y *= series->valuesMultiplier();
            if (i == 0)
                painterPath.moveTo(x, y);
            else
                painterPath.lineTo(x, y);

            if (group->currentMarker) {
                updatePointDelegate(series, group, i, x, y);
            } else {
                auto &rect = group->rects[i];
                qreal size = defaultSize(series);
                rect = QRectF(x - size / 2.0, y - size / 2.0, size, size);
            }
        }
    } else {
        hidePointDelegates(series);
    }
    group->shapePath->setPath(painterPath);
    legendData = { style.color, style.borderColor, series->name() };
}

void PointRenderer::updateSplineSeries(QSplineSeries *series, QLegendData &legendData)
{
    auto group = m_groups.value(series);
    const auto style = getSeriesStyle(group);

    group->shapePath->setStrokeColor(style.color);
    group->shapePath->setStrokeWidth(series->width());
    group->shapePath->setFillColor(QColorConstants::Transparent);

    Qt::PenCapStyle capStyle = series->capStyle();
    if (capStyle == Qt::PenCapStyle::SquareCap)
        group->shapePath->setCapStyle(QQuickShapePath::CapStyle::SquareCap);
    else if (capStyle == Qt::PenCapStyle::FlatCap)
        group->shapePath->setCapStyle(QQuickShapePath::CapStyle::FlatCap);
    else if (capStyle == Qt::PenCapStyle::RoundCap)
        group->shapePath->setCapStyle(QQuickShapePath::CapStyle::RoundCap);

    auto &painterPath = group->painterPath;
    painterPath.clear();

    if (series->isVisible()) {
        auto &&points = series->points();
        group->rects.resize(points.size());
        auto fittedPoints = series->getControlPoints();

        for (int i = 0, j = 0; i < points.size(); ++i, ++j) {
            qreal x, y;
            calculateRenderCoordinates(m_graph->m_axisRenderer, points[i].x(), points[i].y(), &x, &y);

            qreal valuesMultiplier = series->valuesMultiplier();
            y *= valuesMultiplier;
            if (i == 0) {
                painterPath.moveTo(x, y);
            } else {
                qreal x1, y1, x2, y2;
                calculateRenderCoordinates(m_graph->m_axisRenderer,
                                           fittedPoints[j - 1].x(),
                                           fittedPoints[j - 1].y(),
                                           &x1,
                                           &y1);
                calculateRenderCoordinates(m_graph->m_axisRenderer,
                                           fittedPoints[j].x(),
                                           fittedPoints[j].y(),
                                           &x2,
                                           &y2);

                y1 *= valuesMultiplier;
                y2 *= valuesMultiplier;
                painterPath.cubicTo(x1, y1, x2, y2, x, y);
                ++j;
            }

            if (group->currentMarker) {
                updatePointDelegate(series, group, i, x, y);
            } else {
                auto &rect = group->rects[i];
                qreal size = defaultSize(series);
                rect = QRectF(x - size / 2.0, y - size / 2.0, size, size);
            }
        }
    } else {
        hidePointDelegates(series);
    }

    group->shapePath->setPath(painterPath);
    legendData = { style.color, style.borderColor, series->name() };
}

void PointRenderer::handlePolish(QXYSeries *series)
{
    auto theme = m_graph->theme();
    if (!theme)
        return;

    if (!m_graph->m_axisRenderer)
        return;

    if (series->points().isEmpty()) {
        auto group = m_groups.value(series);

        if (group) {
            if (group->shapePath) {
                auto &painterPath = group->painterPath;
                painterPath.clear();
                group->shapePath->setPath(painterPath);
            }

            for (auto m : group->markers)
                m->deleteLater();

            group->markers.clear();
        }

        return;
    }

    if (width() <= 0 || height() <= 0)
        return;

    m_areaWidth = width();
    m_areaHeight = height();

    m_maxVertical = m_graph->m_axisRenderer->m_axisVerticalValueRange > 0
                        ? 1.0 / m_graph->m_axisRenderer->m_axisVerticalValueRange
                        : 100.0;
    m_maxHorizontal = m_graph->m_axisRenderer->m_axisHorizontalValueRange > 0
                          ? 1.0 / m_graph->m_axisRenderer->m_axisHorizontalValueRange
                          : 100.0;

    auto vmin = m_graph->m_axisRenderer->m_axisVerticalMinValue
                        > m_graph->m_axisRenderer->m_axisVerticalMaxValue
                    ? std::abs(m_graph->m_axisRenderer->m_axisVerticalMinValue)
                    : m_graph->m_axisRenderer->m_axisVerticalMinValue;

    m_verticalOffset = (vmin / m_graph->m_axisRenderer->m_axisVerticalValueRange) * m_areaHeight;

    auto hmin = m_graph->m_axisRenderer->m_axisHorizontalMinValue
                        > m_graph->m_axisRenderer->m_axisHorizontalMaxValue
                    ? std::abs(m_graph->m_axisRenderer->m_axisHorizontalMinValue)
                    : m_graph->m_axisRenderer->m_axisHorizontalMinValue;

    m_horizontalOffset = (hmin / m_graph->m_axisRenderer->m_axisHorizontalValueRange) * m_areaWidth;

    if (!m_groups.contains(series)) {
        PointGroup *group = new PointGroup();
        group->series = series;
        m_groups.insert(series, group);

        if (series->type() != QAbstractSeries::SeriesType::Scatter) {
            group->shapePath = new QQuickShapePath(&m_shape);
            group->shapePath->setAsynchronous(true);
            auto data = m_shape.data();
            data.append(&data, m_groups.value(series)->shapePath);
        }
    }

    auto group = m_groups.value(series);

    qsizetype pointCount = series->points().size();

    if ((series->type() == QAbstractSeries::SeriesType::Scatter) && !series->pointDelegate())
        group->currentMarker = m_tempMarker;
    else if (series->pointDelegate())
        group->currentMarker = series->pointDelegate();

    if (group->currentMarker != group->previousMarker) {
        for (auto &&marker : group->markers)
            marker->deleteLater();
        group->markers.clear();
    }
    group->previousMarker = group->currentMarker;

    if (group->currentMarker) {
        qsizetype markerCount = group->markers.size();
        if (markerCount < pointCount) {
            for (qsizetype i = markerCount; i < pointCount; ++i) {
                QQuickItem *item = qobject_cast<QQuickItem *>(
                    group->currentMarker->create(group->currentMarker->creationContext()));
                item->setParent(this);
                item->setParentItem(this);
                group->markers << item;
            }
        } else if (markerCount > pointCount) {
            for (qsizetype i = pointCount; i < markerCount; ++i)
                group->markers[i]->deleteLater();
            group->markers.resize(pointCount);
        }
    } else if (group->markers.size() > 0) {
        for (auto &&marker : group->markers)
            marker->deleteLater();
        group->markers.clear();
    }

    if (group->colorIndex < 0) {
        group->colorIndex = m_graph->graphSeriesCount();
        m_graph->setGraphSeriesCount(group->colorIndex + 1);
    }

    QLegendData legendData;
    if (auto scatter = qobject_cast<QScatterSeries *>(series))
        updateScatterSeries(scatter, legendData);
    else if (auto line = qobject_cast<QLineSeries *>(series))
        updateLineSeries(line, legendData);
    else if (auto spline = qobject_cast<QSplineSeries *>(series))
        updateSplineSeries(spline, legendData);

    updateLegendData(series, legendData);
}

void PointRenderer::afterPolish(QList<QAbstractSeries *> &cleanupSeries)
{
    for (auto series : cleanupSeries) {
        auto xySeries = qobject_cast<QXYSeries *>(series);
        if (xySeries && m_groups.contains(xySeries)) {
            auto group = m_groups.value(xySeries);

            for (auto marker : group->markers)
                marker->deleteLater();

            if (group->shapePath) {
                auto painterPath = group->painterPath;
                painterPath.clear();
                group->shapePath->setPath(painterPath);
            }

            delete group;
            m_groups.remove(xySeries);
        }
    }
}

void PointRenderer::updateSeries(QXYSeries *series)
{
    Q_UNUSED(series);
}

void PointRenderer::afterUpdate(QList<QAbstractSeries *> &cleanupSeries)
{
    Q_UNUSED(cleanupSeries);
}

bool PointRenderer::handleMouseMove(QMouseEvent *event)
{
    if (!m_pressedGroup || !m_pressedGroup->series->isVisible())
        return false;

    if (m_pointPressed && m_pressedGroup->series->isDraggable()) {
        float w = width();
        float h = height();
        double maxVertical = m_graph->m_axisRenderer->m_axisVerticalValueRange > 0
                                 ? 1.0 / m_graph->m_axisRenderer->m_axisVerticalValueRange
                                 : 100.0;
        double maxHorizontal = m_graph->m_axisRenderer->m_axisHorizontalValueRange > 0
                                   ? 1.0 / m_graph->m_axisRenderer->m_axisHorizontalValueRange
                                   : 100.0;

        QPoint delta = m_pressStart - event->pos();

        qreal deltaX = -delta.x() / w / maxHorizontal;
        qreal deltaY = delta.y() / h / maxVertical;

        QPointF point = m_pressedGroup->series->at(m_pressedPointIndex) + QPointF(deltaX, deltaY);
        m_pressedGroup->series->replace(m_pressedPointIndex, point);

        m_pressStart = event->pos();
        m_pointDragging = true;

        return true;
    }
    return false;
}

bool PointRenderer::handleMousePress(QMouseEvent *event)
{
    bool handled = false;
    for (auto &&group : m_groups) {
        if (!group->series->isVisible())
            continue;

        if (!group->series->isSelectable() && !group->series->isDraggable())
            continue;

        int index = 0;
        for (auto &&rect : group->rects) {
            if (rect.contains(event->pos())) {
                m_pointPressed = true;
                m_pressStart = event->pos();
                m_pressedGroup = group;
                m_pressedPointIndex = index;
                handled = true;
            }
            index++;
        }
    }
    return handled;
}

bool PointRenderer::handleMouseRelease(QMouseEvent *event)
{
    bool handled = false;
    if (!m_pointDragging && m_pointPressed && m_pressedGroup
        && m_pressedGroup->series->isSelectable() && m_pressedGroup->series->isVisible()) {
        if (m_pressedGroup->rects[m_pressedPointIndex].contains(event->pos())) {
            if (m_pressedGroup->series->isPointSelected(m_pressedPointIndex)) {
                m_pressedGroup->series->deselectPoint(m_pressedPointIndex);
            } else {
                m_pressedGroup->series->selectPoint(m_pressedPointIndex);
            }
            handled = true;
        }
    }
    m_pointPressed = false;
    m_pointDragging = false;
    return handled;
}

bool PointRenderer::handleHoverMove(QHoverEvent *event)
{
    bool handled = false;
    const QPointF &position = event->position();

    for (auto &&group : m_groups) {
        if (!group->series->isHoverable() || !group->series->isVisible())
            continue;

        auto axisRenderer = group->series->graph()->m_axisRenderer;
        bool isHNegative = axisRenderer->m_axisHorizontalMaxValue
                           < axisRenderer->m_axisHorizontalMinValue;
        bool isVNegative = axisRenderer->m_axisVerticalMaxValue
                           < axisRenderer->m_axisVerticalMinValue;

        if (group->series->type() == QAbstractSeries::SeriesType::Scatter) {
            const QString &name = group->series->name();

            bool hovering = false;

            int index = 0;
            for (auto &&rect : group->rects) {
                if (rect.contains(position.toPoint())) {
                    if (!group->hover) {
                        group->hover = true;
                        emit group->series->hoverEnter(name, position, group->series->at(index));
                    }
                    emit group->series->hover(name, position, group->series->at(index));
                    hovering = true;
                }
                index++;
            }

            if (!hovering && group->hover) {
                group->hover = false;
                emit group->series->hoverExit(name, position);
            }
        } else {
            const qreal x0 = event->position().x();
            const qreal y0 = event->position().y();

            const qreal hoverSize = defaultSize(group->series) / 2.0;
            const QString &name = group->series->name();
            auto &&points = group->series->points();
            // True when line, false when spline
            const bool isLine = group->series->type() == QAbstractSeries::SeriesType::Line;
            if (points.size() >= 2) {
                bool hovering = false;
                auto subpath = group->painterPath.toSubpathPolygons();

                for (int i = 0; i < points.size() - 1; i++) {
                    qreal x1, y1, x2, y2;
                    if (i == 0) {
                        auto element1 = group->painterPath.elementAt(0);
                        auto element2 = group->painterPath.elementAt(isLine ? 1 : 3);
                        x1 = isHNegative ? element2.x : element1.x;
                        y1 = element1.y;
                        x2 = isHNegative ? element1.x : element2.x;
                        y2 = element2.y;
                    } else {
                        bool n = isVNegative | isHNegative;
                        // Each Spline (cubicTo) has 3 elements where third one is the x & y.
                        // So content of elements are:
                        // With Spline:
                        // [0] : MoveToElement
                        // [1] : 1. CurveToElement (c1x, c1y)
                        // [2] : 1. CurveToDataElement (c2x, c2y)
                        // [3] : 1. CurveToDataElement (x, y)
                        // [4] : 2. CurveToElement (c1x, c1y)
                        // ...
                        // With Line:
                        // [0] : MoveToElement
                        // [1] : 1. LineToElement (x, y)
                        // [2] : 2. LineToElement (x, y)
                        // ...
                        int element1Index = n ? (i + 1) : i;
                        int element2Index = n ? i : (i + 1);
                        element1Index = isLine ? element1Index : element1Index * 3;
                        element2Index = isLine ? element2Index : element2Index * 3;
                        auto element1 = group->painterPath.elementAt(element1Index);
                        auto element2 = group->painterPath.elementAt(element2Index);
                        x1 = element1.x;
                        y1 = element1.y;
                        x2 = element2.x;
                        y2 = element2.y;
                    }

                    if (isLine) {
                        qreal denominator = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
                        qreal hoverDistance = qAbs((x2 - x1) * (y1 - y0) - (x1 - x0) * (y2 - y1))
                                              / qSqrt(denominator);

                        if (hoverDistance < hoverSize) {
                            qreal alpha = 0;
                            qreal extrapolation = 0;
                            if (x2 - x1 >= y2 - y1) {
                                if (x2 - x1 != 0) {
                                    alpha = ((x2 - x1) - (x0 - x1)) / qAbs(x2 - x1);
                                    extrapolation = hoverSize / qAbs(x2 - x1);
                                }
                            } else {
                                if (y2 - y1 != 0) {
                                    alpha = ((y2 - y1) - (y0 - y1)) / qAbs(y2 - y1);
                                    extrapolation = hoverSize / qAbs(y2 - y1);
                                }
                            }

                            if (alpha >= -extrapolation && alpha <= 1.0 + extrapolation) {
                                bool n = isVNegative | isHNegative;

                                const QPointF &point1 = points[n ? i + 1 : i];
                                const QPointF &point2 = points[n ? i : i + 1];

                                QPointF point = (point2 * (1.0 - alpha)) + (point1 * alpha);

                                if (!group->hover) {
                                    group->hover = true;
                                    emit group->series->hoverEnter(name, position, point);
                                }

                                emit group->series->hover(name, position, point);
                                hovering = true;
                                handled = true;
                            }
                        }
                    } else { // Spline
                        auto segments = subpath[0];

                        for (auto it = segments.begin(); it != segments.end(); ++it) {
                            if (std::next(it, 1) == segments.end())
                                break;

                            auto it2 = std::next(it, 1);

                            qreal denominator = (it2->x() - it->x()) * (it2->x() - it->x())
                                                + (it2->y() - it->y()) * (it2->y() - it->y());
                            qreal hoverDistance = qAbs((it2->x() - it->x()) * (it->y() - y0)
                                                       - (it->x() - x0) * (it2->y() - it->y()))
                                                  / qSqrt(denominator);

                            if (hoverDistance < hoverSize) {
                                qreal alpha = 0;
                                qreal extrapolation = 0;
                                if (it2->x() - it->x() >= it2->y() - it->y()) {
                                    if (it2->x() - it->x() != 0) {
                                        alpha = ((it2->x() - it->x()) - (x0 - it->x()))
                                                / qAbs(it2->x() - it->x());
                                        extrapolation = hoverSize / qAbs(it2->x() - it->x());
                                    }
                                } else {
                                    if (it2->y() - it->y() != 0) {
                                        alpha = ((it2->y() - it->y()) - (y0 - it->y()))
                                                / qAbs(it2->y() - it->y());
                                        extrapolation = hoverSize / qAbs(it2->y() - it->y());
                                    }
                                }

                                if (alpha >= -extrapolation && alpha <= 1.0 + extrapolation) {
                                    qreal cx1, cy1, cx2, cy2;

                                    reverseRenderCoordinates(axisRenderer,
                                                             it->x(),
                                                             it->y(),
                                                             &cx1,
                                                             &cy1);
                                    reverseRenderCoordinates(axisRenderer,
                                                             it2->x(),
                                                             it2->y(),
                                                             &cx2,
                                                             &cy2);

                                    const QPointF &point1 = {cx1, cy1};
                                    const QPointF &point2 = {cx2, cy2};

                                    QPointF point = (point2 * (1.0 - alpha)) + (point1 * alpha);

                                    if (!group->hover) {
                                        group->hover = true;
                                        emit group->series->hoverEnter(name, position, point);
                                    }

                                    emit group->series->hover(name, position, point);
                                    hovering = true;
                                    handled = true;
                                }
                            }
                        }
                    }
                }

                if (!hovering && group->hover) {
                    group->hover = false;
                    emit group->series->hoverExit(name, position);
                    handled = true;
                }
            }
        }
    }
    return handled;
}

QT_END_NAMESPACE
