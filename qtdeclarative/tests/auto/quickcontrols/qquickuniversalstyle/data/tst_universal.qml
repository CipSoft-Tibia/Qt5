// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Controls
import QtQuick.Controls.Universal

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Universal"

    Component {
        id: button
        Button { }
    }

    Component {
        id: styledButton
        Button {
            Universal.theme: Universal.Dark
            Universal.accent: Universal.Violet
            Universal.foreground: Universal.Brown
            Universal.background: Universal.Yellow
        }
    }

    Component {
        id: window
        Window { }
    }

    Component {
        id: styledWindow
        Window {
            Universal.theme: Universal.Dark
            Universal.accent: Universal.Green
        }
    }

    Component {
        id: loader
        Loader {
            active: false
            sourceComponent: Button { }
        }
    }

    Component {
        id: swipeView
        SwipeView {
            Universal.theme: Universal.Dark
            Button { }
        }
    }

    Component {
        id: menu
        ApplicationWindow {
            Universal.accent: Universal.Red
            property alias menu: popup
            Menu {
                id: popup
                Universal.theme: Universal.Dark
                MenuItem { }
            }
        }
    }

    Component {
        id: comboBox
        ApplicationWindow {
            width: 200
            height: 200
            visible: true
            Universal.accent: Universal.Red
            property alias combo: box
            ComboBox {
                id: box
                Universal.theme: Universal.Dark
                model: 1
            }
        }
    }

    Component {
        id: windowPane
        ApplicationWindow {
            width: 200
            height: 200
            visible: true
            property alias pane: pane
            Pane { id: pane }
        }
    }

    function test_defaults() {
        var control = button.createObject(testCase)
        verify(control)
        verify(control.Universal)
        compare(control.Universal.accent, "#3e65ff") // Universal.Cobalt
        compare(control.Universal.foreground, "#000000") // SystemBaseHighColor
        compare(control.Universal.background, "#ffffff") // SystemAltHighColor
        compare(control.Universal.theme, Universal.Light)
        control.destroy()
    }

    function test_set() {
        var control = button.createObject(testCase)
        verify(control)
        control.Universal.accent = Universal.Steel
        control.Universal.foreground = Universal.Red
        control.Universal.background = Universal.Green
        control.Universal.theme = Universal.Dark
        compare(control.Universal.accent, "#647687") // Universal.Steel
        compare(control.Universal.foreground, "#e51400") // Universal.Red
        compare(control.Universal.background, "#60a917") // Universal.Green
        compare(control.Universal.theme, Universal.Dark)
        control.destroy()
    }

    function test_reset() {
        var control = styledButton.createObject(testCase)
        verify(control)
        compare(control.Universal.accent, "#aa00ff") // Universal.Violet
        compare(control.Universal.foreground, "#825a2c") // Universal.Brown
        compare(control.Universal.background, "#e3c800") // Universal.Yellow
        compare(control.Universal.theme, Universal.Dark)
        control.Universal.accent = undefined
        control.Universal.foreground = undefined
        control.Universal.background = undefined
        control.Universal.theme = undefined
        compare(control.Universal.accent, testCase.Universal.accent)
        compare(control.Universal.foreground, testCase.Universal.foreground)
        compare(control.Universal.background, testCase.Universal.background)
        compare(control.Universal.theme, testCase.Universal.theme)
        control.destroy()
    }

    function test_inheritance_data() {
        return [
            { tag: "accent", value1: "#a20025" /*Universal.Crimson*/, value2: "#6a00ff" /*Universal.Indigo*/ },
            { tag: "foreground", value1: "#a20025" /*Universal.Crimson*/, value2: "#6a00ff" /*Universal.Indigo*/ },
            { tag: "background", value1: "#a20025" /*Universal.Crimson*/, value2: "#6a00ff" /*Universal.Indigo*/ },
            { tag: "theme", value1: Universal.Dark, value2: Universal.Light },
        ]
    }

    function test_inheritance(data) {
        var prop = data.tag
        var parent = button.createObject(testCase)
        parent.Universal[prop] = data.value1
        compare(parent.Universal[prop], data.value1)

        var child1 = button.createObject(parent)
        compare(child1.Universal[prop], data.value1)

        parent.Universal[prop] = data.value2
        compare(parent.Universal[prop], data.value2)
        compare(child1.Universal[prop], data.value2)

        var child2 = button.createObject(parent)
        compare(child2.Universal[prop], data.value2)

        child2.Universal[prop] = data.value1
        compare(child2.Universal[prop], data.value1)
        compare(child1.Universal[prop], data.value2)
        compare(parent.Universal[prop], data.value2)

        parent.Universal[prop] = undefined
        verify(parent.Universal[prop] !== data.value1)
        verify(parent.Universal[prop] !== undefined)
        compare(child1.Universal[prop], parent.Universal[prop])
        verify(child2.Universal[prop] !== parent.Universal[prop])

        var grandChild1 = button.createObject(child1)
        var grandChild2 = button.createObject(child2)
        compare(grandChild1.Universal[prop], child1.Universal[prop])
        compare(grandChild2.Universal[prop], child2.Universal[prop])

        var themelessGrandGrandChild = button.createObject(grandChild1)
        var grandGrandGrandChild1 = button.createObject(themelessGrandGrandChild)
        compare(grandGrandGrandChild1.Universal[prop], parent.Universal[prop])

        child1.Universal[prop] = data.value2
        compare(child1.Universal[prop], data.value2)
        compare(grandChild1.Universal[prop], data.value2)
        compare(grandGrandGrandChild1.Universal[prop], data.value2)

        parent.destroy()
    }

    function test_window() {
        var parent = window.createObject()

        var control = button.createObject(parent.contentItem)
        compare(control.Universal.accent, parent.Universal.accent)
        compare(control.Universal.theme, parent.Universal.theme)

        var styledChild = styledWindow.createObject(window)
        verify(styledChild.Universal.accent !== parent.Universal.accent)
        verify(styledChild.Universal.theme !== parent.Universal.theme)

        var unstyledChild = window.createObject(window)
        compare(unstyledChild.Universal.accent, parent.Universal.accent)
        compare(unstyledChild.Universal.theme, parent.Universal.theme)

        parent.Universal.accent = Universal.Cyan
        compare(control.Universal.accent, "#1ba1e2") // Universal.Cyan
        verify(styledChild.Universal.accent !== Universal.Cyan)
        // ### TODO: compare(unstyledChild.Universal.accent, Universal.Cyan)

        parent.destroy()
    }

    function test_loader() {
        var control = loader.createObject(testCase)
        control.Universal.accent = Universal.Lime
        control.active = true
        compare(control.item.Universal.accent, "#a4c400") // Universal.Lime
        control.Universal.accent = Universal.Pink
        compare(control.item.Universal.accent, "#f472d0") // Universal.Pink
        control.active = false
        control.Universal.accent = Universal.Brown
        control.active = true
        compare(control.item.Universal.accent, "#825a2c") // Universal.Brown
        control.destroy()
    }

    function test_swipeView() {
        var control = swipeView.createObject(testCase)
        verify(control)
        var child = control.itemAt(0)
        verify(child)
        compare(control.Universal.theme, Universal.Dark)
        compare(child.Universal.theme, Universal.Dark)
        control.destroy()
    }

    function test_menu() {
        var container = menu.createObject(testCase)
        verify(container)
        verify(container.menu)
        container.menu.open()
        verify(container.menu.visible)
        var child = container.menu.itemAt(0)
        verify(child)
        compare(container.Universal.theme, Universal.Light)
        compare(container.menu.Universal.theme, Universal.Dark)
        compare(child.Universal.theme, Universal.Dark)
        compare(container.Universal.accent, "#e51400") // Red
        compare(container.menu.Universal.accent, "#e51400") // Red
        compare(child.Universal.accent, "#e51400") // Red
        container.destroy()
    }

    function test_comboBox() {
        var window = comboBox.createObject(testCase)
        verify(window)
        verify(window.combo)
        waitForRendering(window.combo)
        window.combo.forceActiveFocus()
        verify(window.combo.activeFocus)
        keyClick(Qt.Key_Space)
        verify(window.combo.popup.visible)
        var listView = window.combo.popup.contentItem
        verify(listView)
        var child = listView.contentItem.children[0]
        verify(child)
        compare(window.Universal.theme, Universal.Light)
        compare(window.combo.Universal.theme, Universal.Dark)
        compare(child.Universal.theme, Universal.Dark)
        compare(window.Universal.accent, "#e51400") // Red
        compare(window.combo.Universal.accent, "#e51400") // Red
        compare(child.Universal.accent, "#e51400") // Red
        window.destroy()
    }

    function test_windowChange() {
        var ldr = loader.createObject()
        verify(ldr)

        var wnd = window.createObject()
        verify(wnd)

        wnd.Universal.theme = Universal.Dark
        compare(wnd.Universal.theme, Universal.Dark)

        ldr.active = true
        verify(ldr.item)
        compare(ldr.item.Universal.theme, Universal.Light)

        ldr.parent = wnd.contentItem
        compare(ldr.item.Universal.theme, Universal.Dark)

        wnd.destroy()
    }

    function test_colors_data() {
        return [
            { tag: "accent" }, { tag: "background" }, { tag: "foreground" }
        ]
    }

    function test_colors(data) {
        var control = button.createObject(testCase)
        verify(control)

        var prop = data.tag

        // Universal.Color - enum
        control.Universal[prop] = Universal.Red
        compare(control.Universal[prop], "#e51400")

        // Universal.Color - string
        control.Universal[prop] = "Emerald"
        compare(control.Universal[prop], "#008a00")

        // SVG named color
        control.Universal[prop] = "tomato"
        compare(control.Universal[prop], "#ff6347")

        // #rrggbb
        control.Universal[prop] = "#123456"
        compare(control.Universal[prop], "#123456")

        // #aarrggbb
        control.Universal[prop] = "#12345678"
        compare(control.Universal[prop], "#12345678")

        // Qt.rgba() - no alpha
        control.Universal[prop] = Qt.rgba(0.5, 0.5, 0.5)
        compare(control.Universal[prop], "#808080")

        // Qt.rgba() - with alpha
        control.Universal[prop] = Qt.rgba(0.5, 0.5, 0.5, 0.5)
        compare(control.Universal[prop], "#80808080")

        // unknown
        ignoreWarning(Qt.resolvedUrl("tst_universal.qml") + ":20:9: QML Button: unknown Universal." + prop + " value: 123")
        control.Universal[prop] = 123
        ignoreWarning(Qt.resolvedUrl("tst_universal.qml") + ":20:9: QML Button: unknown Universal." + prop + " value: foo")
        control.Universal[prop] = "foo"
        ignoreWarning(Qt.resolvedUrl("tst_universal.qml") + ":20:9: QML Button: unknown Universal." + prop + " value: #1")
        control.Universal[prop] = "#1"

        control.destroy()
    }

    function test_font_data() {
        return [
            {tag: "Control:pixelSize", type: "Control", attribute: "pixelSize", value: 15, window: 20, pane: 10},

            {tag: "GroupBox:pixelSize", type: "GroupBox", attribute: "pixelSize", value: 15, window: 20, pane: 10},
            {tag: "GroupBox:weight", type: "GroupBox", attribute: "weight", value: Font.DemiBold, window: Font.Light, pane: Font.Medium},

            {tag: "TabButton:pixelSize", type: "TabButton", attribute: "pixelSize", value: 24, window: 20, pane: 10},
            {tag: "TabButton:weight", type: "TabButton", attribute: "weight", value: Font.Light, window: Font.Black, pane: Font.Bold}
        ]
    }

    function test_font(data) {
        var window = windowPane.createObject(testCase)
        verify(window)
        verify(window.pane)

        var control = Qt.createQmlObject("import QtQuick.Controls; " + data.type + " { }", window.pane)
        verify(control)

        compare(control.font[data.attribute], data.value)

        window.font[data.attribute] = data.window
        compare(window.font[data.attribute], data.window)
        compare(window.pane.font[data.attribute], data.window)
        compare(control.font[data.attribute], data.window)

        window.pane.font[data.attribute] = data.pane
        compare(window.font[data.attribute], data.window)
        compare(window.pane.font[data.attribute], data.pane)
        compare(control.font[data.attribute], data.pane)

        window.pane.font = undefined
        compare(window.font[data.attribute], data.window)
        compare(window.pane.font[data.attribute], data.window)
        compare(control.font[data.attribute], data.window)

        window.destroy()
    }
}
