/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

import QtQuick 2.2
import QtQuick.Particles 2.0
import fbitem 1.0

Rectangle {
    id: root
    property alias currentText: edit.text
    property alias multisample: fbitem.multisample
    property bool translucency: false

    gradient: Gradient {
        id: grad
        GradientStop { position: 0; color: "steelblue" }
        GradientStop { position: 1; color: "black" }
    }

    onTranslucencyChanged: {
        if (translucency) {
            root.color = "transparent";
            root.gradient = null;
        } else {
            root.color = "white";
            root.gradient = grad;
        }
    }

    ParticleSystem {
        anchors.fill: parent
        running: true

        ImageParticle {
            source: "qrc:///particleresources/glowdot.png"
            alpha: 0
            colorVariation: 1
        }

        Emitter {
            anchors.fill: parent
            lifeSpan: 3000
            emitRate: 30
            size: 50
            sizeVariation: 10
            velocity: PointDirection { xVariation: 10; yVariation: 10; }
            acceleration: PointDirection {
                y: -10
                xVariation: 5
                yVariation: 5
            }
        }
    }

    Rectangle {
        y: 10
        width: parent.width / 2
        height: edit.contentHeight + 4
        anchors.horizontalCenter: parent.horizontalCenter
        border.color: "gray"
        border.width: 2
        radius: 8
        color: "lightGray"
        clip: true
        TextInput {
            id: edit
            anchors.horizontalCenter: parent.horizontalCenter
            maximumLength: 30
            focus: true
            font.pointSize: 20
        }
    }

    FbItem {
        id: fbitem
        anchors.fill: parent
        SequentialAnimation on eye.y {
            loops: Animation.Infinite
            NumberAnimation {
                from: 0
                to: 0.15
                duration: 1000
            }
            NumberAnimation {
                from: 0.15
                to: 0
                duration: 2000
            }
        }
        SequentialAnimation on eye.x {
            loops: Animation.Infinite
            NumberAnimation {
                from: 0
                to: -0.5
                duration: 3000
            }
            NumberAnimation {
                from: -0.5
                to: 0.5
                duration: 3000
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                from: 0.5
                to: 0
                duration: 1000
            }
        }
        SequentialAnimation on rotation.y {
            loops: Animation.Infinite
            NumberAnimation {
                from: 0
                to: 360
                duration: 5000
            }
            NumberAnimation {
                from: 360
                to: 0
                duration: 2500
            }
        }
        SequentialAnimation on rotation.x {
            loops: Animation.Infinite
            NumberAnimation {
                from: 0
                to: 360
                duration: 6000
            }
            NumberAnimation {
                from: 360
                to: 0
                duration: 3000
            }
        }
    }

    Text {
        id: effText
        text: edit.text
        anchors.centerIn: parent
        font.pointSize: 60
        style: Text.Outline
        styleColor: "green"
    }

    ShaderEffectSource {
        id: effSource
        sourceItem: effText
        hideSource: true
    }

    ShaderEffect {
        SequentialAnimation on scale {
            loops: Animation.Infinite
            NumberAnimation { from: 1.0; to: 2.0; duration: 1000; easing.type: Easing.InCirc }
            PauseAnimation { duration: 1000 }
            NumberAnimation { from: 2.0; to: 0.5; duration: 1000; easing.type: Easing.OutExpo }
            NumberAnimation { from: 0.5; to: 1.0; duration: 500 }
            PauseAnimation { duration: 1000 }
        }
        width: effText.width
        height: effText.height
        anchors.centerIn: parent
        property variant source: effSource
        property real amplitude: 0.002
        property real frequency: 10
        property real time: 0
        NumberAnimation on time { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 1000 }
        fragmentShader:
            "uniform lowp float qt_Opacity;" +
            "uniform highp float amplitude;" +
            "uniform highp float frequency;" +
            "uniform highp float time;" +
            "uniform sampler2D source;" +
            "varying highp vec2 qt_TexCoord0;" +
            "void main() {" +
            "    highp vec2 p = sin(time + frequency * qt_TexCoord0);" +
            "    gl_FragColor = texture2D(source, qt_TexCoord0 + amplitude * vec2(p.y, -p.x)) * qt_Opacity;" +
            "}"
    }
}
