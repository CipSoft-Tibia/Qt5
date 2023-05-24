// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import HelperWidgets
import QtQuick.Layouts

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Camera")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Orthographic")
            tooltip: qsTr("Use orthographic camera")
            Layout.fillWidth: true
        }
        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.orthoProjection
                Layout.fillWidth: true
            }
        }
    }
}
