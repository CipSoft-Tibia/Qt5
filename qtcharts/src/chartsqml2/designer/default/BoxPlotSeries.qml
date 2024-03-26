// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtCharts 2.0

ChartView {
    width: 300
    height: 300

    BoxPlotSeries {
        name: "BoxPlotSeries"
        BoxSet { label: "Set1"; values: [3, 4, 5.1, 6.2, 8.5] }
        BoxSet { label: "Set2"; values: [5, 6, 7.5, 8.6, 11.8] }
        BoxSet { label: "Set3"; values: [3.2, 5, 5.7, 8, 9.2] }
    }
}
