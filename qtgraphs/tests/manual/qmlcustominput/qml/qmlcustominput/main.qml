// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtGraphs
import "."

Item {
    id: mainView
    width: 1280
    height: 720

    Data {
        id: graphData
    }

    Item {
        id: dataView
        anchors.bottom: parent.bottom
        width: parent.width
        height: parent.height - buttonLayout.height

        Scatter3D {
            id: scatterGraph
            width: dataView.width
            height: dataView.height
            theme: Theme3D { type: Theme3D.Theme.Ebony }
            shadowQuality: AbstractGraph3D.ShadowQuality.Medium
            cameraYRotation: 30.0
            inputHandler: null

            Scatter3DSeries {
                id: scatterSeriesOne
                itemLabelFormat: "One - X:@xLabel Y:@yLabel Z:@zLabel"
                mesh: Abstract3DSeries.Mesh.Cube

                ItemModelScatterDataProxy {
                    itemModel: graphData.modelOne
                    xPosRole: "xPos"
                    yPosRole: "yPos"
                    zPosRole: "zPos"
                }
            }

            Scatter3DSeries {
                id: scatterSeriesTwo
                itemLabelFormat: "Two - X:@xLabel Y:@yLabel Z:@zLabel"
                mesh: Abstract3DSeries.Mesh.Cube

                ItemModelScatterDataProxy {
                    itemModel: graphData.modelTwo
                    xPosRole: "xPos"
                    yPosRole: "yPos"
                    zPosRole: "zPos"
                }
            }

            Scatter3DSeries {
                id: scatterSeriesThree
                itemLabelFormat: "Three - X:@xLabel Y:@yLabel Z:@zLabel"
                mesh: Abstract3DSeries.Mesh.Cube

                ItemModelScatterDataProxy {
                    itemModel: graphData.modelThree
                    xPosRole: "xPos"
                    yPosRole: "yPos"
                    zPosRole: "zPos"
                }
            }

            onQueriedGraphPositionChanged:
                console.log("Queried Position : " + queriedGraphPosition)
        }

        MouseArea {
            id: inputArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onWheel: (wheel)=> {
                // Adjust zoom level based on what zoom range we're in.
                var zoomLevel = scatterGraph.zoomLevel;
                if (zoomLevel > 100)
                    zoomLevel += wheel.angleDelta.y / 12.0;
                else if (zoomLevel > 50)
                    zoomLevel += wheel.angleDelta.y / 60.0;
                else
                    zoomLevel += wheel.angleDelta.y / 120.0;
                if (zoomLevel > 500)
                    zoomLevel = 500;
                else if (zoomLevel < 10)
                    zoomLevel = 10;

                scatterGraph.zoomLevel = zoomLevel;
            }
            onClicked: {
                console.log("Queried at: " + Qt.point(mouseX, mouseY))
                scatterGraph.scene.graphPositionQuery = Qt.point(mouseX, mouseY)
            }
        }

        Timer {
            id: reselectTimer
            interval: 33
            running: true
            repeat: true
            onTriggered: {
                scatterGraph.scene.selectionQueryPosition = Qt.point(-1, -1);
                scatterGraph.scene.selectionQueryPosition = Qt.point(inputArea.mouseX, inputArea.mouseY);
            }
        }
    }

    NumberAnimation {
        id: cameraAnimationX
        loops: Animation.Infinite
        running: true
        target: scatterGraph
        property:"cameraXRotation"
        from: 0.0
        to: 360.0
        duration: 20000
    }

    SequentialAnimation {
        id: cameraAnimationY
        loops: Animation.Infinite
        running: true

        NumberAnimation {
            target: scatterGraph
            property:"cameraYRotation"
            from: 5.0
            to: 45.0
            duration: 9000
            easing.type: Easing.InOutSine
        }

        NumberAnimation {
            target: scatterGraph
            property:"cameraYRotation"
            from: 45.0
            to: 5.0
            duration: 9000
            easing.type: Easing.InOutSine
        }
    }

    RowLayout {
        id: buttonLayout
        Layout.minimumHeight: shadowToggle.height
        width: parent.width
        anchors.left: parent.left
        spacing: 0

        Button {
            id: shadowToggle
            Layout.fillHeight: true
            Layout.minimumWidth: parent.width / 3 // 3 buttons divided equally in the layout
            text: "Hide Shadows"
            onClicked: {
                if (scatterGraph.shadowQuality === AbstractGraph3D.ShadowQuality.None) {
                    scatterGraph.shadowQuality = AbstractGraph3D.ShadowQuality.Medium;
                    text = "Hide Shadows";
                } else {
                    scatterGraph.shadowQuality = AbstractGraph3D.ShadowQuality.None;
                    text = "Show Shadows";
                }
            }
        }

        Button {
            id: cameraToggle
            Layout.fillHeight: true
            Layout.minimumWidth: parent.width / 3
            text: "Pause Camera"

            onClicked: {
                cameraAnimationX.paused = !cameraAnimationX.paused;
                cameraAnimationY.paused = cameraAnimationX.paused;
                if (cameraAnimationX.paused) {
                    text = "Animate Camera";
                } else {
                    text = "Pause Camera";
                }
            }
        }

        Button {
            id: exitButton
            Layout.fillHeight: true
            Layout.minimumWidth: parent.width / 3
            text: "Quit"
            onClicked: Qt.quit();
        }
    }
}
