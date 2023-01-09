/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: https://www.gnu.org/licenses/fdl-1.3.html.
** $QT_END_LICENSE$
**
****************************************************************************/

//! [file]
import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: window
    width: menu.contentItem.width
    height: menu.contentItem.height
    visible: true

// Indent it like this so that the indenting in the generated doc is normal.
Menu {
    id: menu
    contentItem.parent: window

    MenuItem {
        text: qsTr("New...")
    }
    MenuItem {
        text: qsTr("Open...")
    }
    MenuItem {
        text: qsTr("Save")
    }

    MenuSeparator {
        padding: 0
        topPadding: 12
        bottomPadding: 12
        contentItem: Rectangle {
            implicitWidth: 200
            implicitHeight: 1
            color: "#1E000000"
        }
    }

    MenuItem {
        text: qsTr("Exit")
    }
}
}
//! [file]
