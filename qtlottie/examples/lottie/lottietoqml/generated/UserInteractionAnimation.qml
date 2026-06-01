// Copyright (C) 2025 Mahendra Bhunwal
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file UserInteractionAnimation.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 1500
    implicitHeight: 1500
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 1500; yScale: height / 1500 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "Floor"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            Matrix4x4 { id: _qt_layer0_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 240.976, 1162.33)}
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node1
            transform: TransformGroup {
                id: _qt_node1_transform_base_group
                Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 561.657, 58.293)}
            }
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "transparent"
                fillColor: "#ffdaf0f3"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 561.408 0.001 Q 561.408 24.0435 396.975 41.0434 Q 232.545 58.043 0.001 58.043 Q -232.544 58.043 -396.975 41.0434 Q -561.407 24.0435 -561.407 0.001 Q -561.407 -24.0412 -396.975 -41.042 Q -232.541 -58.043 0.001 -58.043 Q 232.542 -58.043 396.975 -41.042 Q 561.408 -24.0411 561.408 0.001 " }
            }
        }
    }
    LayerItem {
        objectName: "Leaf 1"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            TransformGroup {
                id: _qt_layer1_transform_group_0
                Translate { x: -87.484; y: -492.853 }
                Rotation { id: _qt_layer1_transform_0_1 }
                Translate { x: 153.439; y: -215.027 }
            }
            Matrix4x4 { matrix: _qt_layer5.transformMatrix }
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
                                script:_qt_layer1_transform_0_1.angle = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1900
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 21
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 2100
                                target: _qt_layer1_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 33 }
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
            id: _qt_node2
            transform: TransformGroup {
                id: _qt_node2_transform_base_group
                Matrix4x4 { id: _qt_node2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 79.879, 258.155)}
            }
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "transparent"
                fillColor: "#ff32aa88"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -5.336 -145.628 Q -16.3029 -130.441 -25.015 -113.426 Q -63.9512 -37.3795 -51.4852 60.5185 Q -40.1973 149.166 8.587 229.864 Q 0.957608 129.598 29.572 52.858 Q 37.4302 31.7846 49.5719 -10.7796 L 60.68 -48.413 Q 76.0649 -97.2616 68.4741 -142.624 Q 60.8834 -187.987 30.317 -229.863 Q 32.4403 -220.714 31.0052 -210.876 Q 29.5701 -201.038 24.5767 -190.512 Q 18.6434 -178.004 1.24639 -154.574 L -5.336 -145.628 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            transform: TransformGroup {
                id: _qt_node3_transform_base_group
                Matrix4x4 { id: _qt_node3_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 98.653, 247.677)}
            }
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "#ff81c7af"
                strokeWidth: 1
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -7.559 245.176 Q -35.4522 140.093 -14.789 -5.283 Q -11.7501 -26.6654 -2.13562 -71.048 Q 23.3863 -188.862 7.964 -245.176 " }
            }
        }
    }
    LayerItem {
        objectName: "Leaf 2"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            TransformGroup {
                id: _qt_layer2_transform_group_0
                Translate { x: -143.414; y: -602.278 }
                Rotation { id: _qt_layer2_transform_0_1 }
                Translate { x: 111.625; y: -49.906 }
            }
            Matrix4x4 { matrix: _qt_layer5.transformMatrix }
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
                                script:_qt_layer2_transform_0_1.angle = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1900
                                target: _qt_layer2_transform_0_1
                                property: "angle"
                                to: 6
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 2100
                                target: _qt_layer2_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
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
                Matrix4x4 { id: _qt_node4_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 110.893, 327.705)}
            }
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "transparent"
                fillColor: "#ff81c7af"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -6.38 -188.927 Q 10.1015 -170.937 23.926 -150.351 Q 85.7489 -58.2954 84.4106 68.1755 Q 83.1988 182.696 32.306 291.847 Q 27.0306 162.966 -21.55 68.718 Q -34.8905 42.8373 -57.0129 -9.91174 L -77.06 -56.514 Q -104.327 -116.872 -101.361 -175.706 Q -98.3948 -234.539 -65.196 -291.847 Q -67.9236 -267.964 -51.8323 -242.378 Q -42.2721 -227.176 -16.2409 -199.501 L -6.38 -188.927 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node5
            transform: TransformGroup {
                id: _qt_node5_transform_base_group
                Matrix4x4 { id: _qt_node5_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 93.44, 314.278)}
            }
            ShapePath {
                id: _qt_shapePath_4
                strokeColor: "#ff32aa88"
                strokeWidth: 1
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 47.091 311.778 Q 67.2898 174.174 18.627 -8.648 Q 11.4699 -35.537 -7.66119 -90.9291 Q -58.4476 -237.976 -47.009 -311.778 " }
            }
        }
    }
    LayerItem {
        objectName: "Leaf 3"
        id: _qt_layer3
        transform: TransformGroup {
            id: _qt_layer3_transform_base_group
            TransformGroup {
                id: _qt_layer3_transform_group_0
                Translate { x: -281.166; y: -440.796 }
                Rotation { id: _qt_layer3_transform_0_1 }
                Translate { x: 143.018; y: 48.237 }
            }
            Matrix4x4 { matrix: _qt_layer5.transformMatrix }
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
                                script:_qt_layer3_transform_0_1.angle = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1900
                                target: _qt_layer3_transform_0_1
                                property: "angle"
                                to: 5
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 2100
                                target: _qt_layer3_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 33 }
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
            id: _qt_node6
            transform: TransformGroup {
                id: _qt_node6_transform_base_group
                Matrix4x4 { id: _qt_node6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 144.618, 231.44)}
            }
            ShapePath {
                id: _qt_shapePath_5
                strokeColor: "transparent"
                fillColor: "#ff32aa88"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -52.958 -145.652 Q -34.5552 -137.598 -17.277 -126.697 Q 59.9629 -77.9644 101.1 17.7388 Q 138.35 104.398 136.392 203.57 Q 89.472 108.257 21.473 53.457 Q 2.80131 38.4099 -31.4375 6.05442 L -62.068 -22.366 Q -102.719 -58.7406 -120.091 -104.042 Q -137.462 -149.343 -131.555 -203.57 Q -125.65 -184.672 -105.003 -170.763 Q -92.736 -162.499 -63.9061 -150.329 L -52.958 -145.652 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node7
            transform: TransformGroup {
                id: _qt_node7_transform_base_group
                Matrix4x4 { id: _qt_node7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 141.833, 221.648)}
            }
            ShapePath {
                id: _qt_shapePath_6
                strokeColor: "#ff81c7af"
                strokeWidth: 1
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 139.333 219.148 Q 108.693 108.776 11.117 -12.707 Q -3.23421 -30.5747 -36.1024 -65.9204 Q -123.355 -159.75 -139.333 -219.148 " }
            }
        }
    }
    LayerItem {
        objectName: "Leaf 4"
        id: _qt_layer4
        transform: TransformGroup {
            id: _qt_layer4_transform_base_group
            TransformGroup {
                id: _qt_layer4_transform_group_0
                Translate { x: -19.635; y: -593.982 }
                Rotation { id: _qt_layer4_transform_0_1 }
                Translate { x: 121.975; y: 66.19 }
            }
            Matrix4x4 { matrix: _qt_layer5.transformMatrix }
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
                                script:_qt_layer4_transform_0_1.angle = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1900
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 8
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 2100
                                target: _qt_layer4_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 33 }
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
            id: _qt_node8
            transform: TransformGroup {
                id: _qt_node8_transform_base_group
                Matrix4x4 { id: _qt_node8_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 137.315, 311.193)}
            }
            ShapePath {
                id: _qt_shapePath_7
                strokeColor: "transparent"
                fillColor: "#ff32aa88"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 32.872 -187.038 Q 13.0858 -172.771 -4.631 -155.416 Q -83.8504 -77.8235 -108.206 46.2884 Q -130.261 158.671 -102.579 275.88 Q -71.2596 150.756 -4.561 68.324 Q 13.7536 45.6898 46.1199 -1.4709 L 75.209 -43.037 Q 114.158 -96.6034 123.193 -154.814 Q 132.229 -213.025 111.351 -275.88 Q 109.175 -251.94 88.2251 -230.151 Q 75.7783 -217.206 44.6709 -195.389 L 32.872 -187.038 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node9
            transform: TransformGroup {
                id: _qt_node9_transform_base_group
                Matrix4x4 { id: _qt_node9_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 145.373, 298.241)}
            }
            ShapePath {
                id: _qt_shapePath_8
                strokeColor: "#ff81c7af"
                strokeWidth: 1
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M -109.344 295.741 Q -101.197 156.904 -16.444 -12.239 Q -3.97855 -37.1172 25.997 -87.4764 Q 105.567 -221.155 109.344 -295.741 " }
            }
        }
    }
    LayerItem {
        objectName: "Bucket"
        id: _qt_layer5
        transform: TransformGroup {
            id: _qt_layer5_transform_base_group
            Matrix4x4 { id: _qt_layer5_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 264.208, 905.17)}
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node10
            transform: TransformGroup {
                id: _qt_node10_transform_base_group
                Matrix4x4 { id: _qt_node10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 136.489, 169.795)}
            }
            ShapePath {
                id: _qt_shapePath_9
                strokeColor: "transparent"
                fillColor: "#ffefa068"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 118.258 -133.584 L -118.258 -133.584 Q -118.258 0 -76.763 133.584 L 76.763 133.584 Q 118.258 0.58 118.258 -133.584 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node11
            transform: TransformGroup {
                id: _qt_node11_transform_base_group
                Matrix4x4 { id: _qt_node11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 136.488, 18.23)}
            }
            ShapePath {
                id: _qt_shapePath_10
                strokeColor: "transparent"
                fillColor: "#ffd78a55"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 136.238 17.98 L -136.238 17.98 L -136.238 -17.98 L 136.238 -17.98 L 136.238 17.98 " }
            }
        }
    }
    LayerItem {
        objectName: "Thigh Left"
        id: _qt_layer6
        transform: TransformGroup {
            id: _qt_layer6_transform_base_group
            Matrix4x4 { id: _qt_layer6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -215.142, 113.796)}
            Matrix4x4 { matrix: _qt_layer11.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node12
            transform: TransformGroup {
                id: _qt_node12_transform_base_group
                Matrix4x4 { id: _qt_node12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 218.072, 181.89)}
            }
            ShapePath {
                id: _qt_shapePath_11
                strokeColor: "transparent"
                fillColor: "#ff4388a5"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -150.404 141.58 Q -105.995 169.958 -45.5313 160.671 Q 8.826 152.323 54.402 118.395 Q 88.9712 92.6618 138.687 15.0283 L 145.57 4.307 Q 160.528 -18.8781 171.949 -41.7221 Q 184.324 -66.4751 192.636 -91.001 L 195.42 -98.8969 Q 204.469 -124.127 206.094 -137.171 Q 208.934 -159.961 196.477 -169.943 Q 177.634 -185.042 144.734 -164.316 Q 109.386 -142.044 54.8135 -75.5653 L 38.482 -55.838 Q 27.315 -42.5763 12.5956 -27.6591 L -14.5829 -1.09903 L -15.554 -0.167 Q -29.7008 13.4116 -43.9585 16.5881 Q -54.9469 19.0363 -77.343 16.592 L -93.6854 14.6757 Q -125.517 10.8137 -141.546 10.521 L -144.424 10.4667 Q -183.614 9.71658 -200.304 14.485 Q -201.852 14.9269 -202.763 15.3442 Q -204.066 15.9406 -204.87 16.784 Q -206.005 17.9736 -206.843 20.708 Q -214.554 45.8427 -202.234 77.2742 Q -191.329 105.092 -170.523 125.538 Q -161.253 134.647 -150.404 141.58 " }
            }
        }
    }
    LayerItem {
        objectName: "Leg Left"
        id: _qt_layer7
        transform: TransformGroup {
            id: _qt_layer7_transform_base_group
            Matrix4x4 { id: _qt_layer7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 349.94, 4.94)}
            Matrix4x4 { matrix: _qt_layer6.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node13
            transform: TransformGroup {
                id: _qt_node13_transform_base_group
                Matrix4x4 { id: _qt_node13_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 102.896, 169.245)}
            }
            ShapePath {
                id: _qt_shapePath_12
                strokeColor: "transparent"
                fillColor: "#ff4388a5"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 43.722 -24.86 L 23.929 -72.036 Q 6.76237 -111.023 -8.061 -131.872 Q -14.7431 -141.268 -21.305 -147.819 Q -29.1713 -155.673 -37.639 -160.283 Q -47.1225 -165.445 -57.3758 -166.392 Q -68.3531 -167.406 -77.426 -163.238 Q -88.101 -158.335 -94.7181 -147.011 Q -100.91 -136.415 -101.673 -123.71 Q -102.347 -112.463 -98.9606 -99.7197 Q -96.2156 -89.391 -90.402 -76.835 Q -81.7785 -58.2116 -60.001 -23.474 L -38.8003 9.21602 Q -9.88687 53.0662 2.296 75.879 Q 11.7852 93.6494 34.5827 127.263 Q 52.3582 153.473 60.385 167.089 Q 60.7488 167.708 60.9982 168.012 Q 61.3872 168.487 61.832 168.691 Q 62.2779 168.895 62.887 168.876 Q 63.299 168.863 63.981 168.725 Q 67.4235 168.021 84.458 161.476 Q 102.481 154.55 102.16 153.312 Q 78.6992 62.7552 43.722 -24.86 " }
            }
        }
    }
    LayerItem {
        objectName: "Shooes Right"
        id: _qt_layer8
        transform: TransformGroup {
            id: _qt_layer8_transform_base_group
            TransformGroup {
                id: _qt_layer8_transform_group_0
                Translate { x: -19.561; y: -98.8 }
                Rotation { id: _qt_layer8_transform_0_1 }
                Translate { x: 766.713; y: 252.294 }
            }
            Matrix4x4 { matrix: _qt_layer9.transformMatrix }
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
                                script:_qt_layer8_transform_0_1.angle = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 1900
                                target: _qt_layer8_transform_0_1
                                property: "angle"
                                to: 10
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 2100
                                target: _qt_layer8_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 33 }
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
            id: _qt_node14
            transform: TransformGroup {
                id: _qt_node14_transform_base_group
                Matrix4x4 { id: _qt_node14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 31.502, 64.935)}
            }
            ShapePath {
                id: _qt_shapePath_13
                strokeColor: "transparent"
                fillColor: "#ffdf3a34"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -28.02 51.749 Q -28.1876 51.8672 -28.3499 51.9707 Q -28.5214 52.0802 -28.691 52.176 L -29.1421 52.4218 Q -29.9288 52.8419 -30.2643 53.103 Q -30.8261 53.5403 -31.017 54.079 Q -31.307 54.8948 -30.773 56.13 Q -26.7499 65.399 -7.425 64.227 Q 1.44588 63.6882 6.329 59.8745 Q 11.1345 56.1214 14.23 47.479 Q 30.7683 1.31377 31.234 -49.56 Q 31.2515 -51.5072 31.1589 -52.575 Q 31.0147 -54.2373 30.549 -55.497 Q 28.1613 -61.9627 21.0282 -61.6141 Q 14.433 -61.2918 10.168 -56.233 Q 7.6987 -53.3054 5.91632 -48.8582 Q 4.84924 -46.1957 3.25024 -40.5993 L 2.508 -38.033 Q -1.55707 -24.3072 -6.5115 -13.2089 Q -12.342 -0.148026 -19.518 9.561 Q -20.1407 10.4034 -20.4166 10.8792 Q -20.8604 11.6445 -20.998 12.337 Q -21.155 13.1277 -20.9302 14.7435 L -20.909 14.896 Q -19.3813 26.0652 -20.2278 34.482 Q -21.5001 47.1335 -28.02 51.749 " }
            }
        }
    }
    LayerItem {
        objectName: "Leg Right"
        id: _qt_layer9
        transform: TransformGroup {
            id: _qt_layer9_transform_base_group
            Matrix4x4 { id: _qt_layer9_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -235.873, 176.173)}
            Matrix4x4 { matrix: _qt_layer11.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node15
            transform: TransformGroup {
                id: _qt_node15_transform_base_group
                Matrix4x4 { id: _qt_node15_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 752.51, 250.507)}
            }
            ShapePath {
                id: _qt_shapePath_14
                strokeColor: "transparent"
                fillColor: "#fffab999"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -13.102 -20.911 Q -13.3002 -19.945 -2.79487 -20.1024 Q 6.5591 -20.2425 9.151 -20.622 Q 15.0695 -21.4911 16.5693 -17.1478 Q 17.6433 -14.0371 16.447 -7.768 L 14.4476 2.28662 Q 13.2511 7.85251 11.987 12.231 Q 11.4755 14.0034 11.0428 15.0524 Q 10.4187 16.565 9.593 17.615 Q 7.38729 20.4194 3.03338 21.2733 Q -0.120416 21.8917 -4.682 21.492 L -5.70731 21.4237 L -13.0045 20.8647 Q -17.9026 20.3758 -17.845 19.883 L -15.4309 -0.880154 L -13.102 -20.911 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node16
            transform: TransformGroup {
                id: _qt_node16_transform_base_group
                Matrix4x4 { id: _qt_node16_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 372.487, 148.68)}
            }
            ShapePath {
                id: _qt_shapePath_15
                strokeColor: "transparent"
                fillColor: "#ff5399bd"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 364.402 124.56 Q 367.044 112.773 369.129 97.9672 Q 371.498 81.1436 369.914 80.736 Q 343.9 74.1275 290.551 69.4082 Q 258.746 66.5947 242.914 64.288 L 191.066 57.7327 L 139.076 52.43 Q 118.175 50.4055 76.5482 44.0927 L 35.751 38.225 Q 18.5102 36.015 -16.5228 34.0426 Q -64.0818 31.3648 -87.108 26.962 Q -140.031 16.8393 -180.775 5.691 Q -199.689 0.516046 -209.732 -10.0924 Q -217.474 -18.269 -226.867 -38.745 L -233.6 -53.738 Q -246.623 -83.0516 -254.6 -96.965 L -256.039 -99.4782 Q -275.499 -133.49 -288.152 -145.366 Q -289.327 -146.467 -290.152 -147.033 Q -291.333 -147.845 -292.471 -148.104 Q -294.066 -148.465 -296.849 -147.783 Q -322.38 -141.504 -343.036 -114.8 Q -361.318 -91.1665 -368.189 -62.818 Q -371.251 -50.181 -371.634 -37.318 Q -372.109 -21.4123 -366.321 10.7039 L -364.098 23.401 Q -357.855 61.2929 -338.332 87.042 Q -327.522 101.301 -310.447 112.42 Q -296.788 121.314 -276.642 129.619 Q -257.484 137.516 -234.048 141.102 Q -214.524 144.09 -189.955 144.327 L -148.216 145.006 Q -49.2078 146.946 -0.023 141.679 Q 6.46893 140.984 19.266 138.703 Q 45.9611 133.944 59.852 135.712 Q 65.0907 136.379 75.3962 138.72 L 84.566 140.695 Q 111.246 145.812 167.354 145.06 L 168.924 145.039 Q 203.688 144.575 272.731 137.608 Q 332.517 131.575 362.616 130.51 Q 362.845 130.502 363.322 128.89 Q 363.774 127.363 364.402 124.56 " }
            }
        }
    }
    LayerItem {
        objectName: "Body"
        id: _qt_layer10
        transform: TransformGroup {
            id: _qt_layer10_transform_base_group
            TransformGroup {
                id: _qt_layer10_transform_group_0
                Translate { x: -85.001; y: -367.853 }
                Rotation { id: _qt_layer10_transform_0_1 }
                Translate { x: -155.266; y: 333.193 }
            }
            Matrix4x4 { matrix: _qt_layer11.transformMatrix }
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
                                script:_qt_layer10_transform_0_1.angle = 0
                            }
                        }
                        PauseAnimation { duration: 1167 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer10_transform_0_1
                                property: "angle"
                                to: 7
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 667 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer10_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 866 }
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
            id: _qt_node17
            transform: TransformGroup {
                id: _qt_node17_transform_base_group
                Matrix4x4 { id: _qt_node17_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 109.689, 215.806)}
            }
            ShapePath {
                id: _qt_shapePath_16
                strokeColor: "transparent"
                fillColor: "#ff00b3df"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -29.713 211.669 Q 3.48326 205.202 34.6 191.272 Q 45.5408 186.372 63.8237 176.107 Q 89.033 161.953 86.83 158.619 Q 76.339 142.743 79.821 98.3405 L 80.645 86.55 L 81.0795 76.5902 Q 82.4569 42.1523 86.993 27.482 L 90.6798 15.9352 Q 95.8791 -0.0871806 97.9114 -8.20183 Q 101.269 -21.609 101.986 -33.72 Q 103.723 -63.1045 98.22 -110.088 L 96.8577 -120.478 Q 94.2264 -139.545 94.3405 -149.003 Q 94.5314 -164.837 100.469 -176.584 L 104.071 -183.239 Q 107.141 -188.725 108.493 -191.562 Q 108.948 -192.517 109.138 -193.114 Q 109.418 -193.997 109.376 -194.745 Q 109.278 -196.498 107.555 -197.905 Q 106.321 -198.913 104.04 -199.865 L 96.4044 -202.88 Q 86.6301 -206.633 82.054 -208.959 Q 81.0042 -209.493 78.8178 -210.878 Q 75.4734 -212.997 73.7179 -213.682 Q 70.7648 -214.832 68.3 -214.117 Q 62.0529 -212.304 50.8452 -194.701 Q 46.2286 -187.45 44.256 -185.147 Q 30.4747 -169.057 14.147 -155.685 Q -80.0361 -78.554 -97.213 17.642 Q -102.486 47.1681 -104.801 64.6176 Q -108.249 90.6054 -108.601 111.96 Q -109.142 144.652 -105.704 166.512 Q -99.5766 205.474 -81.834 209.609 Q -54.5966 215.958 -36.983 212.989 L -29.713 211.669 " }
            }
        }
    }
    LayerItem {
        objectName: "Body Null"
        id: _qt_layer11
        transform: TransformGroup {
            id: _qt_layer11_transform_base_group
            Matrix4x4 { id: _qt_layer11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 750, 750)}
        }
    }
    LayerItem {
        objectName: "Hand"
        id: _qt_layer12
        transform: TransformGroup {
            id: _qt_layer12_transform_base_group
            Matrix4x4 { id: _qt_layer12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 25.644, 68.363)}
            Matrix4x4 { matrix: _qt_layer13.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node18
            transform: TransformGroup {
                id: _qt_node18_transform_base_group
                Matrix4x4 { id: _qt_node18_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 107.908, 89.231)}
            }
            ShapePath {
                id: _qt_shapePath_17
                strokeColor: "transparent"
                fillColor: "#fffab999"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 55.33 49.415 Q 72.7064 28.6824 87.558 -4.466 L 98.2126 -30.2184 Q 107.534 -53.0763 107.281 -53.186 L 104.764 -54.1918 Q 101.803 -55.3286 100.464 -56.0579 Q 98.2372 -57.2706 96.974 -58.904 Q 95.9906 -60.1765 94.6329 -63.1019 Q 93.128 -66.3448 91.9865 -67.5795 Q 90.0953 -69.6252 86.958 -69.841 Q 83.584 -70.0736 80.2765 -67.398 Q 77.9112 -65.4847 75.164 -61.634 L 64.527 -46.2394 Q 50.2401 -25.2221 42.13 -15.43 L 36.2691 -8.67929 Q 28.8519 -0.337773 25.654 4.352 Q 21.7505 10.0784 17.0124 15.0673 Q 11.983 20.3628 6.672 24.119 Q 3.04771 26.6841 0.594 25.992 Q -0.334458 25.7298 -1.264 24.974 Q -1.88816 24.4665 -2.81 23.452 Q -19.0866 5.56177 -39.8193 -38.5009 L -51.107 -61.745 Q -57.8202 -74.7416 -64.044 -80.46 Q -73.9815 -89.5885 -84.914 -87.649 Q -92.9658 -86.2215 -98.5031 -78.5594 Q -103.594 -71.5153 -104.503 -62.434 Q -106.62 -41.3137 -95.1866 -15.5253 Q -87.1908 2.50898 -70.568 26.279 Q -57.6892 44.6959 -48.1715 55.3618 Q -34.502 70.6802 -20.492 79.19 Q -12.4826 84.0541 -4.88763 85.9319 Q 4.0051 88.1305 11.958 86.127 Q 15.935 85.1253 19.6226 82.84 Q 21.8813 81.4403 26.1873 77.9986 L 27.295 77.115 Q 34.7733 71.1761 41.9912 63.9951 Q 48.7295 57.2913 55.33 49.415 " }
            }
        }
    }
    LayerItem {
        objectName: "Sholder"
        id: _qt_layer13
        transform: TransformGroup {
            id: _qt_layer13_transform_base_group
            TransformGroup {
                id: _qt_layer13_transform_group_0
                Translate { x: -37.987; y: -48.781 }
                Rotation { id: _qt_layer13_transform_0_1 }
                Translate { x: 140.726; y: 144.641 }
            }
            Matrix4x4 { matrix: _qt_layer10.transformMatrix }
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
                                script:_qt_layer13_transform_0_1.angle = 0
                            }
                        }
                        PauseAnimation { duration: 1167 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer13_transform_0_1
                                property: "angle"
                                to: -18
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 667 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer13_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 866 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer13_transform_base_group.deactivateOverride(_qt_layer13_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node19
            transform: TransformGroup {
                id: _qt_node19_transform_base_group
                Matrix4x4 { id: _qt_node19_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 53.078, 73.933)}
            }
            ShapePath {
                id: _qt_shapePath_18
                strokeColor: "transparent"
                fillColor: "#ff1399b9"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -22.906 69.8 Q -17.4352 71.9189 -6.18033 68.0417 Q 0.165075 65.8558 11.4589 60.3325 L 16.22 58.06 L 30.2528 51.4234 Q 42.8341 45.2892 49.282 42.651 Q 50.3004 42.2356 50.8219 41.916 Q 51.6274 41.4222 52.019 40.779 Q 52.5633 39.8827 52.4142 38.5728 Q 52.3118 37.6724 51.806 36.266 L 45.4245 18.3893 L 32.571 -17.234 Q 28.7988 -27.399 26.5802 -32.4325 Q 22.9216 -40.7335 18.863 -46.743 Q 8.30575 -62.3699 -6.382 -66.284 Q -26.5239 -71.651 -39.7789 -59.5981 Q -52.3387 -48.1774 -52.619 -27.458 Q -52.8879 -7.58894 -44.957 18.586 L -42.2341 28.2593 Q -34.022 58.4386 -25.915 67.732 Q -24.7251 69.095 -22.906 69.8 " }
            }
        }
    }
    LayerItem {
        objectName: "Phone"
        id: _qt_layer14
        transform: TransformGroup {
            id: _qt_layer14_transform_base_group
            Matrix4x4 { id: _qt_layer14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 16.605, -66.4)}
            Matrix4x4 { matrix: _qt_layer15.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node20
            transform: TransformGroup {
                id: _qt_node20_transform_base_group
                Matrix4x4 { id: _qt_node20_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 43.402, 56.916)}
            }
            ShapePath {
                id: _qt_shapePath_19
                strokeColor: "transparent"
                fillColor: "#ff2c3149"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -43.152 49.229 L -37.965 52.9475 L -32.778 56.666 L 5.187 3.719 L 43.152 -49.228 L 32.779 -56.666 L -43.152 49.229 " }
            }
        }
    }
    LayerItem {
        objectName: "Finger"
        id: _qt_layer15
        transform: TransformGroup {
            id: _qt_layer15_transform_base_group
            Matrix4x4 { id: _qt_layer15_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 180.162, -38.203)}
            Matrix4x4 { matrix: _qt_layer12.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node21
            transform: TransformGroup {
                id: _qt_node21_transform_base_group
                Matrix4x4 { id: _qt_node21_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 39.979, 47.93)}
            }
            ShapePath {
                id: _qt_shapePath_20
                strokeColor: "transparent"
                fillColor: "#fffab999"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 25.959 -47.024 L 25.8477 -47.0036 L 25.725 -46.981 Q 13.0301 -44.5827 1.515 -38.651 L -2.35 -36.7268 Q -12.7266 -31.6479 -16.777 -27.686 Q -23.5057 -21.1073 -28.727 -7.549 Q -30.7929 -2.18169 -32.5175 8.31229 Q -33.3608 13.4432 -33.928 16.116 Q -35.6295 24.1368 -38.788 31.534 L -38.82 31.6088 Q -39.2682 32.658 -39.4282 33.1986 Q -39.6961 34.1046 -39.637 34.852 Q -39.5195 36.3365 -38.1686 37.6469 Q -37.238 38.5496 -35.39 39.613 Q -33.9779 40.424 -31.2301 42.2874 Q -24.2821 46.9989 -20.064 47.527 Q -18.4199 47.7325 -17.426 47.389 Q -15.0702 46.5706 -13.4425 44.238 Q -12.4732 42.8487 -11.1015 39.6309 Q -10.2168 37.5554 -9.714 36.631 Q -7.39741 32.3726 -5.20225 27.3725 Q -3.4328 23.3421 -1.316 17.823 Q -0.710976 16.2444 -0.29675 15.3726 Q 0.33194 14.0495 1.051 13.106 Q 4.75555 8.24171 15.4587 4.44241 Q 25.1597 0.998854 28.83 -2.925 Q 30.7206 -4.94631 31.4315 -7.47862 Q 32.2011 -10.22 31.249 -12.543 Q 31.1035 -12.898 30.752 -13.5878 Q 30.0708 -14.9245 29.954 -15.661 Q 29.8453 -16.3522 30.0414 -17.7455 Q 30.1884 -18.7904 30.176 -19.314 Q 30.14 -20.8506 29.1259 -22.1371 Q 28.1119 -23.4235 26.626 -23.818 Q 27.755 -25.322 28.0335 -27.1567 Q 28.3262 -29.085 27.551 -30.674 Q 26.7375 -32.3415 24.9242 -33.0913 Q 23.0669 -33.8592 21.45 -33.095 Q 23.5697 -34.8813 26.2122 -36.045 Q 28.8547 -37.2088 32.02 -37.75 Q 34.7772 -38.2199 36.5653 -39.5894 Q 38.9748 -41.4349 37.509 -43.909 Q 37.3505 -44.1762 37.1124 -44.4398 Q 36.9121 -44.6615 36.634 -44.903 Q 32.8101 -48.2364 25.959 -47.024 " }
            }
        }
    }
    LayerItem {
        objectName: "Neck"
        id: _qt_layer16
        transform: TransformGroup {
            id: _qt_layer16_transform_base_group
            Matrix4x4 { id: _qt_layer16_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 134.433, 1.179)}
            Matrix4x4 { matrix: _qt_layer10.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node22
            transform: TransformGroup {
                id: _qt_node22_transform_base_group
                Matrix4x4 { id: _qt_node22_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 42.472, 47.268)}
            }
            ShapePath {
                id: _qt_shapePath_21
                strokeColor: "transparent"
                fillColor: "#ffdba288"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 41.277 -24.204 Q 41.7318 -25.1582 41.9212 -25.7554 Q 42.2014 -26.6385 42.159 -27.387 Q 42.0614 -29.1399 40.3399 -30.5466 Q 39.1054 -31.5554 36.825 -32.507 L 29.1898 -35.5217 Q 19.4153 -39.2739 14.839 -41.601 Q 13.7948 -42.1319 11.6193 -43.5049 Q 6.09068 -46.9943 3.118 -47.017 L 0.771775 -46.4943 L -0.066 -46.298 Q -5.90605 -43.7425 -16.3824 -27.314 Q -20.9908 -20.0875 -22.96 -17.788 Q -31.6997 -7.58392 -42.222 2.154 Q -36.4646 14.9695 -20.819 27.038 Q 1.20244 44.0239 29.661 47.017 L 29.6404 46.8675 Q 27.0103 27.8037 27.1246 18.3483 Q 27.316 2.51888 33.252 -9.225 L 36.857 -15.8852 Q 39.9257 -21.369 41.277 -24.204 " }
            }
        }
    }
    LayerItem {
        objectName: "Head"
        id: _qt_layer17
        transform: TransformGroup {
            id: _qt_layer17_transform_base_group
            Matrix4x4 { id: _qt_layer17_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, -2.712, -148.464)}
            Matrix4x4 { matrix: _qt_layer16.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node23
            transform: TransformGroup {
                id: _qt_node23_transform_base_group
                Matrix4x4 { id: _qt_node23_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 77.507, 92.863)}
            }
            ShapePath {
                id: _qt_shapePath_22
                strokeColor: "transparent"
                fillColor: "#ff212740"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -76.777 -0.572 Q -77.1668 -33.4786 -61.841 -56.4591 Q -44.7919 -82.0237 -14.856 -83.175 Q 11.2961 -84.1801 30.509 -74.877 Q 35.8992 -72.2669 44.7592 -64.3839 Q 50.5021 -59.2744 53.643 -57.072 L 57.3238 -54.5848 Q 63.2327 -50.668 65.7263 -48.2737 Q 69.8261 -44.3372 71.797 -39.06 Q 77.3297 -24.2453 66.486 9.085 Q 56.8351 38.745 38.4009 58.4984 Q 16.8253 81.6179 -10.001 82.441 Q -40.7936 83.3853 -59.5013 57.2365 Q -76.3709 33.6568 -76.777 -0.572 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node24
            transform: TransformGroup {
                id: _qt_node24_transform_base_group
                Matrix4x4 { id: _qt_node24_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 85.032, 102.319)}
            }
            ShapePath {
                id: _qt_shapePath_23
                strokeColor: "transparent"
                fillColor: "#fffab999"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 61.206 23.251 Q 52.1955 54.3618 34.909 72.248 Q 23.0412 84.5282 7.64425 90.4556 Q -8.71616 96.754 -24.368 93.901 Q -40.5048 90.96 -52.3316 78.9601 Q -63.8215 67.3021 -67.54 50.997 Q -74.9071 18.6881 -62.9734 -15.1851 Q -50.1263 -51.6511 -21.89 -66.014 Q 22.2993 -88.4891 48.9567 -53.4811 Q 73.9306 -20.6841 61.206 23.251 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node25
            transform: TransformGroup {
                id: _qt_node25_transform_base_group
                Matrix4x4 { id: _qt_node25_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 100.38, 72.617)}
            }
            ShapePath {
                id: _qt_shapePath_24
                strokeColor: "transparent"
                fillColor: "#ff212740"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -21.25 -68.792 L -20.8624 -68.6913 L -20.72 -68.654 Q -8.76098 -65.5458 3.401 -60.357 L 5.64986 -59.3861 Q 11.9784 -56.6366 15.1772 -55.7895 Q 20.4024 -54.4057 25.579 -55.069 Q 26.5624 -55.1949 28.5197 -55.5392 Q 32.1705 -56.1815 34.021 -56.201 Q 36.2993 -56.2246 38.8859 -55.6283 Q 40.9819 -55.145 43.597 -54.169 Q 54.0928 -50.2533 62.309 -41.527 Q 64.8653 -38.812 68.3015 -37.3943 Q 70.3979 -36.5294 74.9398 -35.5242 Q 78.4066 -34.7569 80.256 -34.194 Q 86.882 -32.1771 91.809 -25.193 Q 97.7213 -16.8056 97.6982 -5.93125 Q 97.6752 4.94336 91.727 13.305 Q 85.7788 21.6667 75.5129 25.2547 Q 65.2471 28.8428 55.386 26.007 Q 52.1161 25.0674 49.4656 23.1245 Q 47.819 21.9175 44.7496 18.8695 L 43.897 18.025 Q 41.9441 16.1007 39.1115 15.4504 Q 37.388 15.0548 33.6067 14.9535 Q 30.9559 14.8825 29.574 14.739 Q 24.3097 14.1925 14.3942 10.0184 Q 3.10905 5.26774 -2.943 5.199 Q -10.5871 5.11262 -18.3451 10.3395 Q -23.9153 14.0924 -30.574 21.483 L -32.9159 24.0912 Q -40.7432 32.8234 -44.8967 36.8761 Q -51.7851 43.5974 -58.529 48.018 L -62.6771 50.6415 Q -68.9498 54.5379 -71.817 56.857 Q -76.2198 60.4195 -79.0469 63.7686 Q -82.8982 68.331 -83.732 72.367 Q -87.504 67.5022 -89.8071 60.9001 Q -91.1985 56.9116 -93.0655 48.5286 L -94.27 43.275 L -94.5492 42.1223 Q -99.6913 20.906 -99.673 9.961 Q -99.6296 -14.9786 -88.312 -36.485 Q -82.1443 -48.2036 -72.0035 -57.3138 Q -60.8693 -67.3164 -49.065 -70.167 Q -42.6955 -71.705 -35.1504 -71.1741 Q -28.861 -70.7316 -21.25 -68.792 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node26
            transform: TransformGroup {
                id: _qt_node26_transform_base_group
                Matrix4x4 { id: _qt_node26_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 35.885, 109.581)}
            }
            ShapePath {
                id: _qt_shapePath_25
                strokeColor: "transparent"
                fillColor: "#fffab999"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 13.986 -0.092 Q 14.0498 9.76352 9.99975 16.7584 Q 5.94944 23.7538 0.156 23.792 Q -5.63625 23.8302 -9.7785 16.8881 Q -13.9207 9.946 -13.986 0.091 Q -14.0498 -9.76436 -9.99925 -16.7594 Q -5.94854 -23.7548 -0.155 -23.793 Q 5.63743 -23.8313 9.77938 -16.8895 Q 13.9215 -9.94745 13.986 -0.092 " }
            }
        }
    }
    LayerItem {
        objectName: "Like"
        id: _qt_layer18
        transform: TransformGroup {
            id: _qt_layer18_transform_base_group
            TransformGroup {
                id: _qt_layer18_transform_group_0
                Translate { x: -65.376; y: -200.04 }
                Scale { id: _qt_layer18_transform_0_2}
                Rotation { id: _qt_layer18_transform_0_1 }
                Translate { x: 942.858; y: 688.187 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer18_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer18_transform_animation
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
                                script:_qt_layer18_transform_0_1.angle = 49
                            }
                        }
                        PauseAnimation { duration: 667 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer18_transform_0_1
                                property: "angle"
                                to: 0
                            }
                        }
                        PauseAnimation { duration: 3200 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer18_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer18_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 667 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer18_transform_0_2
                                property: "xScale"
                                to: 1
                            }
                            PropertyAnimation {
                                duration: 166
                                target: _qt_layer18_transform_0_2
                                property: "yScale"
                                to: 1
                            }
                        }
                        PauseAnimation { duration: 3200 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer18_transform_base_group.deactivateOverride(_qt_layer18_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node27
            transform: TransformGroup {
                id: _qt_node27_transform_base_group
                Matrix4x4 { id: _qt_node27_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 140.257, 99.908)}
            }
            ShapePath {
                id: _qt_shapePath_26
                strokeColor: "transparent"
                fillColor: "#ffdf3a34"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.655 -99.219 Q 51.5671 -98.1675 81.077 -89.937 Q 117.307 -79.8306 126.244 -45.309 Q 137.089 -3.41727 117.741 29.1091 Q 97.1904 63.6573 54.471 63.769 L 26.7465 63.6997 L -28.684 63.987 Q -43.8582 64.3779 -56.3515 75.4036 Q -64.7572 82.8219 -75.856 99.658 Q -74.1434 86.9405 -75.1917 79.0117 Q -76.688 67.6954 -83.748 62.072 Q -87.1118 59.3924 -95.2745 56.4709 L -99.496 54.898 Q -122.834 45.5014 -131.517 16.9094 Q -139.003 -7.73975 -133.252 -34.502 Q -126.185 -67.3848 -97.4593 -83.6121 Q -73.6574 -97.058 -36.078 -98.559 Q -6.55788 -99.738 12.655 -99.219 " }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node28
            transform: TransformGroup {
                id: _qt_node28_transform_base_group
                Matrix4x4 { id: _qt_node28_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 136.073, 74.435)}
            }
            ShapePath {
                id: _qt_shapePath_27
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -43.003 47.301 Q -43.8406 46.1233 -44.542 44.5851 Q -45.1291 43.2976 -45.667 41.648 Q -46.8178 38.1264 -47.399 34.474 L -48.7106 27.5037 Q -51.25 14.9131 -50.9091 8.868 Q -50.3323 -1.35827 -43.219 -7.153 Q -34.4106 -14.3283 -22.636 -16.863 Q -18.4267 -17.8422 -9.79261 -17.7402 L -7.714 -17.72 Q -7.40327 -17.719 -7.2375 -17.7464 Q -6.97705 -17.7894 -6.812 -17.919 Q -6.56612 -18.1098 -6.49662 -18.4967 Q -6.44844 -18.765 -6.482 -19.208 Q -7.3889 -31.6301 -5.321 -39.221 Q -3.60392 -45.5296 0.2465 -50.4316 Q 5.25418 -56.807 10.665 -55.169 Q 13.684 -54.2544 15.6361 -51.1478 Q 17.3855 -48.3638 17.761 -44.776 Q 18.0356 -42.1511 17.5888 -36.881 Q 17.2821 -33.2629 17.289 -31.453 Q 17.3141 -25.0461 20.2336 -18.9387 Q 23.0609 -13.0246 27.982 -8.643 Q 30.2485 -6.62591 33.109 -5.02313 Q 35.4561 -3.70797 38.791 -2.367 Q 40.2842 -1.76602 43.6483 -1.10386 Q 48.1222 -0.223257 49.905 0.852 Q 51.8099 2.00111 52.374 5.04836 Q 52.7031 6.82662 52.6272 10.6226 Q 52.5928 12.3452 52.623 13.07 L 53.273 27.232 Q 53.3126 27.986 53.6916 30.6481 Q 55.392 42.5919 53.267 44.295 Q 52.819 44.6534 52.1234 44.8381 Q 51.6806 44.9557 50.844 45.055 L 24.177 48.7779 L -2.364 53.253 Q -29.7443 58.1054 -40.759 49.662 Q -41.397 49.1737 -41.9714 48.564 Q -42.5131 47.989 -43.003 47.301 " }
            }
        }
    }
}
