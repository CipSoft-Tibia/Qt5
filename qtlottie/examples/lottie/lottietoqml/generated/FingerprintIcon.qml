// Copyright (C) 2025 Solitudinem
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file FingerprintIcon.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 400
    implicitHeight: 400
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 400; yScale: height / 400 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "Layer 2 Outlines"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            TransformGroup {
                id: _qt_layer0_transform_group_0
                Translate { x: -45.303; y: -53.201 }
                Scale { id: _qt_layer0_transform_0_2}
                Translate { x: 198.824; y: 208.036 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer0_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer0_transform_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                loops: 1
                ParallelAnimation {
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer0_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer0_transform_0_2.yScale = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 267
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 267
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 1.7
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 1.7
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 1.78
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 1.78
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2500 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer0_transform_base_group.deactivateOverride(_qt_layer0_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node1
            transform: TransformGroup {
                id: _qt_node1_transform_base_group
                Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 45.303, 53.201)}
            }
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "transparent"
                fillColor: "#ff857af8"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 34.033 45.052 L -34.033 45.052 Q -38.5976 45.052 -41.8252 41.8247 Q -45.053 38.5974 -45.053 34.033 L -45.053 -34.034 Q -45.053 -38.5983 -41.8252 -41.8252 Q -38.5977 -45.052 -34.033 -45.052 L 34.033 -45.052 Q 38.5977 -45.052 41.8252 -41.8252 Q 45.053 -38.5983 45.053 -34.034 L 45.053 34.033 Q 45.053 38.5974 41.8252 41.8247 Q 38.5976 45.052 34.033 45.052 " }
            }
        }
    }
    LayerItem {
        objectName: "Shape Layer 1"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            Matrix4x4 { id: _qt_layer1_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.87, 0, 0, 0.87, 198.381, 197.917)}
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer1_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer1_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer1.visible = false
                    }
                    PauseAnimation { duration: 500 }
                    ScriptAction {
                        script:_qt_layer1.visible = true
                    }
                    PauseAnimation { duration: 2500 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node2
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -38.25 -41 Q -33.875 -45.0625 -26.5625 -49 Q -11.9375 -56.875 2.75 -56.25 Q 17.4375 -55.625 28.9375 -47.875 Q 34.6875 -44 37.5 -40.25 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_1_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_1_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_1.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_1.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -53.75 -6.5 Q -52.1577 -10.4167 -46.6741 -15.9689 Q -35.7069 -27.0731 -16.25 -35.25 Q 4.22269 -43.8535 22.2969 -33.0103 Q 47.4622 -17.9128 51 32 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_2_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_2_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_2.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_2.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node4
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -56.5 13 Q -52.719 12.784 -26.5 -11.25 Q -12.3975 -24.1773 6.48212 -20.1876 Q 26.0799 -16.0461 29.25 3.25 Q 34.5412 35.4594 42.5498 44.4506 Q 46.554 48.9463 49.5 47 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_3_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_3_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_3.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_3.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node5
            ShapePath {
                id: _qt_shapePath_4
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -54 35.75 Q -44.1123 23.871 -31.4809 13.8378 Q -6.21812 -6.22875 7.5 3 Q 13.547 7.06801 17.8543 25.207 Q 24.0352 51.2368 36.5 59.5 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_4_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_4_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_4.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_4.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node6
            ShapePath {
                id: _qt_shapePath_5
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -46.25 51 Q -38.3125 40.6875 -28.375 32 Q -8.5 14.625 1.5 22.75 Q 5.17755 25.738 4.17312 34.7246 Q 3.20805 43.3592 -1.25 51.5 Q -4.68625 57.775 -13.9368 65.015 Q -18.562 68.635 -22.5 71 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_5_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_5_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_5.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_5.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node7
            ShapePath {
                id: _qt_shapePath_6
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -36.25 59.125 Q -34.095 58.9145 -31.2519 58.1217 Q -25.5656 56.5363 -22.125 53.625 Q -16.0313 48.4688 -13.5938 43.0313 Q -12.375 40.3125 -12.375 38.625 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_6_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_6_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_6.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_6.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node8
            ShapePath {
                id: _qt_shapePath_7
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -10.25 81.5 Q -8.9375 79.125 -6.71875 76.9375 Q -2.28125 72.5625 2.25 73.5 Q 6.78125 74.4375 7.46875 78.3125 Q 7.8125 80.25 7.25 82 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_7_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_7_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_7.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_7.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node9
            ShapePath {
                id: _qt_shapePath_8
                strokeColor: "#ffffffff"
                strokeWidth: 8
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 4
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 20.75 72.25 Q 11 59.5 11 51.25 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_8_trim_start_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_8_trim_start_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_8.trim.start = 0
                        }
                        PauseAnimation { duration: 500 }
                        PropertyAnimation {
                            duration: 500
                            target: _qt_shapePath_8.trim
                            property: "start"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 2000 }
                    }
                }
            }
        }
    }
    LayerItem {
        objectName: "Layer 2 Outlines 2"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            TransformGroup {
                id: _qt_layer2_transform_group_0
                Translate { x: -81.669; y: -36.331 }
                Scale { id: _qt_layer2_transform_0_2}
                Translate { x: 263.283; y: 180.22 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer2_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer2_transform_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                loops: 1
                ParallelAnimation {
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer2_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer2_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 933 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 1.78
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 1.78
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1733 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer2_transform_base_group.deactivateOverride(_qt_layer2_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer2_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer2_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer2.visible = false
                    }
                    PauseAnimation { duration: 933 }
                    ScriptAction {
                        script:_qt_layer2.visible = true
                    }
                    PauseAnimation { duration: 2067 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node10
            transform: TransformGroup {
                id: _qt_node10_transform_base_group
                Matrix4x4 { id: _qt_node10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 90.683, 18.819)}
            }
            ShapePath {
                id: _qt_shapePath_9
                strokeColor: "transparent"
                fillColor: "#ffffc840"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 14.239 -18.569 L -14.242 -18.569 Q -16.9255 -18.569 -18.8226 -16.6706 Q -20.719 -14.773 -20.719 -12.091 L -20.719 3.349 Q -20.719 6.03277 -18.8226 7.92975 Q -16.926 9.827 -14.242 9.827 L -9.563 9.827 L -9.563 18.569 L -0.667 9.827 L 14.239 9.827 Q 16.9227 9.827 18.8207 7.92975 Q 20.719 6.03231 20.719 3.349 L 20.719 -12.091 Q 20.719 -14.7726 18.8207 -16.6706 Q 16.9222 -18.569 14.239 -18.569 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node11
            transform: TransformGroup {
                id: _qt_node11_transform_base_group
                Matrix4x4 { id: _qt_node11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 90.893, 14.252)}
            }
            ShapePath {
                id: _qt_shapePath_10
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -2.439 9.049 Q -3.53615 9.049 -4.319 8.291 L -9.889 2.904 Q -10.6945 2.12478 -10.7129 1.00513 Q -10.7313 -0.114467 -9.952 -0.92 Q -9.17288 -1.72237 -8.0535 -1.74038 Q -6.93438 -1.75838 -6.131 -0.982 L -2.642 2.389 L 5.892 -7.76 Q 6.61006 -8.61537 7.7265 -8.71187 Q 8.84362 -8.80844 9.702 -8.088 Q 10.5574 -7.36693 10.6539 -6.251 Q 10.7504 -5.13438 10.03 -4.279 L -0.374 8.088 Q -1.12839 8.99013 -2.298 9.045 Q -2.345 9.049 -2.439 9.049 " }
            }
        }
    }
    LayerItem {
        objectName: "Ð·Ð² Outlines"
        id: _qt_layer3
        transform: TransformGroup {
            id: _qt_layer3_transform_base_group
            TransformGroup {
                id: _qt_layer3_transform_group_0
                Translate { x: -712.659; y: -465.298 }
                Scale { id: _qt_layer3_transform_0_2}
                Translate { x: 163.796; y: 343.259 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer3_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer3_transform_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                loops: 1
                ParallelAnimation {
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer3_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer3_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1733 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 100 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer3_transform_base_group.deactivateOverride(_qt_layer3_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer3_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer3_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer3.visible = false
                    }
                    PauseAnimation { duration: 1733 }
                    ScriptAction {
                        script:_qt_layer3.visible = true
                    }
                    PauseAnimation { duration: 1267 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node12
            transform: TransformGroup {
                id: _qt_node12_transform_base_group
                Matrix4x4 { id: _qt_node12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 712.901, 464.848)}
            }
            ShapePath {
                id: _qt_shapePath_11
                strokeColor: "transparent"
                fillColor: "#ffffcf64"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.246 -0.252 L 12.75 -0.125 L 13.254 0.002 L 12.75 0.127 L 12.246 0.252 Q 7.81891 1.36215 4.58975 4.58975 Q 1.35985 7.81809 0.25 12.244 L 0.125 12.748 L 0 13.252 L -0.254 12.244 Q -1.36157 7.81822 -4.59062 4.58975 Q -7.81878 1.36218 -12.246 0.252 L -13.254 0.002 L -12.75 -0.125 L -12.246 -0.252 Q -7.81974 -1.36194 -4.59062 -4.59125 Q -1.36181 -7.82026 -0.254 -12.244 L 0 -13.252 L 0.25 -12.244 Q 1.36009 -7.82013 4.58975 -4.59125 Q 7.81987 -1.36191 12.246 -0.252 " }
            }
        }
    }
    LayerItem {
        objectName: "Ð·Ð² Outlines 2"
        id: _qt_layer4
        transform: TransformGroup {
            id: _qt_layer4_transform_base_group
            TransformGroup {
                id: _qt_layer4_transform_group_0
                Translate { x: -751.142; y: -312.77 }
                Scale { id: _qt_layer4_transform_0_2}
                Translate { x: 232.296; y: 71.759 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer4_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer4_transform_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                loops: 1
                ParallelAnimation {
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer4_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer4_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1233 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 100 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer4_transform_base_group.deactivateOverride(_qt_layer4_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer4_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer4_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer4.visible = false
                    }
                    PauseAnimation { duration: 1233 }
                    ScriptAction {
                        script:_qt_layer4.visible = true
                    }
                    PauseAnimation { duration: 1767 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node13
            transform: TransformGroup {
                id: _qt_node13_transform_base_group
                Matrix4x4 { id: _qt_node13_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 751.422, 311.793)}
            }
            ShapePath {
                id: _qt_shapePath_12
                strokeColor: "transparent"
                fillColor: "#ffffcf64"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 9.389 -0.197 L 9.7755 -0.0995 L 10.162 -0.002 L 9.7755 0.096 L 9.389 0.194 Q 5.99375 1.043 3.51837 3.51837 Q 1.043 5.99375 0.194 9.389 L -0.002 10.162 L -0.193 9.389 Q -1.04581 5.99351 -3.52075 3.51837 Q -5.99599 1.04294 -9.388 0.194 L -10.162 -0.002 L -9.775 -0.0995 L -9.388 -0.197 Q -5.99612 -1.04666 -3.52075 -3.52125 Q -1.04584 -5.99538 -0.193 -9.388 L -0.002 -10.162 L 0.096 -9.775 L 0.194 -9.388 Q 1.04303 -5.99562 3.51837 -3.52125 Q 5.99388 -1.04672 9.389 -0.197 " }
            }
        }
    }
    LayerItem {
        objectName: "Ð·Ð² Outlines 3"
        id: _qt_layer5
        transform: TransformGroup {
            id: _qt_layer5_transform_base_group
            TransformGroup {
                id: _qt_layer5_transform_group_0
                Translate { x: -725.861; y: -323.445 }
                Scale { id: _qt_layer5_transform_0_2}
                Translate { x: 187.296; y: 90.759 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer5_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer5_transform_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                loops: 1
                ParallelAnimation {
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer5_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer5_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1567 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 100 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer5_transform_base_group.deactivateOverride(_qt_layer5_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer5_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer5_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer5.visible = false
                    }
                    PauseAnimation { duration: 1567 }
                    ScriptAction {
                        script:_qt_layer5.visible = true
                    }
                    PauseAnimation { duration: 1433 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node14
            transform: TransformGroup {
                id: _qt_node14_transform_base_group
                Matrix4x4 { id: _qt_node14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 725.332, 323.166)}
            }
            ShapePath {
                id: _qt_shapePath_13
                strokeColor: "transparent"
                fillColor: "#ffffcf64"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.334 -0.133 L 6.857 0 L 6.334 0.133 Q 4.04599 0.70369 2.37562 2.37425 Q 0.70556 4.04451 0.131 6.336 L 0.0645 6.5975 L -0.002 6.859 L -0.131 6.336 Q -0.704843 4.04438 -2.37575 2.37425 Q -4.04712 0.703658 -6.338 0.133 L -6.857 0 L -6.338 -0.133 Q -4.04591 -0.707709 -2.37575 -2.37613 Q -0.704541 -4.04558 -0.131 -6.336 L -0.002 -6.859 L 0.0645 -6.5975 L 0.131 -6.336 Q 0.705258 -4.04571 2.37562 -2.37613 Q 4.04479 -0.707742 6.334 -0.133 " }
            }
        }
    }
    LayerItem {
        objectName: "Ð·Ð² Outlines 4"
        id: _qt_layer6
        transform: TransformGroup {
            id: _qt_layer6_transform_base_group
            TransformGroup {
                id: _qt_layer6_transform_group_0
                Translate { x: -673.894; y: -440.86 }
                Scale { id: _qt_layer6_transform_0_2}
                Translate { x: 94.796; y: 299.759 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer6_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer6_transform_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                loops: 1
                ParallelAnimation {
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer6_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer6_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1.85
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 100 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer6_transform_base_group.deactivateOverride(_qt_layer6_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer6_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer6_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer6.visible = false
                    }
                    PauseAnimation { duration: 1400 }
                    ScriptAction {
                        script:_qt_layer6.visible = true
                    }
                    PauseAnimation { duration: 1600 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node15
            transform: TransformGroup {
                id: _qt_node15_transform_base_group
                Matrix4x4 { id: _qt_node15_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 674.112, 440.422)}
            }
            ShapePath {
                id: _qt_shapePath_14
                strokeColor: "transparent"
                fillColor: "#ffffcf64"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.722 -0.138 L 6.9995 -0.0675 L 7.277 0.003 L 6.722 0.139 Q 4.29087 0.748657 2.51813 2.52063 Q 0.745843 4.29213 0.136 6.721 L 0.0675 6.9985 L -0.001 7.276 L -0.141 6.721 Q -0.74751 4.29346 -2.52113 2.52063 Q -4.29354 0.748992 -6.723 0.139 L -7.278 0.003 L -6.723 -0.138 Q -4.29378 -0.747181 -2.52113 -2.51925 Q -0.74757 -4.29222 -0.141 -6.72 L -0.001 -7.275 L 0.0675 -6.9975 L 0.136 -6.72 Q 0.745904 -4.29089 2.51813 -2.51925 Q 4.29111 -0.746847 6.722 -0.138 " }
            }
        }
    }
}
