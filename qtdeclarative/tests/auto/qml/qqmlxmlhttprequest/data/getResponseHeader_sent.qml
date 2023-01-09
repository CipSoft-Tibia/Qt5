import QtQuick 2.0

QtObject {
    property bool test: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;

        x.open("GET", "testdocument.html");
        x.send();

        try {
            x.getResponseHeader("Test-header");
        } catch (e) {
            if (e.code == DOMException.INVALID_STATE_ERR)
                test = true;
        }
    }
}

