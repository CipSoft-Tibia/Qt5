// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Component {
    Node {
        property color lineColor: "green"
        property bool isPolar: false
        property real polarRadius: 0.0
        property real lineWidth: 1

        Model {
            visible: !isPolar
            source: "#Cylinder"
            depthBias: 1
            eulerRotation: Qt.vector3d(0., 0., 90.)
            castsShadows: false
            scale: Qt.vector3d(1., 1., 1.5)
            materials: [ DefaultMaterial {
                    diffuseColor: lineColor
                    specularAmount: 0.0
                }
            ]
        }

        Rectangle {
            x : -(polarRadius * .5)
            y : -(polarRadius * .5)
            width: polarRadius
            height: polarRadius
            radius: width / 2.0
            border.color: lineColor
            border.width: lineWidth
            color: 'transparent'
            visible: isPolar
            antialiasing: false
        }
    }
}
