// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qtnetwork-config.h>

#ifndef QT_NO_HTTP

#include <qabstractoauth.h>
#include <qoauthhttpserverreplyhandler.h>
#include "qabstractoauthreplyhandler_p.h"

#include <private/qoauthhttpserverreplyhandler_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>

#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qnetworkreply.h>

#include <cctype>
#include <cstring>
#include <functional>

QT_BEGIN_NAMESPACE

QOAuthHttpServerReplyHandlerPrivate::QOAuthHttpServerReplyHandlerPrivate(
        QOAuthHttpServerReplyHandler *p) :
    text(QObject::tr("Callback received. Feel free to close this page.")), q_ptr(p)
{
    QObject::connect(&httpServer, &QTcpServer::newConnection,
                     [this]() { _q_clientConnected(); });
}

QOAuthHttpServerReplyHandlerPrivate::~QOAuthHttpServerReplyHandlerPrivate()
{
    if (httpServer.isListening())
        httpServer.close();
}

void QOAuthHttpServerReplyHandlerPrivate::_q_clientConnected()
{
    QTcpSocket *socket = httpServer.nextPendingConnection();

    QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    QObject::connect(socket, &QTcpSocket::readyRead,
                     [this, socket]() { _q_readData(socket); });
}

void QOAuthHttpServerReplyHandlerPrivate::_q_readData(QTcpSocket *socket)
{
    if (!clients.contains(socket))
        clients[socket].port = httpServer.serverPort();

    QHttpRequest *request = &clients[socket];
    bool error = false;

    if (Q_LIKELY(request->state == QHttpRequest::State::ReadingMethod))
        if (Q_UNLIKELY(error = !request->readMethod(socket)))
            qCWarning(lcReplyHandler, "Invalid Method");

    if (Q_LIKELY(!error && request->state == QHttpRequest::State::ReadingUrl))
        if (Q_UNLIKELY(error = !request->readUrl(socket)))
            qCWarning(lcReplyHandler, "Invalid URL");

    if (Q_LIKELY(!error && request->state == QHttpRequest::State::ReadingStatus))
        if (Q_UNLIKELY(error = !request->readStatus(socket)))
            qCWarning(lcReplyHandler, "Invalid Status");

    if (Q_LIKELY(!error && request->state == QHttpRequest::State::ReadingHeader))
        if (Q_UNLIKELY(error = !request->readHeader(socket)))
            qCWarning(lcReplyHandler, "Invalid Header");

    if (error) {
        socket->disconnectFromHost();
        clients.remove(socket);
    } else if (!request->url.isEmpty()) {
        Q_ASSERT(request->state != QHttpRequest::State::ReadingUrl);
        _q_answerClient(socket, request->url);
        clients.remove(socket);
    }
}

void QOAuthHttpServerReplyHandlerPrivate::_q_answerClient(QTcpSocket *socket, const QUrl &url)
{
    Q_Q(QOAuthHttpServerReplyHandler);
    if (!url.path().startsWith(QLatin1String("/") + path)) {
        qCWarning(lcReplyHandler, "Invalid request: %s", qPrintable(url.toString()));
    } else {
        QVariantMap receivedData;
        const QUrlQuery query(url.query());
        const auto items = query.queryItems();
        for (auto it = items.begin(), end = items.end(); it != end; ++it)
            receivedData.insert(it->first, it->second);
        Q_EMIT q->callbackReceived(receivedData);

        const QByteArray html = QByteArrayLiteral("<html><head><title>") +
                qApp->applicationName().toUtf8() +
                QByteArrayLiteral("</title></head><body>") +
                text.toUtf8() +
                QByteArrayLiteral("</body></html>");

        const QByteArray htmlSize = QByteArray::number(html.size());
        const QByteArray replyMessage = QByteArrayLiteral("HTTP/1.0 200 OK \r\n"
                                                          "Content-Type: text/html; "
                                                          "charset=\"utf-8\"\r\n"
                                                          "Content-Length: ") + htmlSize +
                QByteArrayLiteral("\r\n\r\n") +
                html;

        socket->write(replyMessage);
    }
    socket->disconnectFromHost();
}

bool QOAuthHttpServerReplyHandlerPrivate::QHttpRequest::readMethod(QTcpSocket *socket)
{
    bool finished = false;
    while (socket->bytesAvailable() && !finished) {
        char c;
        socket->getChar(&c);
        if (std::isupper(c) && fragment.size() < 6)
            fragment += c;
        else
            finished = true;
    }
    if (finished) {
        if (fragment == "HEAD")
            method = Method::Head;
        else if (fragment == "GET")
            method = Method::Get;
        else if (fragment == "PUT")
            method = Method::Put;
        else if (fragment == "POST")
            method = Method::Post;
        else if (fragment == "DELETE")
            method = Method::Delete;
        else
            qCWarning(lcReplyHandler, "Invalid operation %s", fragment.data());

        state = State::ReadingUrl;
        fragment.clear();

        return method != Method::Unknown;
    }
    return true;
}

