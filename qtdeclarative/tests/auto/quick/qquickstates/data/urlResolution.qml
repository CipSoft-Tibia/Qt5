import QtQuick 2.0
import "Implementation"

Rectangle {
    width: 100
    height: 200

    MyType {
        objectName: "MyType"
        anchors.fill: parent
    }
}
