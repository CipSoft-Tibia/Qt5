/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
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

#include <QCoreApplication>
#include <QHostAddress>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslKey>
#include <QTimer>
#include "rep_timemodel_replica.h"

#include <QRemoteObjectNode>

class tester : public QObject
{
    Q_OBJECT
public:
    tester() : QObject(nullptr)
    {
        QRemoteObjectNode m_client;
        auto socket = setupConnection();
        connect(socket, &QSslSocket::errorOccurred,
                socket, [](QAbstractSocket::SocketError error){
            qDebug() << "QSslSocket::error" << error;
        }) ;
        m_client.addClientSideConnection(socket);

        ptr1.reset(m_client.acquire< MinuteTimerReplica >());
        ptr2.reset(m_client.acquire< MinuteTimerReplica >());
        ptr3.reset(m_client.acquire< MinuteTimerReplica >());
        QTimer::singleShot(0, this, &tester::clear);
        QTimer::singleShot(1, this, &tester::clear);
        QTimer::singleShot(10000, this, &tester::clear);
        QTimer::singleShot(11000, this, &tester::clear);
    }
public slots:
    void clear()
    {
        static int i = 0;
        if (i == 0) {
            i++;
            ptr1.reset();
        } else if (i == 1) {
            i++;
            ptr2.reset();
        } else if (i == 2) {
            i++;
            ptr3.reset();
        } else {
            qApp->quit();
        }
    }

private:
    QScopedPointer<MinuteTimerReplica> ptr1, ptr2, ptr3;

    QSslSocket *setupConnection()
    {
        auto socketClient = new QSslSocket;
        socketClient->setLocalCertificate(QStringLiteral(":/sslcert/client.crt"));
        socketClient->setPrivateKey(QStringLiteral(":/sslcert/client.key"));
        socketClient->setPeerVerifyMode(QSslSocket::VerifyPeer);
        socketClient->connectToHostEncrypted(QStringLiteral("127.0.0.1"), 65511);
        if (!socketClient->waitForEncrypted(-1)) {
            qWarning("Failed to connect to server %s",
                   qPrintable(socketClient->errorString()));
            exit(0);
        }
        return socketClient;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    auto config = QSslConfiguration::defaultConfiguration();
    config.setCaCertificates(QSslCertificate::fromPath(QStringLiteral(":/sslcert/rootCA.pem")));
    QSslConfiguration::setDefaultConfiguration(config);

    tester t;
    return a.exec();
}

#include "main.moc"
