// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQuickView>

int main(int argc, char **argv)
{
    QGuiApplication app(argc,argv);
    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:///places_list.qml")));
    view.show();
    return app.exec();
}
