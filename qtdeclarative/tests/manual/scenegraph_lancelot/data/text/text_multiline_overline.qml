import QtQuick 2.0

Item {
    width: 320
    height: 480
    clip: true

    Text {
        anchors.centerIn: parent
        font.pixelSize: 16
        font.overline: true
        text: "First line\nSecond line\nThird line"
    }

}
