// Copyright (C) 2025 Bashir Ahmad
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file UserInterface.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 1080
    implicitHeight: 1080
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 1080; yScale: height / 1080 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "comp_0"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            Matrix4x4 { id: _qt_layer0_transform_base; matrix: PlanarTransform.fromAffineMatrix(2.04, 0, 0, 2.04, -529.6, -561.6)}
        }
        LayerItem {
            objectName: "phonr Outlines"
            id: _qt_layer1
            transform: TransformGroup {
                id: _qt_layer1_transform_base_group
                TransformGroup {
                    id: _qt_layer1_transform_group_0
                    Translate { x: -122.006; y: -476.81 }
                    Scale { id: _qt_layer1_transform_0_2}
                    Translate { x: 525.064; y: 774.041 }
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
                                    script:_qt_layer1_transform_0_2.xScale = 0
                                }
                                ScriptAction {
                                    script:_qt_layer1_transform_0_2.yScale = 0
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 467
                                    target: _qt_layer1_transform_0_2
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.658, 0, 0.456, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 467
                                    target: _qt_layer1_transform_0_2
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.658, 0, 0.456, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_layer1_transform_0_2
                                    property: "xScale"
                                    to: 0.95
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.664, 0, 0.318, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 166
                                    target: _qt_layer1_transform_0_2
                                    property: "yScale"
                                    to: 0.95
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.664, 0, 0.318, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 200
                                    target: _qt_layer1_transform_0_2
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.654, 0, 0.417, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 200
                                    target: _qt_layer1_transform_0_2
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.654, 0, 0.417, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 3767 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_layer1_transform_base_group.deactivateOverride(_qt_layer1_transform_group_0)
                        }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node1
                transform: TransformGroup {
                    id: _qt_node1_transform_base_group
                    Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 122.006, 238.53)}
                }
                ShapePath {
                    id: _qt_shapePath_0
                    strokeColor: "transparent"
                    fillColor: "#ff4236a4"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 80.906 -238.28 L -80.905 -238.28 Q -97.8258 -238.28 -109.791 -226.315 Q -121.756 -214.351 -121.756 -197.43 L -121.756 197.43 Q -121.756 214.351 -109.791 226.315 Q -97.8258 238.28 -80.905 238.28 L 80.906 238.28 Q 97.8268 238.28 109.791 226.315 Q 121.756 214.351 121.756 197.43 L 121.756 -197.43 Q 121.756 -214.351 109.791 -226.315 Q 97.8268 -238.28 80.906 -238.28 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node2
                transform: TransformGroup {
                    id: _qt_node2_transform_base_group
                    Matrix4x4 { id: _qt_node2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 122.006, 232.307)}
                }
                ShapePath {
                    id: _qt_shapePath_1
                    strokeColor: "transparent"
                    fillColor: "#ffeff0ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 111.768 195.354 L -111.768 195.354 L -111.768 -195.354 L 111.768 -195.354 L 111.768 195.354 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node3
                transform: TransformGroup {
                    id: _qt_node3_transform_base_group
                    Matrix4x4 { id: _qt_node3_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 122.007, 19.358)}
                }
                ShapePath {
                    id: _qt_shapePath_2
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 27.585 2.378 L -27.585 2.378 L -27.585 -2.378 L 27.585 -2.378 L 27.585 2.378 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node4
                transform: TransformGroup {
                    id: _qt_node4_transform_base_group
                    Matrix4x4 { id: _qt_node4_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 122.006, 451.861)}
                }
                ShapePath {
                    id: _qt_shapePath_3
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 12.19 0 Q 12.19 5.04917 8.61987 8.6195 Q 5.04958 12.19 0 12.19 Q -5.049 12.19 -8.6195 8.6195 Q -12.19 5.049 -12.19 0 Q -12.19 -5.049 -8.6195 -8.6195 Q -5.049 -12.19 0 -12.19 Q 5.04958 -12.19 8.61987 -8.6195 Q 12.19 -5.04917 12.19 0 " }
                }
            }
        }
        LayerItem {
            objectName: "detail Outlines"
            id: _qt_layer2
            transform: TransformGroup {
                id: _qt_layer2_transform_base_group
                TransformGroup {
                    id: _qt_layer2_transform_group_0
                    Translate { x: -92.561; y: -95.775 }
                    Translate { id: _qt_layer2_transform_0_0 }
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
                                    script:_qt_layer2_transform_0_0.x = 525.064
                                }
                                ScriptAction {
                                    script:_qt_layer2_transform_0_0.y = 880.255
                                }
                            }
                            PauseAnimation { duration: 1300 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_layer2_transform_0_0
                                    property: "x"
                                    to: 525.064
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.482, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 533
                                    target: _qt_layer2_transform_0_0
                                    property: "y"
                                    to: 608.255
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.538, 0, 0.482, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 2767 }
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
                        PauseAnimation { duration: 1300 }
                        ScriptAction {
                            script:_qt_layer2.visible = true
                        }
                        PauseAnimation { duration: 3300 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node5
                transform: TransformGroup {
                    id: _qt_node5_transform_base_group
                    Matrix4x4 { id: _qt_node5_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 183.137)}
                }
                ShapePath {
                    id: _qt_shapePath_4
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node6
                transform: TransformGroup {
                    id: _qt_node6_transform_base_group
                    Matrix4x4 { id: _qt_node6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 167.684)}
                }
                ShapePath {
                    id: _qt_shapePath_5
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node7
                transform: TransformGroup {
                    id: _qt_node7_transform_base_group
                    Matrix4x4 { id: _qt_node7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 152.232)}
                }
                ShapePath {
                    id: _qt_shapePath_6
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node8
                transform: TransformGroup {
                    id: _qt_node8_transform_base_group
                    Matrix4x4 { id: _qt_node8_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.865, 167.684)}
                }
                ShapePath {
                    id: _qt_shapePath_7
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -23.615 -23.615 L 23.615 -23.615 L 23.615 23.615 L -23.615 23.615 L -23.615 -23.615 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node9
                transform: TransformGroup {
                    id: _qt_node9_transform_base_group
                    Matrix4x4 { id: _qt_node9_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 85.398, 111.977)}
                }
                ShapePath {
                    id: _qt_shapePath_8
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -29.627 -1.525 L 29.627 -1.525 L 29.627 1.525 L -29.627 1.525 L -29.627 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node10
                transform: TransformGroup {
                    id: _qt_node10_transform_base_group
                    Matrix4x4 { id: _qt_node10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 96.524)}
                }
                ShapePath {
                    id: _qt_shapePath_9
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node11
                transform: TransformGroup {
                    id: _qt_node11_transform_base_group
                    Matrix4x4 { id: _qt_node11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 81.072)}
                }
                ShapePath {
                    id: _qt_shapePath_10
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node12
                transform: TransformGroup {
                    id: _qt_node12_transform_base_group
                    Matrix4x4 { id: _qt_node12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.865, 96.524)}
                }
                ShapePath {
                    id: _qt_shapePath_11
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -23.615 -23.615 L 23.615 -23.615 L 23.615 23.615 L -23.615 23.615 L -23.615 -23.615 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node13
                transform: TransformGroup {
                    id: _qt_node13_transform_base_group
                    Matrix4x4 { id: _qt_node13_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 39.318)}
                }
                ShapePath {
                    id: _qt_shapePath_12
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node14
                transform: TransformGroup {
                    id: _qt_node14_transform_base_group
                    Matrix4x4 { id: _qt_node14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 23.865)}
                }
                ShapePath {
                    id: _qt_shapePath_13
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node15
                transform: TransformGroup {
                    id: _qt_node15_transform_base_group
                    Matrix4x4 { id: _qt_node15_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 120.321, 8.413)}
                }
                ShapePath {
                    id: _qt_shapePath_14
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -64.55 -1.525 L 64.55 -1.525 L 64.55 1.525 L -64.55 1.525 L -64.55 -1.525 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node16
                transform: TransformGroup {
                    id: _qt_node16_transform_base_group
                    Matrix4x4 { id: _qt_node16_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.865, 23.865)}
                }
                ShapePath {
                    id: _qt_shapePath_15
                    strokeColor: "transparent"
                    fillColor: "#ffb8c3ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -23.615 -23.615 L 23.615 -23.615 L 23.615 23.615 L -23.615 23.615 L -23.615 -23.615 " }
                }
            }
        }
        LayerItem {
            objectName: "Layer 17 Outlines"
            id: _qt_layer3
            transform: TransformGroup {
                id: _qt_layer3_transform_base_group
                TransformGroup {
                    id: _qt_layer3_transform_group_0
                    Translate { x: -94.449; y: -61.642 }
                    Scale { id: _qt_layer3_transform_0_2}
                    Translate { x: 524.46; y: 433.355 }
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
                            PauseAnimation { duration: 833 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 300
                                    target: _qt_layer3_transform_0_2
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.801, 0, 0.607, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 300
                                    target: _qt_layer3_transform_0_2
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.801, 0, 0.607, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 67
                                    target: _qt_layer3_transform_0_2
                                    property: "xScale"
                                    to: 0.98
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.731, 0, 0.431, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 67
                                    target: _qt_layer3_transform_0_2
                                    property: "yScale"
                                    to: 0.98
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.731, 0, 0.431, 1, 1, 1 ]
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 100
                                    target: _qt_layer3_transform_0_2
                                    property: "xScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.751, 0, 0.231, 1, 1, 1 ]
                                }
                                PropertyAnimation {
                                    duration: 100
                                    target: _qt_layer3_transform_0_2
                                    property: "yScale"
                                    to: 1
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: [ 0.751, 0, 0.231, 1, 1, 1 ]
                                }
                            }
                            PauseAnimation { duration: 3300 }
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
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer3.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node17
                transform: TransformGroup {
                    id: _qt_node17_transform_base_group
                    Matrix4x4 { id: _qt_node17_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 94.449, 61.642)}
                }
                ShapePath {
                    id: _qt_shapePath_16
                    strokeColor: "transparent"
                    fillColor: "#ff9090f6"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 94.199 61.392 L -94.199 61.392 L -94.199 -61.392 L 94.199 -61.392 L 94.199 61.392 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node18
                transform: TransformGroup {
                    id: _qt_node18_transform_base_group
                    Matrix4x4 { id: _qt_node18_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 94.45, 61.642)}
                }
                ShapePath {
                    id: _qt_shapePath_17
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 83.63 52.197 L -83.63 52.197 L -83.63 -52.197 L 83.63 -52.197 L 83.63 52.197 " }
                }
            }
        }
        LayerItem {
            objectName: "Layer 4 Outlines"
            id: _qt_layer4
            transform: TransformGroup {
                id: _qt_layer4_transform_base_group
                Matrix4x4 { id: _qt_layer4_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.541, 83.24)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
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
                        PauseAnimation { duration: 1667 }
                        ScriptAction {
                            script:_qt_layer4.visible = true
                        }
                        PauseAnimation { duration: 2933 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node19
                ShapePath {
                    id: _qt_shapePath_18
                    strokeColor: "#ff000000"
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 2.5 2.5 L 51.103 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_18_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_18_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_18.trim.start = 0
                            }
                            PauseAnimation { duration: 2667 }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_shapePath_18.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 1767 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "Layer 5 Outlines"
            id: _qt_layer5
            transform: TransformGroup {
                id: _qt_layer5_transform_base_group
                Matrix4x4 { id: _qt_layer5_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.541, 70.968)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
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
                        PauseAnimation { duration: 1500 }
                        ScriptAction {
                            script:_qt_layer5.visible = true
                        }
                        PauseAnimation { duration: 3100 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node20
                ShapePath {
                    id: _qt_shapePath_19
                    strokeColor: "#ff000000"
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 2.5 2.5 L 51.103 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_19_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_19_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_19.trim.start = 0
                            }
                            PauseAnimation { duration: 2500 }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_shapePath_19.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 1933 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "Layer 6 Outlines"
            id: _qt_layer6
            transform: TransformGroup {
                id: _qt_layer6_transform_base_group
                Matrix4x4 { id: _qt_layer6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.541, 58.696)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
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
                        PauseAnimation { duration: 1333 }
                        ScriptAction {
                            script:_qt_layer6.visible = true
                        }
                        PauseAnimation { duration: 3267 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node21
                ShapePath {
                    id: _qt_shapePath_20
                    strokeColor: "#ff000000"
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 2.5 2.5 L 51.103 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_20_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_20_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_20.trim.start = 0
                            }
                            PauseAnimation { duration: 2333 }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_shapePath_20.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 2100 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "Layer 7 Outlines"
            id: _qt_layer7
            transform: TransformGroup {
                id: _qt_layer7_transform_base_group
                Matrix4x4 { id: _qt_layer7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.541, 46.424)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
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
                        PauseAnimation { duration: 1167 }
                        ScriptAction {
                            script:_qt_layer7.visible = true
                        }
                        PauseAnimation { duration: 3433 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node22
                ShapePath {
                    id: _qt_shapePath_21
                    strokeColor: "#ff000000"
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 2.5 2.5 L 51.103 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_21_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_21_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_21.trim.start = 0
                            }
                            PauseAnimation { duration: 2167 }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_shapePath_21.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 2267 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "Layer 8 Outlines"
            id: _qt_layer8
            transform: TransformGroup {
                id: _qt_layer8_transform_base_group
                Matrix4x4 { id: _qt_layer8_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.541, 34.152)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
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
                        PauseAnimation { duration: 1000 }
                        ScriptAction {
                            script:_qt_layer8.visible = true
                        }
                        PauseAnimation { duration: 3600 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node23
                ShapePath {
                    id: _qt_shapePath_22
                    strokeColor: "transparent"
                    fillColor: "#ffc5c5ff"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 2.5 2.5 L 51.104 2.5 L 2.5 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_22_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_22_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_22.trim.start = 0
                            }
                            PauseAnimation { duration: 2000 }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_shapePath_22.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 2433 }
                        }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node24
                ShapePath {
                    id: _qt_shapePath_23
                    strokeColor: "#ff000000"
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 2.5 2.5 L 51.103 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_23_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_23_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_23.trim.start = 0
                            }
                            PauseAnimation { duration: 2000 }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_shapePath_23.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 2433 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "Layer 9 Outlines"
            id: _qt_layer9
            transform: TransformGroup {
                id: _qt_layer9_transform_base_group
                Matrix4x4 { id: _qt_layer9_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 23.541, 21.88)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer9_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer9_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer9.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer9.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node25
                ShapePath {
                    id: _qt_shapePath_24
                    strokeColor: "#ff000000"
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 2.5 2.5 L 51.103 2.5 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_24_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_24_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_24.trim.start = 0
                            }
                            PauseAnimation { duration: 1833 }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_shapePath_24.trim
                                property: "start"
                                to: 1
                            }
                            PauseAnimation { duration: 2600 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "Layer 10 Outlines"
            id: _qt_layer10
            transform: TransformGroup {
                id: _qt_layer10_transform_base_group
                Matrix4x4 { id: _qt_layer10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 25.791, 100.331)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer10_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer10_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer10.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer10.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node26
                transform: TransformGroup {
                    id: _qt_node26_transform_base_group
                    Matrix4x4 { id: _qt_node26_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 12.408, 2.457)}
                }
                ShapePath {
                    id: _qt_shapePath_25
                    strokeColor: "transparent"
                    fillColor: "#ffc5c5ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 10.953 2.207 L -11.022 2.207 Q -11.4929 2.207 -11.826 1.87413 Q -12.159 1.54133 -12.159 1.071 L -12.159 -1.071 Q -12.159 -1.54133 -11.826 -1.87413 Q -11.4929 -2.207 -11.022 -2.207 L 10.953 -2.207 Q 11.4525 -2.207 11.8057 -1.85375 Q 12.159 -1.5005 12.159 -1.001 L 12.159 1.001 Q 12.159 1.5005 11.8057 1.85375 Q 11.4525 2.207 10.953 2.207 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node27
                transform: TransformGroup {
                    id: _qt_node27_transform_base_group
                    Matrix4x4 { id: _qt_node27_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 67.772, 2.457)}
                }
                ShapePath {
                    id: _qt_shapePath_26
                    strokeColor: "transparent"
                    fillColor: "#ffc5c5ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 10.615 2.207 L -11.023 2.207 Q -11.4932 2.207 -11.8261 1.87413 Q -12.159 1.54125 -12.159 1.071 L -12.159 -1.071 Q -12.159 -1.54125 -11.8261 -1.87413 Q -11.4932 -2.207 -11.023 -2.207 L 10.615 -2.207 Q 11.2542 -2.207 11.7065 -1.75487 Q 12.159 -1.30258 12.159 -0.663 L 12.159 0.663 Q 12.159 1.30258 11.7065 1.75487 Q 11.2542 2.207 10.615 2.207 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node28
                transform: TransformGroup {
                    id: _qt_node28_transform_base_group
                    Matrix4x4 { id: _qt_node28_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 123.136, 2.457)}
                }
                ShapePath {
                    id: _qt_shapePath_27
                    strokeColor: "transparent"
                    fillColor: "#ffc5c5ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 10.393 2.207 L -11.023 2.207 Q -11.4932 2.207 -11.8261 1.87413 Q -12.159 1.54125 -12.159 1.071 L -12.159 -1.071 Q -12.159 -1.54125 -11.8261 -1.87413 Q -11.4932 -2.207 -11.023 -2.207 L 10.393 -2.207 Q 11.1243 -2.207 11.6416 -1.6895 Q 12.159 -1.17192 12.159 -0.44 L 12.159 0.441 Q 12.159 1.17225 11.6416 1.68963 Q 11.1243 2.207 10.393 2.207 " }
                }
            }
        }
        LayerItem {
            objectName: "red ball Outlines"
            id: _qt_layer11
            transform: TransformGroup {
                id: _qt_layer11_transform_base_group
                TransformGroup {
                    id: _qt_layer11_transform_group_0
                    Translate { x: -4.556; y: -4.556 }
                    Translate { id: _qt_layer11_transform_0_0 }
                }
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer11_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_layer11_transform_animation
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
                                    script:_qt_layer11_transform_0_0.x = 49.939
                                }
                                ScriptAction {
                                    script:_qt_layer11_transform_0_0.y = 102.789
                                }
                            }
                            PauseAnimation { duration: 2833 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_layer11_transform_0_0
                                    property: "x"
                                    to: 38.064
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_layer11_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 166 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 167
                                    target: _qt_layer11_transform_0_0
                                    property: "x"
                                    to: 49.939
                                }
                                PropertyAnimation {
                                    duration: 167
                                    target: _qt_layer11_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 333
                                    target: _qt_layer11_transform_0_0
                                    property: "x"
                                    to: 38.938
                                }
                                PropertyAnimation {
                                    duration: 333
                                    target: _qt_layer11_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 234 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 333
                                    target: _qt_layer11_transform_0_0
                                    property: "x"
                                    to: 49.939
                                }
                                PropertyAnimation {
                                    duration: 333
                                    target: _qt_layer11_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 400 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_layer11_transform_base_group.deactivateOverride(_qt_layer11_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer11_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer11_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer11.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer11.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node29
                transform: TransformGroup {
                    id: _qt_node29_transform_base_group
                    Matrix4x4 { id: _qt_node29_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 4.556, 4.556)}
                }
                ShapePath {
                    id: _qt_shapePath_28
                    strokeColor: "transparent"
                    fillColor: "#ffff325c"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 4.306 0 Q 4.306 1.7835 3.04475 3.04475 Q 1.7835 4.306 0 4.306 Q -1.7835 4.306 -3.04475 3.04475 Q -4.306 1.7835 -4.306 0 Q -4.306 -1.7835 -3.04475 -3.04475 Q -1.7835 -4.306 0 -4.306 Q 1.7835 -4.306 3.04475 -3.04475 Q 4.306 -1.7835 4.306 0 " }
                }
            }
        }
        LayerItem {
            objectName: "yellow ball Outlines"
            id: _qt_layer12
            transform: TransformGroup {
                id: _qt_layer12_transform_base_group
                TransformGroup {
                    id: _qt_layer12_transform_group_0
                    Translate { x: -4.556; y: -4.556 }
                    Translate { id: _qt_layer12_transform_0_0 }
                }
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer12_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_layer12_transform_animation
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
                                    script:_qt_layer12_transform_0_0.x = 97.177
                                }
                                ScriptAction {
                                    script:_qt_layer12_transform_0_0.y = 102.789
                                }
                            }
                            PauseAnimation { duration: 2833 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_layer12_transform_0_0
                                    property: "x"
                                    to: 104.552
                                }
                                PropertyAnimation {
                                    duration: 134
                                    target: _qt_layer12_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 166 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 200
                                    target: _qt_layer12_transform_0_0
                                    property: "x"
                                    to: 97.177
                                }
                                PropertyAnimation {
                                    duration: 200
                                    target: _qt_layer12_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 1267 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_layer12_transform_base_group.deactivateOverride(_qt_layer12_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer12_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer12_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer12.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer12.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node30
                transform: TransformGroup {
                    id: _qt_node30_transform_base_group
                    Matrix4x4 { id: _qt_node30_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 4.556, 4.556)}
                }
                ShapePath {
                    id: _qt_shapePath_29
                    strokeColor: "transparent"
                    fillColor: "#ffffc400"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 4.306 0 Q 4.306 1.7835 3.04475 3.04475 Q 1.7835 4.306 0 4.306 Q -1.7835 4.306 -3.04475 3.04475 Q -4.306 1.7835 -4.306 0 Q -4.306 -1.7835 -3.04475 -3.04475 Q -1.7835 -4.306 0 -4.306 Q 1.7835 -4.306 3.04475 -3.04475 Q 4.306 -1.7835 4.306 0 " }
                }
            }
        }
        LayerItem {
            objectName: "blue ball Outlines"
            id: _qt_layer13
            transform: TransformGroup {
                id: _qt_layer13_transform_base_group
                TransformGroup {
                    id: _qt_layer13_transform_group_0
                    Translate { x: -4.556; y: -4.556 }
                    Translate { id: _qt_layer13_transform_0_0 }
                }
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer13_transform_animation.restart() } }
            ParallelAnimation {
                id:_qt_layer13_transform_animation
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
                                    script:_qt_layer13_transform_0_0.x = 149.666
                                }
                                ScriptAction {
                                    script:_qt_layer13_transform_0_0.y = 102.789
                                }
                            }
                            PauseAnimation { duration: 3333 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 300
                                    target: _qt_layer13_transform_0_0
                                    property: "x"
                                    to: 160.666
                                }
                                PropertyAnimation {
                                    duration: 300
                                    target: _qt_layer13_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 234 }
                            ParallelAnimation {
                                PropertyAnimation {
                                    duration: 333
                                    target: _qt_layer13_transform_0_0
                                    property: "x"
                                    to: 149.666
                                }
                                PropertyAnimation {
                                    duration: 333
                                    target: _qt_layer13_transform_0_0
                                    property: "y"
                                    to: 102.789
                                }
                            }
                            PauseAnimation { duration: 400 }
                        }
                    }
                    ScriptAction {
                        script: {
                            _qt_layer13_transform_base_group.deactivateOverride(_qt_layer13_transform_group_0)
                        }
                    }
                }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer13_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer13_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer13.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer13.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node31
                transform: TransformGroup {
                    id: _qt_node31_transform_base_group
                    Matrix4x4 { id: _qt_node31_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 4.556, 4.556)}
                }
                ShapePath {
                    id: _qt_shapePath_30
                    strokeColor: "transparent"
                    fillColor: "#ff5656bf"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 4.306 0 Q 4.306 1.7835 3.04475 3.04475 Q 1.7835 4.306 0 4.306 Q -1.7835 4.306 -3.04475 3.04475 Q -4.306 1.7835 -4.306 0 Q -4.306 -1.7835 -3.04475 -3.04475 Q -1.7835 -4.306 0 -4.306 Q 1.7835 -4.306 3.04475 -3.04475 Q 4.306 -1.7835 4.306 0 " }
                }
            }
        }
        LayerItem {
            objectName: "blue line Outlines"
            id: _qt_layer14
            transform: TransformGroup {
                id: _qt_layer14_transform_base_group
                Matrix4x4 { id: _qt_layer14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 77.736, 32.684)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer14_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer14_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer14.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer14.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node32
                transform: TransformGroup {
                    id: _qt_node32_transform_base_group
                    Matrix4x4 { id: _qt_node32_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 52.122, 38.517)}
                }
                ShapePath {
                    id: _qt_shapePath_31
                    strokeColor: "#ff5656bf"
                    strokeWidth: 10
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0.34
                    trim.end: 1
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M -28.744 -33.29 Q -34.6215 -24.1756 -33.2565 -13.4329 Q -32.1458 -4.69194 -27.122 1.754 Q -22.8575 7.22567 -16.6929 10.316 Q -10.3075 13.517 -3.119 13.517 Q 8.56563 13.517 17.2376 5.70225 Q 25.8425 -2.05201 27.122 -13.517 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_31_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_31_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_31.trim.start = 0.34
                            }
                            PauseAnimation { duration: 3333 }
                            PropertyAnimation {
                                duration: 300
                                target: _qt_shapePath_31.trim
                                property: "start"
                                to: 0.03
                            }
                            PauseAnimation { duration: 34 }
                            PauseAnimation { duration: 200 }
                            PauseAnimation { duration: 33 }
                            PropertyAnimation {
                                duration: 300
                                target: _qt_shapePath_31.trim
                                property: "start"
                                to: 0.34
                            }
                            PauseAnimation { duration: 400 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "yellow line Outlines"
            id: _qt_layer15
            transform: TransformGroup {
                id: _qt_layer15_transform_base_group
                Matrix4x4 { id: _qt_layer15_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 103.03, -1.102)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer15_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer15_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer15.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer15.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node33
                transform: TransformGroup {
                    id: _qt_node33_transform_base_group
                    Matrix4x4 { id: _qt_node33_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 39.576, 41.893)}
                }
                ShapePath {
                    id: _qt_shapePath_32
                    strokeColor: "#ffffc400"
                    strokeWidth: 10
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0
                    trim.end: 0.55
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 14.374 16.893 Q 14.4718 16.0167 14.5219 15.2014 Q 14.576 14.32 14.576 13.485 Q 14.576 1.23046 6.07875 -7.59037 Q -2.38942 -16.381 -14.576 -16.893 Q -20.4925 -17.546 -27.3309 -14.9646 Q -41.0076 -9.80188 -45.617 6.37 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_32_trim_end_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_32_trim_end_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_32.trim.end = 0.55
                            }
                            PauseAnimation { duration: 2833 }
                            PropertyAnimation {
                                duration: 134
                                target: _qt_shapePath_32.trim
                                property: "end"
                                to: 0.97
                            }
                            PauseAnimation { duration: 33 }
                            PauseAnimation { duration: 133 }
                            PauseAnimation { duration: 34 }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_shapePath_32.trim
                                property: "end"
                                to: 0.55
                            }
                            PauseAnimation { duration: 1267 }
                        }
                    }
                }
            }
        }
        LayerItem {
            objectName: "red line Outlines"
            id: _qt_layer16
            transform: TransformGroup {
                id: _qt_layer16_transform_base_group
                Matrix4x4 { id: _qt_layer16_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 71.296, -1.168)}
                Matrix4x4 { matrix: _qt_layer3.transformMatrix }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_layer16_visible_animation.restart() } }
            ParallelAnimation {
                id: _qt_layer16_visible_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_layer16.visible = false
                        }
                        PauseAnimation { duration: 833 }
                        ScriptAction {
                            script:_qt_layer16.visible = true
                        }
                        PauseAnimation { duration: 3767 }
                    }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node34
                transform: TransformGroup {
                    id: _qt_node34_transform_base_group
                    Matrix4x4 { id: _qt_node34_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 40.867, 49.562)}
                }
                ShapePath {
                    id: _qt_shapePath_33
                    strokeColor: "#ffff325c"
                    strokeWidth: 10
                    capStyle: ShapePath.FlatCap
                    joinStyle: ShapePath.MiterJoin
                    miterLimit: 10
                    fillColor: "transparent"
                    trim.start: 0.4
                    trim.end: 1
                    trim.offset: 0
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 44.951 4.267 Q 42.6368 -13.4754 29.5207 -20.6965 Q 22.8784 -24.3535 15.9122 -24.4959 L 15.867 -24.497 L 15.3799 -24.5254 Q 14.8462 -24.561 14.575 -24.561 Q 1.96583 -24.561 -6.9505 -15.6449 Q -15.867 -6.72858 -15.867 5.881 Q -15.867 11.0689 -14.1554 15.9294 Q -12.5073 20.6095 -9.428 24.561 " }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_33_trim_start_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_33_trim_start_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_33.trim.start = 0.4
                            }
                            PauseAnimation { duration: 2833 }
                            PropertyAnimation {
                                duration: 134
                                target: _qt_shapePath_33.trim
                                property: "start"
                                to: 0.54
                            }
                            PauseAnimation { duration: 33 }
                            PauseAnimation { duration: 133 }
                            PauseAnimation { duration: 34 }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_shapePath_33.trim
                                property: "start"
                                to: 0.4
                            }
                            PauseAnimation { duration: 1267 }
                        }
                    }
                }
                Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_33_trim_end_animation.restart() } }
                ParallelAnimation {
                    id: _qt_shapePath_33_trim_end_animation
                    loops: _qt_node0.animations.loops
                    paused: _qt_node0.animations.paused
                    running: true
                    onLoopsChanged: { if (running) { restart() } }
                    SequentialAnimation {
                        SequentialAnimation {
                            loops: 1
                            ScriptAction {
                                script:_qt_shapePath_33.trim.end = 1
                            }
                            PauseAnimation { duration: 3333 }
                            PropertyAnimation {
                                duration: 300
                                target: _qt_shapePath_33.trim
                                property: "end"
                                to: 0.7
                            }
                            PauseAnimation { duration: 34 }
                            PauseAnimation { duration: 200 }
                            PauseAnimation { duration: 33 }
                            PropertyAnimation {
                                duration: 300
                                target: _qt_shapePath_33.trim
                                property: "end"
                                to: 1
                            }
                            PauseAnimation { duration: 400 }
                        }
                    }
                }
            }
        }
    }
}
