// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick/private/qquickrectangle_p.h>
#include <QtGraphs/qbarseries.h>
#include <QtGraphs/qbarset.h>
#include <private/barsrenderer_p.h>
#include <private/axisrenderer_p.h>
#include <private/qbarseries_p.h>
#include <private/qgraphsview_p.h>

QT_BEGIN_NAMESPACE

static const char* TAG_BAR_COLOR = "barColor";
static const char* TAG_BAR_BORDER_COLOR = "barBorderColor";
static const char* TAG_BAR_BORDER_WIDTH = "barBorderWidth";
static const char* TAG_BAR_SELECTED = "barSelected";
static const char* TAG_BAR_VALUE = "barValue";
static const char* TAG_BAR_LABEL = "barLabel";

BarsRenderer::BarsRenderer(QGraphsView *graph)
    : QQuickItem(graph)
    , m_graph(graph)
{
    setFlag(QQuickItem::ItemHasContents);
    setClip(true);
}

BarsRenderer::~BarsRenderer() {}

// Returns color in this order:
// 1) QBarSet::color if that is defined (alpha > 0).
// 2) QBarSeries::seriesColors at index if that is defined.
// 3) QGraphsTheme::seriesColors at index.
// 4) Black if seriesColors is empty.
QColor BarsRenderer::getSetColor(QBarSeries *series, QBarSet *set, qsizetype barSeriesIndex)
{
    const auto &seriesColors = !series->seriesColors().isEmpty()
            ? series->seriesColors() : m_graph->theme()->seriesColors();
    if (seriesColors.isEmpty())
        return QColorConstants::Black;
    qsizetype index = m_colorIndex + barSeriesIndex;
    index = index % seriesColors.size();
    QColor color = set->color().alpha() != 0
            ? set->color()
            : seriesColors.at(index);
    return color;
}

QColor BarsRenderer::getSetSelectedColor(QBarSeries *series, QBarSet *set)
{
    Q_UNUSED(series);
    auto theme = m_graph->theme();
    QColor color = set->selectedColor().alpha() != 0
            ? set->selectedColor()
            : theme->singleHighlightColor();
    return color;
}

QColor BarsRenderer::getSetBorderColor(QBarSeries *series, QBarSet *set, qsizetype barSeriesIndex)
{
    const auto &borderColors = !series->borderColors().isEmpty()
            ? series->borderColors() : m_graph->theme()->borderColors();
    if (borderColors.isEmpty())
        return QColorConstants::Black;
    qsizetype index = m_colorIndex + barSeriesIndex;
    index = index % borderColors.size();
    QColor color = set->borderColor().alpha() != 0
            ? set->borderColor()
            : borderColors.at(index);
    return color;
}

qreal BarsRenderer::getSetBorderWidth(QBarSeries *series, QBarSet *set)
{
    Q_UNUSED(series);
    auto theme = m_graph->theme();
    qreal borderWidth = set->borderWidth();
    if (qFuzzyCompare(borderWidth, qreal(-1.0)))
        borderWidth = theme->borderWidth();
    return borderWidth;
}

QString BarsRenderer::generateLabelText(QBarSeries *series, qreal value)
{
    static const QString valueTag(QLatin1String("@value"));
    QString valueString = QString::number(value, 'f', series->labelsPrecision());
    QString valueLabel;
    if (series->labelsFormat().isEmpty()) {
        valueLabel = valueString;
    } else {
        valueLabel = series->labelsFormat();
        valueLabel.replace(valueTag, valueString);
    }
    return valueLabel;
}

