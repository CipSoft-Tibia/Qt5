// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtGraphs
//import QtDataVisualization
import "."

Item {
    id: mainview
    width: 1280
    height: 720

    property var customTheme: customSurfaceTheme

    ColorGradient {
        id: customGradient
        ColorGradientStop { position: 0.0; color: "red" }
        ColorGradientStop { position: 1.0; color: "green" }
    }

    ColorGradient {
        id: singleGradient
        ColorGradientStop { position: 0.0; color: "white" }
        ColorGradientStop { position: 1.0; color: "yellow" }
    }

    ColorGradient {
        id: multiGradient
        ColorGradientStop { position: 0.0; color: "white" }
        ColorGradientStop { position: 1.0; color: "blue" }
    }

    Theme3D {
        id: customSurfaceTheme
        type: Theme3D.ThemeUserDefined
        colorStyle: Theme3D.ColorStyleObjectGradient
        backgroundColor: "gray"
        gridLineColor: "lightGray"
        multiHighlightColor: "blue"
        singleHighlightColor: "yellow"
        multiHighlightGradient: multiGradient
        singleHighlightGradient: singleGradient
    }

    Theme3D {
        id: customBarsTheme
        type: Theme3D.ThemeUserDefined
        colorStyle: Theme3D.ColorStyleObjectGradient
        backgroundColor: "gray"
        gridLineColor: "lightGray"
        multiHighlightColor: "blue"
        singleHighlightColor: "yellow"
        multiHighlightGradient: multiGradient
        singleHighlightGradient: singleGradient
    }

    Item {
        id: graphView
        width: mainview.width - settings.width
        height: mainview.height
        anchors.right: mainview.right;

        Surface3D {
            id: surface
            anchors.fill: graphView
            theme: customSurfaceTheme
            shadowQuality: AbstractGraph3D.ShadowQualityNone
            selectionMode: AbstractGraph3D.SelectionNone
            scene.activeCamera.cameraPreset: Camera3D.CameraPresetIsometricLeft
            msaaSamples: 8
            aspectRatio: 3.0
            visible: !barsVisible.checked

            Surface3DSeries {
                id: surfaceSeries
                baseGradient: customGradient
                baseColor: "white"
                HeightMapSurfaceDataProxy {
                    heightMapFile: ":/layer_1.png"
                    autoScaleY: true
                }
                flatShadingEnabled: false
                drawMode: Surface3DSeries.DrawSurface
            }
        }

        Bars3D {
            id: bars
            anchors.fill: graphView
            theme: customBarsTheme
            shadowQuality: AbstractGraph3D.ShadowQualityNone
            selectionMode: AbstractGraph3D.SelectionItemAndRow
            scene.activeCamera.cameraPreset: Camera3D.CameraPresetIsometricLeft
            msaaSamples: 8
            aspectRatio: 3.0
            visible: barsVisible.checked

            Bar3DSeries {
                id: barsSeries
                baseGradient: customGradient
                baseColor: "white"
                ItemModelBarDataProxy {
                    id: barProxy
                    itemModel: ListModel {
                        ListElement{ coords: "0,0"; data: "20.0/10.0/4.75"; }
                        ListElement{ coords: "1,0"; data: "21.1/10.3/3.00"; }
                        ListElement{ coords: "2,0"; data: "22.5/10.7/1.24"; }
                        ListElement{ coords: "3,0"; data: "24.0/10.5/2.53"; }
                        ListElement{ coords: "0,1"; data: "20.2/11.2/3.55"; }
                        ListElement{ coords: "1,1"; data: "21.3/11.5/3.03"; }
                        ListElement{ coords: "2,1"; data: "22.6/11.7/3.46"; }
                        ListElement{ coords: "3,1"; data: "23.4/11.5/4.12"; }
                        ListElement{ coords: "0,2"; data: "20.2/12.3/3.37"; }
                        ListElement{ coords: "1,2"; data: "21.1/12.4/2.98"; }
                        ListElement{ coords: "2,2"; data: "22.5/12.1/3.33"; }
                        ListElement{ coords: "3,2"; data: "23.3/12.7/3.23"; }
                        ListElement{ coords: "0,3"; data: "20.7/13.3/5.34"; }
                        ListElement{ coords: "1,3"; data: "21.5/13.2/4.54"; }
                        ListElement{ coords: "2,3"; data: "22.4/13.6/4.65"; }
                        ListElement{ coords: "3,3"; data: "23.2/13.4/6.67"; }
                        ListElement{ coords: "0,4"; data: "20.6/15.0/6.01"; }
                        ListElement{ coords: "1,4"; data: "21.3/14.6/5.83"; }
                        ListElement{ coords: "2,4"; data: "22.5/14.8/7.32"; }
                        ListElement{ coords: "3,4"; data: "23.7/14.3/6.90"; }
                    }
                    rowRole: "coords"
                    columnRole: "coords"
                    valueRole: "data"
                    rowRolePattern: /(\d),\d/
                    columnRolePattern: /(\d),(\d)/
                    valueRolePattern: /^([^\/]*)\/([^\/]*)\/(.*)$/
                    rowRoleReplace: "\\1"
                    columnRoleReplace: "\\2"
                    valueRoleReplace: "\\3"
                }
            }
        }
    }

    ColumnLayout {
        id: settings
        anchors.top: parent.top
        anchors.left: parent.left
        width: 200
        spacing: 10

        Label {
            text: "Bars3D Graph"
            color: "gray"
        }
        CheckBox {
            id: barsVisible
            onCheckedChanged: {
                customTheme = (checked ? customBarsTheme : customSurfaceTheme)
            }
        }

        Label {
            text: "Ambient Light Strength"
            color: "gray"
        }
        Slider {
            from: 0.0
            to: 1.0
            value: customTheme.ambientLightStrength
            onValueChanged: customTheme.ambientLightStrength = value
        }

        Label {
            text: "Highlight Light Strength"
            color: "gray"
            enabled: barsVisible.checked
        }
        Slider {
            from: 0.0
            to: 10.0
            value: customTheme.highlightLightStrength
            enabled: barsVisible.checked
            onValueChanged: customTheme.highlightLightStrength = value
        }

        Label {
            text: "Light Strength"
            color: "gray"
        }
        Slider {
            from: 0.0
            to: 10.0
            value: customTheme.lightStrength
            onValueChanged: customTheme.lightStrength = value
        }

        Label {
            text: "Light Color; Red"
            color: "gray"
        }
        Slider {
            from: 0.0
            to: 1.0
            value: customTheme.lightColor.r
            onValueChanged: customTheme.lightColor.r = value
        }

        Label {
            text: "Light Color; Green"
            color: "gray"
        }
        Slider {
            from: 0.0
            to: 1.0
            value: customTheme.lightColor.g
            onValueChanged: customTheme.lightColor.g = value
        }

        Label {
            text: "Light Color; Blue"
            color: "gray"
        }
        Slider {
            from: 0.0
            to: 1.0
            value: customTheme.lightColor.b
            onValueChanged: customTheme.lightColor.b = value
        }

        Label {
            text: "Color Style Uniform"
            color: "gray"
        }
        CheckBox {
            checked: (customTheme.colorStyle === Theme3D.ColorStyleUniform)
            onCheckedChanged: {
                if (checked)
                    customTheme.colorStyle = Theme3D.ColorStyleUniform
                else
                    customTheme.colorStyle = Theme3D.ColorStyleObjectGradient
            }
        }

        Label {
            text: "Background"
            color: "gray"
        }
        CheckBox {
            checked: customTheme.backgroundEnabled
            onCheckedChanged: {
                customTheme.backgroundEnabled = checked
            }
        }

        Label {
            text: "Grid"
            color: "gray"
        }
        CheckBox {
            checked: customTheme.gridEnabled
            onCheckedChanged: {
                customTheme.gridEnabled = checked
            }
        }

        Label {
            text: "Labels"
            color: "gray"
        }
        CheckBox {
            checked: customTheme.labelsEnabled
            onCheckedChanged: {
                customTheme.labelsEnabled = checked
            }
        }

        Label {
            text: "Label Background"
            color: "gray"
        }
        CheckBox {
            checked: customTheme.labelBackgroundEnabled
            onCheckedChanged: {
                customTheme.labelBackgroundEnabled = checked
            }
        }

        Label {
            text: "Label Border"
            color: "gray"
        }
        CheckBox {
            checked: customTheme.labelBorderEnabled
            onCheckedChanged: {
                customTheme.labelBorderEnabled = checked
            }
        }
    }
}
