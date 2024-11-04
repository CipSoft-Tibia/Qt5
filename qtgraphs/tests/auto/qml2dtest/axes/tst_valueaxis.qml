// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtGraphs
import QtTest

Item {
    id: top
    height: 150
    width: 150

    ValueAxis {
        id: initial
    }

    Text {
        id: initializedDummy
        font.family: "Sans Serif"
        font.bold: true
    }

    ValueAxis {
        id: initialized

        labelDecimals: 2
        labelFormat: "f"
        min: 2.0
        max: 20.0
        minorTickCount: 1
        tickAnchor: 1
        tickInterval: 2.0

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
        name: "ValueAxis Initial"

        Text { id: dummy }

        function test_1_initial() {
            compare(initial.labelDecimals, -1)
            compare(initial.labelFormat, "")
            compare(initial.min, 0.0)
            compare(initial.max, 10.0)
            compare(initial.minorTickCount, 0)
            compare(initial.tickAnchor, 0)
            compare(initial.tickInterval, 0)
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

            // Properties from ValueAxis
            initial.labelDecimals = 1
            initial.labelFormat = "d"
            initial.min = -10
            initial.max = 0
            initial.minorTickCount = 2
            initial.tickAnchor = 2
            initial.tickInterval = 3.0

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

            // Properties from ValueAxis
            compare(initial.labelDecimals, 1)
            compare(initial.labelFormat, "d")
            compare(initial.min, -10.0)
            compare(initial.max, 0.0)
            compare(initial.minorTickCount, 2)
            compare(initial.tickAnchor, 2)
            compare(initial.tickInterval, 3.0)

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
        name: "ValueAxis Initialized"

        function test_1_initialized() {
            // Properties from ValueAxis
            compare(initialized.labelDecimals, 2)
            compare(initialized.labelFormat, "f")
            compare(initialized.min, 2.0)
            compare(initialized.max, 20.0)
            compare(initialized.minorTickCount, 1)
            compare(initialized.tickAnchor, 1)
            compare(initialized.tickInterval, 2.0)

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
            // Properties from ValueAxis
            initialized.labelDecimals = 1
            initialized.labelFormat = "d"
            initialized.min = -10.0
            initialized.max = 0.0
            initialized.minorTickCount = 2
            initialized.tickAnchor = 2
            initialized.tickInterval = 3.0

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

            // Properties from ValueAxis
            compare(initialized.labelDecimals, 1)
            compare(initialized.labelFormat, "d")
            compare(initialized.min, -10.)
            compare(initialized.max, 0.0)
            compare(initialized.minorTickCount, 2)
            compare(initialized.tickAnchor, 2)
            compare(initialized.tickInterval, 3.0)

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

        function test_3_initialized_change_to_invalid() {
            initialized.max = -10.0
            initialized.min = 10.0
            initialized.minorTickCount = -1

            compare(initialized.max, 10.0)
            compare(initialized.min, 10.0)
            compare(initialized.minorTickCount, 2) // This was set in previous test case
        }
    }
}
