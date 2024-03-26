// Copyright (C) 2017 Juan José Casafranca.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QQuickItem>
#include <QQuickView>
#include <Qt3DQuick/QQmlAspectEngine>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlContext>

#include <Qt3DRender>
#include <iostream>


int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QQuickView view;

    view.setSource(QUrl("qrc:/main.qml"));

    QObject *rootObject = view.rootObject();
    Qt3DCore::QBuffer *buffer = rootObject->findChild<Qt3DCore::QBuffer*>("buffer");

    QObject::connect(buffer, &Qt3DCore::QBuffer::dataChanged,
                     [=](const QByteArray &bytes) {
        //I know I'm receiving int data
        const uint *data = reinterpret_cast<const uint*>(bytes.data());
        qDebug() << "Data received";
        for (uint i = 0; i < 1024; ++i)
            qDebug() << data[i];
    }
    );

    view.show();

    return app.exec();
}
