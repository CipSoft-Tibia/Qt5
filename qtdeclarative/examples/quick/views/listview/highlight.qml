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
import "content"

Rectangle {
    width: 200; height: 300

    // Define a delegate component. The component will be
    // instantiated for each visible item in the list.
    component PetDelegate: Item {
        id: pet
        width: 200; height: 55

        required property int index
        required property string name
        required property string type
        required property int age

        Column {
            SmallText { text: 'Name: ' + pet.name }
            SmallText { text: 'Type: ' + pet.type }
            SmallText { text: 'Age: ' + pet.age }
        }
        // indent the item if it is the current item
        states: State {
            name: "Current"
            when: pet.ListView.isCurrentItem
            PropertyChanges { target: pet; x: 20 }
        }
        transitions: Transition {
            NumberAnimation { properties: "x"; duration: 200 }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: pet.ListView.view.currentIndex = pet.index
        }
    }

//! [0]
    // Define a highlight with customized movement between items.
    component HighlightBar : Rectangle {
        width: 200; height: 50
        color: "#FFFF88"
        y: listView.currentItem.y
        Behavior on y { SpringAnimation { spring: 2; damping: 0.1 } }
    }

    ListView {
        id: listView
        width: 200; height: parent.height
        x: 30

        model: PetsModel {}
        delegate: PetDelegate {}
        focus: true

        // Set the highlight delegate. Note we must also set highlightFollowsCurrentItem
        // to false so the highlight delegate can control how the highlight is moved.
        highlight: HighlightBar {}
        highlightFollowsCurrentItem: false
    }
//! [0]
}
