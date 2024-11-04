// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtGraphs
import QtQuick.Controls.Basic

Rectangle {
    id: mainView
    color: "#404040"

    // Reset the set colors to transparent
    // so theme color is used instead.
    function resetCustomSetColors() {
        set1.color = Qt.rgba(0, 0, 0, 0);
        set2.color = Qt.rgba(0, 0, 0, 0);
        set3.color = Qt.rgba(0, 0, 0, 0);
        set4.color = Qt.rgba(0, 0, 0, 0);
        set1.borderColor = Qt.rgba(0, 0, 0, 0);
        set2.borderColor = Qt.rgba(0, 0, 0, 0);
        set3.borderColor = Qt.rgba(0, 0, 0, 0);
        set4.borderColor = Qt.rgba(0, 0, 0, 0);
        set1.borderWidth = -1;
        set2.borderWidth = -1;
        set3.borderWidth = -1;
        set4.borderWidth = -1;
    }

    function resetCustomGraphTheme() {
        myTheme.gridSmoothing = 1;
        myTheme.axisYLabelsFont.family = undefined;
    }

    FontLoader {
        id: customFont
        source: "images/Sevillana-Regular.ttf"
    }

    Text {
        id: graphToolbarTitle
        anchors.horizontalCenter: graphToolbar.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        text: "Graph Theme"
        font.pixelSize: 12
        color: "#ffffff"
    }

    Row {
        id: graphToolbar
        anchors.top: graphToolbarTitle.bottom
        anchors.margins: 10
        anchors.left: parent.left
        anchors.leftMargin: 60
        spacing: 10
        Button {
            text: "Dark"
            onClicked: {
                mainView.resetCustomGraphTheme();
                background.color = "#202020";
                myTheme.colorTheme = GraphTheme.ColorThemeDark;
            }
        }
        Button {
            text: "Light"
            onClicked: {
                mainView.resetCustomGraphTheme();
                background.color = "#eeeeee";
                myTheme.colorTheme = GraphTheme.ColorThemeLight;
            }
        }
        Button {
            text: "Customize!"
            onClicked: {
                myTheme.gridMajorBarsColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.gridMajorBarsWidth = 1 + 3 * Math.random();
                myTheme.gridMinorBarsColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.gridMinorBarsWidth = 1 + 2 * Math.random();
                myTheme.gridSmoothing = 2 * Math.random();
                myTheme.axisYMajorColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.axisXMajorColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.axisYMinorColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.axisXMinorColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.axisYLabelsColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.axisXLabelsColor = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                myTheme.axisYMajorBarWidth = 1 + 2 * Math.random();
                myTheme.axisXMajorBarWidth = 1 + 2 * Math.random();
                myTheme.axisYMinorBarWidth = 1 + 1 * Math.random();
                myTheme.axisXMinorBarWidth = 1 + 1 * Math.random();
                myTheme.axisYSmoothing = 2 * Math.random();
                myTheme.axisYLabelsFont.family = customFont.font.family;
            }
        }
    }

    Text {
        id: seriesToolbarTitle
        anchors.horizontalCenter: seriesToolbar.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        text: "Series Theme"
        font.pixelSize: 12
        color: "#ffffff"
    }

    Row {
        id: seriesToolbar
        anchors.top: seriesToolbarTitle.bottom
        anchors.margins: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        spacing: 10
        Button {
            text: "Theme1"
            onClicked: {
                mainView.resetCustomSetColors();
                mySeries.theme = seriesTheme;
                seriesTheme.colorTheme = SeriesTheme.SeriesTheme1;
            }
        }
        Button {
            text: "Theme2"
            onClicked: {
                mainView.resetCustomSetColors();
                mySeries.theme = seriesTheme;
                seriesTheme.colorTheme = SeriesTheme.SeriesTheme2;
            }
        }
        Button {
            text: "CustomBW"
            onClicked: {
                mainView.resetCustomSetColors();
                mySeries.theme = customSeriesTheme;
            }
        }
        Button {
            text: "Customize!"
            onClicked: {
                set1.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                set2.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                set3.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                set4.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1.0);
                // Borders a bit lighter/darker than main colors
                var borderMod = 0.5 + Math.random();
                set1.borderColor = Qt.lighter(set1.color, borderMod);
                set2.borderColor = Qt.lighter(set2.color, borderMod);
                set3.borderColor = Qt.lighter(set3.color, borderMod);
                set4.borderColor = Qt.lighter(set4.color, borderMod);
                set1.borderWidth = 2;
                set2.borderWidth = 2;
                set3.borderWidth = 2;
                set4.borderWidth = 2;
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
        anchors.top: graphToolbar.bottom
        anchors.margins: 10
        theme: GraphTheme {
            id: myTheme
            colorTheme: GraphTheme.ColorThemeDark
            axisXLabelsFont.pixelSize: 20
            axisYLabelsFont.pixelSize: 16
        }
        SeriesTheme {
            id: customSeriesTheme
            colors: ["#444444", "#555555", "#666666", "#777777"]
            borderColors: ["#888888", "#999999", "#aaaaaa", "#bbbbbb"]
            borderWidth: 4
        }
        BarSeries {
            id: mySeries
            axisX: BarCategoryAxis { categories: ["2007", "2008", "2009", "2010", "2011", "2012"] }
            axisY: ValueAxis {
                minorTickCount: 4
                max: 20
            }
            theme: SeriesTheme {
                id: seriesTheme
                colorTheme: SeriesTheme.SeriesTheme1
            }
            BarSet { id: set1; label: "Bob"; values: [1, 2, 3, 4, 5, 6] }
            BarSet { id: set2; label: "Susan"; values: [5, 1, 2, 4, 1, 7] }
            BarSet { id: set3; label: "James"; values: [4+3*Math.sin(fA.elapsedTime), 5+5*Math.sin(fA.elapsedTime), 6+2*Math.sin(fA.elapsedTime), 13+2*Math.sin(fA.elapsedTime), 4+3*Math.sin(fA.elapsedTime), 8+4*Math.sin(fA.elapsedTime)] }
            BarSet { id: set4; label: "Frank"; values: [3, 3, 5, 8, 4, 2] }
        }
        FrameAnimation {
            id: fA
            running: true
            onTriggered: chartView.update();
        }
    }

    MouseArea {
        anchors.fill: chartView
        onClicked: fA.paused = !fA.paused;
    }

}
