// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: keyView
    property real keyWidth: (width - (horizontalSpacing * 9)) / 10
    property real keyHeight: (height - (verticalSpacing * 2)) / 3
    property real horizontalSpacing: topLevel.globalMargin / 4
    property real verticalSpacing: topLevel.globalMargin / 4
    property bool locked: false
    property bool hideVowels: false

    Component.onCompleted: topLevel.buttonHeight = keyHeight
    onKeyHeightChanged: topLevel.buttonHeight = keyHeight

    property var keys: [keyA, keyB, keyC, keyD, keyE, keyF, keyG, keyH, keyI, keyJ,
            keyH, keyJ, keyK, keyL, keyM, keyN, keyO, keyP, keyQ, keyR, keyS,
            keyT, keyU, keyV, keyW, keyX, keyY, keyZ];

    function reset() {
        //Resets all key values to their default state
        for (var i = 0; i < keys.length; ++i)
            keys[i].available = true;
    }

    onLockedChanged: {
        if (locked) {
            for (var i = 0; i < keys.length; ++i)
                keys[i].available = false;
        }
    }

    signal letterSelected(string letter)
    signal guessWordPressed()
    signal resetPressed()
    signal revealPressed()

    //Qwerty layout
    Column {
        spacing: keyView.verticalSpacing
        anchors.fill: parent
        Row {
            spacing: keyView.horizontalSpacing
            Key {
                id: keyQ
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "Q";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyW
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "W";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyE
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "E";
                available: true
                purchasable: true
                enabled: !hideVowels
                opacity: hideVowels ? 0.0 : 1.0
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyR
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "R";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyT
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "T";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyY
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "Y";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyU
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "U";
                available: true
                purchasable: true
                enabled: !hideVowels
                opacity: hideVowels ? 0.0 : 1.0
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyI
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "I";
                available: true
                purchasable: true
                enabled: !hideVowels
                opacity: hideVowels ? 0.0 : 1.0
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyO
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "O";
                available: true
                purchasable: true
                enabled: !hideVowels
                opacity: hideVowels ? 0.0 : 1.0
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyP
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "P";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keyView.horizontalSpacing
            Key {
                id: keyA
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "A";
                available: true
                purchasable: true
                enabled: !hideVowels
                opacity: hideVowels ? 0.0 : 1.0
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyS
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "S";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyD
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "D";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyF
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "F";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyG
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "G";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyH
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "H";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyJ
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "J";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyK
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "K";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyL
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "L";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
        }
        Row {
            spacing: keyView.horizontalSpacing
            anchors.horizontalCenter: parent.horizontalCenter
            Key {
                id: keyZ
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "Z";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyX
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "X";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyC
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "C";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyV
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "V";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyB
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "B";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyN
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "N";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
            Key {
                id: keyM
                height: keyView.keyHeight
                width: keyView.keyWidth
                text: "M";
                available: true
                purchasable: false
                onKeyActivated: function(letter) {
                    letterSelected(letter);
                    available = false;
                }
            }
        }
    }
}
