// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Model {
    id: root
    source: "#Rectangle"
    pickable: true
    castsShadows: false

    property string labelText: ""
    property color backgroundColor: "gray"
    property bool backgroundEnabled: false
    property color labelTextColor: "transparent"
    property bool borderEnabled : false
    property font labelFont
    property real labelWidth: -1
    property real labelHeight: -1

    materials: PrincipledMaterial {
        lighting: PrincipledMaterial.NoLighting
        baseColorMap: Texture {
            sourceItem: Item {
                id: labelItem
                width: root.labelWidth
                height: root.labelHeight

                Rectangle {
                    id: labelBackground
                    anchors.fill: parent
                    color: root.backgroundColor
                    visible: root.backgroundEnabled
                    border.color: root.labelTextColor
                    border.width: root.borderEnabled
                                  ? Math.max(0.5, (text0.font.pointSize / 16)) : 0
                    radius: 4
                }

                Text {
                    id: text0
                    anchors.centerIn: parent
                    color: root.labelTextColor
                    text: root.labelText
                    font: root.labelFont
                }
            }
        }
    }
}
