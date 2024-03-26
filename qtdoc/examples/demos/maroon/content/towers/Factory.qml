// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../logic.js" as Logic
import ".."

TowerBase {
    id: container
    rof: 160
    income: 5
    SpriteSequence {
        id: sprite
        width: 64
        height: 64
        interpolate: false
        goalSprite: ""

        Sprite {
            name: "idle"
            source: "../gfx/factory-idle.png"
            frameCount: 4
            frameDuration: 200
        }

        Sprite {
            name: "action"
            source: "../gfx/factory-action.png"
            frameCount: 4
            frameDuration: 90
            to: { "idle" : 1 }
        }

        SequentialAnimation on x {
            loops: Animation.Infinite
            NumberAnimation { from: x; to: x + 4; duration: 900; easing.type: Easing.InOutQuad }
            NumberAnimation { from: x + 4; to: x; duration: 900; easing.type: Easing.InOutQuad }
        }
        SequentialAnimation on y {
            loops: Animation.Infinite
            NumberAnimation { from: y; to: y - 4; duration: 900; easing.type: Easing.InOutQuad }
            NumberAnimation { from: y - 4; to: y; duration: 900; easing.type: Easing.InOutQuad }
        }
    }

    SoundEffect {
        id: actionSound
        source: "../audio/factory-action.wav"
    }

    function fire() {
        actionSound.play()
        sprite.jumpTo("action")
        coinLaunch.start()
    }

    function spawn() {
        coin.target = Logic.gameState.mapToItem(container, 240, -32)
    }

    Image {
        id: coin
        property var target: { "x" : 0, "y" : 0 }
        source: "../gfx/currency.png"
        visible: false
    }

    SequentialAnimation {
        id: coinLaunch
        PropertyAction { target: coin; property: "visible"; value: true }
        ParallelAnimation {
            NumberAnimation { target: coin; property: "x"; from: 16; to: coin.target.x }
            NumberAnimation { target: coin; property: "y"; from: 16; to: coin.target.y }
        }
        PropertyAction { target: coin; property: "visible"; value: false }
    }
}
