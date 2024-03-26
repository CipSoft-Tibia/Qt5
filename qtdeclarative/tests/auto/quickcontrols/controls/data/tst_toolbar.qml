// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ToolBar"

    Component {
        id: toolBar
        ToolBar { }
    }

    Component {
        id: oneChildBar
        ToolBar {
            Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    Component {
        id: twoChildrenBar
        ToolBar {
            Item {
                implicitWidth: 100
                implicitHeight: 30
            }
            Item {
                implicitWidth: 200
                implicitHeight: 60
            }
        }
    }

    Component {
        id: contentBar
        ToolBar {
            contentItem: Item {
                implicitWidth: 100
                implicitHeight: 30
            }
        }
    }

    function test_empty() {
        failOnWarning(/.?/)

        var control = createTemporaryObject(toolBar, testCase)
        verify(control)

        verify(control.contentItem)
        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
    }

    function test_oneChild() {
        var control = createTemporaryObject(oneChildBar, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth >= 100)
        verify(control.implicitHeight >= 30)
    }

    function test_twoChildren() {
        var control = createTemporaryObject(twoChildrenBar, testCase)
        verify(control)

        compare(control.contentWidth, 0)
        compare(control.contentHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
        verify(control.implicitWidth >= 0)
        verify(control.implicitHeight >= 0)
    }

    function test_contentItem() {
        var control = createTemporaryObject(contentBar, testCase)
        verify(control)

        compare(control.contentWidth, 100)
        compare(control.contentHeight, 30)
        compare(control.implicitContentWidth, 100)
        compare(control.implicitContentHeight, 30)
        verify(control.implicitWidth >= 100)
        verify(control.implicitHeight >= 30)
    }
}
