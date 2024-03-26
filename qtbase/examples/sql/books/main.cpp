// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bookwindow.h"

#include <QtWidgets>

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);

    BookWindow win;
    win.show();

    return app.exec();
}
