// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtGraphs
import QtTest

Item {
    id: top
    height: 150
    width: 150

    LineSeries {
        id: initial
    }

    LineSeries {
        id: initialized

        axisX: ValueAxis { max: 4 }
        axisY: ValueAxis { max: 8 }
        width: 10.0
        capStyle: Qt.RoundCap
        pointMarker: Rectangle {
            width: 5
            height: 5
        }

        color: "#ff00ff"
        selectedColor: "#00ff00"
        markerSize: 5.0

        name: "LineSeries"
        visible: false
        selectable: true
        hoverable: true
        opacity: 0.75
        valuesMultiplier: 0.75
    }

    // Values used for changing the properties
    ValueAxis { id: axisx; max: 10 }
    ValueAxis { id: axisy; max: 10 }
    Component { id: marker; Rectangle { width: 10; height: 10 } }

    TestCase {
        name: "LineSeries Initial"

        function test_1_initial() {
            // Properties from QLineSeries
            compare(initial.axisX, null)
            compare(initial.axisY, null)
            compare(initial.width, 2.0)
            compare(initial.capStyle, Qt.SquareCap)
            compare(initial.pointMarker, null)
        }

        function test_2_initial_common() {
            // Properties from QXYSeries
            compare(initial.color, "#ffffff")
            compare(initial.selectedColor, "#000000")
            compare(initial.markerSize, 15.0)

            // Properties from QAbstractSeries
            compare(initial.theme, null)
            compare(initial.name, "")
            compare(initial.visible, true)
            compare(initial.selectable, false)
            compare(initial.hoverable, false)
            compare(initial.opacity, 1.0)
            compare(initial.valuesMultiplier, 1.0)
        }

        function test_3_initial_change() {
            initial.axisX = axisx
            initial.axisY = axisy
            initial.width = 10.0
            initial.capStyle = Qt.RoundCap
            initial.pointMarker = marker

            initial.color = "#ff00ff"
            initial.selectedColor = "#00ff00"
            initial.markerSize = 5.0

            initial.name = "Lines"
            initial.visible = false
            initial.selectable = true
            initial.hoverable = true
            initial.opacity = 0.5
            initial.valuesMultiplier = 0.5

            compare(initial.axisX, axisx)
            compare(initial.axisY, axisy)
            compare(initial.axisX.max, 10)
            compare(initial.axisY.max, 10)
            compare(initial.width, 10.0)
            compare(initial.capStyle, Qt.RoundCap)
            compare(initial.pointMarker, marker)

            compare(initial.color, "#ff00ff")
            compare(initial.selectedColor, "#00ff00")
            compare(initial.markerSize, 5.0)

            compare(initial.name, "Lines")
            compare(initial.visible, false)
            compare(initial.selectable, true)
            compare(initial.hoverable, true)
            compare(initial.opacity, 0.5)
            compare(initial.valuesMultiplier, 0.5)
        }
    }

    TestCase {
        name: "LineSeries Initialized"

        function test_1_initialized() {
            compare(initialized.axisX.max, 4)
            compare(initialized.axisY.max, 8)
            compare(initialized.width, 10.0)
            compare(initialized.capStyle, Qt.RoundCap)
            verify(initialized.pointMarker)

            compare(initialized.color, "#ff00ff")
            compare(initialized.selectedColor, "#00ff00")
            compare(initialized.markerSize, 5.0)

            compare(initialized.name, "LineSeries")
            compare(initialized.visible, false)
            compare(initialized.selectable, true)
            compare(initialized.hoverable, true)
            compare(initialized.opacity, 0.75)
            compare(initialized.valuesMultiplier, 0.75)
        }

        function test_2_initialized_change() {
            initialized.axisX = axisx
            initialized.axisY = axisy
            initialized.width = 1.0
            initialized.capStyle = Qt.SquareCap
            initialized.pointMarker = null

            initialized.color = "#0000ff"
            initialized.selectedColor = "#ff0000"
            initialized.markerSize = 10.0

            initialized.name = "Lines"
            initialized.visible = true
            initialized.selectable = false
            initialized.hoverable = false
            initialized.opacity = 0.5
            initialized.valuesMultiplier = 0.25

            compare(initialized.axisX.max, 10)
            compare(initialized.axisY.max, 10)
            compare(initialized.width, 1.0)
            compare(initialized.capStyle, Qt.SquareCap)
            verify(!initialized.pointMarker)

            compare(initialized.color, "#0000ff")
            compare(initialized.selectedColor, "#ff0000")
            compare(initialized.markerSize, 10.0)

            compare(initialized.name, "Lines")
            compare(initialized.visible, true)
            compare(initialized.selectable, false)
            compare(initialized.hoverable, false)
            compare(initialized.opacity, 0.5)
            compare(initialized.valuesMultiplier, 0.25)
        }

        function test_3_initialized_change_to_null() {
            initialized.axisX = null
            initialized.axisY = null

            verify(!initialized.axisX)
            verify(!initialized.axisY)
        }

        function test_4_initialized_change_to_invalid() {
            initialized.width = -10.0
            initialized.capStyle = -1
            initialized.valuesMultiplier = 2.0 // range 0...1

            compare(initialized.width, 0.0)
            compare(initialized.capStyle, Qt.SquareCap)
            compare(initialized.valuesMultiplier, 1.0)

            initialized.valuesMultiplier = -1.0 // range 0...1
            compare(initialized.valuesMultiplier, 0.0)
        }
    }
}
