// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qhttpserverhttp1protocolhandler_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qpointer.h>
#include <QtHttpServer/qabstracthttpserver.h>
#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverresponder.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qtcpsocket.h>

#include <private/qabstracthttpserver_p.h>
#include <private/qhttpserverliterals_p.h>
#include <private/qhttpserverrequest_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHttpServerHttp1Handler, "qt.httpserver.http1handler")

// https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
static const std::map<QHttpServerResponder::StatusCode, QByteArray> statusString{
#define XX(name, string) { QHttpServerResponder::StatusCode::name, QByteArrayLiteral(string) }
    XX(Continue, "Continue"),
    XX(SwitchingProtocols, "Switching Protocols"),
    XX(Processing, "Processing"),
    XX(Ok, "OK"),
    XX(Created, "Created"),
    XX(Accepted, "Accepted"),
    XX(NonAuthoritativeInformation, "Non-Authoritative Information"),
    XX(NoContent, "No Content"),
    XX(ResetContent, "Reset Content"),
    XX(PartialContent, "Partial Content"),
    XX(MultiStatus, "Multi-Status"),
    XX(AlreadyReported, "Already Reported"),
    XX(IMUsed, "I'm Used"),
    XX(MultipleChoices, "Multiple Choices"),
    XX(MovedPermanently, "Moved Permanently"),
    XX(Found, "Found"),
    XX(SeeOther, "See Other"),
    XX(NotModified, "Not Modified"),
    XX(UseProxy, "Use Proxy"),
    XX(TemporaryRedirect, "Temporary Redirect"),
    XX(PermanentRedirect, "Permanent Redirect"),
    XX(BadRequest, "Bad Request"),
    XX(Unauthorized, "Unauthorized"),
    XX(PaymentRequired, "Payment Required"),
    XX(Forbidden, "Forbidden"),
    XX(NotFound, "Not Found"),
    XX(MethodNotAllowed, "Method Not Allowed"),
    XX(NotAcceptable, "Not Acceptable"),
    XX(ProxyAuthenticationRequired, "Proxy Authentication Required"),
    XX(RequestTimeout, "Request Timeout"),
    XX(Conflict, "Conflict"),
    XX(Gone, "Gone"),
    XX(LengthRequired, "Length Required"),
    XX(PreconditionFailed, "Precondition Failed"),
    XX(PayloadTooLarge, "Request Entity Too Large"),
    XX(UriTooLong, "Request-URI Too Long"),
    XX(UnsupportedMediaType, "Unsupported Media Type"),
    XX(RequestRangeNotSatisfiable, "Requested Range Not Satisfiable"),
    XX(ExpectationFailed, "Expectation Failed"),
    XX(ImATeapot, "I'm a teapot"),
    XX(MisdirectedRequest, "Misdirected Request"),
    XX(UnprocessableEntity, "Unprocessable Entity"),
    XX(Locked, "Locked"),
    XX(FailedDependency, "Failed Dependency"),
    XX(UpgradeRequired, "Upgrade Required"),
    XX(PreconditionRequired, "Precondition Required"),
    XX(TooManyRequests, "Too Many Requests"),
    XX(RequestHeaderFieldsTooLarge, "Request Header Fields Too Large"),
    XX(UnavailableForLegalReasons, "Unavailable For Legal Reasons"),
    XX(InternalServerError, "Internal Server Error"),
    XX(NotImplemented, "Not Implemented"),
    XX(BadGateway, "Bad Gateway"),
    XX(ServiceUnavailable, "Service Unavailable"),
    XX(GatewayTimeout, "Gateway Timeout"),
    XX(HttpVersionNotSupported, "HTTP Version Not Supported"),
    XX(VariantAlsoNegotiates, "Variant Also Negotiates"),
    XX(InsufficientStorage, "Insufficient Storage"),
    XX(LoopDetected, "Loop Detected"),
    XX(NotExtended, "Not Extended"),
    XX(NetworkAuthenticationRequired, "Network Authentication Required"),
    XX(NetworkConnectTimeoutError, "Network Connect Timeout Error"),
#undef XX
};

namespace {

template <qint64 BUFFERSIZE = 128 * 1024>
struct IOChunkedTransfer
{
    // TODO This is not the fastest implementation, as it does read & write
    // in a sequential fashion, but these operation could potentially overlap.
    // TODO Can we implement it without the buffer? Direct write to the target buffer
    // would be great.

    static constexpr qint64 bufferSize = BUFFERSIZE;
    static constexpr qint64 targetWriteBufferSaturation = bufferSize / 2;
    char buffer[BUFFERSIZE];
    qint64 beginIndex = -1;
    qint64 endIndex = -1;
    QPointer<QIODevice> source;
    const QPointer<QIODevice> sink;
    const QMetaObject::Connection bytesWrittenConnection;
    const QMetaObject::Connection readyReadConnection;
    bool inRead = false;

