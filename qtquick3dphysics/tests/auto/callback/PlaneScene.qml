// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Physics

View3D {
    id: viewport
    property real elapsedTime: 0

    environment: SceneEnvironment {
        clearColor: "#d6dbdf"
        backgroundMode: SceneEnvironment.Color
    }

    PerspectiveCamera {
        position: Qt.vector3d(0, 200, 1000)
        clipFar: 2000
        clipNear: 1
    }

    DirectionalLight {
        eulerRotation.x: -45
        eulerRotation.y: 45
        castsShadow: true
        brightness: 1
        shadowFactor: 100
    }

    PhysicsWorld {
        id: world
        scene: viewport.scene
        //maximumTimestep: 0.1
        //minimumTimestep: 0.1
    }

    Connections {
        target: world
        function onFrameDone(timeStep) {
            viewport.elapsedTime += timeStep
        }
    }

    StaticRigidBody {
        position: Qt.vector3d(0, -100, 0)
        eulerRotation: Qt.vector3d(-90, 0, 0)
        collisionShapes: PlaneShape {}
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(500, 500, 0)
            materials: PrincipledMaterial {
                baseColor: "green"
            }
            castsShadows: false
            receivesShadows: true
        }
    }
}