void BarsRenderer::positionLabelItem(QBarSeries *series, QQuickText *textItem, const BarSeriesData &d)
{
    auto pos = series->labelsPosition();
    const bool vertical = m_graph->orientation() == Qt::Orientation::Vertical;
    const float w = textItem->contentWidth() + series->labelsMargin() * 2;
    const float h = textItem->contentHeight() + series->labelsMargin() * 2;
    textItem->setWidth(w);
    textItem->setHeight(h);
    textItem->setHAlign(QQuickText::HAlignment::AlignHCenter);
    textItem->setVAlign(QQuickText::VAlignment::AlignVCenter);
    if (pos == QBarSeries::LabelsPosition::Center) {
        textItem->setX(d.rect.x() + d.rect.width() * 0.5 - w * 0.5);
        textItem->setY(d.rect.y() + d.rect.height() * 0.5 - h * 0.5);
    } else if (pos == QBarSeries::LabelsPosition::InsideEnd) {
        if (vertical) {
            textItem->setX(d.rect.x() + d.rect.width() * 0.5 - w * 0.5);
            textItem->setY(d.rect.y());
        } else {
            textItem->setX(d.rect.x() + d.rect.width() - w);
            textItem->setY(d.rect.y() + d.rect.height() * 0.5 - h * 0.5);
        }
    } else if (pos == QBarSeries::LabelsPosition::InsideBase) {
        if (vertical) {
            textItem->setX(d.rect.x() + d.rect.width() * 0.5 - w * 0.5);
            textItem->setY(d.rect.y() + d.rect.height() - h);
        } else {
            textItem->setX(d.rect.x());
            textItem->setY(d.rect.y() + d.rect.height() * 0.5 - h * 0.5);
        }
    } else {
        // OutsideEnd
        if (vertical) {
            textItem->setX(d.rect.x() + d.rect.width() * 0.5 - w * 0.5);
            textItem->setY(d.rect.y() - h);
        } else {
            textItem->setX(d.rect.x() + d.rect.width());
            textItem->setY(d.rect.y() + d.rect.height() * 0.5 - h * 0.5);
        }
    }
    textItem->update();
}

void BarsRenderer::updateComponents(QBarSeries *series)
{
    int barIndex = 0;
    auto &seriesData = m_seriesData[series];
    auto &barItems = m_barItems[series];
    for (auto i = seriesData.cbegin(), end = seriesData.cend(); i != end; ++i) {
        if (barItems.size() <= barIndex) {
            QQuickItem *item = nullptr;
            // Create more components as needed
            if (series->barDelegate()) {
                item = qobject_cast<QQuickItem *>(
                        series->barDelegate()->create(series->barDelegate()->creationContext()));
            }
            if (!item)
                item = new QQuickRectangle();
            item->setParent(this);
            item->setParentItem(this);
            barItems << item;
        }
        if (barItems.size() > barIndex) {
            BarSeriesData d = *i;
            if (series->barDelegate()) {
                // Set custom bar components
                auto &barItem = barItems[barIndex];
                barItem->setX(d.rect.x());
                barItem->setY(d.rect.y());
                barItem->setWidth(d.rect.width());
                barItem->setHeight(d.rect.height());
                barItem->setVisible(series->isVisible());
                // Check for specific dynamic properties
                if (barItem->property(TAG_BAR_COLOR).isValid())
                    barItem->setProperty(TAG_BAR_COLOR, d.color);
                if (barItem->property(TAG_BAR_BORDER_COLOR).isValid())
                    barItem->setProperty(TAG_BAR_BORDER_COLOR, d.borderColor);
                if (barItem->property(TAG_BAR_BORDER_WIDTH).isValid())
                    barItem->setProperty(TAG_BAR_BORDER_WIDTH, d.borderWidth);
                if (barItem->property(TAG_BAR_SELECTED).isValid())
                    barItem->setProperty(TAG_BAR_SELECTED, d.isSelected);
                if (barItem->property(TAG_BAR_VALUE).isValid())
                    barItem->setProperty(TAG_BAR_VALUE, d.value);
                if (barItem->property(TAG_BAR_LABEL).isValid())
                    barItem->setProperty(TAG_BAR_LABEL, d.label);
            } else {
                // Set default rectangle bars
                auto barItem = qobject_cast<QQuickRectangle *>(barItems[barIndex]);
                if (barItem) {
                    barItem->setX(d.rect.x());
                    barItem->setY(d.rect.y());
                    barItem->setWidth(d.rect.width());
                    barItem->setHeight(d.rect.height());
                    barItem->setVisible(series->isVisible());
                    barItem->setColor(d.color);
                    barItem->border()->setColor(d.borderColor);
                    barItem->border()->setWidth(d.borderWidth);
                    barItem->setRadius(4.0);
                }
            }
        }
        barIndex++;
    }
}

