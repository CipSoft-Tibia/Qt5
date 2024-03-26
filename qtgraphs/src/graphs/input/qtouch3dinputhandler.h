// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTOUCH3DINPUTHANDLER_H
#define QTOUCH3DINPUTHANDLER_H

#include <QtGraphs/q3dinputhandler.h>

QT_BEGIN_NAMESPACE

class QTouch3DInputHandlerPrivate;

class Q_GRAPHS_EXPORT QTouch3DInputHandler : public Q3DInputHandler
{
    Q_DECLARE_PRIVATE(QTouch3DInputHandler)

public:
    explicit QTouch3DInputHandler(QObject *parent = nullptr);
    virtual ~QTouch3DInputHandler();

    // Input event listeners
    void touchEvent(QTouchEvent *event) override;

private:
    Q_DISABLE_COPY(QTouch3DInputHandler)
};

QT_END_NAMESPACE

#endif
