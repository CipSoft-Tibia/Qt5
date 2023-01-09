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
    id: root
    color: "white"
    //width: containerColumn.width
    //height: containerColumn.height + containerColumn.anchors.topMargin
    width: 320
    height: 480

    property bool mirror: false
    property int pxSz: 18
    property variant horizontalAlignment: undefined

    property variant editorType: ["Plain Text", "Styled Text", "Plain Rich Text", "Italic Rich Text", "Plain TextEdit", "Italic TextEdit", "TextInput"]
    property variant text: ["", " ", "Hello world!", "مرحبا العالم!", "Hello world! Hello!\nHello world! Hello!", "مرحبا العالم! مرحبا! مرحبا العالم! مرحبا!" ,"مرحبا العالم! مرحبا! مرحبا Hello world!\nالعالم! مرحبا!"]
    property variant description: ["empty text", "white-space-only text", "left-to-right text", "right-to-left text", "multi-line left-to-right text", "multi-line right-to-left text", "multi-line bidi text"]
    property variant textComponents: [plainTextComponent, styledTextComponent, richTextComponent, italicRichTextComponent, plainTextEdit, italicTextEdit, textInput]

    function shortText(horizontalAlignment) {

        // all the different QML editors have
        // the same alignment values
        switch (horizontalAlignment) {
        case Text.AlignLeft:
            return "L";
        case Text.AlignRight:
            return "R";
        case Text.AlignHCenter:
            return "C";
        case Text.AlignJustify:
            return "J";
        default:
            return "Error";
        }
    }
    Column {
        id: containerColumn
        spacing: 10
        width: editorTypeRow.width
        anchors { top: parent.top; topMargin: 5 }
        ListView {
            width: 320
            height: 320
            id: editorTypeRow
            model: root.editorType.length
            orientation: ListView.Horizontal
            cacheBuffer: 1000//Load the really expensive ones async if possible
            delegate: Item {
                id: delegate

                width: editorColumn.width
                height: editorColumn.height

                required property int index

                Column {
                    id: editorColumn
                    spacing: 5
                    width: textColumn.width+10
                    Text {
                        text: root.editorType[delegate.index]
                        font.pixelSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Column {
                        id: textColumn
                        spacing: 5
                        anchors.horizontalCenter: parent.horizontalCenter
                        Repeater {
                            model: root.textComponents.length
                            delegate: root.textComponents[delegate.index]
                        }
                    }
                }
            }
        }
        Column {
            spacing: 2
            width: parent.width
            Rectangle {
                // button
                height: 50; width: parent.width
                color: mouseArea.pressed ? "black" : "lightgray"
                Column {
                    anchors.centerIn: parent
                    Text {
                        text: root.mirror ? "Mirrored" : "Not mirrored"
                        color: "white"
                        font.pixelSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: "(click here to toggle)"
                        color: "white"
                        font.pixelSize: 10
                        font.italic: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                MouseArea {
                    id: mouseArea
                    property int index: 0
                    anchors.fill: parent
                    onClicked: root.mirror = !root.mirror
                }
            }
            Rectangle {
                // button
                height: 50; width: parent.width
                color: mouseArea2.pressed ? "black" : "gray"
                Column {
                    anchors.centerIn: parent
                    Text {
                        text: {
                            if (root.horizontalAlignment == undefined)
                                return "Implict alignment";
                            switch (root.horizontalAlignment) {
                            case Text.AlignLeft:
                                return "Left alignment";
                            case Text.AlignRight:
                                return "Right alignment";
                            case Text.AlignHCenter:
                                return "Center alignment";
                            case Text.AlignJustify:
                                return "Justify alignment";
                            }
                        }
                        color: "white"
                        font.pixelSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: "(click here to toggle)"
                        color: "white"
                        font.pixelSize: 10
                        font.italic: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                MouseArea {
                    id: mouseArea2
                    property int index: 0
                    anchors.fill: parent
                    onClicked: {
                        if (index < 0) {
                            root.horizontalAlignment = undefined;
                        } else {
                            root.horizontalAlignment = Math.pow(2, index);
                        }
                        index = (index + 2) % 5 - 1;
                    }
                }
            }
        }
    }

    Component {
        id: plainTextComponent
        Text {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            Rectangle {
                z: -1
                color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: styledTextComponent
        Text {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            style: Text.Sunken
            styleColor: "white"
            Rectangle {
                z: -1
                color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: richTextComponent
        Text {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            Rectangle {
                z: -1
                color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: italicRichTextComponent
        Text {
            required property int index

            width: 180
            text: "<i>" + root.text[index] + "</i>"
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            property variant backgroundColor: Qt.rgba(0.8, 0.2, 0.2, 0.3)
            Rectangle {
                z: -1
                color: parent.backgroundColor
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: plainTextEdit
        TextEdit {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            cursorVisible: true
            wrapMode: TextEdit.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            Rectangle {
                z: -1
                color: Qt.rgba(0.5, 0.5, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: italicTextEdit
        TextEdit {
            required property int index

            width: 180
            text: "<i>" + root.text[index] + "<i>"
            font.pixelSize: root.pxSz
            cursorVisible: true
            wrapMode: TextEdit.WordWrap
            textFormat: TextEdit.RichText
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            Rectangle {
                z: -1
                color: Qt.rgba(0.5, 0.5, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: textInput
        Item {
            id: textDelegate
            required property int index
            width: 180
            height: textInput.text.length > 20 ? 3*textInput.height : textInput.height
            TextInput {
                id: textInput
                width: 180
                text: root.text[textDelegate.index]
                font.pixelSize: root.pxSz
                cursorVisible: true
                horizontalAlignment: root.horizontalAlignment
                LayoutMirroring.enabled: root.mirror
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.6, 0.4, 0.2, 0.3)
                    anchors.fill: parent
                }
                Text {
                    text: root.description[textDelegate.index]
                    color: Qt.rgba(1,1,1,1.0)
                    anchors.centerIn: parent
                    font.pixelSize: root.pxSz - 2
                    Rectangle {
                        z: -1
                        color: Qt.rgba(0.3, 0, 0, 0.3)
                        anchors { fill: parent; margins: -3 }
                    }
                }
                Text {
                    color: "white"
                    text: root.shortText(parent.horizontalAlignment)
                    anchors { top: parent.top; right: parent.right; margins: 2 }
                }
            }
        }
    }
}

