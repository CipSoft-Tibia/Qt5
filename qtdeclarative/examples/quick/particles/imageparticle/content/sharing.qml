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

// This example shows how to create your own highlight delegate for a ListView
// that uses a SpringAnimation to provide custom movement when the
// highlight bar is moved between items.

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    id: root

    property real delegateHeight: 65
    width: 200; height: 300
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#EEEEFF" }
        GradientStop { position: 1.0; color: "lightblue" }
    }

    // Define a delegate component.  A component will be
    // instantiated for each visible item in the list.
    component PetDelegate: Item {
        id: pet
        width: 200; height: root.delegateHeight
        z: 10

        required property int index
        required property string name
        required property string type
        required property int age

        Column {
            Text {color: "white"; text: pet.name; font.pixelSize: 18 }
            Text {color: "white"; text: 'Type: ' + pet.type; font.pixelSize: 14 }
            Text {color: "white"; text: 'Age: ' + pet.age; font.pixelSize: 14 }
        }
        MouseArea { anchors.fill: parent; onClicked: listView.currentIndex = pet.index; }
        // indent the item if it is the current item
        states: State {
            name: "Current"
            when: pet.ListView.isCurrentItem
            PropertyChanges { target: pet; x: 20 }
        }
        transitions: Transition {
            NumberAnimation { properties: "x"; duration: 200 }
        }
    }

    // Define a highlight with customized movement between items.
    component HighlightBar : Rectangle {
        z: 0
        width: 200; height: root.delegateHeight
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#99FF99" }
            GradientStop { position: 1.0; color: "#88FF88" }
        }
        y: listView.currentItem.y;
        Behavior on y { SpringAnimation { spring: 2; damping: 0.2 } }
        //! [1]
        ImageParticle {
            anchors.fill: parent
            system: particles
            source: "../../images/flower.png"
            color: "red"
            clip: true
            alpha: 1.0
        }
        //! [1]
    }

    ListView {
        id: listView
        width: 200; height: parent.height

        model: petsModel
        delegate: PetDelegate {}
        focus: true

        // Set the highlight delegate. Note we must also set highlightFollowsCurrentItem
        // to false so the highlight delegate can control how the highlight is moved.
        highlight: HighlightBar {}
        highlightFollowsCurrentItem: false

        ParticleSystem { id: particles }
        Emitter {
            system: particles
            anchors.fill: parent
            emitRate: 0
            lifeSpan: 10000
            size: 24
            sizeVariation: 8
            velocity: AngleDirection { angleVariation: 360; magnitude: 3 }
            maximumEmitted: 10
            startTime: 5000
            Timer { running: true; interval: 10; onTriggered: parent.emitRate = 1; }
        }

        //! [0]
        ImageParticle {
            anchors.fill: parent
            system: particles
            source: "../../images/flower.png"
            alpha: 0.1
            color: "white"
            rotationVariation: 180
            z: -1
        }
        //! [0]
    }

    ListModel {
        id: petsModel
        ListElement {
            name: "Polly"
            type: "Parrot"
            age: 12
            size: "Small"
        }
        ListElement {
            name: "Penny"
            type: "Turtle"
            age: 4
            size: "Small"
        }
        ListElement {
            name: "Warren"
            type: "Rabbit"
            age: 2
            size: "Small"
        }
        ListElement {
            name: "Spot"
            type: "Dog"
            age: 9
            size: "Medium"
        }
        ListElement {
            name: "Schrödinger"
            type: "Cat"
            age: 2
            size: "Medium"
        }
        ListElement {
            name: "Joey"
            type: "Kangaroo"
            age: 1
            size: "Medium"
        }
        ListElement {
            name: "Kimba"
            type: "Bunny"
            age: 65
            size: "Large"
        }
        ListElement {
            name: "Rover"
            type: "Dog"
            age: 5
            size: "Large"
        }
        ListElement {
            name: "Tiny"
            type: "Elephant"
            age: 15
            size: "Large"
        }
    }

}
