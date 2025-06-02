// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtGraphs
import QtQuick.Dialogs
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: mainView
    width: 800
    height: 600
    color: "#202020"

    RowLayout {
        id:bar
        height: 100

        Text {
            Layout.leftMargin: 20
            font.pixelSize: 24
            color: "#ffffff"
            text: "X:"
        }

        Slider {
            id: sliderX

            value: (new Date(1950,1,1)).getTime()
            from: (new Date(1900,1,1)).getTime()
            to: (new Date(2000,1,1)).getTime()
        }

        Text {
            font.pixelSize: 24
            color: "#ffffff"
            text: "Y:"
        }

        Slider {
            id: sliderY

            value: (new Date(1950,1,1)).getTime()
            from: (new Date(1900,1,1)).getTime()
            to: (new Date(2000,1,1)).getTime()
        }

        Text {
            Layout.leftMargin: 20
            font.pixelSize: 24
            color: "#ffffff"
            text: "X Ticks:"
        }

        SpinBox {
            onValueChanged: xAxis.tickInterval = value
        }

        Text {
            Layout.leftMargin: 20
            font.pixelSize: 24
            color: "#ffffff"
            text: "X Format:"
        }
        TextField {
            placeholderText: "MMMM-yyyy"
            onAccepted: xAxis.labelFormat = text
        }
    }

    GraphsView {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: bar.bottom
        anchors.margins: 10
        theme: gtheme

        GraphsTheme {
            id: gtheme
            axisYLabelFont.pixelSize: 8
            colorScheme: GraphsTheme.ColorScheme.Dark
            theme: GraphsTheme.Theme.QtGreen
        }

        axisX: DateTimeAxis {
            id: xAxis
            subTickCount: 2
            labelsAngle: 45
            labelFormat: "MMMM-yyyy"
            tickInterval: 0
            min: new Date(1930,12,31)
            max: new Date(sliderX.value)
        }

        axisY: DateTimeAxis {
            id: yAxis
            subTickCount: 2
            labelsAngle: 45
            labelFormat: "MMMM-yyyy"
            tickInterval: 10
            min: new Date(1930,12,31)
            max: new Date(sliderY.value)
        }

        ToolTip {
            id: tooltip
        }

        onHoverEnter: {
            tooltip.visible = true;
        }

        onHoverExit: {
            tooltip.visible = false;
        }

        onHover: (seriesName, position, value) => {
            tooltip.x = position.x + 1;
            tooltip.y = position.y + 1;
            tooltip.text = new Date(value.x).toString();
        }

        LineSeries {
            id: line
            width: 8
            hoverable: true

            XYPoint { x: new Date(1910, 2, 15);y: new Date(1900, 12, 31) }
            XYPoint { x: new Date(1915, 2, 15);y: new Date(1910, 12, 31) }
            XYPoint { x: new Date(1920, 1, 1); y: new Date(1920, 12, 31) }
            XYPoint { x: new Date(1930, 12, 31); y: new Date(1960, 12, 31) }
            XYPoint { x: new Date(1940, 7, 1); y: new Date(1940, 12, 31) }
            XYPoint { x: new Date(1950, 8, 2); y: new Date(1950, 12, 31) }
            XYPoint { x: new Date(1960, 8, 2); y: new Date(1960, 12, 31) }
        }

        SplineSeries {
            id: spline
            width: 4

            XYPoint { x: new Date(1910, 7, 1); y: new Date(1940, 12, 31) }
            XYPoint { x: new Date(1920, 8, 2); y: new Date(1950, 12, 31) }
            XYPoint { x: new Date(1930, 8, 2); y: new Date(1960, 12, 31) }
            XYPoint { x: new Date(1940, 2, 15);y: new Date(1900, 12, 31) }
            XYPoint { x: new Date(1955, 2, 15);y: new Date(1910, 12, 31) }
            XYPoint { x: new Date(1960, 1, 1); y: new Date(1920, 12, 31) }
            XYPoint { x: new Date(1970, 12, 31); y: new Date(1960, 12, 31) }
        }
    }
}
