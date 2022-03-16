/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtTest 1.0
import QtQuick.Controls 2.12

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "CheckBox"

    Component {
        id: checkBox
        CheckBox { }
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "pressedChanged", "checkedChanged", "checkStateChanged"]
        }
    }

    function test_text() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)

        compare(control.text, "")
        control.text = "CheckBox"
        compare(control.text, "CheckBox")
        control.text = ""
        compare(control.text, "")
    }

    function test_checked() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "checked": true, "checkState": Qt.Checked }]]
        control.checked = true
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "checked": false, "checkState": Qt.Unchecked }]]
        control.checked = false
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)
    }

    function test_checkState() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "checked": true, "checkState": Qt.Checked }]]
        control.checkState = Qt.Checked
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "checked": false, "checkState": Qt.Unchecked }]]
        control.checkState = Qt.Unchecked
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)
    }

    function test_mouse() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        mouseMove(control, control.width * 2, control.height * 2, 0)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["canceled", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // right button
        sequenceSpy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_touch() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)

        var touch = touchEvent(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        touch.move(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["canceled", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        touch.release(0, control, control.width * 2, control.height * 2).commit()
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_keys() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(sequenceSpy.success)

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        // no change
        sequenceSpy.expectedSequence = []
        var keys = [Qt.Key_Enter, Qt.Key_Return, Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            sequenceSpy.reset()
            keyClick(keys[i])
            compare(control.checked, false)
            verify(sequenceSpy.success)
        }
    }

    Component {
        id: checkedBoundBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checked: cb1.checked; enabled: false }
        }
    }

    function test_checked_binding() {
        var container = createTemporaryObject(checkedBoundBoxes, testCase)
        verify(container)

        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        container.cb1.checked = true
        compare(container.cb1.checked, true)
        compare(container.cb1.checkState, Qt.Checked)
        compare(container.cb2.checked, true)
        compare(container.cb2.checkState, Qt.Checked)

        container.cb1.checked = false
        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)
    }

    Component {
        id: checkStateBoundBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checkState: cb1.checkState; enabled: false }
        }
    }

    function test_checkState_binding() {
        var container = createTemporaryObject(checkStateBoundBoxes, testCase)
        verify(container)

        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        container.cb1.checkState = Qt.Checked
        compare(container.cb1.checked, true)
        compare(container.cb1.checkState, Qt.Checked)
        compare(container.cb2.checked, true)
        compare(container.cb2.checkState, Qt.Checked)

        container.cb1.checkState = Qt.Unchecked
        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        compare(container.cb1.tristate, false)
        compare(container.cb2.tristate, false)

        container.cb1.checkState = Qt.PartiallyChecked
        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.PartiallyChecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.PartiallyChecked)

        // note: since Qt Quick Controls 2.4 (Qt 5.11), CheckBox does not
        // force tristate when checkState is set to Qt.PartiallyChecked
        compare(container.cb1.tristate, false)
        compare(container.cb2.tristate, false)
    }

    function test_tristate() {
        var control = createTemporaryObject(checkBox, testCase, {tristate: true})

        var sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.tristate, true)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }]]
        control.checkState = Qt.PartiallyChecked
        compare(control.checked, false)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(sequenceSpy.success)

        // key: partial -> checked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        // key: checked -> unchecked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        // key: unchecked -> partial
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(sequenceSpy.success)

        // mouse: partial -> checked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        // mouse: checked -> unchecked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        // mouse: unchecked -> partial
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, false)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(sequenceSpy.success)
    }

    function test_baseline() {
        var control = createTemporaryObject(checkBox, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    Component {
        id: nextCheckStateBox
        CheckBox {
            tristate: true
            nextCheckState: function() {
                if (checkState === Qt.Checked)
                    return Qt.Unchecked
                else
                    return Qt.Checked
            }
        }
    }

    function test_nextCheckState_data() {
        return [
            { tag: "unchecked", checkState: Qt.Unchecked, expectedState: Qt.Checked },
            { tag: "partially-checked", checkState: Qt.PartiallyChecked, expectedState: Qt.Checked },
            { tag: "checked", checkState: Qt.Checked, expectedState: Qt.Unchecked }
        ]
    }

    function test_nextCheckState(data) {
        var control = createTemporaryObject(nextCheckStateBox, testCase)
        verify(control)

        // mouse
        control.checkState = data.checkState
        compare(control.checkState, data.checkState)
        mouseClick(control)
        compare(control.checkState, data.expectedState)

        // touch
        control.checkState = data.checkState
        compare(control.checkState, data.checkState)
        var touch = touchEvent(control)
        touch.press(0, control).commit().release(0, control).commit()
        compare(control.checkState, data.expectedState)

        // keyboard
        control.forceActiveFocus()
        tryCompare(control, "activeFocus", true)
        control.checkState = data.checkState
        compare(control.checkState, data.checkState)
        keyClick(Qt.Key_Space)
        compare(control.checkState, data.expectedState)
    }
}
