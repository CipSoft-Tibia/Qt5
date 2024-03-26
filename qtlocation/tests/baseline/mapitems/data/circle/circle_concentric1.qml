// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtLocation
import QtPositioning

Map {
    width: 512
    height: 512

    center: QtPositioning.coordinate(0, 20)

    id: map
    plugin: Plugin {
        name: "itemsoverlay"
    }
    zoomLevel: 1
    copyrightsVisible: false

    Repeater {
        id: c
        property var colors: [
            "#30ff0000",
            "#30ffa500",
            "#30ffff00",
            "#3000ff00",
            "#300000ff",
            "#30ee82ee"
        ]
        model: 20
        MapCircle
        {
            center
            {
                longitude: 20
                latitude: 0
            }
            radius: ((c.count-index-1)/(c.count-1)*(6371-1000)+500)*Math.PI*1000
            color: c.colors[Math.floor(index%c.colors.length)]
            border.width: 1
            autoFadeIn: false
            opacity: 1
        }
    }
}
