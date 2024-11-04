// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QMetaMethod>
#include "qquickgraphsbarsseries_p.h"
#include "utils_p.h"

QT_BEGIN_NAMESPACE

QQuickGraphsBar3DSeries::QQuickGraphsBar3DSeries(QObject *parent)
    : QBar3DSeries(parent)
    , m_baseGradient(QJSValue(0))
    , m_singleHighlightGradient(QJSValue(0))
    , m_multiHighlightGradient(QJSValue(0))
    , m_dummyColors(false)
{
    QObject::connect(this,
                     &QBar3DSeries::selectedBarChanged,
                     this,
                     &QQuickGraphsBar3DSeries::selectedBarChanged);
}

QQuickGraphsBar3DSeries::~QQuickGraphsBar3DSeries() {}

QQmlListProperty<QObject> QQuickGraphsBar3DSeries::seriesChildren()
{
    return QQmlListProperty<QObject>(this,
                                     this,
                                     &QQuickGraphsBar3DSeries::appendSeriesChildren,
                                     0,
                                     0,
                                     0);
}

void QQuickGraphsBar3DSeries::appendSeriesChildren(QQmlListProperty<QObject> *list, QObject *element)
{
    QBarDataProxy *proxy = qobject_cast<QBarDataProxy *>(element);
    if (proxy)
        reinterpret_cast<QQuickGraphsBar3DSeries *>(list->data)->setDataProxy(proxy);
}

void QQuickGraphsBar3DSeries::setSelectedBar(const QPointF &position)
{
    QBar3DSeries::setSelectedBar(position.toPoint());
}

QPointF QQuickGraphsBar3DSeries::selectedBar() const
{
    return QPointF(QBar3DSeries::selectedBar());
}

QPointF QQuickGraphsBar3DSeries::invalidSelectionPosition() const
{
    return QPointF(QBar3DSeries::invalidSelectionPosition());
}

void QQuickGraphsBar3DSeries::setBaseGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Base, m_baseGradient);
}

QJSValue QQuickGraphsBar3DSeries::baseGradient() const
{
    return m_baseGradient;
}

void QQuickGraphsBar3DSeries::setSingleHighlightGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Single, m_singleHighlightGradient);
}

QJSValue QQuickGraphsBar3DSeries::singleHighlightGradient() const
{
    return m_singleHighlightGradient;
}

void QQuickGraphsBar3DSeries::setMultiHighlightGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Multi, m_multiHighlightGradient);
}

QJSValue QQuickGraphsBar3DSeries::multiHighlightGradient() const
{
    return m_multiHighlightGradient;
}

QQmlListProperty<QQuickGraphsColor> QQuickGraphsBar3DSeries::rowColors()
{
    return QQmlListProperty<QQuickGraphsColor>(this,
                                               this,
                                               &QQuickGraphsBar3DSeries::appendRowColorsFunc,
                                               &QQuickGraphsBar3DSeries::countRowColorsFunc,
                                               &QQuickGraphsBar3DSeries::atRowColorsFunc,
                                               &QQuickGraphsBar3DSeries::clearRowColorsFunc);
}

void QQuickGraphsBar3DSeries::appendRowColorsFunc(QQmlListProperty<QQuickGraphsColor> *list,
                                                  QQuickGraphsColor *color)
{
    reinterpret_cast<QQuickGraphsBar3DSeries *>(list->data)->addColor(color);
}

qsizetype QQuickGraphsBar3DSeries::countRowColorsFunc(QQmlListProperty<QQuickGraphsColor> *list)
{
    return reinterpret_cast<QQuickGraphsBar3DSeries *>(list->data)->colorList().size();
}

QQuickGraphsColor *QQuickGraphsBar3DSeries::atRowColorsFunc(
    QQmlListProperty<QQuickGraphsColor> *list, qsizetype index)
{
    return reinterpret_cast<QQuickGraphsBar3DSeries *>(list->data)->colorList().at(index);
}

void QQuickGraphsBar3DSeries::clearRowColorsFunc(QQmlListProperty<QQuickGraphsColor> *list)
{
    reinterpret_cast<QQuickGraphsBar3DSeries *>(list->data)->clearColors();
}

void QQuickGraphsBar3DSeries::handleBaseGradientUpdate()
{
    if (!m_baseGradient.isNull())
        Utils::setSeriesGradient(this, m_baseGradient, GradientType::Base);
}

void QQuickGraphsBar3DSeries::handleSingleHighlightGradientUpdate()
{
    if (!m_singleHighlightGradient.isNull())
        Utils::setSeriesGradient(this, m_singleHighlightGradient, GradientType::Single);
}

void QQuickGraphsBar3DSeries::handleMultiHighlightGradientUpdate()
{
    if (!m_multiHighlightGradient.isNull())
        Utils::setSeriesGradient(this, m_multiHighlightGradient, GradientType::Multi);
}

void QQuickGraphsBar3DSeries::handleRowColorUpdate()
{
    int colorCount = m_rowColors.size();
    int changed = 0;

    QQuickGraphsColor *color = qobject_cast<QQuickGraphsColor *>(QObject::sender());
    for (int i = 0; i < colorCount; i++) {
        if (color == m_rowColors.at(i)) {
            changed = i;
            break;
        }
    }
    QList<QColor> list = QBar3DSeries::rowColors();
    list[changed] = m_rowColors.at(changed)->color();
    QBar3DSeries::setRowColors(list);
}

void QQuickGraphsBar3DSeries::addColor(QQuickGraphsColor *color)
{
    if (!color) {
        qWarning("Color is invalid, use Color");
        return;
    }
    clearDummyColors();
    m_rowColors.append(color);
    connect(color,
            &QQuickGraphsColor::colorChanged,
            this,
            &QQuickGraphsBar3DSeries::handleRowColorUpdate);
    QList<QColor> list = QBar3DSeries::rowColors();
    list.append(color->color());
    QBar3DSeries::setRowColors(list);
}

QList<QQuickGraphsColor *> QQuickGraphsBar3DSeries::colorList()
{
    if (m_rowColors.isEmpty()) {
        m_dummyColors = true;
        const QList<QColor> list = QBar3DSeries::rowColors();
        for (const QColor &item : list) {
            QQuickGraphsColor *color = new QQuickGraphsColor(this);
            color->setColor(item);
            m_rowColors.append(color);
            connect(color,
                    &QQuickGraphsColor::colorChanged,
                    this,
                    &QQuickGraphsBar3DSeries::handleRowColorUpdate);
        }
    }
    return m_rowColors;
}

void QQuickGraphsBar3DSeries::clearColors()
{
    clearDummyColors();
    for (const auto color : std::as_const(m_rowColors))
        disconnect(color, 0, this, 0);

    m_rowColors.clear();
    QBar3DSeries::setRowColors(QList<QColor>());
}

void QQuickGraphsBar3DSeries::clearDummyColors()
{
    if (m_dummyColors) {
        qDeleteAll(m_rowColors);
        m_rowColors.clear();
        m_dummyColors = false;
    }
}

QT_END_NAMESPACE
