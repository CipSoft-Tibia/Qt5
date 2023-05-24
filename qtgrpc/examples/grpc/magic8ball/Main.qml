// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes

import qtgrpc.examples.magic8ball

ApplicationWindow {
    id: root
    width: 665
    height: width

    minimumWidth: width
    minimumHeight: height

    visible: true
    title: qsTr("Magic-8-ball Qt GRPC Example")
    Material.theme: Material.Light

    property string textAnswer: ""
    property string textError: ""

    MagicText {
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter

        width: parent.width * 0.9
        height: parent.height/3

        color: "black"

        text: qsTr("For fortune-telling and seeking advice ask the ball"
                   + " a yes-no question and press the button.")
    }

    Rectangle {
        id: magic8ball

        anchors.centerIn: parent

        width: 433
        height: width

        color: "#000000"
        radius: 300
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#4b4b4b" }
            GradientStop { position: 0.33; color: "#212121" }
            GradientStop { position: 1.0; color: "#000000" }
        }
    }

    Rectangle {

        width: 244
        height: width

        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: 6
        radius: 300

        color: "#bababa"
    }

    Rectangle {
        id: magic8ballCenter

        anchors.centerIn: parent

        width: 244
        height: width

        color: "black"
        border.width: 1.5
        border.color: "#bababa"
        radius: 300

        Shape {
            anchors.centerIn: parent
            width: 200
            height: 200
            ShapePath {
                strokeWidth: 4
                strokeColor: "#213f94"
                capStyle: ShapePath.RoundCap

                fillGradient: RadialGradient {
                    centerX: 100
                    centerY: 100
                    focalX: centerX
                    focalY: centerY
                    centerRadius: 50
                    focalRadius: 0

                    GradientStop { position: 0; color: "#1C2F60" }
                    GradientStop { position: 0.5; color: "#000547" }
                    GradientStop { position: 1; color: "#000324" }
                }

                startX: 10
                startY: 40

                PathLine { x: 100.5; y: 190 }
                PathLine { x: 188; y: 40 }
                PathLine { x: 10; y: 40 }
            }
        }
    }

    WaitingAnimation {
        id: waitingAnimation
        anchors.centerIn: parent

        visible: false
    }

    AnimatedAnswer {
        id: answer

        anchors.centerIn: parent
        visible: false
    }

    Connections {
        target: ClientService
        function onMessageRecieved(value) {
            root.textAnswer = value
        }

        function onErrorRecieved(value) {
            root.textError = value
        }
    }

    Rectangle {
        id: button

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter

        width: 200
        height: 50
        radius: 10

        color: handler.pressed ? "#a5a5a5" : "#bebebe"

        MagicText {
            id: btnText
            anchors.centerIn: parent

            text: qsTr("Ask question")
            color: "black"
        }

        TapHandler {
            id: handler

            onTapped: animationTimeout.start()
        }
    }

    Connections {
        target: answer.closingAnimation
        function onStopped()  {
            answer.animationText = ""
            answer.visible = false
            waitingAnimation.visible = true
            waitingAnimation.runAnimation = true
        }
    }

    Connections {
        target: waitingAnimation
        function onRunAnimationChanged()  {
            if (!waitingAnimation.runAnimation) {
                answer.visible = true
                answer.openingAnimation.start()
            }
        }
    }

    Timer {
        id: animationTimeout

        interval: 3000
        repeat: false
        running: false
        onTriggered: ClientService.setMessage()

        onRunningChanged: {
            if (running) {
                answer.closingAnimation.start()
                ClientService.sendRequest()
            } else {
                waitingAnimation.runAnimation = false
                waitingAnimation.visible = false
                answer.animationText = root.textError === "" ? root.textAnswer : root.textError
            }
        }
    }

    footer: MagicText {
        text: root.textError === "" ? "" : "Please, start server: ../magic8ball/SimpleGrpcServer"
    }
}
