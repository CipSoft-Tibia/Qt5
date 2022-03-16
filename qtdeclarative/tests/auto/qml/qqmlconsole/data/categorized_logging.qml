/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12

Item {
    id:root

    LoggingCategory {
        id: testCategory
        name: "qt.test"
    }

    LoggingCategory {
        id: testCategoryStartingFromWarning
        name: "qt.test.warning"
        defaultLogLevel: LoggingCategory.Warning
    }

    LoggingCategory {
        id: emptyCategory
    }

    Component.onCompleted: {
        console.debug(testCategory, "console.debug");
        console.log(testCategory, "console.log");
        console.info(testCategory, "console.info");
        console.warn(testCategory, "console.warn");
        console.error(testCategory, "console.error");
        console.debug(testCategoryStartingFromWarning, "console.debug");
        console.log(testCategoryStartingFromWarning, "console.log");
        console.info(testCategoryStartingFromWarning, "console.info");
        console.warn(testCategoryStartingFromWarning, "console.warn");
        console.error(testCategoryStartingFromWarning, "console.error");

        testCategory.name = "qt.test2";
        testCategory.defaultLogLevel = LoggingCategory.Debug;

        console.error(emptyCategory, "console.error");
    }
}
