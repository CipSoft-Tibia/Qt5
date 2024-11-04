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

#ifndef QQUICKGRAPHSSCATTERSERIES_P_H
#define QQUICKGRAPHSSCATTERSERIES_P_H

#include "qquickgraphscolor_p.h"
#include "qscatter3dseries.h"

#include <QtQml/qqml.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/graphsglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickGraphsScatter3DSeries : public QScatter3DSeries
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> seriesChildren READ seriesChildren CONSTANT)
    Q_PROPERTY(
        QJSValue baseGradient READ baseGradient WRITE setBaseGradient NOTIFY baseGradientChanged)
    Q_PROPERTY(QJSValue singleHighlightGradient READ singleHighlightGradient WRITE
                   setSingleHighlightGradient NOTIFY singleHighlightGradientChanged)
    Q_PROPERTY(QJSValue multiHighlightGradient READ multiHighlightGradient WRITE
                   setMultiHighlightGradient NOTIFY multiHighlightGradientChanged)
    // This is static method in parent class, overload as constant property for qml.
    Q_PROPERTY(int invalidSelectionIndex READ invalidSelectionIndex CONSTANT)
    Q_CLASSINFO("DefaultProperty", "seriesChildren")

    QML_NAMED_ELEMENT(Scatter3DSeries)

public:
    QQuickGraphsScatter3DSeries(QObject *parent = 0);
    ~QQuickGraphsScatter3DSeries() override;

    QQmlListProperty<QObject> seriesChildren();
    static void appendSeriesChildren(QQmlListProperty<QObject> *list, QObject *element);

    void setBaseGradient(QJSValue gradient);
    QJSValue baseGradient() const;
    void setSingleHighlightGradient(QJSValue gradient);
    QJSValue singleHighlightGradient() const;
    void setMultiHighlightGradient(QJSValue gradient);
    QJSValue multiHighlightGradient() const;

    int invalidSelectionIndex() const;

public Q_SLOTS:
    void handleBaseGradientUpdate();
    void handleSingleHighlightGradientUpdate();
    void handleMultiHighlightGradientUpdate();

Q_SIGNALS:
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
