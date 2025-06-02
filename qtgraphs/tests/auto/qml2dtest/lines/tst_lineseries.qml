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

        width: 10.0
        capStyle: Qt.RoundCap
        pointDelegate: Rectangle {
            width: 5
            height: 5
        }

        color: "#ff00ff"
        selectedColor: "#00ff00"
        draggable: true

        name: "LineSeries"
        visible: false
        selectable: true
        hoverable: true
        opacity: 0.75
        valuesMultiplier: 0.75
    }

    // Values used for changing the properties
    Component { id: marker; Rectangle { width: 10; height: 10 } }

    TestCase {
        name: "LineSeries Initial"

        function test_1_initial() {
            // Properties from QLineSeries
            compare(initial.width, 2.0)
            compare(initial.capStyle, Qt.SquareCap)
            compare(initial.pointDelegate, null)
        }

        function test_2_initial_common() {
            // Properties from QXYSeries
            compare(initial.color, "#00000000")
            compare(initial.selectedColor, "#00000000")
            compare(initial.draggable, false)
            compare(initial.selectedPoints, [])

            // Properties from QAbstractSeries
            compare(initial.name, "")
            compare(initial.visible, true)
            compare(initial.selectable, false)
            compare(initial.hoverable, false)
            compare(initial.opacity, 1.0)
            compare(initial.valuesMultiplier, 1.0)
        }

        function test_3_initial_change() {
            initial.width = 10.0
            initial.capStyle = Qt.RoundCap
            initial.pointDelegate = marker

            initial.color = "#ff00ff"
            initial.selectedColor = "#00ff00"
            initial.draggable = true

            initial.name = "Lines"
            initial.visible = false
            initial.selectable = true
            initial.hoverable = true
            initial.opacity = 0.5
            initial.valuesMultiplier = 0.5

            compare(initial.width, 10.0)
            compare(initial.capStyle, Qt.RoundCap)
            compare(initial.pointDelegate, marker)

            compare(initial.color, "#ff00ff")
            compare(initial.selectedColor, "#00ff00")
            compare(initial.draggable, true)

            compare(initial.name, "Lines")
            compare(initial.visible, false)
            compare(initial.selectable, true)
            compare(initial.hoverable, true)
            compare(initial.opacity, 0.5)
            compare(initial.valuesMultiplier, 0.5)
        }
        function test_3_initial_selections() {
            initial.deselectAllPoints();
            initial.clear();
            compare(initial.selectedPoints, [])
            initial.append(0, 0)
            initial.append(1, 1)
            initial.append(2, 2)
            initial.append(3, 3)
            initial.append(4, 4)
            compare(initial.isPointSelected(0), false)
            initial.selectPoint(0);
            compare(initial.isPointSelected(0), true)
            compare(initial.selectedPoints, [0])
            initial.selectPoint(2);
            // Note: Checking just length as the order of the elements may differ
            compare(initial.selectedPoints.length, 2) // [0, 2]
            initial.toggleSelection([3])
            compare(initial.selectedPoints.length, 3) // [0, 2, 3]
            initial.toggleSelection([3])
            compare(initial.selectedPoints.length, 2) // [0, 2]
            initial.setPointSelected(0, false)
            compare(initial.selectedPoints.length, 1) // [0]
            initial.selectAllPoints()
            compare(initial.selectedPoints.length, 5) // [0, 1, 2, 3, 4]
            initial.deselectAllPoints()
            compare(initial.selectedPoints.length, 0)
            initial.selectPoints([1, 2, 3])
            compare(initial.selectedPoints.length, 3) // [1, 2, 3]
            initial.deselectPoints([2, 3, 4])
            compare(initial.selectedPoints.length, 1) // [1]
        }
    }

    TestCase {
        name: "LineSeries Initialized"

        function test_1_initialized() {
            compare(initialized.width, 10.0)
            compare(initialized.capStyle, Qt.RoundCap)
            verify(initialized.pointDelegate)

            compare(initialized.color, "#ff00ff")
            compare(initialized.selectedColor, "#00ff00")
            compare(initialized.draggable, true)

            compare(initialized.name, "LineSeries")
            compare(initialized.visible, false)
            compare(initialized.selectable, true)
            compare(initialized.hoverable, true)
            compare(initialized.opacity, 0.75)
            compare(initialized.valuesMultiplier, 0.75)
        }

        function test_2_initialized_change() {
            initialized.width = 1.0
            initialized.capStyle = Qt.SquareCap
            initialized.pointDelegate = null

            initialized.color = "#0000ff"
            initialized.selectedColor = "#ff0000"
            initialized.draggable = false

            initialized.name = "Lines"
            initialized.visible = true
            initialized.selectable = false
            initialized.hoverable = false
            initialized.opacity = 0.5
            initialized.valuesMultiplier = 0.25

            compare(initialized.width, 1.0)
            compare(initialized.capStyle, Qt.SquareCap)
            verify(!initialized.pointDelegate)

            compare(initialized.color, "#0000ff")
            compare(initialized.selectedColor, "#ff0000")
            compare(initialized.draggable, false)

            compare(initialized.name, "Lines")
            compare(initialized.visible, true)
            compare(initialized.selectable, false)
            compare(initialized.hoverable, false)
            compare(initialized.opacity, 0.5)
            compare(initialized.valuesMultiplier, 0.25)

            // LineSeries signals
            compare(widthSpy.count, 1)
            compare(capStyleSpy.count, 1)

            //QXYSeries signals
            compare(colorSpy.count, 1)
            compare(selectedColorSpy.count, 1)
            compare(pointDelegateSpy.count, 1)
            compare(draggableSpy.count, 1)
            compare(selectedPointsSpy.count, 0)
        }

        function test_3_initialized_change_to_invalid() {
            initialized.width = -10.0
            initialized.capStyle = -1
            initialized.valuesMultiplier = 2.0 // range 0...1

            compare(initialized.width, 0.0)
            compare(initialized.capStyle, Qt.SquareCap)
            compare(initialized.valuesMultiplier, 1.0)

            initialized.valuesMultiplier = -1.0 // range 0...1
            compare(initialized.valuesMultiplier, 0.0)
        }

        // LineSeries signals
        SignalSpy {
            id: widthSpy
            target: initialized
            signalName: "widthChanged"
        }

        SignalSpy {
            id: capStyleSpy
            target: initialized
            signalName: "capStyleChanged"
        }

        // QXYSeries signals
        SignalSpy {
            id: colorSpy
            target: initialized
            signalName: "colorChanged"
        }

        SignalSpy {
            id: selectedColorSpy
            target: initialized
            signalName: "selectedColorChanged"
        }

        SignalSpy {
            id: pointDelegateSpy
            target: initialized
            signalName: "pointDelegateChanged"
        }

        SignalSpy {
            id: draggableSpy
            target: initialized
            signalName: "draggableChanged"
        }

        SignalSpy {
            id: selectedPointsSpy
            target: initialized
            signalName: "selectedPointsChanged"
        }
    }
}
