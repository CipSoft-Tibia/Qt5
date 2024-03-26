// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Window 2.0

import QtTest 1.1

Item {
    Rectangle {
        id: box
        color: "red"
    }

    ShaderEffectSource {
        id: theSource
        sourceItem: box
    }

    Window {
        id: childWindow

        width: 100
        height: 100

        property bool rendered: false;
        visible: true
        onFrameSwapped: rendered = true;

        ShaderEffect {
            property variant source: theSource;
            anchors.fill: parent
        }
    }

    TestCase {
        name: "shadersource-from-other-window"
        when: childWindow.rendered
        function test_endresult() {
            verify(true); // that we got here without problems...
        }
    }
}
