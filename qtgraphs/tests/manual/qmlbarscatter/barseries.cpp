// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "barseries.h"

#include <QtGraphs/QBarSet>

BarSeries::BarSeries(QObject *series)
    : QBarSeries(series)
    , m_cpuUpdater(this)
    , m_axis1(this)
    , m_axis2(this)
    , m_counter(0)
{
    connect(&m_timer, &QTimer::timeout, this, &BarSeries::frameUpdate);
    m_timer.start(100);

    m_axis1.setCategories({"Reading 1", "Reading 2", "Reading 3", "Reading 4", "Reading 5"});

    m_axis2.setMin(0);
    m_axis2.setMax(100);

    setAxisX(&m_axis1);
    setAxisY(&m_axis2);

    m_barList = new QBarSet(this);

    for (int i = 0; i < 5; ++i)
        m_barList->append(double(0));
}

void BarSeries::frameUpdate()
{
    auto reading = m_cpuUpdater.update();

    m_barList->replace(m_counter++, reading);

    if (m_counter == 5)
        m_counter = 0;
}
