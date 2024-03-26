// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtCharts 2.0

Row {
    anchors.fill: parent
    spacing: 5
    property variant series
    property int sliceIndex: 0

    // buttons for selecting the edited object: series, slice or label
    Flow {
        spacing: 5
        flow: Flow.TopToBottom
        Button {
            id: seriesButton
            text: "series"
            unpressedColor: "#79bd8f"
            onClicked: {
                seriesFlow.visible = true;
                slicesFlow.visible = false;
                labelsFlow.visible = false;
                color = "#00a388";
                sliceButton.color = "#79bd8f";
                labelButton.color = "#79bd8f";
            }
        }
        Button {
            id: sliceButton
            text: "slice"
            unpressedColor: "#79bd8f"
            onClicked: {
                seriesFlow.visible = false;
                slicesFlow.visible = true;
                labelsFlow.visible = false;
                color = "#00a388";
                seriesButton.color = "#79bd8f";
                labelButton.color = "#79bd8f";
            }
        }
        Button {
            id: labelButton
            text: "label"
            unpressedColor: "#79bd8f"
            onClicked: {
                seriesFlow.visible = false;
                slicesFlow.visible = false;
                labelsFlow.visible = true;
                color = "#00a388";
                seriesButton.color = "#79bd8f";
                sliceButton.color = "#79bd8f";
            }
        }
    }

    // Buttons for editing series
    Flow {
        id: seriesFlow
        spacing: 5
        flow: Flow.TopToBottom
        visible: false
        Button {
            text: "visible"
            onClicked: series.visible = !series.visible;
        }
        Button {
            text: "series opacity +"
            onClicked: series.opacity += 0.1;
        }
        Button {
            text: "series opacity -"
            onClicked: series.opacity -= 0.1;
        }
        Button {
            text: "series hpos +"
            onClicked: series.horizontalPosition += 0.1;
        }
        Button {
            text: "series hpos -"
            onClicked: series.horizontalPosition -= 0.1;
        }
        Button {
            text: "series vpos +"
            onClicked: series.verticalPosition += 0.1;
        }
        Button {
            text: "series vpos -"
            onClicked: series.verticalPosition -= 0.1;
        }
        Button {
            text: "series size +"
            onClicked: series.size += 0.1;
        }
        Button {
            text: "series size -"
            onClicked: series.size -= 0.1;
        }
        Button {
            text: "series start angle +"
            onClicked: series.startAngle += 1.1;
        }
        Button {
            text: "series start angle -"
            onClicked: series.startAngle -= 1.1;
        }
        Button {
            text: "series end angle +"
            onClicked: series.endAngle += 1.1;
        }
        Button {
            text: "series end angle -"
            onClicked: series.endAngle -= 1.1;
        }
    }

    // Buttons for editing slices
    Flow {
        id: slicesFlow
        spacing: 5
        flow: Flow.TopToBottom
        visible: false

        Button {
            text: "append slice"
            onClicked: series.append("slice" + (series.count + 1), 3.0);
        }
        Button {
            text: "remove slice"
            onClicked: series.remove(series.at(series.count - 1));
        }
        Button {
            text: "slice color"
            onClicked: series.at(sliceIndex).color = main.nextColor();
        }
        Button {
            text: "slice border color"
            onClicked: series.at(sliceIndex).borderColor = main.nextColor();
        }
        Button {
            text: "slice border width +"
            onClicked: series.at(sliceIndex).borderWidth++;
        }
        Button {
            text: "slice border width -"
            onClicked: series.at(sliceIndex).borderWidth--;
        }
        Button {
            text: "slice exploded"
            onClicked: series.at(sliceIndex).exploded = !series.at(sliceIndex).exploded;
        }
        Button {
            text: "slice explode dist +"
            onClicked: series.at(sliceIndex).explodeDistanceFactor += 0.1;
        }
        Button {
            text: "slice explode dist -"
            onClicked: series.at(sliceIndex).explodeDistanceFactor -= 0.1;
        }
    }

    // Buttons for editing labels
    Flow {
        id: labelsFlow
        spacing: 5
        flow: Flow.TopToBottom
        visible: false

        Button {
            text: "label visible"
            onClicked: series.at(sliceIndex).labelVisible = !series.at(sliceIndex).labelVisible;
        }
        Button {
            text: "LabelInsideNormal"
            onClicked: series.at(sliceIndex).labelPosition = PieSlice.LabelInsideNormal;
        }
        Button {
            text: "LabelInsideHorizontal"
            onClicked: series.at(sliceIndex).labelPosition = PieSlice.LabelInsideHorizontal;
        }
        Button {
            text: "LabelInsideTangential"
            onClicked: series.at(sliceIndex).labelPosition = PieSlice.LabelInsideTangential;
        }
        Button {
            text: "LabelOutside"
            onClicked: series.at(sliceIndex).labelPosition = PieSlice.LabelOutside;
        }
        Button {
            text: "label arm len +"
            onClicked: series.at(sliceIndex).labelArmLengthFactor += 0.1;
        }
        Button {
            text: "label arm len -"
            onClicked: series.at(sliceIndex).labelArmLengthFactor -= 0.1;
        }
        Button {
            text: "slice label color"
            onClicked: series.at(sliceIndex).labelColor = main.nextColor();
        }

        FontEditor {
            id: fontEditor
            fontDescription: "label"
            function editedFont() {
                return series.at(sliceIndex).labelFont;
            }
        }
    }
}
