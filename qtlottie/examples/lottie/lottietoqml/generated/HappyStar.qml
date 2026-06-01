// Copyright (C) 2025 Jeffrey Christopher
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file HappyStar.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 180
    implicitHeight: 220
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 180; yScale: height / 220 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "Ellipse 1105"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            Matrix4x4 { id: _qt_layer0_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 91, 203.25)}
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node1
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "transparent"
                fillColor: "#ffdcdcdc"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 0 -12 Q 30.2376 -12 51.6188 -8.48528 Q 73 -4.97056 73 0 Q 73 4.97056 51.6188 8.48528 Q 30.2376 12 0 12 Q -30.2376 12 -51.6188 8.48528 Q -73 4.97056 -73 0 Q -73 -4.97056 -51.6188 -8.48528 Q -30.2376 -12 0 -12 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node2
        }
    }
    LayerItem {
        objectName: "polly"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            Matrix4x4 { id: _qt_layer1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 12.5, 50.5)}
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "transparent"
                fillColor: "#ffffc13b"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 87.5 5.005 L 105.098 40.664 Q 107.186 44.8984 111.86 45.576 L 131.536 48.435 L 151.211 51.294 Q 154.891 51.8287 157.115 54.8099 Q 159.339 57.7909 158.804 61.472 Q 158.366 64.4846 156.186 66.609 L 141.949 80.4865 L 127.713 94.364 Q 124.333 97.6609 125.13 102.315 L 131.856 141.506 Q 132.483 145.172 130.334 148.208 Q 128.185 151.244 124.518 151.871 Q 121.518 152.384 118.824 150.969 L 83.629 132.464 Q 79.4492 130.264 75.271 132.464 L 40.073 150.969 Q 36.7814 152.699 33.2303 151.594 Q 29.6793 150.49 27.949 147.198 Q 26.5325 144.504 27.048 141.502 L 33.768 102.311 Q 34.5647 97.6604 31.187 94.361 L 16.949 80.483 L 2.711 66.605 Q 0.0478435 64.0093 0.0005 60.2902 Q -0.0468411 56.5713 2.549 53.908 Q 4.67698 51.726 7.689 51.29 L 47.041 45.572 Q 51.7124 44.893 53.801 40.66 L 71.398 5.001 Q 73.0472 1.66586 76.5719 0.474375 Q 80.0964 -0.717054 83.431 0.933 Q 86.1515 2.2786 87.5 5.005 " }
            }
        }
    }
    LayerItem {
        objectName: "Path 4473"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            Matrix4x4 { id: _qt_layer2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 78.699, 50.501)}
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node4
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "transparent"
                fillColor: "#fffea547"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 85.013 51.294 L 65.337 48.435 L 45.661 45.576 Q 40.9867 44.8984 38.899 40.664 L 21.302 5.005 Q 19.9544 2.28058 17.232 0.934 Q 13.8981 -0.716064 10.3736 0.47475 Q 6.84894 1.66563 5.199 5 L 2.5995 10.2685 L 0 15.537 Q 1.69645 14.4149 3.72275 14.268 Q 5.8116 14.1165 7.699 15.051 Q 10.0431 16.2099 11.203 18.556 L 26.353 49.254 Q 28.1496 52.8999 32.174 53.483 L 49.1135 55.945 L 66.053 58.407 Q 69.221 58.8675 71.1352 61.4334 Q 73.0495 63.9993 72.589 67.168 Q 72.2129 69.7609 70.335 71.591 L 58.079 83.5385 L 45.823 95.486 Q 42.9112 98.3243 43.598 102.33 L 49.388 136.071 Q 49.9287 139.227 48.0783 141.84 Q 46.2279 144.454 43.071 144.994 Q 42.6748 145.062 42.2604 145.086 Q 41.8631 145.109 41.449 145.092 L 52.626 150.969 Q 55.3165 152.384 58.318 151.872 Q 61.9849 151.245 64.1342 148.209 Q 66.2837 145.173 65.656 141.506 L 58.931 102.314 Q 58.1325 97.6618 61.514 94.365 L 89.987 66.609 Q 92.1673 64.4842 92.605 61.471 Q 93.1405 57.7914 90.9169 54.8105 Q 88.6932 51.8295 85.013 51.294 " }
            }
        }
    }
    LayerItem {
        objectName: "oko prawe"
        id: _qt_layer3
        transform: TransformGroup {
            id: _qt_layer3_transform_base_group
            TransformGroup {
                id: _qt_layer3_transform_group_0
                Translate { x: -6.262; y: -6.262 }
                Scale { id: _qt_layer3_transform_0_2}
                Translate { x: 65.324; y: 94.575 }
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
                                script:_qt_layer3_transform_0_2.xScale = 1
                            }
                            ScriptAction {
                                script:_qt_layer3_transform_0_2.yScale = 1
                            }
                        }
                        PauseAnimation { duration: 467 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 1334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 1866 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 1334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 800 }
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
            id: _qt_node5
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "transparent"
                fillColor: "#ff421c40"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.262 0 Q 8.8555 0 10.6897 1.83425 Q 12.524 3.6685 12.524 6.262 Q 12.524 8.8555 10.6897 10.6897 Q 8.8555 12.524 6.262 12.524 Q 3.6685 12.524 1.83425 10.6897 Q -2.22045e-16 8.8555 0 6.262 Q 0 3.6685 1.83425 1.83425 Q 3.6685 -2.22045e-16 6.262 0 " }
            }
        }
    }
    LayerItem {
        objectName: "oko lewe"
        id: _qt_layer4
        transform: TransformGroup {
            id: _qt_layer4_transform_base_group
            TransformGroup {
                id: _qt_layer4_transform_group_0
                Scale { id: _qt_layer4_transform_0_2}
                Translate { x: 118.098; y: 96.076 }
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
                                script:_qt_layer4_transform_0_2.xScale = 1
                            }
                            ScriptAction {
                                script:_qt_layer4_transform_0_2.yScale = 1
                            }
                        }
                        PauseAnimation { duration: 467 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 1334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 167
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 1866 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 134
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 1334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 133
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 0.31132
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 200
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 800 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer4_transform_base_group.deactivateOverride(_qt_layer4_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node6
            ShapePath {
                id: _qt_shapePath_4
                strokeColor: "transparent"
                fillColor: "#ff421c40"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 0 -6.262 Q 2.59381 -6.262 4.4279 -4.4279 Q 6.262 -2.59381 6.262 0 Q 6.262 2.59381 4.4279 4.4279 Q 2.59381 6.262 0 6.262 Q -2.59381 6.262 -4.4279 4.4279 Q -6.262 2.59381 -6.262 0 Q -6.262 -2.59381 -4.4279 -4.4279 Q -2.59381 -6.262 0 -6.262 " }
            }
        }
    }
    LayerItem {
        objectName: "comp_2"
        id: _qt_layer5
        LayerItem {
            objectName: "Path 4487"
            id: _qt_layer6
            transform: TransformGroup {
                id: _qt_layer6_transform_base_group
                Matrix4x4 { id: _qt_layer6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 82.032, 126.475)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node7
                ShapePath {
                    id: _qt_shapePath_5
                    strokeColor: "transparent"
                    fillColor: "#ff421a40"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 8.86 2.987 Q 2.99869 2.987 0 0 L 0 18.515 Q 0 22.1698 2.60262 24.7724 Q 5.20525 27.375 8.86 27.375 Q 12.5147 27.375 15.1174 24.7724 Q 17.72 22.1698 17.72 18.515 L 17.72 0 Q 14.7213 2.987 8.86 2.987 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4488"
            id: _qt_layer7
            transform: TransformGroup {
                id: _qt_layer7_transform_base_group
                Matrix4x4 { id: _qt_layer7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 82.041, 142.799)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node8
                ShapePath {
                    id: _qt_shapePath_6
                    strokeColor: "transparent"
                    fillColor: "#ffd90658"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 8.85 0 Q 3.47017 0 0 2.592 Q 0.16011 6.11593 2.72813 8.579 Q 5.30441 11.05 8.85 11.05 Q 12.3958 11.05 14.9722 8.579 Q 17.5406 6.11575 17.7 2.592 Q 14.2315 0 8.85 0 " }
                }
            }
        }
    }
    LayerItem {
        objectName: "Path 4485"
        id: _qt_layer8
        transform: TransformGroup {
            id: _qt_layer8_transform_base_group
            TransformGroup {
                id: _qt_layer8_transform_group_0
                Translate { x: -3.266; y: -3.745 }
                Scale { id: _qt_layer8_transform_0_2}
                Translate { id: _qt_layer8_transform_0_0 }
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
                                script:_qt_layer8_transform_0_0.x = 155.851
                            }
                            ScriptAction {
                                script:_qt_layer8_transform_0_0.y = 85.524
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer8_transform_0_0
                                property: "x"
                                to: 166.351
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer8_transform_0_0
                                property: "y"
                                to: 85.524
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_0
                                property: "x"
                                to: 155.851
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_0
                                property: "y"
                                to: 85.524
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_0
                                property: "x"
                                to: 166.351
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_0
                                property: "y"
                                to: 85.524
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer8_transform_0_0
                                property: "x"
                                to: 155.851
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer8_transform_0_0
                                property: "y"
                                to: 85.524
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2600 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer8_transform_0_2.xScale = 1
                            }
                            ScriptAction {
                                script:_qt_layer8_transform_0_2.yScale = 1
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2600 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer8_transform_base_group.deactivateOverride(_qt_layer8_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node9
            ShapePath {
                id: _qt_shapePath_7
                strokeColor: "transparent"
                fillColor: "#ffd31b5c"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.334 1.927 Q 6.28372 1.04598 5.50325 0.47825 Q 4.78958 -0.0408892 3.963 0.003 Q 2.79425 0.064875 1.28025 2.03163 Q 0.52325 3.015 0 3.986 Q 0.631 4.89025 1.49575 5.78 Q 3.22525 7.5595 4.394 7.487 Q 5.21982 7.43579 5.8695 6.83787 Q 6.57999 6.18399 6.529 5.303 Q 6.50567 4.89931 6.247 4.40675 Q 6.01422 3.96348 5.71 3.656 Q 5.97683 3.31573 6.157 2.84925 Q 6.35732 2.33059 6.334 1.927 " }
            }
        }
    }
    LayerItem {
        objectName: "Path 4484"
        id: _qt_layer9
        transform: TransformGroup {
            id: _qt_layer9_transform_base_group
            TransformGroup {
                id: _qt_layer9_transform_group_0
                Translate { x: -3.745; y: -3.266 }
                Scale { id: _qt_layer9_transform_0_2}
                Translate { id: _qt_layer9_transform_0_0 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer9_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer9_transform_animation
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
                                script:_qt_layer9_transform_0_0.x = 136.546
                            }
                            ScriptAction {
                                script:_qt_layer9_transform_0_0.y = 71.227
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer9_transform_0_0
                                property: "x"
                                to: 136.546
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer9_transform_0_0
                                property: "y"
                                to: 59.227
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_0
                                property: "x"
                                to: 136.546
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_0
                                property: "y"
                                to: 71.227
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_0
                                property: "x"
                                to: 136.546
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_0
                                property: "y"
                                to: 59.227
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer9_transform_0_0
                                property: "x"
                                to: 136.546
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer9_transform_0_0
                                property: "y"
                                to: 71.227
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2600 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer9_transform_0_2.xScale = 1
                            }
                            ScriptAction {
                                script:_qt_layer9_transform_0_2.yScale = 1
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer9_transform_0_2
                                property: "xScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer9_transform_0_2
                                property: "yScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_2
                                property: "xScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_2
                                property: "yScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer9_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer9_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2600 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer9_transform_base_group.deactivateOverride(_qt_layer9_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node10
            ShapePath {
                id: _qt_shapePath_8
                strokeColor: "transparent"
                fillColor: "#ffd31b5c"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 1.927 0.197 Q 1.04548 0.247979 0.477875 1.029 Q -0.04091 1.74284 0.003 2.569 Q 0.0655 3.73775 2.0315 5.25175 Q 3.0145 6.00875 3.985 6.532 Q 4.8895 5.901 5.7795 5.03625 Q 7.5595 3.30675 7.487 2.138 Q 7.43578 1.31197 6.83825 0.6625 Q 6.18459 -0.0479834 5.303 0.003 Q 4.89931 0.0263304 4.40675 0.285 Q 3.96348 0.517783 3.656 0.822 Q 3.31543 0.554931 2.84925 0.3745 Q 2.33041 0.173686 1.927 0.197 " }
            }
        }
    }
    LayerItem {
        objectName: "Path 4483"
        id: _qt_layer10
        transform: TransformGroup {
            id: _qt_layer10_transform_base_group
            TransformGroup {
                id: _qt_layer10_transform_group_0
                Translate { x: -6.473; y: -6.353 }
                Scale { id: _qt_layer10_transform_0_2}
                Translate { id: _qt_layer10_transform_0_0 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer10_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer10_transform_animation
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
                                script:_qt_layer10_transform_0_0.x = 149.273
                            }
                            ScriptAction {
                                script:_qt_layer10_transform_0_0.y = 73.541
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer10_transform_0_0
                                property: "x"
                                to: 154.523
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer10_transform_0_0
                                property: "y"
                                to: 68.041
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_0
                                property: "x"
                                to: 149.273
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_0
                                property: "y"
                                to: 73.541
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_0
                                property: "x"
                                to: 154.523
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_0
                                property: "y"
                                to: 68.041
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer10_transform_0_0
                                property: "x"
                                to: 149.273
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer10_transform_0_0
                                property: "y"
                                to: 73.541
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2600 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer10_transform_0_2.xScale = 1
                            }
                            ScriptAction {
                                script:_qt_layer10_transform_0_2.yScale = 1
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer10_transform_0_2
                                property: "xScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 467
                                target: _qt_layer10_transform_0_2
                                property: "yScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_2
                                property: "xScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer10_transform_0_2
                                property: "yScale"
                                to: 1.16
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer10_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 533
                                target: _qt_layer10_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2600 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer10_transform_base_group.deactivateOverride(_qt_layer10_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node11
            ShapePath {
                id: _qt_shapePath_9
                strokeColor: "transparent"
                fillColor: "#ffd31b5c"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.9 0.893 Q 5.63386 -0.235758 3.801 0.054125 Q 2.12535 0.319143 1.062 1.502 Q -0.44175 3.17512 0.17325 7.90438 Q 0.48075 10.269 1.089 12.299 Q 3.17575 12.6702 5.56025 12.7043 Q 10.3293 12.7723 11.818 11.086 Q 12.8708 9.89335 12.9415 8.19862 Q 13.0188 6.34474 11.752 5.216 Q 11.1726 4.69947 10.1508 4.38225 Q 9.23217 4.09708 8.401 4.093 Q 8.30058 3.26679 7.91187 2.38762 Q 7.47944 1.40955 6.9 0.893 " }
            }
        }
    }
    LayerItem {
        objectName: "comp_1"
        id: _qt_layer11
        transform: TransformGroup {
            id: _qt_layer11_transform_base_group
            TransformGroup {
                id: _qt_layer11_transform_group_0
                Translate { x: -90; y: -110 }
                Rotation { id: _qt_layer11_transform_0_1 }
                Translate { x: 90; y: 110 }
            }
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
                                script:_qt_layer11_transform_0_1.angle = 0
                            }
                        }
                        PauseAnimation { duration: 267 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 400
                                target: _qt_layer11_transform_0_1
                                property: "angle"
                                to: 2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 300
                                target: _qt_layer11_transform_0_1
                                property: "angle"
                                to: -4
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 266
                                target: _qt_layer11_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2267 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 433
                                target: _qt_layer11_transform_0_1
                                property: "angle"
                                to: 2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 300
                                target: _qt_layer11_transform_0_1
                                property: "angle"
                                to: -4
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 300
                                target: _qt_layer11_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2467 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer11_transform_base_group.deactivateOverride(_qt_layer11_transform_group_0)
                    }
                }
            }
        }
        LayerItem {
            objectName: "Path 4486"
            id: _qt_layer12
            transform: TransformGroup {
                id: _qt_layer12_transform_base_group
                Matrix4x4 { id: _qt_layer12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 32.673, 81.974)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node12
                ShapePath {
                    id: _qt_shapePath_10
                    strokeColor: "transparent"
                    fillColor: "#ffd6135a"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 111.676 8.211 L 110.441 8.211 Q 110.441 8.0875 110.317 7.8405 Q 110.194 7.5935 110.194 7.47 Q 108.175 3.79972 104.357 1.81813 Q 100.605 -0.129173 96.111 0.058 Q 91.8936 0.359157 87.71 3.022 Q 85.4521 1.58488 83.9114 0.983875 Q 81.9816 0.231137 79.557 0.058 Q 75.2944 -0.312529 71.5271 1.57062 Q 67.7593 3.45403 65.721 6.975 Q 61.8366 4.504 57.321 4.504 Q 53.6145 4.504 49.908 6.975 Q 47.8697 3.45408 44.102 1.57062 Q 40.3349 -0.312535 36.073 0.058 Q 33.9267 0.236801 32.0578 0.983875 Q 30.3478 1.66741 28.413 3.022 Q 26.1551 1.58488 24.6144 0.983875 Q 22.6846 0.231137 20.26 0.058 Q 15.8429 -0.30997 12.014 1.7255 Q 8.23001 3.73711 6.177 7.47 Q 6.177 7.5935 6.0535 7.8405 Q 5.93 8.0875 5.93 8.211 L 4.447 8.211 Q 2.59433 8.211 1.29725 9.50788 Q -2.22045e-16 10.8049 0 12.658 Q 0 14.5112 1.29725 15.8086 Q 2.59442 17.106 4.447 17.106 L 4.694 17.106 Q 5.07707 19.4052 6.05312 21.8 Q 7.09399 24.3538 8.895 27.235 Q 13.0057 33.4981 18.9014 39.0942 Q 25.1485 45.024 28.166 45.024 L 28.413 45.024 Q 31.4307 45.024 37.678 39.0942 Q 43.5745 33.4975 47.684 27.235 Q 50.9899 22.2768 51.885 17.352 Q 54.9733 13.646 57.815 13.646 L 58.062 13.646 Q 60.7799 13.646 64.239 17.599 Q 65.1351 22.5293 68.438 27.483 Q 72.5485 33.7458 78.4449 39.3423 Q 84.6924 45.272 87.71 45.272 L 87.957 45.272 Q 90.9747 45.272 97.2225 39.3423 Q 103.12 33.7453 107.229 27.483 Q 109.03 24.6018 110.07 22.0466 Q 111.046 19.6519 111.429 17.352 L 111.676 17.352 Q 113.529 17.352 114.827 16.0551 Q 116.124 14.7582 116.124 12.905 Q 116.124 11.0281 114.827 9.63138 Q 113.507 8.211 111.676 8.211 " }
                }
            }
        }
        LayerItem {
            objectName: "Rectangle 2187"
            id: _qt_layer13
            transform: TransformGroup {
                id: _qt_layer13_transform_base_group
                Matrix4x4 { id: _qt_layer13_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 126.905, 106.944)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node13
                ShapePath {
                    id: _qt_shapePath_11
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -2.534 -4.448 Q -2.534 -6.64582 -1.79181 -8.19991 Q -1.04962 -9.754 0 -9.754 Q 1.04962 -9.754 1.79181 -8.19991 Q 2.534 -6.64582 2.534 -4.448 L 2.534 4.448 Q 2.534 6.64582 1.79181 8.19991 Q 1.04962 9.754 0 9.754 Q -1.04962 9.754 -1.79181 8.19991 Q -2.534 6.64582 -2.534 4.448 L -2.534 -4.448 " }
                }
            }
        }
        LayerItem {
            objectName: "Rectangle 2183"
            id: _qt_layer14
            transform: TransformGroup {
                id: _qt_layer14_transform_base_group
                Matrix4x4 { id: _qt_layer14_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 121.397, 102.352)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node14
                ShapePath {
                    id: _qt_shapePath_12
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -2.534 0 Q -2.534 -1.89544 -1.79181 -3.23572 Q -1.04962 -4.576 0 -4.576 Q 1.04962 -4.576 1.79181 -3.23572 Q 2.534 -1.89544 2.534 0 Q 2.534 1.89544 1.79181 3.23572 Q 1.04962 4.576 0 4.576 Q -1.04962 4.576 -1.79181 3.23572 Q -2.534 1.89544 -2.534 0 " }
                }
            }
        }
        LayerItem {
            objectName: "Rectangle 2185"
            id: _qt_layer15
            transform: TransformGroup {
                id: _qt_layer15_transform_base_group
                Matrix4x4 { id: _qt_layer15_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 113.92, 109.829)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node15
                ShapePath {
                    id: _qt_shapePath_13
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -2.534 -0.5265 Q -2.534 -2.72432 -1.79181 -4.27841 Q -1.04962 -5.8325 0 -5.8325 Q 1.04962 -5.8325 1.79181 -4.27841 Q 2.534 -2.72432 2.534 -0.5265 L 2.534 0.5265 Q 2.534 2.72432 1.79181 4.27841 Q 1.04962 5.8325 0 5.8325 Q -1.04962 5.8325 -1.79181 4.27841 Q -2.534 2.72432 -2.534 0.5265 L -2.534 -0.5265 " }
                }
            }
        }
        LayerItem {
            objectName: "Rectangle 2186"
            id: _qt_layer16
            transform: TransformGroup {
                id: _qt_layer16_transform_base_group
                Matrix4x4 { id: _qt_layer16_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 70.29, 106.858)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node16
                ShapePath {
                    id: _qt_shapePath_14
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -2.534 -4.448 Q -2.534 -6.64582 -1.79181 -8.19991 Q -1.04962 -9.754 0 -9.754 Q 1.04962 -9.754 1.79181 -8.19991 Q 2.534 -6.64582 2.534 -4.448 L 2.534 4.448 Q 2.534 6.64582 1.79181 8.19991 Q 1.04962 9.754 0 9.754 Q -1.04962 9.754 -1.79181 8.19991 Q -2.534 6.64582 -2.534 4.448 L -2.534 -4.448 " }
                }
            }
        }
        LayerItem {
            objectName: "Rectangle 2184"
            id: _qt_layer17
            transform: TransformGroup {
                id: _qt_layer17_transform_base_group
                Matrix4x4 { id: _qt_layer17_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 62.624, 103.939)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node17
                ShapePath {
                    id: _qt_shapePath_15
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -2.534 -0.0445 Q -2.534 -2.24232 -1.79181 -3.79641 Q -1.04962 -5.3505 0 -5.3505 Q 1.04962 -5.3505 1.79181 -3.79641 Q 2.534 -2.24232 2.534 -0.0445 L 2.534 0.0445 Q 2.534 2.24232 1.79181 3.79641 Q 1.04962 5.3505 0 5.3505 Q -1.04962 5.3505 -1.79181 3.79641 Q -2.534 2.24232 -2.534 0.0445 L -2.534 -0.0445 " }
                }
            }
        }
        LayerItem {
            objectName: "Rectangle 2182"
            id: _qt_layer18
            transform: TransformGroup {
                id: _qt_layer18_transform_base_group
                Matrix4x4 { id: _qt_layer18_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 55.247, 111.315)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node18
                ShapePath {
                    id: _qt_shapePath_16
                    strokeColor: "transparent"
                    fillColor: "#ffffffff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -2.534 0 Q -2.534 -1.89544 -1.79181 -3.23572 Q -1.04962 -4.576 0 -4.576 Q 1.04962 -4.576 1.79181 -3.23572 Q 2.534 -1.89544 2.534 0 Q 2.534 1.89544 1.79181 3.23572 Q 1.04962 4.576 0 4.576 Q -1.04962 4.576 -1.79181 3.23572 Q -2.534 1.89544 -2.534 0 " }
                }
            }
        }
    }
    LayerItem {
        objectName: "comp_0"
        id: _qt_layer19
        transform: TransformGroup {
            id: _qt_layer19_transform_base_group
            TransformGroup {
                id: _qt_layer19_transform_group_0
                Translate { x: -50.5; y: -41.5 }
                Scale { id: _qt_layer19_transform_0_2}
                Translate { id: _qt_layer19_transform_0_0 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer19_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer19_transform_animation
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
                                script:_qt_layer19_transform_0_0.x = 90
                            }
                            ScriptAction {
                                script:_qt_layer19_transform_0_0.y = 7
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1333
                                target: _qt_layer19_transform_0_0
                                property: "x"
                                to: 90.932
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.296, 0, 0.67, 0.421, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 1333
                                target: _qt_layer19_transform_0_0
                                property: "y"
                                to: 83.477
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.296, 0, 0.67, 0.421, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 967
                                target: _qt_layer19_transform_0_0
                                property: "x"
                                to: 92
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.43, 0.378, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 967
                                target: _qt_layer19_transform_0_0
                                property: "y"
                                to: 191.5
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.43, 0.378, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1633
                                target: _qt_layer19_transform_0_0
                                property: "x"
                                to: 90
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 1633
                                target: _qt_layer19_transform_0_0
                                property: "y"
                                to: 7
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.833, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 2367
                                target: _qt_layer19_transform_0_0
                                property: "x"
                                to: 92
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 2367
                                target: _qt_layer19_transform_0_0
                                property: "y"
                                to: 191.5
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer19_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer19_transform_0_2.yScale = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1033
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 1033
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1300
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 1.31
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 1300
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 1.31
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1600
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 1600
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.167, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1067
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 1067
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1333
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 1.31
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 1333
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 1.31
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 667 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer19_transform_base_group.deactivateOverride(_qt_layer19_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer19_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer19_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer19.opacity = 0
                    }
                    PropertyAnimation {
                        duration: 1033
                        target: _qt_layer19
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 1200 }
                    PropertyAnimation {
                        duration: 100
                        target: _qt_layer19
                        property: "opacity"
                        to: 0
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                    }
                    PauseAnimation { duration: 1600 }
                    PropertyAnimation {
                        duration: 1067
                        target: _qt_layer19
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 1200 }
                    PropertyAnimation {
                        duration: 133
                        target: _qt_layer19
                        property: "opacity"
                        to: 0
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: [ 0.333, 0, 0.833, 0.833, 1, 1 ]
                    }
                    PauseAnimation { duration: 667 }
                }
            }
        }
        LayerItem {
            objectName: "Path 4519"
            id: _qt_layer20
            transform: TransformGroup {
                id: _qt_layer20_transform_base_group
                Matrix4x4 { id: _qt_layer20_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -24.682, -26.548)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node19
                ShapePath {
                    id: _qt_shapePath_17
                    strokeColor: "transparent"
                    fillColor: "#ff5b29ff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 7.248 13.747 Q 7.20299 11.0241 9.10962 9.11725 Q 11.0167 7.21001 13.741 7.255 L 13.741 6.491 Q 11.0168 6.53599 9.11063 4.62925 Q 7.20499 2.72301 7.25 0 L 6.8685 0.0015 L 6.487 0.003 Q 6.53201 2.72592 4.62838 4.62975 Q 2.72433 6.53399 0 6.489 L 0 7.253 Q 2.7235 7.20802 4.62787 9.11625 Q 6.53102 11.0232 6.486 13.747 L 7.248 13.747 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4521"
            id: _qt_layer21
            transform: TransformGroup {
                id: _qt_layer21_transform_base_group
                Matrix4x4 { id: _qt_layer21_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 5.252, -32.586)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node20
                ShapePath {
                    id: _qt_shapePath_18
                    strokeColor: "transparent"
                    fillColor: "#ffffa63a"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 4.486 8.385 Q 4.45899 6.72339 5.588 5.59338 Q 6.71736 4.46301 8.379 4.49 L 8.377 3.896 Q 6.71591 3.92299 5.5875 2.79175 Q 4.45999 1.66142 4.487 0 L 4.188 0.001 L 3.889 0.002 Q 3.91675 1.66333 2.78938 2.7905 Q 1.66192 3.91775 0 3.89 L 0.001 4.192 L 0.002 4.494 Q 1.66352 4.46701 2.78988 5.59538 Q 3.91576 6.72327 3.888 8.385 L 4.486 8.385 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4735"
            id: _qt_layer22
            transform: TransformGroup {
                id: _qt_layer22_transform_base_group
                Matrix4x4 { id: _qt_layer22_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -9.529, -38.024)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node21
                transform: TransformGroup {
                    id: _qt_node21_transform_base_group
                    Matrix4x4 { id: _qt_node21_transform_base; matrix: PlanarTransform.fromAffineMatrix(1.42444, 0, 0, 1.42444, 0, 0)}
                }
                ShapePath {
                    id: _qt_shapePath_19
                    strokeColor: "transparent"
                    fillColor: "#ff3aaeff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 4.486 8.385 Q 4.45899 6.72339 5.588 5.59338 Q 6.71736 4.46301 8.379 4.49 L 8.377 3.896 Q 6.71591 3.92299 5.5875 2.79175 Q 4.45999 1.66142 4.487 0 L 4.188 0.001 L 3.889 0.002 Q 3.91675 1.66333 2.78938 2.7905 Q 1.66192 3.91775 0 3.89 L 0.001 4.192 L 0.002 4.494 Q 1.66352 4.46701 2.78988 5.59538 Q 3.91576 6.72327 3.888 8.385 L 4.486 8.385 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4522"
            id: _qt_layer23
            transform: TransformGroup {
                id: _qt_layer23_transform_base_group
                Matrix4x4 { id: _qt_layer23_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 22.908, 2.859)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node22
                ShapePath {
                    id: _qt_shapePath_20
                    strokeColor: "#ffec9454"
                    strokeWidth: 2
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    miterLimit: 0
                    fillColor: "transparent"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 0 0 Q 1.017 0.572 2.212 1.147 L 2.44582 1.25772 Q 3.33615 1.67685 3.67956 1.94885 Q 4.257 2.4062 4.257 2.993 Q 4.257 3.65304 3.57307 4.06874 Q 3.16358 4.31764 2.07871 4.64896 Q 1.03533 4.9676 0.646581 5.1991 Q 0 5.58414 0 6.186 Q 0 6.78562 0.642069 7.25465 Q 1.0283 7.5368 2.06654 7.99325 Q 3.1564 8.4724 3.56846 8.77978 Q 4.257 9.2934 4.257 9.966 Q 4.257 10.6217 3.60061 11.155 Q 3.20639 11.4752 2.15192 12.0031 Q 1.07794 12.5408 0.673962 12.8715 Q 0 13.4232 0 14.108 Q 0 14.7902 0.677781 15.2743 Q 1.08388 15.5643 2.1621 15.9946 Q 3.22679 16.4195 3.62069 16.7019 Q 4.27627 17.1718 4.257 17.832 Q 4.24106 18.3776 3.72275 18.7847 Q 3.34345 19.0827 2.212 19.613 L 0 20.668 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4523"
            id: _qt_layer24
            transform: TransformGroup {
                id: _qt_layer24_transform_base_group
                Matrix4x4 { id: _qt_layer24_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.87462, 0.48481, -0.48481, 0.87462, -39.5441, -9.63882)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node23
                ShapePath {
                    id: _qt_shapePath_21
                    strokeColor: "#ffb40342"
                    strokeWidth: 1.5
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    miterLimit: 0
                    fillColor: "transparent"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 0 0 L 3.833 0.884 L 4.23855 0.969517 Q 5.78108 1.2929 6.3759 1.50269 Q 7.376 1.85542 7.376 2.308 Q 7.376 3.07237 3.60078 3.58535 Q 0 4.07464 0 4.77 Q 0 5.4628 3.58072 6.16374 Q 7.376 6.90669 7.376 7.685 Q 7.376 8.44283 3.72905 9.2561 Q -4.44089e-16 10.0877 0 10.88 Q 0 11.6693 3.74534 12.334 Q 7.42582 12.9872 7.376 13.751 Q 7.34828 14.1717 6.45013 14.4855 Q 6.04185 14.6282 5.34917 14.7928 L 3.833 15.124 L 0 15.939 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4733"
            id: _qt_layer25
            transform: TransformGroup {
                id: _qt_layer25_transform_base_group
                Matrix4x4 { id: _qt_layer25_transform_base; matrix: PlanarTransform.fromAffineMatrix(-0.87462, 0.48481, -0.48481, -0.87462, 49.9943, 31.3011)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node24
                ShapePath {
                    id: _qt_shapePath_22
                    strokeColor: "#ffb40342"
                    strokeWidth: 1
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    miterLimit: 0
                    fillColor: "transparent"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic
                    PathSvg { path: "M 0 12.665 L 3.045 11.962 L 3.36622 11.8943 Q 4.5926 11.6373 5.06563 11.4706 Q 5.861 11.1904 5.861 10.831 Q 5.861 10.2236 2.8613 9.81623 Q 4.44089e-16 9.42766 0 8.875 Q 0 8.32444 2.8449 7.76739 Q 5.861 7.17682 5.861 6.558 Q 5.861 5.95543 2.96261 5.30966 Q -4.44089e-16 4.64957 0 4.02 Q 0 3.39274 2.97614 2.86415 Q 5.90036 2.3448 5.861 1.738 Q 5.83952 1.40402 5.12537 1.15463 Q 4.8009 1.04131 4.25014 0.910422 L 3.045 0.647 L 0 0 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 2161"
            id: _qt_layer26
            transform: TransformGroup {
                id: _qt_layer26_transform_base_group
                Matrix4x4 { id: _qt_layer26_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -29.963, 3.264)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node25
                ShapePath {
                    id: _qt_shapePath_23
                    strokeColor: "transparent"
                    fillColor: "#ff66ccff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 0 5.419 Q 0.773 4.446 1.45 3.091 Q 1.80519 2.38191 2.267 1.212 Q 2.503 0.617 2.71 0 L 3.08087 1.03737 Q 3.53163 2.24412 3.931 3.091 Q 4.57 4.446 5.266 5.419 Q 4.57 6.325 3.931 7.632 Q 3.53163 8.44888 3.08087 9.62963 L 2.71 10.647 Q 2.56425 10.22 2.33387 9.62963 Q 1.87312 8.44888 1.45 7.632 Q 0.773 6.325 0 5.419 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4734"
            id: _qt_layer27
            transform: TransformGroup {
                id: _qt_layer27_transform_base_group
                Matrix4x4 { id: _qt_layer27_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 43.037, 2.264)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node26
                ShapePath {
                    id: _qt_shapePath_24
                    strokeColor: "transparent"
                    fillColor: "#ff66ccff"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 0 5.419 Q 0.773 4.446 1.45 3.091 Q 1.80519 2.38191 2.267 1.212 Q 2.503 0.617 2.71 0 L 3.08087 1.03737 Q 3.53163 2.24412 3.931 3.091 Q 4.57 4.446 5.266 5.419 Q 4.57 6.325 3.931 7.632 Q 3.53163 8.44888 3.08087 9.62963 L 2.71 10.647 Q 2.56425 10.22 2.33387 9.62963 Q 1.87312 8.44888 1.45 7.632 Q 0.773 6.325 0 5.419 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 2162"
            id: _qt_layer28
            transform: TransformGroup {
                id: _qt_layer28_transform_base_group
                Matrix4x4 { id: _qt_layer28_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 3.802, -11.983)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node27
                ShapePath {
                    id: _qt_shapePath_25
                    strokeColor: "transparent"
                    fillColor: "#ffeb6f6d"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 0 5.419 Q 0.773 4.446 1.45 3.091 Q 1.80519 2.38191 2.267 1.212 Q 2.503 0.617 2.71 0 L 3.08087 1.03737 Q 3.53163 2.24412 3.931 3.091 Q 4.57 4.446 5.266 5.419 Q 4.57 6.325 3.931 7.632 Q 3.53163 8.44888 3.08087 9.62963 L 2.71 10.647 Q 2.56425 10.22 2.33387 9.62963 Q 1.87312 8.44888 1.45 7.632 Q 0.773 6.325 0 5.419 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 2173"
            id: _qt_layer29
            transform: TransformGroup {
                id: _qt_layer29_transform_base_group
                Matrix4x4 { id: _qt_layer29_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -49.994, 23.874)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node28
                ShapePath {
                    id: _qt_shapePath_26
                    strokeColor: "transparent"
                    fillColor: "#ff4e024e"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 3.046 0 Q 4.3076 0 5.20025 0.89225 Q 6.093 1.7846 6.093 3.046 Q 6.093 4.3075 5.20025 5.20025 Q 4.3075 6.093 3.046 6.093 Q 1.7846 6.093 0.89225 5.20025 Q -1.11022e-16 4.3076 0 3.046 Q 0 1.7845 0.89225 0.89225 Q 1.7845 -1.11022e-16 3.046 0 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node29
            }
        }
        LayerItem {
            objectName: "Rectangle 2052"
            id: _qt_layer30
            transform: TransformGroup {
                id: _qt_layer30_transform_base_group
                Matrix4x4 { id: _qt_layer30_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 34.434, -12.42)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node30
                ShapePath {
                    id: _qt_shapePath_27
                    strokeColor: "transparent"
                    fillColor: "#ffd31b5c"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -3.0465 -3.0465 L 3.0465 -3.0465 L 3.0465 3.0465 L -3.0465 3.0465 L -3.0465 -3.0465 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node31
            }
        }
        LayerItem {
            objectName: "Rectangle 2532"
            id: _qt_layer31
            transform: TransformGroup {
                id: _qt_layer31_transform_base_group
                Matrix4x4 { id: _qt_layer31_transform_base; matrix: PlanarTransform.fromAffineMatrix(0.707107, 0.707107, -0.707107, 0.707107, 10.434, 31.58)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node32
                ShapePath {
                    id: _qt_shapePath_28
                    strokeColor: "transparent"
                    fillColor: "#ffd31b5c"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M -3.0465 -3.0465 L 3.0465 -3.0465 L 3.0465 3.0465 L -3.0465 3.0465 L -3.0465 -3.0465 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node33
            }
        }
        LayerItem {
            objectName: "Path 2174"
            id: _qt_layer32
            transform: TransformGroup {
                id: _qt_layer32_transform_base_group
                Matrix4x4 { id: _qt_layer32_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -11.199, -2.694)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node34
                ShapePath {
                    id: _qt_shapePath_29
                    strokeColor: "transparent"
                    fillColor: "#ff424242"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 2.031 0 Q 2.8725 0 3.46725 0.59475 Q 4.062 1.1895 4.062 2.031 Q 4.062 2.8725 3.46725 3.46725 Q 2.8725 4.062 2.031 4.062 Q 1.1895 4.062 0.59475 3.46725 Q 1.11022e-16 2.8725 0 2.031 Q 0 1.1895 0.59475 0.59475 Q 1.1895 0 2.031 0 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node35
            }
        }
        LayerItem {
            objectName: "Path 4473"
            id: _qt_layer33
            transform: TransformGroup {
                id: _qt_layer33_transform_base_group
                Matrix4x4 { id: _qt_layer33_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -7.817, -11.454)}
                Matrix4x4 { matrix: _qt_layer38.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node36
            }
        }
        LayerItem {
            objectName: "Path 4474"
            id: _qt_layer34
            transform: TransformGroup {
                id: _qt_layer34_transform_base_group
                Matrix4x4 { id: _qt_layer34_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -9.453, -11.454)}
                Matrix4x4 { matrix: _qt_layer38.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node37
                ShapePath {
                    id: _qt_shapePath_30
                    strokeColor: "transparent"
                    fillColor: "#ffffa63a"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 10.423 16.678 L 10.7815 16.6586 Q 11.3048 16.6325 11.565 16.603 Q 12.0522 16.5471 12.4221 16.4334 Q 12.8661 16.2969 13.211 16.059 Q 13.4848 15.866 13.709 15.61 Q 13.7952 15.5238 13.7948 15.402 Q 13.7943 15.2803 13.708 15.194 L 13.696 15.183 Q 13.5745 15.0307 13.41 14.935 Q 13.071 14.7496 12.711 14.635 Q 11.4139 14.2616 9.71 14.534 Q 8.58502 14.7075 7.647 15.344 Q 7.47752 15.4635 7.343 15.616 Q 7.27489 15.6796 7.27125 15.7723 Q 7.26762 15.8646 7.33 15.933 Q 7.339 15.942 7.359 15.958 Q 7.52321 16.1055 7.727 16.187 Q 8.8209 16.6034 10.423 16.678 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node38
                ShapePath {
                    id: _qt_shapePath_31
                    strokeColor: "transparent"
                    fillColor: "#ffffa63a"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 11.548 7.831 L 10.12 7.89787 Q 9.36651 7.95266 8.773 8.066 Q 7.48937 8.29939 6.39 8.995 Q 6.1921 9.11944 6.038 9.301 Q 5.97353 9.37072 5.97738 9.46612 Q 5.98122 9.56147 6.051 9.626 Q 6.08994 9.66182 6.137 9.678 Q 6.22821 9.71959 6.32688 9.75575 Q 6.4198 9.78981 6.522 9.82 Q 7.75675 10.1747 9.053 10.27 Q 11.458 10.463 13.831 10.063 Q 14.2546 9.99042 14.7376 9.87925 L 15.636 9.655 Q 15.916 9.55555 16.122 9.345 Q 16.2172 9.27081 16.2318 9.15112 Q 16.2463 9.03158 16.172 8.937 Q 16.1626 8.92505 16.1546 8.91625 Q 16.1447 8.90532 16.135 8.897 Q 15.9553 8.6807 15.711 8.545 Q 15.2795 8.32853 14.793 8.195 Q 14.0596 8.00528 13.1458 7.91888 Q 12.5444 7.86202 11.548 7.831 " }
                }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node39
                ShapePath {
                    id: _qt_shapePath_32
                    strokeColor: "transparent"
                    fillColor: "#ffffa63a"
                    fillRule: ShapePath.WindingFill
                    pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                    PathSvg { path: "M 13.339 0 L 13.5031 0.0238949 Q 13.7279 0.0582665 13.841 0.065 Q 16.0859 0.202199 18.241 0.81 Q 18.5177 0.873735 18.6926 1.09713 Q 18.8675 1.32043 18.863 1.604 Q 18.8615 1.8726 18.6909 2.07938 Q 18.5202 2.28627 18.257 2.338 Q 18.004 2.37787 17.763 2.315 Q 16.2465 1.92055 14.67 1.762 Q 12.87 1.59589 11.084 1.702 Q 8.95957 1.81492 6.908 2.359 Q 4.94536 2.86174 3.259 3.989 Q 2.56286 4.45814 2.045 5.116 Q 1.60862 5.65361 1.63762 6.34562 Q 1.66662 7.03737 2.146 7.537 Q 2.84944 8.28744 3.761 8.75 Q 4.05684 8.90646 4.357 8.615 Q 5.08513 7.90943 5.977 7.422 Q 7.50701 6.5935 9.234 6.371 Q 11.6689 6.01689 14.086 6.419 Q 14.787 6.53847 15.3313 6.70662 Q 15.9795 6.90692 16.509 7.205 Q 17.3244 7.62353 17.701 8.46 Q 17.9529 9.0392 17.7324 9.71362 Q 17.5193 10.3654 17.016 10.686 Q 16.2707 11.1447 15.426 11.366 Q 13.7821 11.8079 12.088 11.886 Q 10.8667 11.956 9.678 11.922 Q 7.3261 11.8554 5.15 10.975 Q 4.87604 10.8642 4.707 10.938 Q 4.53375 11.0136 4.444 11.294 Q 4.08675 12.3844 4.551 13.434 Q 4.85956 14.1226 5.39 14.653 Q 5.54831 14.8181 5.68925 14.8202 Q 5.83184 14.8224 5.999 14.659 Q 6.86391 13.8351 7.959 13.375 Q 9.64183 12.6372 11.471 12.859 Q 12.2895 12.9496 13.106 13.142 Q 13.8808 13.3106 14.523 13.77 Q 15.2714 14.2856 15.44 15.177 Q 15.5059 15.6582 15.274 16.088 Q 14.8708 16.946 13.839 17.519 Q 12.5263 18.2263 11.038 18.284 Q 9.06101 18.3795 7.222 17.643 Q 6.9674 17.5439 6.88075 17.6034 Q 6.79376 17.6631 6.799 17.935 Q 6.81276 18.5377 7.049 19.094 Q 7.2677 19.609 7.683 20.097 Q 8.68378 21.2728 10.323 21.265 Q 10.5455 21.2544 10.762 21.29 Q 10.9857 21.3275 11.184 21.445 Q 11.652 21.7534 11.561 22.199 Q 11.5137 22.4407 11.3427 22.6181 Q 11.1715 22.7958 10.931 22.852 Q 10.8689 22.8706 10.738 22.8943 L 10.66 22.909 L 10.247 22.909 L 10.0232 22.879 L 9.609 22.82 Q 8.81543 22.6907 8.15287 22.3908 Q 7.4498 22.0724 6.885 21.558 Q 5.73217 20.5057 5.269 18.658 Q 5.14095 18.1476 5.12036 17.1093 L 5.116 16.903 Q 5.11444 16.6417 4.94 16.447 Q 4.80699 16.3016 4.668 16.18 Q 3.53866 15.1878 2.993 13.791 Q 2.76153 13.1972 2.6925 12.7109 Q 2.60896 12.1223 2.743 11.591 Q 2.78806 11.4114 2.91402 11.0612 Q 3.00054 10.8207 3.038 10.699 Q 3.13432 10.3876 3.06713 10.2294 Q 3.0008 10.0732 2.713 9.924 Q 1.09401 9.08785 0.409 7.766 Q -0.136997 6.71006 0.05 5.797 Q 0.347235 4.35406 1.674 3.202 Q 2.26628 2.68865 3.03713 2.252 Q 3.6743 1.89107 4.533 1.53 Q 5.98823 0.919368 7.614 0.55675 Q 9.1184 0.221202 10.812 0.089 L 11.4084 0.0388352 L 11.865 0 L 13.339 0 " }
                }
            }
        }
        LayerItem {
            objectName: "Path 4475"
            id: _qt_layer35
            transform: TransformGroup {
                id: _qt_layer35_transform_base_group
                Matrix4x4 { id: _qt_layer35_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -9.481, -11.454)}
                Matrix4x4 { matrix: _qt_layer38.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node40
            }
        }
        LayerItem {
            objectName: "Path 4476"
            id: _qt_layer36
            transform: TransformGroup {
                id: _qt_layer36_transform_base_group
                Matrix4x4 { id: _qt_layer36_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -3.475, -3.623)}
                Matrix4x4 { matrix: _qt_layer38.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node41
            }
        }
        LayerItem {
            objectName: "Path 4477"
            id: _qt_layer37
            transform: TransformGroup {
                id: _qt_layer37_transform_base_group
                Matrix4x4 { id: _qt_layer37_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -2.183, 2.962)}
                Matrix4x4 { matrix: _qt_layer38.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node42
            }
        }
        LayerItem {
            objectName: "Group 1943"
            id: _qt_layer38
            transform: TransformGroup {
                id: _qt_layer38_transform_base_group
                Matrix4x4 { matrix: _qt_layer39.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node43
            }
        }
        LayerItem {
            objectName: "noun dizziness 2101908"
            id: _qt_layer39
            transform: TransformGroup {
                id: _qt_layer39_transform_base_group
                Matrix4x4 { id: _qt_layer39_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -15.487, 29.815)}
                Matrix4x4 { matrix: _qt_layer40.transformMatrix }
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node44
            }
        }
        LayerItem {
            objectName: "Group 5658"
            id: _qt_layer40
            transform: TransformGroup {
                id: _qt_layer40_transform_base_group
                Matrix4x4 { id: _qt_layer40_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 49.994, 41.269)}
            }
            Shape {
                preferredRendererType: Shape.CurveRenderer
                id: _qt_node45
            }
        }
    }
}
