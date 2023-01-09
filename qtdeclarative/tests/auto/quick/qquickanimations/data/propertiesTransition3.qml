import QtQuick 2.0

Rectangle {
    width: 400
    height: 400
    Rectangle {
        id: theRect
        objectName: "TheRect"
        color: "red"
        width: 50; height: 50
        x: 100; y: 100
    }

    states: State {
        name: "moved"
        PropertyChanges {
            target: theRect
            x: 200
        }
    }
    transitions: Transition {
        NumberAnimation { targets: theRect; properties: "y" }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.state = "moved"
    }
}
