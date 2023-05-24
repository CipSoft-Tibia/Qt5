// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qhttpserverstream_p.h>

#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qabstracthttpserver.h>
#include <QtHttpServer/qhttpserverresponder.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qtcpsocket.h>

#include <private/qhttpserverrequest_p.h>
#include <private/qabstracthttpserver_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHttpServerStream, "qt.httpserver.stream")

void QHttpServerStream::handleReadyRead()
{
    if (handlingRequest)
        return;

    if (!socket->isTransactionStarted())
        socket->startTransaction();

    if (!request.d->parse(socket)) {
        socket->disconnectFromHost();
        return;
    }

    if (request.d->state != QHttpServerRequestPrivate::State::AllDone)
        return; // Partial read

    qCDebug(lcHttpServerStream) << "Request:" << request;

    QHttpServerResponder responder(this);

#if defined(QT_WEBSOCKETS_LIB)
    if (request.d->upgrade) { // Upgrade
        const auto &upgradeValue = request.value(QByteArrayLiteral("upgrade"));
        if (upgradeValue.compare(QByteArrayLiteral("websocket"), Qt::CaseInsensitive) == 0) {
            static const auto signal =
                    QMetaMethod::fromSignal(&QAbstractHttpServer::newWebSocketConnection);
            if (server->isSignalConnected(signal) && server->handleRequest(request, responder)) {
                // Socket will now be managed by websocketServer
                socket->disconnect();
                socket->rollbackTransaction();
                server->d_func()->websocketServer.handleConnection(socket);
                Q_EMIT socket->readyRead();
            } else {
                qWarning(lcHttpServerStream,
                         "WebSocket received but no slots connected to "
                         "QWebSocketServer::newConnection or request not handled");
                server->missingHandler(request, std::move(responder));
                socket->disconnectFromHost();
            }
            return;
        }
    }
#endif

    socket->commitTransaction();

    if (!server->handleRequest(request, responder))
        server->missingHandler(request, std::move(responder));

    if (handlingRequest)
        disconnect(socket, &QTcpSocket::readyRead, this, &QHttpServerStream::handleReadyRead);
    else if (socket->bytesAvailable() > 0)
        QMetaObject::invokeMethod(socket, &QAbstractSocket::readyRead, Qt::QueuedConnection);
}

void QHttpServerStream::socketDisconnected()
{
    if (!handlingRequest)
        deleteLater();
}

QHttpServerStream::QHttpServerStream(QAbstractHttpServer *server, QTcpSocket *socket)
    : QObject(server),
      server(server),
      socket(socket),
      request(socket->peerAddress(), socket->peerPort(), socket->localAddress(),
              socket->localPort())
{
    socket->setParent(this);

    qCDebug(lcHttpServerStream) << "Connection from:" << socket->peerAddress();
    connect(socket, &QTcpSocket::readyRead, this, &QHttpServerStream::handleReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &QHttpServerStream::socketDisconnected);
}

void QHttpServerStream::write(const QByteArray &ba)
{
    Q_ASSERT(QThread::currentThread() == thread());
    socket->write(ba);
}

void QHttpServerStream::write(const char *body, qint64 size)
{
    Q_ASSERT(QThread::currentThread() == thread());
    socket->write(body, size);
}

void QHttpServerStream::responderDestroyed()
{
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(handlingRequest);
    handlingRequest = false;

    if (socket->state() != QAbstractSocket::ConnectedState) {
        deleteLater();
    } else {
        connect(socket, &QTcpSocket::readyRead, this, &QHttpServerStream::handleReadyRead);
        QMetaObject::invokeMethod(socket, &QAbstractSocket::readyRead, Qt::QueuedConnection);
    }
}

QT_END_NAMESPACE
