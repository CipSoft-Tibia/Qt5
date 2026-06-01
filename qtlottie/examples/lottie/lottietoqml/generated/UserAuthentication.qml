// Copyright (C) 2025 Musa Adanur
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file UserAuthentication.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 600
    implicitHeight: 600
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 600; yScale: height / 600 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "mavilik2"
        id: _qt_layer0
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer0_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer0_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer0.visible = false
                    }
                    PauseAnimation { duration: 1017 }
                    ScriptAction {
                        script:_qt_layer0.visible = true
                    }
                    PauseAnimation { duration: 6983 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node1
            transform: TransformGroup {
                id: _qt_node1_transform_base_group
                Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 229.133, 272.251)}
            }
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "transparent"
                fillColor: "#0007fcf3"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 102.189 78.039 L 40.811 78.078 Q 34.5974 78.078 30.2041 73.6845 Q 25.811 69.2912 25.811 63.078 L 25.811 -7 Q 25.811 -13.2132 30.2041 -17.6065 Q 34.5974 -22 40.811 -22 L 102.189 -22.039 Q 108.402 -22.039 112.796 -17.6455 Q 117.189 -13.252 117.189 -7.039 L 117.189 63.039 Q 117.189 69.252 112.796 73.6455 Q 108.402 78.039 102.189 78.039 " }
            }
        }
    }
    LayerItem {
        objectName: "kmlk Outlines"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            TransformGroup {
                id: _qt_layer1_transform_group_0
                Translate { x: -196.133; y: -282.251 }
                Rotation { id: _qt_layer1_transform_0_1 }
                Translate { id: _qt_layer1_transform_0_0 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer1_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer1_transform_animation
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
                                script:_qt_layer1_transform_0_0.x = 168.133
                            }
                            ScriptAction {
                                script:_qt_layer1_transform_0_0.y = 237.251
                            }
                        }
                        PauseAnimation { duration: 2517 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 198.959
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.511, 0, 0.718, 0.891, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 284.562
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.511, 0, 0.718, 0.891, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 196.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.455, 0.66, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 282.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.455, 0.66, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 566
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 232.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.328, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 566
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 338.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.328, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 184 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 251.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.399, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 303.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.399, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 600
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 203.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.379, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 600
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 283.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.379, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 300 }
                        PauseAnimation { duration: 233 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 367
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 142.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 367
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 279.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 417
                                target: _qt_layer1_transform_0_0
                                property: "x"
                                to: 203.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.464, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 417
                                target: _qt_layer1_transform_0_0
                                property: "y"
                                to: 290.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.464, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 350 }
                        PauseAnimation { duration: 1283 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer1_transform_0_1.angle = -26
                            }
                        }
                        PauseAnimation { duration: 2517 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 383
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.481, 0, 0.737, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: -2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2050 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 233
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 10
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.574, 0, 0.57, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 367
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: -69
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 417
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 12
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.464, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 350
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.192, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1283 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer1_transform_base_group.deactivateOverride(_qt_layer1_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer1_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer1_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer1.opacity = 0
                    }
                    PauseAnimation { duration: 2517 }
                    PropertyAnimation {
                        duration: 83
                        target: _qt_layer1
                        property: "opacity"
                        to: 1
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                    }
                    PauseAnimation { duration: 5400 }
                }
            }
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
                    PauseAnimation { duration: 2517 }
                    ScriptAction {
                        script:_qt_layer1.visible = true
                    }
                    PauseAnimation { duration: 5483 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node2
            transform: TransformGroup {
                id: _qt_node2_transform_base_group
                Matrix4x4 { id: _qt_node2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 229.133, 272.251)}
            }
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "transparent"
                fillColor: "#ffebf2f7"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 67.189 50.039 L -67.189 50.039 Q -73.4026 50.039 -77.7959 45.6455 Q -82.189 41.2522 -82.189 35.039 L -82.189 -35.039 Q -82.189 -41.2522 -77.7959 -45.6455 Q -73.4026 -50.039 -67.189 -50.039 L 67.189 -50.039 Q 73.402 -50.039 77.7955 -45.6455 Q 82.189 -41.252 82.189 -35.039 L 82.189 35.039 Q 82.189 41.252 77.7955 45.6455 Q 73.402 50.039 67.189 50.039 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "#ff95a9b7"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 238.644 261.54 L 265 261.54 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node4
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "#ff95a9b7"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 237.368 285.251 L 282.279 285.251 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node5
            transform: TransformGroup {
                id: _qt_node5_transform_base_group
                Matrix4x4 { id: _qt_node5_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 191.496, 291.855)}
            }
            ShapePath {
                id: _qt_shapePath_4
                strokeColor: "#ff1e4b5b"
                strokeWidth: 6
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff1e4b5b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -21.676 6.755 Q -18.9348 0.706708 -13.1365 -2.97787 Q -7.19256 -6.755 -0.001 -6.755 Q 7.19134 -6.755 13.1359 -2.97787 Q 18.9348 0.706749 21.676 6.755 L -21.676 6.755 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node6
            transform: TransformGroup {
                id: _qt_node6_transform_base_group
                Matrix4x4 { id: _qt_node6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 191.496, 257.388)}
            }
            ShapePath {
                id: _qt_shapePath_5
                strokeColor: "#ff1e4b5b"
                strokeWidth: 6
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "#ff1e4b5b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.326 0 Q 12.326 5.10583 8.71562 8.716 Q 5.10542 12.326 0 12.326 Q -5.106 12.326 -8.716 8.716 Q -12.326 5.106 -12.326 0 Q -12.326 -5.10542 -8.716 -8.71562 Q -5.10583 -12.326 0 -12.326 Q 5.10525 -12.326 8.71562 -8.71562 Q 12.326 -5.10525 12.326 0 " }
            }
        }
    }
    LayerItem {
        objectName: "phone Outlines"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            TransformGroup {
                id: _qt_layer2_transform_group_0
                Translate { x: -300; y: -300 }
                Scale { id: _qt_layer2_transform_0_2}
                Translate { x: 300; y: 300 }
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
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 1.04
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.469, 0, 0.554, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 1.04
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.469, 0, 0.554, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 0.97
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.309, 0, 0.647, 1.019, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 0.97
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.309, 0, 0.647, 1.019, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.289, 0.033, 0.653, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.289, 0.033, 0.653, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 7117 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer2_transform_base_group.deactivateOverride(_qt_layer2_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node7
            transform: TransformGroup {
                id: _qt_node7_transform_base_group
                Matrix4x4 { id: _qt_node7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 300, 300)}
            }
            ShapePath {
                id: _qt_shapePath_6
                strokeColor: "transparent"
                fillColor: "#ccffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 73.781 -104.836 L 74 104.654 Q 74.0105 114.977 67.3002 122.285 Q 60.5899 129.593 51.089 129.605 L 0.135 129.667 L -50.819 129.73 Q -60.32 129.742 -67.0452 122.45 Q -73.7705 115.159 -73.781 104.834 L -74 -104.654 Q -74.0105 -114.978 -67.3004 -122.285 Q -60.5904 -129.593 -51.09 -129.605 L -0.136 -129.667 L 50.818 -129.73 Q 60.3189 -129.742 67.0444 -122.451 Q 73.7698 -115.16 73.781 -104.836 " }
            }
        }
    }
    LayerItem {
        objectName: "phone Outlines 2"
        id: _qt_layer3
        transform: TransformGroup {
            id: _qt_layer3_transform_base_group
            TransformGroup {
                id: _qt_layer3_transform_group_0
                Translate { x: -300; y: -300 }
                Scale { id: _qt_layer3_transform_0_2}
                Translate { x: 300; y: 300 }
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
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1.04
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.469, 0, 0.554, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1.04
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.469, 0, 0.554, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 0.97
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.309, 0, 0.647, 1.019, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 0.97
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.309, 0, 0.647, 1.019, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.289, 0.033, 0.653, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.289, 0.033, 0.653, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 7117 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer3_transform_base_group.deactivateOverride(_qt_layer3_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node8
            transform: TransformGroup {
                id: _qt_node8_transform_base_group
                Matrix4x4 { id: _qt_node8_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 300, 300)}
            }
            ShapePath {
                id: _qt_shapePath_7
                strokeColor: "#ff1f212b"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#00000000"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 73.781 -104.836 L 74 104.654 Q 74.0105 114.977 67.3002 122.285 Q 60.5899 129.593 51.089 129.605 L 0.135 129.667 L -50.819 129.73 Q -60.32 129.742 -67.0452 122.45 Q -73.7705 115.159 -73.781 104.834 L -74 -104.654 Q -74.0105 -114.978 -67.3004 -122.285 Q -60.5904 -129.593 -51.09 -129.605 L -0.136 -129.667 L 50.818 -129.73 Q 60.3189 -129.742 67.0444 -122.451 Q 73.7698 -115.16 73.781 -104.836 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node9
            transform: TransformGroup {
                id: _qt_node9_transform_base_group
                Matrix4x4 { id: _qt_node9_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 299.997, 175.861)}
            }
            ShapePath {
                id: _qt_shapePath_8
                strokeColor: "#ff1f212b"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 33.786 -3.246 Q 33.8708 -0.651631 32.2439 1.24913 Q 30.6169 3.15 28.229 3.246 L -28.229 3.246 Q -30.6162 3.15 -32.243 1.24913 Q -33.8698 -0.651718 -33.785 -3.246 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node10
            transform: TransformGroup {
                id: _qt_node10_transform_base_group
                TransformGroup {
                    id: _qt_node10_transform_group_0
                    Translate { x: -300; y: -300.001 }
                    Scale { id: _qt_node10_transform_0_5}
                    Translate { x: 300; y: 300.001 }
                }
                Matrix4x4 { id: _qt_node10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 263.443, 259.402)}
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node10_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node10_transform_animation
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
                                    script:_qt_node10_transform_0_5.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node10_transform_0_5.yScale = 0
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node10_transform_0_5
                                    property: "xScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node10_transform_0_5
                                    property: "yScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node10_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node10_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1766 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node10_transform_0_5
                                    property: "xScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node10_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5233 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node10_transform_base_group.deactivateOverride(_qt_node10_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node10_opacity_animation.restart() } }
            ParallelAnimation {
                id: _qt_node10_opacity_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_node10.opacity = 1
                        }
                        PauseAnimation { duration: 2450 }
                        PropertyAnimation {
                            duration: 167
                            target: _qt_node10
                            property: "opacity"
                            to: 0
                        }
                        PauseAnimation { duration: 5383 }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_9
                strokeColor: "#ffb8ccd8"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -8.443 8.443 L -8.443 1.34 Q -8.443 -2.71235 -5.57762 -5.57813 Q -2.71215 -8.444 1.34 -8.444 L 8.444 -8.444 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node11
            transform: TransformGroup {
                id: _qt_node11_transform_base_group
                TransformGroup {
                    id: _qt_node11_transform_group_0
                    Translate { x: -300; y: -300.001 }
                    Scale { id: _qt_node11_transform_0_5}
                    Translate { x: 300; y: 300.001 }
                }
                Matrix4x4 { id: _qt_node11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 336.556, 259.402)}
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node11_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node11_transform_animation
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
                                    script:_qt_node11_transform_0_5.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node11_transform_0_5.yScale = 0
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node11_transform_0_5
                                    property: "xScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node11_transform_0_5
                                    property: "yScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node11_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node11_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1766 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node11_transform_0_5
                                    property: "xScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node11_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5233 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node11_transform_base_group.deactivateOverride(_qt_node11_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node11_opacity_animation.restart() } }
            ParallelAnimation {
                id: _qt_node11_opacity_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_node11.opacity = 1
                        }
                        PauseAnimation { duration: 2450 }
                        PropertyAnimation {
                            duration: 167
                            target: _qt_node11
                            property: "opacity"
                            to: 0
                        }
                        PauseAnimation { duration: 5383 }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_10
                strokeColor: "#ffb8ccd8"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 8.444 8.443 L 8.444 1.34 Q 8.444 -2.71227 5.57875 -5.57813 Q 2.71348 -8.444 -1.338 -8.444 L -8.444 -8.444 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node12
            transform: TransformGroup {
                id: _qt_node12_transform_base_group
                TransformGroup {
                    id: _qt_node12_transform_group_0
                    Translate { x: -300; y: -300.001 }
                    Scale { id: _qt_node12_transform_0_5}
                    Translate { x: 300; y: 300.001 }
                }
                Matrix4x4 { id: _qt_node12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 336.556, 340.598)}
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node12_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node12_transform_animation
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
                                    script:_qt_node12_transform_0_5.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node12_transform_0_5.yScale = 0
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node12_transform_0_5
                                    property: "xScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node12_transform_0_5
                                    property: "yScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node12_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node12_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1766 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node12_transform_0_5
                                    property: "xScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node12_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5233 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node12_transform_base_group.deactivateOverride(_qt_node12_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node12_opacity_animation.restart() } }
            ParallelAnimation {
                id: _qt_node12_opacity_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_node12.opacity = 1
                        }
                        PauseAnimation { duration: 2450 }
                        PropertyAnimation {
                            duration: 167
                            target: _qt_node12
                            property: "opacity"
                            to: 0
                        }
                        PauseAnimation { duration: 5383 }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_11
                strokeColor: "#ffb8ccd8"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 8.444 -8.444 L 8.444 -1.34 Q 8.444 2.71285 5.57875 5.5785 Q 2.71365 8.444 -1.338 8.444 L -8.444 8.444 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node13
            transform: TransformGroup {
                id: _qt_node13_transform_base_group
                TransformGroup {
                    id: _qt_node13_transform_group_0
                    Translate { x: -300; y: -300.001 }
                    Scale { id: _qt_node13_transform_0_5}
                    Translate { x: 300; y: 300.001 }
                }
                Matrix4x4 { id: _qt_node13_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 263.443, 340.598)}
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node13_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node13_transform_animation
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
                                    script:_qt_node13_transform_0_5.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node13_transform_0_5.yScale = 0
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node13_transform_0_5
                                    property: "xScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_node13_transform_0_5
                                    property: "yScale"
                                    to: 1.05
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.426, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node13_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node13_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.465, 0, 0.665, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1766 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node13_transform_0_5
                                    property: "xScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node13_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5233 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node13_transform_base_group.deactivateOverride(_qt_node13_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node13_opacity_animation.restart() } }
            ParallelAnimation {
                id: _qt_node13_opacity_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_node13.opacity = 1
                        }
                        PauseAnimation { duration: 2450 }
                        PropertyAnimation {
                            duration: 167
                            target: _qt_node13
                            property: "opacity"
                            to: 0
                        }
                        PauseAnimation { duration: 5383 }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_12
                strokeColor: "#ffb8ccd8"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -8.443 -8.444 L -8.443 -1.34 Q -8.443 2.71292 -5.57762 5.5785 Q -2.71233 8.444 1.34 8.444 L 8.444 8.444 " }
            }
        }
    }
    LayerItem {
        objectName: "kmlk Outlines 2"
        id: _qt_layer4
        transform: TransformGroup {
            id: _qt_layer4_transform_base_group
            TransformGroup {
                id: _qt_layer4_transform_group_0
                Translate { x: -196.133; y: -282.251 }
                Rotation { id: _qt_layer4_transform_0_1 }
                Translate { id: _qt_layer4_transform_0_0 }
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
                                script:_qt_layer4_transform_0_0.x = 168.133
                            }
                            ScriptAction {
                                script:_qt_layer4_transform_0_0.y = 237.251
                            }
                        }
                        PauseAnimation { duration: 2517 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 198.959
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.511, 0, 0.718, 0.891, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 284.562
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.511, 0, 0.718, 0.891, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 196.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.455, 0.66, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 250
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 282.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.455, 0.66, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 566
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 232.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.328, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 566
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 338.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.328, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 184 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 251.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.399, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 483
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 303.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.399, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 600
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 203.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.379, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 600
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 283.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.379, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 300 }
                        PauseAnimation { duration: 233 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 367
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 142.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 367
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 279.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 417
                                target: _qt_layer4_transform_0_0
                                property: "x"
                                to: 203.133
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.464, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 417
                                target: _qt_layer4_transform_0_0
                                property: "y"
                                to: 290.251
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.464, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 350 }
                        PauseAnimation { duration: 1283 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer4_transform_0_1.angle = -26
                            }
                        }
                        PauseAnimation { duration: 2517 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 383
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.481, 0, 0.737, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: -2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2050 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 233
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 10
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.574, 0, 0.57, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 367
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: -69
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 417
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 12
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.464, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 350
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.192, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1283 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer4_transform_base_group.deactivateOverride(_qt_layer4_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer4_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer4_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer4.opacity = 0
                    }
                    PauseAnimation { duration: 2517 }
                    PropertyAnimation {
                        duration: 83
                        target: _qt_layer4
                        property: "opacity"
                        to: 1
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                    }
                    PauseAnimation { duration: 5400 }
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
                    PauseAnimation { duration: 5950 }
                    ScriptAction {
                        script:_qt_layer4.visible = true
                    }
                    PauseAnimation { duration: 2050 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node14
            transform: TransformGroup {
                id: _qt_node14_transform_base_group
                Matrix4x4 { id: _qt_node14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 229.133, 272.251)}
            }
            ShapePath {
                id: _qt_shapePath_13
                strokeColor: "transparent"
                fillColor: "#ffebf2f7"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 67.189 50.039 L -67.189 50.039 Q -73.4026 50.039 -77.7959 45.6455 Q -82.189 41.2522 -82.189 35.039 L -82.189 -35.039 Q -82.189 -41.2522 -77.7959 -45.6455 Q -73.4026 -50.039 -67.189 -50.039 L 67.189 -50.039 Q 73.402 -50.039 77.7955 -45.6455 Q 82.189 -41.252 82.189 -35.039 L 82.189 35.039 Q 82.189 41.252 77.7955 45.6455 Q 73.402 50.039 67.189 50.039 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node15
            ShapePath {
                id: _qt_shapePath_14
                strokeColor: "#ff95a9b7"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 238.644 261.54 L 265 261.54 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node16
            ShapePath {
                id: _qt_shapePath_15
                strokeColor: "#ff95a9b7"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 237.368 285.251 L 282.279 285.251 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node17
            transform: TransformGroup {
                id: _qt_node17_transform_base_group
                Matrix4x4 { id: _qt_node17_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 191.496, 291.855)}
            }
            ShapePath {
                id: _qt_shapePath_16
                strokeColor: "#ff1e4b5b"
                strokeWidth: 6
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff1e4b5b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -21.676 6.755 Q -18.9348 0.706708 -13.1365 -2.97787 Q -7.19256 -6.755 -0.001 -6.755 Q 7.19134 -6.755 13.1359 -2.97787 Q 18.9348 0.706749 21.676 6.755 L -21.676 6.755 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node18
            transform: TransformGroup {
                id: _qt_node18_transform_base_group
                Matrix4x4 { id: _qt_node18_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 191.496, 257.388)}
            }
            ShapePath {
                id: _qt_shapePath_17
                strokeColor: "#ff1e4b5b"
                strokeWidth: 6
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "#ff1e4b5b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.326 0 Q 12.326 5.10583 8.71562 8.716 Q 5.10542 12.326 0 12.326 Q -5.106 12.326 -8.716 8.716 Q -12.326 5.106 -12.326 0 Q -12.326 -5.10542 -8.716 -8.71562 Q -5.10583 -12.326 0 -12.326 Q 5.10525 -12.326 8.71562 -8.71562 Q 12.326 -5.10525 12.326 0 " }
            }
        }
    }
    LayerItem {
        objectName: "face Outlines"
        id: _qt_layer5
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
                    PauseAnimation { duration: 667 }
                    ScriptAction {
                        script:_qt_layer5.visible = true
                    }
                    PauseAnimation { duration: 7333 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node19
            transform: TransformGroup {
                id: _qt_node19_transform_base_group
                TransformGroup {
                    id: _qt_node19_transform_group_0
                    Scale { id: _qt_node19_transform_0_5}
                    Translate { x: 286.683; y: 286.958 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node19_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node19_transform_animation
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
                                    script:_qt_node19_transform_0_5.xScale = 1
                                }
                                ScriptAction {
                                    script:_qt_node19_transform_0_5.yScale = 0
                                }
                            }
                            PauseAnimation { duration: 667 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_node19_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.746, 0, 0.565, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_node19_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.746, 0, 0.565, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 100 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 150
                                    target: _qt_node19_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.492, 0, 0.608, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 150
                                    target: _qt_node19_transform_0_5
                                    property: "yScale"
                                    to: 0.25
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.492, 0, 0.608, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node19_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.325, 0, 0.382, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node19_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.325, 0, 0.382, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1200 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 83
                                    target: _qt_node19_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.279, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 83
                                    target: _qt_node19_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.279, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5500 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node19_transform_base_group.deactivateOverride(_qt_node19_transform_group_0)
                        }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_18
                strokeColor: "transparent"
                fillColor: "#ffb8ccd8"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 7.683 0 Q 7.683 3.18242 5.433 5.43262 Q 3.18283 7.683 0 7.683 Q -3.18225 7.683 -5.43262 5.43262 Q -7.683 3.18225 -7.683 0 Q -7.683 -3.18225 -5.43262 -5.43262 Q -3.18225 -7.683 0 -7.683 Q 3.18283 -7.683 5.433 -5.43262 Q 7.683 -3.18242 7.683 0 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node20
            transform: TransformGroup {
                id: _qt_node20_transform_base_group
                TransformGroup {
                    id: _qt_node20_transform_group_0
                    Scale { id: _qt_node20_transform_0_5}
                    Translate { x: 313.317; y: 286.958 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node20_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node20_transform_animation
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
                                    script:_qt_node20_transform_0_5.xScale = 1
                                }
                                ScriptAction {
                                    script:_qt_node20_transform_0_5.yScale = 0
                                }
                            }
                            PauseAnimation { duration: 667 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_node20_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.746, 0, 0.565, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_node20_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.746, 0, 0.565, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 100 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 150
                                    target: _qt_node20_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.492, 0, 0.608, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 150
                                    target: _qt_node20_transform_0_5
                                    property: "yScale"
                                    to: 0.25
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.492, 0, 0.608, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node20_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.325, 0, 0.382, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_node20_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.325, 0, 0.382, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1200 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 83
                                    target: _qt_node20_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.279, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 83
                                    target: _qt_node20_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.279, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5500 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node20_transform_base_group.deactivateOverride(_qt_node20_transform_group_0)
                        }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_19
                strokeColor: "transparent"
                fillColor: "#ffb8ccd8"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 7.683 0 Q 7.683 3.18242 5.433 5.43262 Q 3.18283 7.683 0 7.683 Q -3.18225 7.683 -5.43262 5.43262 Q -7.683 3.18225 -7.683 0 Q -7.683 -3.18225 -5.43262 -5.43262 Q -3.18225 -7.683 0 -7.683 Q 3.18283 -7.683 5.433 -5.43262 Q 7.683 -3.18242 7.683 0 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node21
            transform: TransformGroup {
                id: _qt_node21_transform_base_group
                TransformGroup {
                    id: _qt_node21_transform_group_0
                    Translate { x: -300; y: -317.774 }
                    Scale { id: _qt_node21_transform_0_12}
                    Scale { id: _qt_node21_transform_0_5}
                    Translate { x: 300; y: 318.759 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node21_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node21_transform_animation
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
                                    script:_qt_node21_transform_0_5.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node21_transform_0_5.yScale = 0
                                }
                            }
                            PauseAnimation { duration: 667 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 33
                                    target: _qt_node21_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 33
                                    target: _qt_node21_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1717 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 66
                                    target: _qt_node21_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 66
                                    target: _qt_node21_transform_0_5
                                    property: "yScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5517 }
                        }
                        SequentialAnimation {
                            ParallelAnimation {
                                ScriptAction {
                                    script:_qt_node21_transform_0_12.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node21_transform_0_12.yScale = -0.2
                                }
                            }
                            PauseAnimation { duration: 667 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_node21_transform_0_12
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.547, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_node21_transform_0_12
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.547, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 1584 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 116
                                    target: _qt_node21_transform_0_12
                                    property: "xScale"
                                    to: 0.7
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 116
                                    target: _qt_node21_transform_0_12
                                    property: "yScale"
                                    to: -0.2
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5467 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node21_transform_base_group.deactivateOverride(_qt_node21_transform_group_0)
                        }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_20
                strokeColor: "#ffb8ccd8"
                strokeWidth: 9
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 284.459 314.823 Q 291.142 320.702 299.99 320.724 Q 308.839 320.746 315.541 314.899 " }
            }
        }
    }
    LayerItem {
        objectName: "scan Outlines"
        id: _qt_layer6
        transform: TransformGroup {
            id: _qt_layer6_transform_base_group
            Matrix4x4 { id: _qt_layer6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 0, 178.5)}
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
                    PauseAnimation { duration: 1217 }
                    ScriptAction {
                        script:_qt_layer6.visible = true
                    }
                    PauseAnimation { duration: 6783 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node22
            opacity: 0
            transform: TransformGroup {
                id: _qt_node22_transform_base_group
                TransformGroup {
                    id: _qt_node22_transform_group_0
                    Translate { x: -300; y: -121.5 }
                    Scale { id: _qt_node22_transform_0_12}
                    Translate { id: _qt_node22_transform_0_7 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node22_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node22_transform_animation
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
                                    script:_qt_node22_transform_0_7.x = 300
                                }
                                ScriptAction {
                                    script:_qt_node22_transform_0_7.y = 82.5
                                }
                            }
                            PauseAnimation { duration: 1283 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 400
                                    target: _qt_node22_transform_0_7
                                    property: "x"
                                    to: 300
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 400
                                    target: _qt_node22_transform_0_7
                                    property: "y"
                                    to: 160.5
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 450
                                    target: _qt_node22_transform_0_7
                                    property: "x"
                                    to: 300
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 450
                                    target: _qt_node22_transform_0_7
                                    property: "y"
                                    to: 82.5
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5867 }
                        }
                        SequentialAnimation {
                            ParallelAnimation {
                                ScriptAction {
                                    script:_qt_node22_transform_0_12.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node22_transform_0_12.yScale = 1
                                }
                            }
                            PauseAnimation { duration: 1217 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 116
                                    target: _qt_node22_transform_0_12
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 116
                                    target: _qt_node22_transform_0_12
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 717 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 150
                                    target: _qt_node22_transform_0_12
                                    property: "xScale"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 150
                                    target: _qt_node22_transform_0_12
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 5800 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node22_transform_base_group.deactivateOverride(_qt_node22_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node22_opacity_animation.restart() } }
            ParallelAnimation {
                id: _qt_node22_opacity_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_node22.opacity = 0
                        }
                        PauseAnimation { duration: 1217 }
                        PropertyAnimation {
                            duration: 33
                            target: _qt_node22
                            property: "opacity"
                            to: 1
                        }
                        PauseAnimation { duration: 883 }
                        PropertyAnimation {
                            duration: 67
                            target: _qt_node22
                            property: "opacity"
                            to: 0
                        }
                        PauseAnimation { duration: 5800 }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_21
                strokeColor: "#ff39fff0"
                strokeWidth: 9
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 259 121.5 L 341 121.5 " }
            }
        }
    }
    LayerItem {
        objectName: "success Outlines"
        id: _qt_layer7
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer7_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer7_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer7.visible = false
                    }
                    PauseAnimation { duration: 5950 }
                    ScriptAction {
                        script:_qt_layer7.visible = true
                    }
                    PauseAnimation { duration: 2050 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node23
            transform: TransformGroup {
                id: _qt_node23_transform_base_group
                TransformGroup {
                    id: _qt_node23_transform_group_0
                    Scale { id: _qt_node23_transform_0_5}
                    Rotation { id: _qt_node23_transform_0_1 }
                    Translate { x: 374.073; y: 326.17 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node23_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node23_transform_animation
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
                                    script:_qt_node23_transform_0_1.angle = -218
                                }
                            }
                            PauseAnimation { duration: 5950 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 1233
                                    target: _qt_node23_transform_0_1
                                    property: "angle"
                                    to: 0
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.34, 0, 0.436, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 817 }
                        }
                        SequentialAnimation {
                            ParallelAnimation {
                                ScriptAction {
                                    script:_qt_node23_transform_0_5.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_node23_transform_0_5.yScale = 0
                                }
                            }
                            PauseAnimation { duration: 5950 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 500
                                    target: _qt_node23_transform_0_5
                                    property: "xScale"
                                    to: 1.1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.546, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 500
                                    target: _qt_node23_transform_0_5
                                    property: "yScale"
                                    to: 1.1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.546, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 283
                                    target: _qt_node23_transform_0_5
                                    property: "xScale"
                                    to: 0.95
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 283
                                    target: _qt_node23_transform_0_5
                                    property: "yScale"
                                    to: 0.95
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 284
                                    target: _qt_node23_transform_0_5
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 284
                                    target: _qt_node23_transform_0_5
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 983 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node23_transform_base_group.deactivateOverride(_qt_node23_transform_group_0)
                        }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_22
                strokeColor: "transparent"
                fillColor: "#ff21d66a"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 10.72 -65.679 L 20.602 -53.285 Q 24.682 -48.17 31.225 -48.17 L 47.07 -48.17 Q 52.6981 -48.1722 56.6799 -44.1935 Q 60.6618 -40.2147 60.664 -34.586 Q 60.6655 -33.0543 60.323 -31.553 L 56.797 -16.105 Q 55.3408 -9.72151 59.419 -4.604 L 69.302 7.783 Q 72.8133 12.186 72.1819 17.7826 Q 71.5505 23.3788 67.147 26.889 Q 65.9485 27.8448 64.573 28.507 L 50.297 35.361 Q 44.3978 38.199 42.945 44.583 L 39.418 60.029 Q 38.164 65.5189 33.3955 68.5141 Q 28.6271 71.5093 23.137 70.256 Q 21.6428 69.9141 20.264 69.251 L 5.991 62.396 Q 0.0922086 59.555 -5.805 62.396 L -20.081 69.251 Q -25.1525 71.6938 -30.4653 69.8351 Q -35.778 67.9765 -38.22 62.905 Q -38.8889 61.5188 -39.228 60.029 L -42.754 44.583 Q -44.2066 38.1996 -50.107 35.361 L -64.383 28.507 Q -69.4546 26.0604 -71.311 20.7442 Q -73.1674 15.4279 -70.721 10.355 Q -70.0583 8.98004 -69.104 7.783 L -59.228 -4.604 Q -55.1428 -9.7228 -56.605 -16.105 L -60.133 -31.553 Q -61.3847 -37.0427 -58.3864 -41.8101 Q -55.3883 -46.5772 -49.898 -47.829 Q -48.4027 -48.17 -46.879 -48.17 L -31.035 -48.17 Q -24.4897 -48.1716 -20.405 -53.285 L -10.529 -65.679 Q -7.0181 -70.0794 -1.42463 -70.709 Q 4.16918 -71.3386 8.57 -67.828 Q 9.76434 -66.8758 10.72 -65.679 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node24
            transform: TransformGroup {
                id: _qt_node24_transform_base_group
                Matrix4x4 { id: _qt_node24_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 374.172, 329.99)}
            }
            ShapePath {
                id: _qt_shapePath_23
                strokeColor: "#ffffffff"
                strokeWidth: 10
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -22.808 -1.196 L -15.054 7.5025 L -7.3 16.201 L 22.808 -16.201 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_23_trim_end_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_23_trim_end_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_23.trim.end = 0
                        }
                        PauseAnimation { duration: 6133 }
                        PropertyAnimation {
                            duration: 550
                            target: _qt_shapePath_23.trim
                            property: "end"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.644, 0.356, 0.406, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 1317 }
                    }
                }
            }
        }
    }
    LayerItem {
        objectName: "nfc-seeklogo.com Outlines"
        id: _qt_layer8
        transform: TransformGroup {
            id: _qt_layer8_transform_base_group
            TransformGroup {
                id: _qt_layer8_transform_group_0
                Translate { x: -190; y: -190 }
                Scale { id: _qt_layer8_transform_0_2}
                Translate { x: 300; y: 300 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer8_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer8_transform_animation
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
                                script:_qt_layer8_transform_0_2.xScale = 0.24
                            }
                            ScriptAction {
                                script:_qt_layer8_transform_0_2.yScale = 0.24
                            }
                        }
                        PauseAnimation { duration: 2983 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 600
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 0.2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.525, 0, 0.462, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 600
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 0.2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.525, 0, 0.462, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 567
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 0.24
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.542, 0, 0.443, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 567
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 0.24
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.542, 0, 0.443, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 567
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 0.2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.542, 0, 0.443, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 567
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 0.2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.542, 0, 0.443, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1133
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 0.24
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.299, 0, 0.264, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 1133
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 0.24
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.299, 0, 0.264, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2150 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer8_transform_base_group.deactivateOverride(_qt_layer8_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer8_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer8_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer8.opacity = 0
                    }
                    PauseAnimation { duration: 2983 }
                    PropertyAnimation {
                        duration: 117
                        target: _qt_layer8
                        property: "opacity"
                        to: 1
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                    }
                    PauseAnimation { duration: 2533 }
                    PropertyAnimation {
                        duration: 167
                        target: _qt_layer8
                        property: "opacity"
                        to: 0
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                    }
                    PauseAnimation { duration: 2200 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer8_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer8_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer8.visible = false
                    }
                    PauseAnimation { duration: 2983 }
                    ScriptAction {
                        script:_qt_layer8.visible = true
                    }
                    PauseAnimation { duration: 5017 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node25
            transform: TransformGroup {
                id: _qt_node25_transform_base_group
                TransformGroup {
                    id: _qt_node25_transform_group_0
                    Translate { id: _qt_node25_transform_0_0 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node25_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node25_transform_animation
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
                                    script:_qt_node25_transform_0_0.x = 324.867
                                }
                                ScriptAction {
                                    script:_qt_node25_transform_0_0.y = 190.047
                                }
                            }
                            PauseAnimation { duration: 3117 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node25_transform_0_0
                                    property: "x"
                                    to: 242.867
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.685, 0, 0.477, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node25_transform_0_0
                                    property: "y"
                                    to: 190.047
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.685, 0, 0.477, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 367
                                    target: _qt_node25_transform_0_0
                                    property: "x"
                                    to: 324.867
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 367
                                    target: _qt_node25_transform_0_0
                                    property: "y"
                                    to: 190.047
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 383
                                    target: _qt_node25_transform_0_0
                                    property: "x"
                                    to: 233.867
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 383
                                    target: _qt_node25_transform_0_0
                                    property: "y"
                                    to: 190.047
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 350
                                    target: _qt_node25_transform_0_0
                                    property: "x"
                                    to: 324.867
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 350
                                    target: _qt_node25_transform_0_0
                                    property: "y"
                                    to: 190.047
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node25_transform_0_0
                                    property: "x"
                                    to: 233.867
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.467, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node25_transform_0_0
                                    property: "y"
                                    to: 190.047
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.467, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node25_transform_0_0
                                    property: "x"
                                    to: 324.867
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node25_transform_0_0
                                    property: "y"
                                    to: 190.047
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.704, 0, 0.467, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 2717 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node25_transform_base_group.deactivateOverride(_qt_node25_transform_group_0)
                        }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_24
                strokeColor: "transparent"
                fillColor: "#ff1f212b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -21.908 140.434 Q -23.6806 139.533 -25.636 137.975 Q -27.592 136.416 -28.566 135.128 Q -30.9279 132.006 -32.048 127.793 Q -32.51 126.055 -32.6431 123.999 Q -32.7762 121.943 -32.501 120.793 Q -31.8331 118.003 -27.572 107.943 Q -23.3073 97.8745 -18.76 83.749 Q -14.2098 69.6204 -11.682 57.645 Q -9.15707 45.692 -7.522 32.839 Q -5.891 20.004 -5.891 0.006 Q -5.891 -19.9914 -7.494 -32.6 Q -9.09961 -45.2191 -11.111 -55.1 Q -13.1248 -64.9997 -16.205 -75.994 Q -19.2877 -86.997 -25.921 -103.494 Q -29.6073 -112.661 -31.0723 -117.348 Q -32.5373 -122.034 -32.514 -124.585 Q -32.4723 -129.175 -30.197 -132.856 Q -27.9241 -136.535 -24.445 -139.19 Q -20.9661 -141.842 -15.586 -142.288 Q -13.3667 -142.472 -11.0297 -142.414 Q -8.69384 -142.355 -7.633 -142.089 Q -6.57225 -141.823 -4.6765 -140.603 Q -2.78201 -139.384 -1.18 -137.937 Q 2.70302 -134.429 7.31 -123.908 Q 11.914 -113.398 15.844 -102.001 Q 19.7671 -90.6223 23.236 -76.805 Q 26.7051 -62.9794 28.77 -50.494 Q 30.8397 -37.9794 31.853 -25.994 Q 32.868 -13.9886 32.868 0.006 Q 32.868 14.0006 31.853 26.006 Q 30.8398 37.9901 28.779 50.506 Q 26.7183 63.0101 22.903 78.006 Q 19.0843 93.0153 14.384 106.006 Q 9.67715 119.015 5.534 127.481 Q 1.3901 135.955 -2.377 138.919 Q -4.59394 140.662 -6.54763 141.347 Q -8.50116 142.031 -11.877 142.249 Q -15.2028 142.463 -17.3099 142.081 Q -19.4162 141.7 -21.908 140.434 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node26
            transform: TransformGroup {
                id: _qt_node26_transform_base_group
                TransformGroup {
                    id: _qt_node26_transform_group_0
                    Translate { id: _qt_node26_transform_0_0 }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_node26_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_node26_transform_animation
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
                                    script:_qt_node26_transform_0_0.x = 257.107
                                }
                                ScriptAction {
                                    script:_qt_node26_transform_0_0.y = 189.94
                                }
                            }
                            PauseAnimation { duration: 3117 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node26_transform_0_0
                                    property: "x"
                                    to: 207.108
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.475, 0, 0.151, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node26_transform_0_0
                                    property: "y"
                                    to: 189.94
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.475, 0, 0.151, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 367
                                    target: _qt_node26_transform_0_0
                                    property: "x"
                                    to: 257.107
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 367
                                    target: _qt_node26_transform_0_0
                                    property: "y"
                                    to: 189.94
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 383
                                    target: _qt_node26_transform_0_0
                                    property: "x"
                                    to: 203.108
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 383
                                    target: _qt_node26_transform_0_0
                                    property: "y"
                                    to: 189.94
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 350
                                    target: _qt_node26_transform_0_0
                                    property: "x"
                                    to: 257.107
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 350
                                    target: _qt_node26_transform_0_0
                                    property: "y"
                                    to: 189.94
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node26_transform_0_0
                                    property: "x"
                                    to: 203.108
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.139, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 334
                                    target: _qt_node26_transform_0_0
                                    property: "y"
                                    to: 189.94
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.167, 0, 0.139, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node26_transform_0_0
                                    property: "x"
                                    to: 257.107
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 366
                                    target: _qt_node26_transform_0_0
                                    property: "y"
                                    to: 189.94
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.483, 0, 0.139, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 2717 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_node26_transform_base_group.deactivateOverride(_qt_node26_transform_group_0)
                        }
                    }
                }
            }
            ShapePath {
                id: _qt_shapePath_25
                strokeColor: "transparent"
                fillColor: "#ff1f212b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -16.956 113.585 Q -18.2367 113.192 -20.4897 111.478 Q -22.7421 109.764 -24.625 107.751 Q -27.3313 104.858 -28.4069 102.889 Q -29.4828 100.92 -29.908 98.081 Q -30.3715 94.9933 -29.9236 92.7941 Q -29.4758 90.5951 -27.387 85.702 Q -24.142 78.0988 -19.796 65.562 Q -15.442 53.0075 -12.928 40.561 Q -10.4118 28.1129 -9.29 13.78 Q -8.16872 -0.557582 -9.296 -14.22 Q -10.4228 -27.8792 -12.897 -40.154 Q -15.368 -52.4129 -18.479 -62.126 Q -21.588 -71.8312 -25.385 -80.589 Q -29.1808 -89.338 -29.869 -92.466 Q -30.1518 -93.7556 -29.9902 -95.9681 Q -29.8286 -98.1815 -29.318 -100.005 Q -28.081 -104.422 -24.157 -108.346 Q -22.5375 -109.965 -20.3393 -111.464 Q -18.1405 -112.963 -16.673 -113.447 Q -15.2049 -113.931 -12.8392 -114.127 Q -10.4746 -114.322 -8.62 -114.113 Q -4.12297 -113.607 0.012 -110.712 Q 4.14958 -107.814 7.401 -101.272 Q 8.7424 -98.575 10.8839 -93.5062 L 14.69 -84.02 Q 18.7296 -73.3005 22.102 -60.098 Q 25.4763 -46.8882 27.291 -35.387 L 28.1266 -30.5231 Q 30.5226 -17.1619 30.504 -1.887 Q 30.4795 17.094 27.2792 35.4819 L 26.748 38.553 Q 24.4531 51.9824 21.138 64.053 Q 17.8252 76.1067 13.73 86.613 L 10.0271 95.6344 Q 8.01287 100.325 6.874 102.579 Q 4.11053 108.046 -0.05 110.969 Q -4.20867 113.892 -9.031 114.215 Q -11.0198 114.348 -13.3471 114.163 Q -15.6754 113.978 -16.956 113.585 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node27
            transform: TransformGroup {
                id: _qt_node27_transform_base_group
                Matrix4x4 { id: _qt_node27_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 115.278, 187.342)}
            }
            ShapePath {
                id: _qt_shapePath_26
                strokeColor: "transparent"
                fillColor: "#ff1f212b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 56.978 80.313 Q 53.9752 79.5577 51.779 78.377 Q 49.5726 77.1917 27.278 59.985 L 5.65275 43.2938 L -15.023 27.33 L -31.543 14.8536 Q -39.8109 8.75061 -43.176 6.548 Q -51.3315 1.211 -52.926 1.211 Q -53.913 1.211 -54.2161 2.595 Q -54.5192 3.97895 -54.514 8.461 Q -54.511 11.4519 -53.7766 17.1 Q -53.0423 22.7478 -52.014 27.689 Q -50.9862 32.6279 -50.2544 38.1917 Q -49.523 43.7523 -49.523 46.623 Q -49.523 53.5813 -51.609 57.888 Q -52.4693 59.6655 -53.9518 61.5508 Q -55.4343 63.436 -56.656 64.306 Q -57.8771 65.1753 -59.8569 65.9435 Q -61.8369 66.7118 -63.396 66.921 Q -67.1787 67.4278 -71.228 65.361 Q -72.899 64.509 -75.2901 62.5376 Q -77.6812 60.5663 -79.368 58.65 Q -83.4574 54.0021 -85.465 48.637 Q -86.2939 46.4219 -87.5431 41.826 Q -88.7926 37.2291 -89.718 32.99 L -90.152 31.0219 Q -93.0306 18.0319 -93.014 0.354 Q -92.9979 -16.6854 -90.2532 -29.1057 L -89.666 -31.789 Q -88.7354 -36.1197 -87.3378 -40.8195 Q -85.9404 -45.5185 -84.909 -47.786 Q -83.8766 -50.0542 -82.4983 -52.358 Q -81.1191 -54.6631 -80.215 -55.631 Q -78.0211 -57.9795 -75.323 -59.384 Q -74.2085 -59.9637 -72.5606 -60.3764 Q -70.9128 -60.789 -69.712 -60.789 Q -68.512 -60.789 -66.546 -60.0351 Q -64.58 -59.2813 -63.02 -58.223 L -32.631 -34.926 L 17.978 4.335 L 45.156 25.173 Q 46.4672 26.1249 47.8242 26.6397 Q 49.1816 27.1547 49.777 26.926 Q 50.4263 26.6771 51.12 24.7822 Q 51.8133 22.8884 52.298 20.042 Q 53.3739 13.7139 53.925 8.05 Q 54.1515 5.71529 54.0073 1.07225 Q 53.863 -3.57069 53.434 -7.755 Q 52.3915 -17.9052 49.993 -27.094 Q 47.5923 -36.2912 44.49 -43.788 Q 41.3876 -51.2832 40.298 -57.788 Q 39.2106 -64.2912 40.404 -68.619 Q 40.8966 -70.4051 42.2029 -72.7197 Q 43.5087 -75.0337 44.85 -76.498 Q 46.8916 -78.7256 48.4603 -79.4788 Q 50.0292 -80.232 53.278 -80.544 Q 55.555 -80.763 57.853 -80.551 Q 60.151 -80.339 61.483 -79.787 Q 62.7321 -79.2697 65.0814 -77.3041 Q 67.4296 -75.3395 69.48 -73.097 Q 74.4473 -67.6605 77.869 -60.45 Q 81.2939 -53.2327 84.781 -42.764 Q 88.2729 -32.2864 90.116 -23.789 L 90.2859 -23.0081 Q 93.032 -10.4026 93.032 4.211 Q 93.032 18.835 90.2853 31.4205 L 90.113 32.211 Q 88.2675 40.7027 85.787 48.592 Q 83.3112 56.4646 79.84 64.084 Q 78.4094 67.2233 76.7445 70.1521 Q 75.0803 73.0797 74.174 74.052 Q 71.9755 76.4088 69.444 78.223 Q 68.399 78.9724 66.6367 79.6569 Q 64.8739 80.3416 63.444 80.553 Q 59.9782 81.0676 56.978 80.313 " }
            }
        }
    }
}
