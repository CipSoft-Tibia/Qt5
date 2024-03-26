// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtCharts 2.0


ChartView {
    width: 300
    height: 300

    StackedBarSeries {
        name: "StackedBarSeries"
        BarSet { label: "Set1"; values: [2, 2, 3] }
        BarSet { label: "Set2"; values: [5, 1, 2] }
        BarSet { label: "Set3"; values: [3, 5, 8] }
    }
}
