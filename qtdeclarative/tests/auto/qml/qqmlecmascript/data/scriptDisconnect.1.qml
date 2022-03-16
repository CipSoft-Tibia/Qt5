import Qt.test 1.0
import QtQuick 2.0
import "scriptDisconnect.1.js" as Script

MyQmlObject {
    property int test: 0

    id: root

    Component.onCompleted: root.argumentSignal.connect(Script.testFunction);

    onBasicSignal: root.argumentSignal.disconnect(Script.testFunction);
}
