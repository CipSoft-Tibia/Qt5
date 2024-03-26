// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.0 as QQ2
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Input 2.0
import Qt3D.Extras 2.0

Entity {
    id : root
    readonly property Camera camera: camera
    readonly property Layer layer: sceneLayer

    readonly property vector3d light1Pos : sphere1.transform.translation
    readonly property vector3d light2Pos : sphere2.transform.translation
    readonly property vector3d light3Pos : light3Transform.translation
    readonly property vector3d light4Pos : Qt.vector3d(5, 2, 7)

    property PointLight light: PointLight {
        color : "white"
        intensity : 0.0
        QQ2.ColorAnimation on color { from: "white"; to: "blue"; duration: 4000; loops: 2 }
        QQ2.SequentialAnimation on intensity {
            QQ2.NumberAnimation {
                from: 0; to: 3.0; duration: 500
            }
            QQ2.NumberAnimation {
                from: 3.0; to: 0.0; duration: 500
            }
            loops: QQ2.Animation.Infinite
        }
    }

    // Global elements
    Camera {
        id: camera
        projectionType: CameraLens.PerspectiveProjection
        fieldOfView: 45
        aspectRatio: 16/9
        nearPlane : 1.0
        farPlane : 1000.0
        position: Qt.vector3d( 0.0, 0.0, -25.0 )
        upVector: Qt.vector3d( 0.0, 1.0, 0.0 )
        viewCenter: Qt.vector3d( 0.0, 0.0, 10.0 )
    }

    SphereMesh {
        id : sphereMesh
        rings: 50
        slices: 100
    }

    SceneEffect { id : sceneMaterialEffect }

    Layer { id: sceneLayer }

    // Scene
    Entity {
        id : sphere1

        property Material material : Material {
            effect : sceneMaterialEffect
            parameters : Parameter { name : "meshColor"; value : "dodgerblue" }
        }

        property Transform transform: Transform {
            id: sphere1Transform
            property real x: -10.0
            translation: Qt.vector3d(x, 0, 5)
        }

        QQ2.SequentialAnimation {
            loops: QQ2.Animation.Infinite
            running: true
            QQ2.NumberAnimation { target: sphere1Transform; property: "x"; to: 3; duration: 2000 }
            QQ2.NumberAnimation { target: sphere1Transform; property: "x"; to: -5; duration: 2000 }
        }

        components : [
            sphereMesh,
            sphere1.material,
            sphere1.transform,
            sceneLayer
        ]
    }

    Entity {
        id : sphere2

        property Material material : Material {
            effect : sceneMaterialEffect
            parameters : Parameter { name : "meshColor"; value : "green" }
        }

        property Transform transform: Transform {
            translation: Qt.vector3d(5, 0, 5)
        }

        components : [
            sphereMesh,
            sphere2.transform,
            sphere2.material,
            sceneLayer
        ]
    }

    Entity {
        id: light3

        property Material material : Material {
            effect : sceneMaterialEffect
            parameters : Parameter { name : "meshColor"; value : "red" }
        }

        property Transform transform: Transform {
            id: light3Transform
            property real y: 2.0
            translation: Qt.vector3d(2, y, 7)
        }

        QQ2.SequentialAnimation {
            loops: QQ2.Animation.Infinite
            running: true
            QQ2.NumberAnimation { target: light3Transform; property: "y"; to: 6; duration: 1000; easing.type: QQ2.Easing.InOutQuad }
            QQ2.NumberAnimation { target: light3Transform; property: "y"; to: -6; duration: 1000; easing.type: QQ2.Easing.InOutQuint }
        }

        components: [
            sphereMesh,
            light3.material,
            light3.transform,
            sceneLayer
        ]
    }
}
