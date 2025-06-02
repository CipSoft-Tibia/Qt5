// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qareaseries.h>
#include <QtGraphs/qsplineseries.h>
#include <private/arearenderer_p.h>
#include <private/pointrenderer_p.h>
#include <private/axisrenderer_p.h>
#include <private/qabstractseries_p.h>
#include <private/qareaseries_p.h>
#include <private/qgraphsview_p.h>
#include <private/qxyseries_p.h>

QT_BEGIN_NAMESPACE

AreaRenderer::AreaRenderer(QGraphsView *graph)
    : QQuickItem(graph)
    , m_graph(graph)
{
    setFlag(QQuickItem::ItemHasContents);
    setClip(true);
    m_shape.setParentItem(this);
    m_shape.setPreferredRendererType(QQuickShape::CurveRenderer);
}

AreaRenderer::~AreaRenderer()
{
    qDeleteAll(m_groups);
}

void AreaRenderer::calculateRenderCoordinates(qreal origX,
                                              qreal origY,
                                              qreal *renderX,
                                              qreal *renderY) const
{
    *renderX = m_areaWidth * origX * m_maxHorizontal - m_horizontalOffset;
    *renderY = m_areaHeight - m_areaHeight * origY * m_maxVertical
               + m_verticalOffset;
}

void AreaRenderer::calculateAxisCoordinates(qreal origX,
                                            qreal origY,
                                            qreal *axisX,
                                            qreal *axisY) const
{
    *axisX = origX / m_areaWidth
             / m_maxHorizontal;
    *axisY = m_graph->m_axisRenderer->m_axisVerticalValueRange
             - origY / m_areaHeight / m_maxVertical;
}

