// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtLocation
import QtPositioning
import "include"

Map {
    id: map
    width: 512
    height: 512

    center: QtPositioning.coordinate(0, 180)


    plugin: Plugin {
        name: "osm"
    }
    zoomLevel: 1
    copyrightsVisible: false

    GeometryAntarctica{}
}
