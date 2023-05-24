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

#ifndef Q3DOBJECT_P_H
#define Q3DOBJECT_P_H

#include "q3dobject.h"

QT_BEGIN_NAMESPACE

class Q3DObject;
class Q3DScene;

class Q3DObjectPrivate
{
    Q_DECLARE_PUBLIC(Q3DObject)

public:
    Q3DObjectPrivate(Q3DObject *q);
    ~Q3DObjectPrivate();

public:
    QVector3D m_position;
    bool m_isDirty;

protected:
    Q3DObject *q_ptr;
};

QT_END_NAMESPACE

#endif
