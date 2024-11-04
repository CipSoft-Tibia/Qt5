// Copyright (C) 2024 The Qt Company Ltd.
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

#ifndef QQUICKGRAPHSSURFACESERIES_P_H
#define QQUICKGRAPHSSURFACESERIES_P_H

#include "qquickgraphscolor_p.h"
#include "qsurface3dseries.h"

#include <QtQml/qqml.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/graphsglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickGraphsSurface3DSeries : public QSurface3DSeries
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> seriesChildren READ seriesChildren CONSTANT)
    // This property is overloaded to use QPointF instead of QPoint to work
    // around qml bug where Qt.point(0, 0) can't be assigned due to error
    // "Cannot assign QPointF to QPoint".
    Q_PROPERTY(
        QPointF selectedPoint READ selectedPoint WRITE setSelectedPoint NOTIFY selectedPointChanged)
    // This is static method in parent class, overload as constant property for
    // qml.
    Q_PROPERTY(QPointF invalidSelectionPosition READ invalidSelectionPosition CONSTANT)
    Q_PROPERTY(
        QJSValue baseGradient READ baseGradient WRITE setBaseGradient NOTIFY baseGradientChanged)
    Q_PROPERTY(QJSValue singleHighlightGradient READ singleHighlightGradient WRITE
                   setSingleHighlightGradient NOTIFY singleHighlightGradientChanged)
    Q_PROPERTY(QJSValue multiHighlightGradient READ multiHighlightGradient WRITE
                   setMultiHighlightGradient NOTIFY multiHighlightGradientChanged)
    Q_CLASSINFO("DefaultProperty", "seriesChildren")

    QML_NAMED_ELEMENT(Surface3DSeries)

public:
    QQuickGraphsSurface3DSeries(QObject *parent = 0);
    ~QQuickGraphsSurface3DSeries() override;

    void setSelectedPoint(const QPointF &position);
    QPointF selectedPoint() const;
    QPointF invalidSelectionPosition() const;

    QQmlListProperty<QObject> seriesChildren();
    static void appendSeriesChildren(QQmlListProperty<QObject> *list, QObject *element);

    void setBaseGradient(QJSValue gradient);
    QJSValue baseGradient() const;
    void setSingleHighlightGradient(QJSValue gradient);
    QJSValue singleHighlightGradient() const;
    void setMultiHighlightGradient(QJSValue gradient);
    QJSValue multiHighlightGradient() const;

public Q_SLOTS:
    void handleBaseGradientUpdate();
    void handleSingleHighlightGradientUpdate();
    void handleMultiHighlightGradientUpdate();

Q_SIGNALS:
    void selectedPointChanged(const QPointF &position);
    void baseGradientChanged(QJSValue gradient);
    void singleHighlightGradientChanged(QJSValue gradient);
    void multiHighlightGradientChanged(QJSValue gradient);

private:
    QJSValue m_baseGradient;            // Not owned
    QJSValue m_singleHighlightGradient; // Not owned
    QJSValue m_multiHighlightGradient;  // Not owned
};

QT_END_NAMESPACE

#endif
