import Qt.test 1.0
import QtQuick 2.0

MyQmlObject {
    objectProperty: if(1) otherObject

    property variant obj

    obj: QtObject {
        id: otherObject
    }
}

