// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/qguiapplication.h>
#include <QtQuick/QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("textalignment-manual-test");
    QCoreApplication::setOrganizationName("QtProject");
    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:/qt/qml/textalignment/textalignment.qml")));
    view.show();

    return app.exec();
}
