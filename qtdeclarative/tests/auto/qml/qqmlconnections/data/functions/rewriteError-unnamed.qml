import QtQuick 2.0
import Test 1.0

TestObject {
    property QtObject connection: Connections {
        function onUnnamedArgumentSignal() { ran = true }
    }
}
