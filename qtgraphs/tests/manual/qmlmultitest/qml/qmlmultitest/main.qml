// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtGraphs
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
                    type: Theme3D.Theme.PrimaryColors
                    font.pointSize: 60
                }
                cameraPreset: AbstractGraph3D.CameraPreset.IsometricLeftHigh

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
                    type: Theme3D.Theme.PrimaryColors
                    font.pointSize: 60
                }
                cameraPreset: AbstractGraph3D.CameraPreset.IsometricLeftHigh

                Scatter3DSeries {
                    itemLabelFormat: "Pop density at (@xLabel N, @zLabel E): @yLabel"
                    mesh: Abstract3DSeries.Mesh.Cube
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
                    type: Theme3D.Theme.Qt
                    font.pointSize: 60
                }
                selectionMode: AbstractGraph3D.SelectionItemAndRow | AbstractGraph3D.SelectionSlice
                cameraPreset: AbstractGraph3D.CameraPreset.IsometricLeftHigh

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
        surfaceGraph.cameraPreset = AbstractGraph3D.CameraPreset.IsometricLeftHigh
        scatterGraph.cameraPreset = AbstractGraph3D.CameraPreset.IsometricLeftHigh
        barGraph.cameraPreset = AbstractGraph3D.CameraPreset.IsometricLeftHigh
        surfaceGraph.zoomLevel = 100.0
        scatterGraph.zoomLevel = 100.0
        barGraph.zoomLevel = 100.0
    }

    function changeMMB() {
        if (barProxy.multiMatchBehavior === ItemModelBarDataProxy.MultiMatchBehavior.Last) {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MultiMatchBehavior.Average
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MultiMatchBehavior.Average
            mmbButton.text = "MMB: Average"
        } else if (barProxy.multiMatchBehavior === ItemModelBarDataProxy.MultiMatchBehavior.Average) {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MultiMatchBehavior.Cumulative
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MultiMatchBehavior.CumulativeY
            mmbButton.text = "MMB: Cumulative"
        } else if (barProxy.multiMatchBehavior === ItemModelBarDataProxy.MultiMatchBehavior.Cumulative) {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MultiMatchBehavior.First
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MultiMatchBehavior.First
            mmbButton.text = "MMB: First"
        } else {
            barProxy.multiMatchBehavior = ItemModelBarDataProxy.MultiMatchBehavior.Last
            surfaceProxy.multiMatchBehavior = ItemModelSurfaceDataProxy.MultiMatchBehavior.Last
            mmbButton.text = "MMB: Last"
        }
    }
}
