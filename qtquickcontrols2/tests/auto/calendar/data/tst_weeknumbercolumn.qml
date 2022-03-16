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
import Qt.labs.calendar 1.0

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "WeekNumberColumn"

    Component {
        id: component
        WeekNumberColumn { }
    }

    function test_locale() {
        var control = component.createObject(testCase)

        compare(control.contentItem.children.length, 6 + 1)

        control.month = 11
        control.year = 2015

        // en_US: [48...53]
        control.locale = Qt.locale("en_US")
        for (var i = 0; i < 6; ++i)
            compare(control.contentItem.children[i].text, (i + 48).toString())

        // no_NO: [49...1]
        control.locale = Qt.locale("no_NO")
        for (var j = 0; j < 5; ++j)
            compare(control.contentItem.children[j].text, (j + 49).toString())
        compare(control.contentItem.children[5].text, "1")

        control.destroy()
    }

    function test_range() {
        var control = component.createObject(testCase)

        control.month = 0
        compare(control.month, 0)

        ignoreWarning(Qt.resolvedUrl("tst_weeknumbercolumn.qml") + ":65:9: QML AbstractWeekNumberColumn: month -1 is out of range [0...11]")
        control.month = -1
        compare(control.month, 0)

        control.month = 11
        compare(control.month, 11)

        ignoreWarning(Qt.resolvedUrl("tst_weeknumbercolumn.qml") + ":65:9: QML AbstractWeekNumberColumn: month 12 is out of range [0...11]")
        control.month = 12
        compare(control.month, 11)

        control.year = -271820
        compare(control.year, -271820)

        ignoreWarning(Qt.resolvedUrl("tst_weeknumbercolumn.qml") + ":65:9: QML AbstractWeekNumberColumn: year -271821 is out of range [-271820...275759]")
        control.year = -271821
        compare(control.year, -271820)

        control.year = 275759
        compare(control.year, 275759)

        ignoreWarning(Qt.resolvedUrl("tst_weeknumbercolumn.qml") + ":65:9: QML AbstractWeekNumberColumn: year 275760 is out of range [-271820...275759]")
        control.year = 275760
        compare(control.year, 275759)

        control.destroy()
    }

    function test_font() {
        var control = component.createObject(testCase)

        verify(control.contentItem.children[0])

        control.font.pixelSize = 123
        compare(control.contentItem.children[0].font.pixelSize, 123)

        control.destroy()
    }
}
