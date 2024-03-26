// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Row {
    anchors.fill: parent
    spacing: 5
    property variant series

    // buttons for selecting the edited object: series, barset or label
    Flow {
        spacing: 5
        flow: Flow.TopToBottom
        Button {
            id: seriesButton
            text: "series"
            unpressedColor: "#79bd8f"
            onClicked: {
                seriesFlow.visible = true;
                setFlow.visible = false;
                labelsFlow.visible = false;
                color = "#00a388";
                setButton.color = "#79bd8f";
                labelButton.color = "#79bd8f";
            }
        }
        Button {
            id: setButton
            text: "BarSet"
            unpressedColor: "#79bd8f"
            onClicked: {
                seriesFlow.visible = false;
                setFlow.visible = true;
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
                setFlow.visible = false;
                labelsFlow.visible = true;
                color = "#00a388";
                seriesButton.color = "#79bd8f";
                setButton.color = "#79bd8f";
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
            text: "opacity +"
            onClicked: series.opacity += 0.1;
        }
        Button {
            text: "opacity -"
            onClicked: series.opacity -= 0.1;
        }
        Button {
            text: "bar width +"
            onClicked: series.barWidth += 0.1;
        }
        Button {
            text: "bar width -"
            onClicked: series.barWidth -= 0.1;
        }
    }

    // Buttons for editing sets
    Flow {
        id: setFlow
        spacing: 5
        flow: Flow.TopToBottom
        visible: false

        Button {
            text: "append set"
            onClicked: {
                var count = series.count;
                series.append("set" + count, [0, 0.1 * count, 0.2 * count, 0.3 * count, 0.4 * count, 0.5 * count, 0.6 * count]);
            }
        }
        Button {
            text: "insert set"
            onClicked: {
                var count = series.count;
                series.insert(count - 1, "set" + count, [0, 0.1 * count, 0.2 * count, 0.3 * count, 0.4 * count, 0.5 * count, 0.6 * count]);
            }
        }
        Button {
            text: "remove set"
            onClicked: series.remove(series.at(series.count - 1));
        }
        Button {
            text: "clear sets"
            onClicked: series.clear();
        }

        Button {
            text: "set 1 append"
            onClicked: series.at(0).append(series.at(0).count + 1);
        }
        Button {
            text: "set 1 replace"
            onClicked: series.at(0).replace(series.at(0).count - 1, series.at(0).at(series.at(0).count - 1) + 1.5);
        }
        Button {
            text: "set 1 remove"
            onClicked: series.at(0).remove(series.at(0).count - 1);
        }

        Button {
            text: "set 1 color"
            onClicked: series.at(0).color = main.nextColor();
        }
        Button {
            text: "set 1 border color"
            onClicked: series.at(0).borderColor = main.nextColor();
        }
        Button {
            text: "set 1 borderWidth +"
            onClicked: series.at(0).borderWidth += 0.5;
        }
        Button {
            text: "set 1 borderWidth -"
            onClicked: series.at(0).borderWidth -= 0.5;
        }
    }


    Flow {
        id: labelsFlow
        spacing: 5
        flow: Flow.TopToBottom
        visible: false

        Button {
            text: "labels visible"
            onClicked: series.labelsVisible = !series.labelsVisible;
        }
        Button {
            text: "labels format"
            onClicked: {
                if (series.labelsFormat === "@value")
                    series.labelsFormat = "@value%"
                else
                    series.labelsFormat = "@value"
            }
        }
        Button {
            text: "labels position"
            onClicked: series.changeLabelsPosition();
        }
        Button {
            text: "set 1 label color"
            onClicked: series.at(0).labelColor = main.nextColor();
        }
        Button {
            text: "labels angle +"
            onClicked: series.labelsAngle = series.labelsAngle + 5;
        }
        Button {
            text: "labels angle -"
            onClicked: series.labelsAngle = series.labelsAngle - 5;
        }
        Button {
            text: "labels precision"
            onClicked: {
                if (series.labelsPrecision == 2)
                    series.labelsPrecision = 4;
                else
                    series.labelsPrecision = 2;
            }
        }

        FontEditor {
            id: fontEditor
            fontDescription: "label"
            function editedFont() {
                return series.at(0).labelFont;
            }
        }
    }
}
