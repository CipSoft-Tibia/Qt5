// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qhttpserverstream_p.h>

#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qabstracthttpserver.h>
#include <QtHttpServer/qhttpserverresponder.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qlocalsocket.h>
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
        if (tcpSocket)
            tcpSocket->disconnectFromHost();
#if QT_CONFIG(localserver)
        else if (localSocket)
            localSocket->disconnectFromServer();
#endif
        return;
    }

    if (request.d->state != QHttpServerRequestPrivate::State::AllDone)
        return; // Partial read

    qCDebug(lcHttpServerStream) << "Request:" << request;

    QHttpServerResponder responder(this);

#if defined(QT_WEBSOCKETS_LIB)
    if (auto *tcpSocket = qobject_cast<QTcpSocket*>(socket)) {
        if (request.d->upgrade) { // Upgrade
            const auto &upgradeValue = request.value(QByteArrayLiteral("upgrade"));
            if (upgradeValue.compare(QByteArrayLiteral("websocket"), Qt::CaseInsensitive) == 0) {
                static const auto signal =
                        QMetaMethod::fromSignal(&QAbstractHttpServer::newWebSocketConnection);
                if (server->isSignalConnected(signal)
                    && server->handleRequest(request, responder)) {
                    // Socket will now be managed by websocketServer
                    socket->disconnect();
                    socket->rollbackTransaction();
                    server->d_func()->websocketServer.handleConnection(tcpSocket);
                    Q_EMIT socket->readyRead();
                } else {
                    qWarning(lcHttpServerStream,
                            "WebSocket received but no slots connected to "
                            "QWebSocketServer::newConnection or request not handled");
                    server->missingHandler(request, std::move(responder));
                    tcpSocket->disconnectFromHost();
                }
                return;
            }
        }
    }
#endif // QT_WEBSOCKETS_LIB

    socket->commitTransaction();

    if (!server->handleRequest(request, responder))
        server->missingHandler(request, std::move(responder));

    if (handlingRequest)
        disconnect(socket, &QIODevice::readyRead, this, &QHttpServerStream::handleReadyRead);
    else if (socket->bytesAvailable() > 0)
        QMetaObject::invokeMethod(socket, &QIODevice::readyRead, Qt::QueuedConnection);
}

void QHttpServerStream::socketDisconnected()
{
    if (!handlingRequest)
        deleteLater();
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

QHttpServerStream::QHttpServerStream(QAbstractHttpServer *server, QIODevice *socket)
    : QObject(server),
      server(server),
      socket(socket),
      tcpSocket(qobject_cast<QTcpSocket *>(socket)),
#if QT_CONFIG(localserver)
      localSocket(qobject_cast<QLocalSocket*>(socket)),
#endif
      request(initRequestFromSocket(tcpSocket))
{
    socket->setParent(this);

    if (tcpSocket) {
        qCDebug(lcHttpServerStream) << "Connection from:" << tcpSocket->peerAddress();
        connect(socket, &QTcpSocket::readyRead, this, &QHttpServerStream::handleReadyRead);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &QHttpServerStream::socketDisconnected);
#if QT_CONFIG(localserver)
    } else if (localSocket) {
        qCDebug(lcHttpServerStream) << "Connection from:" << localSocket->serverName();
        connect(socket, &QLocalSocket::readyRead, this, &QHttpServerStream::handleReadyRead);
        connect(localSocket, &QLocalSocket::disconnected, this, &QHttpServerStream::socketDisconnected);
#endif
    }
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

    if (tcpSocket) {
        if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
            deleteLater();
        } else {
            connect(tcpSocket, &QTcpSocket::readyRead, this, &QHttpServerStream::handleReadyRead);
            QMetaObject::invokeMethod(tcpSocket, &QTcpSocket::readyRead, Qt::QueuedConnection);
        }
#if QT_CONFIG(localserver)
    } else if (localSocket) {
        if (localSocket->state() != QLocalSocket::ConnectedState) {
            deleteLater();
        } else {
            connect(localSocket, &QLocalSocket::readyRead,
                    this, &QHttpServerStream::handleReadyRead);
            QMetaObject::invokeMethod(localSocket, &QLocalSocket::readyRead, Qt::QueuedConnection);
        }
#endif
    }
}

QT_END_NAMESPACE
