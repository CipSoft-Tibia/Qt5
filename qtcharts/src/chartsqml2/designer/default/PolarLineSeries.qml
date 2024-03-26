// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtCharts 2.0


PolarChartView {
    width: 300
    height: 300

    LineSeries {
        name: "LineSeries"
        axisRadial: CategoryAxis {
            min: 0
            max: 20
        }
        axisAngular: ValueAxis {
            tickCount: 9
        }
        XYPoint { x: 0; y: 4.3 }
        XYPoint { x: 2; y: 4.7 }
        XYPoint { x: 4; y: 5.2 }
        XYPoint { x: 6; y: 6.1 }
        XYPoint { x: 8; y: 12.9 }
        XYPoint { x: 9; y: 19.2 }
    }
}
