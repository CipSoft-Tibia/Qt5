// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

import qtgrpc.examples.chat

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 640
    height: 480
    minimumWidth: 640
    minimumHeight: 480
    title: qsTr("Simple Chat")
    Rectangle {
        id: background
        anchors.fill: parent
        color: "#09102b"
    }

    Row {
        spacing: 10
        anchors {
            top: parent.top
            topMargin: 10
            left: parent.left
            leftMargin: 20
        }
        Image {
            source: "qrc:/qt_logo_green_64x64px.png"
            width: implicitWidth
            height: implicitHeight
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: "#f3f3f4"
            font.pointSize: 20
            text: qsTr("Chat")
        }
    }

    Column {
        id: loginControl
        spacing: 5
        visible: SimpleChatEngine.state !== SimpleChatEngine.Connected
        anchors.centerIn: parent
        enabled: SimpleChatEngine.state === SimpleChatEngine.Disconnected
        ChatInputField {
            id: loginField
            width: 200
            placeholderText: qsTr("Login")
            onAccepted:             {
                SimpleChatEngine.login(loginField.text, passwordField.text)
            }
            onVisibleChanged: {
                if (loginControl.visible) {
                    loginField.forceActiveFocus()
                }
            }
            Component.onCompleted: {
                if (loginControl.visible) {
                    loginField.forceActiveFocus()
                }
            }
        }
        ChatInputField {
            id: passwordField
            echoMode: TextInput.Password
            placeholderText: qsTr("Password")
            onAccepted: {
                SimpleChatEngine.login(loginField.text, passwordField.text)
            }
        }
        Button {
            id: loginButton
            anchors.horizontalCenter: parent.horizontalCenter
            width: enterText.implicitWidth + 20
            height: 40
            background: Rectangle {
                radius: 5
                border {
                    width: 1
                    color: loginButton.pressed ? "#41cd52" : "#f3f3f4"
                }
                color:"#53586b"
                Text {
                    id: enterText
                    text : qsTr("Enter")
                    color: "#f3f3f4"
                    anchors.centerIn: parent
                    font.pointSize: 14
                }
            }

            onClicked: {
                SimpleChatEngine.login(loginField.text, passwordField.text)
            }
        }
    }

    ChatView {
        id: chatView
        visible: SimpleChatEngine.state === SimpleChatEngine.Connected
    }

    Text {
        id: connectingText
        visible: SimpleChatEngine.state === SimpleChatEngine.Connecting
        anchors.top: loginControl.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Connecting...")
        font.pointSize: 14
        color: "#f3f3f4"
    }

    Text {
        id: errorText
        visible: false
        anchors.top: loginControl.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 14
        color: "#f3f3f4"
        onVisibleChanged: {
            if (errorText.visible) {
                fadeOutTimer.restart()
            } else {
                fadeOutTimer.stop()
            }
        }

        Timer {
            id: fadeOutTimer
            onTriggered: {
                errorText.visible = false
            }
        }
    }

    Connections {
        target: SimpleChatEngine
        function onAuthFailed() {
            errorText.text = qsTr("This username with this password doesn't exists.")
            errorText.visible = true;
        }
        function onNetworkError(errorString) {
            errorText.text = errorString;
            errorText.visible = true;
        }
    }
}
