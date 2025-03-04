// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttpsocketengine_p.h"
#include "qtcpsocket.h"
#include "qhostaddress.h"
#include "qurl.h"
#include "private/qhttpnetworkreply_p.h"
#include "private/qiodevice_p.h"
#include "qdeadlinetimer.h"
#include "qnetworkinterface.h"

#if !defined(QT_NO_NETWORKPROXY)
#include <qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#define DEBUG

QHttpSocketEngine::QHttpSocketEngine(QObject *parent)
    : QAbstractSocketEngine(*new QHttpSocketEnginePrivate, parent)
{
}

QHttpSocketEngine::~QHttpSocketEngine()
{
}

bool QHttpSocketEngine::initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol)
{
    Q_D(QHttpSocketEngine);
    if (type != QAbstractSocket::TcpSocket)
        return false;

    setProtocol(protocol);
    setSocketType(type);
    d->socket = new QTcpSocket(this);
    d->reply = new QHttpNetworkReply(QUrl(), this);

    // Explicitly disable proxying on the proxy socket itself to avoid
    // unwanted recursion.
    d->socket->setProxy(QNetworkProxy::NoProxy);

    // Intercept all the signals.
    connect(d->socket, SIGNAL(connected()),
            this, SLOT(slotSocketConnected()),
            Qt::DirectConnection);
    connect(d->socket, SIGNAL(disconnected()),
            this, SLOT(slotSocketDisconnected()),
            Qt::DirectConnection);
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(slotSocketReadNotification()),
            Qt::DirectConnection);
    connect(d->socket, SIGNAL(bytesWritten(qint64)),
            this, SLOT(slotSocketBytesWritten()),
            Qt::DirectConnection);
    connect(d->socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
            this, SLOT(slotSocketError(QAbstractSocket::SocketError)),
            Qt::DirectConnection);
    connect(d->socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(slotSocketStateChanged(QAbstractSocket::SocketState)),
            Qt::DirectConnection);

    return true;
}

bool QHttpSocketEngine::initialize(qintptr, QAbstractSocket::SocketState)
{
    return false;
}

void QHttpSocketEngine::setProxy(const QNetworkProxy &proxy)
{
    Q_D(QHttpSocketEngine);
    d->proxy = proxy;
    QString user = proxy.user();
    if (!user.isEmpty())
        d->authenticator.setUser(user);
    QString password = proxy.password();
    if (!password.isEmpty())
        d->authenticator.setPassword(password);
}

qintptr QHttpSocketEngine::socketDescriptor() const
{
    Q_D(const QHttpSocketEngine);
    return d->socket ? d->socket->socketDescriptor() : -1;
}

bool QHttpSocketEngine::isValid() const
{
    Q_D(const QHttpSocketEngine);
    return d->socket;
}

bool QHttpSocketEngine::connectInternal()
{
    Q_D(QHttpSocketEngine);

    d->credentialsSent = false;

    // If the handshake is done, enter ConnectedState state and return true.
    if (d->state == Connected) {
        qWarning("QHttpSocketEngine::connectToHost: called when already connected");
        setState(QAbstractSocket::ConnectedState);
        return true;
    }

    if (d->state == ConnectSent && d->socketState != QAbstractSocket::ConnectedState)
        setState(QAbstractSocket::UnconnectedState);

    // Handshake isn't done. If unconnected, start connecting.
    if (d->state == None && d->socket->state() == QAbstractSocket::UnconnectedState) {
        setState(QAbstractSocket::ConnectingState);
        //limit buffer in internal socket, data is buffered in the external socket under application control
        d->socket->setReadBufferSize(65536);
        d->socket->connectToHost(d->proxy.hostName(), d->proxy.port());
    }

    // If connected (might happen right away, at least for localhost services
    // on some BSD systems), there might already be bytes available.
    if (bytesAvailable())
        slotSocketReadNotification();

    return d->socketState == QAbstractSocket::ConnectedState;
}

