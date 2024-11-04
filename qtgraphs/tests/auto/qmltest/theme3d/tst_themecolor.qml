// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtGraphs
import QtTest 1.0

Item {
    id: top
    width: 150
    height: 150

    Color {
        id: initial
    }

    Color {
        id: initialized
        color: "red"
    }

    Color {
        id: change
    }

    TestCase {
        name: "Color Initial"

        function test_initial() {
            compare(initial.color, "#000000")
        }
    }

    TestCase {
        name: "Color Initialized"

        function test_initialized() {
            compare(initialized.color, "#ff0000")
        }
    }

    TestCase {
        name: "Color Change"

        function test_change() {
            change.color = "blue"

            compare(change.color, "#0000ff")
        }
    }
}
