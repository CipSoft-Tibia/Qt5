// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTHTTPSERVER_H
#define QABSTRACTHTTPSERVER_H

#include <QtCore/qobject.h>

#include <QtHttpServer/qthttpserverglobal.h>

#include <QtNetwork/qhostaddress.h>

#if QT_CONFIG(ssl)
#include <QtNetwork/qssl.h>
#endif

#if defined(QT_WEBSOCKETS_LIB)
#include <QtWebSockets/qwebsocket.h>
#endif // defined(QT_WEBSOCKETS_LIB)

#include <memory>

QT_BEGIN_NAMESPACE

class QHttpServerRequest;
class QHttpServerResponder;
class QSslCertificate;
class QSslConfiguration;
class QSslKey;
class QTcpServer;

class QAbstractHttpServerPrivate;
class Q_HTTPSERVER_EXPORT QAbstractHttpServer : public QObject
{
    Q_OBJECT
    friend class QHttpServerStream;

public:
    explicit QAbstractHttpServer(QObject *parent = nullptr);
    ~QAbstractHttpServer() override;

    quint16 listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    QList<quint16> serverPorts();

    void bind(QTcpServer *server = nullptr);
    QList<QTcpServer *> servers() const;

#if QT_CONFIG(ssl)
    void sslSetup(const QSslCertificate &certificate, const QSslKey &privateKey,
                  QSsl::SslProtocol protocol = QSsl::SecureProtocols);
    void sslSetup(const QSslConfiguration &sslConfiguration);
#endif

#if defined(QT_WEBSOCKETS_LIB)
Q_SIGNALS:
    void newWebSocketConnection();

public:
    bool hasPendingWebSocketConnections() const;
    std::unique_ptr<QWebSocket> nextPendingWebSocketConnection();
#endif // defined(QT_WEBSOCKETS_LIB)

protected:
    QAbstractHttpServer(QAbstractHttpServerPrivate &dd, QObject *parent = nullptr);

    virtual bool handleRequest(const QHttpServerRequest &request,
                               QHttpServerResponder &responder) = 0;
    virtual void missingHandler(const QHttpServerRequest &request,
                                QHttpServerResponder &&responder) = 0;

private:
    Q_DECLARE_PRIVATE(QAbstractHttpServer)
};

QT_END_NAMESPACE

#endif // QABSTRACTHTTPSERVER_H
