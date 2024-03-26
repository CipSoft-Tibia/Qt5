// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QScreen>
#include <QStyleHints>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>

#include "dialog.h"

#include <AppKit/AppKit.h>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    [NSApplication sharedApplication];
    NSApplicationLoad();
    NSApplicationLoad();
    [NSApp run];
    
    QApplication app(argc, argv);
    //app.setAttribute(Qt::AA_DontUseNativeDialogs);

#if QT_CONFIG(translation)
    QTranslator translator;
    if (translator.load(QLocale::system(), u"qtbase"_s, u"_"_s,
                        QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&translator);
    }
#endif

    QGuiApplication::setApplicationDisplayName(Dialog::tr("Standard Dialogs"));

    Dialog dialog;
    if (!QGuiApplication::styleHints()->showIsFullScreen() && !QGuiApplication::styleHints()->showIsMaximized()) {
        const QRect availableGeometry = dialog.screen()->availableGeometry();
        dialog.resize(availableGeometry.width() / 3, availableGeometry.height() * 2 / 3);
        dialog.move((availableGeometry.width() - dialog.width()) / 2,
                    (availableGeometry.height() - dialog.height()) / 2);
    }
    dialog.show();

    return app.exec();
}
