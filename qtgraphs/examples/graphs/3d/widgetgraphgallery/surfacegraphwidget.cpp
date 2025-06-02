// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "surfacegraphwidget.h"

SurfaceGraphWidget::SurfaceGraphWidget() {}

SurfaceGraphWidget::~SurfaceGraphWidget() {}

void SurfaceGraphWidget::initialize()
{
    m_surfaceGraph = new Q3DSurfaceWidgetItem();
    m_surfaceGraph->setWidget(this);
}

Q3DSurfaceWidgetItem *SurfaceGraphWidget::surfaceGraph() const
{
    return m_surfaceGraph;
}