void BarsRenderer::updateValueLabels(QBarSeries *series)
{
    if (!series->barDelegate() && series->isVisible() && series->labelsVisible()) {
        // Update default value labels
        int barIndex = 0;
        auto &seriesData = m_seriesData[series];
        auto &labelTextItems = m_labelTextItems[series];
        for (auto i = seriesData.cbegin(), end = seriesData.cend(); i != end; ++i) {
            if (labelTextItems.size() <= barIndex) {
                // Create more label items as needed
                auto labelItem = new QQuickText(this);
                labelTextItems << labelItem;
            }
            if (labelTextItems.size() > barIndex) {
                // Set label item values
                auto &textItem = labelTextItems[barIndex];
                const auto d = *i;
                if (qFuzzyIsNull(d.value)) {
                    textItem->setVisible(false);
                } else {
                    textItem->setVisible(series->labelsVisible());
                    QString valueLabel = generateLabelText(series, d.value);
                    textItem->setText(valueLabel);
                    positionLabelItem(series, textItem, d);
                    QColor labelColor = d.labelColor;
                    if (labelColor.alpha() == 0) {
                        // TODO: Use graphs theme labels color.
                        labelColor = QColor(255, 255, 255);
                    }
                    textItem->setColor(labelColor);
                    textItem->setRotation(series->labelsAngle());
                }
            }
            barIndex++;
        }
    } else {
        // Hide all possibly existing label items
        auto &labelTextItems = m_labelTextItems[series];
        for (auto textItem : labelTextItems)
            textItem->setVisible(false);
    }
}

void calculateCategoryTotalValues(QBarSeries *series, QList<float> &totalValues, qsizetype valuesPerSet)
{
    totalValues.fill(0, valuesPerSet);
    for (auto s : series->barSets()) {
        QVariantList v = s->values();
        int setIndex = 0;
        for (auto variantValue : std::as_const(v)) {
            if (setIndex < totalValues.size())
                totalValues[setIndex] += variantValue.toReal();
            setIndex++;
        }
    }
}

