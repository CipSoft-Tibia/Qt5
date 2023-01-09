import QtQuick 2.0

Item {
    width: 640
    height: 480
    Column {
        objectName: "column"
        Rectangle {
            objectName: "one"
            color: "red"
            width: 50
            height: 50
        }
        Rectangle {
            objectName: "two"
            color: "red"
            width: 20
            height: 10
        }
        Rectangle {
            objectName: "three"
            color: "red"
            width: 40
            height: 20
        }
    }
}
