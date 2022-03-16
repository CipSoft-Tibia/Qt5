/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0
import QtLocation 5.10

Item{
    id: page
    x: 0; y: 0;
    width: 100
    height: 100

    Plugin { id: testPlugin; name: "qmlgeo.test.plugin"; allowExperimental: true }
    Map { id: map; anchors.fill: parent }
    SignalSpy { id: supportedMapTypesSpy; target: map; signalName: "supportedMapTypesChanged" }
    SignalSpy { id: activeMapTypeChangedSpy; target: map; signalName: "activeMapTypeChanged" }
    SignalSpy { id: minimumZoomLevelChangedSpy; target: map; signalName: "minimumZoomLevelChanged" }
    SignalSpy { id: maximumZoomLevelChangedSpy; target: map; signalName: "maximumZoomLevelChanged" }
    SignalSpy { id: minimumTiltChangedSpy; target: map; signalName: "minimumTiltChanged" }
    SignalSpy { id: maximumTiltChangedSpy; target: map; signalName: "maximumTiltChanged" }
    SignalSpy { id: minimumFieldOfViewChangedSpy; target: map; signalName: "minimumFieldOfViewChanged" }
    SignalSpy { id: maximumFieldOfViewChangedSpy; target: map; signalName: "maximumFieldOfViewChanged" }

    TestCase {
        id: testCase
        name: "MapType"
        when: windowShown

        function initTestCase()
        {
            compare(map.supportedMapTypes.length, 0)
            compare(map.activeMapType.style, MapType.NoMap)
            map.plugin = testPlugin
            tryCompare(supportedMapTypesSpy, "count", 1)
            compare(map.supportedMapTypes.length, 4)
            compare(map.supportedMapTypes[0].style, MapType.StreetMap)
            compare(map.supportedMapTypes[0].name, "StreetMap")
            compare(map.supportedMapTypes[0].description, "StreetMap")
            compare(map.supportedMapTypes[1].style, MapType.SatelliteMapDay)
            compare(map.supportedMapTypes[1].name, "SatelliteMapDay")
            compare(map.supportedMapTypes[1].description, "SatelliteMapDay")
            compare(map.supportedMapTypes[2].style, MapType.CycleMap)
            compare(map.supportedMapTypes[2].name, "CycleMap")
            compare(map.supportedMapTypes[2].description, "CycleMap")
            compare(map.supportedMapTypes[3].style, MapType.CustomMap)
            compare(map.supportedMapTypes[3].name, "AlternateCameraCapabilities")
            compare(map.supportedMapTypes[3].description, "AlternateCameraCapabilities")
            //default
            compare(map.activeMapType.style, MapType.StreetMap)
        }

        function init()
        {
            map.activeMapType = map.supportedMapTypes[0]

            supportedMapTypesSpy.clear()
            activeMapTypeChangedSpy.clear()
            minimumZoomLevelChangedSpy.clear();
            maximumZoomLevelChangedSpy.clear();
            minimumTiltChangedSpy.clear();
            maximumTiltChangedSpy.clear();
            minimumFieldOfViewChangedSpy.clear();
            maximumFieldOfViewChangedSpy.clear();
        }

        function test_setting_types()
        {
            // resetting it first
            map.activeMapType = map.supportedMapTypes[0]
            tryCompare(activeMapTypeChangedSpy, "count", 0)

            map.activeMapType = map.supportedMapTypes[1]
            tryCompare(activeMapTypeChangedSpy, "count", 1)
            compare(map.supportedMapTypes[1].name, map.activeMapType.name)
            compare(map.supportedMapTypes[1].style, map.activeMapType.style)

            map.activeMapType = map.supportedMapTypes[2]
            tryCompare(activeMapTypeChangedSpy, "count", 2)
            compare(map.supportedMapTypes[2].name, map.activeMapType.name)
            compare(map.supportedMapTypes[2].style, map.activeMapType.style)

            map.activeMapType = map.supportedMapTypes[3]
            tryCompare(activeMapTypeChangedSpy, "count", 3)
            compare(map.supportedMapTypes[3].name, map.activeMapType.name)
            compare(map.supportedMapTypes[3].style, map.activeMapType.style)
        }

        function test_maptype_capabilities()
        {
            map.activeMapType = map.supportedMapTypes[0]

            compare(map.minimumZoomLevel, 0)
            compare(map.maximumZoomLevel, 20)
            compare(map.minimumTilt, 0)
            compare(map.maximumTilt, 60)
            compare(map.minimumFieldOfView, 45)
            compare(map.maximumFieldOfView, 45)

            compare(map.activeMapType.cameraCapabilities.minimumZoomLevel, 0)
            compare(map.activeMapType.cameraCapabilities.maximumZoomLevel, 20)
            compare(map.activeMapType.cameraCapabilities.minimumTilt, 0)
            compare(map.activeMapType.cameraCapabilities.maximumTilt, 60)
            compare(map.activeMapType.cameraCapabilities.minimumFieldOfView, 45)
            compare(map.activeMapType.cameraCapabilities.maximumFieldOfView, 45)

            tryCompare(minimumZoomLevelChangedSpy, "count", 0)
            tryCompare(maximumZoomLevelChangedSpy, "count", 0)
            tryCompare(minimumTiltChangedSpy, "count", 0)
            tryCompare(maximumTiltChangedSpy, "count", 0)
            tryCompare(minimumFieldOfViewChangedSpy, "count", 0)
            tryCompare(maximumFieldOfViewChangedSpy, "count", 0)


            map.activeMapType = map.supportedMapTypes[1]

            compare(map.minimumZoomLevel, 0)
            compare(map.maximumZoomLevel, 20)
            compare(map.minimumTilt, 0)
            compare(map.maximumTilt, 60)
            compare(map.minimumFieldOfView, 45)
            compare(map.maximumFieldOfView, 45)

            compare(map.activeMapType.cameraCapabilities.minimumZoomLevel, 0)
            compare(map.activeMapType.cameraCapabilities.maximumZoomLevel, 20)
            compare(map.activeMapType.cameraCapabilities.minimumTilt, 0)
            compare(map.activeMapType.cameraCapabilities.maximumTilt, 60)
            compare(map.activeMapType.cameraCapabilities.minimumFieldOfView, 45)
            compare(map.activeMapType.cameraCapabilities.maximumFieldOfView, 45)

            tryCompare(minimumZoomLevelChangedSpy, "count", 0)
            tryCompare(maximumZoomLevelChangedSpy, "count", 0)
            tryCompare(minimumTiltChangedSpy, "count", 0)
            tryCompare(maximumTiltChangedSpy, "count", 0)
            tryCompare(minimumFieldOfViewChangedSpy, "count", 0)
            tryCompare(maximumFieldOfViewChangedSpy, "count", 0)


            map.activeMapType = map.supportedMapTypes[3]

            compare(map.minimumZoomLevel, 0)
            compare(map.maximumZoomLevel, 19)
            compare(map.minimumTilt, 0)
            compare(map.maximumTilt, 80)
            compare(map.minimumFieldOfView, 1)
            compare(map.maximumFieldOfView, 179)

            compare(map.activeMapType.cameraCapabilities.minimumZoomLevel, 0)
            compare(map.activeMapType.cameraCapabilities.maximumZoomLevel, 19)
            compare(map.activeMapType.cameraCapabilities.minimumTilt, 0)
            compare(map.activeMapType.cameraCapabilities.maximumTilt, 80)
            compare(map.activeMapType.cameraCapabilities.minimumFieldOfView, 1)
            compare(map.activeMapType.cameraCapabilities.maximumFieldOfView, 179)

            tryCompare(minimumZoomLevelChangedSpy, "count", 0)
            tryCompare(maximumZoomLevelChangedSpy, "count", 1)
            tryCompare(minimumTiltChangedSpy, "count", 0)
            tryCompare(maximumTiltChangedSpy, "count", 1)
            tryCompare(minimumFieldOfViewChangedSpy, "count", 1)
            tryCompare(maximumFieldOfViewChangedSpy, "count", 1)


            map.activeMapType = map.supportedMapTypes[0]

            compare(map.minimumZoomLevel, 0)
            compare(map.maximumZoomLevel, 20)
            compare(map.minimumTilt, 0)
            compare(map.maximumTilt, 60)
            compare(map.minimumFieldOfView, 45)
            compare(map.maximumFieldOfView, 45)

            compare(map.activeMapType.cameraCapabilities.minimumZoomLevel, 0)
            compare(map.activeMapType.cameraCapabilities.maximumZoomLevel, 20)
            compare(map.activeMapType.cameraCapabilities.minimumTilt, 0)
            compare(map.activeMapType.cameraCapabilities.maximumTilt, 60)
            compare(map.activeMapType.cameraCapabilities.minimumFieldOfView, 45)
            compare(map.activeMapType.cameraCapabilities.maximumFieldOfView, 45)

            tryCompare(minimumZoomLevelChangedSpy, "count", 0)
            tryCompare(maximumZoomLevelChangedSpy, "count", 2)
            tryCompare(minimumTiltChangedSpy, "count", 0)
            tryCompare(maximumTiltChangedSpy, "count", 2)
            tryCompare(minimumFieldOfViewChangedSpy, "count", 2)
            tryCompare(maximumFieldOfViewChangedSpy, "count", 2)
        }

        function test_maptype_metadata()
        {
            map.activeMapType = map.supportedMapTypes[0]
            compare(Object.keys(map.activeMapType.metadata).length, 0)

            map.activeMapType = map.supportedMapTypes[3]
            compare(Object.keys(map.activeMapType.metadata).length, 1)
            compare(map.activeMapType.metadata['foo'], 42)
        }
    }
}
