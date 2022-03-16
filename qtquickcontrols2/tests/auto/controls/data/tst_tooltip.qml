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
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ToolTip"

    Component {
        id: toolTip
        ToolTip { }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    QtObject {
        id: object
    }

    SignalSpy {
        id: sharedSpy
        target: ToolTip.toolTip
    }

    function test_properties_data() {
        return [
            {tag: "text", property: "text", defaultValue: "", setValue: "Hello", signalName: "textChanged"},
            {tag: "delay", property: "delay", defaultValue: 0, setValue: 1000, signalName: "delayChanged"},
            {tag: "timeout", property: "timeout", defaultValue: -1, setValue: 2000, signalName: "timeoutChanged"}
        ]
    }

    function test_properties(data) {
        var control = createTemporaryObject(toolTip, testCase)
        verify(control)

        compare(control[data.property], data.defaultValue)

        var spy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: data.signalName})
        verify(spy.valid)

        control[data.property] = data.setValue
        compare(control[data.property], data.setValue)
        compare(spy.count, 1)
    }

    function test_attached_data() {
        return [
            {tag: "text", property: "text", defaultValue: "", setValue: "Hello", signalName: "textChanged"},
            {tag: "delay", property: "delay", defaultValue: 0, setValue: 1000, signalName: "delayChanged"},
            {tag: "timeout", property: "timeout", defaultValue: -1, setValue: 2000, signalName: "timeoutChanged"}
        ]
    }

    function test_attached(data) {
        var item1 = createTemporaryObject(mouseArea, testCase)
        verify(item1)

        var item2 = createTemporaryObject(mouseArea, testCase)
        verify(item2)

        // Reset the properties to the expected default values, in case
        // we're not the first test that uses attached properties to be run.
        var sharedTip = ToolTip.toolTip
        sharedTip[data.property] = data.defaultValue

        compare(item1.ToolTip[data.property], data.defaultValue)
        compare(item2.ToolTip[data.property], data.defaultValue)

        var spy1 = signalSpy.createObject(item1, {target: item1.ToolTip, signalName: data.signalName})
        verify(spy1.valid)

        var spy2 = signalSpy.createObject(item2, {target: item2.ToolTip, signalName: data.signalName})
        verify(spy2.valid)

        sharedSpy.signalName = data.signalName
        verify(sharedSpy.valid)
        sharedSpy.clear()

        // change attached properties while the shared tooltip is not visible
        item1.ToolTip[data.property] = data.setValue
        compare(item1.ToolTip[data.property], data.setValue)
        compare(spy1.count, 1)

        compare(spy2.count, 0)
        compare(item2.ToolTip[data.property], data.defaultValue)

        // the shared tooltip is not visible for item1, so the attached
        // property change should therefore not apply to the shared instance
        compare(sharedSpy.count, 0)
        compare(sharedTip[data.property], data.defaultValue)

        // show the shared tooltip for item2
        item2.ToolTip.visible = true
        verify(item2.ToolTip.visible)
        verify(sharedTip.visible)

        // change attached properties while the shared tooltip is visible
        item2.ToolTip[data.property] = data.setValue
        compare(item2.ToolTip[data.property], data.setValue)
        compare(spy2.count, 1)

        // the shared tooltip is visible for item2, so the attached
        // property change should apply to the shared instance
        compare(sharedSpy.count, 1)
        compare(sharedTip[data.property], data.setValue)
    }

    function test_delay_data() {
        return [
            {tag: "imperative:0", delay: 0, imperative: true},
            {tag: "imperative:100", delay: 100, imperative: true},
            {tag: "declarative:0", delay: 0, imperative: false},
            {tag: "declarative:100", delay: 100, imperative: false}
        ]
    }

    function test_delay(data) {
        var control = createTemporaryObject(toolTip, testCase, {delay: data.delay})

        compare(control.visible, false)
        if (data.imperative)
            control.open()
        else
            control.visible = true
        compare(control.visible, data.delay <= 0)
        tryCompare(control, "visible", true)
    }

    function test_timeout_data() {
        return [
            {tag: "imperative", imperative: true},
            {tag: "declarative", imperative: false}
        ]
    }

    function test_timeout(data) {
        var control = createTemporaryObject(toolTip, testCase, {timeout: 100})

        compare(control.visible, false)
        if (data.imperative)
            control.open()
        else
            control.visible = true
        compare(control.visible, true)
        tryCompare(control, "visible", false)
    }

    function test_warning() {
        ignoreWarning(Qt.resolvedUrl("tst_tooltip.qml") + ":78:5: QML QtObject: ToolTip must be attached to an Item")
        ignoreWarning("<Unknown File>:1:30: QML ToolTip: cannot find any window to open popup in.")
        object.ToolTip.show("") // don't crash (QTBUG-56243)
    }

    Component {
        id: toolTipWithExitTransition

        ToolTip {
            enter: Transition {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 }
            }
            exit: Transition {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 1000 }
            }
        }
    }

    function test_makeVisibleWhileExitTransitionRunning_data() {
        return [
            { tag: "imperative", imperative: true },
            { tag: "declarative", imperative: false }
        ]
    }

    function test_makeVisibleWhileExitTransitionRunning(data) {
        var control = createTemporaryObject(toolTipWithExitTransition, testCase)

        // Show, hide, and show the tooltip again. Its exit transition should
        // start and get cancelled, and then its enter transition should run.
        if (data.imperative)
            control.open()
        else
            control.visible = true
        tryCompare(control, "opacity", 1)

        if (data.imperative)
            control.close()
        else
            control.visible = false
        verify(control.exit.running)
        tryVerify(function() { return control.opacity < 1; })

        if (data.imperative)
            control.open()
        else
            control.visible = true
        tryCompare(control, "opacity", 1)
    }

    Component {
        id: buttonAndShortcutComponent

        Item {
            property alias shortcut: shortcut
            property alias button: button

            Shortcut {
                id: shortcut
                sequence: "A"
            }

            Button {
                id: button
                text: "Just a button"
                focusPolicy: Qt.NoFocus

                ToolTip.visible: button.hovered
                ToolTip.text: qsTr("Some helpful text")
            }
        }
    }

    function test_activateShortcutWhileToolTipVisible() {
        if ((Qt.platform.pluginName === "offscreen")
            || (Qt.platform.pluginName === "minimal"))
            skip("Mouse hoovering not functional on offscreen/minimal platforms")

        var root = createTemporaryObject(buttonAndShortcutComponent, testCase)
        verify(root)

        mouseMove(root.button, root.button.width / 2, root.button.height / 2)
        tryCompare(root.button.ToolTip.toolTip, "visible", true)

        var shortcutActivatedSpy = signalSpy.createObject(root, { target: root.shortcut, signalName: "activated" })
        verify(shortcutActivatedSpy.valid)
        keyPress(Qt.Key_A)
        compare(shortcutActivatedSpy.count, 1)
    }

    Component {
        id: hoverComponent
        MouseArea {
            id: hoverArea
            property alias tooltip: tooltip
            hoverEnabled: true
            width: testCase.width
            height: testCase.height
            ToolTip {
                id: tooltip
                x: 10; y: 10
                width: 10; height: 10
                visible: hoverArea.containsMouse
            }
        }
    }

    // QTBUG-63644
    function test_hover() {
        var root = createTemporaryObject(hoverComponent, testCase)
        verify(root)

        var tooltip = root.tooltip
        verify(tooltip)

        for (var pos = 0; pos <= 25; pos += 5) {
            mouseMove(root, pos, pos)
            verify(tooltip.visible)
        }
    }

    Component {
        id: nonAttachedToolTipComponent
        ToolTip { }
    }

    function test_nonAttachedToolTipShowAndHide() {
        var tip = createTemporaryObject(nonAttachedToolTipComponent, testCase)
        verify(tip)
        tip.show("hello");
        verify(tip.visible)
        verify(tip.text === "hello")
        tip.hide()
        tryCompare(tip, "visible", false)
        tip.show("delay", 200)
        verify(tip.visible)
        tryCompare(tip, "visible", false)
    }

    Component {
        id: timeoutButtonRowComponent

        Row {
            Button {
                text: "Timeout: 1"
                ToolTip.text: text
                ToolTip.visible: down
                ToolTip.timeout: 1
            }

            Button {
                text: "Timeout: -1"
                ToolTip.text: text
                ToolTip.visible: down
            }
        }
    }

    // QTBUG-74226
    function test_attachedTimeout() {
        var row = createTemporaryObject(timeoutButtonRowComponent, testCase)
        verify(row)

        // Press the button that has no timeout; it should stay visible.
        var button2 = row.children[1]
        mousePress(button2)
        compare(button2.down, true)
        tryCompare(button2.ToolTip.toolTip, "opened", true)

        // Wait a bit to make sure that it's still visible.
        wait(50)
        compare(button2.ToolTip.toolTip.opened, true)

        // Release and should close.
        mouseRelease(button2)
        compare(button2.down, false)
        tryCompare(button2.ToolTip, "visible", false)

        // Now, press the first button that does have a timeout; it should close on its own eventually.
        var button1 = row.children[0]
        mousePress(button1)
        compare(button1.down, true)
        // We use a short timeout to speed up the test, but tryCompare(...opened, true) then
        // fails because the dialog has already been hidden by that point, so just check that it's
        // immediately visible, which is more or less the same thing.
        compare(button1.ToolTip.visible, true)
        tryCompare(button1.ToolTip, "visible", false)
        mouseRelease(button2)

        // Now, hover over the second button again. It should still stay visible until the mouse is released.
        mousePress(button2)
        compare(button2.down, true)
        tryCompare(button2.ToolTip.toolTip, "opened", true)

        // Wait a bit to make sure that it's still visible.
        wait(50)
        compare(button2.ToolTip.toolTip.opened, true)

        // Release and should close.
        mouseRelease(button2)
        compare(button2.down, false)
        tryCompare(button2.ToolTip, "visible", false)
    }
}
