/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Milian Wolff <milian.wolff@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebChannel module of the Qt Toolkit.
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

#include <QtQuickTest/quicktest.h>
#include <QtQml/qqml.h>

#ifndef QUICK_TEST_SOURCE_DIR
#define QUICK_TEST_SOURCE_DIR Q_NULLPTR
#endif

#include "testtransport.h"
#include "testwebchannel.h"
#include "testobject.h"

int main(int argc, char **argv)
{
    qmlRegisterType<TestTransport>("QtWebChannel.Tests", 1, 0, "TestTransport");
    qmlRegisterType<TestWebChannel>("QtWebChannel.Tests", 1, 0, "TestWebChannel");
    qmlRegisterType<TestObject>("QtWebChannel.Tests", 1, 0, "TestObject");

    return quick_test_main(argc, argv, "qml", QUICK_TEST_SOURCE_DIR);
}