bool QHttpSocketEngine::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_D(QHttpSocketEngine);

    setPeerAddress(address);
    setPeerPort(port);
    d->peerName.clear();

    return connectInternal();
}

bool QHttpSocketEngine::connectToHostByName(const QString &hostname, quint16 port)
{
    Q_D(QHttpSocketEngine);

    setPeerAddress(QHostAddress());
    setPeerPort(port);
    d->peerName = hostname;

    return connectInternal();
}

bool QHttpSocketEngine::bind(const QHostAddress &, quint16)
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return false;
}

bool QHttpSocketEngine::listen(int backlog)
{
    Q_UNUSED(backlog);
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return false;
}

qintptr QHttpSocketEngine::accept()
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return -1;
}

void QHttpSocketEngine::close()
{
    Q_D(QHttpSocketEngine);
    if (d->socket) {
        d->socket->close();
        delete d->socket;
        d->socket = nullptr;
    }
}

qint64 QHttpSocketEngine::bytesAvailable() const
{
    Q_D(const QHttpSocketEngine);
    return d->socket ? d->socket->bytesAvailable() : 0;
}

qint64 QHttpSocketEngine::read(char *data, qint64 maxlen)
{
    Q_D(QHttpSocketEngine);
    qint64 bytesRead = d->socket->read(data, maxlen);

    if (d->socket->state() == QAbstractSocket::UnconnectedState
        && d->socket->bytesAvailable() == 0) {
        emitReadNotification();
    }

    if (bytesRead == -1) {
        // If nothing has been read so far, and the direct socket read
        // failed, return the socket's error. Otherwise, fall through and
        // return as much as we read so far.
        close();
        setError(QAbstractSocket::RemoteHostClosedError, "Remote host closed"_L1);
        setState(QAbstractSocket::UnconnectedState);
        return -1;
    }
    return bytesRead;
}

qint64 QHttpSocketEngine::write(const char *data, qint64 len)
{
    Q_D(QHttpSocketEngine);
    return d->socket->write(data, len);
}

#ifndef QT_NO_UDPSOCKET
#ifndef QT_NO_NETWORKINTERFACE
bool QHttpSocketEngine::joinMulticastGroup(const QHostAddress &,
                                           const QNetworkInterface &)
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return false;
}

bool QHttpSocketEngine::leaveMulticastGroup(const QHostAddress &,
                                            const QNetworkInterface &)
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return false;
}

QNetworkInterface QHttpSocketEngine::multicastInterface() const
{
    return QNetworkInterface();
}

bool QHttpSocketEngine::setMulticastInterface(const QNetworkInterface &)
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return false;
}
#endif // QT_NO_NETWORKINTERFACE

bool QHttpSocketEngine::hasPendingDatagrams() const
{
    qWarning("Operation is not supported");
    return false;
}

qint64 QHttpSocketEngine::pendingDatagramSize() const
{
    qWarning("Operation is not supported");
    return -1;
}
#endif // QT_NO_UDPSOCKET

qint64 QHttpSocketEngine::readDatagram(char *, qint64, QIpPacketHeader *, PacketHeaderOptions)
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return -1;
}

qint64 QHttpSocketEngine::writeDatagram(const char *, qint64, const QIpPacketHeader &)
{
    qWarning("Operation is not supported");
    setError(QAbstractSocket::UnsupportedSocketOperationError, "Unsupported socket operation"_L1);
    return -1;
}

qint64 QHttpSocketEngine::bytesToWrite() const
{
    Q_D(const QHttpSocketEngine);
    if (d->socket) {
        return d->socket->bytesToWrite();
    } else {
        return 0;
    }
}

int QHttpSocketEngine::option(SocketOption option) const
{
    Q_D(const QHttpSocketEngine);
    if (d->socket) {
        // convert the enum and call the real socket
        if (option == QAbstractSocketEngine::LowDelayOption)
            return d->socket->socketOption(QAbstractSocket::LowDelayOption).toInt();
        if (option == QAbstractSocketEngine::KeepAliveOption)
            return d->socket->socketOption(QAbstractSocket::KeepAliveOption).toInt();
    }
    return -1;
}

