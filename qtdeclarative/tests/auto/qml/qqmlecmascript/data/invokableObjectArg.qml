import Qt.test 1.0
import QtQuick 2.0

MyQmlObject {
    id: root
    Component.onCompleted: {
        root.myinvokable(root);
    }
}
