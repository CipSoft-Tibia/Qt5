// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef POINTRENDERER_H
#define POINTRENDERER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QQuickItem>
#include <QtQuickShapes/private/qquickshape_p.h>
#include <QtQuick/private/qsgdefaultinternalrectanglenode_p.h>

QT_BEGIN_NAMESPACE

class QGraphsView;
class QXYSeries;
class QLineSeries;
class QScatterSeries;

class PointRenderer : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    PointRenderer(QQuickItem *parent = nullptr);
    virtual ~PointRenderer();

    void handlePolish(QLineSeries *series);
    void handlePolish(QScatterSeries *series);
    void updateSeries(QXYSeries *series);
    bool handleMouseMove(QMouseEvent *event);
    bool handleMousePress(QMouseEvent *event);
    bool handleMouseRelease(QMouseEvent *event);
    bool handleHoverMove(QHoverEvent *event);

Q_SIGNALS:

private:
    struct PointGroup {
        QXYSeries *series;
        QQuickShapePath *shapePath = nullptr;
        QList<QQuickPathLine *> paths;
        QList<QQuickItem *> markers;
        QList<QSGDefaultInternalRectangleNode *> nodes;
        QList<QRectF> rects;
        int colorIndex = -1;
        bool hover = false;
    };

    QGraphsView *m_graph = nullptr;
    QQuickShape m_shape;
    QMap<QXYSeries *, PointGroup *> m_groups;
    int m_currentColorIndex = 0;

    // Point drag variables
    bool m_pointPressed = false;
    bool m_pointDragging = false;
    QPoint m_pressStart;
    PointGroup *m_pressedGroup = nullptr;
    int m_pressedPointIndex = 0;
};

QT_END_NAMESPACE

#endif // POINTRENDERER_H
