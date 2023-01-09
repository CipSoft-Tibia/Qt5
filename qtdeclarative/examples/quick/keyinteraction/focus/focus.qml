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

import QtQml 2.1
import QtQuick 2.1
import "Core"

Rectangle {
    id: window

    width: 800; height: 640
    color: "#3E606F"

    FocusScope {
        id: mainView

        width: parent.width; height: parent.height
        focus: true

        TabMenu {
            id: tabMenu
            y: 160; width: parent.width; height: 160

            keyUpTarget: listMenu
            keyDownTarget: gridMenu

            focus: true
            activeFocusOnTab: true

            onActiveFocusChanged: {
                if (activeFocus)
                    mainView.state = "showTabViews"
            }
        }

        GridMenu {
            id: gridMenu
            y: 320; width: parent.width; height: 320
            activeFocusOnTab: true

            keyUpTarget: tabMenu
            keyDownTarget: listMenu
            keyLeftTarget: contextMenu

            onActiveFocusChanged: {
                if (activeFocus)
                    mainView.state = "showGridViews"
            }
        }

        ListMenu {
            id: listMenu
            y: 640; width: parent.width; height: 320
            activeFocusOnTab: true

            keyUpTarget: gridMenu
            keyLeftTarget: contextMenu

            onActiveFocusChanged: {
                if (activeFocus)
                    mainView.state = "showListViews"
            }
        }

        Rectangle {
            id: shade
            anchors.fill: parent
            color: "black"
            opacity: 0
        }

        states:  [
            State {
                name: "showTabViews"
                PropertyChanges { target: tabMenu; y:  160 }
                PropertyChanges { target: gridMenu; y: 320 }
                PropertyChanges { target: listMenu; y: 640 }
            },

            State {
                name: "showGridViews"
                PropertyChanges { target: tabMenu; y:    0 }
                PropertyChanges { target: gridMenu; y: 160 }
                PropertyChanges { target: listMenu; y: 480 }
            },

            State {
                name: "showListViews"
                PropertyChanges { target: tabMenu; y: -160 }
                PropertyChanges { target: gridMenu; y: 0 }
                PropertyChanges { target: listMenu; y: 320 }
            }
        ]

        transitions: Transition {
            NumberAnimation { properties: "y"; duration: 600; easing.type: Easing.OutQuint }
        }
    }

    Image {
        source: "Core/images/arrow.png"
        rotation: 90
        anchors.verticalCenter: parent.verticalCenter

        MouseArea {
            anchors.fill: parent; anchors.margins: -10
            onClicked: contextMenu.focus = true
        }
    }

    ContextMenu {
        keyRightTarget: mainView
        id: contextMenu
        x: -265
        width: 260
        height: parent.height
    }

    states: State {
        name: "contextMenuOpen"
        when: !mainView.activeFocus
        PropertyChanges { target: contextMenu; x: 0; open: true }
        PropertyChanges { target: mainView; x: 130 }
        PropertyChanges { target: shade; opacity: 0.25 }
    }

    transitions: Transition {
        NumberAnimation { properties: "x,opacity"; duration: 600; easing.type: Easing.OutQuint }
    }
}
