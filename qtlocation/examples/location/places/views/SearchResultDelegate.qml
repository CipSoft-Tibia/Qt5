/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.5
import QtLocation 5.6
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import "../helper.js" as Helper

Item {
    id: root

    signal showPlaceDetails(variant place,variant distance)
    signal searchFor(string query)

    width: parent.width
    height: childrenRect.height

    //! [PlaceSearchModel place delegate]
    Component {
        id: placeComponent
        Item {
            id: placeRoot
            width: root.width
            height: Math.max(icon.height, 3 * placeName.height)

            Rectangle {
                anchors.fill: parent
                color: "#44ffffff"
                visible: mouse.pressed
            }

            Rectangle {
                anchors.fill: parent
                color: "#dbffde"
                visible: model.sponsored !== undefined ? model.sponsored : false

                Label {
                    text: qsTr("Sponsored result")
                    horizontalAlignment: Text.AlignRight
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    font.pixelSize: 8
                    visible: model.sponsored !== undefined ? model.sponsored : false
                }
            }

            GridLayout {
                rows: 2
                columns: 2
                anchors.fill: parent
                anchors.leftMargin: 30
                flow: GridLayout.TopToBottom

                Image {
                    // anchors.verticalCenter: parent.verticalCenter
                    id:icon
                    source: place.favorite ? "../../resources/star.png" : place.icon.url()
                    Layout.rowSpan: 2
                }

                Label {
                    id: placeName
                    text: place.favorite ? place.favorite.name : place.name
                    Layout.fillWidth: true
                }

                Label {
                    id: distanceText
                    font.italic: true
                    text: Helper.formatDistance(distance)
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 15
                height: 1
                color: "#46a2da"
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    if (model.type === undefined || type === PlaceSearchModel.PlaceResult) {
                        if (!place.detailsFetched)
                            place.getDetails();
                        root.showPlaceDetails(model.place, model.distance);
                    }
                }
            }
        }
    }
    //! [PlaceSearchModel place delegate]

    Component {
        id: proposedSearchComponent

        Item {
            id: proposedSearchRoot

            width: root.width
            height: Math.max(icon.height, 2 * proposedSearchTitle.height)

            Rectangle {
                anchors.fill: parent
                color: "#11ffffff"
                visible: mouse.pressed
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 30

                Image {
                    source: icon.url()
                }

                Label {
                    id: proposedSearchTitle
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Search for " + title
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 15
                height: 1
                color: "#46a2da"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.ListView.view.model.updateWith(index);
            }
        }
    }

    Loader {
        anchors.left: parent.left
        anchors.right: parent.right

        sourceComponent: {
            switch (model.type) {
            case PlaceSearchModel.PlaceResult:
                return placeComponent;
            case PlaceSearchModel.ProposedSearchResult:
                return proposedSearchComponent;
            default:
                //do nothing, don't assign component if result type not recognized
            }
        }
    }
}
