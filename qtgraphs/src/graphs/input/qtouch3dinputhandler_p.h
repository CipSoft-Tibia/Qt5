// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTOUCH3DINPUTHANDLER_P_H
#define QTOUCH3DINPUTHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "q3dinputhandler_p.h"
#include "qtouch3dinputhandler.h"

QT_FORWARD_DECLARE_CLASS(QTimer)

QT_BEGIN_NAMESPACE

class QAbstract3DInputHandler;

class QTouch3DInputHandlerPrivate : public Q3DInputHandlerPrivate
{
    Q_DECLARE_PUBLIC(QTouch3DInputHandler)

public:
    QTouch3DInputHandlerPrivate(QTouch3DInputHandler *q);
    ~QTouch3DInputHandlerPrivate();

    void handlePinchZoom(float distance, const QPoint &pos);
    void handleTapAndHold();
    void handleSelection(const QPointF &position);
    void handleRotation(const QPointF &position);

public:
    QTimer *m_holdTimer;
    QPointF m_startHoldPos;
    QPointF m_touchHoldPos;
};

QT_END_NAMESPACE

#endif
