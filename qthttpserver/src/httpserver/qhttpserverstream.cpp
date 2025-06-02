// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qhttpserverstream_p.h"

#include <QtNetwork/qtcpsocket.h>

#if QT_CONFIG(ssl)
#include <QtNetwork/qsslsocket.h>
#endif

QT_BEGIN_NAMESPACE

QHttpServerStream::QHttpServerStream(QObject *parent)
    : QObject(parent)
{
}

QHttpServerRequest QHttpServerStream::initRequestFromSocket(QTcpSocket *tcpSocket)
{
    if (tcpSocket) {
#if QT_CONFIG(ssl)
        if (auto *ssl = qobject_cast<const QSslSocket *>(tcpSocket)) {
            return QHttpServerRequest(ssl->peerAddress(), ssl->peerPort(),
                                      ssl->localAddress(), ssl->localPort(),
                                      ssl->sslConfiguration());
        }
#endif
        return QHttpServerRequest(tcpSocket->peerAddress(), tcpSocket->peerPort(),
                                  tcpSocket->localAddress(), tcpSocket->localPort());
    }

    return QHttpServerRequest(QHostAddress::LocalHost, 0, QHostAddress::LocalHost, 0);
}

QT_END_NAMESPACE
