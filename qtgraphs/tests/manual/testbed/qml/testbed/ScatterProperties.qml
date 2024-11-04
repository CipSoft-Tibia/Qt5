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

    Row {
        id: toolbar
        anchors.top: parent.top
        anchors.margins: 10
        anchors.left: parent.left
        anchors.leftMargin: 60
        spacing: 10

        Row {
            id: seriesToolbar
            spacing: 10
            Button {
                text: "Theme1"
                onClicked: {
                    seriesTheme.colorTheme = SeriesTheme.SeriesTheme1;
                }
            }
            Button {
                text: "Theme2"
                onClicked: {
                    seriesTheme.colorTheme = SeriesTheme.SeriesTheme2;
                }
            }
        }
    }

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
        anchors.top: toolbar.bottom
        anchors.margins: 10
        backgroundColor: "#202020"

        SeriesTheme {
            id: seriesTheme
            colorTheme: SeriesTheme.SeriesTheme1
        }

        ScatterSeries {
            id: lineSeries1
            name: "First"
            theme: seriesTheme
            selectable: true
            axisX: ValueAxis {
                id: xAxis
                max: 4
            }
            axisY: ValueAxis {
                id: yAxis
                max: 8
            }
            pointMarker: Image {
                property bool selected: false
                source: "images/happy_box.png"
                width: selected ? 96 : 64
                height: selected ? 96 : 64
            }

            XYPoint { x: 0; y: 0 }
            XYPoint { x: 1.1; y: 2.1 }
            XYPoint { x: 1.9; y: 3.3 }
            XYPoint { x: 2.1; y: 2.1 }
            XYPoint { x: 2.9; y: 4.9 }
            XYPoint { x: 3.4; y: 3.0 }
            XYPoint { x: 4.0; y: 3.3 }
        }

        ScatterSeries {
            id: scatterSeries2
            name: "Second"
            theme: seriesTheme
            selectable: true

            XYPoint { x: 0; y: 6.6 }
            XYPoint { x: 0.6; y: 4.1 }
            XYPoint { x: 1.5; y: 5.3 }
            XYPoint { x: 2.2; y: 7.1 }
            XYPoint { x: 3.3; y: 6.9 }
            XYPoint { x: 3.6; y: 5.0 }
            XYPoint { x: 4.0; y: 5.3 }
        }

        ScatterSeries {
            id: scatterSeries3
            name: "Third"
            theme: seriesTheme
            XYPoint { x: 0; y: 2.6 }
            XYPoint { x: 0.2; y: 3.1 }
            XYPoint { x: 1.3; y: 6.3 }
            XYPoint { x: 2.4; y: 5.1 }
            XYPoint { x: 3.5; y: 6.9 }
            XYPoint { x: 3.6; y: 5.2 }
            XYPoint { x: 4.0; y: 3.3 }
        }
    }
}
