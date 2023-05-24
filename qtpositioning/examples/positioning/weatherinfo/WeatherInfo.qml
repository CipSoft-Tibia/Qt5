// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

//! [0]
Window {
    id: window
//! [0]
    width: 360
    height: 640
    visible: true

    Item {
        id: statesItem
        visible: false
        state: "loading"
        states: [
            State {
                name: "loading"
                PropertyChanges { main.opacity: 0 }
                PropertyChanges { wait.opacity: 1 }
            },
            State {
                name: "ready"
                PropertyChanges { main.opacity: 1 }
                PropertyChanges { wait.opacity: 0 }
            }
        ]
    }

//! [1]
    AppModel {
        id: appModel
        onReadyChanged: {
            if (appModel.ready)
                statesItem.state = "ready"
            else
                statesItem.state = "loading"
        }
    }
//! [1]
    Item {
        id: wait
        anchors.fill: parent

        Text {
            text: "Loading weather data..."
            anchors.centerIn: parent
            font.pointSize: 18
        }
    }

    Item {
        id: main
        anchors.fill: parent

        Column {
            spacing: 6

            anchors {
                fill: parent
                topMargin: 6; bottomMargin: 6; leftMargin: 6; rightMargin: 6
            }

            Rectangle {
                width: parent.width
                height: 25
                color: "lightgrey"

                Text {
                    text: (appModel.hasValidCity ? appModel.city : "Unknown location")
                          + (appModel.useGps ? " (GPS)" : "")
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (appModel.useGps) {
                            appModel.useGps = false
                            appModel.city = "Brisbane"
                        } else {
                            switch (appModel.city) {
                            case "Brisbane":
                                appModel.city = "Oslo"
                                break
                            case "Oslo":
                                appModel.city = "Helsinki"
                                break
                            case "Helsinki":
                                appModel.city = "New York"
                                break
                            case "New York":
                                appModel.useGps = true
                                break
                            }
                        }
                    }
                }
            }

        //! [3]
            BigForecastIcon {
                id: current

                width: main.width -12
                height: 2 * (main.height - 25 - 12) / 3

                weatherIcon: (appModel.hasValidWeather
                              ? appModel.weather.weatherIcon
                              : "sunny")
        //! [3]
                topText: (appModel.hasValidWeather
                          ? appModel.weather.temperature
                          : "??")
                bottomText: (appModel.hasValidWeather
                             ? appModel.weather.weatherDescription
                             : "No weather data")

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        appModel.refreshWeather()
                    }
                }
        //! [4]
            }
        //! [4]

            Row {
                id: iconRow
                spacing: 6

                width: main.width - 12
                height: (main.height - 25 - 24) / 3

                property int daysCount: appModel.forecast.length
                property real iconWidth: (daysCount > 0) ? (iconRow.width / daysCount) - 10
                                                         : iconRow.width
                property real iconHeight: iconRow.height

                Repeater {
                    model: appModel.forecast
                    ForecastIcon {
                        required property string dayOfWeek
                        required property string temperature
                        required property string weatherIcon
                        id: forecast1
                        width: iconRow.iconWidth
                        height: iconRow.iconHeight

                        topText: (appModel.hasValidWeather ? dayOfWeek : "??")
                        bottomText: (appModel.hasValidWeather ? temperature : ("??" + "/??"))
                        weatherIcon: (appModel.hasValidWeather ? weatherIcon : "sunny")
                    }
                }
            }
        }
    }
//! [2]
}
//! [2]
