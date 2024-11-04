// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: mainView

    required property Loader loader

    readonly property real listItemWidth: 280
    readonly property real listItemHeight: 40

    anchors.fill: parent

    ListModel {
        id: testsModel
        ListElement {
            name: "Bar Themes"
            file: "BarThemes.qml"
        }
        ListElement {
            name: "Axis & Grid Setup"
            file: "AxisGridSetup.qml"
        }
        ListElement {
            name: "Bar Selection"
            file: "BarSelection.qml"
        }
        ListElement {
            name: "C++ Bar Series"
            file: "CppBarSeries.qml"
        }
        ListElement {
            name: "C++ Line Series"
            file: "CppLineSeries.qml"
        }
        ListElement {
            name: "Line Properties"
            file: "LineProperties.qml"
        }
        ListElement {
            name: "Scatter Properties"
            file: "ScatterProperties.qml"
        }
        ListElement {
            name: "Callout"
            file: "Callout.qml"
        }
    }

    Component {
        id: listComponent
        Button {
            id: button
            required property string name
            required property string file

            width: mainView.listItemWidth
            height: mainView.listItemHeight
            background: Rectangle {
                id: buttonBackground
                border.width: 0.5
                border.color: "#d0808080"
                color: "#d0404040"
                opacity: button.hovered ? 1.0 : 0.5
            }
            contentItem: Text {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#f0f0f0"
                font.pointSize: settings.fontSizeSmall
                text: button.name
            }

            onClicked: {
                mainView.loader.source = button.file
            }
        }
    }

    Text {
        id: topLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        text: qsTr("QtGraphs - Testbed")
        color: "#f0f0f0"
        font.pointSize: settings.fontSizeLarge
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: topLabel.bottom
        anchors.topMargin: 20
        spacing: 20
        ListView {
            id: examplesListView
            width: mainView.listItemWidth
            height: count * mainView.listItemHeight
            model: testsModel
            delegate: listComponent
        }
    }
}
