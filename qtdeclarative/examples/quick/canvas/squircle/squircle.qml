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
import "../contents"
import "../../shared"

Item {
    id: container
    width: 320
    height: 480

    Column {
        spacing: 6
        anchors.fill: parent
        anchors.topMargin: 12
        Text {
            font.pointSize: 24
            font.bold: true
            text: "Squircles"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "squircle.png"
            width: 250
            height: 25
        }

        Canvas {
            id: canvas
            width: 320
            height: 250
            antialiasing: true

            property color strokeStyle: Qt.darker(fillStyle, 1.2)
            property color fillStyle: "#6400aa"

            property int lineWidth: 2
            property int nSize: nCtrl.value
            property real radius: rCtrl.value
            property bool fill: true
            property bool stroke: false
            property real px: width/2
            property real py: height/2 + 10
            property real alpha: 1.0

            onRadiusChanged: requestPaint();
            onLineWidthChanged: requestPaint();
            onNSizeChanged: requestPaint();
            onFillChanged: requestPaint();
            onStrokeChanged: requestPaint();

            onPaint: squircle();

            function squircle() {
                var ctx = canvas.getContext("2d");
                var N = canvas.nSize;
                var R = canvas.radius;

                N=Math.abs(N);
                var M=N;
                if (N>100) M=100;
                if (N<0.00000000001) M=0.00000000001;

                ctx.save();
                ctx.globalAlpha =canvas.alpha;
                ctx.fillStyle = "white";
                ctx.fillRect(0, 0, canvas.width, canvas.height);

                ctx.strokeStyle = canvas.strokeStyle;
                ctx.fillStyle = canvas.fillStyle;
                ctx.lineWidth = canvas.lineWidth;

                ctx.beginPath();
                var i = 0, x, y;
                for (i=0; i<(2*R+1); i++){
                    x = Math.round(i-R) + canvas.px;
                    y = Math.round(Math.pow(Math.abs(Math.pow(R,M)-Math.pow(Math.abs(i-R),M)),1/M)) + canvas.py;

                    if (i == 0)
                        ctx.moveTo(x, y);
                    else
                        ctx.lineTo(x, y);
                }

                for (i=(2*R); i<(4*R+1); i++){
                    x =Math.round(3*R-i)+canvas.px;
                    y = Math.round(-Math.pow(Math.abs(Math.pow(R,M)-Math.pow(Math.abs(3*R-i),M)),1/M)) + canvas.py;
                    ctx.lineTo(x, y);
                }
                ctx.closePath();
                if (canvas.stroke) {
                    ctx.stroke();
                }

                if (canvas.fill) {
                    ctx.fill();
                }
                ctx.restore();
            }
        }

    }
    Column {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        Slider {id: nCtrl ; min: 1 ; max: 10 ; init: 2 ; name: "N"}
        Slider {id: rCtrl ; min: 30 ; max: 180 ; init: 60 ; name: "Radius"}
    }
}
