// Copyright (C) 2023 The Qt Company Ltd.
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

#ifndef QABSTRACT3DINPUTHANDLER_P_H
#define QABSTRACT3DINPUTHANDLER_P_H

#include "qabstract3dinputhandler.h"
#include "qquickgraphsitem_p.h"

QT_BEGIN_NAMESPACE

class QAbstract3DInputHandler;
class Q3DScene;

class QAbstract3DInputHandlerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(QAbstract3DInputHandler)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    QAbstract3DInputHandlerPrivate(QAbstract3DInputHandler *q);
    ~QAbstract3DInputHandlerPrivate();

public:
    enum class InputState { None, Selecting, Rotating, Pinching };

    int m_prevDistance = 0;
    QPoint m_previousInputPos = {};
    InputState m_inputState = InputState::None;

protected:
    QAbstract3DInputHandler *q_ptr;

private:
    QAbstract3DInputHandler::InputView m_inputView;
    QPoint m_inputPosition;

    Q3DScene *m_scene;
    QQuickGraphsItem *m_item;
    bool m_isDefaultHandler;

    friend class QQuickGraphsItem;
};

QT_END_NAMESPACE

#endif
