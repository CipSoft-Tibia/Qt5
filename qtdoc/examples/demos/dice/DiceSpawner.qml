// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick3D
import QtQuick3D.Physics
import QtQuick3D.Helpers
import QtMultimedia

Node {
    id: shapeSpawner
    property var dices: []
    property var dieComponent: Qt.createComponent("PhysicalDie.qml")
    property bool justSpawned: false

    function randomInRange(min, max) {
        return Math.random() * (max - min) + min
    }

    function createDie(position, physicsMaterial, rollForce, diceWidth) {
        let rotation = Qt.vector3d(randomInRange(0, 360),
                                   randomInRange(0, 360), randomInRange(0, 360))
        let settings = {
            "position": position,
            "eulerRotation": rotation,
            "physicsMaterial": physicsMaterial,
            "diceWidth": diceWidth
        }
        let die = dieComponent.createObject(shapeSpawner, settings)
        if (die === null) {
            console.log("Error creating object")
        } else {
            die.applyCentralForce(rollForce)
            dices.push(die)
        }
    }
    SoundEffect {
        id: rollSound
        loops: 0
        source: "sounds/rolling.wav"
    }
    function spawnDice(numberOfDice, physicsMaterial, rollForce, diceWidth) {
        if (!justSpawned && allAtRest()) {
            justSpawned = true;
            reset()
            rollSound.play()

            let degrees45 = Math.PI / 4
            for (var index = 0; index < numberOfDice; index++) {
                let initialPosition = Qt.vector3d(0.11 * Math.cos(
                                                      index / degrees45),
                                                  index * 2.1, 0)
                createDie(initialPosition, physicsMaterial, rollForce, diceWidth)
            }
            spawnTimer.start()
        }
    }

    Timer {
        id: spawnTimer
        interval: 500
        running: false
        repeat: false
        onRunningChanged: {
            if (!running)
                shapeSpawner.justSpawned = false
        }
    }

    function reset() {
        dices.forEach(die => {
                          die.destroy()
                      })
        dices = []
    }

    function allAtRest() {
        let atRest = true
        dices.forEach(die => {
                          atRest &= die.atRest
                      })
        return atRest
    }

    function setDiceWidth(diceWidth) {
        dices.forEach(die => { die.diceWidth = diceWidth; })
    }
}
