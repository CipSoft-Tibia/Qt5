// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.4 as QQ2
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Extras 2.0

Entity {
    id: root

    components: RenderSettings {
        StereoFrameGraph {
            id: stereoFrameGraph
            leftCamera: stereoCamera.leftCamera
            rightCamera: stereoCamera.rightCamera
        }
    }

    // Camera
    StereoCamera {
        id: stereoCamera
        property real circleRotation: 0
        readonly property real cameraRadius: obstaclesRepeater.radius - 50
        readonly property vector3d circlePosition: Qt.vector3d(cameraRadius * Math.cos(circleRotation), 0.0, cameraRadius * Math.sin(circleRotation))
        readonly property vector3d tan: circlePosition.crossProduct(Qt.vector3d(0, 1, 0).normalized())
        viewCenter: planeTransform.translation
        position: circlePosition.plus(Qt.vector3d(0, 45 * Math.sin(circleRotation * 2), 0)).plus(tan.times(-2))

        QQ2.NumberAnimation {
            target: stereoCamera
            property: "circleRotation"
            from: 0; to: Math.PI * 2
            duration: 10000
            loops: QQ2.Animation.Infinite
            running: true
        }
    }

    // Skybox
    SkyboxEntity {
        baseName: "qrc:/assets/cubemaps/miramar/miramar"
        extension: ".webp"
    }

    // Cylinder
    Entity {
        property CylinderMesh cylinder: CylinderMesh {
            radius: 1
            length: 3
            rings: 100
            slices: 20
        }
        property Transform transform: Transform {
            id: cylinderTransform
            property real theta: 0.0
            property real phi: 0.0
            rotation: fromEulerAngles(theta, phi, 0)
        }
        property Material phong: PhongMaterial {}

        QQ2.ParallelAnimation {
            loops: QQ2.Animation.Infinite
            running: true
            QQ2.SequentialAnimation {
                QQ2.NumberAnimation {
                    target: cylinderTransform
                    property: "scale"
                    from: 5; to: 45
                    duration: 2000
                    easing.type: QQ2.Easing.OutInQuad
                }
                QQ2.NumberAnimation {
                    target: cylinderTransform
                    property: "scale"
                    from: 45; to: 5
                    duration: 2000
                    easing.type: QQ2.Easing.InOutQuart
                }
            }
            QQ2.NumberAnimation {
                target: cylinderTransform
                property: "phi"
                from: 0; to: 360
                duration: 4000
            }
            QQ2.NumberAnimation {
                target: cylinderTransform
                property: "theta"
                from: 0; to: 720
                duration: 4000
            }
        }

        components: [cylinder, transform, phong]
    }

    // AirPlane
    Entity {
        components: [
            Mesh {
                source: "assets/obj/toyplane.obj"
            },
            Transform {
                id: planeTransform
                property real rollAngle: 0.0
                translation: Qt.vector3d(Math.sin(stereoCamera.circleRotation * -2) * obstaclesRepeater.radius,
                                         0.0,
                                         Math.cos(stereoCamera.circleRotation * -2) * obstaclesRepeater.radius)
                rotation: fromAxesAndAngles(Qt.vector3d(1.0, 0.0, 0.0), planeTransform.rollAngle,
                                            Qt.vector3d(0.0, 1.0, 0.0), stereoCamera.circleRotation * -2 * 180 / Math.PI + 180)
            },
            PhongMaterial {
                shininess: 20.0
                diffuse: "#ba1a02" // Inferno Orange
            }
        ]

        QQ2.SequentialAnimation {
            running: true
            loops: QQ2.Animation.Infinite

            QQ2.NumberAnimation {
                target: planeTransform
                property: "rollAngle"
                from: 30; to: 45
                duration: 750
            }
            QQ2.NumberAnimation {
                target: planeTransform
                property: "rollAngle"
                from: 45; to: 25
                duration: 500
            }
            QQ2.NumberAnimation {
                target: planeTransform
                property: "rollAngle"
                from: 25; to: 390
                duration: 800
            }
        }
    }

    // Torus obsctacles
    NodeInstantiator {
        id: obstaclesRepeater
        model: 4
        readonly property real radius: 130.0;
        readonly property real det: 1.0 / model
        delegate: Entity {
            components: [
                TorusMesh {
                    radius: 35
                    minorRadius: 5
                    rings: 100
                    slices: 20
                },
                Transform {
                    id: transform
                    readonly property real angle: Math.PI * 2.0 * index * obstaclesRepeater.det
                    translation: Qt.vector3d(obstaclesRepeater.radius * Math.cos(transform.angle),
                                             0.0,
                                             obstaclesRepeater.radius * Math.sin(transform.angle))
                    rotation: fromAxisAndAngle(Qt.vector3d(0.0, 1.0, 0.0), transform.angle * 180 / Math.PI)
                },
                PhongMaterial {
                    diffuse: Qt.rgba(Math.abs(Math.cos(transform.angle)), 204 / 255, 75 / 255, 1)
                    specular: "white"
                    shininess: 20.0
                }
            ]
        }
    }
}

