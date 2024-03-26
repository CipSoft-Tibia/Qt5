import QtQuick

Item {
    id: root
    width: 300
    height: 300
    visible: true

    palette.active.base: "blue"
    palette.inactive.base: "red"
    palette.disabled.base: "gray"

    Rectangle {
        id: background
        objectName: "background"

        anchors.centerIn: parent
        width: parent.width / 2
        height: parent.height / 2

        color: root.palette.base

        Rectangle {
            id: foreground
            objectName: "foreground"

            anchors.centerIn: parent
            width: parent.width / 2
            height: parent.height / 2

            color: root.palette.base
            border.color: "black"
        }
    }
}
