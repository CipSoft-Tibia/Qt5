// Copyright (C) 2025 Marina Trajkovska
// SPDX-License-Identifier: LicenseRef-Lottie-Simple-License-9.13.21
// Generated from Lottie file USAMapWithOutlines.json
import QtQuick
import QtQuick.VectorImage
import QtQuick.VectorImage.Helpers
import QtQuick.Shapes
import Qt.labs.lottieqt.VectorImageHelpers

Item {
    implicitWidth: 600
    implicitHeight: 345
    component AnimationsInfo : QtObject
    {
        property bool paused: false
        property int loops: 1
        signal restart()
    }
    property AnimationsInfo animations : AnimationsInfo {}
    transform: [
        Scale { xScale: width / 600; yScale: height / 345 }
    ]
    objectName: "_q_animation"
    id: _qt_node0
    LayerItem {
        objectName: "Map_01 Outlines 79"
        id: _qt_layer0
        transform: TransformGroup {
            id: _qt_layer0_transform_base_group
            TransformGroup {
                id: _qt_layer0_transform_group_0
                Translate { x: -279.5; y: -517.438 }
                Scale { id: _qt_layer0_transform_0_2}
                Translate { x: 179.5; y: 289.938 }
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
                        PauseAnimation { duration: 500 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer0_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer0_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 633 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer0_transform_base_group.deactivateOverride(_qt_layer0_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer0_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer0_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer0.opacity = 0
                    }
                    PauseAnimation { duration: 500 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer0
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer0
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
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
                    PauseAnimation { duration: 500 }
                    ScriptAction {
                        script:_qt_layer0.visible = true
                    }
                    PauseAnimation { duration: 5300 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node1
            transform: TransformGroup {
                id: _qt_node1_transform_base_group
                Matrix4x4 { id: _qt_node1_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 280.244, 517.404)}
            }
            ShapePath {
                id: _qt_shapePath_0
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -2.848 -1.132 Q -2.219 -1.4465 -1.39125 -1.52513 Q 0.26425 -1.68237 1.258 -0.503 Q 1.82114 0.165394 1.2868 0.689516 Q 0.795023 1.17188 -0.199125 1.21012 Q -1.25139 1.2506 -1.98298 0.716922 Q -2.848 0.0859153 -2.848 -1.132 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 80"
        id: _qt_layer1
        transform: TransformGroup {
            id: _qt_layer1_transform_base_group
            TransformGroup {
                id: _qt_layer1_transform_group_0
                Translate { x: -297.125; y: -524.625 }
                Scale { id: _qt_layer1_transform_0_2}
                Translate { x: 197.125; y: 297.125 }
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
                        PauseAnimation { duration: 467 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer1_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer1_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer1_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer1_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 667 }
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
                    PauseAnimation { duration: 467 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer1
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer1
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 833 }
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
                    PauseAnimation { duration: 467 }
                    ScriptAction {
                        script:_qt_layer1.visible = true
                    }
                    PauseAnimation { duration: 5333 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node2
            transform: TransformGroup {
                id: _qt_node2_transform_base_group
                Matrix4x4 { id: _qt_node2_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 297.883, 524.615)}
            }
            ShapePath {
                id: _qt_shapePath_1
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -3.746 -2.631 Q -3.01025 -2.8335 -2.19088 -2.93475 Q -0.552125 -3.13725 -0.134 -2.631 Q 0.284125 -2.12413 0.853375 -1.31488 L 1.339 -0.607 L 2.5425 0.068 L 3.746 0.743 Q 2.57525 1.41775 1.23725 1.94062 Q -1.43875 2.98638 -2.275 2.227 Q -3.11125 1.46888 -3.51225 -0.656875 Q -3.71275 -1.71975 -3.746 -2.631 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 81"
        id: _qt_layer2
        transform: TransformGroup {
            id: _qt_layer2_transform_base_group
            TransformGroup {
                id: _qt_layer2_transform_group_0
                Translate { x: -315.062; y: -532.75 }
                Scale { id: _qt_layer2_transform_0_2}
                Translate { x: 215.062; y: 305.25 }
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
                        PauseAnimation { duration: 433 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer2_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer2_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer2_transform_base_group.deactivateOverride(_qt_layer2_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer2_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer2_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer2.opacity = 0
                    }
                    PauseAnimation { duration: 433 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer2
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer2
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 867 }
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
                    PauseAnimation { duration: 433 }
                    ScriptAction {
                        script:_qt_layer2.visible = true
                    }
                    PauseAnimation { duration: 5367 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node3
            transform: TransformGroup {
                id: _qt_node3_transform_base_group
                Matrix4x4 { id: _qt_node3_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 315.728, 532.922)}
            }
            ShapePath {
                id: _qt_shapePath_2
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -3.845 -2.214 Q -3.0355 -2.4485 -2.07425 -2.56575 Q -0.15175 -2.80025 0.607 -2.214 L 2.68162 -0.590875 L 3.845 0.328 Q -1.148 3.153 -1.822 2.348 L -3.08663 0.838125 L -3.845 -0.068 L -3.845 -2.214 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 82"
        id: _qt_layer3
        transform: TransformGroup {
            id: _qt_layer3_transform_base_group
            TransformGroup {
                id: _qt_layer3_transform_group_0
                Translate { x: -324; y: -546 }
                Scale { id: _qt_layer3_transform_0_2}
                Translate { x: 224; y: 318.5 }
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
                        PauseAnimation { duration: 400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer3_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer3_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 733 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer3_transform_base_group.deactivateOverride(_qt_layer3_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer3_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer3_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer3.opacity = 0
                    }
                    PauseAnimation { duration: 400 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer3
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer3
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 900 }
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
                    PauseAnimation { duration: 400 }
                    ScriptAction {
                        script:_qt_layer3.visible = true
                    }
                    PauseAnimation { duration: 5400 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node4
            transform: TransformGroup {
                id: _qt_node4_transform_base_group
                Matrix4x4 { id: _qt_node4_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 325.477, 546.6)}
            }
            ShapePath {
                id: _qt_shapePath_3
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -2.957 -7.418 Q -2.147 -7.45325 -1.19625 -7.25887 Q 0.70525 -6.87012 1.409 -5.722 Q 2.11677 -4.56897 3.23358 -2.30404 L 4.085 -0.636 Q 4.25833 -0.322574 4.8798 0.482536 Q 5.90376 1.80907 6.08963 2.36867 Q 6.40602 3.32123 5.211 3.462 Q 4.22288 3.57839 2.55421 3.03107 Q 1.50805 2.68793 1.07996 2.70611 Q 0.389993 2.73541 0 3.462 Q -0.26192 3.95024 -0.654223 4.95518 Q -1.1565 6.24184 -1.49844 6.63524 Q -2.05311 7.27341 -2.957 6.994 Q -4.10939 6.63903 -4.68685 5.27726 Q -5.02364 4.48302 -5.31188 2.71835 L -5.352 2.474 Q -5.45571 1.84988 -6.1177 1.10991 Q -6.68248 0.478604 -6.81513 0.00764214 Q -7.03582 -0.775846 -6.62 -2.033 Q -6.05885 -3.73068 -4.15763 -5.69325 Q -3.01775 -6.86992 -2.957 -7.418 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 78"
        id: _qt_layer4
        transform: TransformGroup {
            id: _qt_layer4_transform_base_group
            TransformGroup {
                id: _qt_layer4_transform_group_0
                Translate { x: -250.75; y: -545.25 }
                Scale { id: _qt_layer4_transform_0_2}
                Translate { x: 150.75; y: 317.75 }
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
                        PauseAnimation { duration: 367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer4_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer4_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 767 }
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
                    PauseAnimation { duration: 367 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer4
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer4
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 933 }
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
                    PauseAnimation { duration: 367 }
                    ScriptAction {
                        script:_qt_layer4.visible = true
                    }
                    PauseAnimation { duration: 5433 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node5
            transform: TransformGroup {
                id: _qt_node5_transform_base_group
                Matrix4x4 { id: _qt_node5_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 249.117, 544.957)}
            }
            ShapePath {
                id: _qt_shapePath_4
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -11.973 -10.527 Q -11.0822 -9.54425 -10.0667 -8.80713 Q -8.03575 -7.33287 -7.412 -8.561 Q -6.58574 -10.1879 -5.826 -10.8074 Q -4.96114 -11.5126 -4.561 -10.527 Q -4.30419 -9.89444 -1.95986 -7.56522 Q 1.46883 -4.15865 2.565 -2.385 Q 3.84823 -0.309723 4.57691 0.476053 Q 5.13493 1.07779 6.62221 2.13865 L 7.269 2.602 Q 9.25241 4.03369 10.4226 5.922 Q 11.7211 8.01726 11.117 9.404 Q 10.8101 10.1092 10.0093 10.0733 Q 9.52527 10.0515 8.36197 9.63247 Q 6.28477 8.88421 5.559 9.404 L 5.48631 9.4561 Q 3.86985 10.615 3.11827 10.5603 Q 1.84073 10.4672 1.283 7.721 Q 0.993339 6.29438 1.18154 4.4273 Q 1.33137 2.94081 1.00015 2.23048 Q 0.488416 1.13305 -1.568 -0.266 Q -3.8368 -1.80954 -5.12301 -3.9673 Q -5.60679 -4.77891 -5.93251 -5.03366 Q -6.43411 -5.42598 -7.412 -5.474 Q -9.33711 -5.56853 -10.7084 -7.4215 Q -11.8401 -8.95076 -11.973 -10.527 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 83"
        id: _qt_layer5
        transform: TransformGroup {
            id: _qt_layer5_transform_base_group
            TransformGroup {
                id: _qt_layer5_transform_group_0
                Translate { x: -199.625; y: -542 }
                Scale { id: _qt_layer5_transform_0_2}
                Translate { x: 99.625; y: 314.5 }
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
                        PauseAnimation { duration: 333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer5_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer5_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 800 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer5_transform_base_group.deactivateOverride(_qt_layer5_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer5_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer5_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer5.opacity = 0
                    }
                    PauseAnimation { duration: 333 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer5
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer5
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 967 }
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
                    PauseAnimation { duration: 333 }
                    ScriptAction {
                        script:_qt_layer5.visible = true
                    }
                    PauseAnimation { duration: 5467 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node6
            transform: TransformGroup {
                id: _qt_node6_transform_base_group
                Matrix4x4 { id: _qt_node6_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 200.52, 542.445)}
            }
            ShapePath {
                id: _qt_shapePath_5
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -3.155 1.868 Q -3.22225 0.76425 -3.0545 -0.35625 Q -2.719 -2.59725 -1.544 -2.681 Q -0.0159738 -2.78991 1.37688 -1.84162 Q 1.98198 -1.42966 2.00636 -1.05917 Q 2.034 -0.63915 1.276 -0.406 Q 0.386777 -0.132775 -0.561304 1.00729 Q -1.17409 1.74416 -1.56585 1.93297 Q -2.20408 2.24058 -3.155 1.868 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 77"
        id: _qt_layer6
        transform: TransformGroup {
            id: _qt_layer6_transform_base_group
            TransformGroup {
                id: _qt_layer6_transform_group_0
                Translate { x: -209; y: -506 }
                Scale { id: _qt_layer6_transform_0_2}
                Translate { x: 109; y: 278.5 }
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
                        PauseAnimation { duration: 300 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer6_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer6_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 833 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer6_transform_base_group.deactivateOverride(_qt_layer6_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer6_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer6_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer6.opacity = 0
                    }
                    PauseAnimation { duration: 300 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer6
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer6
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1000 }
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
                    PauseAnimation { duration: 300 }
                    ScriptAction {
                        script:_qt_layer6.visible = true
                    }
                    PauseAnimation { duration: 5500 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node7
            transform: TransformGroup {
                id: _qt_node7_transform_base_group
                Matrix4x4 { id: _qt_node7_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 199.341, 510.689)}
            }
            ShapePath {
                id: _qt_shapePath_6
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 31.786 -25.555 L 31.786 20.348 L 27.9571 20.1073 Q 23.8634 19.8183 22.539 19.577 Q 21.5534 19.3975 20.6066 17.8581 Q 19.3715 15.8502 17.723 15.334 Q 16.4042 14.9211 15.9502 15.1845 Q 15.6553 15.3556 15.4789 16.1704 Q 15.2159 17.3857 14.641 18.033 Q 13.9426 18.8188 10.7618 20.0134 Q 8.79592 20.7517 7.96916 21.1775 Q 6.58505 21.8905 6.164 22.663 Q 5.74364 23.4343 5.51384 22.5241 Q 5.29183 21.6447 5.44175 20.107 Q 5.85088 15.9106 7.898 15.526 Q 8.98836 15.3215 9.34686 14.6825 Q 9.67415 14.0991 9.24088 13.5495 Q 8.7881 12.9752 7.8357 12.9228 Q 6.72224 12.8615 5.387 13.598 Q 2.67285 15.0948 0.046875 18.5659 Q -2.43516 21.8467 -2.12 22.996 Q -1.69098 24.5532 -3.97375 27.8596 Q -6.67168 31.7674 -10.596 32.439 Q -13.1081 32.8697 -18.9618 34.5607 Q -26.6146 36.7715 -28.512 36.549 Q -30.9299 36.264 -21.6074 32.5834 Q -18.8654 31.5008 -18.109 31.147 Q -15.2836 29.8282 -12.6908 27.8209 Q -9.44 25.3043 -9.44 23.627 Q -9.44 22.4371 -10.6964 21.7461 Q -11.4336 21.3407 -13.261 20.8925 Q -14.59 20.5666 -15.027 20.348 Q -16.679 19.5214 -17.6998 18.7331 Q -19.1994 17.5749 -19.072 16.684 Q -19.016 16.2903 -18.2266 15.379 Q -17.2088 14.2039 -17.1992 13.6032 Q -17.183 12.586 -19.265 11.861 Q -22.7324 10.6536 -23.8881 9.34313 Q -25.1631 7.89747 -24.465 5.512 Q -23.7073 2.92089 -21.3594 0.797625 Q -18.8773 -1.447 -16.375 -1.447 Q -10.5473 -1.447 -9.825 -3.374 Q -9.68801 -3.73971 -9.24666 -4.4831 Q -8.56049 -5.63884 -8.62487 -6.09324 Q -8.73087 -6.84143 -10.403 -7.038 L -12.0016 -7.18338 Q -14.9398 -7.41225 -16.2318 -7.90625 Q -18.4023 -8.7361 -19.265 -10.896 Q -20.59 -14.2124 -20.7099 -15.7419 Q -20.8638 -17.7062 -19.265 -17.839 Q -18.5676 -17.8972 -17.2203 -18.213 Q -15.7792 -18.5508 -15.2282 -18.5167 Q -14.3332 -18.4612 -14.062 -17.647 Q -13.9302 -17.2502 -13.7504 -16.4521 Q -13.4624 -15.1734 -13.1561 -14.7212 Q -12.6556 -13.982 -11.559 -13.982 Q -7.17678 -13.982 -7.899 -15.91 Q -8.12866 -16.5231 -8.74617 -18.9272 Q -9.82263 -23.1181 -10.403 -24.397 Q -10.6212 -24.8775 -11.1544 -25.8941 Q -12.0551 -27.6112 -12.2402 -28.2853 Q -12.5475 -29.4047 -11.752 -29.604 Q -11.2529 -29.729 -9.76421 -29.9034 Q -8.11318 -30.0968 -7.41415 -30.2544 Q -6.23128 -30.521 -5.972 -30.953 Q -5.71509 -31.3827 -1.84636 -32.0513 Q 3.59671 -32.992 4.238 -33.848 Q 4.50151 -34.1997 4.92661 -34.8688 Q 5.54707 -35.8453 5.85224 -36.0196 Q 6.328 -36.2914 6.935 -35.582 Q 7.76007 -34.6192 9.4094 -33.6756 Q 10.0711 -33.297 10.402 -33.076 Q 11.6054 -32.2734 17.53 -29.604 L 19.9917 -28.4526 Q 23.5379 -26.7571 24.851 -26.904 Q 25.1381 -26.936 25.8788 -27.0706 Q 28.0977 -27.4739 29.2081 -27.3521 Q 31.1532 -27.1387 31.786 -25.555 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 71"
        id: _qt_layer7
        transform: TransformGroup {
            id: _qt_layer7_transform_base_group
            TransformGroup {
                id: _qt_layer7_transform_group_0
                Translate { x: -626.091; y: -288.039 }
                Scale { id: _qt_layer7_transform_0_2}
                Translate { x: 526.091; y: 60.539 }
            }
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
                                script:_qt_layer7_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer7_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1500 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer7_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer7_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2934 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer7_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer7_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 33 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer7_transform_base_group.deactivateOverride(_qt_layer7_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer7_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer7_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer7.opacity = 0
                    }
                    PauseAnimation { duration: 1500 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer7
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3100 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer7
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 200 }
                }
            }
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
                    PauseAnimation { duration: 1500 }
                    ScriptAction {
                        script:_qt_layer7.visible = true
                    }
                    PauseAnimation { duration: 4300 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node8
            transform: TransformGroup {
                id: _qt_node8_transform_base_group
                Matrix4x4 { id: _qt_node8_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 626.422, 287.2)}
            }
            ShapePath {
                id: _qt_shapePath_7
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.477 3.899 Q 11.3548 4.50814 8.64424 5.35182 Q 6.26677 6.0918 5.35099 6.5691 Q 3.83705 7.35814 3.59 8.463 Q 3.33208 9.61846 2.18052 10.2368 Q 1.42297 10.6435 1.06701 11.0664 Q 0.465536 11.7809 0.152 13.18 Q -0.336239 15.3569 -3.49534 17.2946 Q -5.62049 18.598 -5.869 19.586 Q -6.11206 20.554 -7.30953 22.2689 Q -8.10991 23.4151 -8.38626 24.0047 Q -8.84529 24.9841 -8.735 25.861 Q -9.71617 24.5115 -10.6111 21.8924 Q -11.5825 19.0495 -12.272 15.211 Q -12.8664 11.8848 -14.7381 8.25525 Q -15.674 6.4405 -16.491 5.291 Q -14.2948 2.89444 -13.608 0.573 Q -13.1333 -1.02954 -12.6334 -6.80995 Q -12.0816 -13.191 -11.503 -15.781 Q -9.5933 -24.3363 -7.301 -22.056 Q -6.01043 -20.7713 -5.55675 -20.5574 Q -4.90907 -20.2519 -4.246 -20.915 Q -4.05833 -21.1019 -3.54466 -21.7547 Q -2.26952 -23.3752 -1.54402 -23.8527 Q -0.271631 -24.6903 0.536 -23.385 Q 1.13397 -22.4158 2.37842 -18.3946 Q 4.48971 -11.5724 5.883 -10.647 Q 6.38507 -10.3135 7.54752 -9.65348 Q 9.72325 -8.41807 10.636 -7.62524 Q 12.1806 -6.28361 12.477 -4.658 Q 12.6693 -3.60513 13.0054 -3.28513 Q 13.2114 -3.089 13.695 -3.089 Q 14.2144 -3.089 14.4919 -2.89287 Q 14.9624 -2.56033 15.342 -1.52 Q 16.1562 0.704966 15.5224 1.72425 Q 15.0323 2.51236 12.477 3.899 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 74"
        id: _qt_layer8
        transform: TransformGroup {
            id: _qt_layer8_transform_base_group
            TransformGroup {
                id: _qt_layer8_transform_group_0
                Translate { x: -611.576; y: -312.871 }
                Scale { id: _qt_layer8_transform_0_2}
                Translate { x: 511.576; y: 85.371 }
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
                                script:_qt_layer8_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer8_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1433 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 2966 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer8_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer8_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 67 }
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
                    PauseAnimation { duration: 1433 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer8
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3133 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer8
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 233 }
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
                    PauseAnimation { duration: 1433 }
                    ScriptAction {
                        script:_qt_layer8.visible = true
                    }
                    PauseAnimation { duration: 4367 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node9
            transform: TransformGroup {
                id: _qt_node9_transform_base_group
                Matrix4x4 { id: _qt_node9_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 611.546, 308.07)}
            }
            ShapePath {
                id: _qt_shapePath_8
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.529 9.068 L 6.529 9.074 Q 5.79647 10.7681 2.49688 12.6613 Q -1.07537 14.7108 -3.69 14.396 Q -4.25105 14.3298 -4.9725 14.3641 Q -5.64876 14.3963 -6.45 14.516 L -6.44709 7.99978 L -6.35929 1.11659 L -6.315 -2.475 Q -6.315 -4.08153 -4.32238 -7.1205 Q -3.40048 -8.52651 -3.132 -9.143 Q -2.60073 -10.3675 -3.949 -12.56 L -2.91939 -13.6679 L -1.83 -14.846 Q -1.0275 -13.6967 -0.108125 -11.8824 Q 1.73063 -8.25362 2.315 -4.928 Q 2.99303 -1.08897 3.94725 1.75238 Q 4.82661 4.3708 5.791 5.72 Q 5.93775 6.90511 6.529 9.068 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 72"
        id: _qt_layer9
        transform: TransformGroup {
            id: _qt_layer9_transform_base_group
            TransformGroup {
                id: _qt_layer9_transform_group_0
                Translate { x: -601.23; y: -309.82 }
                Scale { id: _qt_layer9_transform_0_2}
                Translate { x: 501.23; y: 82.32 }
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
                                script:_qt_layer9_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer9_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer9_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer9_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3000 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer9_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer9_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 100 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer9_transform_base_group.deactivateOverride(_qt_layer9_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer9_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer9_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer9.opacity = 0
                    }
                    PauseAnimation { duration: 1367 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer9
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3167 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer9
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 267 }
                }
            }
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
                    PauseAnimation { duration: 1367 }
                    ScriptAction {
                        script:_qt_layer9.visible = true
                    }
                    PauseAnimation { duration: 4433 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node10
            transform: TransformGroup {
                id: _qt_node10_transform_base_group
                Matrix4x4 { id: _qt_node10_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 601.28, 309.82)}
            }
            ShapePath {
                id: _qt_shapePath_9
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.818 -10.848 Q 6.5497 -10.2328 5.62746 -8.82955 Q 3.636 -5.79939 3.636 -4.198 L 3.592 -0.620426 L 3.50466 6.24598 L 3.502 12.747 Q 0.992624 13.1229 -2.51 14.255 Q -2.2453 10.3994 -1.209 7.201 Q -0.69186 5.60031 -1.53326 4.2316 Q -2.038 3.41052 -3.64164 2.02219 Q -5.24982 0.629928 -5.752 -0.193 Q -6.62469 -1.63137 -6.79772 -2.32071 Q -6.92606 -2.83197 -6.81499 -3.92535 L -6.726 -4.938 Q -6.575 -7.213 -7.064 -9.675 L -5.60531 -9.73121 Q -1.98961 -9.84271 -0.114681 -10.2796 Q 3.12396 -11.0342 4.943 -13.089 L 6.002 -14.254 Q 7.34855 -12.0672 6.818 -10.848 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 73"
        id: _qt_layer10
        transform: TransformGroup {
            id: _qt_layer10_transform_base_group
            TransformGroup {
                id: _qt_layer10_transform_group_0
                Translate { x: -611.596; y: -325.709 }
                Scale { id: _qt_layer10_transform_0_2}
                Translate { x: 511.596; y: 98.209 }
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
                                script:_qt_layer10_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer10_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1300 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer10_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer10_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3034 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer10_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer10_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 133 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer10_transform_base_group.deactivateOverride(_qt_layer10_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer10_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer10_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer10.opacity = 0
                    }
                    PauseAnimation { duration: 1300 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer10
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3200 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer10
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 300 }
                }
            }
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
                    PauseAnimation { duration: 1300 }
                    ScriptAction {
                        script:_qt_layer10.visible = true
                    }
                    PauseAnimation { duration: 4500 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node11
            transform: TransformGroup {
                id: _qt_node11_transform_base_group
                Matrix4x4 { id: _qt_node11_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 612.041, 325.709)}
            }
            ShapePath {
                id: _qt_shapePath_10
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.16 4.165 Q 11.6376 5.00051 10.0334 5.5301 Q 8.10124 6.16794 7.581 7.468 Q 6.81825 6.5955 5.936 5.6775 Q 4.1715 3.8415 3.574 3.614 Q 3.17888 3.46222 1.518 3.961 L 0.172575 4.377 Q -3.52033 5.54319 -4.628 5.448 Q -5.94112 5.33425 -9.32788 6.57175 Q -11.0212 7.1905 -12.452 7.832 L -12.7704 6.1245 Q -12.9324 5.21189 -13.025 4.528 Q -13.3865 1.8435 -13.162 -1.12 Q -9.59729 -2.21623 -7.049 -2.579 Q -6.23473 -2.69477 -5.5475 -2.72587 Q -4.8146 -2.75905 -4.244 -2.695 Q -1.58813 -2.39137 2.04137 -4.3695 Q 5.39392 -6.19668 6.139 -7.832 Q 7.05196 -4.6473 8.153 -1.891 Q 8.55199 -0.896022 10.4192 0.846495 Q 11.6522 1.99704 12.013 2.54553 Q 12.6066 3.44777 12.16 4.165 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 76"
        id: _qt_layer11
        transform: TransformGroup {
            id: _qt_layer11_transform_base_group
            TransformGroup {
                id: _qt_layer11_transform_group_0
                Translate { x: -616.625; y: -333.625 }
                Scale { id: _qt_layer11_transform_0_2}
                Translate { x: 516.625; y: 106.125 }
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
                                script:_qt_layer11_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer11_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1267 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer11_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer11_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3033 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer11_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer11_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 167 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer11_transform_base_group.deactivateOverride(_qt_layer11_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer11_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer11_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer11.opacity = 0
                    }
                    PauseAnimation { duration: 1267 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer11
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3200 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer11
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 333 }
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
                    PauseAnimation { duration: 1267 }
                    ScriptAction {
                        script:_qt_layer11.visible = true
                    }
                    PauseAnimation { duration: 4533 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node12
            transform: TransformGroup {
                id: _qt_node12_transform_base_group
                Matrix4x4 { id: _qt_node12_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 616.681, 333.391)}
            }
            ShapePath {
                id: _qt_shapePath_11
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 2.833 0.06 L 2.57797 0.760769 Q 1.96229 2.47458 1.50149 3.00006 Q 0.800802 3.7991 -0.731 3.914 Q -1.39912 1.96337 -2.18287 -0.935875 L -2.833 -3.445 Q -1.28311 -3.94563 -0.911 -3.793 Q -0.352875 -3.56612 1.29587 -1.73038 Q 2.12025 -0.8125 2.833 0.06 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 75"
        id: _qt_layer12
        transform: TransformGroup {
            id: _qt_layer12_transform_base_group
            TransformGroup {
                id: _qt_layer12_transform_group_0
                Translate { x: -607.604; y: -335.062 }
                Scale { id: _qt_layer12_transform_0_2}
                Translate { x: 507.604; y: 107.562 }
            }
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
                                script:_qt_layer12_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer12_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1233 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer12_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer12_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3033 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer12_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer12_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 200 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer12_transform_base_group.deactivateOverride(_qt_layer12_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer12_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer12_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer12.opacity = 0
                    }
                    PauseAnimation { duration: 1233 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer12
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3200 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer12
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 367 }
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
                    PauseAnimation { duration: 1233 }
                    ScriptAction {
                        script:_qt_layer12.visible = true
                    }
                    PauseAnimation { duration: 4567 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node13
            transform: TransformGroup {
                id: _qt_node13_transform_base_group
                Matrix4x4 { id: _qt_node13_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 607.604, 337.196)}
            }
            ShapePath {
                id: _qt_shapePath_12
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 7.972 0.084 Q 6.34695 0.194728 4.81583 0.727916 Q 3.89066 1.05009 2.19799 1.8716 L 1.784 2.072 Q 1.13588 2.38485 -0.834585 3.15954 Q -2.7989 3.93181 -3.62514 4.30635 Q -5.01855 4.93798 -5.344 5.33 Q -5.57142 5.60482 -6.3361 6.24936 Q -7.05404 6.85449 -7.37 7.177 Q -6.50939 4.39073 -7.91679 -3.07143 L -7.971 -3.359 Q -6.5645 -3.99175 -4.9 -4.60213 Q -1.571 -5.82288 -0.281 -5.711 Q 0.807436 -5.61663 4.4357 -6.7662 L 5.76 -7.177 L 6.44413 -4.70113 Q 7.26888 -1.84037 7.972 0.084 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 54"
        id: _qt_layer13
        transform: TransformGroup {
            id: _qt_layer13_transform_base_group
            TransformGroup {
                id: _qt_layer13_transform_group_0
                Translate { x: -589.687; y: -374.194 }
                Scale { id: _qt_layer13_transform_0_2}
                Translate { x: 489.687; y: 146.694 }
            }
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
                                script:_qt_layer13_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer13_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1233 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer13_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer13_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3000 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer13_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer13_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 233 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer13_transform_base_group.deactivateOverride(_qt_layer13_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer13_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer13_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer13.opacity = 0
                    }
                    PauseAnimation { duration: 1233 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer13
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3167 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer13
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 400 }
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
                    PauseAnimation { duration: 1233 }
                    ScriptAction {
                        script:_qt_layer13.visible = true
                    }
                    PauseAnimation { duration: 4567 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node14
            transform: TransformGroup {
                id: _qt_node14_transform_base_group
                Matrix4x4 { id: _qt_node14_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 590.323, 374.302)}
            }
            ShapePath {
                id: _qt_shapePath_13
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 6.084 2.548 L 5.96058 3.31114 Q 5.52695 6.01685 5.15447 6.84556 Q 4.57208 8.1413 3.288 8.024 Q 2.56428 7.95873 0.537407 6.82789 Q -1.12391 5.90101 -1.84724 5.67227 Q -3.05875 5.28914 -3.541 5.951 Q -6.19931 1.29224 -5.659 -0.108 Q -5.38018 -0.8414 -4.88214 -2.91919 Q -4.28466 -5.41188 -3.90801 -6.30508 Q -3.27507 -7.80606 -2.495 -7.453 Q -1.59969 -7.05147 -1.40363 -3.53583 Q -1.23752 -0.557492 -0.627 -0.108 Q 3.785 3.12995 6.084 2.548 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 70"
        id: _qt_layer14
        transform: TransformGroup {
            id: _qt_layer14_transform_base_group
            TransformGroup {
                id: _qt_layer14_transform_group_0
                Translate { x: -584; y: -322.72 }
                Scale { id: _qt_layer14_transform_0_2}
                Translate { x: 484; y: 95.22 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer14_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer14_transform_animation
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
                                script:_qt_layer14_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer14_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer14_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer14_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3000 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer14_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer14_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer14_transform_base_group.deactivateOverride(_qt_layer14_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer14_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer14_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer14.opacity = 0
                    }
                    PauseAnimation { duration: 1200 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer14
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3166 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer14
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
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
                    PauseAnimation { duration: 1200 }
                    ScriptAction {
                        script:_qt_layer14.visible = true
                    }
                    PauseAnimation { duration: 4600 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node15
            transform: TransformGroup {
                id: _qt_node15_transform_base_group
                Matrix4x4 { id: _qt_node15_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 573.893, 323.005)}
            }
            ShapePath {
                id: _qt_shapePath_14
                strokeColor: "transparent"
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 25.903 21.917 L 25.7544 22.079 L 25.614 22.241 Q 25.614 22.2449 25.6136 22.2455 Q 25.6132 22.2461 25.6109 22.246 L 25.609 22.246 Q 24.6335 22.6646 19.1249 19.7935 Q 16.5865 18.4704 15.862 18.228 Q 14.9391 17.9213 14.5648 17.3538 Q 14.3342 17.0043 14.1645 16.1616 Q 13.9091 14.8933 13.364 14.193 Q 12.6171 13.2364 4.29728 15.6251 Q -2.76203 17.652 -4.909 17.652 Q -8.0036 17.652 -13.4807 18.614 L -21.8419 20.3029 L -22.031 20.344 Q -24.7935 20.9459 -26.221 19.9151 Q -26.9347 19.3997 -27.096 18.764 L -26.6084 18.4677 Q -23.4779 16.5686 -22.3576 15.4923 Q -20.5009 13.7085 -20.68 11.698 Q -20.9219 9.05737 -22.5556 6.65513 Q -23.3725 5.454 -24.141 4.781 Q -21.256 4.30125 -17.8906 3.91738 Q -11.1599 3.14963 -8.758 3.629 Q -6.08344 4.16127 -2.88963 1.94675 Q 0.092 -0.120648 0.092 -1.75 Q 0.092 -2.95313 -1.062 -5.93038 Q -1.639 -7.419 -2.216 -8.667 Q -1.35025 -9.6755 -0.34025 -11.0205 Q 1.67975 -13.7105 2.401 -15.393 Q 3.31737 -17.5307 4.78113 -18.7326 Q 6.42738 -20.0844 9.325 -20.771 L 11.8292 -21.3891 Q 17.1894 -22.7362 19.901 -22.885 Q 20.403 -20.391 20.247 -18.085 L 20.1566 -17.0615 Q 20.0434 -15.9518 20.1751 -15.4335 Q 20.3525 -14.7354 21.247 -13.277 Q 21.7616 -12.4429 23.4102 -11.0319 Q 25.0535 -9.62539 25.5708 -8.79353 Q 26.433 -7.40685 25.903 -5.785 Q 24.842 -2.54083 24.57 1.363 Q 24.3437 4.46965 24.709 7.276 Q 24.8012 7.9938 24.9641 8.94788 L 25.286 10.736 L 25.3407 11.0359 Q 26.7854 18.958 25.903 21.917 " }
            }
            ShapePath {
                id: _qt_shapePath_15
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "transparent"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic
                PathSvg { path: "M 25.903 21.917 L 25.7544 22.079 L 25.614 22.241 Q 25.614 22.2449 25.6136 22.2455 Q 25.6132 22.2461 25.6109 22.246 L 25.609 22.246 Q 24.6335 22.6646 19.1249 19.7935 Q 16.5865 18.4704 15.862 18.228 Q 14.9391 17.9213 14.5648 17.3538 Q 14.3342 17.0043 14.1645 16.1616 Q 13.9091 14.8933 13.364 14.193 Q 12.6171 13.2364 4.29728 15.6251 Q -2.76203 17.652 -4.909 17.652 Q -8.0036 17.652 -13.4807 18.614 L -21.8419 20.3029 L -22.031 20.344 Q -24.7935 20.9459 -26.221 19.9151 Q -26.9347 19.3997 -27.096 18.764 L -26.6084 18.4677 Q -23.4779 16.5686 -22.3576 15.4923 Q -20.5009 13.7085 -20.68 11.698 Q -20.9219 9.05737 -22.5556 6.65513 Q -23.3725 5.454 -24.141 4.781 Q -21.256 4.30125 -17.8906 3.91738 Q -11.1599 3.14963 -8.758 3.629 Q -6.08344 4.16127 -2.88963 1.94675 Q 0.092 -0.120648 0.092 -1.75 Q 0.092 -2.95313 -1.062 -5.93038 Q -1.639 -7.419 -2.216 -8.667 Q -1.35025 -9.6755 -0.34025 -11.0205 Q 1.67975 -13.7105 2.401 -15.393 Q 3.31737 -17.5307 4.78113 -18.7326 Q 6.42738 -20.0844 9.325 -20.771 L 11.8292 -21.3891 Q 17.1894 -22.7362 19.901 -22.885 Q 20.403 -20.391 20.247 -18.085 L 20.1566 -17.0615 Q 20.0434 -15.9518 20.1751 -15.4335 Q 20.3525 -14.7354 21.247 -13.277 Q 21.7616 -12.4429 23.4102 -11.0319 Q 25.0535 -9.62539 25.5708 -8.79353 Q 26.433 -7.40685 25.903 -5.785 Q 24.842 -2.54083 24.57 1.363 Q 24.3437 4.46965 24.709 7.276 Q 24.8012 7.9938 24.9641 8.94788 L 25.286 10.736 L 25.3407 11.0359 Q 26.7854 18.958 25.903 21.917 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 60"
        id: _qt_layer15
        transform: TransformGroup {
            id: _qt_layer15_transform_base_group
            TransformGroup {
                id: _qt_layer15_transform_group_0
                Translate { x: -557.083; y: -508.419 }
                Scale { id: _qt_layer15_transform_0_2}
                Translate { x: 457.083; y: 280.919 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer15_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer15_transform_animation
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
                                script:_qt_layer15_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer15_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1000 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer15_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer15_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer15_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer15_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer15_transform_base_group.deactivateOverride(_qt_layer15_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer15_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer15_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer15.opacity = 0
                    }
                    PauseAnimation { duration: 1000 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer15
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3366 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer15
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
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
                    PauseAnimation { duration: 1000 }
                    ScriptAction {
                        script:_qt_layer15.visible = true
                    }
                    PauseAnimation { duration: 4800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node16
            transform: TransformGroup {
                id: _qt_node16_transform_base_group
                Matrix4x4 { id: _qt_node16_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 532.555, 508.666)}
            }
            ShapePath {
                id: _qt_shapePath_16
                strokeColor: "#ff185aba"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 42.172 28.107 Q 41.9307 28.8322 41.144 29.3172 Q 40.6616 29.6147 39.4155 30.0772 Q 37.7655 30.6896 36.96 31.204 Q 35.2954 32.264 33.8966 32.0247 Q 32.4339 31.7746 32.715 30.237 Q 33.0151 28.5901 31.8226 26.49 Q 30.5972 24.332 29.05 24.048 Q 28.1793 23.8915 27.5882 23.1788 Q 27.2257 22.7417 26.6554 21.5804 Q 25.8646 19.9701 25.188 19.218 Q 24.4257 18.3714 22.539 17.7946 Q 20.1537 17.0654 19.397 15.548 Q 18.8993 14.549 16.6723 11.787 Q 12.7557 6.92976 13.607 6.074 Q 13.8742 5.8062 14.6308 5.15819 Q 16.3157 3.71509 16.7025 3.06082 Q 17.3661 1.93822 15.732 1.631 Q 13.2831 1.17242 11.6995 0.205 Q 10.0164 -0.823203 10.712 -1.656 Q 11.7079 -2.85061 12.6667 -5.61812 Q 13.7961 -8.87762 13.027 -10.163 L 12.6418 -10.8152 Q 11.7917 -12.2668 11.3272 -12.7545 Q 10.5948 -13.5235 9.555 -13.642 Q 8.34022 -13.7749 6.12738 -15.2095 Q 4.18281 -16.4702 2.414 -18.082 Q 0.619591 -19.7185 -0.72425 -20.3326 Q -2.44535 -21.1192 -4.731 -20.982 Q -6.19908 -20.896 -8.654 -18.6559 Q -11.8431 -15.7457 -14.187 -15.96 Q -16.2139 -16.1428 -19.0034 -18.763 Q -22.181 -21.7477 -24.802 -21.951 Q -29.192 -22.2895 -31.5812 -22.2895 Q -34.7289 -22.2895 -36.19 -21.758 Q -36.8312 -21.5232 -37.87 -20.942 Q -35.8418 -22.2985 -37.539 -23.69 L -38.5576 -24.4958 Q -40.441 -25.9618 -40.8059 -26.5208 Q -41.4099 -27.4461 -39.664 -27.557 Q -38.5873 -27.6236 -35.8588 -27.2737 Q -30.2102 -26.5491 -27.695 -27.557 Q -25.2856 -28.5214 -20.0424 -29.2901 Q -17.4207 -29.6745 -15.281 -29.866 Q -14.388 -27.2616 -13.221 -27.168 Q -12.2445 -27.0879 -8.80137 -27.2075 Q -3.45906 -27.3931 -1.448 -27.168 Q -0.0752772 -27.015 4.16289 -27.4135 Q 10.8196 -28.0394 11.87 -27.128 Q 12.2805 -26.7721 12.8982 -26.101 Q 13.6749 -25.2572 13.954 -25.3347 Q 14.357 -25.4465 14.766 -27.117 Q 15.0952 -28.463 16.8931 -29.7173 Q 17.9201 -30.4338 20.0873 -31.4678 Q 21.829 -32.2988 21.829 -32.555 Q 22.5682 -26.9211 24.1684 -21.8535 Q 26.1589 -15.5495 28.661 -13.252 Q 30.0955 -11.9361 31.9017 -10.4305 Q 33.2422 -9.31319 33.5231 -8.9125 Q 33.873 -8.4134 33.873 -7.455 L 33.8365 -6.12307 Q 33.6916 -2.48367 34.451 -1.268 L 35.308 -0.0328611 Q 42.559 10.1797 42.559 14.581 L 42.5742 17.7994 Q 42.6361 26.7132 42.172 28.107 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 55"
        id: _qt_layer16
        transform: TransformGroup {
            id: _qt_layer16_transform_base_group
            TransformGroup {
                id: _qt_layer16_transform_group_0
                Translate { x: -527.891; y: -460.166 }
                Scale { id: _qt_layer16_transform_0_2}
                Translate { x: 427.891; y: 232.666 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer16_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer16_transform_animation
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
                                script:_qt_layer16_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer16_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1067 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer16_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer16_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3133 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer16_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer16_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer16_transform_base_group.deactivateOverride(_qt_layer16_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer16_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer16_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer16.opacity = 0
                    }
                    PauseAnimation { duration: 1067 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer16
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3300 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer16
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
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
                    PauseAnimation { duration: 1067 }
                    ScriptAction {
                        script:_qt_layer16.visible = true
                    }
                    PauseAnimation { duration: 4733 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node17
            transform: TransformGroup {
                id: _qt_node17_transform_base_group
                Matrix4x4 { id: _qt_node17_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 531.241, 457.247)}
            }
            ShapePath {
                id: _qt_shapePath_17
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 22.733 14.859 Q 22.733 16.3202 23.039 18.697 L 23.039 18.713 Q 23.039 18.9673 21.3057 19.7921 Q 19.1519 20.817 18.1313 21.5268 Q 16.3448 22.7693 16.017 24.102 Q 15.6115 25.758 15.2105 25.8682 Q 14.9329 25.9445 14.1598 25.1067 Q 13.5468 24.4424 13.14 24.09 Q 12.0968 23.1876 5.48643 23.8077 Q 1.26997 24.2033 -0.096 24.051 Q -2.09228 23.8284 -7.39729 24.0121 Q -10.8233 24.1307 -11.794 24.051 Q -12.9541 23.9602 -13.844 21.379 Q -14.4032 19.7438 -15.0749 16.0049 L -15.444 14.095 Q -15.7598 12.7257 -14.8804 10.7569 Q -13.9762 8.73269 -13.717 7.005 Q -13.4736 5.38555 -14.2047 3.82054 Q -14.655 2.85676 -16.0825 0.948087 Q -17.9218 -1.51108 -18.701 -3.139 Q -19.6645 -5.14959 -20.0252 -6.74421 Q -20.2519 -7.74676 -20.4 -9.66905 Q -20.5638 -11.7953 -20.813 -13.29 Q -21.2911 -16.1625 -22.9239 -20.293 Q -23.7402 -22.3582 -24.461 -23.849 Q -21.9395 -24.1313 -16.2059 -25.1629 L -13.844 -25.584 L -13.844 -25.59 L -9.73375 -26.2362 Q -7.69635 -26.503 -6.426 -26.503 Q -4.55365 -26.503 -1.919 -27.222 Q -2.66403 -26.6286 -3.09812 -25.9536 Q -3.92456 -24.6687 -2.973 -23.849 Q -1.98425 -22.9946 -0.29375 -22.0064 Q 0.5515 -21.5123 1.199 -21.189 Q 4.22 -18.604 6.667 -15.445 Q 7.70897 -14.0987 11.4332 -11.8425 Q 16.3457 -8.86637 16.593 -6.971 Q 17.1332 -2.83102 19.47 -1.083 Q 23.5903 2.00007 24.461 3.559 L 24.3328 4.06961 Q 23.3991 7.7855 23.1326 9.39902 Q 22.733 11.8183 22.733 14.859 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 62"
        id: _qt_layer17
        transform: TransformGroup {
            id: _qt_layer17_transform_base_group
            TransformGroup {
                id: _qt_layer17_transform_group_0
                Translate { x: -551.463; y: -436.764 }
                Scale { id: _qt_layer17_transform_0_2}
                Translate { x: 451.463; y: 209.264 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer17_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer17_transform_animation
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
                                script:_qt_layer17_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer17_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1100 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer17_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer17_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3100 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer17_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer17_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer17_transform_base_group.deactivateOverride(_qt_layer17_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer17_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer17_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer17.opacity = 0
                    }
                    PauseAnimation { duration: 1100 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer17
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3266 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer17
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer17_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer17_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer17.visible = false
                    }
                    PauseAnimation { duration: 1100 }
                    ScriptAction {
                        script:_qt_layer17.visible = true
                    }
                    PauseAnimation { duration: 4700 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node18
            transform: TransformGroup {
                id: _qt_node18_transform_base_group
                Matrix4x4 { id: _qt_node18_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 550.959, 442.405)}
            }
            ShapePath {
                id: _qt_shapePath_18
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 24.076 -5.465 Q 21.6201 -5.14597 20.677 -3.678 Q 20.0412 -2.70016 20.0469 -0.54258 Q 20.05 0.633166 19.9359 1.16491 Q 19.7471 2.04452 19.131 2.584 L 18.3846 3.22073 Q 13.7851 7.12038 10.962 9.91754 Q 5.71568 15.1156 5.043 17.761 Q 4.16708 16.2143 0.022 13.161 Q -2.32982 11.4231 -2.872 7.326 Q -3.12218 5.44791 -8.06534 2.49944 Q -11.8132 0.263966 -12.862 -1.07 Q -15.322 -4.201 -18.362 -6.763 Q -19.0135 -7.08325 -19.8641 -7.573 Q -21.5654 -8.5525 -22.561 -9.4 Q -23.5177 -10.2115 -22.6864 -11.484 Q -22.2471 -12.1565 -21.5 -12.74 L -21.495 -12.74 Q -19.8292 -13.1934 -16.5365 -14.3701 Q -12.497 -15.8136 -10.98 -16.036 Q 1.14847 -17.7986 2.628 -16.036 Q 3.47386 -15.0248 6.71255 -15.64 Q 9.4586 -16.1616 11.023 -16.036 Q 12.8894 -15.8841 17.5877 -10.6671 Q 22.3007 -5.43378 24.076 -5.465 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 63"
        id: _qt_layer18
        transform: TransformGroup {
            id: _qt_layer18_transform_base_group
            TransformGroup {
                id: _qt_layer18_transform_group_0
                Translate { x: -567; y: -418.435 }
                Scale { id: _qt_layer18_transform_0_2}
                Translate { x: 467; y: 190.935 }
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
                                script:_qt_layer18_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer18_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1133 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer18_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer18_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3066 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer18_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer18_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer18_transform_base_group.deactivateOverride(_qt_layer18_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer18_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer18_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer18.opacity = 0
                    }
                    PauseAnimation { duration: 1133 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer18
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3233 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer18
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer18_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer18_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer18.visible = false
                    }
                    PauseAnimation { duration: 1133 }
                    ScriptAction {
                        script:_qt_layer18.visible = true
                    }
                    PauseAnimation { duration: 4667 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node19
            transform: TransformGroup {
                id: _qt_node19_transform_base_group
                Matrix4x4 { id: _qt_node19_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 557.515, 418.468)}
            }
            ShapePath {
                id: _qt_shapePath_19
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 37.4 -4.434 Q 36.6862 -4.38409 35.1463 -4.41658 Q 33.0727 -4.46034 32.3389 -4.30003 Q 31.1295 -4.03583 31.223 -3.093 Q 31.3746 -1.61499 31.7572 -0.28875 Q 32.1536 1.08531 32.578 1.506 Q 32.7711 1.69815 33.6284 2.12553 Q 34.4844 2.55224 34.6771 2.74385 Q 34.9983 3.06317 34.315 3.232 L 34.1308 3.27756 Q 29.2448 4.48195 27.947 7.066 Q 27.2993 8.34987 25.3491 9.99899 Q 23.3295 11.7067 22.927 13.006 Q 22.6552 13.8885 22.6414 15.5954 Q 22.6301 16.9753 22.4238 17.4387 Q 22.0928 18.182 21 18.182 L 20.4195 18.1797 Q 18.4323 18.168 17.474 18.296 L 17.453 18.296 Q 15.677 18.3287 10.9642 13.0397 Q 6.27067 7.77235 4.406 7.618 Q 2.84154 7.49161 0.0963688 8.01814 Q -3.142 8.63927 -3.988 7.618 Q -5.46493 5.83874 -17.591 7.618 Q -19.1064 7.84299 -23.1388 9.29992 Q -26.4343 10.4906 -28.102 10.949 L -28.109 10.949 Q -30.758 11.667 -32.639 11.667 Q -33.9162 11.667 -35.9674 11.9347 L -40.102 12.582 Q -40.0547 10.8424 -32.7739 3.66799 Q -30.1534 1.08581 -29.677 0.454 Q -28.6823 -0.862434 -23.7761 -2.16757 Q -21.4747 -2.77977 -20.774 -3.122 Q -19.5071 -3.74012 -18.3639 -6.29038 Q -17.7922 -7.5655 -17.474 -8.717 L -16.6958 -8.65082 Q -15.2703 -8.52645 -14.7568 -8.5401 Q -13.9632 -8.5612 -13.539 -8.843 Q -12.381 -9.608 34.911 -18.36 Q 38.9925 -14.733 39.135 -12.673 L 39.2092 -11.7621 Q 39.4833 -8.55988 39.3036 -7.10233 Q 38.9882 -4.54568 37.4 -4.434 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 64"
        id: _qt_layer19
        transform: TransformGroup {
            id: _qt_layer19_transform_base_group
            TransformGroup {
                id: _qt_layer19_transform_group_0
                Translate { x: -568.5; y: -391.547 }
                Scale { id: _qt_layer19_transform_0_2}
                Translate { x: 468.5; y: 164.047 }
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
                                script:_qt_layer19_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer19_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1167 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3033 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer19_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer19_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
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
                    PauseAnimation { duration: 1167 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer19
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3200 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer19
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer19_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer19_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer19.visible = false
                    }
                    PauseAnimation { duration: 1167 }
                    ScriptAction {
                        script:_qt_layer19.visible = true
                    }
                    PauseAnimation { duration: 4633 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node20
            transform: TransformGroup {
                id: _qt_node20_transform_base_group
                Matrix4x4 { id: _qt_node20_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 560.068, 391.33)}
            }
            ShapePath {
                id: _qt_shapePath_20
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 32.079 8.414 Q -14.899 17.159 -16.048 17.923 Q -16.4698 18.2048 -17.2588 18.2259 Q -17.7692 18.2395 -19.1856 18.1151 L -19.958 18.049 Q -21.1299 17.9532 -22.1156 17.9193 Q -23.4791 17.8723 -24.862 17.923 Q -26.65 17.9887 -28.6647 18.2076 L -32.079 18.652 Q -31.619 17.268 -23.138 9.932 Q -22.1824 9.8995 -19.4021 10.3665 L -16.813 10.84 Q -15.1842 11.16 -13.4119 11.0168 Q -9.86712 10.7303 -9.149 8.414 Q -7.86326 4.27416 -4.163 -0.267 Q -2.81878 -1.92325 -3.11674 -4.82471 Q -3.32249 -6.82829 -2.439 -7.357 Q -0.524 -8.506 0.627 -9.844 Q 1.34575 -11.5278 2.352 -13.2575 Q 4.3645 -16.717 5.802 -16.947 Q 6.63783 -17.0804 8.47556 -17.5937 Q 11.516 -18.443 12.127 -18.077 Q 12.3689 -17.9323 13.5143 -17.722 Q 15.5897 -17.3408 16.3829 -16.8676 Q 17.7695 -16.0403 17.111 -14.441 Q 16.7648 -13.6017 16.0102 -12.2577 Q 15.13 -10.6897 15.23 -10.2019 Q 15.3806 -9.46728 17.324 -9.271 L 21.4531 -8.82989 L 25.4594 -8.37756 L 25.789 -8.34 Q 25.869 -7.02067 26.5726 -4.08727 Q 27.3495 -0.848857 27.266 0.307 Q 27.1314 2.21455 27.819 3.99225 Q 28.4516 5.62776 29.377 6.242 Q 29.764 6.49674 30.449 7.03925 Q 31.2713 7.69048 32.079 8.414 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 65"
        id: _qt_layer20
        transform: TransformGroup {
            id: _qt_layer20_transform_base_group
            TransformGroup {
                id: _qt_layer20_transform_group_0
                Translate { x: -589.892; y: -362.814 }
                Scale { id: _qt_layer20_transform_0_2}
                Translate { x: 489.892; y: 135.314 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer20_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer20_transform_animation
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
                                script:_qt_layer20_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer20_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer20_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer20_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3000 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer20_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer20_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer20_transform_base_group.deactivateOverride(_qt_layer20_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer20_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer20_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer20.opacity = 0
                    }
                    PauseAnimation { duration: 1200 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer20
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3166 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer20
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer20_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer20_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer20.visible = false
                    }
                    PauseAnimation { duration: 1200 }
                    ScriptAction {
                        script:_qt_layer20.visible = true
                    }
                    PauseAnimation { duration: 4600 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node21
            transform: TransformGroup {
                id: _qt_node21_transform_base_group
                Matrix4x4 { id: _qt_node21_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 589.825, 362.251)}
            }
            ShapePath {
                id: _qt_shapePath_21
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 11.628 -11.248 Q 11.0956 -10.0946 11.1872 -7.11913 Q 11.3004 -3.44343 10.48 -1.704 Q 9.93433 -0.548223 9.30073 1.97265 Q 8.76176 4.11701 8.36928 4.92683 Q 7.72621 6.25367 6.651 6.501 Q 5.83985 6.68786 4.32364 6.73352 Q 2.98397 6.77386 2.60059 6.96319 Q 1.99512 7.2622 2.257 8.218 Q 2.48569 9.05545 4.27489 10.3457 Q 5.70116 11.3742 6.21312 12.0328 Q 7.07637 13.1434 6.847 14.516 Q 4.48504 15.1057 -0.038 11.831 Q -0.665661 11.3766 -0.836172 8.36342 Q -1.03719 4.81114 -1.955 4.405 Q -2.7561 4.04818 -3.40575 5.56606 Q -3.79234 6.4693 -4.40533 8.98977 Q -4.91605 11.0897 -5.202 11.831 Q -5.75637 13.248 -3.028 17.957 Q -3.1937 18.163 -3.292 18.524 Q -3.40074 18.929 -3.43412 19.4165 Q -3.46561 19.8762 -3.43 20.408 L -3.76232 20.3702 L -7.75786 19.9194 L -11.875 19.48 Q -13.8159 19.2847 -13.9663 18.552 Q -14.0661 18.0656 -13.187 16.5017 Q -12.4344 15.1628 -12.089 14.327 Q -11.4321 12.7334 -12.8149 11.9091 Q -13.606 11.4375 -15.6764 11.0575 Q -16.8204 10.8475 -17.062 10.703 Q -18.0208 9.508 -20.1793 7.3465 L -22.146 5.424 Q -14.111 2.877 -9.603 2.877 Q -5.93887 2.877 -4.343 2.60912 Q -1.86559 2.19328 0.341 0.77 Q 3.08311 -0.991763 4.00025 -2.34763 Q 5.11756 -3.99941 3.782 -5.328 Q 2.58888 -6.523 0.484125 -9.2915 L -1.382 -11.821 Q -0.57 -13.4225 0.0275 -15.2968 Q 1.2225 -19.0452 0.15 -20.409 Q 0.869684 -20.1682 3.39042 -18.8555 Q 8.87156 -16.0011 9.843 -16.418 Q 9.24933 -15.662 9.332 -15.061 Q 9.43666 -14.3223 16.4528 -17.1484 Q 21.5946 -19.2196 21.766 -18.88 Q 21.9281 -18.5541 18.3085 -16.3444 Q 12.2844 -12.6668 11.628 -11.248 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 68"
        id: _qt_layer21
        transform: TransformGroup {
            id: _qt_layer21_transform_base_group
            TransformGroup {
                id: _qt_layer21_transform_group_0
                Translate { x: -567.416; y: -353.423 }
                Scale { id: _qt_layer21_transform_0_2}
                Translate { x: 467.416; y: 125.923 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer21_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer21_transform_animation
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
                                script:_qt_layer21_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer21_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1167 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer21_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer21_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3033 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer21_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer21_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer21_transform_base_group.deactivateOverride(_qt_layer21_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer21_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer21_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer21.opacity = 0
                    }
                    PauseAnimation { duration: 1167 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer21
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3200 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer21
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer21_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer21_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer21.visible = false
                    }
                    PauseAnimation { duration: 1167 }
                    ScriptAction {
                        script:_qt_layer21.visible = true
                    }
                    PauseAnimation { duration: 4633 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node22
            transform: TransformGroup {
                id: _qt_node22_transform_base_group
                Matrix4x4 { id: _qt_node22_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 567.36, 353.056)}
            }
            ShapePath {
                id: _qt_shapePath_22
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 22.531 9.758 Q 20.3101 11.1861 17.8174 11.6032 Q 16.2116 11.872 12.524 11.872 Q 7.99046 11.872 -0.096 14.428 Q -1.80022 14.9668 -3.254 15.508 Q -6.48485 16.7132 -10.6158 16.5846 Q -14.4155 16.4664 -15.378 15.508 Q -16.1936 14.6966 -20.2465 15.2978 Q -21.6632 15.508 -22.116 15.508 Q -23.5725 15.508 -24.9361 9.80497 L -24.946 9.764 L -24.946 9.758 Q -24.8249 7.3825 -25.788 5.743 Q -27.0841 3.53673 -26.1316 0.513063 Q -25.5548 -1.31811 -25.788 -2.298 Q -26.0997 -3.58154 -26.573 -6.41 Q -24.9906 -7.90033 -23.849 -8.806 Q -22.7336 -9.69054 -20.645 -10.953 Q -20.483 -10.3197 -19.7682 -9.8065 Q -18.3388 -8.78 -15.575 -9.38 L -15.4064 -9.41652 L -7.03489 -11.1013 Q -1.54534 -12.062 1.555 -12.062 Q 3.70386 -12.062 10.7682 -14.082 Q 19.0926 -16.4623 19.84 -15.509 Q 20.3843 -14.8118 20.6403 -13.5483 Q 20.8107 -12.7077 21.0417 -12.3592 Q 21.4168 -11.7933 22.341 -11.488 Q 23.4191 -10.1199 22.2169 -6.35913 Q 21.6157 -4.47875 20.799 -2.872 L 22.6763 -0.334625 Q 24.7938 2.44263 25.995 3.642 Q 27.3381 4.974 26.2134 6.63062 Q 25.2904 7.99008 22.531 9.758 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 20"
        id: _qt_layer22
        transform: TransformGroup {
            id: _qt_layer22_transform_base_group
            TransformGroup {
                id: _qt_layer22_transform_group_0
                Translate { x: -558.252; y: -370.808 }
                Scale { id: _qt_layer22_transform_0_2}
                Translate { x: 458.252; y: 143.308 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer22_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer22_transform_animation
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
                                script:_qt_layer22_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer22_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1133 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer22_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer22_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3066 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer22_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer22_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer22_transform_base_group.deactivateOverride(_qt_layer22_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer22_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer22_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer22.opacity = 0
                    }
                    PauseAnimation { duration: 1133 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer22
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3233 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer22
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer22_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer22_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer22.visible = false
                    }
                    PauseAnimation { duration: 1133 }
                    ScriptAction {
                        script:_qt_layer22.visible = true
                    }
                    PauseAnimation { duration: 4667 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node23
            transform: TransformGroup {
                id: _qt_node23_transform_base_group
                Matrix4x4 { id: _qt_node23_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 558.252, 370.777)}
            }
            ShapePath {
                id: _qt_shapePath_23
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 13.237 1.906 Q 12.6389 1.57703 9.66151 2.34191 Q 7.86041 2.80462 7.041 2.925 Q 5.914 1.358 -2.912 2.925 Q -3.02825 1.40562 -8.14425 -0.394125 Q -10.7022 -1.294 -13.237 -1.89 Q -12.795 -1.89 -11.4125 -2.07934 Q -7.45881 -2.62085 -6.663 -1.89 Q -5.72315 -1.02694 -2.01638 -0.920625 Q 2.01327 -0.805045 5.166 -1.89 Q 6.59101 -2.37975 8.247 -2.863 L 10.1776 -1.12612 Q 12.2964 0.826625 13.237 1.906 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 19"
        id: _qt_layer23
        transform: TransformGroup {
            id: _qt_layer23_transform_base_group
            TransformGroup {
                id: _qt_layer23_transform_group_0
                Translate { x: -547.032; y: -383.355 }
                Scale { id: _qt_layer23_transform_0_2}
                Translate { x: 447.032; y: 155.855 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer23_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer23_transform_animation
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
                                script:_qt_layer23_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer23_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1100 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer23_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer23_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3100 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer23_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer23_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer23_transform_base_group.deactivateOverride(_qt_layer23_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer23_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer23_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer23.opacity = 0
                    }
                    PauseAnimation { duration: 1100 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer23
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3266 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer23
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer23_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer23_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer23.visible = false
                    }
                    PauseAnimation { duration: 1100 }
                    ScriptAction {
                        script:_qt_layer23.visible = true
                    }
                    PauseAnimation { duration: 4700 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node24
            transform: TransformGroup {
                id: _qt_node24_transform_base_group
                Matrix4x4 { id: _qt_node24_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 546.821, 383.873)}
            }
            ShapePath {
                id: _qt_shapePath_24
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 18.82 -8.725 Q 17.3875 -8.50187 15.3825 -5.14313 Q 14.38 -3.46375 13.664 -1.829 Q 12.518 -0.53 10.61 0.586 Q 9.72963 1.09913 9.93433 3.04468 Q 10.2307 5.86149 8.892 7.469 Q 5.20431 11.8773 3.924 15.897 Q 3.20837 18.1458 -0.323375 18.4233 Q -2.08925 18.562 -3.712 18.251 L -6.29187 17.7914 Q -9.06213 17.3381 -10.014 17.37 Q -11.404 17.4109 -17.4996 9.47733 L -18.046 8.768 Q -18.5856 8.07156 -18.2973 6.82788 Q -18.0578 5.79493 -17.233 4.344 Q -16.5945 3.20708 -15.1219 1.09758 Q -14.0321 -0.4635 -13.836 -0.898 Q -13.5009 -1.63473 -13.0489 -2.00857 Q -12.7521 -2.25406 -11.9545 -2.62974 Q -10.8318 -3.15853 -10.014 -3.688 Q -8.77202 -4.4952 -8.26586 -5.77794 Q -7.77231 -7.0287 -6.846 -7.966 Q -5.3559 -9.476 -5.1647 -10.0681 Q -4.97729 -10.6484 -5.3417 -12.5822 L -5.362 -12.69 Q -5.54959 -13.6951 -5.05497 -16.322 Q -4.6606 -18.4166 -4.612 -19.496 L -4.60339 -19.4608 Q -3.25412 -13.918 -1.805 -13.918 Q 0.773 -13.2753 3.37475 -12.3048 Q 8.57825 -10.3638 8.697 -8.725 Q 10.9412 -9.1475 13.3287 -9.35875 Q 18.1038 -9.78125 18.82 -8.725 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 69"
        id: _qt_layer24
        transform: TransformGroup {
            id: _qt_layer24_transform_base_group
            TransformGroup {
                id: _qt_layer24_transform_group_0
                Translate { x: -522.46; y: -367.263 }
                Scale { id: _qt_layer24_transform_0_2}
                Translate { x: 422.46; y: 139.763 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer24_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer24_transform_animation
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
                                script:_qt_layer24_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer24_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 1000 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer24_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer24_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer24_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer24_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer24_transform_base_group.deactivateOverride(_qt_layer24_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer24_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer24_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer24.opacity = 0
                    }
                    PauseAnimation { duration: 1000 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer24
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3366 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer24
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer24_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer24_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer24.visible = false
                    }
                    PauseAnimation { duration: 1000 }
                    ScriptAction {
                        script:_qt_layer24.visible = true
                    }
                    PauseAnimation { duration: 4800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node25
            transform: TransformGroup {
                id: _qt_node25_transform_base_group
                Matrix4x4 { id: _qt_node25_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 522.216, 367.338)}
            }
            ShapePath {
                id: _qt_shapePath_25
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 19.78 -4.251 L 19.78 -4.245 Q 19.731 -3.13781 19.3347 -0.990197 Q 18.8378 1.70269 19.026 2.733 L 19.0474 2.84882 Q 19.4126 4.82915 19.2243 5.42399 Q 19.0318 6.03188 17.535 7.578 Q 16.6049 8.53917 16.1089 9.82183 Q 15.5999 11.1379 14.351 11.966 Q 13.5301 12.5086 12.4028 13.0506 Q 11.6015 13.4359 11.3032 13.6878 Q 10.8489 14.0713 10.512 14.827 Q 10.3149 15.2728 9.21946 16.874 Q 7.73963 19.037 7.098 20.203 L 6.7754 20.0529 L 4.565 19.02 Q 3.23317 18.3939 2.09009 18.5152 Q 1.38091 18.5904 0.0221498 19.1535 Q -2.6227 20.2496 -4.968 20.163 Q -7.11851 20.0837 -7.68943 19.9541 Q -8.11378 19.8579 -8.30198 19.5064 Q -8.69707 18.7686 -10.023 18.076 Q -11.1261 17.4996 -12.3296 17.7192 Q -13.3442 17.9044 -14.421 18.654 L -15.7235 13.9265 Q -17.0735 8.865 -17.311 7.195 Q -17.4428 6.28235 -17.6777 2.72358 Q -18.2233 -5.5446 -18.871 -7.677 Q -19.4823 -9.70075 -19.2377 -11.6458 Q -19.1155 -12.6182 -18.871 -13.186 Q -16.1773 -13.527 -10.9139 -14.7201 Q -8.58168 -15.2488 -8.1 -15.302 Q -7.4251 -13.9566 -3.8325 -12.8699 Q 0.179763 -11.6562 3.603 -12.441 Q 6.22527 -13.0419 7.79068 -14.1892 Q 9.07461 -15.1302 10.897 -15.491 Q 12.6679 -15.8441 14.44 -17.1084 Q 15.5232 -17.8811 17.6465 -19.8746 L 18.157 -20.353 Q 18.6299 -17.5287 18.94 -16.258 Q 19.1728 -15.2815 18.5972 -13.4571 Q 17.6471 -10.4459 18.94 -8.249 Q 19.8994 -6.61694 19.78 -4.251 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 61"
        id: _qt_layer25
        transform: TransformGroup {
            id: _qt_layer25_transform_base_group
            TransformGroup {
                id: _qt_layer25_transform_group_0
                Translate { x: -499.958; y: -460.32 }
                Scale { id: _qt_layer25_transform_0_2}
                Translate { x: 399.958; y: 232.82 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer25_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer25_transform_animation
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
                                script:_qt_layer25_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer25_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 933 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer25_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer25_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3266 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer25_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer25_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer25_transform_base_group.deactivateOverride(_qt_layer25_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer25_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer25_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer25.opacity = 0
                    }
                    PauseAnimation { duration: 933 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer25
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3433 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer25
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer25_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer25_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer25.visible = false
                    }
                    PauseAnimation { duration: 933 }
                    ScriptAction {
                        script:_qt_layer25.visible = true
                    }
                    PauseAnimation { duration: 4867 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node26
            transform: TransformGroup {
                id: _qt_node26_transform_base_group
                Matrix4x4 { id: _qt_node26_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 500.053, 461.072)}
            }
            ShapePath {
                id: _qt_shapePath_26
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 17.049 3.129 Q 16.7903 4.86507 15.8874 6.89892 Q 15.0091 8.87744 15.325 10.254 L 15.6927 12.1688 Q 16.364 15.9292 16.923 17.574 Q 14.799 17.7647 12.1965 18.1474 Q 6.9915 18.9126 4.599 19.872 Q 2.10258 20.876 -3.50546 20.1543 Q -6.21405 19.8057 -7.283 19.872 Q -9.0162 19.9834 -8.41694 20.9047 Q -8.05489 21.4613 -6.1853 22.9208 L -5.173 23.724 Q -3.4879 25.1102 -5.501 26.461 Q -5.5043 26.461 -5.51058 26.4639 Q -5.5152 26.466 -5.517 26.466 L -5.69698 26.57 L -6.03 26.76 Q -6.06844 26.7868 -6.109 26.802 Q -6.122 26.807 -6.128 26.814 Q -7.18997 27.3993 -7.68475 27.4571 Q -8.37842 27.5382 -8.622 26.802 Q -9.25854 24.8846 -10.0596 23.7704 Q -11.1901 22.198 -12.071 23.336 Q -12.9116 24.4191 -14.5874 25.5284 Q -15.4253 26.083 -16.095 26.421 Q -15.8219 24.7563 -16.2161 20.9102 Q -16.5996 17.169 -17.049 16.409 Q -17.624 15.451 -17.049 -26.858 L -11.7423 -27.4236 Q -9.29612 -27.66 -8.432 -27.66 L 3.065 -27.66 Q 3.76978 -27.66 4.60638 -27.7179 Q 5.39824 -27.7727 6.318 -27.88 Q 7.03775 -26.3818 7.85313 -24.306 Q 9.48388 -20.1545 9.962 -17.267 Q 10.2109 -15.7647 10.3745 -13.6277 Q 10.5223 -11.6959 10.7487 -10.6885 Q 11.1088 -9.08612 12.071 -7.066 Q 12.8495 -5.42953 14.6868 -2.95809 Q 16.1127 -1.04012 16.5623 -0.0715923 Q 17.2924 1.50109 17.049 3.129 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 18"
        id: _qt_layer26
        transform: TransformGroup {
            id: _qt_layer26_transform_base_group
            TransformGroup {
                id: _qt_layer26_transform_group_0
                Translate { x: -501.894; y: -402.313 }
                Scale { id: _qt_layer26_transform_0_2}
                Translate { x: 401.894; y: 174.813 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer26_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer26_transform_animation
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
                                script:_qt_layer26_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer26_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 900 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer26_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer26_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3300 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer26_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer26_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer26_transform_base_group.deactivateOverride(_qt_layer26_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer26_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer26_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer26.opacity = 0
                    }
                    PauseAnimation { duration: 900 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer26
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3466 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer26
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer26_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer26_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer26.visible = false
                    }
                    PauseAnimation { duration: 900 }
                    ScriptAction {
                        script:_qt_layer26.visible = true
                    }
                    PauseAnimation { duration: 4900 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node27
            transform: TransformGroup {
                id: _qt_node27_transform_base_group
                Matrix4x4 { id: _qt_node27_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 501.894, 402.401)}
            }
            ShapePath {
                id: _qt_shapePath_27
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 34.23 -0.977 Q 25.756 6.362 25.297 7.746 L 24.5106 7.85988 Q 21.0642 8.361 19.869 8.361 Q 18.4007 8.361 13.0008 9.51562 L 4.0895 11.5488 L 1.674 12.114 Q -1.13636 12.7587 -7.422 12.7726 Q -13.5852 12.7863 -15.945 12.211 Q -17.2576 11.8907 -19.0913 12.6593 Q -20.1849 13.1176 -22.4008 14.4742 Q -24.7862 15.9346 -25.861 16.309 Q -27.7454 16.9665 -31.1761 16.7035 Q -32.8915 16.572 -34.23 16.309 Q -33.9012 15.5217 -33.7574 15.0212 Q -33.5238 14.2079 -32.984 12.735 Q -32.409 11.1522 -32.6459 10.6006 Q -32.8064 10.2267 -33.759 9.919 Q -33.0681 9.24176 -31.5333 9.54113 Q -29.7545 9.88805 -28.583 11.504 Q -27.7172 12.6994 -27.4529 11.839 Q -27.2911 11.3125 -27.2898 9.19223 Q -27.2889 7.6901 -27.24 7.017 Q -27.1602 5.89749 -26.7124 5.48681 Q -26.431 5.22875 -25.6195 5.07332 Q -24.4504 4.84937 -23.792 4.338 Q -22.9285 3.66445 -22.7539 3.17634 Q -22.6157 2.78979 -22.6731 1.52853 Q -22.7465 -0.082158 -22.707 -1.408 Q -21.9142 -1.55175 -21.0044 -1.62363 Q -19.1846 -1.76737 -18.599 -1.408 Q -17.9007 -0.97675 -17.0653 -0.97675 Q -16.2297 -0.97675 -14.795 -1.408 Q -13.8026 -1.70552 -12.054 -2.928 Q -10.6609 -3.902 -10.201 -3.902 Q -9.77239 -3.902 -8.79068 -4.94188 Q -8.20841 -5.55864 -7.95657 -5.71725 Q -7.54082 -5.97909 -7.328 -5.626 Q -6.801 -5.2985 -6.08225 -5.16062 Q -4.64475 -4.88487 -3.686 -5.833 Q -2.67497 -6.83675 -0.431375 -9.43375 L 2.497 -12.901 L 2.5921 -13.0193 Q 4.22353 -15.0503 5.517 -15.953 Q 6.5912 -16.7059 7.60437 -16.8918 Q 8.80594 -17.1122 9.907 -16.533 Q 11.2309 -15.8373 11.6248 -15.0957 Q 11.8125 -14.7424 12.2359 -14.6456 Q 12.8059 -14.5153 14.951 -14.436 Q 17.2908 -14.3489 19.9302 -15.4504 Q 21.2864 -16.0163 21.9942 -16.092 Q 23.1351 -16.2141 24.464 -15.585 L 26.7574 -14.5059 L 26.992 -14.396 Q 26.1647 -12.9014 25.9244 -11.8369 Q 25.635 -10.5554 26.176 -9.838 L 26.7354 -9.09246 Q 32.8389 -0.935285 34.23 -0.977 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 66"
        id: _qt_layer27
        transform: TransformGroup {
            id: _qt_layer27_transform_base_group
            TransformGroup {
                id: _qt_layer27_transform_group_0
                Translate { x: -493.322; y: -377.654 }
                Scale { id: _qt_layer27_transform_0_2}
                Translate { x: 393.322; y: 150.154 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer27_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer27_transform_animation
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
                                script:_qt_layer27_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer27_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 867 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer27_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer27_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer27_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer27_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer27_transform_base_group.deactivateOverride(_qt_layer27_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer27_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer27_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer27.opacity = 0
                    }
                    PauseAnimation { duration: 867 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer27
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer27
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer27_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer27_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer27.visible = false
                    }
                    PauseAnimation { duration: 867 }
                    ScriptAction {
                        script:_qt_layer27.visible = true
                    }
                    PauseAnimation { duration: 4933 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node28
            transform: TransformGroup {
                id: _qt_node28_transform_base_group
                Matrix4x4 { id: _qt_node28_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 493.163, 377.619)}
            }
            ShapePath {
                id: _qt_shapePath_28
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 11.348 -2.979 Q 11.5843 -1.29712 12.9268 3.80063 L 14.222 8.562 Q 12.9467 9.45649 11.3249 11.478 L 11.21 11.621 L 8.29362 15.0977 Q 6.05944 17.7009 5.051 18.708 Q 4.096 19.6586 2.663 19.3824 Q 1.9465 19.2442 1.421 18.916 Q 1.20968 18.5618 0.795827 18.8242 Q 0.545124 18.9831 -0.034856 19.6013 Q -1.01312 20.644 -1.44 20.644 Q -1.89832 20.644 -3.28631 21.6205 Q -5.02944 22.8469 -6.019 23.145 Q -7.4485 23.577 -8.28063 23.577 Q -9.11275 23.577 -9.808 23.145 Q -10.3911 22.785 -12.2044 22.929 Q -13.111 23.001 -13.901 23.145 Q -13.8266 20.5364 -13.2697 19.3967 Q -12.8826 18.6044 -11.4589 17.3729 L -10.462 16.49 Q -8.94888 15.0894 -10.0976 13.5986 Q -10.672 12.8532 -11.549 12.388 L -11.549 1.63 Q -11.549 -2.28859 -12.8881 -6.51076 Q -13.4661 -8.33295 -13.649 -9.317 Q -14.0071 -11.2364 -13.8639 -16.2301 L -13.649 -20.84 Q -12.719 -20.647 -11.5149 -20.7421 Q -9.10662 -20.9324 -7.736 -22.373 Q -5.3505 -22.71 -2.44062 -22.9988 Q 3.37913 -23.5763 6.001 -23.335 Q 7.42979 -23.2014 9.796 -23.508 Q 9.553 -22.9363 9.4315 -21.9568 Q 9.1885 -19.9977 9.796 -17.959 Q 10.4404 -15.8113 10.9833 -7.48255 Q 11.2169 -3.89838 11.348 -2.979 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 67"
        id: _qt_layer28
        transform: TransformGroup {
            id: _qt_layer28_transform_base_group
            TransformGroup {
                id: _qt_layer28_transform_group_0
                Translate { x: -503.457; y: -341 }
                Scale { id: _qt_layer28_transform_0_2}
                Translate { x: 403.457; y: 113.5 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer28_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer28_transform_animation
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
                                script:_qt_layer28_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer28_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 833 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer28_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer28_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3366 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer28_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer28_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 267 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer28_transform_base_group.deactivateOverride(_qt_layer28_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer28_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer28_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer28.opacity = 0
                    }
                    PauseAnimation { duration: 833 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer28
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer28
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 433 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer28_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer28_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer28.visible = false
                    }
                    PauseAnimation { duration: 833 }
                    ScriptAction {
                        script:_qt_layer28.visible = true
                    }
                    PauseAnimation { duration: 4967 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node29
            transform: TransformGroup {
                id: _qt_node29_transform_base_group
                Matrix4x4 { id: _qt_node29_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 503.719, 330.608)}
            }
            ShapePath {
                id: _qt_shapePath_29
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 17.267 9.692 Q 16.9918 10.4326 15.2406 11.005 Q 14.3422 11.2987 14.0132 11.5051 Q 13.469 11.8465 13.469 12.375 Q 13.469 13.0177 12.0181 15.7534 Q 10.9398 17.7865 10.5733 18.6941 Q 9.95426 20.2269 10.249 20.821 Q 9.77134 20.8743 7.45682 21.4084 Q 2.25862 22.608 -0.401 22.951 Q -2.74867 23.2571 -4.175 23.123 Q -6.7825 22.8824 -12.5695 23.4596 Q -15.463 23.7482 -17.835 24.085 Q -16.0912 22.247 -14.7625 19.4071 Q -13.3352 16.3566 -13.469 14.298 Q -13.5674 12.7916 -15.4909 7.57433 Q -17.1735 3.01055 -17.266 1.822 Q -17.3727 0.407345 -16.037 -3.42784 Q -15.0703 -6.20327 -14.989 -7.199 Q -14.5865 -12.0019 -13.469 -12.571 Q -13.1184 -12.7498 -12.3769 -11.9105 Q -11.9124 -11.3847 -11.6538 -11.204 Q -11.2168 -10.8986 -10.811 -11.034 Q -10.3277 -11.1975 -10.3193 -13.5032 Q -10.3058 -17.1991 -9.104 -17.752 Q -7.61489 -18.4397 -7.30475 -18.7844 Q -7.05862 -19.0579 -7.208 -19.669 L -7.25043 -19.8354 Q -7.65325 -21.3884 -7.48892 -22.1456 Q -7.18318 -23.5543 -5.119 -22.744 Q -4.32124 -22.4293 -2.31419 -21.8026 Q 1.23734 -20.6935 2.74283 -19.9341 Q 5.30841 -18.6399 5.695 -16.987 Q 6.30528 -14.3548 6.33325 -13.0983 Q 6.36712 -11.5766 5.695 -10.078 Q 5.43821 -9.50625 4.14949 -7.8371 Q 2.1579 -5.25758 1.59632 -3.94463 Q 0.627721 -1.68007 2.087 -0.481 Q 4.19305 1.24666 6.11987 -0.193875 Q 7.83348 -1.47501 8.16 -3.935 Q 8.45737 -6.18205 9.67762 -7.006 Q 10.9374 -7.85663 11.763 -6.048 Q 18.013 7.6768 17.267 9.692 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 29"
        id: _qt_layer29
        transform: TransformGroup {
            id: _qt_layer29_transform_base_group
            TransformGroup {
                id: _qt_layer29_transform_group_0
                Translate { x: -467.463; y: -461.474 }
                Scale { id: _qt_layer29_transform_0_2}
                Translate { x: 367.463; y: 233.974 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer29_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer29_transform_animation
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
                                script:_qt_layer29_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer29_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 800 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer29_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer29_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer29_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer29_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 300 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer29_transform_base_group.deactivateOverride(_qt_layer29_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer29_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer29_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer29.opacity = 0
                    }
                    PauseAnimation { duration: 800 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer29
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer29
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 467 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer29_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer29_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer29.visible = false
                    }
                    PauseAnimation { duration: 800 }
                    ScriptAction {
                        script:_qt_layer29.visible = true
                    }
                    PauseAnimation { duration: 5000 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node30
            transform: TransformGroup {
                id: _qt_node30_transform_base_group
                Matrix4x4 { id: _qt_node30_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 466.883, 461.474)}
            }
            ShapePath {
                id: _qt_shapePath_30
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 16.66 25.945 Q 15.2715 25.801 13.6439 25.7526 Q 10.3886 25.6559 9.193 26.134 Q 8.62758 26.3605 7.74394 26.5008 Q 7.16038 26.5935 6.93833 26.6949 Q 6.58723 26.8551 6.406 27.242 Q 5.08537 26.582 4.22963 24.996 Q 3.80175 24.203 3.638 23.542 Q 3.69528 23.0818 3.8345 22.3589 Q 4.04435 21.2692 3.84129 21.0107 Q 3.56577 20.66 2.139 20.66 L -14.453 20.66 Q -15.0992 19.5072 -15.4224 18.1742 Q -16.0686 15.5083 -14.453 14.607 Q -12.4294 13.4793 -10.5407 10.7889 Q -8.58153 7.99794 -8.997 6.538 Q -9.14453 6.01842 -9.52487 4.95501 Q -10.72 1.61341 -10.72 -0.955 Q -10.72 -2.2086 -10.2869 -3.73691 Q -9.93896 -4.96502 -9.95534 -5.75926 Q -9.98195 -7.04944 -10.72 -9.025 Q -11.7432 -11.7626 -11.0701 -13.8721 Q -10.684 -15.0825 -9.3564 -16.659 L -8.997 -17.093 Q -8.54972 -17.654 -7.73938 -18.2448 Q -6.50706 -19.1433 -5.82 -20.551 Q -4.62432 -22.9899 -3.64625 -24.0106 Q -1.98908 -25.74 0.479 -25.74 Q 1.78447 -25.74 5.66173 -26.1329 L 13.3726 -26.9792 L 15.706 -27.243 Q 15.132 14.994 15.706 15.951 Q 16.1548 16.7098 16.5385 20.4436 Q 16.933 24.2831 16.66 25.945 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 58"
        id: _qt_layer30
        transform: TransformGroup {
            id: _qt_layer30_transform_base_group
            TransformGroup {
                id: _qt_layer30_transform_group_0
                Translate { x: -464.857; y: -381.773 }
                Scale { id: _qt_layer30_transform_0_2}
                Translate { x: 364.857; y: 154.273 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer30_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer30_transform_animation
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
                                script:_qt_layer30_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer30_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 767 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer30_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer30_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer30_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer30_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 333 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer30_transform_base_group.deactivateOverride(_qt_layer30_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer30_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer30_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer30.opacity = 0
                    }
                    PauseAnimation { duration: 767 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer30
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3534 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer30
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 500 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer30_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer30_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer30.visible = false
                    }
                    PauseAnimation { duration: 767 }
                    ScriptAction {
                        script:_qt_layer30.visible = true
                    }
                    PauseAnimation { duration: 5033 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node31
            transform: TransformGroup {
                id: _qt_node31_transform_base_group
                Matrix4x4 { id: _qt_node31_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 465.646, 382.615)}
            }
            ShapePath {
                id: _qt_shapePath_31
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 16.683 11.556 L 15.6805 12.4426 Q 14.25 13.6782 13.8608 14.4733 Q 13.3009 15.617 13.226 18.234 Q 13.1862 19.5697 13.2599 21.1914 Q 13.3175 22.4592 13.179 22.8479 Q 13.0041 23.3388 12.139 24.016 Q 11.4796 24.531 10.3089 24.7562 Q 9.49682 24.9125 9.21532 25.172 Q 8.76738 25.585 8.688 26.711 Q 8.63911 27.3882 8.63828 28.8991 Q 8.6371 31.0337 8.47494 31.5639 Q 8.20994 32.4302 7.342 31.227 Q 6.17004 29.6005 4.38875 29.2515 Q 2.85239 28.9505 2.16 29.631 Q 1.44135 29.395 1.01912 29.186 Q 0.293644 28.8268 -0.325 28.255 Q -1.36833 27.2862 -1.57729 26.5876 Q -1.70769 26.1517 -1.51976 25.3855 Q -1.25734 24.3157 -1.475 23.439 Q -1.68216 22.6057 -2.17413 22.1928 Q -2.48647 21.9306 -3.34312 21.5915 Q -5.22209 20.8475 -6.497 19.582 Q -8.3473 17.7446 -8.19388 14.5938 Q -8.06295 11.905 -6.842 10.33 Q -6.16228 9.44991 -6.83144 9.04673 Q -7.22974 8.80674 -8.59747 8.61291 Q -9.42684 8.49537 -9.753 8.405 Q -10.7426 8.12655 -12.8421 6.621 Q -14.8301 5.19538 -16.046 3.97 Q -17.0054 3.005 -18.1571 0.115 Q -18.733 -1.33 -19.117 -2.582 Q -18.9732 -3.83525 -18.5655 -5.37762 Q -17.75 -8.46237 -16.43 -9.908 Q -16.2055 -10.1506 -16.0417 -10.4511 Q -15.8903 -10.7291 -15.787 -11.064 Q -15.384 -12.4084 -16.0753 -15.8026 Q -16.6581 -18.6637 -16.43 -19.738 Q -16.2048 -20.7973 -15.6323 -21.3398 Q -15.2593 -21.6932 -14.156 -22.2237 Q -12.0384 -23.2421 -10.489 -24.554 Q -8.97873 -25.8313 -10.564 -28.6885 Q -11.7428 -30.8131 -14.509 -33.621 L 10.069 -33.621 Q 10.5429 -29.8781 11.1661 -28.2243 Q 11.9837 -26.0546 13.479 -25.907 L 13.2638 -21.2808 Q 13.1202 -16.2693 13.479 -14.343 Q 13.663 -13.3563 14.2435 -11.5293 Q 15.59 -7.29121 15.59 -3.356 L 15.59 7.439 Q 16.4718 7.906 17.0492 8.65413 Q 18.2043 10.1504 16.683 11.556 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 59"
        id: _qt_layer31
        transform: TransformGroup {
            id: _qt_layer31_transform_base_group
            TransformGroup {
                id: _qt_layer31_transform_group_0
                Translate { x: -454.412; y: -324.295 }
                Scale { id: _qt_layer31_transform_0_2}
                Translate { x: 354.412; y: 96.795 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer31_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer31_transform_animation
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
                                script:_qt_layer31_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer31_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 733 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer31_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer31_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3366 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer31_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer31_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 367 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer31_transform_base_group.deactivateOverride(_qt_layer31_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer31_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer31_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer31.opacity = 0
                    }
                    PauseAnimation { duration: 733 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer31
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer31
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 533 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer31_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer31_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer31.visible = false
                    }
                    PauseAnimation { duration: 733 }
                    ScriptAction {
                        script:_qt_layer31.visible = true
                    }
                    PauseAnimation { duration: 5067 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node32
            transform: TransformGroup {
                id: _qt_node32_transform_base_group
                Matrix4x4 { id: _qt_node32_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 454.671, 323.951)}
            }
            ShapePath {
                id: _qt_shapePath_32
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 22.477 0.386 Q 21.7381 1.86274 20.4268 3.68183 Q 19.0258 5.62536 18.793 6.99248 Q 18.4412 9.05874 19.397 13.464 Q 20.0344 16.3978 20.6878 22.1475 L 20.977 24.611 L -3.673 24.611 Q -5.502 22.759 -7.158 21.397 Q -7.39125 20.9513 -7.52875 20.145 Q -7.80375 18.5325 -7.325 16.73 Q -7.10344 15.8939 -7.17688 15.17 Q -7.2367 14.5802 -7.514 13.943 Q -7.6425 13.6494 -7.94802 13.0794 Q -8.78092 11.5256 -8.865 10.384 Q -8.93445 9.3959 -9.89654 8.70791 Q -10.4731 8.29563 -11.9698 7.69283 Q -14.3901 6.71812 -14.823 5.768 Q -15.5143 4.25262 -20.4813 1.42787 Q -22.2293 0.433794 -22.874 -0.001 Q -24.3053 -0.95975 -23.6493 -3.84375 Q -23.3213 -5.28575 -22.707 -6.536 Q -22.8018 -7.4015 -22.8491 -8.62763 Q -22.9439 -11.0799 -22.707 -12.883 Q -22.553 -14.0279 -21.9035 -14.7313 Q -21.522 -15.1445 -20.6451 -15.635 Q -19.591 -16.2245 -19.436 -16.788 Q -19.1941 -17.6717 -19.2909 -19.3088 Q -19.3393 -20.1273 -19.436 -20.769 Q -18.6639 -20.1904 -14.5156 -22.1697 Q -10.2048 -24.2264 -9.054 -23.844 Q -8.00701 -23.4953 -6.44585 -22.0711 Q -4.72181 -20.4983 -3.673 -20.596 Q -2.0625 -20.2542 -0.163625 -19.7682 Q 3.63412 -18.7962 5.076 -18.075 Q 6.9188 -17.1528 7.67075 -16.9511 Q 8.6174 -16.6972 10.267 -16.788 Q 12.5606 -16.9202 15.6739 -15.6667 Q 19.3983 -14.1672 19.784 -11.919 Q 19.6466 -10.8275 18.3072 -6.99735 Q 17.0913 -3.52043 16.8839 -2.40354 Q 16.5385 -0.542456 17.863 -1.535 Q 20.6222 -3.60665 22.3307 -3.45825 Q 24.3111 -3.28624 22.477 0.386 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 56"
        id: _qt_layer32
        transform: TransformGroup {
            id: _qt_layer32_transform_base_group
            TransformGroup {
                id: _qt_layer32_transform_group_0
                Translate { x: -478.003; y: -300.027 }
                Scale { id: _qt_layer32_transform_0_2}
                Translate { x: 378.003; y: 72.527 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer32_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer32_transform_animation
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
                                script:_qt_layer32_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer32_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 700 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer32_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer32_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer32_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer32_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 400 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer32_transform_base_group.deactivateOverride(_qt_layer32_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer32_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer32_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer32.opacity = 0
                    }
                    PauseAnimation { duration: 700 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer32
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer32
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 567 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer32_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer32_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer32.visible = false
                    }
                    PauseAnimation { duration: 700 }
                    ScriptAction {
                        script:_qt_layer32.visible = true
                    }
                    PauseAnimation { duration: 5100 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node33
            transform: TransformGroup {
                id: _qt_node33_transform_base_group
                Matrix4x4 { id: _qt_node33_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 479.464, 301.37)}
            }
            ShapePath {
                id: _qt_shapePath_33
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 18.165 2.319 Q 10.8564 1.24865 7.95888 1.65575 Q 5.05372 2.06392 2.81 4.483 Q 1.43726 5.96299 -1.71263 7.3175 Q -4.8528 8.66782 -5.057 10.224 Q -5.43136 8.1273 -9.05975 6.72825 Q -12.0929 5.55871 -14.327 5.682 Q -15.9345 5.76638 -16.8559 5.53025 Q -17.5898 5.34216 -19.384 4.483 Q -20.7884 3.8105 -24.4886 2.904 L -27.908 2.132 Q -26.588 2.01801 -16.8987 -5.43873 L -12.923 -8.431 Q -12.2525 -8.8889 -12.3338 -8.5336 Q -12.3862 -8.30451 -12.9556 -7.28836 Q -15.7311 -2.335 -12.923 -2.335 Q -10.8237 -2.335 -8.14455 0.115935 Q -6.65933 1.47465 -5.81278 1.89233 Q -4.41 2.58445 -2.81 2.142 Q -1.31004 1.72555 2.02358 -0.39987 Q 5.51818 -2.62792 7.34861 -3.25769 Q 10.4365 -4.32012 12.923 -3.051 L 15.8671 -1.5936 Q 21.0896 0.955228 22.0314 1.76376 Q 23.6018 3.11195 18.165 2.319 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 27"
        id: _qt_layer33
        transform: TransformGroup {
            id: _qt_layer33_transform_base_group
            TransformGroup {
                id: _qt_layer33_transform_group_0
                Translate { x: -443.713; y: -482.808 }
                Scale { id: _qt_layer33_transform_0_2}
                Translate { x: 343.713; y: 255.308 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer33_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer33_transform_animation
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
                                script:_qt_layer33_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer33_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 667 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer33_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer33_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer33_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer33_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 433 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer33_transform_base_group.deactivateOverride(_qt_layer33_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer33_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer33_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer33.opacity = 0
                    }
                    PauseAnimation { duration: 667 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer33
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3534 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer33
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 600 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer33_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer33_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer33.visible = false
                    }
                    PauseAnimation { duration: 667 }
                    ScriptAction {
                        script:_qt_layer33.visible = true
                    }
                    PauseAnimation { duration: 5133 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node34
            transform: TransformGroup {
                id: _qt_node34_transform_base_group
                Matrix4x4 { id: _qt_node34_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 450.823, 483.039)}
            }
            ShapePath {
                id: _qt_shapePath_34
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 23.8 19.789 L 22.9458 19.4429 Q 20.1509 18.3026 18.9634 18.0701 Q 16.9537 17.6768 16.494 18.827 Q 16.1612 19.6552 15.3411 20.0893 Q 14.8449 20.3519 13.6573 20.6469 Q 12.2977 20.9845 11.68 21.328 Q 10.2993 22.0909 8.7 21.7105 Q 6.98598 21.3028 6.296 19.789 Q 5.79404 18.6882 4.81327 17.9923 Q 3.76259 17.2468 3.024 16.142 Q 1.8873 14.4355 -0.84525 13.7638 Q -3.56044 13.0963 -3.705 14.411 Q -3.85469 15.7255 -5.41538 16.5243 Q -6.99902 17.3348 -8.709 16.908 Q -9.22964 16.7781 -10.8164 16.217 Q -14.1706 15.0309 -16.0864 14.6281 Q -19.4479 13.9215 -21.596 14.603 Q -22.5135 12.0584 -21.8634 7.902 Q -21.3066 4.34209 -20.252 2.497 Q -19.7203 1.56841 -19.6739 0.790866 Q -19.6431 0.275494 -19.8716 -0.92139 Q -20.1568 -2.41598 -20.252 -3.555 Q -20.412 -5.47588 -21.1737 -6.52944 Q -21.6704 -7.21652 -23.2286 -8.33543 L -24.552 -9.318 Q -26.5239 -10.8677 -25.0085 -21.24 L -24.857 -22.285 L 5.142 -22.285 Q 5.142 -19.7149 6.34373 -16.3715 Q 6.72485 -15.3111 6.873 -14.793 Q 7.29063 -13.3324 5.32212 -10.5421 Q 3.42461 -7.85251 1.39 -6.724 Q -0.233125 -5.82275 0.416125 -3.15725 Q 0.74075 -1.8245 1.39 -0.672 L 18.063 -0.672 Q 19.497 -0.672 19.774 -0.320978 Q 19.9781 -0.0623709 19.7673 1.0272 Q 19.6275 1.74995 19.57 2.21 Q 19.7345 2.87125 20.1644 3.66438 Q 21.0241 5.25063 22.351 5.91 Q 22.1338 6.36104 22.069 7.301 Q 21.9881 8.44082 20.5728 10.277 Q 19.7573 11.335 19.6966 11.7391 Q 19.5999 12.3829 20.724 12.684 Q 21.8569 12.9847 22.0987 13.4127 Q 22.2489 13.6784 22.0719 14.2969 Q 21.7834 15.3053 22.265 15.945 L 23.0762 16.9623 Q 24.5174 18.7284 24.8236 19.3462 Q 25.3502 20.4083 23.8 19.789 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 28"
        id: _qt_layer34
        transform: TransformGroup {
            id: _qt_layer34_transform_base_group
            TransformGroup {
                id: _qt_layer34_transform_group_0
                Translate { x: -443; y: -435.009 }
                Scale { id: _qt_layer34_transform_0_2}
                Translate { x: 343; y: 207.509 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer34_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer34_transform_animation
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
                                script:_qt_layer34_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer34_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 633 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer34_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer34_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3366 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer34_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer34_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 467 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer34_transform_base_group.deactivateOverride(_qt_layer34_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer34_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer34_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer34.opacity = 0
                    }
                    PauseAnimation { duration: 633 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer34
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer34
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 633 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer34_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer34_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer34.visible = false
                    }
                    PauseAnimation { duration: 633 }
                    ScriptAction {
                        script:_qt_layer34.visible = true
                    }
                    PauseAnimation { duration: 5167 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node35
            transform: TransformGroup {
                id: _qt_node35_transform_base_group
                Matrix4x4 { id: _qt_node35_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 479.176, 434.985)}
            }
            ShapePath {
                id: _qt_shapePath_35
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 60.488 -25.133 Q 60.1695 -23.9772 59.5973 -22.6975 Q 58.4528 -20.138 57.184 -19.518 Q 56.4824 -19.175 54.1804 -18.5608 Q 49.2661 -17.2496 48.269 -15.928 Q 47.7917 -15.2933 45.1649 -12.6994 Q 37.8763 -5.50228 37.829 -3.757 L 37.829 -3.751 L 35.4578 -3.32938 Q 29.6767 -2.29208 27.134 -2.008 Q 26.2055 -1.90122 25.4088 -1.84675 Q 24.5641 -1.789 23.853 -1.789 L 12.26 -1.789 Q 11.3896 -1.789 8.92163 -1.55225 L 3.571 -0.987 L 1.31585 -0.733765 L -6.5227 0.120582 Q -10.471 0.518 -11.796 0.518 Q -14.287 0.518 -15.9602 2.25 Q -16.9468 3.27113 -18.155 5.713 Q -18.8491 7.12374 -20.0922 8.02334 Q -20.9093 8.61463 -21.361 9.176 L -21.7256 9.61321 Q -23.0646 11.1918 -23.4538 12.4035 Q -24.1322 14.5151 -23.1 17.256 Q -22.3551 19.2337 -22.3282 20.5253 Q -22.3117 21.3204 -22.6629 22.5499 Q -23.1 24.0799 -23.1 25.335 L -53.231 25.335 L -53.0611 24.0835 Q -52.9848 23.4664 -52.952 23.027 Q -52.7889 20.9165 -53.5062 20.34 Q -53.9474 19.9854 -55.3011 20.0949 Q -55.8582 20.14 -56.141 20.14 Q -56.8678 20.14 -58.088 19.824 Q -56.72 -1.5 -57.59 -3.809 L -59.3649 -8.8585 L -60.488 -12.177 L -60.452 -13.528 L -34.6782 -14.4064 Q -17.7053 -14.8069 -17.303 -14.099 Q -16.9786 -13.533 -17.2756 -11.1893 Q -17.4678 -9.67333 -17.3969 -9.21316 Q -17.2806 -8.45737 -16.532 -8.905 L -15.7964 -9.32047 Q -14.8052 -9.86092 -14.3838 -10.4508 Q -13.8448 -11.2053 -12.666 -13.908 L -12.6408 -13.9663 L -12.1375 -15.1314 L -11.796 -15.928 Q -10.4455 -15.664 -8.71463 -15.532 Q -5.25287 -15.268 -3.351 -15.928 Q -2.26721 -16.3037 0.138803 -17.7697 Q 2.37501 -19.1321 3.47884 -19.5926 Q 5.32977 -20.3647 6.655 -20.043 Q 9.0364 -19.4659 15.2545 -19.4796 Q 21.5978 -19.4936 24.433 -20.141 L 26.8278 -20.699 L 35.8358 -22.7453 Q 41.3062 -23.91 42.791 -23.91 Q 43.9937 -23.91 47.4529 -24.4104 L 48.269 -24.528 Q 52.5431 -25.1494 55.543 -25.26 Q 56.9399 -25.3108 58.3133 -25.2636 Q 59.311 -25.2294 60.488 -25.133 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 24"
        id: _qt_layer35
        transform: TransformGroup {
            id: _qt_layer35_transform_base_group
            TransformGroup {
                id: _qt_layer35_transform_group_0
                Translate { x: -437.943; y: -398.83 }
                Scale { id: _qt_layer35_transform_0_2}
                Translate { x: 337.943; y: 171.33 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer35_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer35_transform_animation
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
                                script:_qt_layer35_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer35_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 600 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer35_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer35_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer35_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer35_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 500 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer35_transform_base_group.deactivateOverride(_qt_layer35_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer35_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer35_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer35.opacity = 0
                    }
                    PauseAnimation { duration: 600 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer35
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer35
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 667 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer35_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer35_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer35.visible = false
                    }
                    PauseAnimation { duration: 600 }
                    ScriptAction {
                        script:_qt_layer35.visible = true
                    }
                    PauseAnimation { duration: 5200 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node36
            transform: TransformGroup {
                id: _qt_node36_transform_base_group
                Matrix4x4 { id: _qt_node36_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 438.153, 399.321)}
            }
            ShapePath {
                id: _qt_shapePath_36
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 30.262 15.848 Q 29.7214 17.328 29.4868 18.1461 Q 29.3424 18.6495 29.013 19.441 L 28.7111 20.1499 L 28.2804 21.1563 L 28.15 21.461 Q 26.9804 24.166 26.4455 24.921 Q 26.0274 25.5112 25.0435 26.0515 L 24.313 26.467 Q 23.5703 26.9149 23.4548 26.1593 Q 23.3845 25.6992 23.5748 24.1831 Q 23.8694 21.8375 23.547 21.271 Q 23.1486 20.5625 6.30625 20.9632 L -19.27 21.842 L -19.2586 21.3902 L -19.23 20.151 L -19.2251 19.9674 L -19.212 19.428 L -19.161 16.738 L -19.1537 16.4711 L -19.148 16.242 L -19.1455 16.0485 L -19.143 15.855 L -19.087 11.894 L -19.0491 6.9275 L -19.081 2.273 L -19.092 1.638 L -19.1193 0.513676 L -19.121 0.459 L -19.121 0.454 L -19.1272 0.25104 L -19.1418 -0.197307 L -19.161 -0.626 Q -19.206 -1.79997 -19.303 -2.792 Q -19.9411 -8.85791 -22.467 -9.91 Q -23.009 -10.1393 -23.0725 -12.915 Q -23.1446 -16.072 -23.906 -16.072 Q -24.0444 -16.072 -24.217 -16.194 Q -24.2863 -16.2425 -24.355 -16.298 Q -24.4657 -16.3973 -24.527 -16.465 Q -24.6246 -16.5693 -24.712 -16.673 Q -24.7157 -16.6761 -24.7167 -16.6809 Q -24.7173 -16.6833 -24.717 -16.685 Q -24.7893 -16.767 -24.849 -16.852 L -24.9192 -16.944 L -24.954 -16.99 Q -25.7096 -18.0225 -27.225 -20.882 L -28.0728 -22.4958 L -29.371 -24.942 L -29.451 -25.0874 L -29.612 -25.386 Q -30.0612 -26.1799 -30.372 -26.656 Q -30.4058 -26.7152 -30.4801 -26.8201 L -30.499 -26.847 L -30.6664 -27.0935 Q -30.7498 -27.2115 -30.827 -27.31 L -30.8912 -27.3896 Q -30.9552 -27.47 -30.987 -27.506 Q -31.035 -27.56 -31.096 -27.621 L 11.537 -27.621 Q 11.4348 -27.2879 11.2828 -27.009 Q 11.1192 -26.7091 10.895 -26.466 Q 9.575 -25.0222 8.759 -21.9417 Q 8.351 -20.4015 8.207 -19.15 Q 8.59075 -17.9 9.16663 -16.4573 Q 10.3184 -13.5717 11.279 -12.608 Q 12.493 -11.3845 14.4814 -9.96025 Q 16.5808 -8.45649 17.572 -8.178 Q 17.8979 -8.08794 18.7265 -7.97069 Q 20.0946 -7.7771 20.493 -7.53744 Q 21.1624 -7.13478 20.482 -6.256 Q 19.2611 -4.68375 19.1301 -1.99825 Q 18.9767 1.14866 20.827 2.984 Q 22.1019 4.24715 23.9808 4.98967 Q 24.8375 5.32822 25.1498 5.58992 Q 25.6418 6.00211 25.849 6.834 Q 26.0667 7.71043 25.8042 8.77945 Q 25.6163 9.54463 25.7467 9.97996 Q 25.9557 10.6775 26.999 11.645 Q 27.6148 12.2129 28.3382 12.5712 Q 28.7576 12.779 29.4772 13.0171 L 29.486 13.02 Q 30.4396 13.3287 30.6004 13.7039 Q 30.8376 14.2577 30.262 15.848 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 25"
        id: _qt_layer36
        transform: TransformGroup {
            id: _qt_layer36_transform_base_group
            TransformGroup {
                id: _qt_layer36_transform_group_0
                Translate { x: -427.442; y: -354.75 }
                Scale { id: _qt_layer36_transform_0_2}
                Translate { x: 327.442; y: 127.25 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer36_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer36_transform_animation
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
                                script:_qt_layer36_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer36_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 567 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer36_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer36_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer36_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer36_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 533 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer36_transform_base_group.deactivateOverride(_qt_layer36_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer36_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer36_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer36.opacity = 0
                    }
                    PauseAnimation { duration: 567 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer36
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3534 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer36
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 700 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer36_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer36_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer36.visible = false
                    }
                    PauseAnimation { duration: 567 }
                    ScriptAction {
                        script:_qt_layer36.visible = true
                    }
                    PauseAnimation { duration: 5233 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node37
            transform: TransformGroup {
                id: _qt_node37_transform_base_group
                Matrix4x4 { id: _qt_node37_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 427.711, 354.75)}
            }
            ShapePath {
                id: _qt_shapePath_37
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 27.118 3.113 Q 25.5626 4.41607 23.4364 5.42855 Q 22.3281 5.95634 21.9533 6.30775 Q 21.3778 6.84717 21.151 7.9 Q 20.9225 8.96646 21.5083 11.8076 Q 22.2038 15.1812 21.799 16.518 L -21.013 16.518 Q -21.7714 15.7631 -21.7826 14.4235 Q -21.7894 13.6118 -21.4185 11.7685 Q -21.013 9.7535 -21.013 8.762 Q -21.013 6.72443 -23.232 3.05188 Q -25.2856 -0.34695 -27.5 -2.443 Q -28.193 -3.09624 -28.3479 -4.13662 Q -28.4764 -4.99989 -28.238 -6.206 Q -28.0847 -6.99668 -27.6094 -8.5492 Q -27.1257 -10.1293 -27.0591 -10.7315 Q -26.9507 -11.7112 -27.5 -11.984 Q -28.6694 -12.559 -27.4131 -14.5955 Q -26.785 -15.6138 -25.923 -16.517 L 19.228 -16.517 Q 19.5054 -15.8829 19.5659 -15.2956 Q 19.6401 -14.5745 19.42 -13.742 Q 18.9387 -11.947 19.2148 -10.341 Q 19.3528 -9.538 19.587 -9.094 Q 21.244 -7.74 23.075 -5.895 L 23.082 -5.895 Q 25.8592 -3.10548 27.0429 -0.994625 Q 28.6349 1.8445 27.118 3.113 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 57"
        id: _qt_layer37
        transform: TransformGroup {
            id: _qt_layer37_transform_base_group
            TransformGroup {
                id: _qt_layer37_transform_group_0
                Translate { x: -413.573; y: -304.682 }
                Scale { id: _qt_layer37_transform_0_2}
                Translate { x: 313.562; y: 77.265 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer37_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer37_transform_animation
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
                                script:_qt_layer37_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer37_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 533 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer37_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer37_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3366 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer37_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer37_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 567 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer37_transform_base_group.deactivateOverride(_qt_layer37_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer37_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer37_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer37.opacity = 0
                    }
                    PauseAnimation { duration: 533 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer37
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer37
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 733 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer37_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer37_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer37.visible = false
                    }
                    PauseAnimation { duration: 533 }
                    ScriptAction {
                        script:_qt_layer37.visible = true
                    }
                    PauseAnimation { duration: 5267 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node38
            transform: TransformGroup {
                id: _qt_node38_transform_base_group
                Matrix4x4 { id: _qt_node38_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 424.651, 304.765)}
            }
            ShapePath {
                id: _qt_shapePath_38
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 12.71 -7.836 L 11.6376 -6.39048 Q 10.1456 -4.42117 9.78324 -3.61946 Q 9.1947 -2.31729 10.033 -1.685 Q 10.1292 -1.043 10.1774 -0.22425 Q 10.2736 1.41325 10.033 2.297 Q 9.87918 2.86101 8.83166 3.45064 Q 7.96037 3.94107 7.58146 4.35432 Q 6.93643 5.05781 6.784 6.203 Q 6.54712 8.00613 6.64187 10.4589 Q 6.68925 11.6853 6.784 12.551 Q 6.1735 13.8015 5.84737 15.2439 Q 5.19513 18.1286 6.617 19.088 Q 7.25755 19.5228 8.99479 20.5173 Q 13.9313 23.343 14.619 24.859 Q 15.0483 25.8087 17.4528 26.7834 Q 18.9402 27.3864 19.5132 27.7987 Q 20.4696 28.4869 20.539 29.475 Q 20.6224 30.6175 21.4499 32.173 Q 21.7526 32.742 21.88 33.035 L -22.922 33.035 L -22.922 14.634 Q -22.922 13.3378 -23.237 12.7955 Q -23.44 12.446 -24.0989 12.0836 Q -25.4253 11.354 -26.27 9.954 Q -26.4807 8.57755 -26.369 7.2125 Q -26.2783 6.10461 -25.949 4.795 Q -25.7896 4.16308 -25.4165 2.92542 Q -24.1372 -1.31783 -24.487 -4.183 Q -24.8912 -7.46415 -26.4645 -11.5696 Q -27.5468 -14.3941 -27.794 -16.012 Q -28.1559 -18.3564 -28.0111 -24.0401 L -27.794 -29.255 L -14.039 -29.573 L -9.454 -33.035 Q -8.88075 -31.2563 -7.97288 -29.3809 Q -6.15712 -25.6301 -4.484 -25.147 Q -3.09137 -24.7467 0.698772 -25.1733 Q 3.75329 -25.5171 4.302 -25.147 Q 4.50824 -25.0088 5.36122 -24.3114 Q 12.2509 -18.6786 15.381 -18.996 Q 17.7704 -19.2341 20.4456 -20.4834 Q 21.7833 -21.108 22.643 -21.685 L 28.374 -18.973 Q 27.5142 -18.7855 26.368 -18.3094 Q 24.0755 -17.3571 22.643 -15.914 Q 21.6864 -14.9511 18.5051 -12.7081 Q 13.8226 -9.40665 12.71 -7.836 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 13"
        id: _qt_layer38
        transform: TransformGroup {
            id: _qt_layer38_transform_base_group
            TransformGroup {
                id: _qt_layer38_transform_group_0
                Translate { x: -375.5; y: -481.12 }
                Scale { id: _qt_layer38_transform_0_2}
                Translate { x: 275.5; y: 253.62 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer38_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer38_transform_animation
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
                                script:_qt_layer38_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer38_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 500 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer38_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer38_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer38_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer38_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 600 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer38_transform_base_group.deactivateOverride(_qt_layer38_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer38_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer38_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer38.opacity = 0
                    }
                    PauseAnimation { duration: 500 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer38
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer38
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 767 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer38_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer38_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer38.visible = false
                    }
                    PauseAnimation { duration: 500 }
                    ScriptAction {
                        script:_qt_layer38.visible = true
                    }
                    PauseAnimation { duration: 5300 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node39
            transform: TransformGroup {
                id: _qt_node39_transform_base_group
                Matrix4x4 { id: _qt_node39_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 365.503, 481.488)}
            }
            ShapePath {
                id: _qt_shapePath_39
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 64.684 4.06 Q 63.6245 5.91566 63.0645 9.49763 Q 62.4105 13.6803 63.332 16.241 Q 60.1178 17.2611 55.7764 20.1551 Q 51.4037 23.0701 49.891 25.231 Q 48.8278 26.7498 42.8366 28.7419 Q 37.0327 30.6717 35.099 32.191 Q 33.6553 33.325 30.0664 35.3865 Q 24.0949 38.8164 24.655 40.311 Q 25.2091 41.7868 24.8751 45.315 Q 24.655 47.6396 24.655 49.011 Q 24.655 50.9295 26.3268 56.0214 Q 27.6939 60.1851 27.267 60.612 Q 27.1702 60.7087 27.0012 60.8986 Q 26.3896 61.5859 25.6765 61.6145 Q 24.5494 61.6597 21.755 60.612 Q 19.9721 59.944 17.0781 59.8128 Q 14.9867 59.7179 14.0427 59.3727 Q 12.5135 58.8136 11.312 57.132 Q 10.0553 55.372 7.31193 53.6189 Q 5.74305 52.6163 5.20998 52.0087 Q 4.351 51.0296 4.351 49.591 Q 4.351 48.2216 4.77582 46.4675 Q 5.09117 45.1655 5.03128 44.4917 Q 4.93668 43.4275 4.061 42.051 Q 3.31436 40.8783 1.71825 39.0536 Q -1.14289 35.7826 -2.61 32.481 Q -5.45518 26.0821 -7.542 22.331 L -7.98801 21.5051 Q -9.1162 19.3849 -10.0873 18.4281 Q -11.66 16.8783 -14.793 15.661 L -17.698 14.4456 Q -21.2607 12.8983 -22.7882 12.6366 Q -25.2466 12.2155 -26.976 13.63 Q -28.5272 14.8993 -29.5106 19.2224 Q -30.4157 23.202 -31.907 23.202 Q -34.0827 23.202 -35.0979 22.7305 Q -36.113 22.259 -40.028 19.43 Q -47.0605 14.3543 -48.441 6.09 Q -48.8097 3.87817 -49.6259 2.7723 Q -50.1608 2.04755 -51.7367 0.966541 Q -54.215 -0.733415 -55.982 -2.61 Q -57.5512 -4.27691 -59.8743 -7.1568 Q -63.5406 -11.7019 -65.844 -12.469 L -65.844 -16.241 L -47.1345 -14.355 L -28.425 -12.469 L -26.587 -62.352 L 2.901 -62.352 L 2.901 -38.861 Q 3.98875 -39.151 5.18525 -39.1147 Q 7.57825 -39.0423 8.122 -37.411 Q 8.45496 -36.4129 9.29256 -35.9402 Q 9.79895 -35.6544 11.0113 -35.3838 Q 12.8879 -34.9651 13.633 -34.22 Q 14.1362 -33.7171 14.5947 -32.8356 Q 15.1193 -31.8273 15.8181 -31.3462 Q 16.9553 -30.5632 19.725 -29.871 Q 22.3903 -29.2043 24.9473 -29.217 Q 27.1249 -29.2278 29.006 -28.13 Q 30.2258 -27.419 30.994 -26.6127 Q 31.3554 -26.2334 31.666 -26.2198 Q 31.9766 -26.2061 33.479 -26.5033 L 34.384 -26.681 Q 36.3068 -27.0536 38.6727 -27.8222 Q 41.0242 -28.5862 42.349 -28.42 Q 43.7996 -28.2388 46.2654 -28.1663 L 48.441 -28.13 Q 50.7236 -27.7022 53.8104 -26.808 L 55.194 -26.419 Q 56.4173 -26.101 57.142 -26.101 Q 57.4252 -26.101 57.9829 -26.1463 Q 59.3374 -26.2563 59.7787 -25.9001 Q 60.4961 -25.3211 60.333 -23.201 Q 60.302 -22.7656 60.2248 -22.1388 L 60.055 -20.88 L 59.9038 -19.8367 Q 58.3778 -9.39078 60.363 -7.83 L 61.693 -6.84066 Q 63.2586 -5.71415 63.7577 -5.02273 Q 64.5229 -3.96262 64.684 -2.03 Q 64.7795 -0.883461 65.0661 0.620906 Q 65.2954 1.82518 65.2646 2.3437 Q 65.218 3.126 64.684 4.06 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 17"
        id: _qt_layer39
        transform: TransformGroup {
            id: _qt_layer39_transform_base_group
            TransformGroup {
                id: _qt_layer39_transform_group_0
                Translate { x: -393.5; y: -433.444 }
                Scale { id: _qt_layer39_transform_0_2}
                Translate { x: 293.5; y: 205.944 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer39_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer39_transform_animation
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
                                script:_qt_layer39_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer39_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 467 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer39_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer39_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer39_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer39_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 633 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer39_transform_base_group.deactivateOverride(_qt_layer39_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer39_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer39_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer39.opacity = 0
                    }
                    PauseAnimation { duration: 467 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer39
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3534 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer39
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer39_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer39_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer39.visible = false
                    }
                    PauseAnimation { duration: 467 }
                    ScriptAction {
                        script:_qt_layer39.visible = true
                    }
                    PauseAnimation { duration: 5333 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node40
            transform: TransformGroup {
                id: _qt_node40_transform_base_group
                Matrix4x4 { id: _qt_node40_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 381.387, 433.646)}
            }
            ShapePath {
                id: _qt_shapePath_40
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 39.282 21.117 L 37.8907 20.7281 Q 34.8177 19.8427 32.548 19.419 L 30.3776 19.3825 Q 27.9179 19.31 26.471 19.13 Q 25.1502 18.9655 22.8072 19.724 Q 20.4477 20.4878 18.529 20.858 L 17.6278 21.0342 Q 16.1278 21.3296 15.8176 21.316 Q 15.5075 21.3024 15.1466 20.9254 Q 14.3804 20.1248 13.165 19.419 Q 11.2887 18.3292 9.11704 18.34 Q 6.56673 18.3526 3.908 17.691 Q 1.14747 17.0042 0.0136558 16.2264 Q -0.682925 15.7486 -1.20583 14.7471 Q -1.66308 13.8713 -2.165 13.372 Q -2.90804 12.632 -4.78051 12.2162 Q -5.98978 11.9476 -6.49484 11.664 Q -7.33023 11.1947 -7.662 10.204 Q -8.2045 8.584 -10.591 8.512 Q -11.7843 8.476 -12.869 8.764 L -12.869 -14.559 L -39.194 -14.559 L -40.645 -21.721 L -35.2905 -21.1625 L -29.936 -20.604 L 37.049 -18.134 L 37.046 -17.9397 L 37.043 -17.747 L 37.037 -17.5 L 37.031 -17.253 L 37.0237 -16.8584 L 37.0013 -15.6654 L 36.978 -14.569 L 36.9704 -14.2004 L 36.962 -13.849 L 36.9337 -12.6508 L 36.921 -12.161 L 36.885 -10.815 L 38.007 -7.504 L 39.78 -2.465 Q 40.646 -0.161 39.282 21.117 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 23"
        id: _qt_layer40
        transform: TransformGroup {
            id: _qt_layer40_transform_base_group
            TransformGroup {
                id: _qt_layer40_transform_group_0
                Translate { x: -384.987; y: -396.786 }
                Scale { id: _qt_layer40_transform_0_2}
                Translate { x: 284.987; y: 169.286 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer40_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer40_transform_animation
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
                                script:_qt_layer40_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer40_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 433 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer40_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer40_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3366 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer40_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer40_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 667 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer40_transform_base_group.deactivateOverride(_qt_layer40_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer40_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer40_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer40.opacity = 0
                    }
                    PauseAnimation { duration: 433 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer40
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer40
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 833 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer40_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer40_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer40.visible = false
                    }
                    PauseAnimation { duration: 433 }
                    ScriptAction {
                        script:_qt_layer40.visible = true
                    }
                    PauseAnimation { duration: 5367 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node41
            transform: TransformGroup {
                id: _qt_node41_transform_base_group
                Matrix4x4 { id: _qt_node41_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 384.997, 396.786)}
            }
            ShapePath {
                id: _qt_shapePath_41
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 33.479 14.349 L 33.4523 16.3691 L 33.421 18.294 L -0.0575 17.061 L -33.536 15.828 L -33.536 -18.294 L 25.296 -18.294 Q 26.8158 -15.4511 27.579 -14.418 L 27.6143 -14.3719 L 27.684 -14.28 Q 27.7181 -14.2325 27.7481 -14.1943 Q 27.7838 -14.1487 27.816 -14.113 Q 27.817 -14.108 27.823 -14.102 L 27.9177 -13.994 L 28.008 -13.895 Q 28.0707 -13.8269 28.181 -13.728 L 28.319 -13.625 Q 28.4923 -13.504 28.633 -13.504 Q 29.3975 -13.504 29.4699 -10.359 Q 29.5336 -7.59393 30.078 -7.366 Q 32.6199 -6.31806 33.259 -0.276 L 33.3401 0.694375 L 33.404 1.88 L 33.4215 2.34242 L 33.444 2.957 L 33.444 2.962 L 33.461 3.62992 L 33.473 4.135 L 33.484 4.768 L 33.516 9.40287 L 33.479 14.349 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 21"
        id: _qt_layer41
        transform: TransformGroup {
            id: _qt_layer41_transform_base_group
            TransformGroup {
                id: _qt_layer41_transform_group_0
                Translate { x: -372.059; y: -360.446 }
                Scale { id: _qt_layer41_transform_0_2}
                Translate { x: 272.059; y: 132.946 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer41_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer41_transform_animation
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
                                script:_qt_layer41_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer41_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 400 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer41_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer41_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer41_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer41_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 700 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer41_transform_base_group.deactivateOverride(_qt_layer41_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer41_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer41_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer41.opacity = 0
                    }
                    PauseAnimation { duration: 400 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer41
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer41
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 867 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer41_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer41_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer41.visible = false
                    }
                    PauseAnimation { duration: 400 }
                    ScriptAction {
                        script:_qt_layer41.visible = true
                    }
                    PauseAnimation { duration: 5400 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node42
            transform: TransformGroup {
                id: _qt_node42_transform_base_group
                Matrix4x4 { id: _qt_node42_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 372.059, 360.446)}
            }
            ShapePath {
                id: _qt_shapePath_42
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M -20.711 17.613 L -20.711 7.267 L -38.035 5.346 L -36.7245 -6.1335 L -35.414 -17.613 L 12.638 -15.728 Q 13.6368 -14.8658 14.8038 -14.0516 Q 17.1378 -12.4234 17.979 -12.664 Q 19.5968 -13.1271 21.9445 -13.0991 Q 25.1654 -13.0607 26.924 -11.831 Q 26.6865 -10.6249 26.8155 -9.7605 Q 26.971 -8.71893 27.664 -8.065 Q 29.8773 -5.96839 31.9301 -2.56763 Q 34.148 1.10659 34.148 3.145 Q 34.148 4.13725 33.7425 6.15371 Q 33.3716 7.99796 33.3784 8.81003 Q 33.3897 10.1502 34.148 10.905 L 34.163 10.9199 Q 34.2238 10.9805 34.259 11.02 L 34.3175 11.0906 Q 34.3849 11.1703 34.419 11.216 Q 34.5705 11.4082 34.75 11.676 L 34.7605 11.691 Q 34.8386 11.802 34.876 11.865 Q 35.2215 12.3942 35.638 13.13 L 35.7762 13.3822 L 35.882 13.573 L 37.177 15.9936 L 38.035 17.613 L -20.711 17.613 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 22"
        id: _qt_layer42
        transform: TransformGroup {
            id: _qt_layer42_transform_base_group
            TransformGroup {
                id: _qt_layer42_transform_group_0
                Translate { x: -369.021; y: -327.663 }
                Scale { id: _qt_layer42_transform_0_2}
                Translate { x: 269.021; y: 100.163 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer42_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer42_transform_animation
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
                                script:_qt_layer42_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer42_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer42_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer42_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3367 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer42_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer42_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 733 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer42_transform_base_group.deactivateOverride(_qt_layer42_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer42_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer42_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer42.opacity = 0
                    }
                    PauseAnimation { duration: 367 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer42
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3534 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer42
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 900 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer42_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer42_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer42.visible = false
                    }
                    PauseAnimation { duration: 367 }
                    ScriptAction {
                        script:_qt_layer42.visible = true
                    }
                    PauseAnimation { duration: 5433 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node43
            transform: TransformGroup {
                id: _qt_node43_transform_base_group
                Matrix4x4 { id: _qt_node43_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 369.021, 327.663)}
            }
            ShapePath {
                id: _qt_shapePath_43
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 32.354 10.135 Q 31.4915 11.0375 30.863 12.0548 Q 29.606 14.0892 30.776 14.663 Q 31.3259 14.9359 31.2169 15.9156 Q 31.1499 16.5177 30.6648 18.0972 Q 30.1887 19.6469 30.035 20.436 Q 28.2749 19.2087 25.052 19.1699 Q 22.701 19.1415 21.083 19.604 Q 20.2411 19.8446 17.9049 18.2189 Q 16.7367 17.406 15.737 16.545 L -8.308 15.604 L -32.353 14.663 L -29.51 -10.197 L -28.337 -20.436 L -28.332 -20.436 L 0.485 -19.1935 L 29.302 -17.951 Q 28.9694 -16.6492 28.878 -15.5471 Q 28.7655 -14.1896 28.979 -12.82 Q 29.8303 -11.428 31.1674 -10.7024 Q 31.8316 -10.342 32.0364 -9.99428 Q 32.354 -9.45499 32.354 -8.166 L 32.354 10.135 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 15"
        id: _qt_layer43
        transform: TransformGroup {
            id: _qt_layer43_transform_base_group
            TransformGroup {
                id: _qt_layer43_transform_group_0
                Translate { x: -370.302; y: -291.295 }
                Scale { id: _qt_layer43_transform_0_2}
                Translate { x: 270.302; y: 63.795 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer43_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer43_transform_animation
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
                                script:_qt_layer43_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer43_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer43_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer43_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3366 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer43_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer43_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 767 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer43_transform_base_group.deactivateOverride(_qt_layer43_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer43_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer43_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer43.opacity = 0
                    }
                    PauseAnimation { duration: 333 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer43
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3533 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer43
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 933 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer43_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer43_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer43.visible = false
                    }
                    PauseAnimation { duration: 333 }
                    ScriptAction {
                        script:_qt_layer43.visible = true
                    }
                    PauseAnimation { duration: 5467 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node44
            transform: TransformGroup {
                id: _qt_node44_transform_base_group
                Matrix4x4 { id: _qt_node44_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 370.48, 291.295)}
            }
            ShapePath {
                id: _qt_shapePath_44
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 27.834 18.012 L -29.762 15.523 L -25.932 -18.013 Q -12.9424 -16.8881 -0.358125 -16.3574 Q 13.0681 -15.7912 25.974 -15.904 L 25.7554 -10.709 Q 25.6096 -5.047 25.974 -2.712 Q 26.2233 -1.101 27.3139 1.71113 Q 28.8998 5.80026 29.307 9.069 Q 29.6591 11.9225 28.371 16.1476 Q 27.9947 17.3817 27.834 18.012 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 12"
        id: _qt_layer44
        transform: TransformGroup {
            id: _qt_layer44_transform_base_group
            TransformGroup {
                id: _qt_layer44_transform_group_0
                Translate { x: -308.742; y: -438.291 }
                Scale { id: _qt_layer44_transform_0_2}
                Translate { x: 208.742; y: 210.791 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer44_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer44_transform_animation
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
                                script:_qt_layer44_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer44_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer44_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer44_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer44_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer44_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 800 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer44_transform_base_group.deactivateOverride(_qt_layer44_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer44_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer44_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer44.opacity = 0
                    }
                    PauseAnimation { duration: 333 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer44
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer44
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 967 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer44_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer44_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer44.visible = false
                    }
                    PauseAnimation { duration: 333 }
                    ScriptAction {
                        script:_qt_layer44.visible = true
                    }
                    PauseAnimation { duration: 5467 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node45
            transform: TransformGroup {
                id: _qt_node45_transform_base_group
                Matrix4x4 { id: _qt_node45_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 308.742, 438.364)}
            }
            ShapePath {
                id: _qt_shapePath_45
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 32.926 -19.302 L 29.848 -19.302 L 28.018 30.388 L -9.228 26.634 L -9.228 30.388 Q -20.7068 26.5617 -22.799 28.655 Q -23.4269 29.2832 -23.916 30.3378 Q -24.3606 31.2963 -24.9429 31.5612 Q -25.839 31.969 -28.285 31.833 Q -29.1109 31.7879 -30.3589 31.833 L -32.927 31.977 L -22.192 -32.122 L 31.477 -26.49 L 32.926 -19.302 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 9"
        id: _qt_layer45
        transform: TransformGroup {
            id: _qt_layer45_transform_base_group
            TransformGroup {
                id: _qt_layer45_transform_group_0
                Translate { x: -318.828; y: -386.92 }
                Scale { id: _qt_layer45_transform_0_2}
                Translate { x: 218.828; y: 159.42 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer45_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer45_transform_animation
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
                                script:_qt_layer45_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer45_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 300 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer45_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer45_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer45_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer45_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 833 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer45_transform_base_group.deactivateOverride(_qt_layer45_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer45_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer45_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer45.opacity = 0
                    }
                    PauseAnimation { duration: 300 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer45
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer45
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1000 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer45_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer45_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer45.visible = false
                    }
                    PauseAnimation { duration: 300 }
                    ScriptAction {
                        script:_qt_layer45.visible = true
                    }
                    PauseAnimation { duration: 5500 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node46
            transform: TransformGroup {
                id: _qt_node46_transform_base_group
                Matrix4x4 { id: _qt_node46_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 318.828, 386.92)}
            }
            ShapePath {
                id: _qt_shapePath_46
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 32.201 -18.944 L 32.201 25.65 L 21.504 24.53 L -32.201 18.909 L -28.459 -3.3705 L -24.717 -25.65 L 14.865 -20.87 L 32.201 -18.944 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 14"
        id: _qt_layer46
        transform: TransformGroup {
            id: _qt_layer46_transform_base_group
            TransformGroup {
                id: _qt_layer46_transform_group_0
                Translate { x: -306.084; y: -339.077 }
                Scale { id: _qt_layer46_transform_0_2}
                Translate { x: 206.084; y: 111.577 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer46_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer46_transform_animation
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
                                script:_qt_layer46_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer46_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 267 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer46_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer46_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer46_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer46_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.892, 0, 0.346, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 867 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer46_transform_base_group.deactivateOverride(_qt_layer46_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer46_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer46_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer46.opacity = 0
                    }
                    PauseAnimation { duration: 267 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer46
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer46
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1033 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer46_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer46_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer46.visible = false
                    }
                    PauseAnimation { duration: 267 }
                    ScriptAction {
                        script:_qt_layer46.visible = true
                    }
                    PauseAnimation { duration: 5533 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node47
            transform: TransformGroup {
                id: _qt_node47_transform_base_group
                Matrix4x4 { id: _qt_node47_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 306.084, 339.077)}
            }
            ShapePath {
                id: _qt_shapePath_47
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 32.966 -21.495 L 30.124 3.496 L 27.501 26.537 L -12.071 21.755 L -22.5185 20.4945 L -32.966 19.234 L -30.268 5.388 L -23.723 -26.537 L 4.6215 -24.016 L 32.966 -21.495 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 10"
        id: _qt_layer47
        transform: TransformGroup {
            id: _qt_layer47_transform_base_group
            TransformGroup {
                id: _qt_layer47_transform_group_0
                Translate { x: -296.882; y: -287.963 }
                Scale { id: _qt_layer47_transform_0_2}
                Translate { x: 196.882; y: 60.463 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer47_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer47_transform_animation
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
                                script:_qt_layer47_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer47_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 233 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer47_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer47_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer47_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer47_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 900 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer47_transform_base_group.deactivateOverride(_qt_layer47_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer47_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer47_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer47.opacity = 0
                    }
                    PauseAnimation { duration: 233 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer47
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer47
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1067 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer47_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer47_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer47.visible = false
                    }
                    PauseAnimation { duration: 233 }
                    ScriptAction {
                        script:_qt_layer47.visible = true
                    }
                    PauseAnimation { duration: 4767 }
                    ScriptAction {
                        script:_qt_layer47.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node48
            transform: TransformGroup {
                id: _qt_node48_transform_base_group
                Matrix4x4 { id: _qt_node48_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 296.56, 288.375)}
            }
            ShapePath {
                id: _qt_shapePath_48
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 47.535 -15.188 L 43.695 18.49 L 43.689 18.49 L 42.513 28.786 L 14.0925 26.264 L -14.328 23.742 Q -15.9317 23.6011 -18.7334 25.9123 Q -20.3684 27.261 -21.1713 27.5957 Q -22.4982 28.149 -23.596 27.303 Q -25.4164 25.8967 -27.7607 26.0811 Q -29.2532 26.1986 -30.159 25.86 Q -31.7265 25.2739 -32.1855 22.7576 Q -32.475 21.1705 -32.475 16.626 Q -32.475 14.2166 -34.1111 13.4483 Q -35.0684 12.9989 -37.4182 13.0134 Q -38.9042 13.0225 -39.426 12.874 Q -40.3741 12.6042 -39.9409 11.0876 Q -39.6757 10.1587 -38.4224 7.66712 Q -37.259 5.35413 -36.82 4.217 Q -36.0165 2.1349 -36.8792 0.663212 Q -37.39 -0.208069 -39.0812 -1.43226 Q -39.7208 -1.89522 -40.006 -2.132 Q -40.8362 -2.82167 -42.0819 -5.35063 Q -43.4011 -8.0289 -44.263 -8.481 Q -45.2949 -9.02162 -45.9391 -11.7109 Q -46.2612 -13.0555 -46.377 -14.292 Q -46.6668 -15.3643 -46.8116 -16.725 Q -47.1014 -19.4465 -46.377 -20.889 Q -45.6932 -22.2523 -44.9655 -25.7351 Q -44.59 -27.5323 -44.275 -29.5343 L -44.263 -29.61 Q -23.305 -24.9019 -2.33488 -21.4683 Q 23.1064 -17.3025 47.535 -15.188 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 8"
        id: _qt_layer48
        transform: TransformGroup {
            id: _qt_layer48_transform_base_group
            TransformGroup {
                id: _qt_layer48_transform_group_0
                Translate { x: -267.769; y: -372.746 }
                Scale { id: _qt_layer48_transform_0_2}
                Translate { x: 167.769; y: 145.246 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer48_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer48_transform_animation
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
                                script:_qt_layer48_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer48_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 200 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer48_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer48_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer48_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer48_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 933 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer48_transform_base_group.deactivateOverride(_qt_layer48_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer48_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer48_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer48.opacity = 0
                    }
                    PauseAnimation { duration: 200 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer48
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer48
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1100 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer48_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer48_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer48.visible = false
                    }
                    PauseAnimation { duration: 200 }
                    ScriptAction {
                        script:_qt_layer48.visible = true
                    }
                    PauseAnimation { duration: 4800 }
                    ScriptAction {
                        script:_qt_layer48.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node49
            transform: TransformGroup {
                id: _qt_node49_transform_base_group
                Matrix4x4 { id: _qt_node49_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 267.769, 372.746)}
            }
            ShapePath {
                id: _qt_shapePath_49
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 25.895 -11.617 L 18.429 33.022 L -25.895 28.36 L -15.523 -33.021 L 7.736 -28.007 L 6.39 -21.0745 L 5.044 -14.142 L 25.895 -11.617 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 7"
        id: _qt_layer49
        transform: TransformGroup {
            id: _qt_layer49_transform_base_group
            TransformGroup {
                id: _qt_layer49_transform_group_0
                Translate { x: -254.209; y: -436.16 }
                Scale { id: _qt_layer49_transform_0_2}
                Translate { x: 154.209; y: 208.66 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer49_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer49_transform_animation
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
                                script:_qt_layer49_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer49_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 167 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer49_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer49_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer49_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer49_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 967 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer49_transform_base_group.deactivateOverride(_qt_layer49_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer49_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer49_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer49.opacity = 0
                    }
                    PauseAnimation { duration: 167 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer49
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer49
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1133 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer49_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer49_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer49.visible = false
                    }
                    PauseAnimation { duration: 167 }
                    ScriptAction {
                        script:_qt_layer49.visible = true
                    }
                    PauseAnimation { duration: 4833 }
                    ScriptAction {
                        script:_qt_layer49.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node50
            transform: TransformGroup {
                id: _qt_node50_transform_base_group
                Matrix4x4 { id: _qt_node50_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 254.209, 436.572)}
            }
            ShapePath {
                id: _qt_shapePath_50
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 31.916 -30.362 L 26.549 1.714 L 21.182 33.79 L 20.0793 33.8645 Q 5.658 34.849 0.419 31.911 Q -2.89371 30.0536 -9.90073 25.3924 Q -17.5799 20.2841 -20.079 19.189 Q -23.2088 17.8176 -27.6213 14.4185 Q -31.6495 11.3154 -31.916 10.515 Q -29.2704 9.06938 -29.1736 8.00912 Q -29.1252 7.479 -29.606 7.238 Q -29.2205 6.3225 -28.5946 5.14175 Q -27.3429 2.78025 -26.141 1.454 Q -25.0625 0.266051 -24.855 -0.247456 Q -24.698 -0.636123 -24.8427 -1.42474 Q -24.947 -1.99353 -24.987 -2.397 Q -25.1057 -3.6045 -23.9032 -4.4715 Q -23.302 -4.905 -22.677 -5.097 L -23.832 -8.951 L -24.987 -12.805 L -22.677 -26.302 L -19.9835 -24.952 L -17.29 -23.602 L -13.826 -27.072 L -12.48 -35.029 L 9.718 -32.6955 L 31.916 -30.362 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 3"
        id: _qt_layer50
        transform: TransformGroup {
            id: _qt_layer50_transform_base_group
            TransformGroup {
                id: _qt_layer50_transform_group_0
                Translate { x: -221.922; y: -365.5 }
                Scale { id: _qt_layer50_transform_0_2}
                Translate { x: 121.922; y: 138 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer50_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer50_transform_animation
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
                                script:_qt_layer50_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer50_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 133 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer50_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer50_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer50_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer50_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1000 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer50_transform_base_group.deactivateOverride(_qt_layer50_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer50_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer50_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer50.opacity = 0
                    }
                    PauseAnimation { duration: 133 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer50
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer50
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1167 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer50_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer50_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer50.visible = false
                    }
                    PauseAnimation { duration: 133 }
                    ScriptAction {
                        script:_qt_layer50.visible = true
                    }
                    PauseAnimation { duration: 4867 }
                    ScriptAction {
                        script:_qt_layer50.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node51
            transform: TransformGroup {
                id: _qt_node51_transform_base_group
                Matrix4x4 { id: _qt_node51_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 221.922, 375.88)}
            }
            ShapePath {
                id: _qt_shapePath_51
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 29.924 -36.257 L 24.73 -5.6075 L 19.536 25.042 L 18.191 32.98 L 14.726 36.441 L 12.0325 35.094 L 9.339 33.747 L 7.029 47.213 L -29.924 -10.098 L -21.067 -47.212 L 4.316 -41.76 L 17.12 -39.0085 L 29.924 -36.257 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 11"
        id: _qt_layer51
        transform: TransformGroup {
            id: _qt_layer51_transform_base_group
            TransformGroup {
                id: _qt_layer51_transform_group_0
                Translate { x: -254.255; y: -316 }
                Scale { id: _qt_layer51_transform_0_2}
                Translate { x: 154.255; y: 88.5 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer51_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer51_transform_animation
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
                                script:_qt_layer51_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer51_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 100 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer51_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer51_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer51_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer51_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1033 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer51_transform_base_group.deactivateOverride(_qt_layer51_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer51_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer51_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer51.opacity = 0
                    }
                    PauseAnimation { duration: 100 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer51
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer51
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1200 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer51_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer51_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer51.visible = false
                    }
                    PauseAnimation { duration: 100 }
                    ScriptAction {
                        script:_qt_layer51.visible = true
                    }
                    PauseAnimation { duration: 4900 }
                    ScriptAction {
                        script:_qt_layer51.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node52
            transform: TransformGroup {
                id: _qt_node52_transform_base_group
                Matrix4x4 { id: _qt_node52_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 254.255, 300.68)}
            }
            ShapePath {
                id: _qt_shapePath_52
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 27.659 11.562 L 24.3955 27.571 L 21.132 43.58 L -2.111 38.558 L -27.659 33.038 Q -27.054 31.8158 -26.413 30.3766 Q -25.131 27.4984 -24.951 26.414 Q -24.8684 25.9159 -24.8924 25.3578 Q -24.9486 24.052 -23.511 20.339 Q -22.6874 18.2118 -20.8758 15.5668 Q -19.839 14.0531 -19.6501 13.4265 Q -19.3581 12.4581 -20.055 11.562 Q -20.6164 10.8421 -22.4024 9.53779 Q -24.1588 8.2551 -24.5102 7.65078 Q -25.0864 6.65983 -23.609 5.875 Q -20.1328 4.02728 -16.4667 0.19775 Q -12.0058 -4.46209 -13.718 -6.564 Q -14.9939 -8.12809 -16.99 -9.544 L -16.99 -9.55 Q -15.7085 -19.93 -13.263 -25.657 Q -11.7186 -29.2726 -10.3379 -36.7879 Q -9.6475 -40.5455 -9.266 -43.58 L -2.111 -41.92 L -2.12003 -41.8621 Q -3.13012 -35.3554 -4.214 -33.178 Q -4.934 -31.7318 -4.646 -29.0033 Q -4.502 -27.639 -4.214 -26.564 Q -4.09875 -25.3245 -3.77825 -23.9765 Q -3.13725 -21.2805 -2.111 -20.738 Q -1.25391 -20.2849 0.0578641 -17.6007 Q 1.29698 -15.0652 2.123 -14.374 Q 2.4069 -14.1363 3.04371 -13.6715 Q 4.72489 -12.4446 5.23252 -11.5716 Q 6.08991 -10.0971 5.291 -8.011 Q 4.85435 -6.87055 3.6971 -4.55116 Q 2.45105 -2.05379 2.18728 -1.12279 Q 1.75659 0.397338 2.699 0.668 Q 3.21789 0.81677 4.69563 0.807479 Q 7.03262 0.792787 7.98474 1.24323 Q 9.612 2.01308 9.612 4.428 Q 9.612 8.9845 9.9 10.5756 Q 10.3566 13.0982 11.916 13.685 Q 12.8167 14.0243 14.3008 13.907 Q 16.6319 13.7226 18.442 15.132 Q 19.5336 15.9802 20.8534 15.4256 Q 21.6519 15.09 23.2782 13.7378 Q 26.0645 11.4211 27.659 11.562 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 5"
        id: _qt_layer52
        transform: TransformGroup {
            id: _qt_layer52_transform_base_group
            TransformGroup {
                id: _qt_layer52_transform_group_0
                Translate { x: -213.674; y: -268.083 }
                Scale { id: _qt_layer52_transform_0_2}
                Translate { x: 113.674; y: 40.583 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer52_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer52_transform_animation
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
                                script:_qt_layer52_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer52_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 67 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer52_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer52_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer52_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer52_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1067 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer52_transform_base_group.deactivateOverride(_qt_layer52_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer52_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer52_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer52.opacity = 0
                    }
                    PauseAnimation { duration: 67 }
                    PropertyAnimation {
                        duration: 466
                        target: _qt_layer52
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 534
                        target: _qt_layer52
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1233 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer52_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer52_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer52.visible = false
                    }
                    PauseAnimation { duration: 67 }
                    ScriptAction {
                        script:_qt_layer52.visible = true
                    }
                    PauseAnimation { duration: 4933 }
                    ScriptAction {
                        script:_qt_layer52.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node53
            transform: TransformGroup {
                id: _qt_node53_transform_base_group
                Matrix4x4 { id: _qt_node53_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 213.215, 268.083)}
            }
            ShapePath {
                id: _qt_shapePath_53
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 31.411 -11.125 Q 31.0282 -8.11725 30.3356 -4.39262 Q 28.9504 3.05662 27.401 6.641 Q 24.9435 12.3282 23.663 22.604 Q 21.7033 21.2228 19.5753 20.3209 Q 17.3899 19.3946 15.678 19.255 Q 14.362 19.1461 11.697 19.4368 Q 6.83985 19.9666 3.255 19.255 Q 0.365182 18.6812 -3.21085 18.2876 Q -6.13473 17.9658 -7.723 17.536 Q -8.7303 17.2633 -9.8386 16.8134 Q -11.5911 16.102 -14.945 16.102 L -17.0625 16.127 Q -20.6302 16.1915 -21.8293 15.9077 Q -23.7348 15.4567 -23.901 13.807 Q -24.1769 11.0608 -25.5516 8.33387 Q -27.2352 4.99445 -29.378 4.852 Q -28.984 2.89015 -28.523 -1.961 Q -28.2124 -5.19606 -29.75 -10.9996 Q -30.7909 -14.9281 -30.256 -15.724 Q -29.352 -17.0702 -24.9116 -15.7233 Q -20.3137 -14.3285 -16.967 -11.422 Q -13.6943 -8.5798 -11.695 -9.19987 Q -9.85303 -9.77116 -10.323 -12.569 Q -10.6836 -14.719 -10.8279 -18.8765 L -10.9 -22.604 Q 5.231 -17.266 31.411 -11.125 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 4"
        id: _qt_layer53
        transform: TransformGroup {
            id: _qt_layer53_transform_base_group
            TransformGroup {
                id: _qt_layer53_transform_group_0
                Translate { x: -203.035; y: -303.472 }
                Scale { id: _qt_layer53_transform_0_2}
                Translate { x: 103.035; y: 75.972 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer53_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer53_transform_animation
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
                                script:_qt_layer53_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer53_transform_0_2.yScale = 0
                            }
                        }
                        PauseAnimation { duration: 33 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer53_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 634
                                target: _qt_layer53_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3333 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer53_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer53_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1100 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer53_transform_base_group.deactivateOverride(_qt_layer53_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer53_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer53_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer53.opacity = 0
                    }
                    PauseAnimation { duration: 33 }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer53
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer53
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1267 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer53_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer53_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer53.visible = false
                    }
                    PauseAnimation { duration: 33 }
                    ScriptAction {
                        script:_qt_layer53.visible = true
                    }
                    PauseAnimation { duration: 4967 }
                    ScriptAction {
                        script:_qt_layer53.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node54
            transform: TransformGroup {
                id: _qt_node54_transform_base_group
                Matrix4x4 { id: _qt_node54_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 204.001, 303.472)}
            }
            ShapePath {
                id: _qt_shapePath_54
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 29.786 8.757 Q 30.486 9.6514 30.1928 10.6178 Q 30.003 11.2432 28.9616 12.7538 Q 27.1414 15.3941 26.314 17.518 Q 24.8691 21.2244 24.9257 22.5275 Q 24.9499 23.0842 24.867 23.581 Q 24.6864 24.6641 23.3986 27.5374 Q 22.7547 28.974 22.147 30.194 L -3.293 24.736 L -38.426 17.194 Q -38.6775 14.2145 -38.3083 12.3654 Q -37.9212 10.4275 -36.476 7.701 Q -35.2874 5.46031 -30.3938 -6.37394 Q -24.292 -21.1299 -22.876 -23.484 Q -21.8401 -25.2107 -21.2108 -26.8499 Q -20.6186 -28.3921 -20.261 -30.194 Q -18.1144 -30.0509 -16.4282 -26.6868 Q -15.052 -23.9408 -14.775 -21.174 Q -14.6082 -19.5131 -12.7007 -19.0592 Q -11.5004 -18.7736 -7.92865 -18.8387 L -5.805 -18.864 Q -2.44524 -18.864 -0.690099 -18.1482 Q 0.420042 -17.6955 1.429 -17.421 Q 3.02016 -16.9881 5.94944 -16.6635 Q 9.53087 -16.2666 12.425 -15.689 Q 16.0165 -14.9722 20.8822 -15.5057 Q 23.5498 -15.7983 24.867 -15.689 Q 26.5822 -15.5479 28.7708 -14.6152 Q 30.9027 -13.7068 32.865 -12.316 L 32.865 -12.31 Q 34.873 -10.8955 36.152 -9.336 Q 37.8722 -7.2379 33.3908 -2.586 Q 29.7063 1.23853 26.215 3.081 Q 24.731 3.86486 25.3097 4.85371 Q 25.6627 5.45677 27.4271 6.7365 Q 29.2219 8.03824 29.786 8.757 " }
            }
        }
    }
    LayerItem {
        objectName: "Map_01 Outlines 2"
        id: _qt_layer54
        transform: TransformGroup {
            id: _qt_layer54_transform_base_group
            TransformGroup {
                id: _qt_layer54_transform_group_0
                Translate { x: -183.413; y: -383.96 }
                Scale { id: _qt_layer54_transform_0_2}
                Translate { x: 83.413; y: 156.46 }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer54_transform_animation.restart() } }
        ParallelAnimation {
            id:_qt_layer54_transform_animation
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
                                script:_qt_layer54_transform_0_2.xScale = 0
                            }
                            ScriptAction {
                                script:_qt_layer54_transform_0_2.yScale = 0
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer54_transform_0_2
                                property: "xScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 633
                                target: _qt_layer54_transform_0_2
                                property: "yScale"
                                to: 1
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.423, 0, 0, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 3334 }
                        ParallelAnimation {
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer54_transform_0_2
                                property: "xScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                            PropertyAnimation {
                                duration: 700
                                target: _qt_layer54_transform_0_2
                                property: "yScale"
                                to: 0
                                easing.type: Easing.BezierSpline
                                easing.bezierCurve: [ 0.895, 0, 0.349, 1, 1, 1 ]
                            }
                        }
                        PauseAnimation { duration: 1133 }
                    }
                }
                ScriptAction {
                    script: {
                        _qt_layer54_transform_base_group.deactivateOverride(_qt_layer54_transform_group_0)
                    }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer54_opacity_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer54_opacity_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer54.opacity = 0
                    }
                    PropertyAnimation {
                        duration: 467
                        target: _qt_layer54
                        property: "opacity"
                        to: 1
                    }
                    PauseAnimation { duration: 3500 }
                    PropertyAnimation {
                        duration: 533
                        target: _qt_layer54
                        property: "opacity"
                        to: 0
                    }
                    PauseAnimation { duration: 1300 }
                }
            }
        }
        Connections { target: _qt_node0.animations; function onRestart() {_qt_layer54_visible_animation.restart() } }
        ParallelAnimation {
            id: _qt_layer54_visible_animation
            loops: _qt_node0.animations.loops
            paused: _qt_node0.animations.paused
            running: true
            onLoopsChanged: { if (running) { restart() } }
            SequentialAnimation {
                SequentialAnimation {
                    loops: 1
                    ScriptAction {
                        script:_qt_layer54.visible = true
                    }
                    PauseAnimation { duration: 5000 }
                    ScriptAction {
                        script:_qt_layer54.visible = false
                    }
                    PauseAnimation { duration: 800 }
                }
            }
        }
        Shape {
            preferredRendererType: Shape.CurveRenderer
            id: _qt_node55
            transform: TransformGroup {
                id: _qt_node55_transform_base_group
                Matrix4x4 { id: _qt_node55_transform_base; matrix: PlanarTransform.fromAffineMatrix(1, 0, 0, 1, 193.538, 383.96)}
            }
            ShapePath {
                id: _qt_shapePath_55
                strokeColor: "#ffffffff"
                strokeWidth: 1.5
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                miterLimit: 0
                fillColor: "#ff185aba"
                fillRule: ShapePath.WindingFill
                pathHints: ShapePath.PathQuadratic | ShapePath.PathNonIntersecting | ShapePath.PathNonOverlappingControlPointTriangles
                PathSvg { path: "M 35.225 49.939 Q 35.265 50.3431 35.3694 50.9129 Q 35.5142 51.7031 35.357 52.0925 Q 35.1491 52.607 34.069 53.796 Q 32.8652 55.1254 31.6117 57.4911 Q 30.985 58.674 30.599 59.591 Q 31.0805 59.8327 31.0321 60.3643 Q 30.9354 61.4272 28.286 62.876 Q 28.0939 62.2976 21.1669 61.2463 Q 14.7281 60.269 12.964 60.269 Q 11.8796 60.269 8.98837 58.8205 Q 7.54275 58.0962 6.314 57.372 Q 6.603 55.5612 6.45837 53.5696 Q 6.16913 49.5864 4.001 48.682 Q 1.73232 47.7344 0.0867444 46.3385 Q -0.940146 45.4675 -2.75233 43.3897 L -2.937 43.178 Q -3.88165 42.0966 -6.16125 40.5561 Q -8.79665 38.7752 -9.876 37.385 Q -10.7774 36.2239 -15.5971 35.1579 Q -18.6338 34.4862 -19.8213 33.9914 Q -21.8051 33.1647 -22.019 31.88 Q -22.3004 30.1882 -20.464 27.3214 Q -18.675 24.5283 -19.417 23.191 Q -20.1475 21.8731 -21.7931 16.9761 Q -23.4736 11.9755 -24.331 10.225 Q -24.9356 8.98962 -24.9419 8.2913 Q -24.9458 7.85598 -24.6236 7.24117 Q -23.912 5.88305 -24.331 3.783 Q -24.7308 1.78165 -25.6291 1.11504 Q -26.1332 0.740935 -26.3562 0.233286 Q -26.7246 -0.605377 -26.933 -2.59 Q -27.1728 -4.86878 -26.0472 -8.00885 Q -25.3487 -9.95764 -25.3873 -10.8936 Q -25.4494 -12.4003 -26.933 -13.887 Q -28.943 -15.9002 -29.1595 -18.9236 Q -29.3469 -21.5387 -30.692 -23.156 Q -31.2794 -23.8623 -32.4131 -25.0176 Q -34.3964 -27.0389 -34.8395 -28.3018 Q -35.5417 -30.303 -34.45 -33.584 Q -33.383 -36.7907 -35.0508 -40.4304 Q -36.267 -43.0845 -35.127 -44.591 Q -34.5234 -45.3888 -32.7943 -47.0072 Q -29.9693 -49.6513 -28.9028 -51.4313 Q -27.0815 -54.4712 -27.512 -58.206 L -27.6224 -59.1616 L -28.015 -62.875 L -10.463 -59.092 L 7.089 -55.309 L 2.654 -36.6745 L -1.781 -18.04 L 16.722 10.7355 L 35.225 39.511 L 37.538 47.233 Q 36.912 47.4255 36.3099 47.86 Q 35.1056 48.729 35.225 49.939 " }
            }
        }
    }
}
