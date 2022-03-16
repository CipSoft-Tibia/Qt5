/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

#include <QQmlEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QtQuick/QQuickView>


/*
   This example illustrates using a QQmlNetworkAccessManagerFactory to
   create a QNetworkAccessManager with a proxy.

   Usage:
     networkaccessmanagerfactory [-host <proxy> -port <port>] [file]
*/

#if QT_CONFIG(networkproxy)
static QString proxyHost;
static int proxyPort = 0;
#endif // networkproxy

class MyNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override;
};

QNetworkAccessManager *MyNetworkAccessManagerFactory::create(QObject *parent)
{
    QNetworkAccessManager *nam = new QNetworkAccessManager(parent);
#if QT_CONFIG(networkproxy)
    if (!proxyHost.isEmpty()) {
        qDebug() << "Created QNetworkAccessManager using proxy" << (proxyHost + ":" + QString::number(proxyPort));
        QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy, proxyHost, proxyPort);
        nam->setProxy(proxy);
    }
#endif // networkproxy

    return nam;
}

int main(int argc, char ** argv)
{
    QUrl source("qrc:view.qml");

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
#if QT_CONFIG(networkproxy)
    QCommandLineOption proxyHostOption("host", "The proxy host to use.", "host");
    parser.addOption(proxyHostOption);
    QCommandLineOption proxyPortOption("port", "The proxy port to use.", "port", "0");
    parser.addOption(proxyPortOption);
#endif // networkproxy
    parser.addPositionalArgument("file", "The file to use.");
    QCommandLineOption helpOption = parser.addHelpOption();
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    QStringList arguments = QCoreApplication::arguments();
    if (!parser.parse(arguments)) {
        qWarning() << parser.helpText() << '\n' << parser.errorText();
        exit(1);
    }
    if (parser.isSet(helpOption)) {
        qWarning() << parser.helpText();
        exit(0);
    }
#if QT_CONFIG(networkproxy)
    if (parser.isSet(proxyHostOption))
        proxyHost = parser.value(proxyHostOption);
    if (parser.isSet(proxyPortOption)) {
        bool ok = true;
        proxyPort = parser.value(proxyPortOption).toInt(&ok);
        if (!ok || proxyPort < 1 || proxyPort > 65535) {
            qWarning() << parser.helpText() << "\nNo valid port given. It should\
                          be a number between 1 and 65535";
            exit(1);
        }
    }
#endif // networkproxy
    if (parser.positionalArguments().count() == 1)
        source = QUrl::fromLocalFile(parser.positionalArguments().first());

    QQuickView view;
    MyNetworkAccessManagerFactory networkManagerFactory;
    view.engine()->setNetworkAccessManagerFactory(&networkManagerFactory);

    view.setSource(source);
    view.show();

    return app.exec();
}

