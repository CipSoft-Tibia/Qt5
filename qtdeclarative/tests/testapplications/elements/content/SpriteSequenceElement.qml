// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: spriteimageelementtest
    anchors.fill: parent
    property string testtext: ""
    SpriteSequence {
        id: spriteimage
        sprites: [Sprite {
            name: "happy"
            source: "pics/squarefacesprite2.png"
            frameCount: 6
            frameDuration: 120
            to: {"silly": 1, "sad":0}
        }, Sprite {
            name: "silly"
            source: "pics/squarefacesprite.png"
            frameCount: 6
            frameDuration: 120
            to: {"happy": 1, "sad": 0}
        }, Sprite {
            name: "sad"
            source: "pics/squarefacesprite3.png"
            frameCount: 6
            frameDuration: 120
            to: {"evil": 0.5, "sad": 1, "cyclops" : 0}
        }, Sprite {
            name: "cyclops"
            source: "pics/squarefacesprite4.png"
            frameCount: 6
            frameDuration: 120
            to: {"love": 0.1, "boggled": 0.1, "cyclops" : 0.1}
        }, Sprite {
            name: "evil"
            source: "pics/squarefacesprite5.png"
            frameCount: 6
            frameDuration: 120
            to: {"sad": 1.0, "cyclops" : 0}
        }, Sprite {
            name: "love"
            source: "pics/squarefacesprite6.png"
            frameCount: 6
            frameDuration: 120
            to: {"love": 0.1, "boggled": 0.1, "cyclops" : 0.1}
        }, Sprite {
            name: "boggled"
            source: "pics/squarefacesprite7.png"
            frameCount: 6
            frameDuration: 120
            to: {"love": 0.1, "boggled": 0.1, "cyclops" : 0.1, "dying":0}
        }, Sprite {
            name: "dying"
            source: "pics/squarefacespriteX.png"
            frameCount: 4
            frameDuration: 120
            to: {"dead":1.0}
        }, Sprite {
            name: "dead"
            source: "pics/squarefacespriteXX.png"
            frameCount: 1
            frameDuration: 10000
        }]

        width: 300
        height: 300
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            StateChangeScript { script: spriteimage.jumpTo("happy"); }
            PropertyChanges { target: spriteimageelementtest
                testtext: "This is a SpriteSequence element. It should be animating currently."+
                "It should alternate between winking and sticking out its tongue." }
        },
        State { name: "stochastic2"; when: statenum == 2
            StateChangeScript { script: spriteimage.jumpTo("sad"); }
            PropertyChanges { target: spriteimageelementtest
                testtext: "The sprite should now be animating between frowning and being evil."+
                "This should not be alternating, but mostly frowning with the occasional evil eyes."+
                "After an evil eyes animation, it should return to frowning at least once." }
        },
        State { name: "stochastic3"; when: statenum == 3
            StateChangeScript { script: spriteimage.jumpTo("cyclops"); }
            PropertyChanges { target: spriteimageelementtest
                testtext: "The sprite should now be animating fairly randomly between three animations where it does silly things with its eyes.\n"+
                "Next the sprite will animate into a static 'dead' state."+
                "When it does, it should first animate to and play through the 'big eyes' animation (if it is not currently playing that animation) before it enters the dying animation."}
        },
        State { name: "dead"; when: statenum == 4
            PropertyChanges { target: spriteimage; goalSprite: "dead" }
            PropertyChanges { target: spriteimageelementtest
                testtext: "After a brief dying animation, the image should now be static.\n"+
                "Advance to restart the test." }
        }
    ]
}
