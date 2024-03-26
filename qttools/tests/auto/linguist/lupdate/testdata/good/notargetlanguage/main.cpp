// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

























#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    app.installTranslator(&translator);

    QObject::tr("un mot","toto",1);

    return app.exec();
}
