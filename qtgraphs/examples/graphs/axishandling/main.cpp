// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/qguiapplication.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlengine.h>
#include <QtQml>

#ifdef QMAKE_BUILD
#include "customformatter.h"
Q_DECLARE_METATYPE(CustomFormatter *)
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef QMAKE_BUILD
    qmlRegisterType<CustomFormatter>("AxisHandling", 1, 0, "CustomFormatter");
#endif

    QQuickView viewer;

    // The following are needed to make examples run without having to install the module
    // in desktop environments.
#ifdef Q_OS_WIN
    QString extraImportPath(QStringLiteral("%1/../../../../%2"));
#else
    QString extraImportPath(QStringLiteral("%1/../../../%2"));
#endif
    viewer.engine()->addImportPath(extraImportPath.arg(QGuiApplication::applicationDirPath(),
                                      QString::fromLatin1("qml")));
    QObject::connect(viewer.engine(), &QQmlEngine::quit, &viewer, &QWindow::close);

    viewer.setTitle(QStringLiteral("Axis Handling"));

    viewer.setSource(QUrl("qrc:/qml/axishandling/main.qml"));
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);
    viewer.show();

    return app.exec();
}
