// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CUSTOMINPUTHANDLER_H
#define CUSTOMINPUTHANDLER_H

#include <QtGraphs/QAbstract3DInputHandler>
#include <QtGraphs/qabstract3dgraph.h>

class CustomInputHandler : public QAbstract3DInputHandler
{
    Q_OBJECT
public:
    explicit CustomInputHandler(QAbstract3DGraph *graph, QObject *parent = 0);

    virtual void mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event, const QPoint &mousePos) override;

private:
    void onPositionQueryChanged(const QVector3D &position);

private:
    QAbstract3DGraph *m_graph;
};

#endif
