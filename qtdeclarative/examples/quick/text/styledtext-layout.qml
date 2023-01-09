/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick 2.0

Rectangle {
    id: main
    width: 320; height: 480
    focus: true

    property real offset: 0
    property real margin: 8

    Text {
        id: myText
        anchors.fill: parent
        anchors.margins: 10
        wrapMode: Text.WordWrap
        font.family: "Times New Roman"
        font.pixelSize: 14
        textFormat: Text.StyledText
        horizontalAlignment: Text.AlignJustify

        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer at ante dui <a href=\"http://www.digia.com\">www.digia.com</a>.<br/>Curabitur ante est, pulvinar quis adipiscing a, iaculis id ipsum. Nunc blandit condimentum odio vel egestas.<br><ul type=\"bullet\"><li>Coffee<ol type=\"a\"><li>Espresso<li>Cappuccino<li>Latte</ol><li>Juice<ol type=\"1\"><li>Orange</li><li>Apple</li><li>Pineapple</li><li>Tomato</li></ol></li></ul><p><font color=\"#434343\"><i>Proin consectetur <b>sapien</b> in ipsum lacinia sit amet mattis orci interdum. Quisque vitae accumsan lectus. Ut nisi turpis, sollicitudin ut dignissim id, fermentum ac est. Maecenas nec libero leo. Sed ac leo eget ipsum ultricies viverra sit amet eu orci. Praesent et tortor risus, viverra accumsan sapien. Sed faucibus eleifend lectus, sed euismod urna porta eu. Quisque vitae accumsan lectus. Ut nisi turpis, sollicitudin ut dignissim id, fermentum ac est. Maecenas nec libero leo. Sed ac leo eget ipsum ultricies viverra sit amet eu orci."

//! [layout]
        onLineLaidOut: (line) => {
            line.width = width / 2  - main.margin

            if (line.y + line.height >= height) {
                line.y -= height - main.margin
                line.x = width / 2 + main.activeFocusmargin
            }

            if (line.isLast) {
                lastLineMarker.x = line.x + line.implicitWidth
                lastLineMarker.y = line.y + (line.height - lastLineMarker.height) / 2
            }
        }
//! [layout]

        Rectangle {
            id: lastLineMarker
            color: "#44cccccc"
            width: theEndText.width + margin
            height: theEndText.height + margin

            Text {
                id: theEndText
                text: "THE\nEND"
                anchors.centerIn: parent
                font.pixelSize: myText.font.pixelSize / 2
            }
        }
    }

}