void BarsRenderer::updateVerticalBars(QBarSeries *series, qsizetype setCount, qsizetype valuesPerSet)
{
    bool stacked = series->barsType() == QBarSeries::BarsType::Stacked
            || series->barsType() == QBarSeries::BarsType::StackedPercent;
    bool percent = series->barsType() == QBarSeries::BarsType::StackedPercent;
    // Bars area width & height
    float w = width();
    float h = height();
    // Max width of a bar if no separation between sets.
    float maxBarWidth = w / (setCount * valuesPerSet) - m_barMargin;
    if (stacked)
        maxBarWidth = w / valuesPerSet;
    // Actual bar width.
    float barWidth = maxBarWidth * series->barWidth();
    // Helper to keep barsets centered when bar width is less than max width.
    float barCentering = (maxBarWidth - barWidth) * setCount * 0.5;
    if (stacked)
        barCentering = (maxBarWidth - barWidth) * 0.5;

    auto &seriesData = m_seriesData[series];
    auto &rectNodesInputRects = m_rectNodesInputRects[series];
    // Clear the selection rects
    // These will be filled only if series is selectable
    rectNodesInputRects.clear();
    seriesData.clear();

    float seriesPos = 0;
    float posXInSet = 0;
    QList<float> posYListInSet;
    if (stacked)
        posYListInSet.fill(0, valuesPerSet);
    QList<float> totalValuesListInSet;
    if (percent)
        calculateCategoryTotalValues(series, totalValuesListInSet, valuesPerSet);

    int barIndexInSet = 0;
    int barSeriesIndex = 0;
    QList<QLegendData> legendDataList;
    for (auto s : series->barSets()) {
        QVariantList v = s->values();
        qsizetype valuesCount = v.size();
        if (valuesCount == 0)
            continue;
        seriesPos = 0;
        barIndexInSet = 0;
        BarSelectionRect *barSelectionRect = nullptr;
        if (series->isSelectable() || series->isHoverable()) {
            rectNodesInputRects << BarSelectionRect();
            barSelectionRect = &rectNodesInputRects.last();
            barSelectionRect->barSet = s;
            barSelectionRect->series = series;
        }

        QColor color = getSetColor(series, s, barSeriesIndex);
        QColor borderColor = getSetBorderColor(series, s, barSeriesIndex);
        qreal borderWidth = getSetBorderWidth(series, s);

        // Update legendData
        legendDataList.push_back({
                color,
                borderColor,
                s->label()
        });
        // Apply series opacity
        color.setAlpha(color.alpha() * series->opacity());
        borderColor.setAlpha(borderColor.alpha() * series->opacity());
        const auto selectedBars = s->selectedBars();
        for (auto variantValue : std::as_const(v)) {
            const float realValue = variantValue.toReal();
            float value = (realValue - m_graph->m_axisRenderer->m_axisVerticalMinValue) * series->valuesMultiplier();
            if (percent) {
                if (auto totalValue = totalValuesListInSet.at(barIndexInSet))
                    value *= (100.0 / totalValue);
            }
            const bool isSelected = selectedBars.contains(barIndexInSet);
            double delta = m_graph->m_axisRenderer->m_axisVerticalMaxValue - m_graph->m_axisRenderer->m_axisVerticalMinValue;
            double maxValues = delta > 0 ? 1.0 / delta : 100.0;
            float barLength = h * value * maxValues;
            float barY = h - barLength;
            float barX = seriesPos + posXInSet + barCentering;
            if (stacked) {
                barY = h - barLength - posYListInSet[barIndexInSet];
                barX = seriesPos + barCentering;
            }
            QRectF barRect(barX, barY, barWidth, barLength);
            if (barSelectionRect)
                barSelectionRect->rects << barRect;

            // Collect the series data
            BarSeriesData d;
            d.rect = barRect;
            d.color = isSelected ? getSetSelectedColor(series, s) : color;
            d.borderColor = borderColor;
            d.borderWidth = borderWidth;
            d.isSelected = isSelected;
            d.label = s->label();
            d.labelColor = s->labelColor();
            d.value = realValue;
            seriesData << d;

            if (stacked)
                posYListInSet[barIndexInSet] += barLength;
            barIndexInSet++;
            seriesPos = ((float)barIndexInSet / valuesPerSet) * w;
        }
        posXInSet += barWidth + m_barMargin;
        barSeriesIndex++;
    }
    series->d_func()->setLegendData(legendDataList);
}

