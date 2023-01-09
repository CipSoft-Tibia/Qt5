import QtQuick 2.0
Rectangle {
    width: 400
    height: 400
    Rectangle {
        id: rect
        objectName: "MyRect"
        width: 100; height: 100; color: "green"
        Behavior on x {
            objectName: "MyBehavior";
            NumberAnimation { targets: rect; properties: "y"; duration: 200; }
        }
    }
    MouseArea {
        id: clicker
        anchors.fill: parent
    }
    states: State {
        name: "moved"
        when: clicker.pressed
        PropertyChanges {
            target: rect
            x: 200
        }
    }
}
