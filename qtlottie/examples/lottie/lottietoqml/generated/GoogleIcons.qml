// Copyright (C) 2025 Sarpreet Kalyan
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file GoogleIcons.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 500
    implicitHeight: 500
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 500; yScale: height / 500 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "Forms Outlines - Group 10"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            TransformGroup {
                id: _qt_layer0_transform_group_0
                Translate { x: -50.899; y: -69.348 }
                Scale { xScale: 5; yScale: 5 }
                Rotation { id: _qt_layer0_transform_0_1 }
                Translate { id: _qt_layer0_transform_0_0 }
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
                                script:_qt_layer0_transform_0_0.x = 254.497
                            }
                            ScriptAction {
                                script:_qt_layer0_transform_0_0.y = 346.738
                            }
                        }
                        PauseAnimation { duration: 400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 120
                                target: _qt_layer0_transform_0_0
                                property: "x"
                                to: 254.497
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 120
                                target: _qt_layer0_transform_0_0
                                property: "y"
                                to: 331.738
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 140
                                target: _qt_layer0_transform_0_0
                                property: "x"
                                to: 254.497
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 140
                                target: _qt_layer0_transform_0_0
                                property: "y"
                                to: 346.738
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer0_transform_0_0
                                property: "x"
                                to: 254.497
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.181, 0, 0.58, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer0_transform_0_0
                                property: "y"
                                to: 339.738
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.181, 0, 0.58, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer0_transform_0_0
                                property: "x"
                                to: 254.497
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.346, 0, 0.702, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 100
                                target: _qt_layer0_transform_0_0
                                property: "y"
                                to: 346.738
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.346, 0, 0.702, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 340 }
                    }
                    SequentialAnimation {
                        ParallelAnimation {
                            ScriptAction {
                                script:_qt_layer0_transform_0_1.angle = 0
                            }
                        }
                        PauseAnimation { duration: 400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 120
                                target: _qt_layer0_transform_0_1
                                property: "angle"
                                to: -2
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 160
                                target: _qt_layer0_transform_0_1
                                property: "angle"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.333, 0, 0.667, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 520 }
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
                Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 50.899, 47.696)}
            }
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "transparent"
                fillColor: "#ff7248b9"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 4.626 -21.652 L -13.135 -21.652 Q -14.1775 -21.652 -14.9148 -20.9147 Q -15.652 -20.1775 -15.652 -19.135 L -15.652 19.137 Q -15.652 20.1787 -14.9148 20.9154 Q -14.1775 21.652 -13.135 21.652 L 13.137 21.652 Q 14.1788 21.652 14.9154 20.9154 Q 15.652 20.1787 15.652 19.137 L 15.652 -10.625 L 4.626 -21.652 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 9"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node2
            transform: TransformGroup {
                id: _qt_node2_transform_base_group
                Matrix4x4 { id: _qt_node2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 61.038, 31.557)}
            }
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "transparent"
                fillColor: "#ff56368a"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -5.513 5.514 L 5.513 5.514 L -5.513 -5.514 L -5.513 5.514 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 8"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            Matrix4x4 { matrix: _qt_layer3.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "#ffffffff"
                strokeWidth: 2.5
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 47.337 43.978 L 59.729 43.978 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 7"
        id: _qt_layer3
        transform: TransformGroup {
            id: _qt_layer3_transform_base_group
            TransformGroup {
                id: _qt_layer3_transform_group_0
                Translate { x: -43.522; y: -43.978 }
                Translate { id: _qt_layer3_transform_0_0 }
            }
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
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
                                script:_qt_layer3_transform_0_0.x = 43.522
                            }
                            ScriptAction {
                                script:_qt_layer3_transform_0_0.y = 43.978
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer3_transform_0_0
                                property: "x"
                                to: 43.522
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer3_transform_0_0
                                property: "y"
                                to: 38.978
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
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
            id: _qt_node4
            transform: TransformGroup {
                id: _qt_node4_transform_base_group
                Matrix4x4 { id: _qt_node4_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 43.522, 43.978)}
            }
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 1.5 0 Q 1.5 -0.621 1.0605 -1.0605 Q 0.621 -1.5 0 -1.5 Q -0.621577 -1.5 -1.06088 -1.0605 Q -1.5 -0.621173 -1.5 0 Q -1.5 0.62175 -1.06088 1.06088 Q -0.62175 1.5 0 1.5 Q 0.621173 1.5 1.0605 1.06088 Q 1.5 0.621577 1.5 0 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 6"
        id: _qt_layer4
        transform: TransformGroup {
            id: _qt_layer4_transform_base_group
            Matrix4x4 { matrix: _qt_layer5.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node5
            ShapePath {
                id: _qt_shapePath_4
                strokeColor: "#ffffffff"
                strokeWidth: 2.5
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 47.337 50 L 59.729 50 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 5"
        id: _qt_layer5
        transform: TransformGroup {
            id: _qt_layer5_transform_base_group
            TransformGroup {
                id: _qt_layer5_transform_group_0
                Translate { x: -43.522; y: -50 }
                Translate { id: _qt_layer5_transform_0_0 }
            }
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
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
                                script:_qt_layer5_transform_0_0.x = 43.522
                            }
                            ScriptAction {
                                script:_qt_layer5_transform_0_0.y = 50
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer5_transform_0_0
                                property: "x"
                                to: 43.522
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer5_transform_0_0
                                property: "y"
                                to: 43.978
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer5_transform_base_group.deactivateOverride(_qt_layer5_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node6
            transform: TransformGroup {
                id: _qt_node6_transform_base_group
                Matrix4x4 { id: _qt_node6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 43.522, 50)}
            }
            ShapePath {
                id: _qt_shapePath_5
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 1.5 0 Q 1.5 -0.621577 1.0605 -1.06088 Q 0.621173 -1.5 0 -1.5 Q -0.62175 -1.5 -1.06088 -1.06088 Q -1.5 -0.62175 -1.5 0 Q -1.5 0.62175 -1.06088 1.06088 Q -0.62175 1.5 0 1.5 Q 0.621173 1.5 1.0605 1.06088 Q 1.5 0.621577 1.5 0 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 4"
        id: _qt_layer6
        transform: TransformGroup {
            id: _qt_layer6_transform_base_group
            Matrix4x4 { matrix: _qt_layer7.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node7
            ShapePath {
                id: _qt_shapePath_6
                strokeColor: "#ffffffff"
                strokeWidth: 2.5
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 47.337 55.718 L 59.729 55.718 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 3"
        id: _qt_layer7
        transform: TransformGroup {
            id: _qt_layer7_transform_base_group
            TransformGroup {
                id: _qt_layer7_transform_group_0
                Translate { x: -43.522; y: -55.718 }
                Translate { id: _qt_layer7_transform_0_0 }
            }
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer7_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer7_transform_animation
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
                                script:_qt_layer7_transform_0_0.x = 43.522
                            }
                            ScriptAction {
                                script:_qt_layer7_transform_0_0.y = 55.718
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer7_transform_0_0
                                property: "x"
                                to: 43.522
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer7_transform_0_0
                                property: "y"
                                to: 50
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer7_transform_base_group.deactivateOverride(_qt_layer7_transform_group_0)
                    }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node8
            transform: TransformGroup {
                id: _qt_node8_transform_base_group
                Matrix4x4 { id: _qt_node8_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 43.522, 55.718)}
            }
            ShapePath {
                id: _qt_shapePath_7
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 1.5 0 Q 1.5 -0.621577 1.0605 -1.06088 Q 0.621173 -1.5 0 -1.5 Q -0.62175 -1.5 -1.06088 -1.06088 Q -1.5 -0.62175 -1.5 0 Q -1.5 0.621173 -1.06088 1.0605 Q -0.621577 1.5 0 1.5 Q 0.621 1.5 1.0605 1.0605 Q 1.5 0.621 1.5 0 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 11"
        id: _qt_layer8
        transform: TransformGroup {
            id: _qt_layer8_transform_base_group
            Matrix4x4 { matrix: _qt_layer9.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node9
            ShapePath {
                id: _qt_shapePath_8
                strokeColor: "#ffffffff"
                strokeWidth: 2.5
                capStyle: ShapePath.FlatCap
                joinStyle: ShapePath.MiterJoin
                miterLimit: 10
                fillColor: "transparent"
                trim.start: 0
                trim.end: 0
                trim.offset: 0
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 47.337 55.718 L 59.729 55.718 " }
            }
            Connections { target: _qt_node0.animations; function onRestart() {_qt_shapePath_8_trim_end_animation.restart() } }
            ParallelAnimation {
                id: _qt_shapePath_8_trim_end_animation
                loops: _qt_node0.animations.loops
                paused: _qt_node0.animations.paused
                running: true
                onLoopsChanged: { if (running) { restart() } }
                SequentialAnimation {
                    SequentialAnimation {
                        loops: 1
                        ScriptAction {
                            script:_qt_shapePath_8.trim.end = 0
                        }
                        PauseAnimation { duration: 300 }
                        PropertyAnimation {
                            duration: 440
                            target: _qt_shapePath_8.trim
                            property: "end"
                            to: 1
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [ 0.511, 0, 0.259, 1, 1, 1 ]
                        }
                        PauseAnimation { duration: 460 }
                    }
                }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 12"
        id: _qt_layer9
        transform: TransformGroup {
            id: _qt_layer9_transform_base_group
            TransformGroup {
                id: _qt_layer9_transform_group_0
                Translate { x: -43.522; y: -55.718 }
                Translate { id: _qt_layer9_transform_0_0 }
            }
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
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
                                script:_qt_layer9_transform_0_0.x = 43.522
                            }
                            ScriptAction {
                                script:_qt_layer9_transform_0_0.y = 60.718
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_0
                                property: "x"
                                to: 43.522
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 500
                                target: _qt_layer9_transform_0_0
                                property: "y"
                                to: 55.718
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.754, 0, 0.494, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
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
            transform: TransformGroup {
                id: _qt_node10_transform_base_group
                Matrix4x4 { id: _qt_node10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 43.522, 55.718)}
            }
            ShapePath {
                id: _qt_shapePath_9
                strokeColor: "transparent"
                fillColor: "#ffffffff"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 1.5 0 Q 1.5 -0.621577 1.0605 -1.06088 Q 0.621173 -1.5 0 -1.5 Q -0.62175 -1.5 -1.06088 -1.06088 Q -1.5 -0.62175 -1.5 0 Q -1.5 0.621173 -1.06088 1.0605 Q -0.621577 1.5 0 1.5 Q 0.621 1.5 1.0605 1.0605 Q 1.5 0.621 1.5 0 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 2"
        id: _qt_layer10
        transform: TransformGroup {
            id: _qt_layer10_transform_base_group
            Matrix4x4 { id: _qt_layer10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1.07583, 0, -2.99529)}
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node11
            transform: TransformGroup {
                id: _qt_node11_transform_base_group
                Matrix4x4 { id: _qt_node11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 51.478, 39.5)}
            }
            ShapePath {
                id: _qt_shapePath_10
                strokeColor: "transparent"
                fillColor: "#ff7248b9"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 10.891 -1.978 L -10.891 -1.978 L -10.891 1.978 L 10.891 1.978 L 10.891 -1.978 " }
            }
        }
    }
    LayerItem {
        objectName: "Forms Outlines - Group 1"
        id: _qt_layer11
        transform: TransformGroup {
            id: _qt_layer11_transform_base_group
            Matrix4x4 { id: _qt_layer11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 0, 0.001)}
            Matrix4x4 { matrix: _qt_layer0.transformMatrix }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node12
            transform: TransformGroup {
                id: _qt_node12_transform_base_group
                Matrix4x4 { id: _qt_node12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 51.478, 60.826)}
            }
            ShapePath {
                id: _qt_shapePath_11
                strokeColor: "transparent"
                fillColor: "#ff7248b9"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 10.891 -1.979 L -10.891 -1.979 L -10.891 1.979 L 10.891 1.979 L 10.891 -1.979 " }
            }
        }
    }
}
