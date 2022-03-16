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
import QtQuick.Window 2.3
import "../shared" as Shared

QtObject {
    property real defaultSpacing: 10
    property SystemPalette palette: SystemPalette { }

    property var controlWindow: Window {
        width: col.implicitWidth + defaultSpacing * 2
        height: col.implicitHeight + defaultSpacing * 2
        color: palette.window
        title: "Control Window"
        Column {
            id: col
            anchors.fill: parent
            anchors.margins: defaultSpacing
            spacing: defaultSpacing
            property real cellWidth: col.width / 3 - spacing
            Shared.Label { text: "Control the second window:" }
            Grid {
                id: grid
                columns: 3
                spacing: defaultSpacing
                width: parent.width
                Shared.Button {
                    id: showButton
                    width: col.cellWidth
                    text: testWindow.visible ? "Hide" : "Show"
                    onClicked: testWindow.visible = !testWindow.visible
                }
                //! [windowedCheckbox]
                Shared.CheckBox {
                    text: "Windowed"
                    height: showButton.height
                    width: col.cellWidth
                    Binding on checked { value: testWindow.visibility === Window.Windowed }
                    onClicked: testWindow.visibility = Window.Windowed
                }
                //! [windowedCheckbox]
                Shared.CheckBox {
                    height: showButton.height
                    width: col.cellWidth
                    text: "Full Screen"
                    Binding on checked { value: testWindow.visibility === Window.FullScreen }
                    onClicked: testWindow.visibility = Window.FullScreen
                }
                Shared.Button {
                    id: autoButton
                    width: col.cellWidth
                    text: "Automatic"
                    onClicked: testWindow.visibility = Window.AutomaticVisibility
                }
                Shared.CheckBox {
                    height: autoButton.height
                    text: "Minimized"
                    Binding on checked { value: testWindow.visibility === Window.Minimized }
                    onClicked: testWindow.visibility = Window.Minimized
                }
                Shared.CheckBox {
                    height: autoButton.height
                    text: "Maximized"
                    Binding on checked { value: testWindow.visibility === Window.Maximized }
                    onClicked: testWindow.visibility = Window.Maximized
                }
            }
            function visibilityToString(v) {
                switch (v) {
                case Window.Windowed:
                    return "windowed";
                case Window.Minimized:
                    return "minimized";
                case Window.Maximized:
                    return "maximized";
                case Window.FullScreen:
                    return "fullscreen";
                case Window.AutomaticVisibility:
                    return "automatic";
                case Window.Hidden:
                    return "hidden";
                }
                return "unknown";
            }
            Shared.Label {
                id: visibilityLabel
                text: "second window is " + (testWindow.visible ? "visible" : "invisible") +
                      " and has visibility " + parent.visibilityToString(testWindow.visibility)
            }
            Rectangle {
                color: palette.text
                width: parent.width
                height: 1
            }
            CurrentScreen { }
            Rectangle {
                color: palette.text
                width: parent.width
                height: 1
            }
            AllScreens { width: parent.width }
        }
    }

    property var testWindow: Window {
        width: 320
        height: 240
        color: "#215400"
        title: "Test Window with color " + color
        flags: Qt.Window | Qt.WindowFullscreenButtonHint
        Rectangle {
            anchors.fill: parent
            anchors.margins: defaultSpacing
            Shared.Label {
                anchors.centerIn: parent
                text: "Second Window"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: testWindow.color = "#e0c31e"
            }
            Shared.Button {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: defaultSpacing
                text: testWindow.visibility === Window.FullScreen ? "exit fullscreen" : "go fullscreen"
                width: 150
                onClicked: {
                    if (testWindow.visibility === Window.FullScreen)
                        testWindow.visibility = Window.AutomaticVisibility
                    else
                        testWindow.visibility = Window.FullScreen
                }
            }
            Shared.Button {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: defaultSpacing
                text: "X"
                width: 30
                onClicked: testWindow.close()
            }
        }
    }

    property var splashWindow: Splash {
        onTimeout: controlWindow.visible = true
    }
}
