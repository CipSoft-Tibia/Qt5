/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Virtual Keyboard module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Window 2.2
import QtQuick.VirtualKeyboard 2.2
import QtQuick.VirtualKeyboard.Settings 2.2
import "content"

Item {
    width: 1280
    height: 720

    Item {
        id: appContainer
        width: Screen.width < Screen.height ? parent.height : parent.width
        height: Screen.width < Screen.height ? parent.width : parent.height
        anchors.centerIn: parent
        rotation: Screen.width < Screen.height ? 90 : 0
        Basic {
            id: virtualKeyboard
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: inputPanel.top
            handwritingInputPanelActive: handwritingInputPanel.active
        }

        /*  Handwriting input panel for full screen handwriting input.

            This component is an optional add-on for the InputPanel component, that
            is, its use does not affect the operation of the InputPanel component,
            but it also can not be used as a standalone component.

            The handwriting input panel is positioned to cover the entire area of
            application. The panel itself is transparent, but once it is active the
            user can draw handwriting on it.
        */
        HandwritingInputPanel {
            z: 79
            id: handwritingInputPanel
            anchors.fill: parent
            inputPanel: inputPanel
            Rectangle {
                z: -1
                anchors.fill: parent
                color: "black"
                opacity: 0.10
            }
        }

        /*  Container area for the handwriting mode button.

            Handwriting mode button can be moved freely within the container area.
            In this example, a single click changes the handwriting mode and a
            double-click changes the availability of the full screen handwriting input.
        */
        Item {
            z: 99
            visible: handwritingInputPanel.enabled && Qt.inputMethod.visible
            anchors { left: parent.left; top: parent.top; right: parent.right; bottom: inputPanel.top; }
            HandwritingModeButton {
                id: handwritingModeButton
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                floating: true
                flipable: true
                width: 76
                height: width
                state: handwritingInputPanel.state
                onClicked: handwritingInputPanel.active = !handwritingInputPanel.active
                onDoubleClicked: handwritingInputPanel.available = !handwritingInputPanel.available
            }
        }

        /*  Keyboard input panel.

            The keyboard is anchored to the bottom of the application.
        */
        InputPanel {
            id: inputPanel
            z: 89
            y: appContainer.height
            anchors.left: parent.left
            anchors.right: parent.right
            states: State {
                name: "visible"
                /*  The visibility of the InputPanel can be bound to the Qt.inputMethod.visible property,
                    but then the handwriting input panel and the keyboard input panel can be visible
                    at the same time. Here the visibility is bound to InputPanel.active property instead,
                    which allows the handwriting panel to control the visibility when necessary.
                */
                when: inputPanel.active
                PropertyChanges {
                    target: inputPanel
                    y: appContainer.height - inputPanel.height
                }
            }
            transitions: Transition {
                id: inputPanelTransition
                from: ""
                to: "visible"
                reversible: true
                enabled: !VirtualKeyboardSettings.fullScreenMode
                ParallelAnimation {
                    NumberAnimation {
                        properties: "y"
                        duration: 250
                        easing.type: Easing.InOutQuad
                    }
                }
            }
            Binding {
                target: InputContext
                property: "animating"
                value: inputPanelTransition.running
            }
            AutoScroller {}
        }

        Binding {
            target: VirtualKeyboardSettings
            property: "fullScreenMode"
            value: appContainer.height > 0 && (appContainer.width / appContainer.height) > (16.0 / 9.0)
        }
    }
}
