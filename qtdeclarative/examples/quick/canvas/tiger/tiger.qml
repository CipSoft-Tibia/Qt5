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
import "tiger.js" as Tiger

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
            text: "Tiger with SVG path"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }

        Canvas {
            id: canvas
            width: 320
            height: 280
            antialiasing: true

            property string strokeStyle: "steelblue"
            property string fillStyle: "yellow"
            property bool fill: true
            property bool stroke: true
            property real alpha: alphaCtrl.value
            property real scale: scaleCtrl.value
            property real rotate: rotateCtrl.value
            property int frame: 0

            onFillChanged: requestPaint();
            onStrokeChanged: requestPaint();
            onAlphaChanged: requestPaint();
            onScaleChanged: requestPaint();
            onRotateChanged: requestPaint();

            onPaint: {
                var ctx = canvas.getContext('2d');
                var originX = canvas.width/2 + 30
                var originY = canvas.height/2 + 60

                ctx.save();
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.globalAlpha = canvas.alpha;
                ctx.globalCompositeOperation = "source-over";

                ctx.translate(originX, originY)
                ctx.scale(canvas.scale, canvas.scale);
                ctx.rotate(canvas.rotate);
                ctx.translate(-originX, -originY)

                ctx.strokeStyle = Qt.rgba(.3, .3, .3,1);
                ctx.lineWidth = 1;

                //! [0]
                for (var i = 0; i < Tiger.tiger.length; i++) {
                    if (Tiger.tiger[i].width != undefined)
                        ctx.lineWidth = Tiger.tiger[i].width;

                    if (Tiger.tiger[i].path != undefined)
                        ctx.path = Tiger.tiger[i].path;

                    if (Tiger.tiger[i].fill != undefined) {
                        ctx.fillStyle = Tiger.tiger[i].fill;
                        ctx.fill();
                    }

                    if (Tiger.tiger[i].stroke != undefined) {
                        ctx.strokeStyle = Tiger.tiger[i].stroke;
                        ctx.stroke();
                    }
                }
                //! [0]
                ctx.restore();
            }
        }
    }
    Column {
        id: controls
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        Slider {id: scaleCtrl ; min: 0.1 ; max: 1 ; init: 0.3 ; name: "Scale"}
        Slider {id: rotateCtrl ; min: 0 ; max: Math.PI*2 ; init: 0 ; name: "Rotate"}
        Slider {id: alphaCtrl ; min: 0 ; max: 1 ; init: 1 ; name: "Alpha"}
    }
}
