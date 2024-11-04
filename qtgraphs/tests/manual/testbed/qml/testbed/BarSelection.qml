// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtGraphs
import QtQuick.Controls.Basic

Rectangle {
    anchors.fill: parent
    color: "#404040"

    GraphsView {
        id: chartView
        anchors.fill: parent
        anchors.margins: 20 * px
        anchors.topMargin: 80 * px
        theme: GraphTheme {
            id: myTheme
            colorTheme: GraphTheme.ColorThemeDark
            axisXLabelsFont.pixelSize: 20
        }
        BarSeries {
            id: barSeries
            axisX: BarCategoryAxis { categories: ["2023", "2024", "2025", "2026"] }
            axisY: ValueAxis { }
            selectable: true
            BarSet { id: set1; label: "Axel"; values: [1, 2, 3, 4] }
            BarSet { id: set2; label: "Frank"; values: [8, 2, 6, 0] }
            BarSet { id: set3; label: "James"; values: [4+3*Math.sin(fA.elapsedTime), 5+3*Math.sin(fA.elapsedTime), 2, 3] }
            FrameAnimation {
                id: fA
                running: true
            }
        }
    }
    Text {
        id: text1
        y: 10
        x: 80
        color: "#ffffff"
        font.pixelSize: 20
        text: "Set 1, selected bars: " + set1.selectedBars + "\n" +
              "Set 2, selected bars: " + set2.selectedBars + "\n" +
              "Set 3, selected bars: " + set3.selectedBars;
    }

    SettingsView {
        Item {
            width: 260
            height: 10
        }
        Button {
            width: 250
            text: "Select All"
            onClicked: barSeries.selectAll();
        }
        Button {
            width: 250
            text: "Deselect All"
            onClicked: barSeries.deselectAll();
        }
        Button {
            width: 250
            text: "Select Set 1"
            onClicked: set1.selectAllBars();
        }
        Button {
            width: 250
            text: "Deselect Set 1"
            onClicked: set1.deselectAllBars();
        }
        Button {
            width: 250
            text: "Select Set 3, bar 0"
            onClicked: set3.selectBar(0);
        }
        Button {
            width: 250
            text: "Select Set 3, bars 1 and 2"
            onClicked: set3.selectBars([1, 2]);
        }
        Button {
            width: 250
            text: "Toggle Set 3, bars 1 and 2"
            onClicked: set3.toggleSelection([1, 2]);
        }
    }
}