bool QHttpSocketEngine::setOption(SocketOption option, int value)
{
    Q_D(QHttpSocketEngine);
    if (d->socket) {
        // convert the enum and call the real socket
        if (option == QAbstractSocketEngine::LowDelayOption)
            d->socket->setSocketOption(QAbstractSocket::LowDelayOption, value);
        if (option == QAbstractSocketEngine::KeepAliveOption)
            d->socket->setSocketOption(QAbstractSocket::KeepAliveOption, value);
        return true;
    }
    return false;
}

bool QHttpSocketEngine::waitForRead(QDeadlineTimer deadline, bool *timedOut)
{
    Q_D(const QHttpSocketEngine);

    if (!d->socket || d->socket->state() == QAbstractSocket::UnconnectedState)
        return false;

    // Wait for more data if nothing is available.
    if (!d->socket->bytesAvailable()) {
        if (!d->socket->waitForReadyRead(deadline.remainingTime())) {
            if (d->socket->state() == QAbstractSocket::UnconnectedState)
                return true;
            setError(d->socket->error(), d->socket->errorString());
            if (timedOut && d->socket->error() == QAbstractSocket::SocketTimeoutError)
                *timedOut = true;
            return false;
        }
    }

    waitForProtocolHandshake(deadline);

    // Report any error that may occur.
    if (d->state != Connected) {
        setError(d->socket->error(), d->socket->errorString());
        if (timedOut && d->socket->error() == QAbstractSocket::SocketTimeoutError)
            *timedOut = true;
        return false;
    }
    return true;
}

bool QHttpSocketEngine::waitForWrite(QDeadlineTimer deadline, bool *timedOut)
{
    Q_D(const QHttpSocketEngine);

    // If we're connected, just forward the call.
    if (d->state == Connected) {
        if (d->socket->bytesToWrite()) {
            if (!d->socket->waitForBytesWritten(deadline.remainingTime())) {
                if (d->socket->error() == QAbstractSocket::SocketTimeoutError && timedOut)
                    *timedOut = true;
                return false;
            }
        }
        return true;
    }

    waitForProtocolHandshake(deadline);

    // Report any error that may occur.
    if (d->state != Connected) {
//        setError(d->socket->error(), d->socket->errorString());
        if (timedOut && d->socket->error() == QAbstractSocket::SocketTimeoutError)
            *timedOut = true;
    }

    return true;
}

bool QHttpSocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                                           bool checkRead, bool checkWrite,
                                           QDeadlineTimer deadline, bool *timedOut)
{
    Q_UNUSED(checkRead);

    if (!checkWrite) {
        // Not interested in writing? Then we wait for read notifications.
        bool canRead = waitForRead(deadline, timedOut);
        if (readyToRead)
            *readyToRead = canRead;
        return canRead;
    }

    // Interested in writing? Then we wait for write notifications.
    bool canWrite = waitForWrite(deadline, timedOut);
    if (readyToWrite)
        *readyToWrite = canWrite;
    return canWrite;
}

void QHttpSocketEngine::waitForProtocolHandshake(QDeadlineTimer deadline) const
{
    Q_D(const QHttpSocketEngine);

    // If we're not connected yet, wait until we are (and until bytes have
    // been received, i.e. the socket has connected, we have sent the
    // greeting, and then received the response), or until an error occurs.
    while (d->state != Connected && d->socket->waitForReadyRead(deadline.remainingTime())) {
        // Loop while the protocol handshake is taking place.
    }
}

bool QHttpSocketEngine::isReadNotificationEnabled() const
{
    Q_D(const QHttpSocketEngine);
    return d->readNotificationEnabled;
}

void QHttpSocketEngine::setReadNotificationEnabled(bool enable)
{
    Q_D(QHttpSocketEngine);
    if (d->readNotificationEnabled == enable)
        return;

    d->readNotificationEnabled = enable;
    if (enable) {
        // Enabling read notification can trigger a notification.
        if (bytesAvailable()) {
            slotSocketReadNotification();
        } else if (d->socket && d->socket->state() == QAbstractSocket::UnconnectedState) {
            emitReadNotification();
        }
    }
}

