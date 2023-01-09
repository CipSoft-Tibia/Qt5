/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtNfc module.
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

import QtQuick 2.4

Item {
    id: page
    width: ListView.view.width;
    height: ListView.view.height

    Image {
        source: "qrc:/cork.jpg"
        anchors.centerIn: parent
        width: parent.width - 20
        height: parent.height - 20
        fillMode: Image.PreserveAspectCrop

        Text {
            anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 10}
            text: name;
            font { pixelSize: 30; bold: true }
            color: "white"
            style: Text.Outline; styleColor: "black"
        }

        Repeater {
            model: notes
            Item {
                id: stickyPage

                x: ListView.width * (0.7 * Math.random() + 0.1)
                y: ListView.height * (0.7 * Math.random() + 0.1)

                rotation: -listView.horizontalVelocity / 200;
                Behavior on rotation {
                    SpringAnimation { spring: 2.0; damping: 0.15 }
                }

                Item {
                    id: sticky
                    scale: mouse.pressed ? 1 : 0.7
                    rotation: mouse.pressed ? 8 : 0
                    Behavior on rotation{
                        NumberAnimation {duration: 200 }
                    }
                    Behavior on scale{
                        NumberAnimation { duration: 200 }
                    }

                    Image {
                        id: stickyImage
                        x: 8 + -width * 0.6 / 2; y: -20
                        source: "qrc:/note-yellow.png"
                        scale: 0.6; transformOrigin: Item.TopLeft
                        smooth: true
                    }

                    TextEdit {
                        id: myText
                        text: noteText
                        x: -104; y: 36; width: 215; height: 200
                        smooth: true
                        font.pixelSize: 24
                        readOnly: false
                        rotation: -8
                        wrapMode: TextEdit.Wrap
                    }

                    Item {
                        id: interactionItem
                        x: stickyImage.x; y: -20
                        width: stickyImage.width * stickyImage.scale
                        height: stickyImage.height * stickyImage.scale

                        MouseArea {
                            id: mouse
                            anchors.fill: parent
                            drag.target: stickyPage
                            drag.axis: Drag.XandYAxis
                        }
                        Image {
                            id: writeButton
                            source: "qrc:/NfcFlag.png"
                            rotation: -8    // Note image itself is rotated
                            anchors { bottom: parent.bottom; right:parent.right }
                            scale: flagMouse.pressed ? 1.3 : 1
                            MouseArea {
                                id: flagMouse
                                anchors.fill: parent
                            }
                        }
                    }
                }

                Image {
                    x: -width / 2; y: -height * 0.5 / 2
                    source: "qrc:/tack.png"
                    scale: 0.7; transformOrigin: Item.TopLeft
                }
            }
        }
    }
}








