/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QTextStream>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>

static bool parseArgs(QStringList& args, QVariantMap& parameters)
{

    while (!args.isEmpty()) {

        QString param = args.takeFirst();

        if (param.startsWith("--help")) {
            QTextStream out(stdout);
            out << "Usage: " << endl;
            out << "--plugin.<parameter_name> <parameter_value>    -  Sets parameter = value for plugin" << endl;
            out.flush();
            return true;
        }

        if (param.startsWith("--plugin.")) {

            param.remove(0, 9);

            if (args.isEmpty() || args.first().startsWith("--")) {
                parameters[param] = true;
            } else {

                QString value = args.takeFirst();

                if (value == "true" || value == "on" || value == "enabled") {
                    parameters[param] = true;
                } else if (value == "false" || value == "off"
                           || value == "disable") {
                    parameters[param] = false;
                } else {
                    parameters[param] = value;
                }
            }
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);

    QVariantMap parameters;
    QStringList args(QCoreApplication::arguments());

    // Fetch tokens from the environment, if present
    const QByteArray mapboxMapID = qgetenv("MAPBOX_MAP_ID");
    const QByteArray mapboxAccessToken = qgetenv("MAPBOX_ACCESS_TOKEN");
    const QByteArray hereAppID = qgetenv("HERE_APP_ID");
    const QByteArray hereToken = qgetenv("HERE_TOKEN");
    const QByteArray esriToken = qgetenv("ESRI_TOKEN");

    if (!mapboxMapID.isEmpty())
        parameters["mapbox.map_id"] = QString::fromLocal8Bit(mapboxMapID);
    if (!mapboxAccessToken.isEmpty())
        parameters["mapbox.access_token"] = QString::fromLocal8Bit(mapboxAccessToken);
    if (!hereAppID.isEmpty())
        parameters["here.app_id"] = QString::fromLocal8Bit(hereAppID);
    if (!hereToken.isEmpty())
        parameters["here.token"] = QString::fromLocal8Bit(hereToken);
    if (!esriToken.isEmpty())
        parameters["esri.token"] = QString::fromLocal8Bit(esriToken);

    if (parseArgs(args, parameters))
        return 0;

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(":/imports"));
    engine.load(QUrl(QStringLiteral("qrc:///places.qml")));
    QObject::connect(&engine, SIGNAL(quit()), qApp, SLOT(quit()));

    QObject *item = engine.rootObjects().first();
    Q_ASSERT(item);

    QMetaObject::invokeMethod(item, "initializeProviders",
                                 Q_ARG(QVariant, QVariant::fromValue(parameters)));

    return application.exec();
}
