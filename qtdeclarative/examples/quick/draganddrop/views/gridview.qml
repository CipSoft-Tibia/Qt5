/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

import QtQml 2.14
import QtQuick 2.14
import QtQml.Models 2.14

GridView {
    id: root
    width: 320; height: 480
    cellWidth: 80; cellHeight: 80

    displaced: Transition {
        NumberAnimation { properties: "x,y"; easing.type: Easing.OutQuad }
    }

//! [0]
    model: DelegateModel {
//! [0]
        id: visualModel
        model: ListModel {
            id: colorModel
            ListElement { color: "blue" }
            ListElement { color: "green" }
            ListElement { color: "red" }
            ListElement { color: "yellow" }
            ListElement { color: "orange" }
            ListElement { color: "purple" }
            ListElement { color: "cyan" }
            ListElement { color: "magenta" }
            ListElement { color: "chartreuse" }
            ListElement { color: "aquamarine" }
            ListElement { color: "indigo" }
            ListElement { color: "black" }
            ListElement { color: "lightsteelblue" }
            ListElement { color: "violet" }
            ListElement { color: "grey" }
            ListElement { color: "springgreen" }
            ListElement { color: "salmon" }
            ListElement { color: "blanchedalmond" }
            ListElement { color: "forestgreen" }
            ListElement { color: "pink" }
            ListElement { color: "navy" }
            ListElement { color: "goldenrod" }
            ListElement { color: "crimson" }
            ListElement { color: "teal" }
        }
//! [1]
        delegate: DropArea {
            id: delegateRoot
            required property color color;

            width: 80; height: 80

            onEntered: function(drag) {
                visualModel.items.move((drag.source as Icon).visualIndex, icon.visualIndex)
            }

            property int visualIndex: DelegateModel.itemsIndex

            Icon {
                id: icon
                dragParent: root
                visualIndex: delegateRoot.visualIndex
                color: delegateRoot.color
            }
        }
//! [1]
    }
}
