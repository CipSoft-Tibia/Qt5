import QtQuick 2.0

Rectangle {
    color: "white"
    width: 240
    height: 320
    Rectangle { id: masterRect; objectName: "masterRect"; x: 26; width: 96; height: 20; color: "red" }
    Rectangle {
        id: rect1; objectName: "rect1"
        y: 20; width: 10; height: 10
        anchors.left: masterRect.left
    }
    Rectangle {
        id: rect2; objectName: "rect2"
        y: 20; width: 10; height: 10
        anchors.left: masterRect.right
    }
    Rectangle {
        id: rect3; objectName: "rect3"
        y: 20; width: 10; height: 10
        anchors.left: masterRect.horizontalCenter
    }
    Rectangle {
        id: rect4; objectName: "rect4"
        y: 30; width: 10; height: 10
        anchors.right: masterRect.left
    }
    Rectangle {
        id: rect5; objectName: "rect5"
        y: 30; width: 10; height: 10
        anchors.right: masterRect.right
    }
    Rectangle {
        id: rect6; objectName: "rect6"
        y: 30; width: 10; height: 10
        anchors.right: masterRect.horizontalCenter
    }
    Rectangle {
        id: rect7; objectName: "rect7"
        y: 50; width: 10; height: 10
        anchors.left: parent.left
    }
    Rectangle {
        id: rect8; objectName: "rect8"
        y: 50; width: 10; height: 10
        anchors.left: parent.right
    }
    Rectangle {
        id: rect9; objectName: "rect9"
        y: 50; width: 10; height: 10
        anchors.left: parent.horizontalCenter
    }
    Rectangle {
        id: rect10; objectName: "rect10"
        y: 60; width: 10; height: 10
        anchors.right: parent.left
    }
    Rectangle {
        id: rect11; objectName: "rect11"
        y: 60; width: 10; height: 10
        anchors.right: parent.right
    }
    Rectangle {
        id: rect12; objectName: "rect12"
        y: 60; width: 10; height: 10
        anchors.right: parent.horizontalCenter
    }
    Rectangle {
        id: rect13; objectName: "rect13"
        x: 200; width: 10; height: 10
        anchors.top: masterRect.bottom
    }
    Rectangle {
        id: rect14; objectName: "rect14"
        width: 10; height: 10; color: "steelblue"
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle {
        id: rect15; objectName: "rect15"
        y: 200; height: 10
        anchors.left: masterRect.left
        anchors.right: masterRect.right
    }
    Rectangle {
        id: rect16; objectName: "rect16"
        y: 220; height: 10
        anchors.left: masterRect.left
        anchors.horizontalCenter: masterRect.right
    }
    Rectangle {
        id: rect17; objectName: "rect17"
        y: 240; height: 10
        anchors.right: masterRect.right
        anchors.horizontalCenter: masterRect.left
    }
    Rectangle {
        id: rect18; objectName: "rect18"
        x: 180; width: 10
        anchors.top: masterRect.bottom
        anchors.bottom: rect12.top
    }
    Rectangle {
        id: rect19; objectName: "rect19"
        y: 70; width: 10; height: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Rectangle {
        id: rect20; objectName: "rect20"
        y: 70; width: 10; height: 10
        anchors.horizontalCenter: parent.right
    }
    Rectangle {
        id: rect21; objectName: "rect21"
        y: 70; width: 10; height: 10
        anchors.horizontalCenter: parent.left
    }
    Rectangle {
        id: rect22; objectName: "rect22"
        width: 10; height: 10
        anchors.centerIn: masterRect
    }
    Rectangle {
        id: rect23; objectName: "rect23"
        anchors.left: masterRect.left
        anchors.leftMargin: 5
        anchors.right: masterRect.right
        anchors.rightMargin: 5
        anchors.top: masterRect.top
        anchors.topMargin: 5
        anchors.bottom: masterRect.bottom
        anchors.bottomMargin: 5
    }
    Rectangle {
        id: rect24; objectName: "rect24"
        width: 10; height: 10
        anchors.horizontalCenter: masterRect.left
        anchors.horizontalCenterOffset: width/2
    }
    Rectangle {
        id: rect25; objectName: "rect25"
        width: 10; height: 10
        anchors.verticalCenter: rect12.top
        anchors.verticalCenterOffset: height/2
    }
    Rectangle {
        id: rect26; objectName: "rect26"
        width: 10; height: 10
        anchors.baseline: masterRect.top
        anchors.baselineOffset: height/2
    }
    Text {
        id: text1; objectName: "text1"
        y: 200;
        text: "Hello"
    }
    Text {
        id: text2; objectName: "text2"
        anchors.baseline: text1.baseline
        anchors.left: text1.right
        text: "World"
    }
}
