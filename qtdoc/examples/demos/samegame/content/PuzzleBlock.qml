// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Item {
    id: block
    property bool dying: false
    property bool spawned: false
    property int type: 0
    property ParticleSystem particleSystem

    Behavior on x {
        enabled: spawned;
        NumberAnimation{ easing.type: Easing.OutBounce }
    }
    Behavior on y {
        NumberAnimation{ easing.type: Easing.InQuad }
    }

    Image {
        id: img
        source: {
            if (type == 0){
                "gfx/red-puzzle.png";
            } else if (type == 1) {
                "gfx/blue-puzzle.png";
            } else if (type == 2) {
                "gfx/green-puzzle.png";
            } else {
                "gfx/yellow-puzzle.png";
            }
        }
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -4
        anchors.horizontalCenterOffset: 4
    }

    //Foreground particles
    BlockEmitter {
        id: particles
        system: particleSystem
        group: {
            if (type == 0){
                "red";
            } else if (type == 1) {
                "blue";
            } else if (type == 2) {
                "green";
            } else {
                "yellow";
            }
        }
        anchors.fill: parent
    }

    states: [
        State {
            name: "AliveState"; when: spawned == true && dying == false
            PropertyChanges { target: img; opacity: 1 }
        },

        State {
            name: "DeathState"; when: dying == true
            PropertyChanges { target: img; scale: 2 }
            StateChangeScript { script: particles.pulse(200); }
            PropertyChanges { target: img; opacity: 0 }
            StateChangeScript { script: block.destroy(1000); }
        }
    ]
}
