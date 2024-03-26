// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

import "." as DialogsImpl

FileDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    leftPadding: 24
    rightPadding: 24

    standardButtons: T.Dialog.Open | T.Dialog.Cancel

    Material.elevation: 24

    Dialog {
        id: overwriteConfirmationDialog
        objectName: "confirmationDialog"
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        dim: true
        modal: true
        title: qsTr("“%1” already exists. Do you want to replace it?").arg(control.fileName)

        Label {
            text: qsTr("A file with the same name already exists in %1.\nReplacing it will overwrite its current contents.").arg(control.currentFolderName)
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignHCenter
            standardButtons: DialogButtonBox.Yes | DialogButtonBox.No
        }
    }

    FileDialogImpl.buttonBox: buttonBox
    FileDialogImpl.nameFiltersComboBox: nameFiltersComboBox
    FileDialogImpl.fileDialogListView: fileDialogListView
    FileDialogImpl.breadcrumbBar: breadcrumbBar
    FileDialogImpl.fileNameLabel: fileNameLabel
    FileDialogImpl.fileNameTextField: fileNameTextField
    FileDialogImpl.overwriteConfirmationDialog: overwriteConfirmationDialog

    background: Rectangle {
        implicitWidth: 600
        implicitHeight: 400
        radius: 2
        color: control.Material.dialogColor

        layer.enabled: control.Material.elevation > 0
        layer.effect: ElevationEffect {
            elevation: control.Material.elevation
        }
    }

    header: ColumnLayout {
        spacing: 12

        Label {
            text: control.title
            visible: control.title.length > 0
            elide: Label.ElideRight
            font.bold: true
            font.pixelSize: 16

            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.topMargin: 24
            Layout.fillWidth: true
        }

        DialogsImpl.FolderBreadcrumbBar {
            id: breadcrumbBar
            dialog: control

            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width - 48
        }
    }

    contentItem: ListView {
        id: fileDialogListView
        objectName: "fileDialogListView"
        clip: true

        ScrollBar.vertical: ScrollBar {}

        model: FolderListModel {
            folder: control.currentFolder
            nameFilters: control.selectedNameFilter.globs
            showDirsFirst: PlatformTheme.themeHint(PlatformTheme.ShowDirectoriesFirst)
            sortCaseSensitive: false
        }
        delegate: DialogsImpl.FileDialogDelegate {
            objectName: "fileDialogDelegate" + index
            width: ListView.view.width
            highlighted: ListView.isCurrentItem
            dialog: control
            fileDetailRowWidth: nameFiltersComboBox.width

            KeyNavigation.backtab: breadcrumbBar
            KeyNavigation.tab: fileNameTextField.visible ? fileNameTextField : nameFiltersComboBox
        }
    }

    footer: GridLayout {
        columnSpacing: 20
        columns: 3

        Label {
            id: fileNameLabel
            text: qsTr("File name")
            visible: false

            Layout.topMargin: 12
            Layout.leftMargin: 20
        }

        TextField {
            id: fileNameTextField
            objectName: "fileNameTextField"
            visible: false

            Layout.topMargin: 12
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Filter")

            Layout.row: 1
            Layout.topMargin: fileNameTextField.visible ? 0 : 12
            Layout.leftMargin: 20
        }

        ComboBox {
            id: nameFiltersComboBox
            model: control.nameFilters
            flat: true

            verticalPadding: 0
            topInset: 0
            bottomInset: 0
            Layout.topMargin: fileNameTextField.visible ? 0 : 12
            Layout.fillWidth: true
        }

        DialogButtonBox {
            id: buttonBox
            standardButtons: control.standardButtons
            spacing: 12
            padding: 0
            topInset: 0
            bottomInset: 0

            Layout.row: 1
            Layout.column: 2
            Layout.topMargin: fileNameTextField.visible ? 0 : 12
            Layout.rightMargin: 20
        }
    }

    Overlay.modal: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.5)
    }

    Overlay.modeless: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.12)
    }
}
