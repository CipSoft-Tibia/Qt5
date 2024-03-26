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
    name: "Button"

    Component {
        id: button
        Button { }
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "doubleClicked", "pressedChanged", "downChanged", "checkedChanged"]
        }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        compare(control.highlighted, false)
        compare(control.flat, false)
    }

    function test_text() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        compare(control.text, "")
        control.text = "Button"
        compare(control.text, "Button")
        control.text = ""
        compare(control.text, "")
    }

    function test_mouse() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }]]
        mouseMove(control, control.width * 2, control.height * 2, 0)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["canceled", { "pressed": false }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // right button
        sequenceSpy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // double click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked",
                                        ["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        "doubleClicked",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released"]
        mouseDoubleClickSequence(control, control.width / 2, control.height / 2, Qt.LeftButton)
        verify(sequenceSpy.success)
    }

    function test_touch() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        let touch = touchEvent(control)
        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }]]
        touch.move(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["canceled", { "pressed": false }]]
        touch.release(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_multiTouch() {
        let control1 = createTemporaryObject(button, testCase)
        verify(control1)

        let pressedCount1 = 0

        let pressedSpy1 = signalSpy.createObject(control1, {target: control1, signalName: "pressedChanged"})
        verify(pressedSpy1.valid)

        let touch = touchEvent(control1)
        touch.press(0, control1, 0, 0).commit().move(0, control1, control1.width - 1, control1.height - 1).commit()

        compare(pressedSpy1.count, ++pressedCount1)
        compare(control1.pressed, true)

        // second touch point on the same control is ignored
        touch.stationary(0).press(1, control1, 0, 0).commit()
        touch.stationary(0).move(1, control1).commit()
        touch.stationary(0).release(1).commit()

        compare(pressedSpy1.count, pressedCount1)
        compare(control1.pressed, true)

        let control2 = createTemporaryObject(button, testCase, {y: control1.height})
        verify(control2)

        let pressedCount2 = 0

        let pressedSpy2 = signalSpy.createObject(control2, {target: control2, signalName: "pressedChanged"})
        verify(pressedSpy2.valid)

        // press the second button
        touch.stationary(0).press(2, control2, 0, 0).commit()

        compare(pressedSpy2.count, ++pressedCount2)
        compare(control2.pressed, true)

        compare(pressedSpy1.count, pressedCount1)
        compare(control1.pressed, true)

        // release both buttons
        touch.release(0, control1).release(2, control2).commit()

        compare(pressedSpy2.count, ++pressedCount2)
        compare(control2.pressed, false)

        compare(pressedSpy1.count, ++pressedCount1)
        compare(control1.pressed, false)
    }

    function test_keys() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        verify(sequenceSpy.success)

        // no change
        sequenceSpy.expectedSequence = []
        // Not testing Key_Enter and Key_Return because QGnomeTheme uses them for
        // pressing buttons and the CI uses the QGnomeTheme platform theme.
        let keys = [Qt.Key_Escape, Qt.Key_Tab]
        for (let i = 0; i < keys.length; ++i) {
            sequenceSpy.reset()
            keyClick(keys[i])
            verify(sequenceSpy.success)
        }
    }

    function eventErrorMessage(actual, expected) {
        return "actual event:" + JSON.stringify(actual) + ", expected event:" + JSON.stringify(expected)
    }

    function test_autoRepeat() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        compare(control.autoRepeat, false)
        control.autoRepeat = true
        compare(control.autoRepeat, true)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let clickSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(clickSpy.valid)
        let pressSpy = signalSpy.createObject(control, {target: control, signalName: "pressed"})
        verify(pressSpy.valid)
        let releaseSpy = signalSpy.createObject(control, {target: control, signalName: "released"})
        verify(releaseSpy.valid)

        // auto-repeat mouse click
        mousePress(control)
        compare(control.pressed, true)
        clickSpy.wait()
        clickSpy.wait()
        compare(pressSpy.count, clickSpy.count + 1)
        compare(releaseSpy.count, clickSpy.count)
        mouseRelease(control)
        compare(control.pressed, false)
        compare(clickSpy.count, pressSpy.count)
        compare(releaseSpy.count, pressSpy.count)

        clickSpy.clear()
        pressSpy.clear()
        releaseSpy.clear()

        // auto-repeat key click
        keyPress(Qt.Key_Space)
        compare(control.pressed, true)
        clickSpy.wait()
        clickSpy.wait()
        compare(pressSpy.count, clickSpy.count + 1)
        compare(releaseSpy.count, clickSpy.count)
        keyRelease(Qt.Key_Space)
        compare(control.pressed, false)
        compare(clickSpy.count, pressSpy.count)
        compare(releaseSpy.count, pressSpy.count)

        clickSpy.clear()
        pressSpy.clear()
        releaseSpy.clear()

        mousePress(control)
        compare(control.pressed, true)
        clickSpy.wait()
        compare(pressSpy.count, clickSpy.count + 1)
        compare(releaseSpy.count, clickSpy.count)

        // move inside during repeat -> continue repeat
        mouseMove(control, control.width / 4, control.height / 4)
        clickSpy.wait()
        compare(pressSpy.count, clickSpy.count + 1)
        compare(releaseSpy.count, clickSpy.count)

        clickSpy.clear()
        pressSpy.clear()
        releaseSpy.clear()

        // move outside during repeat -> stop repeat
        mouseMove(control, -1, -1)
        // NOTE: The following wait() is NOT a reliable way to test that the
        // auto-repeat timer is not running, but there's no way dig into the
        // private APIs from QML. If this test ever fails in the future, it
        // indicates that the auto-repeat timer logic is broken.
        wait(125)
        compare(clickSpy.count, 0)
        compare(pressSpy.count, 0)
        compare(releaseSpy.count, 0)

        mouseRelease(control, -1, -1)
        compare(control.pressed, false)
        compare(clickSpy.count, 0)
        compare(pressSpy.count, 0)
        compare(releaseSpy.count, 0)
    }

    function test_baseline() {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_checkable() {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        verify(control.hasOwnProperty("checkable"))
        verify(!control.checkable)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        verify(!control.checked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": true }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        control.checkable = true
        mouseClick(control)
        verify(control.checked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": false }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        mouseClick(control)
        verify(!control.checked)
        verify(sequenceSpy.success)
    }

    function test_highlighted() {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        verify(!control.highlighted)

        control.highlighted = true
        verify(control.highlighted)
    }

    function test_spacing() {
        let control = createTemporaryObject(button, testCase, { text: "Some long, long, long text" })
        verify(control)
        verify(control.contentItem.implicitWidth + control.leftPadding + control.rightPadding > control.background.implicitWidth)

        let textLabel = findChild(control.contentItem, "label")
        verify(textLabel)

        // The implicitWidth of the IconLabel that all buttons use as their contentItem
        // should be equal to the implicitWidth of the Text while no icon is set.
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth)

        // That means that spacing shouldn't affect it.
        control.spacing += 100
        compare(control.contentItem.implicitWidth, textLabel.implicitWidth)

        // The implicitWidth of the Button itself should, therefore, also never include spacing while no icon is set.
        compare(control.implicitWidth, textLabel.implicitWidth + control.leftPadding + control.rightPadding)
    }

    function test_display_data() {
        return [
            { "tag": "IconOnly", display: Button.IconOnly },
            { "tag": "TextOnly", display: Button.TextOnly },
            { "tag": "TextUnderIcon", display: Button.TextUnderIcon },
            { "tag": "TextBesideIcon", display: Button.TextBesideIcon },
            { "tag": "IconOnly, mirrored", display: Button.IconOnly, mirrored: true },
            { "tag": "TextOnly, mirrored", display: Button.TextOnly, mirrored: true },
            { "tag": "TextUnderIcon, mirrored", display: Button.TextUnderIcon, mirrored: true },
            { "tag": "TextBesideIcon, mirrored", display: Button.TextBesideIcon, mirrored: true }
        ]
    }

    function test_display(data) {
        let control = createTemporaryObject(button, testCase, {
            text: "Button",
            display: data.display,
            "icon.source": "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png",
            "LayoutMirroring.enabled": !!data.mirrored
        })
        verify(control)
        compare(control.icon.source, "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png")

        let iconImage = findChild(control.contentItem, "image")
        let textLabel = findChild(control.contentItem, "label")

        switch (control.display) {
        case Button.IconOnly:
            verify(iconImage)
            verify(!textLabel)
            compare(iconImage.x, (control.availableWidth - iconImage.width) / 2)
            compare(iconImage.y, (control.availableHeight - iconImage.height) / 2)
            break;
        case Button.TextOnly:
            verify(!iconImage)
            verify(textLabel)
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        case Button.TextUnderIcon:
            verify(iconImage)
            verify(textLabel)
            compare(iconImage.x, (control.availableWidth - iconImage.width) / 2)
            compare(textLabel.x, (control.availableWidth - textLabel.width) / 2)
            verify(iconImage.y < textLabel.y)
            break;
        case Button.TextBesideIcon:
            verify(iconImage)
            verify(textLabel)
            if (control.mirrored)
                verify(textLabel.x < iconImage.x)
            else
                verify(iconImage.x < textLabel.x)
            compare(iconImage.y, (control.availableHeight - iconImage.height) / 2)
            compare(textLabel.y, (control.availableHeight - textLabel.height) / 2)
            break;
        }
    }
}
