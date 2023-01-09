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
import QtQuick 2.0
import QtQuick.Particles 2.0

ParticleSystem {
    id: particleSystem

    property int score

    function explode(x,y) {
        fireEmitter.burst(100,x,y);
    }

    Emitter {
        id: emitter
        group: "bears"
        width: parent.width
        emitRate: 1
        NumberAnimation on emitRate {
            id: goFaster
            from: 1
            to: 16
            running: particleSystem.running
            loops: 1
            duration: 60000 * 5
            easing.type: Easing.Linear
        }
        lifeSpan: 4000 + 800*(16-emitRate)
        maximumEmitted: 128
        size: 64
        velocity: PointDirection{ y: 40 + 10 * emitter.emitRate }
    }

    Emitter {
        id: fireEmitter
        enabled: false
        maximumEmitted: 6000
        group: "flame"
        emitRate: 1000
        size: 16
        endSize: 8
        velocity: CumulativeDirection { AngleDirection {angleVariation: 180; magnitudeVariation: 120;} PointDirection { y: -60 }}
        lifeSpan: 400
    }
    Emitter {
        id: heartEmitter
        enabled: false
        maximumEmitted: 6000
        group: "hearts"
        emitRate: 1000
        size: 16
        endSize: 8
        velocity: AngleDirection {angleVariation: 180; magnitudeVariation: 180;}
        lifeSpan: 600
    }
    Emitter {
        id: bloodEmitter
        enabled: false
        maximumEmitted: 6000
        group: "blood"
        emitRate: 1000
        size: 16
        endSize: 8
        velocity: CumulativeDirection { AngleDirection {angleVariation: 180; magnitudeVariation: 80;} PointDirection { y: 40 }}
        lifeSpan: 600
    }

    Affector {
        width: parent.width
        height: 64
        once: true
        y: parent.height - 32
        groups: "bears"
        onAffectParticles: function(particles) {
            for (var i=0;i<particles.length; i++) {
                if (particles[i].animationIndex != 0) {
                    particleSystem.score++;
                    bloodEmitter.burst(100, particles[i].x, particles[i].y);
                } else {
                    particleSystem.score--;
                    heartEmitter.burst(100, particles[i].x, particles[i].y);
                }
                particles[i].update = 1.0;
                particles[i].t -= 1000.0;
            }
        }
    }
    ImageParticle {
        groups: ["flame"]
        source: "blur-circle.png"
        z: 4
        colorVariation: 0.1
        color: "#ffa24d"
        alpha: 0.4
    }
    ImageParticle {
        groups: ["blood"]
        color: "red"
        z: 2
        source: "blur-circle3.png"
        alpha: 0.2
    }
    ImageParticle {
        groups: ["hearts"]
        color: "#ff66AA"
        z: 3
        source: "heart-blur.png"
        alpha: 0.4
        autoRotation: true
    }
    ImageParticle {
        groups: ["bears"]
        z: 1
        spritesInterpolate: false
        sprites:[
        Sprite{
            name: "floating"
            source: "Bear1.png"
            frameCount: 9
            frameWidth: 256
            frameHeight: 256
            frameDuration: 80
            to: {"still":0, "flailing":0}
        },
        Sprite{
            name: "flailing"
            source: "Bear2.png"
            frameCount: 8
            frameWidth: 256
            frameHeight: 256
            frameDuration: 80
            to: {"falling":1}
        },
        Sprite{
            name: "falling"
            source: "Bear3.png"
            frameCount: 5
            frameWidth: 256
            frameHeight: 256
            frameDuration: 80
            to: {"falling":1}
        }
        ]
    }
}
