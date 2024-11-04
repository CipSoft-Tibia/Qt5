// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtGraphs
import QtTest

Item {
    id: top
    height: 150
    width: 150

    BarCategoryAxis {
        id: initial
    }

    Text {
        id: initializedDummy
        font.family: "Sans Serif"
        font.bold: true
    }

    BarCategoryAxis {
        id: initialized

        min: "min"
        max: "max"
        categories: ["min", "max"]

        // alignment: Qt.AlignTop // read-only
        gridVisible: false
        labelsAngle: 25
        labelsVisible: false
        lineVisible: false
        minorGridVisible: false
        // orientation: Qt.Vertical // read-only
        titleColor: "#ff0000"
        titleFont: initializedDummy.font
        titleText: "Initialized"
        titleVisible: false
        visible: false
    }

    TestCase {
        name: "BarCategoryAxis Initial"

        Text { id: dummy }

        function test_1_initial() {
            compare(initial.categories, [])
            compare(initial.count, 0)
            compare(initial.min, "")
            compare(initial.max, "")
        }

        function test_2_initial_common() {
            // Common properties from AbstractAxis
            compare(initial.alignment, 0)
            compare(initial.gridVisible, true)
            compare(initial.labelsAngle, 0)
            compare(initial.labelsVisible, true)
            compare(initial.lineVisible, true)
            compare(initial.minorGridVisible, true)
            compare(initial.orientation, 0)
            compare(initial.titleColor, "#000000")
            // Initial font needs to be tested like this, as different platforms have different default font (QFont())
            compare(initial.titleFont.family, dummy.font.family)
            compare(initial.titleText, "")
            compare(initial.titleVisible, true)
            compare(initial.visible, true)
        }

        function test_3_initial_change() {
            dummy.font.family = "Arial"

            // Properties from BarCategoryAxis
            initial.categories = ["one", "two"]
            initial.min = "zero"
            initial.max = "three"

            // Common properties from AbstractAxis
            // initial.alignment = Qt.AlignRight // read-only
            initial.gridVisible = false
            initial.labelsAngle = 45
            initial.labelsVisible = false
            initial.lineVisible = false
            initial.minorGridVisible = false
            // initial.orientation = Qt.Horizontal // read-only
            initial.titleColor = "#ffffff"
            initial.titleFont = dummy.font
            initial.titleText = "Dummy"
            initial.titleVisible = false
            initial.visible = false

            // Properties from BarCategoryAxis
            compare(initial.categories, ["one", "two"])
            compare(initial.count, 2)
            // TODO: QTBUG-121718
            // compare(initial.min, "zero")
            // compare(initial.max, "three")

            // Common properties from AbstractAxis
            // compare(initial.alignment, Qt.AlignRight) // read-only
            compare(initial.gridVisible, false)
            compare(initial.labelsAngle, 45)
            compare(initial.labelsVisible, false)
            compare(initial.lineVisible, false)
            compare(initial.minorGridVisible, false)
            // compare(initial.orientation, Qt.Horizontal) // read-only
            compare(initial.titleColor, "#ffffff")
            compare(initial.titleFont, dummy.font)
            compare(initial.titleText, "Dummy")
            compare(initial.titleVisible, false)
            compare(initial.visible, false)
        }
    }

    TestCase {
        name: "BarCategoryAxis Initialized"

        function test_1_initialized() {
            // Properties from BarCategoryAxis
            compare(initialized.categories, ["min", "max"])
            compare(initialized.count, 2)
            compare(initialized.min, "min")
            compare(initialized.max, "max")

            // Common properties from AbstractAxis
            // compare(initialized.alignment, Qt.AlignTop) // read-only
            compare(initialized.gridVisible, false)
            compare(initialized.labelsAngle, 25)
            compare(initialized.labelsVisible, false)
            compare(initialized.lineVisible, false)
            compare(initialized.minorGridVisible, false)
            // compare(initialized.orientation, Qt.Vertical) // read-only
            compare(initialized.titleColor, "#ff0000")
            compare(initialized.titleFont, initializedDummy.font)
            compare(initialized.titleText, "Initialized")
            compare(initialized.titleVisible, false)
            compare(initialized.visible, false)
        }

        function test_2_initialized_change() {
            // Properties from BarCategoryAxis
            initialized.categories = ["one", "two"]
            initialized.min = "zero"
            initialized.max = "three"

            // Common properties from AbstractAxis
            // initialized.alignment = Qt.AlignRight // read-only
            initialized.gridVisible = true
            initialized.labelsAngle = 45
            initialized.labelsVisible = true
            initialized.lineVisible = true
            initialized.minorGridVisible = true
            // initialized.orientation = Qt.Horizontal // read-only
            initialized.titleColor = "#ffffff"
            initialized.titleFont = dummy.font
            initialized.titleText = "Dummy"
            initialized.titleVisible = true
            initialized.visible = true

            // Properties from BarCategoryAxis
            compare(initialized.categories, ["one", "two"])
            compare(initialized.count, 2)
            // TODO: QTBUG-121718
            // compare(initialized.min, "zero")
            // compare(initialized.max, "three")

            // Common properties from AbstractAxis
            // compare(initialized.alignment, Qt.AlignTop) // read-only
            compare(initialized.gridVisible, true)
            compare(initialized.labelsAngle, 45)
            compare(initialized.labelsVisible, true)
            compare(initialized.lineVisible, true)
            compare(initialized.minorGridVisible, true)
            // compare(initialized.orientation, Qt.Vertical) // read-only
            compare(initialized.titleColor, "#ffffff")
            compare(initialized.titleFont, dummy.font)
            compare(initialized.titleText, "Dummy")
            compare(initialized.titleVisible, true)
            compare(initialized.visible, true)
        }

        function test_3_initialized_clear() {
            initialized.clear()

            compare(initialized.categories, [])
            compare(initialized.count, 0)
            compare(initialized.min, "")
            compare(initialized.max, "")
        }
    }
}
