// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/qhttpserverresponder.h>
#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverresponse.h>
#include <private/qhttpserverresponder_p.h>
#include <private/qhttpserverliterals_p.h>
#include <private/qhttpserverrequest_p.h>
#include <private/qhttpserverresponse_p.h>
#include <private/qhttpserverstream_p.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtimer.h>
#include <QtNetwork/qtcpsocket.h>
#include <map>
#include <memory>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN_TAGGED(QHttpServerResponder::StatusCode, QHttpServerResponder__StatusCode)

/*!
    \class QHttpServerResponder
    \since 6.4
    \inmodule QtHttpServer
    \brief API for sending replies from an HTTP server.

    Provides functions for writing back to an HTTP client with overloads for
    serializing JSON objects. It also has support for writing HTTP headers and
    status code.
*/

/*!
    \typealias QHttpServerResponder::HeaderList

    Type alias for std::initializer_list<std::pair<QByteArray, QByteArray>>
*/

/*!
    \enum QHttpServerResponder::StatusCode

    HTTP status codes

    \value Continue
    \value SwitchingProtocols
    \value Processing

    \value Ok
    \value Created
    \value Accepted
    \value NonAuthoritativeInformation
    \value NoContent
    \value ResetContent
    \value PartialContent
    \value MultiStatus
    \value AlreadyReported
    \value IMUsed

    \value MultipleChoices
    \value MovedPermanently
    \value Found
    \value SeeOther
    \value NotModified
    \value UseProxy

    \value TemporaryRedirect
    \value PermanentRedirect

    \value BadRequest
    \value Unauthorized
    \value PaymentRequired
    \value Forbidden
    \value NotFound
    \value MethodNotAllowed
    \value NotAcceptable
    \value ProxyAuthenticationRequired
    \value RequestTimeout
    \value Conflict
    \value Gone
    \value LengthRequired
    \value PreconditionFailed
    \value PayloadTooLarge
    \value UriTooLong
    \value UnsupportedMediaType
    \value RequestRangeNotSatisfiable
    \value ExpectationFailed
    \value ImATeapot
    \value MisdirectedRequest
    \value UnprocessableEntity
    \value Locked
    \value FailedDependency
    \value UpgradeRequired
    \value PreconditionRequired
    \value TooManyRequests
    \value RequestHeaderFieldsTooLarge
    \value UnavailableForLegalReasons

    \value InternalServerError
    \value NotImplemented
    \value BadGateway
    \value ServiceUnavailable
    \value GatewayTimeout
    \value HttpVersionNotSupported
    \value VariantAlsoNegotiates
    \value InsufficientStorage
    \value LoopDetected
    \value NotExtended
    \value NetworkAuthenticationRequired
    \value NetworkConnectTimeoutError
*/

/*!
    \internal
*/
static const QLoggingCategory &rspLc()
{
    static const QLoggingCategory category("qt.httpserver.response");
    return category;
}

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

/*!
    \internal
*/
template <qint64 BUFFERSIZE = 128 * 1024>
struct IOChunkedTransfer
{
    // TODO This is not the fastest implementation, as it does read & write
    // in a sequential fashion, but these operation could potentially overlap.
    // TODO Can we implement it without the buffer? Direct write to the target buffer
    // would be great.

    const qint64 bufferSize = BUFFERSIZE;
    char buffer[BUFFERSIZE];
    qint64 beginIndex = -1;
    qint64 endIndex = -1;
    QPointer<QIODevice> source;
    const QPointer<QIODevice> sink;
    const QMetaObject::Connection bytesWrittenConnection;
    const QMetaObject::Connection readyReadConnection;
    IOChunkedTransfer(QIODevice *input, QIODevice *output) :
        source(input),
        sink(output),
        bytesWrittenConnection(QObject::connect(sink.data(), &QIODevice::bytesWritten, sink.data(), [this]() {
              writeToOutput();
        })),
        readyReadConnection(QObject::connect(source.data(), &QIODevice::readyRead, source.data(), [this]() {
            readFromInput();
        }))
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

    inline bool isBufferEmpty()
    {
        Q_ASSERT(beginIndex <= endIndex);
        return beginIndex == endIndex;
    }

