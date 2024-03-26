// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts
import QtQuick.Templates as T

ColorDialogImpl {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    leftPadding: 6
    rightPadding: 6

    // Ensure that the background's border is visible.
    leftInset: -1
    rightInset: -1
    topInset: -1
    bottomInset: -1

    standardButtons: T.Dialog.Ok | T.Dialog.Cancel

    isHsl: true

    ColorDialogImpl.eyeDropperButton: eyeDropperButton
    ColorDialogImpl.buttonBox: buttonBox
    ColorDialogImpl.colorPicker: colorPicker
    ColorDialogImpl.colorInputs: inputs
    ColorDialogImpl.alphaSlider: alphaSlider

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 600
        color: control.palette.window
        border.color: control.palette.dark
    }

    header: Pane {
        palette.window: control.palette.light
        padding: 20

        contentItem: RowLayout {
            Label {
                objectName: "titleLabel"
                text: control.title
                elide: Label.ElideRight
                font.bold: true

                Layout.preferredWidth: control.title.length > 0 ? implicitWidth : 0
                Layout.leftMargin: 12
                Layout.alignment: Qt.AlignLeft
            }
            Button {
                id: eyeDropperButton
                objectName: "eyeDropperButton"
                icon.source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/eye-dropper.png"
                flat: true
                visible: false

                Layout.preferredWidth: implicitHeight
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 6
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 12
        SaturationLightnessPicker {
            id: colorPicker
            objectName: "colorPicker"
            color: control.color

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Slider {
            id: hueSlider
            objectName: "hueSlider"
            orientation: Qt.Horizontal
            value: control.hue
            implicitHeight: 20
            onMoved: function() { control.hue = value; }
            handle: PickerHandle {
                x: hueSlider.leftPadding + (hueSlider.horizontal
                    ? hueSlider.visualPosition * (hueSlider.availableWidth - width)
                    : (hueSlider.availableWidth - width) / 2)
                y: hueSlider.topPadding + (hueSlider.horizontal
                    ? (hueSlider.availableHeight - height) / 2
                    : hueSlider.visualPosition * (hueSlider.availableHeight - height))
                picker: hueSlider
            }
            background: Rectangle {
                anchors.fill: parent
                anchors.leftMargin: hueSlider.handle.width / 2
                anchors.rightMargin: hueSlider.handle.width / 2
                border.width: 2
                border.color: control.palette.dark
                radius: 10
                color: "transparent"
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 10
                    gradient: HueGradient {
                        orientation: Gradient.Horizontal
                    }
                }
            }

            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
        }

        Slider {
            id: alphaSlider
            objectName: "alphaSlider"
            orientation: Qt.Horizontal
            value: control.alpha
            implicitHeight: 20
            handle: PickerHandle {
                x: alphaSlider.leftPadding + (alphaSlider.horizontal
                                              ? alphaSlider.visualPosition * (alphaSlider.availableWidth - width)
                                              : (alphaSlider.availableWidth - width) / 2)
                y: alphaSlider.topPadding + (alphaSlider.horizontal
                                             ? (alphaSlider.availableHeight - height) / 2
                                             : alphaSlider.visualPosition * (alphaSlider.availableHeight - height))
                picker: alphaSlider
            }
            background: Rectangle {
                anchors.fill: parent
                anchors.leftMargin: parent.handle.width / 2
                anchors.rightMargin: parent.handle.width / 2
                border.width: 2
                border.color: control.palette.dark
                radius: 10
                color: "transparent"

                Image {
                    anchors.fill: alphaSliderGradient
                    source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/checkers.png"
                    fillMode: Image.Tile
                }

                Rectangle {
                    id: alphaSliderGradient
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 10
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop {
                            position: 0
                            color: "transparent"
                        }
                        GradientStop {
                            position: 1
                            color: Qt.rgba(control.color.r,
                                           control.color.g,
                                           control.color.b,
                                           1)
                        }
                    }
                }
            }

            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
        }

        ColorInputs {
            id: inputs

            color: control.color

            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 12
        }
    }

    footer: Rectangle {
        color: control.palette.light
        implicitWidth: rowLayout.implicitWidth
        implicitHeight: rowLayout.implicitHeight

        RowLayout {
            id: rowLayout
            width: parent.width
            height: parent.height
            spacing: 20

            Label {
                text: qsTr("Color")

                Layout.leftMargin: 20
            }

            Rectangle {
                implicitWidth: 32
                implicitHeight: 32
                border.width: 2
                border.color: control.palette.dark
                color: "transparent"

                Image {
                    anchors.fill: parent
                    anchors.margins: 4
                    source: "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/checkers.png"
                    fillMode: Image.Tile
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    color: control.color
                }
            }

            Item {
                // empty space filler
                Layout.fillWidth: true
            }

            DialogButtonBox {
                id: buttonBox
                standardButtons: control.standardButtons
                palette.window: control.palette.light
                spacing: 12
                horizontalPadding: 0
                verticalPadding: 20

                Layout.rightMargin: 20
            }
        }
    }

    Overlay.modal: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.5)
    }

    Overlay.modeless: Rectangle {
        color: Color.transparent(control.palette.shadow, 0.12)
    }
}
