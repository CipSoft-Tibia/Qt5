/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.2

Item {
    // This enum is actually keyboard-related, but it serves its purpose
    // as an indication of direction for us.
    property int direction: Qt.LeftArrow
    property bool on: false

    property bool flashing: false

    scale: direction === Qt.LeftArrow ? 1 : -1
//! [1]
    Timer {
        id: flashTimer
        interval: 500
        running: on
        repeat: true
        onTriggered: flashing = !flashing
    }
//! [1]
//! [2]
    function paintOutlinePath(ctx) {
        ctx.beginPath();
        ctx.moveTo(0, height * 0.5);
        ctx.lineTo(0.6 * width, 0);
        ctx.lineTo(0.6 * width, height * 0.28);
        ctx.lineTo(width, height * 0.28);
        ctx.lineTo(width, height * 0.72);
        ctx.lineTo(0.6 * width, height * 0.72);
        ctx.lineTo(0.6 * width, height);
        ctx.lineTo(0, height * 0.5);
    }
//! [2]
    Canvas {
        id: backgroundCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            paintOutlinePath(ctx);

            ctx.lineWidth = 1;
            ctx.strokeStyle = "black";
            ctx.stroke();
        }
    }
//! [3]
    Canvas {
        id: foregroundCanvas
        anchors.fill: parent
        visible: on && flashing

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            paintOutlinePath(ctx);

            ctx.fillStyle = "green";
            ctx.fill();
        }
    }
//! [3]
}
