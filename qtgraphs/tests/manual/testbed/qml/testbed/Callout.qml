// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtGraphs
import QtQuick.Controls.Basic
import QtQuick.Dialogs

Rectangle {
    id: mainview
    width: 800
    height: 600
    color: "#404040"

    Rectangle {
        id: background
        anchors.fill: chartView
        color: "#202020"
        border.color: "#606060"
        border.width: 2
        radius: 10
    }

    GraphsView {
        id: chartView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        anchors.margins: 10
        backgroundColor: "#202020"

        onHoverEnter: {
            tooltip.visible = true;
        }

        onHoverExit: {
            tooltip.visible = false;
        }

        onHover: (seriesName, position, value) => {
            tooltip.x = position.x + 1;
            tooltip.y = position.y + 1;
            tooltip.text = "Series: " + seriesName + ", X: " + value.x.toFixed(1) + ", Y: " + value.y.toFixed(1);
        }

        SeriesTheme {
            id: seriesTheme
            colorTheme: SeriesTheme.SeriesTheme1
        }

        BarSeries {
            id: barSeries
            name: "First"
            hoverable: true
            axisX: BarCategoryAxis { categories: ["2023", "2024", "2025", "2026"] }
            axisY: ValueAxis {
                id: yAxis
                max: 8
            }
            BarSet { id: set1; label: "Axel"; values: [1, 2, 3, 4] }
        }

        LineSeries {
            id: lineSeries
            name: "Second"
            theme: seriesTheme
            hoverable: true

            XYPoint { x: 0; y: 6.6 }
            XYPoint { x: 0.6; y: 4.1 }
            XYPoint { x: 1.5; y: 5.3 }
            XYPoint { x: 2.2; y: 7.1 }
            XYPoint { x: 3.3; y: 6.9 }
            XYPoint { x: 3.6; y: 5.0 }
            XYPoint { x: 4.0; y: 5.3 }
        }

        ScatterSeries {
            id: scatterSeries
            name: "Third"
            theme: seriesTheme
            hoverable: true

            XYPoint { x: 0; y: 2.6 }
            XYPoint { x: 0.2; y: 3.1 }
            XYPoint { x: 1.3; y: 6.3 }
            XYPoint { x: 2.4; y: 5.1 }
            XYPoint { x: 3.5; y: 6.9 }
            XYPoint { x: 3.6; y: 5.2 }
            XYPoint { x: 4.0; y: 3.3 }
        }

        ToolTip {
            id: tooltip
        }
    }
}
