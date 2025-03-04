// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtGraphs
import QtTest 1.0

Item {
    id: top
    width: 150
    height: 150

    Theme3D {
        id: initial
    }

    Gradient {
        id: gradient1
        stops: [
            GradientStop { color: "red"; position: 0 },
            GradientStop { color: "blue"; position: 1 }
        ]
    }

    Gradient {
        id: gradient2
        stops: [
            GradientStop { color: "green"; position: 0 },
            GradientStop { color: "red"; position: 1 }
        ]
    }

    Gradient {
        id: gradient3
        stops: [
            GradientStop { color: "gray"; position: 0 },
            GradientStop { color: "darkgray"; position: 1 }
        ]
    }

    Color {
        id: color1
        color: "red"
    }

    Color {
        id: color2
        color: "blue"
    }

    Theme3D {
        id: initialized
        ambientLightStrength: 0.3
        backgroundColor: "#ff0000"
        backgroundEnabled: false
        baseColors: [color1, color2]
        baseGradients: [gradient1, gradient2]
        colorStyle: Theme3D.ColorStyle.RangeGradient
        font.family: "Arial"
        gridEnabled: false
        gridLineColor: "#00ff00"
        labelBackgroundColor: "#ff00ff"
        labelBackgroundEnabled: false
        labelBorderEnabled: false
        labelTextColor: "#00ffff"
        lightColor: "#ffff00"
        lightStrength: 2.5
        multiHighlightColor: "#ff00ff"
        multiHighlightGradient: gradient3
        shadowStrength: 12.5
        singleHighlightColor: "#ff0000"
        singleHighlightGradient: gradient3
        type: Theme3D.Theme.UserDefined // Default values will be overwritten by initialized values
        windowColor: "#fff00f"
    }

    Theme3D {
        id: change
    }

    Theme3D {
        id: invalid
    }

    TestCase {
        name: "Theme3D Initial"

        Text { id: dummy }

        function test_initial() {
            compare(initial.ambientLightStrength, 0.25)
            compare(initial.backgroundColor, "#000000")
            compare(initial.backgroundEnabled, true)
            compare(initial.baseColors.length, 1)
            compare(initial.baseColors[0].color, "#000000")
            // TODO: Do we actually need to support this? QTBUG-116923
            //compare(initial.baseGradients.length, 1)
            //compare(initial.baseGradients[0].stops[0], "#000000")
            //compare(initial.baseGradients[0].stops[1], "#ffffff")
            compare(initial.colorStyle, Theme3D.ColorStyle.Uniform)
            // Initial font needs to be tested like this, as different platforms have different default font (QFont())
            compare(initial.font.family, dummy.font.family)
            compare(initial.gridEnabled, true)
            compare(initial.gridLineColor, "#ffffff")
            compare(initial.labelBackgroundColor, "#a0a0a4")
            compare(initial.labelBackgroundEnabled, true)
            compare(initial.labelBorderEnabled, true)
            compare(initial.labelTextColor, "#ffffff")
            compare(initial.lightColor, "#ffffff")
            compare(initial.lightStrength, 5)
            compare(initial.multiHighlightColor, "#0000ff")
            compare(initial.multiHighlightGradient, 0)
            compare(initial.shadowStrength, 25)
            compare(initial.singleHighlightColor, "#ff0000")
            compare(initial.singleHighlightGradient, 0)
            compare(initial.type, Theme3D.Theme.UserDefined)
            compare(initial.windowColor, "#000000")
        }
    }

    TestCase {
        name: "Theme3D Initialized"

        function test_initialized() {
            compare(initialized.ambientLightStrength, 0.3)
            compare(initialized.backgroundColor, "#ff0000")
            compare(initialized.backgroundEnabled, false)
            compare(initialized.baseColors.length, 2)
            compare(initialized.baseColors[0].color, "#ff0000")
            compare(initialized.baseColors[1].color, "#0000ff")
            compare(initialized.baseGradients.length, 2)
            compare(initialized.baseGradients[0], gradient1)
            compare(initialized.baseGradients[1], gradient2)
            compare(initialized.colorStyle, Theme3D.ColorStyle.RangeGradient)
            compare(initialized.font.family, "Arial")
            compare(initialized.gridEnabled, false)
            compare(initialized.gridLineColor, "#00ff00")
            compare(initialized.labelBackgroundColor, "#ff00ff")
            compare(initialized.labelBackgroundEnabled, false)
            compare(initialized.labelBorderEnabled, false)
            compare(initialized.labelTextColor, "#00ffff")
            compare(initialized.lightColor, "#ffff00")
            compare(initialized.lightStrength, 2.5)
            compare(initialized.multiHighlightColor, "#ff00ff")
            compare(initialized.multiHighlightGradient, gradient3)
            compare(initialized.shadowStrength, 12.5)
            compare(initialized.singleHighlightColor, "#ff0000")
            compare(initialized.singleHighlightGradient, gradient3)
            compare(initialized.type, Theme3D.Theme.UserDefined)
            compare(initialized.windowColor, "#fff00f")
        }
    }

    TestCase {
        name: "Theme3D Change"

        Color {
            id: color3
            color: "red"
        }

        Gradient {
            id: gradient4
            stops: [
                GradientStop { color: "red"; position: 0 },
                GradientStop { color: "blue"; position: 1 }
            ]
        }

        function test_1_change() {
            change.type = Theme3D.Theme.StoneMoss // Default values will be overwritten by the following sets
            change.ambientLightStrength = 0.3
            change.backgroundColor = "#ff0000"
            change.backgroundEnabled = false
            change.baseColors = [color3, color2]
            change.baseGradients = [gradient4, gradient2]
            change.colorStyle = Theme3D.ColorStyle.ObjectGradient
            change.font.family = "Arial"
            change.gridEnabled = false
            change.gridLineColor = "#00ff00"
            change.labelBackgroundColor = "#ff00ff"
            change.labelBackgroundEnabled = false
            change.labelBorderEnabled = false
            change.labelTextColor = "#00ffff"
            change.lightColor = "#ffff00"
            change.lightStrength = 2.5
            change.multiHighlightColor = "#ff00ff"
            change.multiHighlightGradient = gradient3
            change.shadowStrength = 50
            change.singleHighlightColor = "#ff0000"
            change.singleHighlightGradient = gradient3
            change.windowColor = "#fff00f"

            compare(change.ambientLightStrength, 0.3)
            compare(change.backgroundColor, "#ff0000")
            compare(change.backgroundEnabled, false)
            compare(change.baseColors.length, 2)
            compare(change.baseColors[0].color, "#ff0000")
            compare(change.baseColors[1].color, "#0000ff")
            compare(change.baseGradients.length, 2)
            compare(change.baseGradients[0], gradient4)
            compare(change.baseGradients[1], gradient2)
            compare(change.colorStyle, Theme3D.ColorStyle.ObjectGradient)
            compare(change.font.family, "Arial")
            compare(change.gridEnabled, false)
            compare(change.gridLineColor, "#00ff00")
            compare(change.labelBackgroundColor, "#ff00ff")
            compare(change.labelBackgroundEnabled, false)
            compare(change.labelBorderEnabled, false)
            compare(change.labelTextColor, "#00ffff")
            compare(change.lightColor, "#ffff00")
            compare(change.lightStrength, 2.5)
            compare(change.multiHighlightColor, "#ff00ff")
            compare(change.multiHighlightGradient, gradient3)
            compare(change.shadowStrength, 50)
            compare(change.singleHighlightColor, "#ff0000")
            compare(change.singleHighlightGradient, gradient3)
            compare(change.type, Theme3D.Theme.StoneMoss)
            compare(change.windowColor, "#fff00f")
        }

        function test_2_change_color() {
            color3.color = "white"
            compare(change.baseColors[0].color, "#ffffff")
        }

        function test_3_change_gradient() {
            gradient4.stops[0].color = "black"
            compare(change.baseGradients[0].stops[0].color, "#000000")
        }
    }


    TestCase {
        name: "Theme3D Invalid"

        function test_invalid() {
            invalid.ambientLightStrength = -1.0
            compare(invalid.ambientLightStrength, 0.25)
            invalid.ambientLightStrength = 1.1
            compare(invalid.ambientLightStrength, 0.25)
            invalid.lightStrength = -1.0
            compare(invalid.lightStrength, 5.0)
            invalid.lightStrength = 10.1
            compare(invalid.lightStrength, 5.0)
            invalid.shadowStrength = -1.0
            compare(invalid.shadowStrength, 25.0)
            invalid.shadowStrength = 100.1
            compare(invalid.shadowStrength, 25.0)
        }
    }
}
