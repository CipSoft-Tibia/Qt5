// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Dialogs.quickimpl as DialogsQuickImpl

DialogsQuickImpl.FileDialogDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 16
    verticalPadding: 8
    spacing: 16

    icon.width: 16
    icon.height: 16
    icon.color: enabled ? Material.foreground : Material.hintTextColor
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/folder-icon-square.png"

    file: fileUrl

    required property int index
    required property string fileName
    required property url fileUrl
    required property date fileModified

    contentItem: FolderDialogDelegateLabel {
        delegate: control
        fileDetailRowTextColor: control.Material.hintTextColor
    }

    background: Rectangle {
        implicitHeight: control.Material.delegateHeight

        color: control.highlighted ? Color.transparent(control.Material.accentColor, 0.08) : "transparent"

        Ripple {
            width: parent.width
            height: parent.height

            clip: visible
            pressed: control.pressed
            anchor: control
            active: control.down || control.visualFocus || control.hovered
            color: control.highlighted ? control.Material.highlightedRippleColor : control.Material.rippleColor
        }
    }
}
