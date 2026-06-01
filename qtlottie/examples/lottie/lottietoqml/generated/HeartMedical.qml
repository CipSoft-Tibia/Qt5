// Copyright (C) 2025 Natalia ┼Üwierz
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file HeartMedical.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 600
    implicitHeight: 436
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 600; yScale: height / 436 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "Heart/heartbeat Outlines"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            TransformGroup {
                id: _qt_layer0_transform_group_0
                Translate { x: -220; y: -210.5 }
                Scale { id: _qt_layer0_transform_0_2}
                Translate { x: 300; y: 218 }
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
                                script:_qt_layer0_transform_0_2.xScale = 0.85455
                            }
                            ScriptAction {
                                script:_qt_layer0_transform_0_2.yScale = 0.85455
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 283
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 283
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 34
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 0.92273
                            }
                            PropertyAnimation {
                                duration: 34
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 0.92273
                            }
                        }
                        PauseAnimation { duration: 16 }
                        PauseAnimation { duration: 417 }
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
                Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 219.764, 210.459)}
            }
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "transparent"
                fillColor: "#ffff5555"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 217.971 -88.444 Q 219.132 -137.547 185.243 -173.296 Q 151.206 -209.2 102.397 -209.729 Q 70.1688 -210.081 42.4787 -193.988 Q 15.6282 -178.384 -0.002 -151.526 Q -15.6365 -178.386 -42.4823 -193.99 Q -70.1711 -210.083 -102.4 -209.729 Q -151.209 -209.2 -185.245 -173.296 Q -219.133 -137.548 -217.975 -88.444 Q -214.355 64.5834 -5.814 207.798 Q -3.186 209.605 -0.0015 209.605 Q 3.183 209.605 5.811 207.798 Q 214.356 64.5762 217.971 -88.444 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node2
            transform: TransformGroup {
                id: _qt_node2_transform_base_group
                Matrix4x4 { id: _qt_node2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 302.42, 212.443)}
            }
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "transparent"
                fillColor: "#fff72b2b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 135.373 -94.791 Q 135.373 60.0653 -76.842 205.807 Q -79.4748 207.616 -82.6605 207.62 Q -85.8498 207.624 -88.482 205.817 Q -111.016 190.347 -135.374 170.307 Q -29.3125 97.4198 23.7183 22.2823 Q 76.749 -52.8552 76.749 -130.243 Q 76.749 -175.003 46.939 -208.229 Q 85.4467 -198.586 110.209 -167.278 Q 135.373 -135.462 135.373 -94.791 " }
            }
        }
    }
    LayerItem {
        objectName: "Beep/heartbeat Outlines"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            Matrix4x4 { id: _qt_layer1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 44, 97)}
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            transform: TransformGroup {
                id: _qt_node3_transform_base_group
                Matrix4x4 { id: _qt_node3_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 255.75, 120.802)}
            }
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "transparent"
                fillColor: "#ff60daff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -1.449 120.552 Q -6.20508 120.552 -9.8665 117.518 Q -13.5292 114.483 -14.414 109.806 L -46.621 -61.295 L -63.758 -20.57 L -80.895 20.155 Q -82.392 23.7137 -85.5691 25.9024 Q -88.7462 28.091 -92.602 28.22 Q -96.4645 28.3519 -99.782 26.3832 Q -103.097 24.4158 -104.832 20.966 L -113.996 2.755 L -242.312 2.755 Q -247.774 2.755 -251.637 -1.10763 Q -255.5 -4.97027 -255.5 -10.431 Q -255.5 -15.8969 -251.637 -19.7604 Q -247.774 -23.623 -242.312 -23.623 L -105.867 -23.623 Q -102.147 -23.623 -98.9774 -21.6806 Q -95.8089 -19.739 -94.121 -16.427 L -53.93 -111.931 Q -52.2369 -115.958 -48.4756 -118.178 Q -44.7158 -120.397 -40.379 -119.931 Q -36.0357 -119.472 -32.8274 -116.512 Q -29.6208 -113.553 -28.809 -109.26 L -2.254 31.829 L 8.739 -34.185 Q 9.52933 -38.9577 13.2189 -42.0859 Q 16.9072 -45.213 21.742 -45.213 L 64.789 -45.213 Q 71.9435 -45.213 75.84 -39.224 L 96.051 -8.201 L 100.41 -13.5035 L 104.77 -18.806 Q 108.73 -23.623 114.961 -23.623 L 242.313 -23.623 Q 247.774 -23.623 251.637 -19.7593 Q 255.5 -15.8951 255.5 -10.431 Q 255.5 -4.97035 251.637 -1.10763 Q 247.774 2.755 242.313 2.755 L 121.196 2.755 L 113.106 12.597 L 105.016 22.439 Q 100.773 27.6017 94.098 27.237 Q 87.4238 26.8682 83.774 21.267 L 70.7095 1.2165 L 57.645 -18.834 L 32.922 -18.834 L 11.559 109.53 Q 10.7738 114.26 7.1305 117.379 Q 3.48744 120.498 -1.309 120.548 L -1.38133 120.55 L -1.449 120.552 " }
            }
        }
    }
    LayerItem {
        objectName: "dot/heartbeat Outlines"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            TransformGroup {
                id: _qt_layer2_transform_group_0
                Translate { x: -22; y: -22 }
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
                                script:_qt_layer2_transform_0_0.x = -18
                            }
                            ScriptAction {
                                script:_qt_layer2_transform_0_0.y = 205.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 150
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 191
                            }
                            PropertyAnimation {
                                duration: 150
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 208.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 17
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 203
                            }
                            PropertyAnimation {
                                duration: 17
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 231.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 256
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.437, 0.438, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 124.5
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.437, 0.438, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 116
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 296
                            }
                            PropertyAnimation {
                                duration: 116
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 310.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 321
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 189.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 34
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 365
                            }
                            PropertyAnimation {
                                duration: 34
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 188.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 33
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 392
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.33, 0.33, 0.665, 0.666, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 33
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 230.5
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.33, 0.33, 0.665, 0.666, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 17
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 413
                            }
                            PropertyAnimation {
                                duration: 17
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 208.5
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer2_transform_0_0
                                property: "x"
                                to: 584
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer2_transform_0_0
                                property: "y"
                                to: 206.5
                            }
                        }
                        PauseAnimation { duration: 17 }
                        PauseAnimation { duration: 33 }
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
            id: _qt_node4
            transform: TransformGroup {
                id: _qt_node4_transform_base_group
                Matrix4x4 { id: _qt_node4_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 21.75, 21.75)}
            }
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 21.5 0 Q 21.5 8.9055 15.2028 15.2028 Q 8.9055 21.5 0 21.5 Q -8.9055 21.5 -15.2028 15.2028 Q -21.5 8.9055 -21.5 0 Q -21.5 -8.9055 -15.2028 -15.2028 Q -8.9055 -21.5 0 -21.5 Q 8.9055 -21.5 15.2028 -15.2028 Q 21.5 -8.9055 21.5 0 " }
            }
        }
    }
}
