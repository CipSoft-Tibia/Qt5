/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt-project.org/legal
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
import QtQuick.Window 2.2
import QtWayland.Compositor 1.14

WaylandOutput {
    id: output
    property alias gridSurfaces: listModel

    sizeFollowsWindow: true
    window: Window {
        width: 1024
        height: 760
        visible: true

        Image {
            id: background
            anchors.fill: parent
            fillMode: Image.Tile
            source: "qrc:/images/background.jpg"
            smooth: true
            GridView {
                id: gridView
                anchors.fill: parent
                model: ListModel {
                    id: listModel
                }
                interactive: false
                cellWidth: 200
                cellHeight: 200
                delegate: WaylandQuickItem {
                    id: item
                    surface: gridSurface
                    width: gridView.cellWidth
                    height: gridView.cellHeight
                    sizeFollowsSurface: false
                    inputEventsEnabled: false
                    allowDiscardFrontBuffer: true
                    MouseArea {
                        anchors.fill: parent
                        onClicked: item.surface.activated()
                    }
                }
            }
        }
    }

    XdgOutputV1 {
        name: "WL-2"
        description: "Overview screen"
        logicalPosition: output.position
        logicalSize: Qt.size(output.geometry.width / output.scaleFactor,
                             output.geometry.height / output.scaleFactor)
    }
}
