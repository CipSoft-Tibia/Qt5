/****************************************************************************
*
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

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: window
    width: 320; height: 480
    Rectangle {
        id: sky
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "DeepSkyBlue"
            }
            GradientStop {
                position: 1.0
                color: "SkyBlue"
            }
        }
    }

    Rectangle {
        id: ground
        width: parent.height * 2
        height: parent.height
        y: parent.height/2
        x: parent.width/2 - parent.height
        transformOrigin: Item.Top
        rotation: 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: "ForestGreen"; }
            GradientStop { position: 1.0; color: "DarkGreen"; }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPositionChanged: {
            var rot = Math.atan2(mouseY - window.height/2,mouseX - window.width/2) * 180/Math.PI;
            ground.rotation = rot;
        }
    }

    ParticleSystem { id: sys }
    //! [0]
    Gravity {
        system: sys
        magnitude: 32
        angle: ground.rotation + 90
    }
    //! [0]
    Emitter {
        system: sys
        anchors.centerIn: parent
        emitRate: 1
        lifeSpan: 10000
        size: 64
    }
    ImageParticle {
        anchors.fill: parent
        system: sys
        source: "../../images/realLeaf1.png"
    }

}
