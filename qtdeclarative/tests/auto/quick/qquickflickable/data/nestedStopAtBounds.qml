import QtQuick 2.0

Flickable {
    id: outer
    objectName: "outerFlickable"
    width: 400
    height: 400
    contentX: 50
    contentY: 50
    contentWidth: 500
    contentHeight: 500
    flickableDirection: inner.flickableDirection

    Rectangle {
        x: 100
        y: 100
        width: 300
        height: 300

        color: "yellow"
        objectName: "yellowRect"

        Flickable {
            id: inner
            objectName: "innerFlickable"
            anchors.fill: parent
            contentX: 100
            contentY: 100
            contentWidth: 400
            contentHeight: 400
            boundsBehavior: Flickable.StopAtBounds
            Rectangle {
                anchors.fill: parent
                anchors.margins: 100
                objectName: "blueRect"
                color: "blue"
            }
            MouseArea {
                anchors.fill: parent
                objectName: "mouseArea"
            }
        }
    }
}