void BarsRenderer::updateHorizontalBars(QBarSeries *series, qsizetype setCount, qsizetype valuesPerSet)
{
    bool stacked = series->barsType() == QBarSeries::BarsType::Stacked
            || series->barsType() == QBarSeries::BarsType::StackedPercent;
    bool percent = series->barsType() == QBarSeries::BarsType::StackedPercent;
    // Bars area width & height
    float w = width();
    float h = height();
    // Max width of a bar if no separation between sets.
    float maxBarWidth = h / (setCount * valuesPerSet) - m_barMargin;
    if (stacked)
        maxBarWidth = h / valuesPerSet;
    // Actual bar width.
    float barWidth = maxBarWidth * series->barWidth();
    // Helper to keep barsets centered when bar width is less than max width.
    float barCentering = (maxBarWidth - barWidth) * setCount * 0.5;
    if (stacked)
        barCentering = (maxBarWidth - barWidth) * 0.5;

    auto &seriesData = m_seriesData[series];
    auto &rectNodesInputRects = m_rectNodesInputRects[series];
    // Clear the selection rects
    // These will be filled only if series is selectable
    rectNodesInputRects.clear();
    seriesData.clear();

    float seriesPos = 0;
    float posYInSet = 0;
    QList<float> posXListInSet;
    if (stacked)
        posXListInSet.fill(0, valuesPerSet);
    QList<float> totalValuesListInSet;
    if (percent)
        calculateCategoryTotalValues(series, totalValuesListInSet, valuesPerSet);
    int barIndexInSet = 0;
    int barSerieIndex = 0;
    QList<QLegendData> legendDataList;
    for (auto s : series->barSets()) {
        QVariantList v = s->values();
        qsizetype valuesCount = v.size();
        if (valuesCount == 0)
            continue;
        seriesPos = 0;
        barIndexInSet = 0;
        BarSelectionRect *barSelectionRect = nullptr;
        if (series->isSelectable() || series->isHoverable()) {
            rectNodesInputRects << BarSelectionRect();
            barSelectionRect = &rectNodesInputRects.last();
            barSelectionRect->barSet = s;
            barSelectionRect->series = series;
        }

        QColor color = getSetColor(series, s, barSerieIndex);
        QColor borderColor = getSetBorderColor(series, s, barSerieIndex);
        qreal borderWidth = getSetBorderWidth(series, s);
        // Update legendData
        legendDataList.push_back({
                color,
                borderColor,
                s->label()
        });
        // Apply series opacity
        color.setAlpha(color.alpha() * series->opacity());
        borderColor.setAlpha(borderColor.alpha() * series->opacity());
        const auto selectedBars = s->selectedBars();
        for (auto variantValue : std::as_const(v)) {
            const float realValue = variantValue.toReal();
            float value = (realValue - m_graph->m_axisRenderer->m_axisHorizontalMinValue) * series->valuesMultiplier();
            if (percent) {
                if (auto totalValue = totalValuesListInSet.at(barIndexInSet))
                    value *= (100.0 / totalValue);
            }
            const bool isSelected = selectedBars.contains(barIndexInSet);
            double delta = m_graph->m_axisRenderer->m_axisHorizontalMaxValue - m_graph->m_axisRenderer->m_axisHorizontalMinValue;
            double maxValues = delta > 0 ? 1.0 / delta : 100.0;
            float barLength = w * value * maxValues;
            float barY = seriesPos + posYInSet + barCentering;
            float barX = 0;
            if (stacked) {
                barY = seriesPos + barCentering;
                barX = posXListInSet[barIndexInSet];
            }
            QRectF barRect(barX, barY, barLength, barWidth);
            if (barSelectionRect)
                barSelectionRect->rects << barRect;

            // Collect the series data
            BarSeriesData d;
            d.rect = barRect;
            d.color = isSelected ? getSetSelectedColor(series, s) : color;
            d.borderColor = borderColor;
            d.borderWidth = borderWidth;
            d.isSelected = isSelected;
            d.label = s->label();
            d.labelColor = s->labelColor();
            d.value = realValue;
            seriesData << d;

            if (stacked)
                posXListInSet[barIndexInSet] += barLength;
            barIndexInSet++;
            seriesPos = ((float)barIndexInSet / valuesPerSet) * h;
        }
        posYInSet += barWidth + m_barMargin;
        barSerieIndex++;
    }
    series->d_func()->setLegendData(legendDataList);
}