bool QHttpSocketEngine::isWriteNotificationEnabled() const
{
    Q_D(const QHttpSocketEngine);
    return d->writeNotificationEnabled;
}

void QHttpSocketEngine::setWriteNotificationEnabled(bool enable)
{
    Q_D(QHttpSocketEngine);
    d->writeNotificationEnabled = enable;
    if (enable && d->state == Connected && d->socket->state() == QAbstractSocket::ConnectedState)
        QMetaObject::invokeMethod(this, "writeNotification", Qt::QueuedConnection);
}

bool QHttpSocketEngine::isExceptionNotificationEnabled() const
{
    Q_D(const QHttpSocketEngine);
    return d->exceptNotificationEnabled;
}

void QHttpSocketEngine::setExceptionNotificationEnabled(bool enable)
{
    Q_D(QHttpSocketEngine);
    d->exceptNotificationEnabled = enable;
}

void QHttpSocketEngine::slotSocketConnected()
{
    Q_D(QHttpSocketEngine);

    // Send the greeting.
    const char method[] = "CONNECT";
    QByteArray peerAddress = d->peerName.isEmpty() ?
                             d->peerAddress.toString().toLatin1() :
                             QUrl::toAce(d->peerName);
    QByteArray path = peerAddress + ':' + QByteArray::number(d->peerPort);
    QByteArray data = method;
    data += ' ';
    data += path;
    data += " HTTP/1.1\r\n";
    data += "Proxy-Connection: keep-alive\r\n";
    data += "Host: " + peerAddress + "\r\n";
    if (!d->proxy.hasRawHeader("User-Agent"))
        data += "User-Agent: Mozilla/5.0\r\n";
    const auto headers = d->proxy.rawHeaderList();
    for (const QByteArray &header : headers)
        data += header + ": " + d->proxy.rawHeader(header) + "\r\n";
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(d->authenticator);
    //qDebug() << "slotSocketConnected: priv=" << priv << (priv ? (int)priv->method : -1);
    if (priv && priv->method != QAuthenticatorPrivate::None) {
        d->credentialsSent = true;
        data += "Proxy-Authorization: " + priv->calculateResponse(method, path, d->proxy.hostName());
        data += "\r\n";
    }
    data += "\r\n";
//     qDebug() << ">>>>>>>> sending request" << this;
//     qDebug() << data;
//     qDebug(">>>>>>>");
    d->socket->write(data);
    d->state = ConnectSent;
}

void QHttpSocketEngine::slotSocketDisconnected()
{
}

