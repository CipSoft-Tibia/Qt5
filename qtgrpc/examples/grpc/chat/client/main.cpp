// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>
#include <QIcon>
#include <QFont>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QGuiApplication::setWindowIcon(QIcon(":/qt_logo_green_64x64px.png"));

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [](){
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.loadFromModule("qtgrpc.examples.chat", "Main");

    return app.exec();
}
