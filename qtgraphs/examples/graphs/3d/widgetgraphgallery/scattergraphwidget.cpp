// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "scattergraphwidget.h"

ScatterGraphWidget::ScatterGraphWidget() {}

ScatterGraphWidget::~ScatterGraphWidget() {}

void ScatterGraphWidget::initialize()
{
    m_scatterGraph = new Q3DScatterWidgetItem();
    m_scatterGraph->setWidget(this);
}

Q3DScatterWidgetItem *ScatterGraphWidget::scatterGraph() const
{
    return m_scatterGraph;
}