    IOChunkedTransfer(QIODevice *input, QIODevice *output) :
          source(input),
          sink(output),
          bytesWrittenConnection(connectToBytesWritten(this, output)),
          readyReadConnection(QObject::connect(source.data(), &QIODevice::readyRead, source.data(),
                                               [this]() { readFromInput(); }))
    {
        Q_ASSERT(!source->atEnd());  // TODO error out
        QObject::connect(sink.data(), &QObject::destroyed, source.data(), &QObject::deleteLater);
        QObject::connect(source.data(), &QObject::destroyed, source.data(), [this]() {
            delete this;
        });
        readFromInput();
    }

    ~IOChunkedTransfer()
    {
        QObject::disconnect(bytesWrittenConnection);
        QObject::disconnect(readyReadConnection);
    }

    static QMetaObject::Connection connectToBytesWritten(IOChunkedTransfer *that, QIODevice *device)
    {
        auto send = [that]() { that->writeToOutput(); };
#if QT_CONFIG(ssl)
        if (auto *sslSocket = qobject_cast<QSslSocket *>(device)) {
            return QObject::connect(sslSocket, &QSslSocket::encryptedBytesWritten, sslSocket,
                                    std::move(send));
        }
#endif
        return QObject::connect(device, &QIODevice::bytesWritten, device, std::move(send));
    }

    inline bool isBufferEmpty()
    {
        Q_ASSERT(beginIndex <= endIndex);
        return beginIndex == endIndex;
    }

    void readFromInput()
    {
        if (inRead)
            return;
        if (source.isNull())
            return;

        if (!isBufferEmpty()) // We haven't consumed all the data yet.
            return;
        QScopedValueRollback inReadGuard(inRead, true);

        while (isBufferEmpty()) {
            beginIndex = 0;
            endIndex = source->read(buffer, bufferSize);
            if (endIndex < 0) {
                endIndex = beginIndex; // Mark the buffer as empty
                qCWarning(lcHttpServerHttp1Handler, "Error reading chunk: %ls",
                        qUtf16Printable(source->errorString()));
                break;
            }
            if (endIndex == 0)
                break;
            memset(buffer + endIndex, 0, sizeof(buffer) - std::size_t(endIndex));
            writeToOutput();
        }
    }

    void writeToOutput()
    {
        if (sink.isNull() || source.isNull())
            return;

        if (isBufferEmpty())
            return;

        // If downstream has enough data to write already,
        // don't bother writing more now. That would only lead to
        // higher, unnecessary memory usage.
        if (sink->bytesToWrite() >= targetWriteBufferSaturation)
            return;
#if QT_CONFIG(ssl)
        if (auto *sslSocket = qobject_cast<QSslSocket *>(sink.data())) {
            const qint64 budget = targetWriteBufferSaturation - sink->bytesToWrite();
            if (sslSocket->encryptedBytesToWrite() >= budget)
                return;
        }
#endif

        const auto writtenBytes = sink->write(buffer + beginIndex, endIndex);
        if (writtenBytes < 0) {
            qCWarning(lcHttpServerHttp1Handler, "Error writing chunk: %ls",
                      qUtf16Printable(sink->errorString()));
            return;
        }
        beginIndex += writtenBytes;
        if (isBufferEmpty()) {
            if (source->bytesAvailable() && !inRead)
                readFromInput();
            else if (source->atEnd())  // Finishing
                source->deleteLater();
        }
    }
};

} // anonymous namespace


QHttpServerHttp1ProtocolHandler::QHttpServerHttp1ProtocolHandler(QAbstractHttpServer *server,
                                                                 QIODevice *socket)
    : QHttpServerStream(server),
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
        qCDebug(lcHttpServerHttp1Handler) << "Connection from:" << tcpSocket->peerAddress();
        connect(socket, &QTcpSocket::readyRead,
                this, &QHttpServerHttp1ProtocolHandler::handleReadyRead);
        connect(tcpSocket, &QTcpSocket::disconnected,
                this, &QHttpServerHttp1ProtocolHandler::socketDisconnected);
#if QT_CONFIG(localserver)
    } else if (localSocket) {
        qCDebug(lcHttpServerHttp1Handler) << "Connection from:" << localSocket->serverName();
        connect(socket, &QLocalSocket::readyRead,
                this, &QHttpServerHttp1ProtocolHandler::handleReadyRead);
        connect(localSocket, &QLocalSocket::disconnected,
                this, &QHttpServerHttp1ProtocolHandler::socketDisconnected);
#endif
    }
}

