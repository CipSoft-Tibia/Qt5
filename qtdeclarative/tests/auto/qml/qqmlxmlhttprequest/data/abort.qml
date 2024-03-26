import QtQuick 2.0

QtObject {
    property string urlDummy
    property string url

    property bool seenDone: false
    property bool didNotSeeUnsent: true
    property bool endStateUnsent: false
    property bool dataOK: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.open("GET", urlDummy);
        x.setRequestHeader("Test-header", "TestValue");
        x.send();

        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.DONE)
                seenDone = true;
            else if (x.readyState == XMLHttpRequest.UNSENT)
                didNotSeeUnsent = false;
        }

        x.abort();

        if (x.readyState == XMLHttpRequest.UNSENT)
            endStateUnsent = true;

        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.DONE)
                dataOK = (x.responseText == "QML Rocks!\n");
        }
        x.open("PUT", url);
        x.send("Test Data\n");
    }
}
