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

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Universal 2.12
import Qt.labs.settings 1.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Quick Controls 2"

    function help() {
        let displayingControl = listView.currentIndex !== -1
        let currentControlName = displayingControl
            ? listView.model.get(listView.currentIndex).title.toLowerCase() : ""
        let url = "https://doc.qt.io/qt-5/"
            + (displayingControl
               ? "qml-qtquick-controls2-" + currentControlName + ".html"
               : "qtquick-controls2-qmlmodule.html");
        Qt.openUrlExternally(url)
    }

    Settings {
        id: settings
        property string style: "Default"
    }

    Shortcut {
        sequences: ["Esc", "Back"]
        enabled: stackView.depth > 1
        onActivated: navigateBackAction.trigger()
    }

    Shortcut {
        sequence: StandardKey.HelpContents
        onActivated: help()
    }

    Action {
        id: navigateBackAction
        icon.name: stackView.depth > 1 ? "back" : "drawer"
        onTriggered: {
            if (stackView.depth > 1) {
                stackView.pop()
                listView.currentIndex = -1
            } else {
                drawer.open()
            }
        }
    }

    Shortcut {
        sequence: "Menu"
        onActivated: optionsMenuAction.trigger()
    }

    Action {
        id: optionsMenuAction
        icon.name: "menu"
        onTriggered: optionsMenu.open()
    }

    header: ToolBar {
        Material.foreground: "white"

        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                action: navigateBackAction
            }

            Label {
                id: titleLabel
                text: listView.currentItem ? listView.currentItem.text : "Gallery"
                font.pixelSize: 20
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                action: optionsMenuAction

                Menu {
                    id: optionsMenu
                    x: parent.width - width
                    transformOrigin: Menu.TopRight

                    Action {
                        text: "Settings"
                        onTriggered: settingsDialog.open()
                    }
                    Action {
                        text: "Help"
                        onTriggered: help()
                    }
                    Action {
                        text: "About"
                        onTriggered: aboutDialog.open()
                    }
                }
            }
        }
    }

    Drawer {
        id: drawer
        width: Math.min(window.width, window.height) / 3 * 2
        height: window.height
        interactive: stackView.depth === 1

        ListView {
            id: listView

            focus: true
            currentIndex: -1
            anchors.fill: parent

            delegate: ItemDelegate {
                width: listView.width
                text: model.title
                highlighted: ListView.isCurrentItem
                onClicked: {
                    listView.currentIndex = index
                    stackView.push(model.source)
                    drawer.close()
                }
            }

            model: ListModel {
                ListElement { title: "BusyIndicator"; source: "qrc:/pages/BusyIndicatorPage.qml" }
                ListElement { title: "Button"; source: "qrc:/pages/ButtonPage.qml" }
                ListElement { title: "CheckBox"; source: "qrc:/pages/CheckBoxPage.qml" }
                ListElement { title: "ComboBox"; source: "qrc:/pages/ComboBoxPage.qml" }
                ListElement { title: "DelayButton"; source: "qrc:/pages/DelayButtonPage.qml" }
                ListElement { title: "Dial"; source: "qrc:/pages/DialPage.qml" }
                ListElement { title: "Dialog"; source: "qrc:/pages/DialogPage.qml" }
                ListElement { title: "Delegates"; source: "qrc:/pages/DelegatePage.qml" }
                ListElement { title: "Frame"; source: "qrc:/pages/FramePage.qml" }
                ListElement { title: "GroupBox"; source: "qrc:/pages/GroupBoxPage.qml" }
                ListElement { title: "PageIndicator"; source: "qrc:/pages/PageIndicatorPage.qml" }
                ListElement { title: "ProgressBar"; source: "qrc:/pages/ProgressBarPage.qml" }
                ListElement { title: "RadioButton"; source: "qrc:/pages/RadioButtonPage.qml" }
                ListElement { title: "RangeSlider"; source: "qrc:/pages/RangeSliderPage.qml" }
                ListElement { title: "ScrollBar"; source: "qrc:/pages/ScrollBarPage.qml" }
                ListElement { title: "ScrollIndicator"; source: "qrc:/pages/ScrollIndicatorPage.qml" }
                ListElement { title: "Slider"; source: "qrc:/pages/SliderPage.qml" }
                ListElement { title: "SpinBox"; source: "qrc:/pages/SpinBoxPage.qml" }
                ListElement { title: "StackView"; source: "qrc:/pages/StackViewPage.qml" }
                ListElement { title: "SwipeView"; source: "qrc:/pages/SwipeViewPage.qml" }
                ListElement { title: "Switch"; source: "qrc:/pages/SwitchPage.qml" }
                ListElement { title: "TabBar"; source: "qrc:/pages/TabBarPage.qml" }
                ListElement { title: "TextArea"; source: "qrc:/pages/TextAreaPage.qml" }
                ListElement { title: "TextField"; source: "qrc:/pages/TextFieldPage.qml" }
                ListElement { title: "ToolTip"; source: "qrc:/pages/ToolTipPage.qml" }
                ListElement { title: "Tumbler"; source: "qrc:/pages/TumblerPage.qml" }
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: Pane {
            id: pane

            Image {
                id: logo
                width: pane.availableWidth / 2
                height: pane.availableHeight / 2
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -50
                fillMode: Image.PreserveAspectFit
                source: "images/qt-logo.png"
            }

            Label {
                text: "Qt Quick Controls 2 provides a set of controls that can be used to build complete interfaces in Qt Quick."
                anchors.margins: 20
                anchors.top: logo.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: arrow.top
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter
                wrapMode: Label.Wrap
            }

            Image {
                id: arrow
                source: "images/arrow.png"
                anchors.left: parent.left
                anchors.bottom: parent.bottom
            }
        }
    }

    Dialog {
        id: settingsDialog
        x: Math.round((window.width - width) / 2)
        y: Math.round(window.height / 6)
        width: Math.round(Math.min(window.width, window.height) / 3 * 2)
        modal: true
        focus: true
        title: "Settings"

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            settings.style = styleBox.displayText
            settingsDialog.close()
        }
        onRejected: {
            styleBox.currentIndex = styleBox.styleIndex
            settingsDialog.close()
        }

        contentItem: ColumnLayout {
            id: settingsColumn
            spacing: 20

            RowLayout {
                spacing: 10

                Label {
                    text: "Style:"
                }

                ComboBox {
                    id: styleBox
                    property int styleIndex: -1
                    model: availableStyles
                    Component.onCompleted: {
                        styleIndex = find(settings.style, Qt.MatchFixedString)
                        if (styleIndex !== -1)
                            currentIndex = styleIndex
                    }
                    Layout.fillWidth: true
                }
            }

            Label {
                text: "Restart required"
                color: "#e41e25"
                opacity: styleBox.currentIndex !== styleBox.styleIndex ? 1.0 : 0.0
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    Dialog {
        id: aboutDialog
        modal: true
        focus: true
        title: "About"
        x: (window.width - width) / 2
        y: window.height / 6
        width: Math.min(window.width, window.height) / 3 * 2
        contentHeight: aboutColumn.height

        Column {
            id: aboutColumn
            spacing: 20

            Label {
                width: aboutDialog.availableWidth
                text: "The Qt Quick Controls 2 module delivers the next generation user interface controls based on Qt Quick."
                wrapMode: Label.Wrap
                font.pixelSize: 12
            }

            Label {
                width: aboutDialog.availableWidth
                text: "In comparison to the desktop-oriented Qt Quick Controls 1, Qt Quick Controls 2 "
                    + "are an order of magnitude simpler, lighter and faster, and are primarily targeted "
                    + "towards embedded and mobile platforms."
                wrapMode: Label.Wrap
                font.pixelSize: 12
            }
        }
    }
}
