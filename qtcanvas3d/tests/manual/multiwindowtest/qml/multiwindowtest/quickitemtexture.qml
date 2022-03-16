/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCanvas3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtCanvas3D 1.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2

import "quickitemtexture.js" as GLCode

Rectangle {
    id: mainView
    anchors.fill: parent
    visible: true
    color: "#f9f9f9"

    property alias canvas3d: canvas3d
    property string canvasName: ""
    property var previousParent: null

    onParentChanged: {
        if (previousParent && previousParent.handleParentChange)
            previousParent.handleParentChange()
        previousParent = parent
    }

    ColumnLayout {
        Layout.fillWidth: true
        x: 4
        y: 4
        Rectangle {
            id: textureSource
            color: "lightgreen"
            width: 256
            height: 256
            border.color: "blue"
            border.width: 4
            layer.enabled: true
            layer.smooth: true
            Label {
                anchors.fill: parent
                anchors.margins: 16
                text: "X Rot:" + (canvas3d.xRotAnim | 0) + "\n"
                    + "Y Rot:" + (canvas3d.yRotAnim | 0) + "\n"
                    + "Z Rot:" + (canvas3d.zRotAnim | 0) + "\n"
                    + "FPS:" + canvas3d.fps
                color: "red"
                font.pointSize: 30
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }
        Button {
            Layout.fillWidth: true
            Layout.minimumWidth: 256
            text: textureSource.visible ? "Hide texture source" : "Show texture source"
            onClicked: textureSource.visible = !textureSource.visible
        }
        Button {
            Layout.fillWidth: true
            Layout.minimumWidth: 256
            text: "Quit"
            onClicked: Qt.quit()
        }
    }

    Canvas3D {
        id: canvas3d
        anchors.fill:parent
        focus: true
        property double xRotAnim: 0
        property double yRotAnim: 0
        property double zRotAnim: 0
        property bool isRunning: true

        // Emitted when one time initializations should happen
        onInitializeGL: {
            GLCode.initializeGL(canvas3d, textureSource);
        }

        // Emitted each time Canvas3D is ready for a new frame
        onPaintGL: {
            if (canvas3d.renderTarget === Canvas3D.RenderTargetOffscreenBuffer)
                GLCode.paintGL(canvas3d, true);
            else
                GLCode.paintGL(canvas3d, false);
        }

        onResizeGL: {
            GLCode.resizeGL(canvas3d);
        }

        onContextLost: {
            console.log("Context lost on: ", mainView.canvasName)
        }

        onContextRestored: {
            console.log("Context restored on: ", mainView.canvasName)
        }

        Keys.onSpacePressed: {
            canvas3d.isRunning = !canvas3d.isRunning
            if (canvas3d.isRunning) {
                objAnimationX.pause();
                objAnimationY.pause();
                objAnimationZ.pause();
            } else {
                objAnimationX.resume();
                objAnimationY.resume();
                objAnimationZ.resume();
            }
        }

        SequentialAnimation {
            id: objAnimationX
            loops: Animation.Infinite
            running: true
            NumberAnimation {
                target: canvas3d
                property: "xRotAnim"
                from: 0.0
                to: 120.0
                duration: 7000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                target: canvas3d
                property: "xRotAnim"
                from: 120.0
                to: 0.0
                duration: 7000
                easing.type: Easing.InOutQuad
            }
        }

        SequentialAnimation {
            id: objAnimationY
            loops: Animation.Infinite
            running: true
            NumberAnimation {
                target: canvas3d
                property: "yRotAnim"
                from: 0.0
                to: 240.0
                duration: 5000
                easing.type: Easing.InOutCubic
            }
            NumberAnimation {
                target: canvas3d
                property: "yRotAnim"
                from: 240.0
                to: 0.0
                duration: 5000
                easing.type: Easing.InOutCubic
            }
        }

        SequentialAnimation {
            id: objAnimationZ
            loops: Animation.Infinite
            running: true
            NumberAnimation {
                target: canvas3d
                property: "zRotAnim"
                from: -100.0
                to: 100.0
                duration: 3000
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                target: canvas3d
                property: "zRotAnim"
                from: 100.0
                to: -100.0
                duration: 3000
                easing.type: Easing.InOutSine
            }
        }
    }
}
