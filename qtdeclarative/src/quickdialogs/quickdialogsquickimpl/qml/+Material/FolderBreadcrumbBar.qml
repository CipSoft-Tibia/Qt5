// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Dialogs.quickimpl as DialogsQuickImpl

DialogsQuickImpl.FolderBreadcrumbBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + (upButton ? upButton.implicitWidth + upButtonSpacing : 0)
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    upButtonSpacing: 20
    padding: 1

    background: Rectangle {
        color: control.Material.backgroundColor
    }
    contentItem: ListView {
        currentIndex: control.currentIndex
        model: control.contentModel
        orientation: ListView.Horizontal
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 0
        interactive: false
        clip: true
    }
    buttonDelegate: Button {
        id: buttonDelegateRoot
        text: folderName
        flat: true
        font.capitalization: Font.MixedCase

        // The default of 100 is a bit too wide for short directory names.
        Binding {
            target: buttonDelegateRoot.background
            property: "implicitWidth"
            value: control.Material.buttonHeight
        }

        required property int index
        required property string folderName
    }
    separatorDelegate: IconImage {
        id: iconImage
        source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/crumb-separator-icon-square.png"
        sourceSize: Qt.size(8, 8)
        // The image is 8x8, and add 2 px padding on each side.
        width: 8 + 4
        height: control.contentItem.height
        color: control.Material.hintTextColor
        y: (control.height - height) / 2
    }
    upButton: ToolButton {
        x: control.leftPadding
        y: control.topPadding
        icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/up-icon-thick-square.png"
        icon.width: 16
        icon.height: 16
        width: height
        focusPolicy: Qt.TabFocus
    }
    textField: TextField {
        text: (control.dialog as DialogsQuickImpl.FileDialogImpl)?.selectedFile
            ?? (control.dialog as DialogsQuickImpl.FolderDialogImpl).currentFolder
    }
}
