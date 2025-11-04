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

    property real maxSegmentCount: 10
    property real minSegmentCount: 1

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
            theme: GraphsTheme { theme: GraphsTheme.Theme.QtGreen }
            shadowQuality: Graphs3D.ShadowQuality.Medium
            cameraYRotation: 30.0

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

            Component.onCompleted: {
                scatterGraph.unsetDefaultInputHandler();
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
                if (scatterGraph.shadowQuality === Graphs3D.ShadowQuality.None) {
                    scatterGraph.shadowQuality = Graphs3D.ShadowQuality.Medium;
                    text = "Hide Shadows";
                } else {
                    scatterGraph.shadowQuality = Graphs3D.ShadowQuality.None;
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

    RowLayout {
        id: sliderLayout
        width: buttonLayout.width / 3
        anchors.top: buttonLayout.bottom
        anchors.left: parent.left
        spacing: 10

        Column {
            id: xColumn
            Label {
                text: "X Axis Segment Sliders"
            }

            Slider {
                id: xSegmentCountSlider
                snapMode: Slider.SnapAlways
                from: mainView.minSegmentCount
                to: mainView.maxSegmentCount
                value: 5

                onValueChanged: scatterGraph.axisX.segmentCount = value
            }

            Slider {
                id: xSubSegmentCountSlider
                from: mainView.minSegmentCount
                to: mainView.maxSegmentCount
                value: 1

                onValueChanged: scatterGraph.axisX.subSegmentCount = value
            }
        }

        Column {
            id: yColumn
            Label {
                text: "Y Axis Segment Sliders"
            }

            Slider {
                id: ySegmentCountSlider
                clip: true
                from: mainView.minSegmentCount
                to: mainView.maxSegmentCount
                value: 5

                onValueChanged: scatterGraph.axisY.segmentCount = value
            }

            Slider {
                id: ySubSegmentCountSlider
                from: mainView.minSegmentCount
                to: mainView.maxSegmentCount
                value: 1

                onValueChanged: scatterGraph.axisY.subSegmentCount = value
            }
        }

        Column {
            id: zColumn
            Label {
                text: "Z Axis Segment Sliders"
            }

            Slider {
                id: zSegmentCountSlider
                from: mainView.minSegmentCount
                to: mainView.maxSegmentCount
                value: 5

                onValueChanged: scatterGraph.axisZ.segmentCount = value
            }

            Slider {
                id: zSubSegmentCountSlider
                from: mainView.minSegmentCount
                to: mainView.maxSegmentCount
                value: 1

                onValueChanged: scatterGraph.axisZ.subSegmentCount = value
            }
        }
    }
}
