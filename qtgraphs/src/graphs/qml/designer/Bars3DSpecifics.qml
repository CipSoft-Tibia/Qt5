// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import HelperWidgets
import QtQuick.Layouts
import StudioTheme 1.0 as StudioTheme
import QtQuick.Controls as Controls

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Bars")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Uniform Scaling")
                tooltip: qsTr("Proportionally scale multiple series")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.multiSeriesUniform
                    Layout.fillWidth: true
                }
            }
            PropertyLabel {
                text: qsTr("Thickness")
                tooltip: qsTr("Thickness ratio between X and Z dimension")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.barThickness
                    minimumValue: 0.01
                    maximumValue: 100.0
                    stepSize: 0.01
                    decimals: 2
                    Layout.fillWidth: true
                }
            }

            PropertyLabel {
                text: qsTr("Spacing")
                tooltip: qsTr("Bar spacing in the X and Z dimensions")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.barSpacing_width
                    minimumValue: 0.0
                    maximumValue: 10.0
                    stepSize: 0.01
                    decimals: 2
                    Layout.fillWidth: true
                }
                ControlLabel {
                    text: qsTr("col")
                    width: StudioTheme.Values.actionIndicatorWidth
                }

                SpinBox {
                    backendValue: backendValues.barSpacing_height
                    minimumValue: 0.0
                    maximumValue: 10.0
                    stepSize: 0.01
                    decimals: 2
                    Layout.fillWidth: true
                }
                ControlLabel {
                    text: qsTr("row")
                    width: StudioTheme.Values.actionIndicatorWidth
                }

            }
            PropertyLabel {
                text: qsTr("Relative Spacing")
                tooltip: qsTr("Set bar spacing relative to thickness")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.barSpacingRelative
                    Layout.fillWidth: true
                }
            }
            PropertyLabel {
                text: qsTr("Series Margin")
                tooltip: qsTr("Margin between series columns in X and Z dimensions")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                SpinBox {
                    backendValue: backendValues.barSeriesMargin_width
                    minimumValue: 0.0
                    maximumValue: 1.0
                    stepSize: 0.01
                    decimals: 2
                    Layout.fillWidth: true
                }
                ControlLabel {
                    text: qsTr("col")
                    width: StudioTheme.Values.actionIndicatorWidth
                }

                SpinBox {
                    backendValue: backendValues.barSeriesMargin_height
                    minimumValue: 0.0
                    maximumValue: 1.0
                    stepSize: 0.01
                    decimals: 2
                    Layout.fillWidth: true
                }
                ControlLabel {
                    text: qsTr("row")
                    width: StudioTheme.Values.actionIndicatorWidth
                }

            }
            PropertyLabel {
                text: qsTr("Floor Level")
                tooltip: qsTr("Floor level in Y-axis data coordinates")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.floorLevel
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    Layout.fillWidth: true
                }
            }
            PropertyLabel {
                text: qsTr("Selection Mode")
                tooltip: qsTr("Bar selection mode")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                id: selectionLayout
                property bool isInModel: backendValue.isInModel;
                property bool isInSubState: backendValue.isInSubState;
                property bool selectionChangedFlag: selectionChanged
                property variant backendValue: backendValues.selectionMode
                property variant valueFromBackend: backendValue.value
                property string enumScope: "AbstractGraph3D"
                property string enumSeparator: " | "
                property int checkedCount: 0
                property bool selectionItem: false
                property bool selectionRow: false
                property bool selectionColumn: false
                property bool selectionSlice: false
                property bool selectionMulti: false

                function checkValue(checkedVariable, variableText, expressionBase) {
                    var expressionStr = expressionBase
                    if (checkedVariable) {
                        if (expressionStr !== "") {
                            expressionStr += enumSeparator
                        }
                        expressionStr += enumScope
                        expressionStr += "."
                        expressionStr += variableText
                        checkedCount++
                    }
                    return expressionStr
                }

                function composeSelectionMode() {
                    var expressionStr = ""
                    checkedCount = 0
                    expressionStr = checkValue(selectionItem, "SelectionItem", expressionStr)
                    expressionStr = checkValue(selectionRow, "SelectionRow", expressionStr)
                    expressionStr = checkValue(selectionColumn, "SelectionColumn", expressionStr)
                    expressionStr = checkValue(selectionSlice, "SelectionSlice", expressionStr)
                    expressionStr = checkValue(selectionMulti, "SelectionMultiSeries", expressionStr)

                    if (checkedCount === 0)
                        backendValue.expression = enumScope + ".SelectionNone"
                    else
                        backendValue.expression = expressionStr
                }

                function evaluate() {
                    if (backendValue.value === undefined)
                        return

                    selectionItem = (backendValue.expression.indexOf("SelectionItem") !== -1)
                    selectionRow = (backendValue.expression.indexOf("SelectionRow") !== -1)
                    selectionColumn = (backendValue.expression.indexOf("SelectionColumn") !== -1)
                    selectionSlice = (backendValue.expression.indexOf("SelectionSlice") !== -1)
                    selectionMulti = (backendValue.expression.indexOf("SelectionMultiSeries") !== -1)

                    selectionItemBox.checked = selectionItem
                    selectionRowBox.checked = selectionRow
                    selectionColumnBox.checked = selectionColumn
                    selectionSliceBox.checked = selectionSlice
                    selectionMultiSeriesBox.checked = selectionMulti
                }

                onSelectionChangedFlagChanged: evaluate()

                onIsInModelChanged: evaluate()

                onIsInSubStateChanged: evaluate()

                onBackendValueChanged: evaluate()

                onValueFromBackendChanged: evaluate()

                ColumnLayout {
                    anchors.fill: parent

                    Controls.CheckBox {
                        id: selectionItemBox
                        text: "SelectionItem"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.selectionItem = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: selectionRowBox
                        text: "SelectionRow"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.selectionRow = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: selectionColumnBox
                        text: "SelectionColumn"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.selectionColumn = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: selectionSliceBox
                        text: "SelectionSlice"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.selectionSlice = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: selectionMultiSeriesBox
                        text: "SelectionMultiSeries"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.selectionMulti = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                }
            }
        }
    }

    GraphsSection {}

    GraphsCameraSection {}
}
