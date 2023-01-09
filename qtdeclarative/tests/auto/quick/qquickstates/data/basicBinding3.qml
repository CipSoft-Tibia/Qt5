import QtQuick 2.0
Rectangle {
    id: myRectangle

    property color sourceColor: "red"
    property color sourceColor2: "blue"
    width: 100; height: 100
    color: sourceColor
    states: State {
        name: "blue"
        PropertyChanges { target: myRectangle; color: sourceColor2 }
    }
}
