import QtQuick 2.0

QtObject {
    id: root

    property real realProperty
    property alias aliasProperty: root.realProperty
}