void QHttpSocketEngine::slotSocketReadNotification()
{
    Q_D(QHttpSocketEngine);
    if (d->state != Connected && d->socket->bytesAvailable() == 0)
        return;

    if (d->state == Connected) {
        // Forward as a read notification.
        if (d->readNotificationEnabled)
            emitReadNotification();
        return;
    }

    if (d->state == ConnectSent) {
        d->reply->d_func()->state = QHttpNetworkReplyPrivate::NothingDoneState;
        d->state = ReadResponseHeader;
    }

    if (d->state == ReadResponseHeader) {
        bool ok = readHttpHeader();
        if (!ok) {
            // protocol error, this isn't HTTP
            d->socket->close();
            setState(QAbstractSocket::UnconnectedState);
            setError(QAbstractSocket::ProxyProtocolError, tr("Did not receive HTTP response from proxy"));
            emitConnectionNotification();
            return;
        }
        if (d->state == ReadResponseHeader)
            return; // readHttpHeader() was not done yet, need to wait for more header data
    }

    if (d->state == ReadResponseContent) {
        qint64 skipped = d->socket->skip(d->pendingResponseData);
        if (skipped == -1) {
            d->socket->disconnectFromHost();
            emitWriteNotification();
            return;
        }
        d->pendingResponseData -= uint(skipped);
        if (d->pendingResponseData > 0)
            return;
        if (d->reply->statusCode() == 407)
            d->state = SendAuthentication;
    }

    int statusCode = d->reply->statusCode();
    QAuthenticatorPrivate *priv = nullptr;
    if (statusCode == 200) {
        d->state = Connected;
        setLocalAddress(d->socket->localAddress());
        setLocalPort(d->socket->localPort());
        d->inboundStreamCount = d->outboundStreamCount = 1;
        setState(QAbstractSocket::ConnectedState);
        d->authenticator.detach();
        priv = QAuthenticatorPrivate::getPrivate(d->authenticator);
        priv->hasFailed = false;
    } else if (statusCode == 407) {
        if (d->authenticator.isNull())
            d->authenticator.detach();
        priv = QAuthenticatorPrivate::getPrivate(d->authenticator);

        priv->parseHttpResponse(d->reply->header(), true, d->proxy.hostName());

        if (priv->phase == QAuthenticatorPrivate::Invalid) {
            // problem parsing the reply
            d->socket->close();
            setState(QAbstractSocket::UnconnectedState);
            setError(QAbstractSocket::ProxyProtocolError, tr("Error parsing authentication request from proxy"));
            emitConnectionNotification();
            return;
        }

        if (priv->phase == QAuthenticatorPrivate::Done
            || (priv->phase == QAuthenticatorPrivate::Start
                && (priv->method == QAuthenticatorPrivate::Ntlm
                    || priv->method == QAuthenticatorPrivate::Negotiate))) {
            if (priv->phase == QAuthenticatorPrivate::Start)
                priv->phase = QAuthenticatorPrivate::Phase1;
            bool credentialsWasSent = d->credentialsSent;
            if (d->credentialsSent) {
                // Remember that (e.g.) NTLM is two-phase, so only reset when the authentication is
                // not currently in progress. 407 response again means the provided
                // username/password were invalid.
                d->authenticator.detach();
                priv = QAuthenticatorPrivate::getPrivate(d->authenticator);
                priv->hasFailed = true;
                d->credentialsSent = false;
                priv->phase = QAuthenticatorPrivate::Done;
            }
            if ((priv->method != QAuthenticatorPrivate::Ntlm
                 && priv->method != QAuthenticatorPrivate::Negotiate)
                || credentialsWasSent)
                proxyAuthenticationRequired(d->proxy, &d->authenticator);
        }

        bool willClose;
        QByteArray proxyConnectionHeader = d->reply->headerField("Proxy-Connection");
        // Although most proxies use the unofficial Proxy-Connection header, the Connection header
        // from http spec is also allowed.
        if (proxyConnectionHeader.isEmpty())
            proxyConnectionHeader = d->reply->headerField("Connection");
        if (proxyConnectionHeader.compare("close", Qt::CaseInsensitive) == 0) {
            willClose = true;
        } else if (proxyConnectionHeader.compare("keep-alive", Qt::CaseInsensitive) == 0) {
            willClose = false;
        } else {
            // no Proxy-Connection header, so use the default
            // HTTP 1.1's default behaviour is to keep persistent connections
            // HTTP 1.0 or earlier, so we expect the server to close
            willClose = (d->reply->majorVersion() * 0x100 + d->reply->minorVersion()) <= 0x0100;
        }

        if (willClose) {
            // the server will disconnect, so let's avoid receiving an error
            // especially since the signal below may trigger a new event loop
            d->socket->disconnectFromHost();
            d->socket->readAll();
            //We're done with the reply and need to reset it for the next connection
            delete d->reply;
            d->reply = new QHttpNetworkReply(QUrl(), this);
        }

        if (priv->phase == QAuthenticatorPrivate::Done) {
            d->authenticator = QAuthenticator();
            setError(QAbstractSocket::ProxyAuthenticationRequiredError, tr("Authentication required"));
            d->socket->disconnectFromHost();
        } else {
            // close the connection if it isn't already and reconnect using the chosen authentication method
            d->state = SendAuthentication;
            if (willClose) {
                d->socket->connectToHost(d->proxy.hostName(), d->proxy.port());
            } else {
                // send the HTTP CONNECT again
                slotSocketConnected();
            }
            return;
        }
    } else {
        d->socket->close();
        setState(QAbstractSocket::UnconnectedState);
        if (statusCode == 403 || statusCode == 405) {
            // 403 Forbidden
            // 405 Method Not Allowed
            setError(QAbstractSocket::SocketAccessError, tr("Proxy denied connection"));
        } else if (statusCode == 404) {
            // 404 Not Found: host lookup error
            setError(QAbstractSocket::HostNotFoundError, QAbstractSocket::tr("Host not found"));
        } else if (statusCode == 503) {
            // 503 Service Unavailable: Connection Refused
            setError(QAbstractSocket::ConnectionRefusedError, QAbstractSocket::tr("Connection refused"));
        } else {
            // Some other reply
            //qWarning("UNEXPECTED RESPONSE: [%s]", responseHeader.toString().toLatin1().data());
            setError(QAbstractSocket::ProxyProtocolError, tr("Error communicating with HTTP proxy"));
        }
    }

    // The handshake is done; notify that we're connected (or failed to connect)
    emitConnectionNotification();
}

