// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtGraphs

Item {
    anchors.fill: parent

    Text {
        id: title
        text: "Simple Pie Graph"
        anchors.horizontalCenter: parent.horizontalCenter
        color: "#f0f0f0"
        y: parent.height * .1
    }

    GraphsView {
        id: chartView
        width: parent.width
        height: parent.height
        anchors.top: title.bottom
        anchors.bottom: parent.bottom

        property variant otherSlice: 0

        theme: GraphsTheme {
            id: myTheme
            colorScheme: GraphsTheme.ColorScheme.Dark
            theme: GraphsTheme.Theme.QtGreen
        }

        PieSeries {
            id: pieSeries
            PieSlice {
                label: "Volkswagen"
                labelVisible: true
                value: 13.5
            }
            PieSlice {
                label: "Toyota"
                labelVisible: true
                labelPosition: PieSlice.LabelPosition.InsideHorizontal
                labelColor: 'black'
                value: 10.9
            }
            PieSlice {
                label: "Ford"
                labelVisible: true
                labelPosition: PieSlice.LabelPosition.InsideNormal
                labelColor: 'black'
                value: 8.6
            }
            PieSlice {
                label: "Skoda"
                labelVisible: true
                labelPosition: PieSlice.LabelPosition.InsideTangential
                labelColor: 'black'
                value: 8.2
            }
            PieSlice {
                label: "Volvo"
                labelVisible: true
                value: 6.8
            }
        }

        Component.onCompleted: {
            otherSlice = pieSeries.append("Others", 52.0);
            otherSlice.labelVisible = true;
            pieSeries.find("Volkswagen").exploded = true;
        }
    }
}
