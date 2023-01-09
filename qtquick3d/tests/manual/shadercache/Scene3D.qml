/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Window 2.12
import QtQuick3D 1.15

View3D {
    property Material material
    renderMode: View3D.Offscreen
    environment: SceneEnvironment {
        clearColor: "#808080"
        backgroundMode: SceneEnvironment.Color
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    PerspectiveCamera {
        position: Qt.vector3d(0, 400, 600)
        eulerRotation.x: -30
        clipFar: 2000
    }

    //! [directional light]
    DirectionalLight {
        id: light1
        color: Qt.rgba(1.0, 0.1, 0.1, 1.0)
        ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        position: Qt.vector3d(0, 200, 0)
        rotation: Quaternion.fromEulerAngles(-135, -90, 0)
        shadowMapQuality: Light.ShadowMapQualityHigh
        visible: true
        castsShadow: true
        brightness: 100
        SequentialAnimation on rotation {
            loops: Animation.Infinite
            QuaternionAnimation {
                to: Quaternion.fromEulerAngles(-45, -90, 0)
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            QuaternionAnimation {
                to: Quaternion.fromEulerAngles(-135, -90, 0)
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }
    }
    //! [directional light]

    //! [point light]
    PointLight {
        id: light2
        color: Qt.rgba(0.1, 1.0, 0.1, 1.0)
        ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        position: Qt.vector3d(0, 300, 0)
        shadowMapFar: 2000
        shadowMapQuality: Light.ShadowMapQualityHigh
        visible: true
        castsShadow: true
        brightness: 100
        SequentialAnimation on x {
            loops: Animation.Infinite
            NumberAnimation {
                to: 400
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                to: 0
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }
    }
    //! [point light]

    //! [area light]
    AreaLight {
        id: light3
        color: Qt.rgba(0.1, 0.1, 1.0, 1.0)
        ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        position: Qt.vector3d(-50, 250, 150)
        eulerRotation.x: -90
        width: 1000
        height: 200
        shadowMapFar: 2000
        shadowMapQuality: Light.ShadowMapQualityHigh
        visible: true
        castsShadow: true
        brightness: 100
        SequentialAnimation on z {
            loops: Animation.Infinite
            NumberAnimation {
                to: -150
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                to: 150
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }
    }
    //! [area light]

    //! [spot light]
    SpotLight {
        id: light4
        color: Qt.rgba(1.0, 0.9, 0.7, 1.0)
        ambientColor: Qt.rgba(0.0, 0.0, 0.0, 0.0)
        position: Qt.vector3d(0, 250, 0)
        eulerRotation.x: -45
        shadowMapFar: 2000
        shadowMapQuality: Light.ShadowMapQualityHigh
        visible: true
        castsShadow: true
        brightness: 100
        coneAngle: 50
        innerConeAngle: 30
        PropertyAnimation on eulerRotation.y {
            loops: Animation.Infinite
            from: 0
            to: -360
            duration: 10000
        }
    }
    //! [spot light]

    //! [rectangle models]
    Model {
        source: "#Rectangle"
        y: -200
        scale: Qt.vector3d(15, 15, 15)
        eulerRotation.x: -90
        materials: [
            material
        ]
    }
    Model {
        source: "#Rectangle"
        z: -400
        scale: Qt.vector3d(15, 15, 15)
        materials: [
            material
        ]
    }
    //! [rectangle models]

    //! [teapot model]
    Model {
        source: "teapot.mesh"
        y: -100
        scale: Qt.vector3d(75, 75, 75)
        materials: [
            material
        ]

        NumberAnimation  on eulerRotation.y {
            loops: Animation.Infinite
            duration: 5000
            from: 0
            to: -360
        }
    }
    //! [teapot model]

    //! [light models]
    Model {
        source: "#Cube"
        position: light1.position
        rotation: light1.rotation
        property real size: 0.1
        scale: Qt.vector3d(size, size, size)
        materials: [
            DefaultMaterial {
                diffuseColor: light1.color
                opacity: 0.4
            }
        ]
    }
    Model {
        source: "#Cube"
        position: light2.position
        rotation: light2.rotation
        property real size: 0.1
        scale: Qt.vector3d(size, size, size)
        materials: [
            DefaultMaterial {
                diffuseColor: light2.color
                opacity: 0.4
            }
        ]
    }
    Model {
        source: "#Cube"
        position: light3.position
        rotation: light3.rotation
        property real size: 0.1
        scale: Qt.vector3d(size, size, size)
        materials: [
            DefaultMaterial {
                diffuseColor: light3.color
                opacity: 0.4
            }
        ]
    }
    Model {
        source: "#Cube"
        position: light4.position
        rotation: light4.rotation
        property real size: 0.1
        scale: Qt.vector3d(size, size, size)
        materials: [
            DefaultMaterial {
                diffuseColor: light4.color
                opacity: 0.4
            }
        ]
    }
    //! [light models]
}
