// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include "test_file.h"
#include <qguiapplication.h>

int main( int argc, char **argv )
{
    QGuiApplication a( argc, argv );
    SomeObject sc;
    return a.exec();
}
