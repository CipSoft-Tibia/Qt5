import QtQuick 2.0

QtObject {
    property bool emptyArg: false

    property string relativeUrl
    property string absoluteUrl

    property QtObject incorectArgCount1: Qt.createComponent()
    property QtObject incorectArgCount2: Qt.createComponent("main.qml", 10)

    property bool asyncResult: false
    property var asyncComponent

    function asyncStatusChanged() {
        if (asyncComponent.status == Component.Ready)
            asyncResult = true;
    }

    Component.onCompleted: {
        emptyArg = (Qt.createComponent("") == null);
        var r = Qt.createComponent("createComponentData.qml");
        relativeUrl = r.url;

        var a = Qt.createComponent("http://www.example.com/test.qml");
        absoluteUrl = a.url;

        asyncComponent = Qt.createComponent("TestComponent.qml", Component.Asynchronous);
        if (asyncComponent.status != Component.Loading)
            return;
        asyncComponent.statusChanged.connect(asyncStatusChanged);
    }
}
