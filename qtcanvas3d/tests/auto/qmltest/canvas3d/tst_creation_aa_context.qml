/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCanvas3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtCanvas3D 1.0
import QtTest 1.0

Item {
    id: top
    height: 150
    width: 150

    Canvas3D {
        id: aa_context
        anchors.fill: parent
        property int initStatus: 0
        onInitializeGL: {
            getContext("antialias", {antialias:true})
            initStatus = 1
        }
    }

    TestCase {
        name: "Canvas3D_creation_aa_context"
        when: windowShown

        function test_aa_context() {
            waitForRendering(aa_context)
            tryCompare(aa_context, "initStatus", 1)
            compare(aa_context.context.canvas, aa_context)
        }
    }
}
