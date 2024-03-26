// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "RadioButton"

    Component {
        id: radioButton
        RadioButton { }
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "pressedChanged", "checkedChanged"]
        }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(radioButton, testCase)
        verify(control)
    }

    function test_text() {
        var control = createTemporaryObject(radioButton, testCase)
        verify(control)

        compare(control.text, "")
        control.text = "RadioButton"
        compare(control.text, "RadioButton")
        control.text = ""
        compare(control.text, "")
    }

    function test_checked() {
        var control = createTemporaryObject(radioButton, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = [] // No change expected
        compare(control.checked, false)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = ["checkedChanged"]
        control.checked = true
        compare(control.checked, true)
        verify(sequenceSpy.success)

        sequenceSpy.reset()
        control.checked = false
        compare(control.checked, false)
        verify(sequenceSpy.success)
    }

    function test_mouse() {
        var control = createTemporaryObject(radioButton, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                        ["checkedChanged", { "pressed": false, "checked": true }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // attempt uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }]]
        mouseMove(control, control.width * 2, control.height * 2, 0)
        compare(control.pressed, false)
        sequenceSpy.expectedSequence = [["canceled", { "pressed": false, "checked": true }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // right button
        sequenceSpy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_touch() {
        var control = createTemporaryObject(radioButton, testCase)
        verify(control)

        var touch = touchEvent(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                        ["checkedChanged", { "pressed": false, "checked": true }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // attempt uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed"]
        // Don't want to double-click.
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed"]
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }]]
        touch.move(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        sequenceSpy.expectedSequence = [["canceled", { "pressed": false, "checked": true }]]
        touch.release(0, control, control.width * 2, control.height * 2).commit()
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_keys() {
        var control = createTemporaryObject(radioButton, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(sequenceSpy.success)

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false }],
                                        ["checkedChanged", { "pressed": false, "checked": true }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        verify(sequenceSpy.success)

        // attempt uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        verify(sequenceSpy.success)

        // no change
        sequenceSpy.expectedSequence = []
        // Not testing Key_Enter and Key_Return because QGnomeTheme uses them for
        // pressing buttons and the CI uses the QGnomeTheme platform theme.
        var keys = [Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            sequenceSpy.reset()
            keyClick(keys[i])
            compare(control.checked, true)
            verify(sequenceSpy.success)
        }
    }

    Component {
        id: twoRadioButtons
        Item {
            property RadioButton rb1: RadioButton { id: rb1 }
            property RadioButton rb2: RadioButton { id: rb2; checked: rb1.checked; enabled: false }
        }
    }

    function test_binding() {
        var container = createTemporaryObject(twoRadioButtons, testCase)
        verify(container)

        compare(container.rb1.checked, false)
        compare(container.rb2.checked, false)

        container.rb1.checked = true
        compare(container.rb1.checked, true)
        compare(container.rb2.checked, true)

        container.rb1.checked = false
        compare(container.rb1.checked, false)
        compare(container.rb2.checked, false)
    }

    Component {
        id: radioButtonGroup
        Column {
            // auto-exclusive buttons behave as if they were in their own exclusive group
            RadioButton { }
            RadioButton { }

            // explicitly grouped buttons are only exclusive with each other, not with
            // auto-exclusive buttons, and the autoExclusive property is ignored
            ButtonGroup { id: eg }
            RadioButton { ButtonGroup.group: eg }
            RadioButton { ButtonGroup.group: eg; autoExclusive: false }

            ButtonGroup { id: eg2 }
            RadioButton { id: rb1; Component.onCompleted: eg2.addButton(rb1) }
            RadioButton { id: rb2; Component.onCompleted: eg2.addButton(rb2) }

            // non-exclusive buttons don't affect the others
            RadioButton { autoExclusive: false }
            RadioButton { autoExclusive: false }
        }
    }

    function test_autoExclusive() {
        var container = createTemporaryObject(radioButtonGroup, testCase)
        compare(container.children.length, 8)

        var checkStates = [false, false, false, false, false, false, false, false]
        for (var i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[0].checked = true
        checkStates[0] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[1].checked = true
        checkStates[0] = false
        checkStates[1] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[2].checked = true
        checkStates[2] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[3].checked = true
        checkStates[2] = false
        checkStates[3] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[4].checked = true
        checkStates[4] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[5].checked = true
        checkStates[4] = false
        checkStates[5] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[6].checked = true
        checkStates[6] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[7].checked = true
        checkStates[7] = true
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])

        container.children[0].checked = true
        checkStates[0] = true
        checkStates[1] = false
        for (i = 0; i < 8; ++i)
            compare(container.children[i].checked, checkStates[i])
    }

    function test_baseline() {
        var control = createTemporaryObject(radioButton, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }
}
