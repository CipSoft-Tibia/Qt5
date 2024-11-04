// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtGraphs
import QtTest

Item {
    id: top
    height: 150
    width: 150

    BarSet {
        id: initial
    }

    BarSet {
        id: initialized
        label: "Set1"
        labelColor: "green"
        color: "red"
        borderColor: "black"
        borderWidth: 3
        values: [1, 2, 3, 4, 5, 6]
    }

    TestCase {
        name: "BarSet Initial"

        function test_1_initial() {
            compare(initial.label, "")
            compare(initial.labelColor, "#00000000")
            compare(initial.color, "#00000000")
            compare(initial.borderColor, "#00000000")
            compare(initial.borderWidth, -1)
            compare(initial.values, [])
            compare(initial.count, 0)
            compare(initial.selectedBars, [])
        }

        function test_2_initial_change() {
            var values = [1, 2, 3]

            initial.label = "BarSet"
            initial.labelColor = "red"
            initial.color = "blue"
            initial.borderColor = "yellow"
            initial.borderWidth = 2
            initial.values = values

            compare(initial.label, "BarSet")
            compare(initial.labelColor, "#ff0000")
            compare(initial.color, "#0000ff")
            compare(initial.borderColor, "#ffff00")
            compare(initial.borderWidth, 2)
            compare(initial.values, values)
            compare(initial.count, 3)
            compare(initial.selectedBars, [])
        }
    }

    TestCase {
        name: "BarSet Initialized"

        function test_1_initialized() {
            compare(initialized.label, "Set1")
            compare(initialized.labelColor, "#008000")
            compare(initialized.color, "#ff0000")
            compare(initialized.borderColor, "#000000")
            compare(initialized.borderWidth, 3)
            compare(initialized.values, [1, 2, 3, 4, 5, 6])
            compare(initialized.count, 6)
            compare(initialized.selectedBars, [])
        }

        function test_2_initialized_change() {
            var values = [10, 20, 30]

            initialized.label = "BarSet"
            initialized.labelColor = "red"
            initialized.color = "blue"
            initialized.borderColor = "yellow"
            initialized.borderWidth = 2
            initialized.values = values

            compare(initialized.label, "BarSet")
            compare(initialized.labelColor, "#ff0000")
            compare(initialized.color, "#0000ff")
            compare(initialized.borderColor, "#ffff00")
            compare(initialized.borderWidth, 2)
            compare(initialized.values, values)
            compare(initialized.count, 3)
            compare(initialized.selectedBars, [])
        }

        function test_3_initialized_change_to_null() {
            initialized.label = ""
            initialized.values = []

            compare(initialized.label, "")
            compare(initialized.values, [])
            compare(initialized.count, 0)
            compare(initialized.selectedBars, [])
        }
    }
}