void QHttpServerHttp1ProtocolHandler::responderDestroyed()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (protocolChanged) {
        deleteLater();
        return;
    }
    Q_ASSERT(handlingRequest);
    handlingRequest = false;

    if (tcpSocket) {
        if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
            deleteLater();
        } else {
            connect(tcpSocket, &QTcpSocket::readyRead,
                    this, &QHttpServerHttp1ProtocolHandler::handleReadyRead);
            QMetaObject::invokeMethod(tcpSocket, &QTcpSocket::readyRead, Qt::QueuedConnection);
        }
#if QT_CONFIG(localserver)
    } else if (localSocket) {
        if (localSocket->state() != QLocalSocket::ConnectedState) {
            deleteLater();
        } else {
            connect(localSocket, &QLocalSocket::readyRead,
                    this, &QHttpServerHttp1ProtocolHandler::handleReadyRead);
            QMetaObject::invokeMethod(localSocket, &QLocalSocket::readyRead, Qt::QueuedConnection);
        }
#endif
    }
}

void QHttpServerHttp1ProtocolHandler::startHandlingRequest()
{
    handlingRequest = true;
}

void QHttpServerHttp1ProtocolHandler::socketDisconnected()
{
    if (!handlingRequest)
        deleteLater();
}

void QHttpServerHttp1ProtocolHandler::handleReadyRead()
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

    qCDebug(lcHttpServerHttp1Handler) << "Request:" << request;

    QHttpServerResponder responder(this);

#if defined(QT_WEBSOCKETS_LIB)
    if (auto *tcpSocket = qobject_cast<QTcpSocket*>(socket)) {
        if (request.d->upgrade) { // Upgrade
            const auto &upgradeValue = request.value(QByteArrayLiteral("upgrade"));
            if (upgradeValue.compare(QByteArrayLiteral("websocket"), Qt::CaseInsensitive) == 0) {
                const auto upgradeResponse = server->verifyWebSocketUpgrade(request);
                static const auto signal =
                        QMetaMethod::fromSignal(&QAbstractHttpServer::newWebSocketConnection);
                if (server->isSignalConnected(signal)
                    && upgradeResponse.type()
                            != QHttpServerWebSocketUpgradeResponse::ResponseType::PassToNext) {
                    if (upgradeResponse.type()
                        == QHttpServerWebSocketUpgradeResponse::ResponseType::Accept) {
                        // Socket will now be managed by websocketServer
                        protocolChanged = true;
                        socket->disconnect();
                        socket->rollbackTransaction();
                        socket->setParent(nullptr);
                        server->d_func()->websocketServer.handleConnection(tcpSocket);
                        Q_EMIT socket->readyRead();
                    } else {
                        qCDebug(lcHttpServerHttp1Handler, "WebSocket upgrade denied: %ls",
                                qUtf16Printable(QLatin1StringView(upgradeResponse.denyMessage())));
                        QByteArray buffer;
                        buffer.append("HTTP/1.1 ");
                        buffer.append(QByteArray::number(quint32(upgradeResponse.denyStatus())));
                        buffer.append(" ");
                        buffer.append(upgradeResponse.denyMessage());
                        buffer.append("\r\n\r\n");
                        tcpSocket->write(buffer);
                    }
                } else {
                    if (!server->isSignalConnected(signal)) {
                        qCWarning(lcHttpServerHttp1Handler,
                                  "WebSocket received but no slots connected to "
                                  "QWebSocketServer::newConnection");
                    }
                    server->missingHandler(request, responder);
                    tcpSocket->disconnectFromHost();
                }
                return;
            }
        }
    }
#endif // QT_WEBSOCKETS_LIB

    socket->commitTransaction();

    if (!server->handleRequest(request, responder))
        server->missingHandler(request, responder);

    if (handlingRequest)
        disconnect(socket, &QIODevice::readyRead, this, &QHttpServerHttp1ProtocolHandler::handleReadyRead);
    else if (socket->bytesAvailable() > 0)
        QMetaObject::invokeMethod(socket, &QIODevice::readyRead, Qt::QueuedConnection);
}

void QHttpServerHttp1ProtocolHandler::write(const QByteArray &body, const QHttpHeaders &headers,
                                        QHttpServerResponder::StatusCode status, quint32 streamId)
{
    Q_UNUSED(streamId);
    Q_ASSERT(state == TransferState::Ready);
    writeStatusAndHeaders(status, headers);
    write(body);
    state = TransferState::Ready;
}

void QHttpServerHttp1ProtocolHandler::write(QHttpServerResponder::StatusCode status, quint32 streamId)
{
    Q_UNUSED(streamId);
    Q_ASSERT(state == TransferState::Ready);
    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::ContentType,
                   QHttpServerLiterals::contentTypeXEmpty());
    headers.append(QHttpHeaders::WellKnownHeader::ContentLength, "0");
    writeStatusAndHeaders(status, headers);
    state = TransferState::Ready;
}

