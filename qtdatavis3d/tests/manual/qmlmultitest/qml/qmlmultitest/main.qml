// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtDataVisualization 1.2
import "."

Item {
    id: mainView
    width: 800
    height: 600

    Data {
        id: data
    }

    GridLayout {
        id: gridLayout
        columns: 2
        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors.top: mainView.top
        anchors.bottom: mainView.bottom
        anchors.left: mainView.left
        anchors.right: mainView.right

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.color: surfaceGraph.theme.gridLineColor
            border.width: 2
            color: "#00000000"

            Surface3D {
                id: surfaceGraph
                anchors.fill: parent
                anchors.margins: parent.border.width
                theme: Theme3D {
                    type: Theme3D.ThemePrimaryColors
                    font.pointSize: 60
                }
                scene.activeCamera.cameraPreset: Camera3D.CameraPresetIsometricLeftHigh

                Surface3DSeries {
                    itemLabelFormat: "Pop density at (@xLabel N, @zLabel E): @yLabel"
                    ItemModelSurfaceDataProxy {
                        id: surfaceProxy
                        itemModel: data.sharedData
                        // The surface data points are not neatly lined up in rows and columns,
                        // so we define explicit row and column roles.
                        rowRole: "coords"
                        columnRole: "coords"
                        xPosRole: "data"
                        zPosRole: "data"
                        yPosRole: "data"
                        rowRolePattern: /(\d),\d/
                        columnRolePattern: /(\d),(\d)/
                        xPosRolePattern: /^([asd]*)([fgh]*)([jkl]*)[^\/]*\/([^\/]*)\/.*$/
                        yPosRolePattern: /^([^\/]*)\/([^\/]*)\/(.*)$/
                        zPosRolePattern: /^([asd]*)([qwe]*)([tyu]*)([fgj]*)([^\/]*)\/[^\/]*\/.*$/
                        rowRoleReplace: "\\1"
                        columnRoleReplace: "\\2"
                        xPosRoleReplace: "\\4"
                        yPosRoleReplace: "\\3"
                        zPosRoleReplace: "\\5"
                    }
                }
            }
        }

        // We'll use one grid cell for buttons
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true

            GridLayout {
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                columns: 2

                Button {
                    Layout.minimumWidth: parent.width / 2
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: "Clear Selections"
                    onClicked: clearSelections() // call a helper function to keep button itself simpler
                }

                Button {
                    Layout.minimumWidth: parent.width / 2
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: "Quit"
                    onClicked: Qt.quit();
                }

                Button {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: "Reset Cameras"
                    onClicked: resetCameras() // call a helper function to keep button itself simpler
                }

                Button {
                    id: mmbButton
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: "MMB: Last"
                    onClicked: changeMMB() // call a helper function to keep button itself simpler
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.color: scatterGraph.theme.gridLineColor
            border.width: 2
            color: "#00000000"

            Scatter3D {
                id: scatterGraph
                anchors.fill: parent
                anchors.margins: parent.border.width
                theme: Theme3D {
                    type: Theme3D.ThemeDigia
                    font.pointSize: 60
                }
                scene.activeCamera.cameraPreset: Camera3D.CameraPresetIsometricLeftHigh

                Scatter3DSeries {
                    itemLabelFormat: "Pop density at (@xLabel N, @zLabel E): @yLabel"
                    mesh: Abstract3DSeries.MeshCube
                    ItemModelScatterDataProxy {
                        id: scatterProxy
                        itemModel: data.sharedData
                        // Mapping model roles to scatter series item coordinates.
                        xPosRole: "data"
                        zPosRole: "data"
                        yPosRole: "data"
                        rotationRole: "coords"
                        xPosRolePattern: /^([asd]*)([fgh]*)([jkl]*)[^\/]*\/([^\/]*)\/.*$/
                        yPosRolePattern: /^([^\/]*)\/([^\/]*)\/(.*)$/
                        zPosRolePattern: /^([asd]*)([qwe]*)([tyu]*)([fgj]*)([^\/]*)\/[^\/]*\/.*$/
                        rotationRolePattern: /(\d)\,(\d)/
                        xPosRoleReplace: "\\4"
                        yPosRoleReplace: "\\3"
                        zPosRoleReplace: "\\5"
                        rotationRoleReplace: "@\\2\\1,0,1,0"
                    }
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.color: barGraph.theme.gridLineColor
            border.width: 2
            color: "#00000000"

            Bars3D {
                id: barGraph
                anchors.fill: parent
                anchors.margins: parent.border.width
                theme: Theme3D {
                    type: Theme3D.ThemeQt
                    font.pointSize: 60
                }
                selectionMode: AbstractGraph3D.SelectionItemAndRow | AbstractGraph3D.SelectionSlice
                scene.activeCamera.cameraPreset: Camera3D.CameraPresetIsometricLeftHigh

                Bar3DSeries {
                    itemLabelFormat: "@seriesName: @valueLabel"
                    name: "Population density"

                    ItemModelBarDataProxy {
                        id: barProxy
                        itemModel: data.sharedData
                        // Mapping model roles to bar series rows, columns, and values.
                        rowRole: "coords"
                        columnRole: "coords"
                        valueRole: "data"
                        rotationRole: "coords"
                        rowRolePattern: /(\d),\d/
                        columnRolePattern: /(\d),(\d)/
                        valueRolePattern: /^([^\/]*)\/([^\/]*)\/(.*)$/
                        rotationRolePattern: /(\d)\,(\d)/
                        rowRoleReplace: "\\1"
                        columnRoleReplace: "\\2"
                        valueRoleReplace: "\\3"
                        rotationRoleReplace: "\\2\\1"
                    }
                }
            }
        }
    }

    function clearSelections() {
        barGraph.clearSelection()
        scatterGraph.clearSelection()
        surfaceGraph.clearSelection()
    }

    function resetCameras() {
        surfaceGraph.scene.activeCamera.cameraPreset = Camera3D.CameraPresetIsometricLeftHigh
        scatterGraph.scene.activeCamera.cameraPreset = Camera3D.CameraPresetIsometricLeftHigh
        barGraph.scene.activeCamera.cameraPreset = Camera3D.CameraPresetIsometricLeftHigh
        surfaceGraph.scene.activeCamera.zoomLevel = 100.0
        scatterGraph.scene.activeCamera.zoomLevel = 100.0
        barGraph.scene.activeCamera.zoomLevel = 100.0
    }

    function changeMMB() {
        if (barProxy.multiMatchBehavior === ItemModelBarDataProxy.MMBLast) {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MMBAverage
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MMBAverage
            mmbButton.text = "MMB: Average"
        } else if (barProxy.multiMatchBehavior === ItemModelBarDataProxy.MMBAverage) {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MMBCumulative
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MMBCumulativeY
            mmbButton.text = "MMB: Cumulative"
        } else if (barProxy.multiMatchBehavior === ItemModelBarDataProxy.MMBCumulative) {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MMBFirst
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MMBFirst
            mmbButton.text = "MMB: First"
        } else {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MMBLast
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MMBLast
            mmbButton.text = "MMB: Last"
        }
    }
}
