// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SURFACEGRAPH_H
#define SURFACEGRAPH_H

#include <QtCore/qobject.h>
#include <QtGraphs/q3dsurface.h>
#include "surfacegraphmodifier.h"

class SurfaceGraph : public QObject
{
    Q_OBJECT
public:
    SurfaceGraph();

    void initialize();
    QWidget *surfaceWidget() { return m_surfaceWidget; }

private:
    SurfaceGraphModifier *m_modifier = nullptr;
    Q3DSurface *m_surfaceGraph = nullptr;
    QWidget *m_surfaceWidget = nullptr;
};

#endif
