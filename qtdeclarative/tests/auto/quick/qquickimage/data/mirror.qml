import QtQuick 2.0

Rectangle {
    width: 300
    height: 250
    Image {
        objectName: "image"
        smooth: false
        anchors.fill: parent
        source: "pattern.png"
    }
}
