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

FocusScope {
    id: menu
    clip: true
    required property Item keyUpTarget
    required property Item keyLeftTarget

    ListView {
        id: list1
        y: activeFocus ? 10 : 40; width: parent.width / 3; height: parent.height - 20
        focus: true
        KeyNavigation.up: menu.keyUpTarget
        KeyNavigation.left: menu.keyLeftTarget
        KeyNavigation.right: list2
        model: 10; cacheBuffer: 200
        delegate: ListViewDelegate {}

        Behavior on y {
            NumberAnimation { duration: 600; easing.type: Easing.OutQuint }
        }
    }

    ListView {
        id: list2
        y: activeFocus ? 10 : 40; x: parseInt(parent.width / 3); width: parent.width / 3; height: parent.height - 20
        KeyNavigation.up: menu.keyUpTarget
        KeyNavigation.left: list1
        KeyNavigation.right: list3
        model: 10; cacheBuffer: 200
        delegate: ListViewDelegate {}

        Behavior on y {
            NumberAnimation { duration: 600; easing.type: Easing.OutQuint }
        }
    }

    ListView {
        id: list3
        y: activeFocus ? 10 : 40; x: parseInt(2 * parent.width / 3); width: parent.width / 3; height: parent.height - 20
        KeyNavigation.up: menu.keyUpTarget
        KeyNavigation.left: list2
        model: 10; cacheBuffer: 200
        delegate: ListViewDelegate {}

        Behavior on y {
            NumberAnimation { duration: 600; easing.type: Easing.OutQuint }
        }
    }

    Rectangle { width: parent.width; height: 1; color: "#D1DBBD" }

    Rectangle {
        y: 1; width: parent.width; height: 10
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#3E606F" }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Rectangle {
        y: parent.height - 10; width: parent.width; height: 10
        gradient: Gradient {
            GradientStop { position: 1.0; color: "#3E606F" }
            GradientStop { position: 0.0; color: "transparent" }
        }
    }
}