bool QHttpSocketEngine::readHttpHeader()
{
    Q_D(QHttpSocketEngine);

    if (d->state != ReadResponseHeader)
        return false;

    bool ok = true;
    if (d->reply->d_func()->state == QHttpNetworkReplyPrivate::NothingDoneState) {
        // do not keep old content sizes, status etc. around
        d->reply->d_func()->clearHttpLayerInformation();
        d->reply->d_func()->state = QHttpNetworkReplyPrivate::ReadingStatusState;
    }
    if (d->reply->d_func()->state == QHttpNetworkReplyPrivate::ReadingStatusState) {
        ok = d->reply->d_func()->readStatus(d->socket) != -1;
        if (ok && d->reply->d_func()->state == QHttpNetworkReplyPrivate::ReadingStatusState)
            return true; //Not done parsing headers yet, wait for more data
    }
    if (ok && d->reply->d_func()->state == QHttpNetworkReplyPrivate::ReadingHeaderState) {
        ok = d->reply->d_func()->readHeader(d->socket) != -1;
        if (ok && d->reply->d_func()->state == QHttpNetworkReplyPrivate::ReadingHeaderState)
            return true; //Not done parsing headers yet, wait for more data
    }
    if (ok) {
        bool contentLengthOk;
        int contentLength = d->reply->headerField("Content-Length").toInt(&contentLengthOk);
        if (contentLengthOk && contentLength > 0)
            d->pendingResponseData = contentLength;
        d->state = ReadResponseContent; // we are done reading the header
    }
    return ok;
}

void QHttpSocketEngine::slotSocketBytesWritten()
{
    Q_D(QHttpSocketEngine);
    if (d->state == Connected && d->writeNotificationEnabled)
        emitWriteNotification();
}

void QHttpSocketEngine::slotSocketError(QAbstractSocket::SocketError error)
{
    Q_D(QHttpSocketEngine);

    if (d->state != Connected) {
        // we are in proxy handshaking stages
        if (error == QAbstractSocket::HostNotFoundError)
            setError(QAbstractSocket::ProxyNotFoundError, tr("Proxy server not found"));
        else if (error == QAbstractSocket::ConnectionRefusedError)
            setError(QAbstractSocket::ProxyConnectionRefusedError, tr("Proxy connection refused"));
        else if (error == QAbstractSocket::SocketTimeoutError)
            setError(QAbstractSocket::ProxyConnectionTimeoutError, tr("Proxy server connection timed out"));
        else if (error == QAbstractSocket::RemoteHostClosedError)
            setError(QAbstractSocket::ProxyConnectionClosedError, tr("Proxy connection closed prematurely"));
        else
            setError(error, d->socket->errorString());
        emitConnectionNotification();
        return;
    }

    // We're connected
    if (error == QAbstractSocket::SocketTimeoutError)
        return;                 // ignore this error

    d->state = None;
    setError(error, d->socket->errorString());
    if (error != QAbstractSocket::RemoteHostClosedError)
        qDebug() << "QHttpSocketEngine::slotSocketError: got weird error =" << error;
    //read notification needs to always be emitted, otherwise the higher layer doesn't get the disconnected signal
    emitReadNotification();
}

