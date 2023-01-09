/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
    id: focusRect
    property string text

    x: 62
    y: 75
    width: 75
    height: 50
    radius: 6
    border.width: 4
    border.color: "white"
    color: "firebrick"

    // Set an 'elastic' behavior on the focusRect's x property.
    Behavior on x {
        NumberAnimation {
            easing.type: Easing.OutElastic
            easing.amplitude: 3.0
            easing.period: 2.0
            duration: 300
        }
    }

    //! [0]
    // Set an 'elastic' behavior on the focusRect's y property.
    Behavior on y {
        NumberAnimation {
            easing.type: Easing.OutElastic
            easing.amplitude: 3.0
            easing.period: 2.0
            duration: 300
        }
    }
    //! [0]

    Text {
        id: focusText
        text: focusRect.text
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 16
        font.bold: true

        // Set a behavior on the focusText's x property:
        // Set the opacity to 0, set the new text value, then set the opacity back to 1.
        Behavior on text {
            SequentialAnimation {
                NumberAnimation {
                    target: focusText
                    property: "opacity"
                    to: 0
                    duration: 150
                }
                NumberAnimation {
                    target: focusText
                    property: "opacity"
                    to: 1
                    duration: 150
                }
            }
        }
    }
}
