import Test 1.0
import QtQuick 2.0

QtObject {
    property variant other
    other: MyTypeObject { id: obj }
    property alias enumAlias: obj.enumProperty;
}

