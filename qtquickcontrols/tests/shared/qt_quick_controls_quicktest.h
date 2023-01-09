/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef QT_QUICK_CONTROLS_QUICKTEST_H
#define QT_QUICK_CONTROLS_QUICKTEST_H

#include "qt_quick_controls_testapp.h"
#include <QtQuickTest/quicktestglobal.h>

QT_BEGIN_NAMESPACE

#ifdef QUICK_TEST_SOURCE_DIR
#define QT_QUICK_CONTROLS_TEST_MAIN_VAR QUICK_TEST_SOURCE_DIR
#else
#define QT_QUICK_CONTROLS_TEST_MAIN_VAR 0
#endif

#define QT_QUICK_CONTROLS_TEST_MAIN(name) \
    int main(int argc, char **argv) \
    { \
        QTEST_SET_MAIN_SOURCE_PATH \
        QtQuickControlsTestApp* app = 0; \
        if (!QCoreApplication::instance()) \
            app = new QtQuickControlsTestApp(argc, argv); \
        int i = quick_test_main(argc, argv, #name, QT_QUICK_CONTROLS_TEST_MAIN_VAR); \
        delete app; \
        return i; \
    }

QT_END_NAMESPACE

#endif // QT_QUICK_CONTROLS_QUICKTEST_H
