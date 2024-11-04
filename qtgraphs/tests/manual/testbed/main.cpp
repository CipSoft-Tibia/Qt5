// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlapplicationengine.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/testbed/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
