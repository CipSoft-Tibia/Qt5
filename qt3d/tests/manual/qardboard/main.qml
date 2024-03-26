// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Extras 2.0
import QtQuick 2.4 as QQ2

QardboardRootEntity {
    id: root
    cameraPosition: cameraController.position

    Entity {
        id: cameraController

        function posAtAngle(a) {
            return Qt.vector3d(cameraRadius * Math.cos(a), 0.0, cameraRadius * Math.sin(a))
        }
        function rotAtAngle(a) {
            return a * 180 / Math.PI + 90
        }

        property real circleRotation: 0
        readonly property real cameraRadius: obstaclesRepeater.radius
        readonly property vector3d circlePosition: cameraController.posAtAngle(cameraController.circleRotation)
        property vector3d position: planeTransform.translation.plus(Qt.vector3d(0, 0, 35))

        QQ2.NumberAnimation {
            target: cameraController
            property: "circleRotation"
            from: 0; to: Math.PI * 2
            duration: 10000
            loops: QQ2.Animation.Infinite
            running: true
        }
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
                translation: cameraController.posAtAngle(cameraController.circleRotation)
                rotation: fromAxesAndAngles(Qt.vector3d(1.0, 0.0, 0.0), planeTransform.rollAngle,
                                            Qt.vector3d(0.0, -1.0, 0.0), cameraController.rotAtAngle(cameraController.circleRotation))
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
                from: -30; to: 45
                duration: 1500
            }
            QQ2.NumberAnimation {
                target: planeTransform
                property: "rollAngle"
                from: 45; to: -30
                duration: 3500
            }
        }
    }

    // Cylinder
    Entity {
        property GeometryRenderer cylinder: CylinderMesh {
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
