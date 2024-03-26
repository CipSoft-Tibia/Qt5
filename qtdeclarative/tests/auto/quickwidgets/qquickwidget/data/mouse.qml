import QtQuick 2.0

Rectangle {
    width: 50
    height: 50

    property bool wasClicked: false
    property bool wasDoubleClicked: false
    property bool wasMoved: false
    color: ma.pressed ? "wheat" : "lightsteelblue"

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        onClicked: wasClicked = true
        onDoubleClicked: wasDoubleClicked = true
        onMouseXChanged: wasMoved = true
    }
}