void QHttpServerHttp1ProtocolHandler::write(QIODevice *data, const QHttpHeaders &headers,
                                        QHttpServerResponder::StatusCode status, quint32 streamId)
{
    Q_UNUSED(streamId);
    Q_ASSERT(state == TransferState::Ready);
    std::unique_ptr<QIODevice, QScopedPointerDeleteLater> input(data);

    input->setParent(nullptr);
    if (!input->isOpen()) {
        if (!input->open(QIODevice::ReadOnly)) {
            // TODO Add developer error handling
            qCDebug(lcHttpServerHttp1Handler, "500: Could not open device %ls",
                    qUtf16Printable(input->errorString()));
            write(QHttpServerResponder::StatusCode::InternalServerError, 0);
            return;
        }
    } else if (!(input->openMode() & QIODevice::ReadOnly)) {
        // TODO Add developer error handling
        qCDebug(lcHttpServerHttp1Handler) << "500: Device is opened in a wrong mode"
                                          << input->openMode();
        write(QHttpServerResponder::StatusCode::InternalServerError, 0);
        return;
    }

    QHttpHeaders allHeaders(headers);
    if (!input->isSequential()) { // Non-sequential QIODevice should know its data size
        allHeaders.append(QHttpHeaders::WellKnownHeader::ContentLength,
                          QByteArray::number(input->size()));
    }

    writeStatusAndHeaders(status, allHeaders);

    if (input->atEnd()) {
        qCDebug(lcHttpServerHttp1Handler, "No more data available.");
        return;
    }

    // input takes ownership of the IOChunkedTransfer pointer inside his constructor
    new IOChunkedTransfer<>(input.release(), socket);
    state = TransferState::Ready;
}

void QHttpServerHttp1ProtocolHandler::writeBeginChunked(const QHttpHeaders &headers,
                                                    QHttpServerResponder::StatusCode status,
                                                    quint32 streamId)
{
    Q_UNUSED(streamId);
    Q_ASSERT(state == TransferState::Ready);
    QHttpHeaders allHeaders(headers);
    allHeaders.append(QHttpHeaders::WellKnownHeader::TransferEncoding, "chunked");
    writeStatusAndHeaders(status, allHeaders);
    state = TransferState::ChunkedTransferBegun;
}

void QHttpServerHttp1ProtocolHandler::writeChunk(const QByteArray &data, quint32 streamId)
{
    Q_UNUSED(streamId);
    Q_ASSERT(state == TransferState::ChunkedTransferBegun);

    if (data.length() == 0) {
        qCWarning(lcHttpServerHttp1Handler, "Chunk must have length > 0");
        return;
    }

    write(QByteArray::number(data.length(), 16));
    write("\r\n");
    write(data);
    write("\r\n");
}

void QHttpServerHttp1ProtocolHandler::writeEndChunked(const QByteArray &data,
                                                      const QHttpHeaders &trailers,
                                                      quint32 streamId)
{
    Q_UNUSED(streamId);
    Q_ASSERT(state == TransferState::ChunkedTransferBegun);
    writeChunk(data, 0);
    write("0\r\n");
    for (qsizetype i = 0; i < trailers.size(); ++i) {
        const auto name = trailers.nameAt(i);
        const auto value = trailers.valueAt(i);
        writeHeader({ name.data(), name.size() }, value.toByteArray());
    }
    write("\r\n");
    state = TransferState::Ready;
}

void QHttpServerHttp1ProtocolHandler::writeStatusAndHeaders(QHttpServerResponder::StatusCode status,
                                                        const QHttpHeaders &headers)
{
    Q_ASSERT(state == TransferState::Ready);
    QByteArray payload;
    payload.append("HTTP/1.1 ");
    payload.append(QByteArray::number(quint32(status)));
    const auto it = statusString.find(status);
    if (it != statusString.end()) {
        payload.append(" ");
        payload.append(statusString.at(status));
    }
    payload.append("\r\n");

    for (qsizetype i = 0; i < headers.size(); ++i) {
        const auto name = headers.nameAt(i);
        payload.append(QByteArrayView(name.data(), name.size()) + ": "
                       + headers.valueAt(i).toByteArray() + "\r\n");
    }
    payload.append("\r\n");
    write(payload);
    state = TransferState::HeadersSent;
}

void QHttpServerHttp1ProtocolHandler::writeHeader(const QByteArray &key, const QByteArray &value)
{
    write(key + ": " + value + "\r\n");
}

void QHttpServerHttp1ProtocolHandler::write(const QByteArray &ba)
{
    Q_ASSERT(QThread::currentThread() == thread());
    socket->write(ba);
}

void QHttpServerHttp1ProtocolHandler::write(const char *body, qint64 size)
{
    Q_ASSERT(QThread::currentThread() == thread());
    socket->write(body, size);
}

QT_END_NAMESPACE
