import QtQuick
import QtQuick.Controls

Item {
    width: 640
    height: 480
    visible: true

    property alias outerMouseArea: outerMouseArea
    property alias buttonInTheMiddle: buttonInTheMiddle
    property alias innerMouseArea: innerMouseArea

    MouseArea {
        id: outerMouseArea
        x: 10
        y: 10
        width: 200
        height: 200
        hoverEnabled: true
        Rectangle {
            anchors.fill: parent
            border.width: 1
            color: outerMouseArea.containsMouse ? "cyan" : "transparent"
        }

        Button {
            id: buttonInTheMiddle
            width: parent.width - 20
            height: parent.height - 20
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: hovered ? "hovered" : ""

            MouseArea {
                id: innerMouseArea
                width: parent.width - 20
                height: parent.height - 20
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                hoverEnabled: true
                Rectangle {
                    anchors.fill: parent
                    border.width: 1
                    color: innerMouseArea.containsMouse ? "yellow" : "transparent"
                }
            }
        }
    }
}