bool QOAuthHttpServerReplyHandlerPrivate::QHttpRequest::readUrl(QTcpSocket *socket)
{
    bool finished = false;
    while (socket->bytesAvailable() && !finished) {
        char c;
        socket->getChar(&c);
        if (std::isspace(c))
            finished = true;
        else
            fragment += c;
    }
    if (finished) {
        if (!fragment.startsWith("/")) {
            qCWarning(lcReplyHandler, "Invalid URL path %s", fragment.constData());
            return false;
        }
        url.setUrl(QStringLiteral("http://127.0.0.1:") + QString::number(port) +
                   QString::fromUtf8(fragment));
        state = State::ReadingStatus;
        if (!url.isValid()) {
            qCWarning(lcReplyHandler, "Invalid URL %s", fragment.constData());
            return false;
        }
        fragment.clear();
        return true;
    }
    return true;
}

bool QOAuthHttpServerReplyHandlerPrivate::QHttpRequest::readStatus(QTcpSocket *socket)
{
    bool finished = false;
    while (socket->bytesAvailable() && !finished) {
        char c;
        socket->getChar(&c);
        fragment += c;
        if (fragment.endsWith("\r\n")) {
            finished = true;
            fragment.resize(fragment.size() - 2);
        }
    }
    if (finished) {
        if (!std::isdigit(fragment.at(fragment.size() - 3)) ||
                !std::isdigit(fragment.at(fragment.size() - 1))) {
            qCWarning(lcReplyHandler, "Invalid version");
            return false;
        }
        version = qMakePair(fragment.at(fragment.size() - 3) - '0',
                            fragment.at(fragment.size() - 1) - '0');
        state = State::ReadingHeader;
        fragment.clear();
    }
    return true;
}

bool QOAuthHttpServerReplyHandlerPrivate::QHttpRequest::readHeader(QTcpSocket *socket)
{
    while (socket->bytesAvailable()) {
        char c;
        socket->getChar(&c);
        fragment += c;
        if (fragment.endsWith("\r\n")) {
            if (fragment == "\r\n") {
                state = State::ReadingBody;
                fragment.clear();
                return true;
            } else {
                fragment.chop(2);
                const int index = fragment.indexOf(':');
                if (index == -1)
                    return false;

                const QByteArray key = fragment.mid(0, index).trimmed();
                const QByteArray value = fragment.mid(index + 1).trimmed();
                headers.insert(key, value);
                fragment.clear();
            }
        }
    }
    return false;
}

QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler(QObject *parent) :
    QOAuthHttpServerReplyHandler(QHostAddress::Any, 0, parent)
{}

QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler(quint16 port, QObject *parent) :
    QOAuthHttpServerReplyHandler(QHostAddress::Any, port, parent)
{}

QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler(const QHostAddress &address,
                                                           quint16 port, QObject *parent) :
    QOAuthOobReplyHandler(parent),
    d_ptr(new QOAuthHttpServerReplyHandlerPrivate(this))
{
    listen(address, port);
}

QOAuthHttpServerReplyHandler::~QOAuthHttpServerReplyHandler()
{}

QString QOAuthHttpServerReplyHandler::callback() const
{
    Q_D(const QOAuthHttpServerReplyHandler);

    Q_ASSERT(d->httpServer.isListening());
    const QUrl url(QString::fromLatin1("http://127.0.0.1:%1/%2")
                   .arg(d->httpServer.serverPort()).arg(d->path));
    return url.toString(QUrl::EncodeDelimiters);
}

QString QOAuthHttpServerReplyHandler::callbackPath() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->path;
}

void QOAuthHttpServerReplyHandler::setCallbackPath(const QString &path)
{
    Q_D(QOAuthHttpServerReplyHandler);

    QString copy = path;
    while (copy.startsWith(QLatin1Char('/')))
        copy = copy.mid(1);

    d->path = copy;
}

QString QOAuthHttpServerReplyHandler::callbackText() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->text;
}

void QOAuthHttpServerReplyHandler::setCallbackText(const QString &text)
{
    Q_D(QOAuthHttpServerReplyHandler);
    d->text = text;
}

quint16 QOAuthHttpServerReplyHandler::port() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->httpServer.serverPort();
}

bool QOAuthHttpServerReplyHandler::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QOAuthHttpServerReplyHandler);
    return d->httpServer.listen(address, port);
}

void QOAuthHttpServerReplyHandler::close()
{
    Q_D(QOAuthHttpServerReplyHandler);
    return d->httpServer.close();
}

bool QOAuthHttpServerReplyHandler::isListening() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->httpServer.isListening();
}

QT_END_NAMESPACE

#include "moc_qoauthhttpserverreplyhandler.cpp"

#endif // QT_NO_HTTP
