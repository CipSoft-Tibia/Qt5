// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.2
import QtTest 1.0

TestCase {
    id: testCase
    name: "Tests_Layout"
    when:windowShown
    width:400
    height:400

    function test_invalidParent() {
        ignoreWarning('<Unknown File>:1:49: QML QtObject: Layout must be attached to Item elements')
        var object = Qt.createQmlObject('import QtQuick 2.2; import QtQuick.Layouts 1.0; QtObject { Layout.fillWidth: true }', testCase, '');
        object.destroy()
    }

    function test_defaultPropertyAliasCrash() {
        var containerUserComponent = Qt.createComponent("ContainerUser.qml");
        compare(containerUserComponent.status, Component.Ready);

        var containerUser = containerUserComponent.createObject(testCase);
        verify(containerUser);

        // Shouldn't crash.
        containerUser.destroy();
    }
}

