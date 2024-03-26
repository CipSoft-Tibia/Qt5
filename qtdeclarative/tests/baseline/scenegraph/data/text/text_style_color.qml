import QtQuick 2.0

//vary font style and style color

Item {
    width: 320
    height: 480
    Text {
        id: text_0000
        anchors.top: parent.top
        anchors.left: parent.left
        width: 150
        text: "The quick brown fox jumps over the lazy dog. 0123456789. style: Normal"
        style: Text.Normal
        styleColor: "red"
        color: "black"
        font.family: "Arial"
        font.pointSize: 12
        wrapMode: Text.Wrap
    }
    Text {
        id: text_0001
        anchors.top: text_0000.bottom
        anchors.left: parent.left
        width: 150
        text: "The quick brown fox jumps over the lazy dog. 0123456789. style: Outline"
        style: Text.Outline
        styleColor: "blue"
        color: "black"
        font.family: "Helvetica"
        font.pointSize: 12
        wrapMode: Text.Wrap
    }
    Text {
        id: text_0002
        width: 150
        anchors.top: text_0001.bottom
        anchors.left: text_0001.left
        text: "The quick brown fox jumps over the lazy dog. 0123456789. style: Raised"
        style: Text.Raised
        styleColor: "red"
        color: "black"
        font.family: "Courier"
        font.pointSize: 12
        wrapMode: Text.Wrap
    }
    Text {
        id: text_0003
        width: 150
        anchors.top: text_0002.bottom
        anchors.left: text_0002.left
        text: "The quick brown fox jumps over the lazy dog. 0123456789. style: Sunken"
        style: Text.Sunken
        styleColor: "yellow"
        color: "black"
        font.family: "Calibri"
        font.pointSize: 12
        wrapMode: Text.Wrap
    }
}
