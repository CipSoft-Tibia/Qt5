// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0 as QQ2
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Input 2.0
import Qt3D.Logic 2.0
import Qt3D.Extras 2.0

import "planets.js" as Planets

Entity {
    id: sceneRoot

    property bool ready: false

    property real cameraNear: 0
    property real xLookAtOffset: 0
    property real yLookAtOffset: 0
    property real zLookAtOffset: 0
    property real xCameraOffset: 0
    property real yCameraOffset: 0
    property real zCameraOffset: 0

    property var planetData
    property var planets: []

    property vector3d defaultUp: Qt.vector3d(0, 1, 0)
    property vector3d defaultCameraPosition: Qt.vector3d(Planets.solarDistance,
                                                         Planets.solarDistance,
                                                         Planets.solarDistance)
    property vector3d tiltAxis: Qt.vector3d(0, 0, 1)
    property vector3d rollAxis: Qt.vector3d(0, 1, 0)
    property real cameraDistance: 1
    property vector3d oldCameraPosition
    property vector3d oldFocusedPlanetPosition

    property color ambientStrengthStarfield: "#000000"
    property color ambientStrengthSun: "#ffffff"
    property color ambientStrengthClouds: "#000000"
    property color ambientStrengthRing: "#111111"
    property color ambientStrengthPlanet: "#222222"
    property real shininessSpecularMap: 50.0
    property real shininessClouds: 10.0
    property real shininessBasic: 1.0

    property real saturnRingInnerRadius
    property real saturnRingOuterRadius
    property real uranusRingInnerRadius
    property real uranusRingOuterRadius

    // Time variables
    property int year: 2000
    property int month: 1
    property int day: 1
    // Time scale formula based on http://www.stjarnhimlen.se/comp/ppcomp.html
    property real startD: 367 * year - 7 * (year + (month + 9) / 12) / 4 + 275 * month / 9 + day - 730530
    property real oldTimeD: startD
    property real currTimeD: startD
    property real deltaTimeD: 0
    property real daysPerFrame
    property real daysPerFrameScale

    property real planetScale

    property bool focusedScaling: false
    property int focusedMinimumScale: 20
    property real actualScale

    // Animate solar system with LogicComponent
    FrameAction {
        onTriggered: {
            frames++
            animate(focusedPlanet)
        }
    }

    PlanetsLight {
        id: light
        ratio: width / height
    }

    Camera {
        id: camera
        projectionType: CameraLens.PerspectiveProjection
        fieldOfView: 45
        aspectRatio: width / height
        nearPlane: 2500.0
        farPlane: 20000.0
        position: defaultCameraPosition
        upVector: defaultUp
        viewCenter: Qt.vector3d( xLookAtOffset, yLookAtOffset, zLookAtOffset )
    }

    FirstPersonCameraController { camera: camera }

    components: [
        PlanetFrameGraph {
            id: framegraph
            viewCamera: camera
            lightCamera: light.lightCamera
        },
        InputSettings {}
    ]

    PlanetEffect {
        id: effectD
        light: light
    }

    PlanetEffect {
        id: effectDB
        light: light
        vertexES: "qrc:/shaders/es2/planetDB.vert"
        fragmentES: "qrc:/shaders/es2/planetDB.frag"
        vertexGL: "qrc:/shaders/gl3/planetDB.vert"
        fragmentGL: "qrc:/shaders/gl3/planetDB.frag"
        vertexRHI: "qrc:/shaders/rhi/planetDB.vert"
        fragmentRHI: "qrc:/shaders/rhi/planetDB.frag"
    }

    PlanetEffect {
        id: effectDSB
        light: light
        vertexES: "qrc:/shaders/es2/planetDB.vert"
        fragmentES: "qrc:/shaders/es2/planetDSB.frag"
        vertexGL: "qrc:/shaders/gl3/planetDB.vert"
        fragmentGL: "qrc:/shaders/gl3/planetDSB.frag"
        vertexRHI: "qrc:/shaders/rhi/planetDB.vert"
        fragmentRHI: "qrc:/shaders/rhi/planetDSB.frag"
    }

    PlanetEffect {
        id: cloudEffect
        light: light
        vertexES: "qrc:/shaders/es2/planetD.vert"
        fragmentES: "qrc:/shaders/es2/planetDS.frag"
        vertexGL: "qrc:/shaders/gl3/planetD.vert"
        fragmentGL: "qrc:/shaders/gl3/planetDS.frag"
        vertexRHI: "qrc:/shaders/rhi/planetD.vert"
        fragmentRHI: "qrc:/shaders/rhi/planetDS.frag"
    }

    SunEffect {
        id: sunEffect
    }

    ShadowEffect {
        id: shadowMapEffect
        shadowTexture: framegraph.shadowTexture
        light: light
    }

    //! [2]
    QQ2.Component.onCompleted: {
        planetData = Planets.loadPlanetData()
        // Push in the correct order
        planets.push(sun)
        planets.push(mercury)
        planets.push(venus)
        planets.push(earth)
        planets.push(mars)
        planets.push(jupiter)
        planets.push(saturn)
        planets.push(uranus)
        planets.push(neptune)
        planets.push(moon)
        // TODO: Once support for creating meshes from arrays is implemented take these into use
        //saturnRing.makeRing()
        //uranusRing.makeRing()
        saturnRingOuterRadius = planetData[Planets.SATURN].radius + Planets.saturnOuterRadius
        saturnRingInnerRadius = planetData[Planets.SATURN].radius + 0.006630
        uranusRingOuterRadius = planetData[Planets.URANUS].radius + Planets.uranusOuterRadius
        uranusRingInnerRadius = planetData[Planets.URANUS].radius + 0.002
        ready = true
        changeScale(1200)
        changeSpeed(0.2)
        setLookAtOffset(Planets.SUN)
    }
    //! [2]

    //! [0]
    QQ2.NumberAnimation {
        id: lookAtOffsetAnimation
        target: sceneRoot
        properties: "xLookAtOffset, yLookAtOffset, zLookAtOffset"
        to: 0
//        easing.type: Easing.InOutQuint
        duration: 1250
    }

    QQ2.NumberAnimation {
        id: cameraOffsetAnimation
        target: sceneRoot
        properties: "xCameraOffset, yCameraOffset, zCameraOffset"
        to: 0
//        easing.type: Easing.InOutQuint
        duration: 2500
    }
    //! [0]

    QQ2.Behavior on cameraNear {
        QQ2.PropertyAnimation {
//            easing.type: Easing.InOutQuint
            duration: 2500
        }
    }

    function changePlanetFocus(oldPlanet, focusedPlanet) {
        setOldPlanet(oldPlanet, focusedPlanet)
        setLookAtOffset(focusedPlanet)
        setCameraOffset(oldPlanet, focusedPlanet)
        lookAtOffsetAnimation.restart()
        cameraOffsetAnimation.restart()
    }

    function setOldPlanet(oldPlanet, focusedPlanet) {
        oldCameraPosition = camera.position

        var planet = 0
        if (oldPlanet !== Planets.SOLAR_SYSTEM)
            planet = oldPlanet
        oldFocusedPlanetPosition = Qt.vector3d(planets[planet].x,
                                               planets[planet].y,
                                               planets[planet].z)
        checkScaling(focusedPlanet)
    }

    function setScale(value, focused) {
        // Save actual scale
        if (!focused)
            actualScale = value

        // Limit minimum scaling in focus mode to avoid jitter caused by rounding errors
        if (value <= focusedMinimumScale && (focusedScaling || focused))
            planetScale = focusedMinimumScale
        else
            planetScale = actualScale
        return planetScale
    }

    function checkScaling(focusedPlanet) {
        if (focusedPlanet !== Planets.SOLAR_SYSTEM) {
            // Limit minimum scaling in focus mode to avoid jitter caused by rounding errors
            if (actualScale <= focusedMinimumScale) {
                planetScale = focusedMinimumScale
                changeScale(focusedMinimumSfocusedPlanetcale, true)
            }
            focusedScaling = true
        } else if (focusedScaling === true) {
            // Restore normal scaling
            focusedScaling = false
            changeScale(actualScale, false)
        }
    }

    function setLookAtOffset(focusedPlanet) {
        var offset = oldFocusedPlanetPosition

        var planet = 0
        if (focusedPlanet !== Planets.SOLAR_SYSTEM)
            planet = focusedPlanet

        var focusedPlanetPosition = Qt.vector3d(planets[planet].x,
                                                planets[planet].y,
                                                planets[planet].z)
        offset = offset.minus(focusedPlanetPosition)

        xLookAtOffset = offset.x
        yLookAtOffset = offset.y
        zLookAtOffset = offset.z
    }

    function setCameraOffset(oldPlanet, focusedPlanet) {
        var offset = oldCameraPosition

        var planet = 0
        if (focusedPlanet !== Planets.SOLAR_SYSTEM)
            planet = focusedPlanet

        var newCameraPosition = getNewCameraPosition(focusedPlanet, Planets.getOuterRadius(planet))

        if (focusedPlanet !== Planets.SUN)
            offset = offset.minus(newCameraPosition)

        if (oldPlanet === Planets.SOLAR_SYSTEM && focusedPlanet === Planets.SUN) {
            xCameraOffset = Math.abs(offset.x)
            yCameraOffset = Math.abs(offset.y)
            zCameraOffset = Math.abs(offset.z)
        } else { // from a planet to another
            xCameraOffset = offset.x
            yCameraOffset = offset.y
            zCameraOffset = offset.z
        }
    }

    function getNewCameraPosition(focusedPlanet, radius) {
        var position
        if (focusedPlanet === Planets.SOLAR_SYSTEM) {
            position = defaultCameraPosition
            position = position.times(cameraDistance)
        } else if (focusedPlanet === Planets.SUN) {
            position = Qt.vector3d(radius * planetScale * 2,
                                   radius * planetScale * 2,
                                   radius * planetScale * 2)
            position = position.times(cameraDistance)
        } else {
            var vec1 = Qt.vector3d(planets[focusedPlanet].x,
                                   planets[focusedPlanet].y,
                                   planets[focusedPlanet].z)
            var vec2 = defaultUp
            vec1 = vec1.normalized()
            vec2 = vec2.crossProduct(vec1)
            vec2 = vec2.times(radius * planetScale * cameraDistance * 4)
            vec2 = vec2.plus(Qt.vector3d(planets[focusedPlanet].x,
                                         planets[focusedPlanet].y,
                                         planets[focusedPlanet].z))
            vec1 = Qt.vector3d(0, radius * planetScale, 0)
            vec2 = vec2.plus(vec1)
            position = vec2
        }
        return position
    }

    function advanceTime(focusedPlanet) {
        if (focusedPlanet === Planets.SOLAR_SYSTEM)
            daysPerFrame = daysPerFrameScale * 10
        else
            daysPerFrame = daysPerFrameScale * planetData[focusedPlanet].period / 100.0

        // Advance the time in days
        oldTimeD = currTimeD
        currTimeD = currTimeD + daysPerFrame
        deltaTimeD = currTimeD - oldTimeD
    }

    function positionPlanet(i) {
        var planet = planetData[i]
        var target = planets[i]

        if (i !== Planets.SUN) {
            // Calculate the planet orbital elements from the current time in days
            var N = (planet.N1 + planet.N2 * currTimeD) * Math.PI / 180
            var iPlanet = (planet.i1 + planet.i2 * currTimeD) * Math.PI / 180
            var w = (planet.w1 + planet.w2 * currTimeD) * Math.PI / 180
            var a = planet.a1 + planet.a2 * currTimeD
            var e = planet.e1 + planet.e2 * currTimeD
            var M = (planet.M1 + planet.M2 * currTimeD) * Math.PI / 180
            var E = M + e * Math.sin(M) * (1.0 + e * Math.cos(M))

            var xv = a * (Math.cos(E) - e)
            var yv = a * (Math.sqrt(1.0 - e * e) * Math.sin(E))
            var v = Math.atan2(yv, xv)

            // Calculate the distance (radius)
            // TODO: Math.hypot() is ES6 and QML JS is only ES5 currently. A patch to QtQml is
            // required to get Math.hypot() to work.
            //var r = Math.hypot(xv, yv)
            var r = Math.sqrt(Math.pow(xv, 2) + Math.pow(yv, 2))

            // From http://www.davidcolarusso.com/astro/
            // Modified to compensate for the right handed coordinate system of OpenGL
            var xh = r * (Math.cos(N) * Math.cos(v + w)
                          - Math.sin(N) * Math.sin(v + w) * Math.cos(iPlanet))
            var zh = -r * (Math.sin(N) * Math.cos(v + w)
                           + Math.cos(N) * Math.sin(v + w) * Math.cos(iPlanet))
            var yh = r * (Math.sin(w + v) * Math.sin(iPlanet))

            // Apply the position offset from the center of orbit to the bodies
            var centerOfOrbit = planet.centerOfOrbit
            target.x = planets[centerOfOrbit].x + xh * Planets.auScale
            target.y = planets[centerOfOrbit].y + yh * Planets.auScale
            target.z = planets[centerOfOrbit].z + zh * Planets.auScale
        }
        // Calculate the rotation (roll) of the bodies. Tilt does not change.
        target.roll += (deltaTimeD / planet.period) * 360 // In degrees
    }

    function changeScale(scale, focused) {
        if (!ready)
            return

        var scaling = setScale(scale, focused)
        sun.r = planetData[Planets.SUN].radius * scaling / 10
        mercury.r = planetData[Planets.MERCURY].radius * scaling
        venus.r = planetData[Planets.VENUS].radius * scaling
        earth.r = planetData[Planets.EARTH].radius * scaling
        earthClouds.r = planetData[Planets.EARTH].radius * scaling * 1.02
        moon.r = planetData[Planets.MOON].radius * scaling
        mars.r = planetData[Planets.MARS].radius * scaling
        jupiter.r = planetData[Planets.JUPITER].radius * scaling
        saturn.r = planetData[Planets.SATURN].radius * scaling
        saturnRing.outerRadius = saturnRingOuterRadius * scaling
        saturnRing.innerRadius = saturnRingInnerRadius * scaling
        uranus.r = planetData[Planets.URANUS].radius * scaling
        uranusRing.outerRadius = uranusRingOuterRadius * scaling
        uranusRing.innerRadius = uranusRingInnerRadius * scaling
        neptune.r = planetData[Planets.NEPTUNE].radius * scaling
    }

    function changeSpeed(speed) {
        daysPerFrameScale = speed
    }

    function changeCameraDistance(distance) {
        cameraDistance = distance
    }

    //! [3]
    function animate(focusedPlanet) {
        if (!ready)
            return

        advanceTime(focusedPlanet)
        for (var i = 0; i <= Planets.NUM_SELECTABLE_PLANETS; i++)
            positionPlanet(i)

        updateCamera(focusedPlanet)
    }
    //! [3]

    function updateCamera(focusedPlanet) {
        // Get the appropriate near plane position for the camera and animate it with QML animations
        var outerRadius = Planets.getOuterRadius(focusedPlanet)
        cameraNear = outerRadius
        camera.nearPlane = cameraNear
        light.near = cameraNear

        // Calculate position
        var cameraPosition = getNewCameraPosition(focusedPlanet, outerRadius)
        var cameraOffset = Qt.vector3d(xCameraOffset, yCameraOffset, zCameraOffset)
        cameraPosition = cameraPosition.plus(cameraOffset)

        // Calculate look-at point
        var lookAtPlanet = Planets.SUN
        if (focusedPlanet !== Planets.SOLAR_SYSTEM)
            lookAtPlanet = focusedPlanet
        var cameraLookAt = Qt.vector3d(planets[lookAtPlanet].x,
                                       planets[lookAtPlanet].y,
                                       planets[lookAtPlanet].z)
        var lookAtOffset = Qt.vector3d(xLookAtOffset, yLookAtOffset, zLookAtOffset)
        cameraLookAt = cameraLookAt.plus(lookAtOffset)

        // Set position and look-at
        camera.viewCenter = cameraLookAt
        camera.position = Qt.vector3d(cameraPosition.x, cameraPosition.y, cameraPosition.z)
        camera.upVector = defaultUp
    }

    //
    // STARFIELD
    //

    Entity {
        id: starfieldEntity

        Mesh {
            id: starfield
            source: "qrc:/meshes/starfield.obj"
        }

        PlanetMaterial {
            id: materialStarfield
            effect: effectD
            ambientLight: ambientStrengthStarfield
            specularColor: Qt.rgba(0.0, 0.0, 0.0, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/galaxy_starfield.jpg"
            shininess: 1000000.0
        }

        property Transform transformStarfield: Transform {
            scale: 8500
            translation: Qt.vector3d(0, 0, 0)
        }

        components: [ starfield, materialStarfield, transformStarfield ]
    }

    //
    // SUN
    //

    Entity {
        id: sunEntity

        Planet {
            id: sun
            tilt: planetData[Planets.SUN].tilt
        }

        PlanetMaterial {
            id: materialSun
            effect: sunEffect
            ambientLight: ambientStrengthSun
            diffuseMap: "qrc:/images/solarsystemscope/sunmap.jpg"
        }

        property Transform transformSun: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(sun.x, sun.y, sun.z))
                m.rotate(sun.tilt, tiltAxis)
                m.rotate(sun.roll, rollAxis)
                m.scale(sun.r)
                return m
            }
        }

        components: [ sun, materialSun, transformSun ]
    }

    //
    // PLANETS
    //

    // MERCURY

    Entity {
        id: mercuryEntity

        Planet {
            id: mercury
            tilt: planetData[Planets.MERCURY].tilt
        }

        PlanetMaterial {
            id: materialMercury
            effect: effectDB
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/mercurymap.jpg"
            normalMap: "qrc:/images/solarsystemscope/mercurynormal.jpg"
            shininess: shininessSpecularMap
        }

        property Transform transformMercury: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(mercury.x, mercury.y, mercury.z))
                m.rotate(mercury.tilt, tiltAxis)
                m.rotate(mercury.roll, rollAxis)
                m.scale(mercury.r)
                return m
            }
        }

        components: [ mercury, materialMercury, transformMercury ]
    }

    // VENUS

    Entity {
        id: venusEntity

        Planet {
            id: venus
            tilt: planetData[Planets.VENUS].tilt
        }

        PlanetMaterial {
            id: materialVenus
            effect: effectDB
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/venusmap.jpg"
            normalMap: "qrc:/images/solarsystemscope/venusnormal.jpg"
            shininess: shininessSpecularMap
        }

        property Transform transformVenus: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(venus.x, venus.y, venus.z))
                m.rotate(venus.tilt, tiltAxis)
                m.rotate(venus.roll, rollAxis)
                m.scale(venus.r)
                return m
            }
        }

        components: [ venus, materialVenus, transformVenus ]
    }

    // EARTH

    //! [1]
    Entity {
        id: earthEntity

        Planet {
            id: earth
            tilt: planetData[Planets.EARTH].tilt
        }

        PlanetMaterial {
            id: materialEarth
            effect: effectDSB
            ambientLight: ambientStrengthPlanet
            diffuseMap: "qrc:/images/solarsystemscope/earthmap2k.jpg"
            specularMap: "qrc:/images/solarsystemscope/earthspec2k.jpg"
            normalMap: "qrc:/images/solarsystemscope/earthnormal2k.jpg"
            shininess: shininessSpecularMap
        }

        property Transform transformEarth: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(earth.x, earth.y, earth.z))
                m.rotate(earth.tilt, tiltAxis)
                m.rotate(earth.roll, rollAxis)
                m.scale(earth.r)
                return m
            }
        }

        components: [ earth, materialEarth, transformEarth ]
    }
    //! [1]

    // EARTH CLOUDS

    Entity {
        id: earthCloudsEntity

        Planet {
            id: earthClouds
            tilt: planetData[Planets.EARTH].tilt
        }

        PlanetMaterial {
            id: materialEarthClouds
            effect: cloudEffect
            ambientLight: ambientStrengthClouds
            diffuseMap: "qrc:/images/solarsystemscope/earthcloudmapcolortrans.png"
            specularMap: "qrc:/images/solarsystemscope/earthcloudmapspec.jpg"
            shininess: shininessClouds
            opacity: 0.2
        }

        property Transform transformEarthClouds: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(earth.x, earth.y, earth.z))
                m.rotate(earth.tilt, tiltAxis)
                m.rotate(earth.roll / 1.2, rollAxis)
                m.scale(earthClouds.r)
                return m
            }
        }

        components: [ earthClouds, materialEarthClouds, transformEarthClouds ]
    }

    // MOON

    Entity {
        id: moonEntity

        Planet {
            id: moon
            tilt: planetData[Planets.MOON].tilt
        }

        PlanetMaterial {
            id: materialMoon
            effect: effectDB
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/moonmap2k.jpg"
            normalMap: "qrc:/images/solarsystemscope/moonnormal2k.jpg"
            shininess: shininessSpecularMap
        }

        property Transform transformMoon: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(moon.x, moon.y, moon.z))
                m.rotate(moon.tilt, tiltAxis)
                m.rotate(moon.roll, rollAxis)
                m.scale(moon.r)
                return m
            }
        }

        components: [ moon, materialMoon, transformMoon ]
    }

    // MARS

    Entity {
        id: marsEntity

        Planet {
            id: mars
            tilt: planetData[Planets.MARS].tilt
        }

        PlanetMaterial {
            id: materialMars
            effect: effectDB
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/marsmap2k.jpg"
            normalMap: "qrc:/images/solarsystemscope/marsnormal2k.jpg"
            shininess: shininessSpecularMap
        }

        property Transform transformMars: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(mars.x, mars.y, mars.z))
                m.rotate(mars.tilt, tiltAxis)
                m.rotate(mars.roll, rollAxis)
                m.scale(mars.r)
                return m
            }
        }

        components: [ mars, materialMars, transformMars ]
    }

    // JUPITER

    Entity {
        id: jupiterEntity

        Planet {
            id: jupiter
            tilt: planetData[Planets.JUPITER].tilt
        }

        PlanetMaterial {
            id: materialJupiter
            effect: effectD
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/jupitermap.jpg"
            shininess: shininessBasic
        }

        property Transform transformJupiter: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(jupiter.x, jupiter.y, jupiter.z))
                m.rotate(jupiter.tilt, tiltAxis)
                m.rotate(jupiter.roll, rollAxis)
                m.scale(jupiter.r)
                return m
            }
        }

        components: [ jupiter, materialJupiter, transformJupiter ]
    }

    // SATURN

    Entity {
        id: saturnEntity

        Planet {
            id: saturn
            tilt: planetData[Planets.SATURN].tilt
        }

        PlanetMaterial {
            id: materialSaturn
            effect: shadowMapEffect
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/saturnmap.jpg"
            shininess: shininessBasic
        }

        property Transform transformSaturn: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(saturn.x, saturn.y, saturn.z))
                m.rotate(saturn.tilt, tiltAxis)
                m.rotate(saturn.roll, rollAxis)
                m.scale(saturn.r)
                return m
            }
        }

        components: [ saturn, materialSaturn, transformSaturn ]
    }

    // SATURN RING

    Entity {
        id: saturnRingEntity

        Ring {
            id: saturnRing
            innerRadius: saturnRingInnerRadius
            outerRadius: saturnRingOuterRadius
        }

        PlanetMaterial {
            id: materialSaturnRing
            effect: shadowMapEffect
            ambientLight: ambientStrengthRing
            specularColor: Qt.rgba(0.01, 0.01, 0.01, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/saturnringcolortrans.png"
            shininess: shininessBasic
            opacity: 0.4
        }

        property Transform transformSaturnRing: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(saturn.x, saturn.y, saturn.z))
                m.rotate(saturn.tilt, tiltAxis)
                m.rotate(saturn.roll / 10, rollAxis)
                m.scale((saturnRing.innerRadius + saturnRing.outerRadius) / 1.75)
                return m
            }
        }

        components: [ saturnRing, materialSaturnRing, transformSaturnRing ]
    }

    // URANUS

    Entity {
        id: uranusEntity

        Planet {
            id: uranus
            tilt: planetData[Planets.URANUS].tilt
        }

        PlanetMaterial {
            id: materialUranus
            effect: shadowMapEffect
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/uranusmap.jpg"
            shininess: shininessBasic
        }

        property Transform transformUranus: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(uranus.x, uranus.y, uranus.z))
                m.rotate(uranus.tilt, tiltAxis)
                m.rotate(uranus.roll, rollAxis)
                m.scale(uranus.r)
                return m
            }
        }

        components: [ uranus, materialUranus, transformUranus ]
    }

    // URANUS RING

    Entity {
        id: uranusRingEntity

        Ring {
            id: uranusRing
            innerRadius: uranusRingInnerRadius
            outerRadius: uranusRingOuterRadius
        }

        PlanetMaterial {
            id: materialUranusRing
            effect: shadowMapEffect
            ambientLight: ambientStrengthRing
            specularColor: Qt.rgba(0.01, 0.01, 0.01, 1.0)
            diffuseMap: "qrc:/images/nasa/uranusringcolortrans.png"
            shininess: shininessBasic
            opacity: 0.4
        }

        property Transform transformUranusRing: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(uranus.x, uranus.y, uranus.z))
                m.rotate(uranus.tilt, tiltAxis)
                m.rotate(uranus.roll / 10, rollAxis)
                m.scale((uranusRing.innerRadius + uranusRing.outerRadius) / 1.75)
                return m
            }
        }

        components: [ uranusRing, materialUranusRing, transformUranusRing ]
    }

    // NEPTUNE

    Entity {
        id: neptuneEntity

        Planet {
            id: neptune
            tilt: planetData[Planets.NEPTUNE].tilt
        }

        PlanetMaterial {
            id: materialNeptune
            effect: effectD
            ambientLight: ambientStrengthPlanet
            specularColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            diffuseMap: "qrc:/images/solarsystemscope/neptunemap.jpg"
            shininess: shininessBasic
        }

        property Transform transformNeptune: Transform {
            matrix: {
                var m = Qt.matrix4x4()
                m.translate(Qt.vector3d(neptune.x, neptune.y, neptune.z))
                m.rotate(neptune.tilt, tiltAxis)
                m.rotate(neptune.roll, rollAxis)
                m.scale(neptune.r)
                return m
            }
        }

        components: [ neptune, materialNeptune, transformNeptune ]
    }
}
