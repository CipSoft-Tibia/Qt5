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

#ifndef Q3DLIGHT_P_H
#define Q3DLIGHT_P_H

#include "q3dlight.h"
#include "q3dobject_p.h"

QT_BEGIN_NAMESPACE

class Q3DScene;
class Q3DLight;

class Q3DLightPrivate : public Q3DObjectPrivate
{
    Q_DECLARE_PUBLIC(Q3DLight)

public:
    Q3DLightPrivate(Q3DLight *q);
    ~Q3DLightPrivate();

    void sync(Q3DLight &other);

public:
    bool m_automaticLight;
};

QT_END_NAMESPACE

#endif





