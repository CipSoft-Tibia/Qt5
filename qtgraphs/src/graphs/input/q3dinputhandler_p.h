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

#ifndef Q3DINPUTHANDLER_P_H
#define Q3DINPUTHANDLER_P_H

#include "qabstract3dinputhandler_p.h"
#include "q3dinputhandler.h"

QT_BEGIN_NAMESPACE

class Abstract3DController;

class Q3DInputHandlerPrivate : public QAbstract3DInputHandlerPrivate
{
    Q_DECLARE_PUBLIC(Q3DInputHandler)

public:
    Q3DInputHandlerPrivate(Q3DInputHandler *q);
    ~Q3DInputHandlerPrivate();

public Q_SLOTS:
    void handleSceneChange(Q3DScene *scene);
    void handleQueriedGraphPositionChange();

protected:
    bool m_rotationEnabled;
    bool m_zoomEnabled;
    bool m_selectionEnabled;
    bool m_zoomAtTargetEnabled;
    bool m_zoomAtTargetPending;

    Abstract3DController *m_controller; // Not owned

    float m_requestedZoomLevel;
    float m_driftMultiplier;
};

QT_END_NAMESPACE

#endif
