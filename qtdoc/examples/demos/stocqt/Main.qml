// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Layouts
import content
import custom.StockEngine

Rectangle {
    id: mainWindow
    width: 360
    height: 800
    visible: true
    color: "#101010"

    property alias currentIndex: root.currentIndex
    property alias stateGroup: stateGroup
    property bool portrait: width < height

    ListView {
        id: root
        width: parent.width
        anchors.top: parent.top
        anchors.bottom: navbar.top

        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 250
        focus: false
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        interactive: false

        Component.onCompleted: StockEngine.updateStockListModel()

        model: ObjectModel {
            Item {
                id: stockContainer
                width: root.width
                height: root.height
                StockView {
                    id: stockView
                    anchors.fill: parent
                    visible: false
                    width: root.width
                    height: root.height
                }

                StockListView {
                    id: listView
                    visible: true
                    anchors.fill: parent
                    width: root.width
                    height: root.height
                }
            }

            FavoriteView {
                id: favoriteView
                width: root.width
                height: root.height
            }

            SettingsView {
                id: infoView
                width: root.width
                height: root.height
            }
        }
    }


    Navbar {
        id: navbar
        width: parent.width
        height: portrait? 72 : 40
        anchors.bottom: parent.bottom
        state: "Home"
        homeButton.onClicked: root.currentIndex = 0
        favoriteButton.onClicked: root.currentIndex = 1
        settingsButton.onClicked: root.currentIndex = 2
    }

    onCurrentIndexChanged: {
        const navStates = ["Home", "Favorite", "Settings"]
        navbar.state = navStates[currentIndex]
    }

    StateGroup {
        id: stateGroup
        states: [
            State {
                name: "ListView"
            },
            State {
                name: "StockView"
                PropertyChanges {
                    target: listView
                    visible: false
                }
                PropertyChanges {
                    target: stockView
                    visible: true
                }
                PropertyChanges {
                    target: navbar
                    state: "Home"
                }
            }
        ]
    }
}