void AreaRenderer::handlePolish(QAreaSeries *series)
{
    auto theme = m_graph->theme();
    if (!theme)
        return;

    if (!m_graph->m_axisRenderer)
        return;

    QXYSeries *upper = series->upperSeries();
    QXYSeries *lower = series->lowerSeries();

    if (!upper)
        return;

    if (!m_groups.contains(series)) {
        PointGroup *group = new PointGroup();
        group->series = series;
        m_groups.insert(series, group);

        group->shapePath = new QQuickShapePath(&m_shape);
        auto data = m_shape.data();
        data.append(&data, m_groups.value(series)->shapePath);
    }

    auto group = m_groups.value(series);

    if (upper->points().count() < 2 || (lower && lower->points().count() < 2)) {
        auto painterPath = group->painterPath;
        painterPath.clear();
        group->shapePath->setPath(painterPath);
        return;
    }

    m_areaWidth = width();
    m_areaHeight = height();

    m_maxVertical = m_graph->m_axisRenderer->m_axisVerticalValueRange > 0
                        ? 1.0 / m_graph->m_axisRenderer->m_axisVerticalValueRange
                        : 100.0;
    m_maxHorizontal = m_graph->m_axisRenderer->m_axisHorizontalValueRange > 0
                          ? 1.0 / m_graph->m_axisRenderer->m_axisHorizontalValueRange
                          : 100.0;
    m_verticalOffset = (m_graph->m_axisRenderer->m_axisVerticalMinValue
                        / m_graph->m_axisRenderer->m_axisVerticalValueRange)
                       * m_areaHeight;
    m_horizontalOffset = (m_graph->m_axisRenderer->m_axisHorizontalMinValue
                          / m_graph->m_axisRenderer->m_axisHorizontalValueRange)
                         * m_areaWidth;

    auto &painterPath = group->painterPath;
    painterPath.clear();

    if (group->colorIndex < 0) {
        group->colorIndex = m_graph->graphSeriesCount();
        m_graph->setGraphSeriesCount(group->colorIndex + 1);
    }

    const auto &seriesColors = theme->seriesColors();
    qsizetype index = group->colorIndex % seriesColors.size();
    QColor color = series->color().alpha() != 0
            ? series->color()
            : seriesColors.at(index);
    const auto &borderColors = theme->borderColors();
    index = group->colorIndex % borderColors.size();
    QColor borderColor = series->borderColor().alpha() != 0
            ? series->borderColor()
            : borderColors.at(index);

    if (series->isSelected()) {
        color = series->selectedColor().alpha() != 0 ? series->selectedColor() : color.lighter();
        borderColor = series->selectedBorderColor().alpha() != 0 ? series->selectedBorderColor()
                                                                 : borderColor.lighter();
    }

    qreal borderWidth = series->borderWidth();
    if (qFuzzyCompare(borderWidth, qreal(-1.0)))
        borderWidth = theme->borderWidth();

    group->shapePath->setStrokeWidth(borderWidth);
    group->shapePath->setStrokeColor(borderColor);
    group->shapePath->setFillColor(color);
    group->shapePath->setCapStyle(QQuickShapePath::CapStyle::SquareCap);

    auto &&upperPoints = upper->points();
    QList<QPointF> fittedPoints;
    if (upper->type() == QAbstractSeries::SeriesType::Spline)
        fittedPoints = qobject_cast<QSplineSeries *>(upper)->getControlPoints();

    int extraPointCount = lower ? 0 : 3;

    if (series->isVisible()) {
        for (int i = 0, j = 0; i < upperPoints.size() + extraPointCount; ++i, ++j) {
            qreal x, y;
            if (i == upperPoints.size())
                calculateRenderCoordinates(upperPoints[upperPoints.size() - 1].x(), 0, &x, &y);
            else if (i == upperPoints.size() + 1)
                calculateRenderCoordinates(upperPoints[0].x(), 0, &x, &y);
            else if (i == upperPoints.size() + 2)
                calculateRenderCoordinates(upperPoints[0].x(), upperPoints[0].y(), &x, &y);
            else
                calculateRenderCoordinates(upperPoints[i].x(), upperPoints[i].y(), &x, &y);

            if (i == 0) {
                painterPath.moveTo(x, y);
            } else {
                if (i < upper->points().size()
                    && upper->type() == QAbstractSeries::SeriesType::Spline) {
                    qreal x1, y1, x2, y2;
                    calculateRenderCoordinates(fittedPoints[j - 1].x(),
                                               fittedPoints[j - 1].y(),
                                               &x1,
                                               &y1);
                    calculateRenderCoordinates(fittedPoints[j].x(), fittedPoints[j].y(), &x2, &y2);

                    painterPath.cubicTo(x1, y1, x2, y2, x, y);
                    ++j;
                } else {
                    painterPath.lineTo(x, y);
                }
            }
        }
    }

    if (lower && series->isVisible()) {
        auto &&lowerPoints = lower->points();
        QList<QPointF> fittedPoints;
        if (lower->type() == QAbstractSeries::SeriesType::Spline)
            fittedPoints = qobject_cast<QSplineSeries *>(lower)->getControlPoints();

        for (int i = 0, j = 0; i < lowerPoints.size(); ++i, ++j) {
            qreal x, y;
            calculateRenderCoordinates(lowerPoints[lowerPoints.size() - 1 - i].x(),
                                       lowerPoints[lowerPoints.size() - 1 - i].y(),
                                       &x,
                                       &y);

            if (i > 0 && lower->type() == QAbstractSeries::SeriesType::Spline) {
                qreal x1, y1, x2, y2;
                calculateRenderCoordinates(fittedPoints[fittedPoints.size() - 1 - j + 1].x(),
                                           fittedPoints[fittedPoints.size() - 1 - j + 1].y(),
                                           &x1,
                                           &y1);
                calculateRenderCoordinates(fittedPoints[fittedPoints.size() - 1 - j].x(),
                                           fittedPoints[fittedPoints.size() - 1 - j].y(),
                                           &x2,
                                           &y2);

                painterPath.cubicTo(x1, y1, x2, y2, x, y);
                ++j;
            } else {
                painterPath.lineTo(x, y);
            }
        }

        qreal x, y;
        calculateRenderCoordinates(upperPoints[0].x(), upperPoints[0].y(), &x, &y);
        painterPath.lineTo(x, y);
    }

    group->shapePath->setPath(painterPath);

    QList<QLegendData> legendDataList = {{color, borderColor, series->name()}};
    series->d_func()->setLegendData(legendDataList);
}

void AreaRenderer::afterPolish(QList<QAbstractSeries *> &cleanupSeries)
{
    for (auto series : cleanupSeries) {
        auto areaSeries = qobject_cast<QAreaSeries *>(series);
        if (areaSeries && m_groups.contains(areaSeries)) {
            auto group = m_groups.value(areaSeries);

            auto painterPath = group->painterPath;
            painterPath.clear();
            group->shapePath->setPath(painterPath);

            delete group;
            m_groups.remove(areaSeries);
        }
    }
}

void AreaRenderer::afterUpdate(QList<QAbstractSeries *> &cleanupSeries)
{
    Q_UNUSED(cleanupSeries);
}

void AreaRenderer::updateSeries(QAreaSeries *series)
{
    Q_UNUSED(series);
}

