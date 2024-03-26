// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <QtGraphs/qutils.h>
//! [0]

#include <QtGui/qguiapplication.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlengine.h>

#ifdef QMAKE_BUILD
#include "datasource.h"
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef QMAKE_BUILD
    qmlRegisterType<DataSource>("SurfaceGallery", 1, 0, "DataSource");
#endif

    QQuickView viewer;

    //! [1]
    // Enable antialiasing in direct rendering mode
    viewer.setFormat(qDefaultSurfaceFormat(true));
    //! [1]

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

    viewer.setTitle(QStringLiteral("Surface Graph Gallery"));

    viewer.setSource(QUrl("qrc:/qml/surfacegallery/main.qml"));
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);
    viewer.show();

    return app.exec();
}
