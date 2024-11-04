// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QMetaMethod>
#include "qquickgraphsscatterseries_p.h"
#include "utils_p.h"

QT_BEGIN_NAMESPACE

QQuickGraphsScatter3DSeries::QQuickGraphsScatter3DSeries(QObject *parent)
    : QScatter3DSeries(parent)
    , m_baseGradient(QJSValue(0))
    , m_singleHighlightGradient(QJSValue(0))
    , m_multiHighlightGradient(QJSValue(0))
{}

QQuickGraphsScatter3DSeries::~QQuickGraphsScatter3DSeries() {}

QQmlListProperty<QObject> QQuickGraphsScatter3DSeries::seriesChildren()
{
    return QQmlListProperty<QObject>(this,
                                     this,
                                     &QQuickGraphsScatter3DSeries::appendSeriesChildren,
                                     0,
                                     0,
                                     0);
}

void QQuickGraphsScatter3DSeries::appendSeriesChildren(QQmlListProperty<QObject> *list,
                                                       QObject *element)
{
    QScatterDataProxy *proxy = qobject_cast<QScatterDataProxy *>(element);
    if (proxy)
        reinterpret_cast<QQuickGraphsScatter3DSeries *>(list->data)->setDataProxy(proxy);
}

void QQuickGraphsScatter3DSeries::setBaseGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Base, m_baseGradient);
}

QJSValue QQuickGraphsScatter3DSeries::baseGradient() const
{
    return m_baseGradient;
}

void QQuickGraphsScatter3DSeries::setSingleHighlightGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Single, m_singleHighlightGradient);
}

QJSValue QQuickGraphsScatter3DSeries::singleHighlightGradient() const
{
    return m_singleHighlightGradient;
}

void QQuickGraphsScatter3DSeries::setMultiHighlightGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Multi, m_multiHighlightGradient);
}

QJSValue QQuickGraphsScatter3DSeries::multiHighlightGradient() const
{
    return m_multiHighlightGradient;
}

int QQuickGraphsScatter3DSeries::invalidSelectionIndex() const
{
    return QScatter3DSeries::invalidSelectionIndex();
}

void QQuickGraphsScatter3DSeries::handleBaseGradientUpdate()
{
    if (!m_baseGradient.isNull())
        Utils::setSeriesGradient(this, m_baseGradient, GradientType::Base);
}

void QQuickGraphsScatter3DSeries::handleSingleHighlightGradientUpdate()
{
    if (!m_singleHighlightGradient.isNull())
        Utils::setSeriesGradient(this, m_singleHighlightGradient, GradientType::Single);
}

void QQuickGraphsScatter3DSeries::handleMultiHighlightGradientUpdate()
{
    if (!m_multiHighlightGradient.isNull())
        Utils::setSeriesGradient(this, m_multiHighlightGradient, GradientType::Multi);
}

QT_END_NAMESPACE
