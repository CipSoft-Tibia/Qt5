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

Item {
    id: selector
    property int curIdx: 0
    property int maxIdx: 3
    property int gridWidth: 240
    property Flickable flickable
    width: parent.width
    height: 64
    function advance(steps) {
         var nextIdx = curIdx + steps
         if (nextIdx < 0 || nextIdx > maxIdx)
            return;
         flickable.contentX += gridWidth * steps;
         curIdx += steps;
    }
    Image {
        source: "arrow.png"
        MouseArea{
            anchors.fill: parent
            onClicked: selector.advance(-1)
        }
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        opacity: selector.curIdx == 0 ? 0.2 : 1.0
        Behavior on opacity {NumberAnimation{}}
    }
    Image {
        source: "arrow.png"
        mirror: true
        MouseArea{
            anchors.fill: parent
            onClicked: selector.advance(1)
        }
        opacity: selector.curIdx == selector.maxIdx ? 0.2 : 1.0
        Behavior on opacity {NumberAnimation{}}
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
    }
    Repeater {
        model: [ "Scale", "Repeat", "Scale/Repeat", "Round" ]
        delegate: Text {
            required property string modelData
            required property int index

            text: modelData
            anchors.verticalCenter: parent.verticalCenter

            x: (index - selector.curIdx) * 80 + 140
            Behavior on x { NumberAnimation{} }

            opacity: selector.curIdx == index ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation{} }
        }
    }
}
