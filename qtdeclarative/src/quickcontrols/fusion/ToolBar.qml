// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

T.ToolBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    horizontalPadding: 6
    topPadding: control.position === T.ToolBar.Footer ? 1 : 0
    bottomPadding: control.position === T.ToolBar.Header ? 1 : 0

    background: Rectangle {
        implicitHeight: 26

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.lighter(control.palette.window, 1.04)
            }
            GradientStop {
                position: 1
                color: control.palette.window
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: control.position === T.ToolBar.Header ? Fusion.lightShade : Fusion.darkShade
        }

        Rectangle {
            y: parent.height - height
            width: parent.width
            height: 1
            color: control.position === T.ToolBar.Header ? Fusion.darkShade : Fusion.lightShade
        }
    }
}
