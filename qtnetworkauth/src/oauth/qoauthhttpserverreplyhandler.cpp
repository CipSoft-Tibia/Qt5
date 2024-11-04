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
#include <QtCore/private/qlocale_p.h>

#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qnetworkreply.h>

#include <cstring>
#include <functional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QOAuthHttpServerReplyHandler
    \inmodule QtNetworkAuth
    \ingroup oauth
    \since 5.8

    \brief Handles loopback redirects by setting up a local HTTP server.

    This class serves as a reply handler for
    \l {https://datatracker.ietf.org/doc/html/rfc6749}{OAuth 2.0} authorization
    processes that use
    \l {https://datatracker.ietf.org/doc/html/rfc8252#section-7.3}{loopback redirection}.

    The \l {https://datatracker.ietf.org/doc/html/rfc6749#section-3.1.2}
    {redirect URI} is where the authorization server redirects the
    user-agent (typically, and preferably, the system browser) once
    the authorization part of the flow is complete. Loopback redirect
    URIs use \c http as the scheme and either \e localhost or an IP
    address literal as the host (see \l {IPv4 and IPv6}).

    QOAuthHttpServerReplyHandler sets up a localhost server. Once the
    authorization server redirects the browser to this localhost address,
    the reply handler parses the redirection URI query parameters,
    and then signals authorization completion with
    \l {QAbstractOAuthReplyHandler::callbackReceived}{a signal}.

    To handle other redirect URI schemes, see QOAuthUriSchemeReplyHandler.

    The following code illustrates the usage. First, the needed variables:

    \snippet src_oauth_replyhandlers.cpp httpserver-variables

    Followed up by the OAuth setup (error handling omitted for brevity):

    \snippet src_oauth_replyhandlers.cpp httpserver-oauth-setup

    Finally, we then set up the URI scheme reply-handler:

    \snippet src_oauth_replyhandlers.cpp httpserver-handler-setup

    \section1 IPv4 and IPv6

    Currently if the handler is a loopback address, IPv4 any address,
    or IPv6 any address, the used callback is in the form of
    \e {http://localhost:{port}/{path}}. Otherwise, for specific
    IP addresses, the actual IP literal is used. For instance
    \e {http://192.168.0.2:{port}/{path}} in the case of IPv4.
*/
QOAuthHttpServerReplyHandlerPrivate::QOAuthHttpServerReplyHandlerPrivate(
        QOAuthHttpServerReplyHandler *p) :
    text(QObject::tr("Callback received. Feel free to close this page.")), path(u'/'), q_ptr(p)
{
    QObject::connect(&httpServer, &QTcpServer::newConnection, q_ptr,
                     [this]() { _q_clientConnected(); });
}

QOAuthHttpServerReplyHandlerPrivate::~QOAuthHttpServerReplyHandlerPrivate()
{
    if (httpServer.isListening())
        httpServer.close();
}

QString QOAuthHttpServerReplyHandlerPrivate::callback() const
{
    QUrl url;
    url.setScheme(u"http"_s);
    url.setPort(callbackPort);
    url.setPath(path);

    // convert Any and Localhost addresses to "localhost"
    if (callbackAddress.isLoopback() || callbackAddress == QHostAddress::AnyIPv4
        || callbackAddress == QHostAddress::Any || callbackAddress == QHostAddress::AnyIPv6) {
        url.setHost(u"localhost"_s);
    } else {
        url.setHost(callbackAddress.toString());
    }

    return url.toString(QUrl::EncodeSpaces | QUrl::EncodeUnicode | QUrl::EncodeDelimiters
                            | QUrl::EncodeReserved);
}

void QOAuthHttpServerReplyHandlerPrivate::_q_clientConnected()
{
    QTcpSocket *socket = httpServer.nextPendingConnection();

    QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    QObject::connect(socket, &QTcpSocket::readyRead, q_ptr,
                     [this, socket]() { _q_readData(socket); });
}

void QOAuthHttpServerReplyHandlerPrivate::_q_readData(QTcpSocket *socket)
{
    QHttpRequest *request = nullptr;
    if (auto it = clients.find(socket); it == clients.end()) {
        request = &clients[socket];     // insert it
        request->port = httpServer.serverPort();
    } else {
        request = &*it;
    }

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
    if (url.path() != path) {
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
        if (ascii_isspace(c))
            finished = true;
        else
            fragment += c;
    }
    if (finished) {
        url = QUrl::fromEncoded(fragment);
        state = State::ReadingStatus;

        if (!fragment.startsWith(u'/') || !url.isValid() || !url.scheme().isNull()
                || !url.host().isNull()) {
            qCWarning(lcReplyHandler, "Invalid request: %s", fragment.constData());
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

/*!
    Constructs a QOAuthHttpServerReplyHandler object using \a parent as a
    parent object. Calls \l {listen()} with port \c 0 and address
    \l {QHostAddress::SpecialAddress}{Null}.

    \sa listen()
*/
QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler(QObject *parent) :
    QOAuthHttpServerReplyHandler(QHostAddress::Null, 0, parent)
{}

/*!
    Constructs a QOAuthHttpServerReplyHandler object using \a parent as a
    parent object. Calls \l {listen()} with \a port and address
    \l {QHostAddress::SpecialAddress}{Null}.

    \sa listen()
*/
QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler(quint16 port, QObject *parent) :
    QOAuthHttpServerReplyHandler(QHostAddress::Null, port, parent)
{}

/*!
    Constructs a QOAuthHttpServerReplyHandler object using \a parent as a
    parent object. Calls \l {listen()} with \a address and \a port.

    \sa listen()
*/
QOAuthHttpServerReplyHandler::QOAuthHttpServerReplyHandler(const QHostAddress &address,
                                                           quint16 port, QObject *parent) :
    QOAuthOobReplyHandler(parent),
    d_ptr(new QOAuthHttpServerReplyHandlerPrivate(this))
{
    listen(address, port);
}

/*!
    Destroys the QOAuthHttpServerReplyHandler object.
    Stops listening for connections / redirections.

    \sa close()
*/
QOAuthHttpServerReplyHandler::~QOAuthHttpServerReplyHandler()
{}

QString QOAuthHttpServerReplyHandler::callback() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->callback();
}

/*!
    Returns the path that is used as the path component of the
    \l callback() / \l{https://datatracker.ietf.org/doc/html/rfc6749#section-3.1.2}
    {OAuth2 redirect_uri parameter}.

    \sa setCallbackPath()
*/
QString QOAuthHttpServerReplyHandler::callbackPath() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->path;
}

/*!
    Sets \a path to be used as the path component of the
    \l callback().

    \sa callbackPath()
*/
void QOAuthHttpServerReplyHandler::setCallbackPath(const QString &path)
{
    Q_D(QOAuthHttpServerReplyHandler);
    // pass through QUrl to ensure normalization
    QUrl url;
    url.setPath(path);
    d->path = url.path(QUrl::FullyEncoded);
    if (d->path.isEmpty())
        d->path = u'/';
}

/*!
    Returns the text that is used in response to the
    redirection at the end of the authorization stage.

    The text is wrapped in a simple HTML page, and displayed to
    the user by the browser / user-agent which did the redirection.

    The default text is
    \badcode
    Callback received. Feel free to close this page.
    \endcode

    \sa setCallbackText()
*/
QString QOAuthHttpServerReplyHandler::callbackText() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->text;
}

/*!
    Sets \a text to be used in response to the
    redirection at the end of the authorization stage.

    \sa callbackText()
*/
void QOAuthHttpServerReplyHandler::setCallbackText(const QString &text)
{
    Q_D(QOAuthHttpServerReplyHandler);
    d->text = text;
}

/*!
    Returns the port on which this handler is listening,
    otherwise returns 0.

    \sa listen(), isListening()
*/
quint16 QOAuthHttpServerReplyHandler::port() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->httpServer.serverPort();
}

/*!
    Tells this handler to listen for incoming connections / redirections
    on \a address and \a port. Returns \c true if listening is successful,
    and \c false otherwise.

    Active listening is only required when performing the initial
    authorization phase, typically initiated by a
    QOAuth2AuthorizationCodeFlow::grant() call.

    It is recommended to close the listener after successful authorization.
    Listening is not needed for
    \l {QOAuth2AuthorizationCodeFlow::requestAccessToken()}{requesting access tokens}
    or refreshing them.

    If this function is called with \l {QHostAddress::SpecialAddress}{Null}
    as the \a address, the handler will attempt to listen to
    \l {QHostAddress::SpecialAddress}{LocalHost}, and if that fails,
    \l {QHostAddress::SpecialAddress}{LocalHostIPv6}.

    See also \l {IPv4 and IPv6}.

    \sa close(), isListening(), QTcpServer::listen()
*/
bool QOAuthHttpServerReplyHandler::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QOAuthHttpServerReplyHandler);
    bool success = false;

    if (address.isNull()) {
        // try IPv4 first, for greatest compatibility
        success = d->httpServer.listen(QHostAddress::LocalHost, port);
        if (!success)
            success = d->httpServer.listen(QHostAddress::LocalHostIPv6, port);
    }
    if (!success)
        success = d->httpServer.listen(address, port);

    if (success) {
        // Callback ('redirect_uri') value may be needed after this handler is closed
        d->callbackAddress = d->httpServer.serverAddress();
        d->callbackPort = d->httpServer.serverPort();
    }

    return success;
}

/*!
    Tells this handler to stop listening for connections / redirections.

    \sa listen()
*/
void QOAuthHttpServerReplyHandler::close()
{
    Q_D(QOAuthHttpServerReplyHandler);
    return d->httpServer.close();
}

/*!
    Returns \c true if this handler is currently listening,
    and \c false otherwise.

    \sa listen(), close()
*/
bool QOAuthHttpServerReplyHandler::isListening() const
{
    Q_D(const QOAuthHttpServerReplyHandler);
    return d->httpServer.isListening();
}

QT_END_NAMESPACE

#include "moc_qoauthhttpserverreplyhandler.cpp"

#endif // QT_NO_HTTP
