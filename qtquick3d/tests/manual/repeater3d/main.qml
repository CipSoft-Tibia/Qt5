/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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


Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("3D repeater")

    function randomWithRange(min, max) {
        return Math.floor(Math.random() * (max - min + 1) ) + min;
    }

    Component {
        id: animatedCube

        Model {
            source: "#Cube"
            position: Qt.vector3d(randomWithRange(-300, 300),
                                  randomWithRange(-300, 300),
                                  randomWithRange(-300, 300))

            SequentialAnimation on eulerRotation.y {
                running: true
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: randomWithRange(100, 10000)
                    from: 0
                    to: 360
                }
            }

            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(randomWithRange(0, 255) / 255,
                                      randomWithRange(0, 255) / 255,
                                      randomWithRange(0, 255) / 255,
                                      1.0);
            }
        }
    }

    View3D {
        id: viewport
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
        }

        DirectionalLight {
            id: directionalLight
        }

        PerspectiveCamera {
            id: camera
            z: 600
        }

        Repeater3D {
            model: 100
            delegate: animatedCube
        }

    }
}