// Point inside triangle code from
// https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
float sign(QPoint p1, QPoint p2, QPoint p3)
{
    return (p1.x() - p3.x()) * (p2.y() - p3.y()) - (p2.x() - p3.x()) * (p1.y() - p3.y());
}

bool pointInTriangle(QPoint pt, QPoint v1, QPoint v2, QPoint v3)
{
    float d1, d2, d3;
    bool hasNeg, hasPos;

    d1 = sign(pt, v1, v2);
    d2 = sign(pt, v2, v3);
    d3 = sign(pt, v3, v1);

    hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(hasNeg && hasPos);
}

bool AreaRenderer::pointInArea(QPoint pt, QAreaSeries *series) const
{
    QList<QPointF> upperPoints = series->upperSeries()->points();
    QList<QPointF> lowerPoints;

    if (series->lowerSeries())
        lowerPoints = series->lowerSeries()->points();

    QList<QPointF> *firstPoints = &upperPoints;
    if (lowerPoints.size() > upperPoints.size())
        firstPoints = &lowerPoints;

    for (int i = 0; i < firstPoints->size() - 1; ++i) {
        qreal x1, y1, x2, y2, x3, y3, x4, y4;
        calculateRenderCoordinates((*firstPoints)[i].x(), (*firstPoints)[i].y(), &x1, &y1);
        calculateRenderCoordinates((*firstPoints)[i + 1].x(), (*firstPoints)[i + 1].y(), &x2, &y2);

        bool needSecondTriangleTest = true;
        if (series->lowerSeries()) {
            QList<QPointF> *secondPoints = &lowerPoints;
            if (lowerPoints.size() > upperPoints.size())
                secondPoints = &upperPoints;

            qsizetype firstIndex = i;
            qsizetype secondIndex = i + 1;

            if (firstIndex >= secondPoints->size())
                firstIndex = secondPoints->size() - 1;
            if (secondIndex >= secondPoints->size())
                needSecondTriangleTest = false;

            calculateRenderCoordinates((*secondPoints)[firstIndex].x(),
                                       (*secondPoints)[firstIndex].y(),
                                       &x3,
                                       &y3);

            if (needSecondTriangleTest) {
                calculateRenderCoordinates((*secondPoints)[secondIndex].x(),
                                           (*secondPoints)[secondIndex].y(),
                                           &x4,
                                           &y4);
            } else {
                x4 = 0.0;
                y4 = 0.0;
            }
        } else {
            calculateRenderCoordinates(upperPoints[i].x(), 0, &x3, &y3);
            calculateRenderCoordinates(upperPoints[i + 1].x(), 0, &x4, &y4);
        }

        QPoint point1(x1, y1);
        QPoint point2(x2, y2);
        QPoint point3(x3, y3);
        QPoint point4(x4, y4);

        if (pointInTriangle(pt, point1, point2, point3)
            || (needSecondTriangleTest && pointInTriangle(pt, point2, point3, point4))) {
            return true;
        }
    }

    return false;
}

bool AreaRenderer::handleMousePress(QMouseEvent *event)
{
    bool handled = false;
    for (auto &&group : m_groups) {
        if (!group->series->isSelectable() || !group->series->isVisible())
            continue;

        if (!group->series->upperSeries() || group->series->upperSeries()->count() < 2)
            continue;

        if (group->series->lowerSeries() && group->series->lowerSeries()->count() < 2)
            continue;

        if (pointInArea(event->pos(), group->series)) {
            group->series->setSelected(!group->series->isSelected());
            handled = true;
        }
    }
    return handled;
}

bool AreaRenderer::handleHoverMove(QHoverEvent *event)
{
    bool handled = false;
    const QPointF &position = event->position();

    for (auto &&group : m_groups) {
        if (!group->series->isHoverable() || !group->series->isVisible())
            continue;

        if (!group->series->upperSeries() || group->series->upperSeries()->count() < 2)
            continue;

        if (group->series->lowerSeries() && group->series->lowerSeries()->count() < 2)
            continue;

        const QString &name = group->series->name();

        bool hovering = false;
        if (pointInArea(position.toPoint(), group->series)) {
            qreal x, y;
            calculateAxisCoordinates(position.x(), position.y(), &x, &y);

            if (!group->hover) {
                group->hover = true;
                emit group->series->hoverEnter(name, position, QPointF(x, y));
            }

            emit group->series->hover(name, position, QPointF(x, y));
            hovering = true;
            handled = true;
        }

        if (!hovering && group->hover) {
            group->hover = false;
            emit group->series->hoverExit(name, position);
            handled = true;
        }
    }
    return handled;
}

QT_END_NAMESPACE
