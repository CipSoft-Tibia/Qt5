// Copyright (C) 2020 The Qt Company.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.2
import QtQuick.Window 2.12 as Win
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Input 2.0
import Qt3D.Extras 2.0

import QtQuick.Scene3D 2.0

Win.Window {
    id: win
    width: 300
    height: 350
    visible: true
    x: Win.Screen.width / 2 - width / 2
    y: Win.Screen.height / 2 - height / 2
    Item {
        id: mioitem
        width: 300
        height: 300
        Scene3D {
            id: scene3d
            anchors.fill: parent
            Entity {
                id: sceneRoot

                Camera {
                    id: camera
                    projectionType: CameraLens.PerspectiveProjection
                    fieldOfView: 45
                    aspectRatio: 16/9
                    nearPlane : 0.1
                    farPlane : 1000.0
                    position: Qt.vector3d( 0.0, 0.0, -40.0 )
                    upVector: Qt.vector3d( 0.0, 1.0, 0.0 )
                    viewCenter: Qt.vector3d( 0.0, 0.0, 0.0 )
                }

                OrbitCameraController {
                    camera: camera
                }

                components: [
                    RenderSettings {
                        activeFrameGraph: ForwardRenderer {
                            clearColor: Qt.rgba(0, 0.5, 1, 1)
                            camera: camera
                        }
                    },
                    // Event Source will be set by the Qt3DQuickWindow
                    InputSettings { }
                ]

                PhongMaterial {
                    id: material
                }

                TorusMesh {
                    id: torusMesh
                    radius: 5
                    minorRadius: 1
                    rings: 100
                    slices: 20
                }

                Transform {
                    id: torusTransform
                    scale3D: Qt.vector3d(1.5, 1, 0.5)
                    rotation: fromAxisAndAngle(Qt.vector3d(1, 0, 0), 45)
                }

                Entity {
                    id: torusEntity
                    components: [ torusMesh, material, torusTransform ]
                }

                SphereMesh {
                    id: sphereMesh
                    radius: 3
                }

                Transform {
                    id: sphereTransform
                    property real userAngle: 0.0
                    matrix: {
                        var m = Qt.matrix4x4();
                        m.rotate(userAngle, Qt.vector3d(0, 1, 0));
                        m.translate(Qt.vector3d(20, 0, 0));
                        return m;
                    }
                }

                NumberAnimation {
                    target: sphereTransform
                    property: "userAngle"
                    duration: 10000
                    from: 0
                    to: 360

                    loops: Animation.Infinite
                    running: true
                }

                Entity {
                    id: sphereEntity
                    components: [ sphereMesh, material, sphereTransform ]
                }
            }
        }
    }
    Rectangle {
        height: 50
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "yellow"
        MouseArea {
            anchors.fill: parent
            z: 5
            onClicked: {
                win.screen = (win.screen === Qt.application.screens[0] ? Qt.application.screens[1] : Qt.application.screens[0])
            }
        }
        Text {
            anchors.centerIn: parent
            minimumPointSize: 12
            fontSizeMode: Text.Fit
            text: "Move to the other screen"
        }
    }
}
