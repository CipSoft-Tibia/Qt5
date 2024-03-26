// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtTest
import QtLocation
import QtPositioning

Item {
    id: root
    function cloneArray(a) {
        var i = a.length
        var arr = new Array(i)
        while (i--) arr[i] = a[i];
        return arr
    }
    Plugin { id: testPlugin; name: "qmlgeo.test.plugin"; allowExperimental: true }
    Plugin { id: errorPlugin; name: "qmlgeo.test.plugin"; allowExperimental: true
        parameters: [
            PluginParameter { name: "error"; value: "1"},
            PluginParameter { name: "errorString"; value: "This error was expected. No worries !"}
        ]
    }

    property variant coordinate1: QtPositioning.coordinate(51, 0)
    property variant coordinate2: QtPositioning.coordinate(52, 0)

    property variant boundingBox1: QtPositioning.rectangle()
    property variant boundingBox2: QtPositioning.rectangle()

    property variant circle1: QtPositioning.circle()
    property variant circle2: QtPositioning.circle(tr, 4000)

    Component.onCompleted: {
        boundingBox1.topLeft = bl
        boundingBox1.bottomRight = bl
        boundingBox1.width = 10

        boundingBox2.topLeft = bl
        boundingBox2.bottomRight = bl
        boundingBox2.width = 20
    }

    property variant bl: QtPositioning.coordinate(0, 0)
    property variant tl: QtPositioning.coordinate(1, 0)
    property variant tr: QtPositioning.coordinate(1, 1)
    property variant br: QtPositioning.coordinate(0, 1)
    property variant ntr: QtPositioning.coordinate(3, 3)

    property variant unitBox: QtPositioning.rectangle(tl, br)
    property routeSegment emptySegment

    property route emptyRoute
    TestCase {
        name: "RouteManeuver RouteSegment and MapRoute"

        // TODO enable when we have map route
        //MapRoute {id: emptyMapRoute}

        property variant emptyBox: QtPositioning.rectangle()

        property variant emptyCoordinate: QtPositioning.coordinate()

        // TODO enable when we have map route
        /*
        SignalSpy {id: mapRouteDetailLevelSpy; target: emptyMapRoute; signalName: "detailLevelChanged"}
        SignalSpy {id: mapRouteColorSpy; target: emptyMapRoute.border; signalName: "colorChanged"}
        SignalSpy {id: mapRouteWidthSpy; target: emptyMapRoute.border; signalName: "widthChanged"}
        SignalSpy {id: mapRouteRouteSpy; target: emptyMapRoute; signalName: "routeChanged"}
        function test_maproute_defaults() {
            compare(mapRouteRouteSpy.count, 0)
            compare(mapRouteColorSpy.count, 0)
            compare(mapRouteDetailLevelSpy.count, 0)
            compare (emptyMapRoute.detailLevel, 6)
            emptyMapRoute.border.color = 'green'
            emptyMapRoute.detailLevel = 3
            compare(mapRouteRouteSpy.count, 0)
            compare(mapRouteColorSpy.count, 1)
            compare(mapRouteDetailLevelSpy.count, 1)
            emptyMapRoute.border.color = 'green'
            emptyMapRoute.detailLevel = 3
            compare(mapRouteColorSpy.count, 1)
            compare(mapRouteDetailLevelSpy.count, 1)
            emptyMapRoute.route = emptyRoute
            compare(mapRouteRouteSpy.count, 1)
            compare(emptyMapRoute.route, emptyRoute)
            // width
            compare(mapRouteWidthSpy.count, 0)
            emptyMapRoute.border.width = 123
            compare(mapRouteWidthSpy.count, 1)
            compare(emptyMapRoute.border.width, 123)
            emptyMapRoute.border.width = 123
            compare(mapRouteWidthSpy.count, 1)
            emptyMapRoute.border.width = -1
            compare(mapRouteWidthSpy.count, 1)
            compare(emptyMapRoute.border.width, 123)
            emptyMapRoute.border.width = 0
            compare(mapRouteWidthSpy.count, 1)
            compare(emptyMapRoute.border.width, 123)
        }
        */

        function test_route_defaults() {
            compare(emptyRoute.travelTime, 0)
            compare(emptyRoute.distance,0)
            compare(emptyRoute.path.length,0)
            compare(emptyRoute.segments.length,0)
            compare(emptyRoute.bounds.topLeft.latitude, emptyBox.topLeft.latitude)
            compare(emptyRoute.bounds.bottomRight.longitude, emptyBox.bottomRight.longitude)
        }

        function test_routesegment_defaults() {
            let emptyManeuver = emptySegment.maneuver
            compare(emptySegment.travelTime, 0)
            compare(emptySegment.distance, 0)
            compare(emptySegment.path.length, 0)
            compare(emptySegment.maneuver.valid, emptyManeuver.valid)
            compare(emptySegment.maneuver.instructionText, emptyManeuver.instructionText)
            compare(emptySegment.maneuver.waypoint.valid, emptyManeuver.waypoint.valid)
        }
        function test_maneuver_defaults() {
            let emptyManeuver = emptySegment.maneuver
            compare(emptyManeuver.valid, false)
            compare(emptyManeuver.instructionText, "")
            compare(emptyManeuver.direction, RouteManeuver.NoDirection)
            compare(emptyManeuver.timeToNextInstruction,0)
            compare(emptyManeuver.distanceToNextInstruction,0)
            compare(emptyManeuver.waypoint.latitude, emptyCoordinate.latitude)
            compare(emptyManeuver.waypoint.longitude, emptyCoordinate.longitude)
            compare(emptyManeuver.position.latitude, emptyCoordinate.latitude)
            compare(emptyManeuver.position.longitude, emptyCoordinate.longitude)
        }
    }

    TestCase {
        name: "MapRouteModel and MapRouteQuery"
        RouteModel {id: emptyModel}
        RouteQuery {id: emptyQuery}

        function test_model_default_properties() {
            compare (emptyModel.autoUpdate, false, "Automatic update")
            compare (emptyModel.status, RouteModel.Null, "Model status")
            compare (emptyModel.errorString, "", "Model error")
            compare (emptyModel.error, RouteModel.NoError)
            compare (emptyModel.count, 0, "Model count")
            emptyModel.get(192) // don't do stupid

            compare (emptyQuery.numberAlternativeRoutes, 0, "Number of alternative routes")
            compare (emptyQuery.travelModes, RouteQuery.CarTravel, "Travel mode")
            compare (emptyQuery.routeOptimizations, RouteQuery.FastestRoute, "Route optimization")
            compare (emptyQuery.segmentDetail, RouteQuery.BasicSegmentData)
            compare (emptyQuery.maneuverDetail, RouteQuery.BasicManeuvers)
            compare (emptyQuery.waypoints.length, 0, "Waypoints")
            compare (emptyQuery.excludedAreas.length, 0, "excluded areas")
            compare (emptyQuery.featureTypes.length, 0, "Feature types")
        }

        RouteQuery {
            id: withExcludedAreas
            excludedAreas: [
                {
                    bottomLeft: { latitude: 22.5, longitude: 22.5},
                    topRight: { latitude: 23.5, longitude: 23.5}
                },
                {
                    topLeft: { latitude: 23.5, longitude: 22.5},
                    bottomRight: { latitude: 22.5, longitude: 23.5},
                },
                {
                    center: { latitude: 0.0, longitude: 0.0},
                    width: 1.0,
                    height: 1.0
                }
            ]
        }
        function test_initialized_model() {
            compare(withExcludedAreas.excludedAreas.length, 3)
            compare(withExcludedAreas.excludedAreas[0].width, 1.0)
            compare(withExcludedAreas.excludedAreas[1], withExcludedAreas.excludedAreas[0])
            let topLeft = QtPositioning.coordinate(0.5, -0.5)
            compare(withExcludedAreas.excludedAreas[2].topLeft, topLeft)
        }

        SignalSpy {id: autoUpdateSpy; target: emptyModel; signalName: "autoUpdateChanged"}
        SignalSpy {id: pluginSpy; target: emptyModel ; signalName: "pluginChanged"}

        SignalSpy {id: travelModesSpy; target: emptyQuery; signalName: "travelModesChanged"}
        SignalSpy {id: waypointsSpy; target: emptyQuery; signalName: "waypointsChanged"}
        SignalSpy {id: exclusionSpy; target: emptyQuery; signalName: "excludedAreasChanged"}
        SignalSpy {id: featureTypesSpy; target: emptyQuery; signalName: "featureTypesChanged"}
        SignalSpy {id: segmentDetailSpy; target: emptyQuery; signalName: "segmentDetailChanged"}
        SignalSpy {id: maneuverDetailSpy; target: emptyQuery; signalName: "maneuverDetailChanged"}
        SignalSpy {id: numberAlterNativeRoutesSpy; target: emptyQuery; signalName: "numberAlternativeRoutesChanged"}
        SignalSpy {id: routeOptimizationsSpy; target: emptyQuery; signalName: "routeOptimizationsChanged"}
        SignalSpy {id: queryDetailsChangedSpy; target: emptyQuery; signalName: "queryDetailsChanged"}
        function test_model_setters() {
            // Autoupdate
            compare(autoUpdateSpy.count, 0)
            emptyModel.autoUpdate = true
            compare(autoUpdateSpy.count, 1)
            compare(emptyModel.autoUpdate, true)
            emptyModel.autoUpdate = true // mustn't retrigger 'changed' -signal
            compare(autoUpdateSpy.count, 1)
            emptyModel.autoUpdate = false
            compare(autoUpdateSpy.count, 2)

            // Travelmodes
            compare(travelModesSpy.count, 0)
            emptyQuery.travelModes = RouteQuery.BicycleTravel
            compare(travelModesSpy.count, 1)
            compare(emptyQuery.travelModes, RouteQuery.BicycleTravel)
            emptyQuery.travelModes = RouteQuery.BicycleTravel | RouteQuery.PedestrianTravel
            compare(emptyQuery.travelModes, RouteQuery.BicycleTravel | RouteQuery.PedestrianTravel)
            compare(travelModesSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)

            // Basic adding and removing of waypoint
            queryDetailsChangedSpy.clear()
            compare(waypointsSpy.count, 0)
            emptyQuery.addWaypoint(coordinate1)
            compare(waypointsSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            emptyQuery.addWaypoint(coordinate1)
            compare(waypointsSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)
            compare(emptyQuery.waypoints.length, 2)
            emptyQuery.removeWaypoint(coordinate1)
            compare(waypointsSpy.count, 3)
            compare(queryDetailsChangedSpy.count, 3)
            compare(emptyQuery.waypoints.length, 1)
            emptyQuery.removeWaypoint(coordinate2) // coordinate2 isn't in the list, must not impact
            compare(waypointsSpy.count, 3)
            compare(queryDetailsChangedSpy.count, 3)
            emptyQuery.removeWaypoint(coordinate1)
            compare(waypointsSpy.count, 4)
            emptyQuery.removeWaypoint(coordinate1) // doesn't exist anymore, must not impact
            compare(waypointsSpy.count, 4)
            compare(emptyQuery.waypoints.length, 0)
            // Check correct ordering of waypoints
            waypointsSpy.clear()
            emptyQuery.addWaypoint(coordinate1)
            emptyQuery.addWaypoint(coordinate2)
            emptyQuery.addWaypoint(coordinate1)
            emptyQuery.addWaypoint(coordinate2)
            compare(waypointsSpy.count, 4)
            compare(emptyQuery.waypoints[0], coordinate1)
            compare(emptyQuery.waypoints[1], coordinate2)
            compare(emptyQuery.waypoints[2], coordinate1)
            compare(emptyQuery.waypoints[3], coordinate2)
            emptyQuery.removeWaypoint(coordinate1) // remove one from the middle, check that one added last is removed
            compare(emptyQuery.waypoints.length, 3)
            compare(emptyQuery.waypoints[0], coordinate1)
            compare(emptyQuery.waypoints[1], coordinate2)
            compare(emptyQuery.waypoints[2], coordinate2)
            waypointsSpy.clear()
            emptyQuery.clearWaypoints()
            compare(emptyQuery.waypoints.length, 0)
            compare(waypointsSpy.count, 1)

            // Altering the waypoint contents should trigger signal
            emptyQuery.clearWaypoints()
            queryDetailsChangedSpy.clear();
            emptyQuery.addWaypoint(coordinate1)
            compare(queryDetailsChangedSpy.count, 1);

            // verify coordinate is disconnected
            emptyQuery.removeWaypoint(coordinate1)
            compare (queryDetailsChangedSpy.count, 2)

            // verify that the same coordinate can be added to the waypoints
            emptyQuery.addWaypoint(coordinate1)
            compare(queryDetailsChangedSpy.count, 3);
            emptyQuery.addWaypoint(coordinate1)
            compare(queryDetailsChangedSpy.count, 4);
            compare (emptyQuery.waypoints.length, 2)
            queryDetailsChangedSpy.clear()

            // verify that removing duplicate coordinate leaves remaining ones
            emptyQuery.removeWaypoint(coordinate1)
            compare (queryDetailsChangedSpy.count, 1)
            compare (emptyQuery.waypoints.length, 1)

            // verify that clearing works
            emptyQuery.clearWaypoints()
            compare(queryDetailsChangedSpy.count, 2);
            compare (emptyQuery.waypoints.length, 0)

            // Excluded areas
            queryDetailsChangedSpy.clear()
            compare(exclusionSpy.count, 0)
            emptyQuery.addExcludedArea(boundingBox1)
            compare(exclusionSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            emptyQuery.addExcludedArea(boundingBox1)
            // doesn't make sense to put same area twice
            compare(exclusionSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.excludedAreas.length, 1)
            emptyQuery.removeExcludedArea(boundingBox1)
            compare(exclusionSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)
            compare(emptyQuery.excludedAreas.length, 0)
            emptyQuery.removeExcludedArea(boundingBox2) // boundingBox2 isn't in the list, must not impact
            compare(exclusionSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)
            emptyQuery.removeExcludedArea(boundingBox1) // doesn't exist anymore, must not impact
            compare(exclusionSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)
            // Check correct ordering of exclusion
            exclusionSpy.clear()
            emptyQuery.addExcludedArea(boundingBox1)
            emptyQuery.addExcludedArea(boundingBox2)
            emptyQuery.addExcludedArea(boundingBox1)
            emptyQuery.addExcludedArea(boundingBox2)
            compare(exclusionSpy.count, 2)
            compare(emptyQuery.excludedAreas[0], boundingBox1)
            compare(emptyQuery.excludedAreas[1], boundingBox2)
            emptyQuery.removeExcludedArea(boundingBox1) // remove first and check all geos ok
            compare(emptyQuery.excludedAreas[0], boundingBox2)
            exclusionSpy.clear()
            emptyQuery.clearExcludedAreas()
            compare(emptyQuery.excludedAreas.length, 0)
            compare(exclusionSpy.count, 1)

            // verify that clearing works
            emptyQuery.addExcludedArea(unitBox);
            compare(emptyQuery.excludedAreas.length, 1);
            queryDetailsChangedSpy.clear();
            emptyQuery.clearExcludedAreas();
            compare(queryDetailsChangedSpy.count, 1);
            compare(emptyQuery.excludedAreas.length, 0)

            // Feature types and weights
            queryDetailsChangedSpy.clear()
            compare(emptyQuery.featureTypes.length, 0)
            compare(featureTypesSpy.count, 0)
            emptyQuery.setFeatureWeight(RouteQuery.TollFeature, RouteQuery.AvoidFeatureWeight);
            compare(featureTypesSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            emptyQuery.setFeatureWeight(RouteQuery.HighwayFeature, RouteQuery.PreferFeatureWeight);
            compare(featureTypesSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)
            compare(emptyQuery.featureTypes.length, 2)
            compare(emptyQuery.featureTypes[0], RouteQuery.TollFeature)
            compare(emptyQuery.featureTypes[1], RouteQuery.HighwayFeature)
            // Verify feature weights are as set
            compare(emptyQuery.featureWeight(RouteQuery.TollFeature), RouteQuery.AvoidFeatureWeight);
            compare(emptyQuery.featureWeight(RouteQuery.HighwayFeature), RouteQuery.PreferFeatureWeight);
            // Neutralize a weight, feature should disappear
            emptyQuery.setFeatureWeight(RouteQuery.TollFeature, RouteQuery.NeutralFeatureWeight);
            compare(featureTypesSpy.count, 3)
            compare(queryDetailsChangedSpy.count, 3)
            compare(emptyQuery.featureTypes.length, 1)
            compare(emptyQuery.featureWeight(RouteQuery.TollFeature), RouteQuery.NeutralFeatureWeight);
            compare(emptyQuery.featureWeight(RouteQuery.HighwayFeature), RouteQuery.PreferFeatureWeight);
            compare(emptyQuery.featureTypes[0], RouteQuery.HighwayFeature)
            compare(emptyQuery.featureWeight(emptyQuery.featureTypes[0]), RouteQuery.PreferFeatureWeight)
            compare(featureTypesSpy.count, 3)
            compare(queryDetailsChangedSpy.count, 3)
            compare(emptyQuery.featureTypes.length, 1)

            // Put some feature weights and then reset them with NoFeature
            emptyQuery.setFeatureWeight(RouteQuery.FerryFeature, RouteQuery.RequireFeatureWeight);
            emptyQuery.setFeatureWeight(RouteQuery.MotorPoolLaneFeature, RouteQuery.DisallowFeatureWeight);
            compare(featureTypesSpy.count, 5)
            compare(queryDetailsChangedSpy.count, 5)
            compare(emptyQuery.featureTypes.length, 3)
            emptyQuery.setFeatureWeight(RouteQuery.NoFeature, RouteQuery.NeutralFeatureWeight)
            compare(featureTypesSpy.count, 6)
            compare(queryDetailsChangedSpy.count, 6)
            compare(emptyQuery.featureTypes.length, 0)

            // Segment details
            queryDetailsChangedSpy.clear()
            compare(segmentDetailSpy.count, 0)
            compare(emptyQuery.segmentDetail, RouteQuery.BasicSegmentData)
            emptyQuery.segmentDetail = RouteQuery.NoSegmentData
            compare(segmentDetailSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.segmentDetail, RouteQuery.NoSegmentData)
            emptyQuery.segmentDetail = RouteQuery.NoSegmentData
            compare(segmentDetailSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.segmentDetail, RouteQuery.NoSegmentData)

            // Maneuver details
            queryDetailsChangedSpy.clear()
            compare(maneuverDetailSpy.count, 0)
            compare(emptyQuery.maneuverDetail, RouteQuery.BasicManeuvers)
            emptyQuery.maneuverDetail = RouteQuery.NoManeuvers
            compare(maneuverDetailSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.maneuverDetail, RouteQuery.NoManeuvers)
            emptyQuery.maneuverDetail = RouteQuery.NoManeuvers
            compare(maneuverDetailSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.maneuverDetail, RouteQuery.NoManeuvers)

            // NumberAlternativeRoutes
            queryDetailsChangedSpy.clear()
            compare(numberAlterNativeRoutesSpy.count, 0)
            compare(emptyQuery.numberAlternativeRoutes, 0)
            emptyQuery.numberAlternativeRoutes = 2
            compare(numberAlterNativeRoutesSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.numberAlternativeRoutes, 2)
            emptyQuery.numberAlternativeRoutes = 2
            compare(numberAlterNativeRoutesSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.numberAlternativeRoutes, 2)

            // Route optimization
            queryDetailsChangedSpy.clear()
            compare(routeOptimizationsSpy.count, 0)
            compare(emptyQuery.routeOptimizations, RouteQuery.FastestRoute)
            emptyQuery.routeOptimizations = RouteQuery.ShortestRoute
            compare(routeOptimizationsSpy.count, 1)
            compare(queryDetailsChangedSpy.count, 1)
            compare(emptyQuery.routeOptimizations, RouteQuery.ShortestRoute)
            emptyQuery.routeOptimizations = RouteQuery.ShortestRoute | RouteQuery.MostScenicRoute
            compare(routeOptimizationsSpy.count, 2)
            compare(queryDetailsChangedSpy.count, 2)
            compare(emptyQuery.routeOptimizations, RouteQuery.ShortestRoute | RouteQuery.MostScenicRoute)

            // Must act gracefully
            emptyModel.reset()
            emptyModel.update()

            // Plugin
            compare(pluginSpy.count, 0)
            emptyModel.plugin = testPlugin
            compare(pluginSpy.count, 1)
            compare(emptyModel.plugin, testPlugin)
            emptyModel.plugin = testPlugin
            compare(pluginSpy.count, 1)
            emptyModel.plugin = errorPlugin
            compare(pluginSpy.count, 2)

            // Must act gracefully
            emptyModel.reset()
            emptyModel.update()
        }
        // Test that model acts gracefully when plugin is not set or is invalid
        // (does not support routing)
        RouteModel {id: errorModel; plugin: errorPlugin}
        RouteModel {id: errorModelNoPlugin}
        SignalSpy {id: countInvalidSpy; target: errorModel; signalName: "countChanged"}
        SignalSpy {id: errorSpy; target: errorModel; signalName: "errorChanged"}
        function test_error_plugin() {
            // test plugin not set
            compare(errorModelNoPlugin.error,RouteModel.NoError)
            errorModelNoPlugin.update()
            compare(errorModelNoPlugin.error,RouteModel.EngineNotSetError)
            console.log(errorModelNoPlugin.errorString)

            //plugin set but otherwise not offering anything
            compare(errorModel.error,RouteModel.EngineNotSetError)
            compare(errorModel.errorString,"This error was expected. No worries !")
            errorSpy.clear()
            errorModel.update()
            compare(errorModel.error,RouteModel.EngineNotSetError)
            compare(errorModel.errorString,qsTr("Cannot route, route manager not set."))
            compare(errorSpy.count, 1)
            errorSpy.clear()
            errorModel.cancel()
            compare(errorModel.error,RouteModel.NoError)
            compare(errorModel.errorString,"")
            compare(errorSpy.count, 1)
            errorSpy.clear()
            errorModel.reset()
            compare(errorModel.error,RouteModel.NoError)
            compare(errorModel.errorString,"")
            compare(errorSpy.count, 0)
            errorSpy.clear()
            errorModel.update()
            compare(errorModel.error,RouteModel.EngineNotSetError)
            compare(errorModel.errorString,qsTr("Cannot route, route manager not set."))
            compare(errorSpy.count, 1)
            errorSpy.clear()
            var data = errorModel.get(-1)
            compare(data, emptyRoute)
        }
    }

    Plugin {
        id: testPlugin_immediate;
        name: "qmlgeo.test.plugin"
        allowExperimental: true
        parameters: [
            // Parms to guide the test plugin
            PluginParameter { name: "gc_supported"; value: true},
            PluginParameter { name: "gc_finishRequestImmediately"; value: true},
            PluginParameter { name: "gc_validateWellKnownValues"; value: true}
        ]
    }

    Plugin {
        id: testPlugin_slacker;
        name: "qmlgeo.test.plugin"
        allowExperimental: true
        parameters: [
            // Parms to guide the test plugin
            PluginParameter { name: "gc_finishRequestImmediately"; value: false}
        ]
    }

    Plugin {
        id: testPlugin_slacker_alt
        name: "qmlgeo.test.plugin"
        allowExperimental: true
        PluginParameter { name: "gc_finishRequestImmediately"; value: false}
        PluginParameter { name: "gc_alternateGeoRoute"; value: true}
    }

    Plugin {
        id: basicRoutingPlugin_slacker;
        name: "qmlgeo.test.plugin"
        allowExperimental: true
        parameters: [
            // Parms to guide the test plugin
            PluginParameter { name: "gc_finishRequestImmediately"; value: false}
        ]
    }

    property variant rcoordinate1: QtPositioning.coordinate(50, 50)
    property variant rcoordinate2: QtPositioning.coordinate(51, 52)
    property variant rcoordinate3: QtPositioning.coordinate(53, 54)
    property variant rcoordinate4: QtPositioning.coordinate(55, 56)
    property variant rcoordinate5: QtPositioning.coordinate(57, 58)

    property variant fcoordinate1: QtPositioning.coordinate(60, 60)
    property variant fcoordinate2: QtPositioning.coordinate(61, 62)
    property variant fcoordinate3: QtPositioning.coordinate(63, 64)
    property variant fcoordinate4: QtPositioning.coordinate(65, 66)
    property variant fcoordinate5: QtPositioning.coordinate(67, 68)

    property variant f2coordinate1: QtPositioning.coordinate(60, 60)
    property variant f2coordinate2: QtPositioning.coordinate(61, 62)
    property variant f2coordinate3: QtPositioning.coordinate(63, 64)

    property geoCoordinate waypoint1: QtPositioning.coordinate(70, 70)
    property geoCoordinate waypoint2: QtPositioning.coordinate(71, 71)

    RouteQuery {id: routeQuery}
    property var routeQueryDefaultWaypoints: [
        { latitude: 60, longitude: 60 },
        { latitude: 61, longitude: 62 },
        { latitude: 63, longitude: 64 },
        { latitude: 65, longitude: 66 },
        { latitude: 67, longitude: 68 }
    ]
    property var routeQuery2DefaultWaypoints: [
        f2coordinate1,
        f2coordinate2,
        f2coordinate3
    ]
    RouteQuery {
        id: filledRouteQuery
        numberAlternativeRoutes: 0
        waypoints: routeQueryDefaultWaypoints
    }
    RouteQuery {
        id: filledRouteQuery2
        numberAlternativeRoutes: 0
        waypoints: routeQuery2DefaultWaypoints
    }
    RouteModel {
        id: routeModelAutomatic;
        plugin: testPlugin_slacker;
        query: filledRouteQuery;
        autoUpdate: true
    }
    RouteModel {
        id: routeModelAutomaticAltImpl;
        plugin: testPlugin_slacker_alt;
        query: filledRouteQuery;
        autoUpdate: true
    }

    SignalSpy {id: automaticRoutesSpy; target: routeModelAutomatic; signalName: "routesChanged" }
    SignalSpy {id: automaticRoutesSpyAlt; target: routeModelAutomaticAltImpl; signalName: "routesChanged" }

    RouteModel {id: routeModel; plugin: testPlugin_immediate; query: routeQuery }
    SignalSpy {id: testRoutesSpy; target: routeModel; signalName: "routesChanged"}
    SignalSpy {id: testCountSpy; target: routeModel; signalName: "countChanged" }
    SignalSpy {id: testStatusSpy; target: routeModel; signalName: "statusChanged"}
    SignalSpy {id: testErrorStringSpy; target: routeModel; signalName: "errorChanged"}
    SignalSpy {id: testErrorSpy; target: routeModel; signalName: "errorChanged"}
    SignalSpy {id: testWaypointsSpy; target: routeQuery; signalName: "waypointsChanged"}

    RouteModel {id: routeModelSlack; plugin: basicRoutingPlugin_slacker; query: routeQuery }
    SignalSpy {id: testRoutesSlackSpy; target: routeModelSlack; signalName: "routesChanged"}
    SignalSpy {id: testCountSlackSpy; target: routeModelSlack; signalName: "countChanged" }
    SignalSpy {id: testStatusSlackSpy; target: routeModelSlack; signalName: "statusChanged"}
    SignalSpy {id: testErrorStringSlackSpy; target: routeModelSlack; signalName: "errorChanged"}
    SignalSpy {id: testErrorSlackSpy; target: routeModelSlack; signalName: "errorChanged"}
    SignalSpy {id: testPluginSlackSpy; target: routeModelSlack; signalName: "pluginChanged"}

    RouteModel {id: routeModelEquals; plugin: testPlugin_immediate; query: routeQuery }

    TestCase {
        name: "Routing"
        function clear_immediate_model() {
            routeModel.reset()
            testRoutesSpy.clear()
            testCountSpy.clear()
            testStatusSpy.clear()
            testErrorStringSpy.clear()
            testErrorSpy.clear()
        }
        function clear_slacker_model() {
            routeModelSlack.reset()
            testRoutesSlackSpy.clear()
            testCountSlackSpy.clear()
            testStatusSlackSpy.clear()
            testErrorStringSlackSpy.clear()
            testErrorSlackSpy.clear()
        }

        function test_reset() {
            clear_immediate_model();
            routeQuery.numberAlternativeRoutes = 72 // 'altroutes - 70' is the echoed errorcode
            routeModel.update()
            verify (testErrorStringSpy.count > 0)
            verify (testErrorSpy.count > 0)
            compare (routeModel.errorString, "error")
            compare (routeModel.error, RouteModel.CommunicationError)
            compare (routeModel.count, 0)
            compare (testStatusSpy.count, 2)
            compare (routeModel.status, RouteModel.Error)
            routeModel.reset()
            compare (routeModel.status, RouteModel.Null)
            compare (routeModel.errorString, "")
            compare (routeModel.error, RouteModel.NoError)
            // Check that ongoing req is aborted
            clear_slacker_model()
            routeQuery.numberAlternativeRoutes = 3
            routeModelSlack.update()
            wait (100)
            routeModelSlack.reset()
            wait (200)
            compare (routeModelSlack.count, 0)
            // Check that results are cleared
            routeModelSlack.update()
            tryCompare(routeModelSlack, "count", 3) // numberALternativeRoutes
            routeModelSlack.reset()
            compare (routeModelSlack.count, 0)
            // Check that changing plugin resets any ongoing requests
            clear_slacker_model()
            routeQuery.numberAlternativeRoutes = 3
            compare (testPluginSlackSpy.count, 0)
            routeModelSlack.update()
            wait (100)
            routeModelSlack.plugin = testPlugin_immediate
            wait (200)
            compare (routeModelSlack.count, 0) // should be no updates
            compare (testPluginSlackSpy.count, 1)
            // test that works
            routeModelSlack.update()
            compare (routeModelSlack.count, 3)
            // return back
            routeModelSlack.plugin = testPlugin_slacker
        }

        function test_error_routing() {
            // Basic immediate error
            clear_immediate_model();
            routeQuery.numberAlternativeRoutes = 72 // 'altroutes - 70' is the echoed errorcode
            routeModel.update()
            compare (testErrorStringSpy.count, 1)
            compare (testErrorSpy.count, 1)
            compare (routeModel.errorString, "error")
            compare (routeModel.error, RouteModel.CommunicationError)
            compare (routeModel.count, 0)
            compare (testStatusSpy.count, 2)
            compare (routeModel.status, RouteModel.Error)
            // Basic delayed error
            clear_slacker_model()
            routeQuery.numberAlternativeRoutes = 72
            routeModelSlack.update()
            compare (testErrorStringSlackSpy.count, 0)
            compare (testErrorSlackSpy.count, 0)
            if (routeModelSlack.errorString == "")
                tryCompare(testErrorStringSlackSpy, "count", 1)
            else
                tryCompare(testErrorStringSlackSpy, "count", 2)
            compare (routeModelSlack.errorString, "error")
            compare (routeModelSlack.error, RouteModel.CommunicationError)
            compare (routeModelSlack.count, 0)
            // check that we recover
            routeQuery.numberAlternativeRoutes = 1
            routeModelSlack.update()
            tryCompare(routeModelSlack, "count", 1)
            compare (testCountSlackSpy.count, 1)
            compare (routeModelSlack.errorString, "")
            compare (routeModelSlack.error, RouteModel.NoError)
        }
        function test_basic_routing() {
            compare (testRoutesSpy.count, 0)
            compare (routeModel.errorString, "")
            compare (routeModel.error, RouteModel.NoError)
            compare (testCountSpy.count, 0)
            compare (routeModel.count, 0)
            compare (routeQuery.waypoints.length, 0)
            compare (testWaypointsSpy.count, 0)
            routeQuery.addWaypoint(rcoordinate1)
            routeQuery.addWaypoint(rcoordinate2)
            routeQuery.addWaypoint(rcoordinate3)
            routeQuery.addWaypoint(rcoordinate4)
            routeQuery.addWaypoint(rcoordinate5)
            compare (testWaypointsSpy.count, 5)
            compare (routeQuery.waypoints.length, 5)
            routeQuery.numberAlternativeRoutes = 1 // how many routes to get back, > 70 indicates error
            routeModel.update()
            routeModelEquals.update()
            tryCompare (testRoutesSpy, "count", 1) // 5 sec
            tryCompare (testCountSpy, "count", 1)
            compare (routeModel.count, 1)
             // the test plugin echoes waypoints back as the path of the route:
            compare (routeQuery.waypoints.length, 5)
            compare (routeModel.get(0).path.length, 5)
            compare (routeModel.get(0).path[0].latitude, routeQuery.waypoints[0].latitude)
            // test comparison
            var route1 = routeModel.get(0)
            var route2 = routeModelEquals.get(0)
            verify(route1 === route2)
            // check reset() functionality
            routeModel.reset()
            tryCompare (testRoutesSpy, "count", 2) // 5 sec
            tryCompare (testCountSpy, "count", 2)
            compare (routeModel.count, 0)

            // delayed responses
            compare (testRoutesSlackSpy.count, 0)
            compare (routeModelSlack.errorString, "")
            compare (routeModel.error, RouteModel.NoError)
            compare (testCountSlackSpy.count, 0)
            compare (routeModelSlack.count, 0)
            routeModelSlack.update()
            wait (100)
            compare (testRoutesSlackSpy.count, 0)
            compare (testCountSlackSpy.count, 0)
            tryCompare(testRoutesSlackSpy, "count", 1)
            compare (testCountSlackSpy.count, 1)
            compare(routeModelSlack.count, 1)
            compare (routeModelSlack.get(0).path.length, 5)
            compare (routeModelSlack.get(0).path[0].latitude, routeQuery.waypoints[0].latitude)

            // Frequent updates, previous requests are aborted
            routeModelSlack.reset()
            testRoutesSlackSpy.clear()
            testCountSlackSpy.clear()
            routeModelSlack.update()
            wait (100)
            compare(testRoutesSlackSpy.count, 0)
            compare(testCountSlackSpy.count, 0)
            routeModelSlack.update()
            wait (100)
            compare(testRoutesSlackSpy.count, 0)
            compare(testCountSlackSpy.count, 0)
            routeModelSlack.update()
            wait (100)
            compare(testRoutesSlackSpy.count, 0)
            compare(testCountSlackSpy.count, 0)
            routeModelSlack.update()
            wait (100)
            compare(testRoutesSlackSpy.count, 0)
            compare(testCountSlackSpy.count, 0)
            tryCompare(testRoutesSlackSpy, "count", 1)
            compare(testCountSlackSpy.count, 1)
            compare(routeModelSlack.count, 1)

            test_basic_routing_automatic(routeModelAutomatic, automaticRoutesSpy, "routeModelAutomatic")
            test_basic_routing_automatic(routeModelAutomaticAltImpl, automaticRoutesSpyAlt, "routeModelAutomaticAltImpl")
        }

        function test_basic_routing_automatic(model, spy, label) {
            if (label === undefined)
                return
            console.log("testing",label)
            // Autoupdate
            spy.clear();
            filledRouteQuery.numberAlternativeRoutes = 1 // 'altroutes - 70' is the echoed errorcode
            tryCompare (spy, "count", 1) // 5 sec
            compare(model.count, 1) // There should be a route already
            compare (model.get(0).path.length, 5)
            compare (model.get(0).path[0].latitude, filledRouteQuery.waypoints[0].latitude)

            if (label === "routeModelAutomaticAltImpl") // Test that it is an altImpl
                compare(model.get(0).travelTime,  123456)

            // Remove a waypoint and check that autoupdate works
            filledRouteQuery.removeWaypoint(fcoordinate2)
            tryCompare (spy, "count", 2)
            compare (model.get(0).path.length, 4)
            compare (model.get(0).path[0].latitude, fcoordinate1.latitude)

            // Add a waypoint and check that autoupdate works
            filledRouteQuery.addWaypoint(fcoordinate2);
            tryCompare (spy, "count", 3)
            compare(model.count, 1);
            compare(model.get(0).path.length, 5);
            compare(model.get(0).path[0].latitude, filledRouteQuery.waypoints[0].latitude);

            // Change contents of a coordinate and check that autoupdate works
            filledRouteQuery.waypoints = [
                { latitude: fcoordinate1.latitude + 1, longitude: fcoordinate1.longitude },
                { latitude: 61, longitude: 62 },
                { latitude: 63, longitude: 64 },
                { latitude: 65, longitude: 66 },
                { latitude: 67, longitude: 68 }
            ];
            tryCompare (spy, "count", 4)
            compare(model.get(0).path[0].latitude, fcoordinate1.latitude + 1) // new value should be echoed

            /* Test waypoints */
            // Verify that bearing is NaN for coordinates
            verify(isNaN(filledRouteQuery.waypoints[0].bearing))
            var numWaypoints = filledRouteQuery.waypoints.length
            // Add a waypoint with bearing
            filledRouteQuery.addWaypoint(waypoint1)
            tryCompare (spy, "count", 5)
//            compare(filledRouteQuery.waypoints[numWaypoints].bearing, 42)
            // testing Waypoint to coordinate conversion
//            compare(filledRouteQuery.waypoints[numWaypoints], filledRouteQuery.waypoints[numWaypoints].coordinate)
            waypoint1.latitude += 0.1
            compare(model.get(0).distance, 0)
            tryCompare (spy, "count", 5)
            numWaypoints++;
            filledRouteQuery.addWaypoint(waypoint2)
            numWaypoints++;
            tryCompare (spy, "count", 6)
//            compare(filledRouteQuery.waypointObjects()[numWaypoints-1].bearing, 43)
            compare(model.get(0).distance, 0)
            // remove non-existent waypoint
            filledRouteQuery.removeWaypoint(waypoint1)
            tryCompare (spy, "count", 6)

            // Change query
            model.query = filledRouteQuery2
            filledRouteQuery2.numberAlternativeRoutes = 3
            tryCompare (spy, "count", 7)
            compare (model.get(0).path.length, 3)

            // Verify that the old query is disconnected internally ie. does not trigger update
            filledRouteQuery.waypoints = [
                { latitude: fcoordinate1.latitude + 2, longitude: fcoordinate1.longitude },
                { latitude: 61, longitude: 62 },
                { latitude: 63, longitude: 64 },
                { latitude: 65, longitude: 66 },
                { latitude: 67, longitude: 68 }
            ];
            wait(800) // wait to hope no further updates comes through
            compare (spy.count, 7)
            compare(model.get(0).path.length, 3);

            // departure time
            verify(!model.get(0).extendedAttributes["tst_departureTime"])
            var invalidDate = new Date("abc"); // to create an invalid date
            var validDate = new Date("2011-02-07T11:05:00");

            filledRouteQuery2.departureTime = validDate
            tryCompare(spy, "count", 8)
            compare(model.get(0).extendedAttributes["tst_departureTime"], validDate)

            filledRouteQuery2.departureTime = invalidDate
            tryCompare (spy, "count", 9)
            verify(!model.get(0).extendedAttributes["tst_departureTime"])

            // ReSetting
            filledRouteQuery.numberAlternativeRoutes = 0
            filledRouteQuery2.numberAlternativeRoutes = 0
            filledRouteQuery.waypoints = routeQueryDefaultWaypoints
            filledRouteQuery2.waypoints = routeQuery2DefaultWaypoints

            waypoint1.coordinate = QtPositioning.coordinate(70, 70)
            waypoint2.bearing = 42
            waypoint2.coordinate = QtPositioning.coordinate(71, 71)
            waypoint2.bearing = 43
        }



        function test_route_query_handles_destroyed_qml_objects() {
            var coordinate = QtPositioning.coordinate(11, 52);
            routeQuery.addWaypoint(coordinate);
            wait(300);
            routeQuery.clearWaypoints();
        }
    }
}




