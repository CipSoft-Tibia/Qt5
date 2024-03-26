// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import HelperWidgets
import QtQuick.Layouts

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("TabBar")

        SectionLayout {
            Label {
                text: qsTr("Position")
                tooltip: qsTr("Position of the tabbar.")
            }
            SecondColumnLayout {
                ComboBox {
                    backendValue: backendValues.position
                    model: [ "Header", "Footer" ]
                    scope: "TabBar"
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Content Width")
                tooltip: qsTr("Content height used for calculating the total implicit width.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.contentWidth
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Content Height")
                tooltip: qsTr("Content height used for calculating the total implicit height.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.contentHeight
                    Layout.fillWidth: true
                }
            }
        }
    }

    ContainerSection {
        width: parent.width
    }

    ControlSection {
        width: parent.width
    }

    FontSection {
        width: parent.width
    }

    PaddingSection {
        width: parent.width
    }
}
