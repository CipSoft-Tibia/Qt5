// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtGraphs
import QtTest 1.1

Item {
    id: top
    height: 150
    width: 150

    // Scene3D is uncreatable, so it needs to be accessed via a graph
//    TODO: Needs either redoing, or fixing the code; see QTBUG-110000 & QTBUG-110001
//    Bars3D {
//        id: initial
//    }

//    Bars3D {
//        id: initialized
//        scene.activeCamera: Camera3D { zoomLevel: 200 }
//        scene.devicePixelRatio: Screen.devicePixelRatio
//        scene.graphPositionQuery: Qt.point(0, 0)
//        scene.primarySubViewport: Qt.rect(0, 0, 50, 50)
//        scene.secondarySubViewport: Qt.rect(50, 50, 100, 100)
//        scene.secondarySubviewOnTop: false
//        scene.selectionQueryPosition: Qt.point(0, 0)
//        scene.slicingActive: true
//    }

//    Bars3D {
//        id: change
//    }

//    Bars3D {
//        id: invalid
//    }

//    TODO: Needs either redoing, or fixing the code; see QTBUG-110000 & QTBUG-110001
//    TestCase {
//        name: "Scene3D Initial"

//        function test_initial() {
//            verify(initial.scene.activeCamera)
//            verify(initial.scene.activeLight)
//            compare(initial.scene.devicePixelRatio, Screen.devicePixelRatio)
//            compare(initial.scene.graphPositionQuery, Qt.point(-1, -1))
//            compare(initial.scene.invalidSelectionPoint, Qt.point(-1, -1))
//            compare(initial.scene.primarySubViewport.x, 0)
//            compare(initial.scene.primarySubViewport.y, 0)
//            compare(initial.scene.primarySubViewport.width, 0)
//            compare(initial.scene.primarySubViewport.height, 0)
//            compare(initial.scene.secondarySubViewport.x, 0)
//            compare(initial.scene.secondarySubViewport.y, 0)
//            compare(initial.scene.secondarySubViewport.width, 0)
//            compare(initial.scene.secondarySubViewport.height, 0)
//            compare(initial.scene.secondarySubviewOnTop, true)
//            compare(initial.scene.selectionQueryPosition, Qt.point(-1, -1))
//            compare(initial.scene.slicingActive, false)
//            compare(initial.scene.viewport.x, 0)
//            compare(initial.scene.viewport.y, 0)
//            compare(initial.scene.viewport.width, 0)
//            compare(initial.scene.viewport.height, 0)
//        }
//    }

//    TODO: Needs either redoing, or fixing the code; see QTBUG-110000 & QTBUG-110001
//    TestCase {
//        name: "Scene3D Initialized"

//        function test_initialized() {
//            compare(initialized.scene.activeCamera.zoomLevel, 200)
//            compare(initialized.scene.devicePixelRatio, Screen.devicePixelRatio)
//            compare(initialized.scene.graphPositionQuery, Qt.point(0, 0))
//            compare(initialized.scene.primarySubViewport.x, 0)
//            compare(initialized.scene.primarySubViewport.y, 0)
//            compare(initialized.scene.primarySubViewport.width, 50)
//            compare(initialized.scene.primarySubViewport.height, 50)
//            compare(initialized.scene.secondarySubViewport.x, 50)
//            compare(initialized.scene.secondarySubViewport.y, 50)
//            compare(initialized.scene.secondarySubViewport.width, 100)
//            compare(initialized.scene.secondarySubViewport.height, 100)
//            compare(initialized.scene.secondarySubviewOnTop, false)
//            compare(initialized.scene.selectionQueryPosition, Qt.point(0, 0))
//            compare(initialized.scene.slicingActive, true)
//            compare(initialized.scene.viewport.x, 0)
//            compare(initialized.scene.viewport.y, 0)
//            compare(initialized.scene.viewport.width, 150)
//            compare(initialized.scene.viewport.height, 150)
//        }
//    }

//    TODO: Needs either redoing, or fixing the code; see QTBUG-110000 & QTBUG-110001
//    TestCase {
//        name: "Scene3D Change"

//        Camera3D {
//            id: camera1
//            zoomLevel: 200
//        }

//        function test_change() {
//            change.scene.activeCamera = camera1
//            change.scene.devicePixelRatio = 2.0
//            change.scene.graphPositionQuery = Qt.point(0, 0)
//            change.scene.primarySubViewport = Qt.rect(0, 0, 50, 50)
//            change.scene.secondarySubViewport = Qt.rect(50, 50, 100, 100)
//            change.scene.secondarySubviewOnTop = false
//            change.scene.selectionQueryPosition = Qt.point(0, 0) // TODO: When doing signal checks, add tests to check that queries return something (asynchronously)
//            change.scene.slicingActive = true

//            compare(change.scene.activeCamera.zoomLevel, 200)
//            compare(change.scene.devicePixelRatio, 2.0)
//            compare(change.scene.graphPositionQuery, Qt.point(0, 0))
//            compare(change.scene.primarySubViewport.x, 0)
//            compare(change.scene.primarySubViewport.y, 0)
//            compare(change.scene.primarySubViewport.width, 50)
//            compare(change.scene.primarySubViewport.height, 50)
//            compare(change.scene.secondarySubViewport.x, 50)
//            compare(change.scene.secondarySubViewport.y, 50)
//            compare(change.scene.secondarySubViewport.width, 100)
//            compare(change.scene.secondarySubViewport.height, 100)
//            compare(change.scene.secondarySubviewOnTop, false)
//            compare(change.scene.selectionQueryPosition, Qt.point(0, 0))
//            compare(change.scene.slicingActive, true)
//            compare(change.scene.viewport.x, 0)
//            compare(change.scene.viewport.y, 0)
//            compare(change.scene.viewport.width, 150)
//            compare(change.scene.viewport.height, 150)
//        }
//    }

//    TODO: Needs either redoing, or fixing the code; see QTBUG-110000 & QTBUG-110001
//    TestCase {
//        name: "Scene3D Invalid"

//        function test_invalid() {
//            invalid.scene.primarySubViewport = Qt.rect(0, 0, -50, -50)
//            compare(invalid.scene.primarySubViewport.x, 0)
//            compare(invalid.scene.primarySubViewport.y, 0)
//            compare(invalid.scene.primarySubViewport.width, 0)
//            compare(invalid.scene.primarySubViewport.height, 0)
//        }
//    }
}
