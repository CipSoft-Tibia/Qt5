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
import QtQuick.Templates 2.12 as T

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "Control"

    Component {
        id: component
        Control { }
    }

    Component {
        id: rectangle
        Rectangle { }
    }

    Component {
        id: button
        T.Button { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function test_padding() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        var paddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "paddingChanged"})
        verify(paddingSpy.valid)

        var topPaddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "topPaddingChanged"})
        verify(topPaddingSpy.valid)

        var leftPaddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "leftPaddingChanged"})
        verify(leftPaddingSpy.valid)

        var rightPaddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "rightPaddingChanged"})
        verify(rightPaddingSpy.valid)

        var bottomPaddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "bottomPaddingChanged"})
        verify(bottomPaddingSpy.valid)

        var horizontalPaddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "horizontalPaddingChanged"})
        verify(horizontalPaddingSpy.valid)

        var verticalPaddingSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "verticalPaddingChanged"})
        verify(verticalPaddingSpy.valid)

        var paddingChanges = 0
        var topPaddingChanges = 0
        var leftPaddingChanges = 0
        var rightPaddingChanges = 0
        var bottomPaddingChanges = 0
        var horizontalPaddingChanges = 0
        var verticalPaddingChanges = 0

        compare(control.padding, 0)
        compare(control.topPadding, 0)
        compare(control.leftPadding, 0)
        compare(control.rightPadding, 0)
        compare(control.bottomPadding, 0)
        compare(control.horizontalPadding, 0)
        compare(control.verticalPadding, 0)
        compare(control.availableWidth, 0)
        compare(control.availableHeight, 0)

        control.width = 100
        control.height = 100

        control.padding = 10
        compare(control.padding, 10)
        compare(control.topPadding, 10)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)
        compare(control.horizontalPadding, 10)
        compare(control.verticalPadding, 10)
        compare(paddingSpy.count, ++paddingChanges)
        compare(topPaddingSpy.count, ++topPaddingChanges)
        compare(leftPaddingSpy.count, ++leftPaddingChanges)
        compare(rightPaddingSpy.count, ++rightPaddingChanges)
        compare(bottomPaddingSpy.count, ++bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, ++horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, ++verticalPaddingChanges)

        control.topPadding = 20
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)
        compare(control.horizontalPadding, 10)
        compare(control.verticalPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, ++topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.leftPadding = 30
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)
        compare(control.horizontalPadding, 10)
        compare(control.verticalPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, ++leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.rightPadding = 40
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 10)
        compare(control.horizontalPadding, 10)
        compare(control.verticalPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, ++rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.bottomPadding = 50
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 10)
        compare(control.verticalPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, ++bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.padding = 60
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 60)
        compare(control.verticalPadding, 60)
        compare(paddingSpy.count, ++paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, ++horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, ++verticalPaddingChanges)

        control.horizontalPadding = 80
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 80)
        compare(control.verticalPadding, 60)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, ++horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.verticalPadding = 90
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 80)
        compare(control.verticalPadding, 90)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, ++verticalPaddingChanges)

        control.leftPadding = undefined
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 80)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 80)
        compare(control.verticalPadding, 90)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, ++leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.rightPadding = undefined
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 80)
        compare(control.rightPadding, 80)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 80)
        compare(control.verticalPadding, 90)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, ++rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.topPadding = undefined
        compare(control.padding, 60)
        compare(control.topPadding, 90)
        compare(control.leftPadding, 80)
        compare(control.rightPadding, 80)
        compare(control.bottomPadding, 50)
        compare(control.horizontalPadding, 80)
        compare(control.verticalPadding, 90)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, ++topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.bottomPadding = undefined
        compare(control.padding, 60)
        compare(control.topPadding, 90)
        compare(control.leftPadding, 80)
        compare(control.rightPadding, 80)
        compare(control.bottomPadding, 90)
        compare(control.horizontalPadding, 80)
        compare(control.verticalPadding, 90)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, ++bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.horizontalPadding = undefined
        compare(control.padding, 60)
        compare(control.topPadding, 90)
        compare(control.leftPadding, 60)
        compare(control.rightPadding, 60)
        compare(control.bottomPadding, 90)
        compare(control.horizontalPadding, 60)
        compare(control.verticalPadding, 90)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, ++leftPaddingChanges)
        compare(rightPaddingSpy.count, ++rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, ++horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, verticalPaddingChanges)

        control.verticalPadding = undefined
        compare(control.padding, 60)
        compare(control.topPadding, 60)
        compare(control.leftPadding, 60)
        compare(control.rightPadding, 60)
        compare(control.bottomPadding, 60)
        compare(control.horizontalPadding, 60)
        compare(control.verticalPadding, 60)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, ++topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, ++bottomPaddingChanges)
        compare(horizontalPaddingSpy.count, horizontalPaddingChanges)
        compare(verticalPaddingSpy.count, ++verticalPaddingChanges)
    }

    function test_availableSize() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        var availableWidthSpy = signalSpy.createObject(control, {target: control, signalName: "availableWidthChanged"})
        verify(availableWidthSpy.valid)

        var availableHeightSpy = signalSpy.createObject(control, {target: control, signalName: "availableHeightChanged"})
        verify(availableHeightSpy.valid)

        var availableWidthChanges = 0
        var availableHeightChanges = 0

        control.width = 100
        compare(control.availableWidth, 100)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.height = 100
        compare(control.availableHeight, 100)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.padding = 10
        compare(control.availableWidth, 80)
        compare(control.availableHeight, 80)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.topPadding = 20
        compare(control.availableWidth, 80)
        compare(control.availableHeight, 70)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.leftPadding = 30
        compare(control.availableWidth, 60)
        compare(control.availableHeight, 70)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.rightPadding = 40
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 70)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.bottomPadding = 50
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 30)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.padding = 60
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 30)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.width = 0
        compare(control.availableWidth, 0)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.height = 0
        compare(control.availableHeight, 0)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)
    }

    function test_mirrored() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        var mirroredSpy = signalSpy.createObject(control, {target: control, signalName: "mirroredChanged"})
        verify(mirroredSpy.valid)

        control.locale = Qt.locale("en_US")
        compare(control.locale.name, "en_US")
        verify(!control.LayoutMirroring.enabled)
        compare(control.mirrored, false)

        control.locale = Qt.locale("ar_EG")
        compare(control.mirrored, true)
        compare(mirroredSpy.count, 1)

        control.LayoutMirroring.enabled = true
        compare(control.mirrored, true)
        compare(mirroredSpy.count, 1)

        control.locale = Qt.locale("en_US")
        compare(control.mirrored, true)
        compare(mirroredSpy.count, 1)

        control.LayoutMirroring.enabled = false
        compare(control.mirrored, false)
        compare(mirroredSpy.count, 2)
    }

    function test_background() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        control.background = component.createObject(control)

        // background has no x or width set, so its width follows control's width
        control.width = 320
        compare(control.background.width, control.width)

        // background has no y or height set, so its height follows control's height
        compare(control.background.height, control.height)
        control.height = 240

        // change implicit size (QTBUG-66455)
        control.background.implicitWidth = 160
        control.background.implicitHeight = 120
        compare(control.background.width, control.width)
        compare(control.background.height, control.height)

        // has width => width does not follow
        control.background.width /= 2
        control.width += 20
        verify(control.background.width !== control.width)

        // reset width => width follows again
        control.background.width = undefined
        control.width += 20
        compare(control.background.width, control.width)

        // has x => width does not follow
        control.background.x = 10
        control.width += 20
        verify(control.background.width !== control.width)

        // has height => height does not follow
        control.background.height /= 2
        control.height -= 20
        verify(control.background.height !== control.height)

        // reset height => height follows again
        control.background.height = undefined
        control.height -= 20
        compare(control.background.height, control.height)

        // has y => height does not follow
        control.background.y = 10
        control.height -= 20
        verify(control.background.height !== control.height)
    }

    Component {
        id: component2
        T.Control {
            id: item2
            objectName: "item2"
            property alias item2_2: _item2_2;
            property alias item2_3: _item2_3;
            property alias item2_4: _item2_4;
            property alias item2_5: _item2_5;
            property alias item2_6: _item2_6;
            font.family: "Arial"
            T.Control {
                id: _item2_2
                objectName: "_item2_2"
                T.Control {
                    id: _item2_3
                    objectName: "_item2_3"
                }
            }
            T.TextArea {
                id: _item2_4
                objectName: "_item2_4"
                text: "Text Area"
            }
            T.TextField {
                id: _item2_5
                objectName: "_item2_5"
                text: "Text Field"
            }
            T.Label {
                id: _item2_6
                objectName: "_item2_6"
                text: "Label"
            }
        }
    }

    function test_font() {
        var control2 = createTemporaryObject(component2, testCase)
        verify(control2)
        verify(control2.item2_2)
        verify(control2.item2_3)
        verify(control2.item2_4)
        verify(control2.item2_5)
        verify(control2.item2_6)

        compare(control2.font.family, "Arial")
        compare(control2.item2_2.font.family, control2.font.family)
        compare(control2.item2_3.font.family, control2.font.family)
        compare(control2.item2_4.font.family, control2.font.family)
        compare(control2.item2_5.font.family, control2.font.family)
        compare(control2.item2_6.font.family, control2.font.family)

        control2.font.pointSize = 48
        compare(control2.item2_2.font.pointSize, 48)
        compare(control2.item2_3.font.pointSize, 48)
        compare(control2.item2_4.font.pointSize, 48)
        compare(control2.item2_5.font.pointSize, 48)

        control2.font.bold = true
        compare(control2.item2_2.font.weight, Font.Bold)
        compare(control2.item2_3.font.weight, Font.Bold)
        compare(control2.item2_4.font.weight, Font.Bold)
        compare(control2.item2_5.font.weight, Font.Bold)

        control2.item2_2.font.pointSize = 36
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_3.font.pointSize, 36)

        control2.item2_2.font.weight = Font.Light
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_3.font.pointSize, 36)

        compare(control2.item2_3.font.family, control2.item2_2.font.family)
        compare(control2.item2_3.font.pointSize, control2.item2_2.font.pointSize)
        compare(control2.item2_3.font.weight, control2.item2_2.font.weight)

        control2.font.pointSize = 50
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_3.font.pointSize, 36)
        compare(control2.item2_4.font.pointSize, 50)
        compare(control2.item2_5.font.pointSize, 50)
        compare(control2.item2_6.font.pointSize, 50)

        control2.item2_3.font.pointSize = 60
        compare(control2.item2_3.font.pointSize, 60)

        control2.item2_3.font.weight = Font.Normal
        compare(control2.item2_3.font.weight, Font.Normal)

        control2.item2_4.font.pointSize = 16
        compare(control2.item2_4.font.pointSize, 16)

        control2.item2_4.font.weight = Font.Normal
        compare(control2.item2_4.font.weight, Font.Normal)

        control2.item2_5.font.pointSize = 32
        compare(control2.item2_5.font.pointSize, 32)

        control2.item2_5.font.weight = Font.DemiBold
        compare(control2.item2_5.font.weight, Font.DemiBold)

        control2.item2_6.font.pointSize = 36
        compare(control2.item2_6.font.pointSize, 36)

        control2.item2_6.font.weight = Font.Black
        compare(control2.item2_6.font.weight, Font.Black)

        compare(control2.font.family, "Arial")
        compare(control2.font.pointSize, 50)
        compare(control2.font.weight, Font.Bold)

        compare(control2.item2_2.font.family, "Arial")
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_2.font.weight, Font.Light)

        compare(control2.item2_3.font.family, "Arial")
        compare(control2.item2_3.font.pointSize, 60)
        compare(control2.item2_3.font.weight, Font.Normal)

        compare(control2.item2_4.font.family, "Arial")
        compare(control2.item2_4.font.pointSize, 16)
        compare(control2.item2_4.font.weight, Font.Normal)

        compare(control2.item2_5.font.family, "Arial")
        compare(control2.item2_5.font.pointSize, 32)
        compare(control2.item2_5.font.weight, Font.DemiBold)

        compare(control2.item2_6.font.family, "Arial")
        compare(control2.item2_6.font.pointSize, 36)
        compare(control2.item2_6.font.weight, Font.Black)
    }

    Component {
        id: component3
        T.Control {
            id: item3
            objectName: "item3"
            property alias item3_2: _item3_2;
            property alias item3_3: _item3_3;
            property alias item3_4: _item3_4;
            property alias item3_5: _item3_5;
            property alias item3_6: _item3_6;
            property alias item3_7: _item3_7;
            property alias item3_8: _item3_8;
            font.family: "Arial"
            Item {
                id: _item3_2
                objectName: "_item3_2"
                T.Control {
                    id: _item3_3
                    objectName: "_item3_3"
                    Item {
                        id: _item3_6
                        objectName: "_item3_6"
                        T.Control {
                            id: _item3_7
                            objectName: "_item3_7"
                        }
                    }
                }
                T.TextArea {
                    id: _item3_4
                    objectName: "_item3_4"
                    text: "Text Area"
                }
                T.TextField {
                    id: _item3_5
                    objectName: "_item3_5"
                    text: "Text Field"
                }
                T.Label {
                    id: _item3_8
                    objectName: "_item3_8"
                    text: "Label"
                }
            }
        }
    }

    function test_font_2() {
        var control3 = createTemporaryObject(component3, testCase)
        verify(control3)
        verify(control3.item3_2)
        verify(control3.item3_3)
        verify(control3.item3_4)
        verify(control3.item3_5)
        verify(control3.item3_6)
        verify(control3.item3_7)
        verify(control3.item3_8)

        compare(control3.font.family, "Arial")
        compare(control3.item3_3.font.family, control3.font.family)
        compare(control3.item3_4.font.family, control3.font.family)
        compare(control3.item3_5.font.family, control3.font.family)
        compare(control3.item3_7.font.family, control3.font.family)
        compare(control3.item3_8.font.family, control3.font.family)

        control3.font.pointSize = 48
        compare(control3.item3_3.font.pointSize, 48)
        compare(control3.item3_4.font.pointSize, 48)
        compare(control3.item3_5.font.pointSize, 48)

        control3.font.bold = true
        compare(control3.item3_3.font.weight, Font.Bold)
        compare(control3.item3_4.font.weight, Font.Bold)
        compare(control3.item3_5.font.weight, Font.Bold)

        compare(control3.item3_3.font.family, control3.font.family)
        compare(control3.item3_3.font.pointSize, control3.font.pointSize)
        compare(control3.item3_3.font.weight, control3.font.weight)
        compare(control3.item3_7.font.family, control3.font.family)
        compare(control3.item3_7.font.pointSize, control3.font.pointSize)
        compare(control3.item3_7.font.weight, control3.font.weight)

        control3.item3_3.font.pointSize = 60
        compare(control3.item3_3.font.pointSize, 60)

        control3.item3_3.font.weight = Font.Normal
        compare(control3.item3_3.font.weight, Font.Normal)

        control3.item3_4.font.pointSize = 16
        compare(control3.item3_4.font.pointSize, 16)

        control3.item3_4.font.weight = Font.Normal
        compare(control3.item3_4.font.weight, Font.Normal)

        control3.item3_5.font.pointSize = 32
        compare(control3.item3_5.font.pointSize, 32)

        control3.item3_5.font.weight = Font.DemiBold
        compare(control3.item3_5.font.weight, Font.DemiBold)

        control3.item3_8.font.pointSize = 36
        compare(control3.item3_8.font.pointSize, 36)

        control3.item3_8.font.weight = Font.Black
        compare(control3.item3_8.font.weight, Font.Black)

        control3.font.pointSize = 100
        compare(control3.font.pointSize, 100)
        compare(control3.item3_3.font.pointSize, 60)
        compare(control3.item3_4.font.pointSize, 16)
        compare(control3.item3_5.font.pointSize, 32)
        compare(control3.item3_8.font.pointSize, 36)

        compare(control3.font.family, "Arial")
        compare(control3.font.pointSize, 100)
        compare(control3.font.weight, Font.Bold)

        compare(control3.item3_3.font.family, "Arial")
        compare(control3.item3_3.font.pointSize, 60)
        compare(control3.item3_3.font.weight, Font.Normal)
        compare(control3.item3_7.font.family, control3.item3_3.font.family)
        compare(control3.item3_7.font.pointSize, control3.item3_3.font.pointSize)
        compare(control3.item3_7.font.weight, control3.item3_3.font.weight)

        compare(control3.item3_4.font.family, "Arial")
        compare(control3.item3_4.font.pointSize, 16)
        compare(control3.item3_4.font.weight, Font.Normal)

        compare(control3.item3_5.font.family, "Arial")
        compare(control3.item3_5.font.pointSize, 32)
        compare(control3.item3_5.font.weight, Font.DemiBold)

        compare(control3.item3_8.font.family, "Arial")
        compare(control3.item3_8.font.pointSize, 36)
        compare(control3.item3_8.font.weight, Font.Black)
    }

    Component {
        id: component4
        T.Control {
            id: item4
            objectName: "item4"
            property alias item4_2: _item4_2;
            property alias item4_3: _item4_3;
            property alias item4_4: _item4_4;
            T.Control {
                id: _item4_2
                objectName: "_item4_2"
                font.pixelSize: item4.font.pixelSize + 10
                T.Control {
                    id: _item4_3
                    objectName: "_item4_3"
                    font.pixelSize: item4.font.pixelSize - 1
                }
                T.Control {
                    id: _item4_4
                    objectName: "_item4_4"
                }
            }
        }
    }

    function test_font_3() {
        var control4 = createTemporaryObject(component4, testCase)
        verify(control4)
        verify(control4.item4_2)
        verify(control4.item4_3)
        verify(control4.item4_4)

        var family = control4.font.family
        var ps = control4.font.pixelSize

        compare(control4.item4_2.font.family, control4.font.family)
        compare(control4.item4_3.font.family, control4.font.family)
        compare(control4.item4_4.font.family, control4.font.family)

        compare(control4.item4_2.font.pixelSize, control4.font.pixelSize + 10)
        compare(control4.item4_3.font.pixelSize, control4.font.pixelSize - 1)
        compare(control4.item4_4.font.pixelSize, control4.font.pixelSize + 10)

        control4.item4_2.font.pixelSize = control4.font.pixelSize + 15
        compare(control4.item4_2.font.pixelSize, control4.font.pixelSize + 15)
        compare(control4.item4_3.font.pixelSize, control4.font.pixelSize - 1)
        compare(control4.item4_4.font.pixelSize, control4.font.pixelSize + 15)
    }

    function test_font_explicit_attributes_data() {
        return [
            {tag: "bold", value: true},
            {tag: "capitalization", value: Font.Capitalize},
            {tag: "family", value: "Courier"},
            {tag: "italic", value: true},
            {tag: "strikeout", value: true},
            {tag: "underline", value: true},
            {tag: "weight", value: Font.Black},
            {tag: "wordSpacing", value: 55}
        ]
    }

    function test_font_explicit_attributes(data) {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        var child = component.createObject(control)
        verify(child)

        var controlSpy = signalSpy.createObject(control, {target: control, signalName: "fontChanged"})
        verify(controlSpy.valid)

        var childSpy = signalSpy.createObject(child, {target: child, signalName: "fontChanged"})
        verify(childSpy.valid)

        var defaultValue = control.font[data.tag]
        child.font[data.tag] = defaultValue

        compare(child.font[data.tag], defaultValue)
        compare(childSpy.count, 0)

        control.font[data.tag] = data.value

        compare(control.font[data.tag], data.value)
        compare(controlSpy.count, 1)

        compare(child.font[data.tag], defaultValue)
        compare(childSpy.count, 0)
    }

    function test_locale() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        control.locale = Qt.locale("en_US")
        compare(control.locale.name, "en_US")

        control.locale = Qt.locale("nb_NO")
        compare(control.locale.name, "nb_NO")
    }

    Component {
        id: component5
        T.Control {
            id: item2
            objectName: "item2"
            property alias localespy: _lspy;
            property alias mirroredspy: _mspy;
            property alias localespy_2: _lspy_2;
            property alias mirroredspy_2: _mspy_2;
            property alias localespy_3: _lspy_3;
            property alias mirroredspy_3: _mspy_3;
            property alias item2_2: _item2_2;
            property alias item2_3: _item2_3;
            T.Control {
                id: _item2_2
                objectName: "_item2_2"
                T.Control {
                    id: _item2_3
                    objectName: "_item2_3"

                    SignalSpy {
                        id: _lspy_3
                        target: item2_3
                        signalName: "localeChanged"
                    }

                    SignalSpy {
                        id: _mspy_3
                        target: item2_3
                        signalName: "mirroredChanged"
                    }
                }

                SignalSpy {
                    id: _lspy_2
                    target: item2_2
                    signalName: "localeChanged"
                }

                SignalSpy {
                    id: _mspy_2
                    target: item2_2
                    signalName: "mirroredChanged"
                }
            }

            SignalSpy {
                id: _lspy
                target: item2
                signalName: "localeChanged"
            }

            SignalSpy {
                id: _mspy
                target: item2
                signalName: "mirroredChanged"
            }
        }
    }

    function test_locale_2() {
        var control = createTemporaryObject(component5, testCase)
        verify(control)
        verify(control.item2_2)
        verify(control.item2_3)

        var defaultLocale = Qt.locale()

        compare(control.locale.name, defaultLocale.name)
        compare(control.item2_2.locale.name, defaultLocale.name)
        compare(control.item2_3.locale.name, defaultLocale.name)

        control.locale = Qt.locale("nb_NO")
        control.localespy.wait()
        compare(control.localespy.count, 1)
        compare(control.mirroredspy.count, 0)
        compare(control.locale.name, "nb_NO")
        compare(control.item2_2.locale.name, "nb_NO")
        compare(control.item2_3.locale.name, "nb_NO")
        compare(control.localespy_2.count, 1)
        compare(control.mirroredspy_2.count, 0)
        compare(control.localespy_3.count, 1)
        compare(control.mirroredspy_3.count, 0)

        control.locale = Qt.locale("ar_EG")
        control.localespy.wait()
        compare(control.localespy.count, 2)
        compare(control.mirroredspy.count, 1)
        compare(control.locale.name, "ar_EG")
        compare(control.item2_2.locale.name, "ar_EG")
        compare(control.item2_3.locale.name, "ar_EG")
        compare(control.localespy_2.count, 2)
        compare(control.mirroredspy_2.count, 1)
        compare(control.localespy_3.count, 2)
        compare(control.mirroredspy_3.count, 1)
    }

    Component {
        id: component6
        T.Control {
            id: item6
            objectName: "item6"
            property alias localespy: _lspy;
            property alias mirroredspy: _mspy;
            property alias localespy_5: _lspy_5;
            property alias mirroredspy_5: _mspy_5;
            property alias item6_2: _item6_2;
            property alias item6_3: _item6_3;
            property alias item6_4: _item6_4;
            property alias item6_5: _item6_5;
            Item {
                id: _item6_2
                objectName: "_item6_2"
                T.Control {
                    id: _item6_3
                    objectName: "_item6_3"
                    Item {
                        id: _item6_4
                        objectName: "_item6_4"
                        T.Control {
                            id: _item6_5
                            objectName: "_item6_5"

                            SignalSpy {
                                id: _lspy_5
                                target: _item6_5
                                signalName: "localeChanged"
                            }

                            SignalSpy {
                                id: _mspy_5
                                target: _item6_5
                                signalName: "mirroredChanged"
                            }
                        }
                    }
                }
            }

            SignalSpy {
                id: _lspy
                target: item6
                signalName: "localeChanged"
            }

            SignalSpy {
                id: _mspy
                target: item6
                signalName: "mirroredChanged"
            }
        }
    }

    function test_locale_3() {
        var control = createTemporaryObject(component6, testCase)
        verify(control)
        verify(control.item6_2)
        verify(control.item6_3)
        verify(control.item6_4)
        verify(control.item6_5)

        var defaultLocale = Qt.locale()

        compare(control.locale.name, defaultLocale.name)
        compare(control.item6_5.locale.name, defaultLocale.name)

        control.locale = Qt.locale("nb_NO")
        control.localespy.wait()
        compare(control.localespy.count, 1)
        compare(control.mirroredspy.count, 0)
        compare(control.locale.name, "nb_NO")
        compare(control.item6_5.locale.name, "nb_NO")
        compare(control.localespy_5.count, 1)
        compare(control.mirroredspy_5.count, 0)

        control.locale = Qt.locale("ar_EG")
        control.localespy.wait()
        compare(control.localespy.count, 2)
        compare(control.mirroredspy.count, 1)
        compare(control.locale.name, "ar_EG")
        compare(control.item6_5.locale.name, "ar_EG")
        compare(control.localespy_5.count, 2)
        compare(control.mirroredspy_5.count, 1)
    }

    function test_hover_data() {
        return [
            { tag: "normal", target: component, pressed: false },
            { tag: "pressed", target: button, pressed: true }
        ]
    }

    function test_hover(data) {
        var control = createTemporaryObject(data.target, testCase, {width: 100, height: 100})
        verify(control)

        compare(control.hovered, false)
        compare(control.hoverEnabled, Qt.styleHints.useHoverEffects)

        control.hoverEnabled = false

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.hovered, false)

        control.hoverEnabled = true

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.hovered, true)

        if (data.pressed) {
            mousePress(control, control.width / 2, control.height / 2)
            compare(control.hovered, true)
        }

        mouseMove(control, -10, -10)
        compare(control.hovered, false)

        if (data.pressed) {
            mouseRelease(control, -10, control.height / 2)
            compare(control.hovered, false)
        }

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.hovered, true)

        control.visible = false
        compare(control.hovered, false)
    }

    function test_hoverEnabled() {
        var control = createTemporaryObject(component, testCase)
        compare(control.hoverEnabled, Qt.styleHints.useHoverEffects)

        var child = component.createObject(control)
        var grandChild = component.createObject(child)

        var childExplicitHoverEnabled = component.createObject(control, {hoverEnabled: true})
        var grandChildExplicitHoverDisabled = component.createObject(childExplicitHoverEnabled, {hoverEnabled: false})

        var childExplicitHoverDisabled = component.createObject(control, {hoverEnabled: false})
        var grandChildExplicitHoverEnabled = component.createObject(childExplicitHoverDisabled, {hoverEnabled: true})

        control.hoverEnabled = false
        compare(control.hoverEnabled, false)
        compare(grandChild.hoverEnabled, false)

        compare(childExplicitHoverEnabled.hoverEnabled, true)
        compare(grandChildExplicitHoverDisabled.hoverEnabled, false)

        compare(childExplicitHoverDisabled.hoverEnabled, false)
        compare(grandChildExplicitHoverEnabled.hoverEnabled, true)

        control.hoverEnabled = true
        compare(control.hoverEnabled, true)
        compare(grandChild.hoverEnabled, true)

        compare(childExplicitHoverEnabled.hoverEnabled, true)
        compare(grandChildExplicitHoverDisabled.hoverEnabled, false)

        compare(childExplicitHoverDisabled.hoverEnabled, false)
        compare(grandChildExplicitHoverEnabled.hoverEnabled, true)
    }

    function test_implicitSize() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        var implicitWidthSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitWidthChanged"})
        verify(implicitWidthSpy.valid)

        var implicitHeightSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitHeightChanged"})
        verify(implicitHeightSpy.valid)

        var implicitContentWidthSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitContentWidthChanged"})
        verify(implicitContentWidthSpy.valid)

        var implicitContentHeightSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitContentHeightChanged"})
        verify(implicitContentHeightSpy.valid)

        var implicitBackgroundWidthSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitBackgroundWidthChanged"})
        verify(implicitBackgroundWidthSpy.valid)

        var implicitBackgroundHeightSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitBackgroundHeightChanged"})
        verify(implicitBackgroundHeightSpy.valid)

        var implicitWidthChanges = 0
        var implicitHeightChanges = 0
        var implicitContentWidthChanges = 0
        var implicitContentHeightChanges = 0
        var implicitBackgroundWidthChanges = 0
        var implicitBackgroundHeightChanges = 0

        compare(control.implicitWidth, 0)
        compare(control.implicitHeight, 0)
        compare(control.implicitContentWidth, 0)
        compare(control.implicitContentHeight, 0)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)

        control.contentItem = rectangle.createObject(control, {implicitWidth: 10, implicitHeight: 20})
        compare(control.implicitWidth, 10)
        compare(control.implicitHeight, 20)
        compare(control.implicitContentWidth, 10)
        compare(control.implicitContentHeight, 20)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)
        compare(implicitHeightSpy.count, ++implicitHeightChanges)
        compare(implicitBackgroundWidthSpy.count, implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, implicitBackgroundHeightChanges)
        compare(implicitContentWidthSpy.count, ++implicitContentWidthChanges)
        compare(implicitContentHeightSpy.count, ++implicitContentHeightChanges)

        control.contentItem.implicitWidth += 1
        control.contentItem.implicitHeight += 1
        compare(control.implicitWidth, 11)
        compare(control.implicitHeight, 21)
        compare(control.implicitContentWidth, 11)
        compare(control.implicitContentHeight, 21)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)
        compare(implicitHeightSpy.count, ++implicitHeightChanges)
        compare(implicitContentWidthSpy.count, ++implicitContentWidthChanges)
        compare(implicitContentHeightSpy.count, ++implicitContentHeightChanges)
        compare(implicitBackgroundWidthSpy.count, implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, implicitBackgroundHeightChanges)

        control.background = rectangle.createObject(control, {implicitWidth: 20, implicitHeight: 30})
        compare(control.implicitWidth, 20)
        compare(control.implicitHeight, 30)
        compare(control.implicitContentWidth,11)
        compare(control.implicitContentHeight, 21)
        compare(control.implicitBackgroundWidth, 20)
        compare(control.implicitBackgroundHeight, 30)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)
        compare(implicitHeightSpy.count, ++implicitHeightChanges)
        compare(implicitContentWidthSpy.count, implicitContentWidthChanges)
        compare(implicitContentHeightSpy.count, implicitContentHeightChanges)
        compare(implicitBackgroundWidthSpy.count, ++implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, ++implicitBackgroundHeightChanges)

        control.background.implicitWidth += 1
        control.background.implicitHeight += 1
        compare(control.implicitWidth, 21)
        compare(control.implicitHeight, 31)
        compare(control.implicitContentWidth, 11)
        compare(control.implicitContentHeight, 21)
        compare(control.implicitBackgroundWidth, 21)
        compare(control.implicitBackgroundHeight, 31)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)
        compare(implicitHeightSpy.count, ++implicitHeightChanges)
        compare(implicitContentWidthSpy.count, implicitContentWidthChanges)
        compare(implicitContentHeightSpy.count, implicitContentHeightChanges)
        compare(implicitBackgroundWidthSpy.count, ++implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, ++implicitBackgroundHeightChanges)
    }

    function test_baseline() {
        var control = createTemporaryObject(component, testCase)
        verify(control)

        compare(control.baselineOffset, 0)

        var baselineSpy = signalSpy.createObject(control, {target: control, signalName: "baselineOffsetChanged"})
        verify(baselineSpy.valid)

        control.contentItem = rectangle.createObject(control, {baselineOffset: 12})
        compare(control.baselineOffset, 12)
        compare(baselineSpy.count, 1)

        control.padding = 6
        compare(control.baselineOffset, 18)
        compare(baselineSpy.count, 2)

        control.baselineOffset = 3
        compare(control.baselineOffset, 3)
        compare(baselineSpy.count, 3)

        control.padding = 9
        compare(control.baselineOffset, 3)
        compare(baselineSpy.count, 3)

        control.baselineOffset = undefined
        compare(control.baselineOffset, 21)
        compare(baselineSpy.count, 4)

        control.contentItem.baselineOffset = 3
        compare(control.baselineOffset, 12)
        compare(baselineSpy.count, 5)

        control.contentItem = null
        compare(control.baselineOffset, 0)
        compare(baselineSpy.count, 6)
    }

    function test_inset() {
        var control = createTemporaryObject(component, testCase, {background: rectangle.createObject(control)})
        verify(control)

        var topInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "topInsetChanged"})
        verify(topInsetSpy.valid)

        var leftInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "leftInsetChanged"})
        verify(leftInsetSpy.valid)

        var rightInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "rightInsetChanged"})
        verify(rightInsetSpy.valid)

        var bottomInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "bottomInsetChanged"})
        verify(bottomInsetSpy.valid)

        var topInsetChanges = 0
        var leftInsetChanges = 0
        var rightInsetChanges = 0
        var bottomInsetChanges = 0

        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)

        control.width = 100
        control.height = 100
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 100)
        compare(control.background.height, 100)

        control.topInset = 10
        compare(control.topInset, 10)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, ++topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 10)
        compare(control.background.width, 100)
        compare(control.background.height, 90)

        control.leftInset = 20
        compare(control.topInset, 10)
        compare(control.leftInset, 20)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, ++leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 10)
        compare(control.background.width, 80)
        compare(control.background.height, 90)

        control.rightInset = 30
        compare(control.topInset, 10)
        compare(control.leftInset, 20)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, ++rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 10)
        compare(control.background.width, 50)
        compare(control.background.height, 90)

        control.bottomInset = 40
        compare(control.topInset, 10)
        compare(control.leftInset, 20)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, ++bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 10)
        compare(control.background.width, 50)
        compare(control.background.height, 50)

        control.topInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 20)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, ++topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 0)
        compare(control.background.width, 50)
        compare(control.background.height, 60)

        control.leftInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, ++leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 70)
        compare(control.background.height, 60)

        control.rightInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, ++rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 100)
        compare(control.background.height, 60)

        control.bottomInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, ++bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 100)
        compare(control.background.height, 100)
    }
}