void QHttpSocketEngine::slotSocketStateChanged(QAbstractSocket::SocketState state)
{
    Q_UNUSED(state);
}

void QHttpSocketEngine::emitPendingReadNotification()
{
    Q_D(QHttpSocketEngine);
    d->readNotificationPending = false;
    if (d->readNotificationEnabled)
        readNotification();
}

void QHttpSocketEngine::emitPendingWriteNotification()
{
    Q_D(QHttpSocketEngine);
    d->writeNotificationPending = false;
    if (d->writeNotificationEnabled)
        writeNotification();
}

void QHttpSocketEngine::emitPendingConnectionNotification()
{
    Q_D(QHttpSocketEngine);
    d->connectionNotificationPending = false;
    connectionNotification();
}

void QHttpSocketEngine::emitReadNotification()
{
    Q_D(QHttpSocketEngine);
    // if there is a connection notification pending we have to emit the readNotification
    // in case there is connection error. This is only needed for Windows, but it does not
    // hurt in other cases.
    if ((d->readNotificationEnabled && !d->readNotificationPending) || d->connectionNotificationPending) {
        d->readNotificationPending = true;
        QMetaObject::invokeMethod(this, "emitPendingReadNotification", Qt::QueuedConnection);
    }
}

void QHttpSocketEngine::emitWriteNotification()
{
    Q_D(QHttpSocketEngine);
    if (d->writeNotificationEnabled && !d->writeNotificationPending) {
        d->writeNotificationPending = true;
        QMetaObject::invokeMethod(this, "emitPendingWriteNotification", Qt::QueuedConnection);
    }
}

void QHttpSocketEngine::emitConnectionNotification()
{
    Q_D(QHttpSocketEngine);
    if (!d->connectionNotificationPending) {
        d->connectionNotificationPending = true;
        QMetaObject::invokeMethod(this, "emitPendingConnectionNotification", Qt::QueuedConnection);
    }
}

QHttpSocketEnginePrivate::QHttpSocketEnginePrivate()
    : readNotificationEnabled(false)
    , writeNotificationEnabled(false)
    , exceptNotificationEnabled(false)
    , readNotificationPending(false)
    , writeNotificationPending(false)
    , connectionNotificationPending(false)
    , credentialsSent(false)
    , pendingResponseData(0)
{
    socket = nullptr;
    reply = nullptr;
    state = QHttpSocketEngine::None;
}

QHttpSocketEnginePrivate::~QHttpSocketEnginePrivate()
{
}

QAbstractSocketEngine *QHttpSocketEngineHandler::createSocketEngine(QAbstractSocket::SocketType socketType,
                                                                    const QNetworkProxy &proxy,
                                                                    QObject *parent)
{
    if (socketType != QAbstractSocket::TcpSocket)
        return nullptr;

    // proxy type must have been resolved by now
    if (proxy.type() != QNetworkProxy::HttpProxy)
        return nullptr;

    // we only accept active sockets
    if (!qobject_cast<QAbstractSocket *>(parent))
        return nullptr;

    QHttpSocketEngine *engine = new QHttpSocketEngine(parent);
    engine->setProxy(proxy);
    return engine;
}

QAbstractSocketEngine *QHttpSocketEngineHandler::createSocketEngine(qintptr, QObject *)
{
    return nullptr;
}

QT_END_NAMESPACE

#endif // !QT_NO_NETWORKPROXY

#include "moc_qhttpsocketengine_p.cpp"
