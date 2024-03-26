// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QApplication>

#include "localewidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    LocaleWidget wgt;
    wgt.show();
    return app.exec();
}
