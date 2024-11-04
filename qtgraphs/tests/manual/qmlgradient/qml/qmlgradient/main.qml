// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtGraphs
import "."

Item {
    id: mainwindow

    function updateinfoLabels()
    {
        if (surfaceGraph.theme.baseGradients[0] === mainGradient)
            gradientLabel.text = "Main gradient";
        else if (surfaceGraph.theme.baseGradients[0] === secondaryGradient)
            gradientLabel.text = "Secondary gradient";
    }

    width: 1024
    height: 768
    visible: true

    Item {
        id: surfaceview
        width: mainwindow.width
        height: mainwindow.height

        anchors.top: mainwindow.top
        anchors.left: mainwindow.left

        Gradient {
            id: mainGradient
            GradientStop { position: 0.0; color: "red"}
            GradientStop { position: 0.5; color: "green"}
            GradientStop { position: 0.8; color: "blue"}
            GradientStop { position: 0.6; color: "yellow"}
            GradientStop { position: 0.8; color: "black"}
            GradientStop { position: 1.0; color: "peru"}
        }

        Gradient {
            id: secondaryGradient
            GradientStop { position: 0.0; color: "crimson"}
            GradientStop { position: 0.5; color: "chartreuse"}
            GradientStop { position: 0.8; color: "blueviolet"}
            GradientStop { position: 0.6; color: "gold"}
            GradientStop { position: 0.8; color: "darkslategrey"}
            GradientStop { position: 1.0; color: "seagreen"}
        }

        Gradient {
            id: seriesGradient
            GradientStop { position: 0.0; color: "gold" }
            GradientStop { position: 0.5; color: "crimson" }
            GradientStop { position: 1.0; color: "blueviolet" }
        }

        Theme3D {
            id: mainTheme
            type: Theme3D.Theme.StoneMoss

            colorStyle: Theme3D.ColorStyle.RangeGradient
            baseGradients: [mainGradient]
        }

        Theme3D {
            id: secondaryTheme
            type: Theme3D.Theme.ArmyBlue
            baseGradients: [secondaryGradient]
        }

        Surface3D {
            id: surfaceGraph
            width: surfaceview.width
            height: surfaceview.height
            theme: mainTheme

            shadowQuality: AbstractGraph3D.ShadowQuality.Medium
            selectionMode: AbstractGraph3D.SelectionSlice | AbstractGraph3D.SelectionItemAndRow
            cameraPreset: AbstractGraph3D.CameraPreset.IsometricLeft
            axisY.min: 0.0
            axisY.max: 500.0
            axisX.segmentCount: 10
            axisX.subSegmentCount: 2
            axisX.labelFormat: "%i"
            axisZ.segmentCount: 10
            axisZ.subSegmentCount: 2
            axisZ.labelFormat: "%i"
            axisY.segmentCount: 5
            axisY.subSegmentCount: 2
            axisY.labelFormat: "%i"
            axisY.title: "Height"
            axisX.title: "Latitude"
            axisZ.title: "Longitude"

            Surface3DSeries {
                id: heightSeries
                drawMode: Surface3DSeries.DrawSurface
                visible: true
                flatShadingEnabled: false

                HeightMapSurfaceDataProxy {
                    heightMapFile: ":/map"
                }
            }
        }

        RowLayout {
            id: buttonLayout
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Button {
                id: toggleTheme
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: qsTr("Toggle theme")
                onClicked: {
                    if (surfaceGraph.theme == mainTheme) {
                        surfaceGraph.theme = secondaryTheme;
                        themeLabel.text = "Secondary theme";
                        updateinfoLabels();
                    } else if (surfaceGraph.theme == secondaryTheme) {
                        surfaceGraph.theme = mainTheme;
                        updateinfoLabels();
                        themeLabel.text = "Main theme";
                    }
                }
            }

            Button {
                id: toggleGradient
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: qsTr("Toggle theme gradient")
                onClicked: {
                    if (surfaceGraph.theme.baseGradients[0] === mainGradient) {
                        surfaceGraph.theme.baseGradients[0] = secondaryGradient;
                        updateinfoLabels();
                    } else if (surfaceGraph.theme.baseGradients[0] === secondaryGradient) {
                        surfaceGraph.theme.baseGradients[0] = mainGradient;
                        updateinfoLabels();
                    }
                }
            }

            Button {
                id: toggleSeriesGradient
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: qsTr("Override theme gradient with series gradient")

                onClicked: {
                    heightSeries.baseGradient = seriesGradient;
                    gradientLabel.text = "Series gradient";
                }
            }
        }

        ColumnLayout {
            id: infoLayout
            anchors.top: buttonLayout.bottom
            anchors.left: parent.left

            Rectangle {
                Layout.minimumHeight: 20

                Label {
                    id: themeLabel
                    text: qsTr("Main theme")
                }
            }

            Rectangle {
                Layout.minimumHeight: 20

                Label {
                    id: gradientLabel
                    text: qsTr("Main gradient")
                }
            }
        }
    }
}
