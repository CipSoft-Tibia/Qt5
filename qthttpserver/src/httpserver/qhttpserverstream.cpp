// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhttpserverstream_p.h"

#include <QtNetwork/qtcpsocket.h>

#if QT_CONFIG(ssl)
#include <QtNetwork/qsslsocket.h>
#endif

QT_BEGIN_NAMESPACE

static QHttpServerParser initParserFromSocket(QIODevice *socket)
{
    QTcpSocket *tcpSocket = qobject_cast<QTcpSocket *>(socket);
    if (tcpSocket) {
        return QHttpServerParser(tcpSocket->peerAddress(), tcpSocket->peerPort(),
                                 tcpSocket->localAddress(), tcpSocket->localPort());
    }
    return QHttpServerParser(QHostAddress::LocalHost, 0, QHostAddress::LocalHost, 0);
}

#if QT_CONFIG(ssl)
static QSslConfiguration initSslConfigurationFromSocket(QIODevice *socket)
{
    if (auto *ssl = qobject_cast<const QSslSocket *>(socket))
        return ssl->sslConfiguration();

    return QSslConfiguration();
}
#endif

QHttpServerStream::QHttpServerStream(QIODevice *socket, QObject *parent)
    : QObject(parent)
    , parser(initParserFromSocket(socket))
#if QT_CONFIG(ssl)
    , sslConfiguration(initSslConfigurationFromSocket(socket))
#endif
{
}

QT_END_NAMESPACE
