import QtQuick 2.0

QtObject {
    id: root

    property int testProperty
    property alias aliasProperty: root.testProperty
}

