// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QMetaMethod>
#include "qquickgraphssurfaceseries_p.h"
#include "utils_p.h"

QT_BEGIN_NAMESPACE

QQuickGraphsSurface3DSeries::QQuickGraphsSurface3DSeries(QObject *parent)
    : QSurface3DSeries(parent)
    , m_baseGradient(QJSValue(0))
    , m_singleHighlightGradient(QJSValue(0))
    , m_multiHighlightGradient(QJSValue(0))
{
    QObject::connect(this,
                     &QSurface3DSeries::selectedPointChanged,
                     this,
                     &QQuickGraphsSurface3DSeries::selectedPointChanged);
}

QQuickGraphsSurface3DSeries::~QQuickGraphsSurface3DSeries() {}

void QQuickGraphsSurface3DSeries::setSelectedPoint(const QPointF &position)
{
    QSurface3DSeries::setSelectedPoint(position.toPoint());
}

QPointF QQuickGraphsSurface3DSeries::selectedPoint() const
{
    return QPointF(QSurface3DSeries::selectedPoint());
}

QPointF QQuickGraphsSurface3DSeries::invalidSelectionPosition() const
{
    return QPointF(QSurface3DSeries::invalidSelectionPosition());
}

QQmlListProperty<QObject> QQuickGraphsSurface3DSeries::seriesChildren()
{
    return QQmlListProperty<QObject>(this,
                                     this,
                                     &QQuickGraphsSurface3DSeries::appendSeriesChildren,
                                     0,
                                     0,
                                     0);
}

void QQuickGraphsSurface3DSeries::appendSeriesChildren(QQmlListProperty<QObject> *list,
                                                       QObject *element)
{
    QSurfaceDataProxy *proxy = qobject_cast<QSurfaceDataProxy *>(element);
    if (proxy)
        reinterpret_cast<QQuickGraphsSurface3DSeries *>(list->data)->setDataProxy(proxy);
}

void QQuickGraphsSurface3DSeries::setBaseGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Base, m_baseGradient);
}

QJSValue QQuickGraphsSurface3DSeries::baseGradient() const
{
    return m_baseGradient;
}

void QQuickGraphsSurface3DSeries::setSingleHighlightGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Single, m_singleHighlightGradient);
}

QJSValue QQuickGraphsSurface3DSeries::singleHighlightGradient() const
{
    return m_singleHighlightGradient;
}

void QQuickGraphsSurface3DSeries::setMultiHighlightGradient(QJSValue gradient)
{
    Utils::connectSeriesGradient(this, gradient, GradientType::Multi, m_multiHighlightGradient);
}

QJSValue QQuickGraphsSurface3DSeries::multiHighlightGradient() const
{
    return m_multiHighlightGradient;
}

void QQuickGraphsSurface3DSeries::handleBaseGradientUpdate()
{
    if (!m_baseGradient.isNull())
        Utils::setSeriesGradient(this, m_baseGradient, GradientType::Base);
}

void QQuickGraphsSurface3DSeries::handleSingleHighlightGradientUpdate()
{
    if (!m_singleHighlightGradient.isNull())
        Utils::setSeriesGradient(this, m_singleHighlightGradient, GradientType::Single);
}

void QQuickGraphsSurface3DSeries::handleMultiHighlightGradientUpdate()
{
    if (!m_multiHighlightGradient.isNull())
        Utils::setSeriesGradient(this, m_multiHighlightGradient, GradientType::Multi);
}

QT_END_NAMESPACE
