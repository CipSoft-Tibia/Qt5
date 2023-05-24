// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtTest

import QmlTestUri
import qtgrpc.tests

TestCase {
    id: root
    name: "qtgrpcClientRegistration"

    property simpleStringMessage messageArg;
    property simpleStringMessage messageResponse;

    property var clientQml
    property var setResponse: function(value) { root.messageResponse = value }
    property var errorCallback: function() { console.log("Error is handled!") }

    function createItem() {
        return Qt.createQmlObject("import QtQuick; import qtgrpc.tests; Client {  }", root)
    }

    function test_1initialization() {
        clientQml = root.createItem()
    }

    function test_clientTypeIsObject() {
        compare(typeof clientQml, "object")
    }

    function test_clientTypeTestMethod()
    {
        // There is no established connection yet, so warning is generated
        for (var i = 0; i < 2; i++)
            ignoreWarning("QObject::connect(QGrpcOperation, QQmlEngine): invalid nullptr parameter")
        clientQml.testMethod(root.messageArg, root.setResponse, root.errorCallback)
    }

}
