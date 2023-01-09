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

import QtQuick 2.12
import QtQuick.Controls 2.3 as QQC2
import Qt.labs.settings 1.0
import "qml"
import "qml/Style"

QQC2.ApplicationWindow {
    id: window
    visible: true
    width: 320
    height: 320
    title: qsTr("Wearable")

    Settings {
        id: settings
        property bool wireless
        property bool bluetooth
        property int brightness
        property bool darkTheme
        property bool demoMode
    }

    Binding {
        target: UIStyle
        property: "darkTheme"
        value: settings.darkTheme
    }

    // We need the settings object both here and in SettingsPage,
    // so for convenience, we declare it as a property of the root object so that
    // it will be available to all of the QML files that we load.
    property alias settings: settings

    background: Image {
        source: "images/background-" + (settings.darkTheme ? "dark" : "light") + ".png"
    }

    header: NaviButton {
        id: homeButton

        edge: Qt.TopEdge
        enabled: stackView.depth > 1
        imageSource: "images/home.png"

        onClicked: stackView.pop(null)
    }

    footer: NaviButton {
        id: backButton

        edge: Qt.BottomEdge
        enabled: stackView.depth > 1
        imageSource: "images/back.png"

        onClicked: stackView.pop()
    }

    QQC2.StackView {
        id: stackView

        focus: true
        anchors.fill: parent

        initialItem: LauncherPage {
            onLaunched: stackView.push(page)
        }
    }

    DemoMode {
        stackView: stackView
    }

    DemoModeIndicator {
        id: demoModeIndicator
        y: settings.demoMode ? -height : -height * 2
        anchors.horizontalCenter: parent.horizontalCenter
        height: header.height
        z: window.header.z + 1
    }

    MouseArea {
        enabled: settings.demoMode
        anchors.fill: parent
        onClicked: {
            // Stop demo mode and return to the launcher page.
            settings.demoMode = false
            stackView.pop(null)
        }
    }
}