void BarsRenderer::handlePolish(QBarSeries *series)
{
    auto theme = m_graph->theme();
    if (!theme)
        return;

    if (!m_graph->m_axisRenderer)
        return;

    qsizetype setCount = series->barSets().size();
    auto &seriesData = m_seriesData[series];
    auto &barItems = m_barItems[series];
    auto &rectNodesInputRects = m_rectNodesInputRects[series];
    if (setCount == 0) {
        for (int i = 0; i < barItems.size(); i++)
            barItems[i]->deleteLater();
        barItems.clear();

        series->d_func()->clearLegendData();
        rectNodesInputRects.clear();
        seriesData.clear();
        return;
    }

    if (m_colorIndex < 0)
        m_colorIndex = m_graph->graphSeriesCount();
    m_graph->setGraphSeriesCount(m_colorIndex + setCount);

    if (series->barDelegateDirty() && !barItems.isEmpty()) {
        // Bars delegate has changed, so remove the old items.
        for (int i = 0; i < barItems.size(); i++)
            barItems[i]->deleteLater();
        barItems.clear();
        series->setBarDelegateDirty(false);
    }

    // Get bars values
    qsizetype valuesPerSet = series->barSets().first()->values().size();
    if (m_graph->orientation() == Qt::Orientation::Vertical)
        updateVerticalBars(series, setCount, valuesPerSet);
    else
        updateHorizontalBars(series, setCount, valuesPerSet);
    updateComponents(series);
    updateValueLabels(series);

    // Remove additional components
    for (qsizetype i = barItems.size() - 1; i >= seriesData.size(); --i)
        barItems[i]->deleteLater();
    const auto range = barItems.size() - seriesData.size();
    if (range > 0)
        barItems.remove(seriesData.size(), range);
}

void BarsRenderer::updateSeries(QBarSeries *series)
{
    Q_UNUSED(series);
}

void BarsRenderer::afterUpdate(QList<QAbstractSeries *> &cleanupSeries)
{
    Q_UNUSED(cleanupSeries);
}

void BarsRenderer::afterPolish(QList<QAbstractSeries *> &cleanupSeries)
{
    for (auto &cleanupSerie : cleanupSeries) {
        auto series = qobject_cast<QBarSeries *>(cleanupSerie);
        if (series && m_barItems.contains(series)) {
            // Remove custom bar items
            auto &barItems = m_barItems[series];
            for (int i = 0; i < barItems.size(); i++)
                barItems[i]->deleteLater();
            barItems.clear();
            m_barItems.remove(series);
        }
        if (series && m_labelTextItems.contains(series)) {
            // Remove bar label items
            auto &labelTextItems = m_labelTextItems[series];
            for (int i = 0; i < labelTextItems.size(); i++)
                labelTextItems[i]->deleteLater();
            labelTextItems.clear();
            m_labelTextItems.remove(series);
        }
    }
}

bool BarsRenderer::handleMousePress(QMouseEvent *event)
{
    bool handled = false;
    for (auto &rectNodesInputRects : m_rectNodesInputRects) {
        for (auto &barSelection : rectNodesInputRects) {
            if (!barSelection.series->isSelectable())
                continue;
            qsizetype indexInSet = 0;
            for (auto &rect : barSelection.rects) {
                if (rect.contains(event->pos())) {
                    // TODO: Currently just toggling selection
                    QList<qsizetype> indexList = {indexInSet};
                    barSelection.barSet->toggleSelection(indexList);
                    handled = true;
                }
                indexInSet++;
            }
        }
    }
    return handled;
}

bool BarsRenderer::handleHoverMove(QHoverEvent *event)
{
    bool handled = false;
    const QPointF &position = event->position();

    bool hovering = false;
    for (auto &rectNodesInputRects : m_rectNodesInputRects) {
        for (auto &barSelection : rectNodesInputRects) {
            int indexInSet = 0;

            for (auto &rect : barSelection.rects) {
                if (rect.contains(event->position().toPoint())) {
                    const QString &name = barSelection.series->name();
                    const QPointF point(indexInSet, barSelection.barSet->at(indexInSet));

                    if (!m_currentHoverSeries) {
                        m_currentHoverSeries = barSelection.series;
                        emit barSelection.series->hoverEnter(name, position, point);
                    }

                    emit barSelection.series->hover(name, position, point);
                    hovering = true;
                    handled = true;
                }
                indexInSet++;
            }
        }
    }

    if (!hovering && m_currentHoverSeries) {
        emit m_currentHoverSeries->hoverExit(m_currentHoverSeries->name(), position);
        m_currentHoverSeries = nullptr;
        handled = true;
    }
    return handled;
}

QT_END_NAMESPACE
