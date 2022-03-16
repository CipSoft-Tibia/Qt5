import QtQuick 2.0

Item {
    width: 400; height: 500;
    property int bindingWidth: text.width
    property int bindingHeight: text.height

    TextInput {
        id: text
        anchors.fill: parent
    }
}
