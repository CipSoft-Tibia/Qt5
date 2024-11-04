// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphs
//! [0]

Item {
    id: mainView
    width: 800
    height: 600

    property int iconDimension: 38
    property int margin: 30
    property int spacing: 10
    property int radius: 6
    property int buttonMinWidth: 175

    //! [1]
    Data {
        id: seriesData
    }
    //! [1]

    //! [2]
    Theme3D {
        id: themeQt
        type: Theme3D.Theme.Qt
        font.pointSize: 40
    }

    Theme3D {
        id: themeRetro
        type: Theme3D.Theme.Retro
    }
    //! [2]

    //! [5]
    component CustomButton : RoundButton {
        id: buttonRoot
        //! [5]
        //! [6]
        property alias source: iconImage.source

        Layout.minimumWidth: buttonMinWidth
        Layout.fillWidth: true

        radius: mainView.radius

        background: Rectangle {
            radius: mainView.radius
            color: "white"
            border.color: "black"
        }
        //! [6]
        //! [7]
        contentItem: Row {
            id :content
            IconImage {
                id: iconImage
                width: iconDimension
                height: iconDimension
                color: "transparent"
            }
            Label {
                text: buttonRoot.text
                horizontalAlignment: Text.AlignLeft
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        //! [7]
    }

    //! [3]
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: margin
        spacing: spacing
        //! [3]
        //! [4]
        GridLayout {
            Layout.fillWidth: true
            rowSpacing: spacing
            columnSpacing: spacing

            columns: mainView.width < mainView.buttonMinWidth * 2 + mainView.spacing + mainView.margin * 2 // width of 2 buttons
                     ? 1
                     : (mainView.width < mainView.buttonMinWidth * 4 + mainView.spacing * 3 + mainView.margin * 2 // width of 4 buttons
                        ? 2
                        : 4)
            //! [4]
            //! [8]
            CustomButton {
                id: shadowButton
                text: graph.shadowQuality === AbstractGraph3D.ShadowQuality.None ?
                          qsTr("Show Shadows") : qsTr("Hide Shadows")
                source: graph.shadowQuality === AbstractGraph3D.ShadowQuality.None ?
                            "qrc:/images/shadow.svg" : "qrc:/images/shadow_hide.svg"
                onClicked: {
                    graph.shadowQuality = graph.shadowQuality === AbstractGraph3D.ShadowQuality.None ?
                                AbstractGraph3D.ShadowQuality.High :
                                AbstractGraph3D.ShadowQuality.None
                }
            }
            //! [8]

            CustomButton {
                id: smoothButton
                text: qsTr("Smooth Series One")
                source: graph.meshSmooth ?
                            "qrc:/images/flatten.svg" : "qrc:/images/smooth_curve.svg"
                onClicked: {
                    text = graph.meshSmooth ? qsTr("Smooth Series One") : qsTr("Flat Series One")
                    graph.meshSmooth = !graph.meshSmooth
                }
            }

            CustomButton {
                id: cameraButton
                text: qsTr("Camera Placement")
                source: graph.cameraPreset === AbstractGraph3D.CameraPreset.Front ?
                                        "qrc:/images/camera.svg" : "qrc:/images/camera2.svg"
                onClicked: {
                    graph.cameraPreset = graph.cameraPreset === AbstractGraph3D.CameraPreset.Front ?
                                AbstractGraph3D.CameraPreset.IsometricRightHigh :
                                AbstractGraph3D.CameraPreset.Front
                }
            }

            CustomButton {
                id: backgroundButton
                text: qsTr("Hide Background")
                source: graph.theme.backgroundEnabled ?
                                        "qrc:/images/background_hide.svg" : "qrc:/images/background.svg"
                onClicked: {
                    graph.theme.backgroundEnabled = !graph.theme.backgroundEnabled
                    text = graph.theme.backgroundEnabled ? qsTr("Hide Background") : qsTr("Show Background")
                }
            }
        }

        //! [9]
        Graph {
            id: graph
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        //! [9]

        //! [10]
        CustomButton {
            id: themeButton
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: false
            //! [10]
            text: qsTr("Change Theme")
            source: "qrc:/images/theme.svg"
            onClicked: {
                graph.theme = graph.theme.type === Theme3D.Theme.Retro ? themeQt : themeRetro
                backgroundButton.text = graph.theme.backgroundEnabled ? qsTr("Hide Background") : qsTr("Show Background")
            }
        }
    }
}