    void readFromInput()
    {
        if (source.isNull())
            return;

        if (!isBufferEmpty()) // We haven't consumed all the data yet.
            return;
        beginIndex = 0;
        endIndex = source->read(buffer, bufferSize);
        if (endIndex < 0) {
            endIndex = beginIndex; // Mark the buffer as empty
            qCWarning(rspLc, "Error reading chunk: %ls", qUtf16Printable(source->errorString()));
        } else if (endIndex) {
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

        const auto writtenBytes = sink->write(buffer + beginIndex, endIndex);
        if (writtenBytes < 0) {
            qCWarning(rspLc, "Error writing chunk: %ls", qUtf16Printable(sink->errorString()));
            return;
        }
        beginIndex += writtenBytes;
        if (isBufferEmpty()) {
            if (source->bytesAvailable())
                QTimer::singleShot(0, source.data(), [this]() { readFromInput(); });
            else if (source->atEnd()) // Finishing
                source->deleteLater();
        }
    }
};

/*!
    \internal
*/
QHttpServerResponder::QHttpServerResponder(QHttpServerStream *stream)
    : d_ptr(new QHttpServerResponderPrivate(stream))
{
    Q_ASSERT(stream);
    Q_ASSERT(!stream->handlingRequest);
    stream->handlingRequest = true;
}

/*!
    Move-constructs a QHttpServerResponder instance, making it point
    at the same object that \a other was pointing to.
*/
QHttpServerResponder::QHttpServerResponder(QHttpServerResponder &&other)
    : d_ptr(std::move(other.d_ptr))
{}

/*!
    Destroys a QHttpServerResponder.
*/
QHttpServerResponder::~QHttpServerResponder()
{
    Q_D(QHttpServerResponder);
    if (d) {
        Q_ASSERT(d->stream);
        d->stream->responderDestroyed();
    }
}

/*!
    Answers a request with an HTTP status code \a status and
    HTTP headers \a headers. The I/O device \a data provides the body
    of the response. If \a data is sequential, the body of the
    message is sent in chunks: otherwise, the function assumes all
    the content is available and sends it all at once but the read
    is done in chunks.

    \note This function takes the ownership of \a data.
*/
void QHttpServerResponder::write(QIODevice *data,
                                 HeaderList headers,
                                 StatusCode status)
{
    Q_D(QHttpServerResponder);
    Q_ASSERT(d->stream);
    std::unique_ptr<QIODevice, QScopedPointerDeleteLater> input(data);

    input->setParent(nullptr);
    if (!input->isOpen()) {
        if (!input->open(QIODevice::ReadOnly)) {
            // TODO Add developer error handling
            qCDebug(rspLc, "500: Could not open device %ls", qUtf16Printable(input->errorString()));
            write(StatusCode::InternalServerError);
            return;
        }
    } else if (!(input->openMode() & QIODevice::ReadOnly)) {
        // TODO Add developer error handling
        qCDebug(rspLc) << "500: Device is opened in a wrong mode" << input->openMode();
        write(StatusCode::InternalServerError);
        return;
    }

    writeStatusLine(status);

    if (!input->isSequential()) { // Non-sequential QIODevice should know its data size
        writeHeader(QHttpServerLiterals::contentLengthHeader(),
                    QByteArray::number(input->size()));
    }

    for (auto &&header : headers)
        writeHeader(header.first, header.second);

    d->stream->write("\r\n");

    if (input->atEnd()) {
        qCDebug(rspLc, "No more data available.");
        return;
    }

    // input takes ownership of the IOChunkedTransfer pointer inside his constructor
    new IOChunkedTransfer<>(input.release(), d->stream->socket);
}

/*!
    Answers a request with an HTTP status code \a status and a
    MIME type \a mimeType. The I/O device \a data provides the body
    of the response. If \a data is sequential, the body of the
    message is sent in chunks: otherwise, the function assumes all
    the content is available and sends it all at once but the read
    is done in chunks.

    \note This function takes the ownership of \a data.
*/
void QHttpServerResponder::write(QIODevice *data,
                                 const QByteArray &mimeType,
                                 StatusCode status)
{
    write(data,
          {{ QHttpServerLiterals::contentTypeHeader(), mimeType }},
          status);
}

/*!
    Answers a request with an HTTP status code \a status, JSON
    document \a document and HTTP headers \a headers.

    Note: This function sets HTTP Content-Type header as "application/json".
*/
void QHttpServerResponder::write(const QJsonDocument &document,
                                 HeaderList headers,
                                 StatusCode status)
{
    const QByteArray &json = document.toJson();

    writeStatusLine(status);
    writeHeader(QHttpServerLiterals::contentTypeHeader(),
                QHttpServerLiterals::contentTypeJson());
    writeHeader(QHttpServerLiterals::contentLengthHeader(),
                QByteArray::number(json.size()));
    writeHeaders(std::move(headers));
    writeBody(document.toJson());
}

/*!
    Answers a request with an HTTP status code \a status, and JSON
    document \a document.

    Note: This function sets HTTP Content-Type header as "application/json".
*/
void QHttpServerResponder::write(const QJsonDocument &document,
                                 StatusCode status)
{
    write(document, {}, status);
}

/*!
    Answers a request with an HTTP status code \a status,
    HTTP Headers \a headers and a body \a data.

    Note: This function sets HTTP Content-Length header.
*/
void QHttpServerResponder::write(const QByteArray &data,
                                 HeaderList headers,
                                 StatusCode status)
{
    writeStatusLine(status);

    for (auto &&header : headers)
        writeHeader(header.first, header.second);

    writeHeader(QHttpServerLiterals::contentLengthHeader(),
                QByteArray::number(data.size()));
    writeBody(data);
}

/*!
    Answers a request with an HTTP status code \a status, a
    MIME type \a mimeType and a body \a data.
*/
void QHttpServerResponder::write(const QByteArray &data,
                                 const QByteArray &mimeType,
                                 StatusCode status)
{
    write(data,
          {{ QHttpServerLiterals::contentTypeHeader(), mimeType }},
          status);
}

/*!
    Answers a request with an HTTP status code \a status.

    Note: This function sets HTTP Content-Type header as "application/x-empty".
*/
void QHttpServerResponder::write(StatusCode status)
{
    write(QByteArray(), QHttpServerLiterals::contentTypeXEmpty(), status);
}

/*!
    Answers a request with an HTTP status code \a status and
    HTTP Headers \a headers.
*/
void QHttpServerResponder::write(HeaderList headers, StatusCode status)
{
    write(QByteArray(), std::move(headers), status);
}

/*!
    This function writes HTTP status line with an HTTP status code \a status.
*/
void QHttpServerResponder::writeStatusLine(StatusCode status)
{
    Q_D(QHttpServerResponder);
    Q_ASSERT(d->stream);
    d->bodyStarted = false;
    d->stream->write("HTTP/1.1 ");
    d->stream->write(QByteArray::number(quint32(status)));
    const auto it = statusString.find(status);
    if (it != statusString.end()) {
        d->stream->write(" ");
        d->stream->write(statusString.at(status));
    }
    d->stream->write("\r\n");
}

/*!
    This function writes an HTTP header \a header
    with \a value.
*/
void QHttpServerResponder::writeHeader(const QByteArray &header,
                                       const QByteArray &value)
{
    Q_D(const QHttpServerResponder);
    Q_ASSERT(d->stream);
    d->stream->write(header);
    d->stream->write(": ");
    d->stream->write(value);
    d->stream->write("\r\n");
}

/*!
    This function writes HTTP headers \a headers.
*/
void QHttpServerResponder::writeHeaders(HeaderList headers)
{
    for (auto &&header : headers)
        writeHeader(header.first, header.second);
}

/*!
    This function writes HTTP body \a body with size \a size.
*/
void QHttpServerResponder::writeBody(const char *body, qint64 size)
{
    Q_D(QHttpServerResponder);

    Q_ASSERT(d->stream);

    if (!d->bodyStarted) {
        d->stream->write("\r\n");
        d->bodyStarted = true;
    }

    d->stream->write(body, size);
}

/*!
    This function writes HTTP body \a body.
*/
void QHttpServerResponder::writeBody(const char *body)
{
    writeBody(body, qstrlen(body));
}

/*!
    This function writes HTTP body \a body.
*/
void QHttpServerResponder::writeBody(const QByteArray &body)
{
    writeBody(body.constData(), body.size());
}

/*!
    Sends a HTTP \a response to the client.

    \since 6.5
*/
void QHttpServerResponder::sendResponse(const QHttpServerResponse &response)
{
    const auto &d = response.d_ptr;

    writeStatusLine(d->statusCode);

    for (auto &&header : d->headers)
        writeHeader(header.first, header.second);

    writeHeader(QHttpServerLiterals::contentLengthHeader(),
                QByteArray::number(d->data.size()));

    writeBody(d->data);
}

QT_END_NAMESPACE
