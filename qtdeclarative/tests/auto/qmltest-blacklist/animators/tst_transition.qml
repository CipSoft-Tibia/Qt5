// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testcase
        name: "animators-transition"
        when: box.scale == 2
        function test_endresult() {
            compare(box.scaleChangeCounter, 1);
            compare(box.scale, 2);
            var image = grabImage(root);
            verify(image.pixel(0, 0) == Qt.rgba(1, 0, 0));
            verify(image.pixel(199, 199) == Qt.rgba(0, 0, 1));
        }
    }

    states: [
        State {
            name: "one"
            PropertyChanges { target: box; scale: 1 }
        },
        State {
            name: "two"
            PropertyChanges { target: box; scale: 2 }
        }
    ]
    state: "one"

    transitions: [
        Transition {
            ScaleAnimator { duration: 100; }
        }
    ]

    Box {
        id: box
    }

    Timer {
        interval: 100;
        repeat: false
        running: true
        onTriggered: root.state = "two"
    }
}
