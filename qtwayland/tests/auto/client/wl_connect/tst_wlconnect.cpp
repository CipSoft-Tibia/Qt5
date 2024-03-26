// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QGuiApplication>
#include <qpa/qplatformintegrationfactory_p.h>

class tst_WlConnect : public QObject
{
    Q_OBJECT
private slots:
    void failsGracefully()
    {
        // This tests whether the Wayland platform integration will fail gracefully when it's
        // unable to connect to a compositor

        // Make sure the connection actually fails
        setenv("XDG_RUNTIME_DIR", "/dev/null", 1); // a place where there are no Wayland sockets
        setenv("WAYLAND_DISPLAY", "qt_invalid_socket", 1); // just to be sure

        QStringList arguments;
        QString platformPluginPath;
        int argc = 0;
        char **argv = nullptr; //It's not currently used by the wayland plugin
        auto *platformIntegration = QPlatformIntegrationFactory::create("wayland", arguments, argc, argv, platformPluginPath);

        // The factory method should return nullptr to signify it failed gracefully
        Q_ASSERT(!platformIntegration);
    }
};

QTEST_APPLESS_MAIN(tst_WlConnect)
#include <tst_wlconnect.moc>
