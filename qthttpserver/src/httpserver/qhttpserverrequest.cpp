// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qhttpserverrequest_p.h"

#include <QtHttpServer/qhttpserverrequest.h>

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qtcpsocket.h>
#if QT_CONFIG(ssl)
#include <QtNetwork/qsslsocket.h>
#endif

Q_LOGGING_CATEGORY(lc, "qt.httpserver.request")

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if !defined(QT_NO_DEBUG_STREAM)

/*!
    \fn QDebug QHttpServerRequest::operator<<(QDebug debug, const QHttpServerRequest &request)

    Writes information about \a request to the \a debug stream.

    \sa QDebug
 */
Q_HTTPSERVER_EXPORT QDebug operator<<(QDebug debug, const QHttpServerRequest &request)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QHttpServerRequest(";
    debug << "(Url: " << request.url() << ")";
    debug << "(Headers: ";
    auto headers = request.headers();
    bool firstHeader = true;
    for (auto i = headers.begin(); i != headers.end(); ++i) {
        if (firstHeader)
            firstHeader = false;
        else
            debug << ", ";
        debug << "(" << i->first << ": " << i->second << ")";
    }
    debug << ")";
    debug << "(RemoteHost: " << request.remoteAddress() << ")";
    debug << "(BodySize: " << request.body().size() << ")";
    debug << ')';
    return debug;
}

#endif

/*!
    \internal
*/
bool QHttpServerRequestPrivate::parseRequestLine(QByteArrayView line)
{
    // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    auto i = line.indexOf(' ');
    if (i == -1)
        return false;
    const auto requestMethod = line.first(i);
    i++;

    while (i < line.size() && line[i] == ' ')
        i++;

    auto j = line.indexOf(' ', i);
    if (j == -1)
        return false;

    const auto requestUrl = line.sliced(i, j - i);
    i = j + 1;

    while (i < line.size() && line[i] == ' ')
        i++;

    if (i >= line.size())
        return false;

    j = line.indexOf(' ', i);

    const auto protocol = j == -1 ? line.sliced(i) : line.sliced(i, j - i);
    if (protocol.size() != 8 || !protocol.startsWith("HTTP"))
        return false;

    parser.setMajorVersion(protocol[5] - '0');
    parser.setMinorVersion(protocol[7] - '0');

    if (requestMethod == "GET")
        method = QHttpServerRequest::Method::Get;
    else if (requestMethod == "PUT")
        method = QHttpServerRequest::Method::Put;
    else if (requestMethod == "DELETE")
        method = QHttpServerRequest::Method::Delete;
    else if (requestMethod == "POST")
        method = QHttpServerRequest::Method::Post;
    else if (requestMethod == "HEAD")
        method = QHttpServerRequest::Method::Head;
    else if (requestMethod == "OPTIONS")
        method = QHttpServerRequest::Method::Options;
    else if (requestMethod == "PATCH")
        method = QHttpServerRequest::Method::Patch;
    else if (requestMethod == "CONNECT")
        method = QHttpServerRequest::Method::Connect;
    else
        method = QHttpServerRequest::Method::Unknown;

    url = QUrl::fromEncoded(requestUrl.toByteArray());
    return true;
}

/*!
    \internal
*/
qsizetype QHttpServerRequestPrivate::readRequestLine(QAbstractSocket *socket)
{
    if (fragment.isEmpty()) {
        // reserve bytes for the request line. This is better than always append() which reallocs
        // the byte array
        fragment.reserve(32);
    }

    qsizetype bytes = 0;
    char c;
    qsizetype haveRead = 0;

    do {
        haveRead = socket->read(&c, 1);
        if (haveRead == -1)
            return -1; // unexpected EOF
        else if (haveRead == 0)
            break; // read more later
        else if (haveRead == 1 && fragment.size() == 0
                 && (c == '\v' || c == '\n' || c == '\r' || c == ' ' || c == '\t'))
            continue; // Ignore all whitespace that was trailing from a previous request on that
                      // socket

        bytes++;

        // allow both CRLF & LF (only) line endings
        if (c == '\n') {
            // remove the CR at the end
            if (fragment.endsWith('\r')) {
                fragment.truncate(fragment.size() - 1);
            }
            bool ok = parseRequestLine(fragment);
            state = State::ReadingHeader;
            fragment.clear();
            if (!ok) {
                return -1;
            }
            break;
        } else {
            fragment.append(c);
        }
    } while (haveRead == 1);

    return bytes;
}

/*!
    \internal
*/
qint64 QHttpServerRequestPrivate::contentLength() const
{
    bool ok = false;
    QByteArray value = parser.firstHeaderField("content-length");
    qint64 length = value.toULongLong(&ok);
    if (ok)
        return length;
    return -1; // the header field is not set
}

/*!
    \internal
*/
qsizetype QHttpServerRequestPrivate::readHeader(QAbstractSocket *socket)
{
    if (fragment.isEmpty()) {
        // according to
        // https://maqentaer.com/devopera-static-backup/http/dev.opera.com/articles/view/mama-http-headers/index.html
        // the average size of the header block is 381 bytes. reserve bytes. This is better than
        // always append() which reallocs the byte array.
        fragment.reserve(512);
    }

    qint64 bytes = 0;
    char c = 0;
    bool allHeaders = false;
    qint64 haveRead = 0;
    do {
        haveRead = socket->read(&c, 1);
        if (haveRead == 0) {
            // read more later
            break;
        } else if (haveRead == -1) {
            // connection broke down
            return -1;
        } else {
            fragment.append(c);
            bytes++;

            if (c == '\n') {
                // check for possible header endings. As per HTTP rfc,
                // the header endings will be marked by CRLFCRLF. But
                // we will allow CRLFCRLF, CRLFLF, LFCRLF, LFLF
                if (fragment.endsWith("\n\r\n") || fragment.endsWith("\n\n"))
                    allHeaders = true;

                // there is another case: We have no headers. Then the fragment equals just the line
                // ending
                if ((fragment.size() == 2 && fragment.endsWith("\r\n"))
                    || (fragment.size() == 1 && fragment.endsWith("\n")))
                    allHeaders = true;
            }
        }
    } while (!allHeaders && haveRead > 0);

    // we received all headers now parse them
    if (allHeaders) {
        parser.parseHeaders(fragment);
        fragment.clear(); // next fragment

        auto hostUrl = QString::fromUtf8(headerField("host"));
        if (!hostUrl.isEmpty())
            url.setAuthority(hostUrl);

        if (url.scheme().isEmpty()) {
#if QT_CONFIG(ssl)
            auto sslSocket = qobject_cast<QSslSocket *>(socket);
            url.setScheme(sslSocket && sslSocket->isEncrypted() ? u"https"_s : u"http"_s);
#else
            url.setScheme(u"http"_s);
#endif
        }

        if (url.host().isEmpty())
            url.setHost(u"127.0.0.1"_s);

        if (url.port() == -1)
            url.setPort(port);

        bodyLength = contentLength(); // cache the length

        // cache isChunked() since it is called often
        // FIXME: the RFC says that anything but "identity" should be interpreted as chunked (4.4
        // [2])
        chunkedTransferEncoding = headerField("transfer-encoding").toLower().contains("chunked");

        QByteArray connectionHeaderField = headerField("connection");
        upgrade = connectionHeaderField.toLower().contains("upgrade");

        if (chunkedTransferEncoding || bodyLength > 0) {
            if (headerField("expect").compare("100-continue", Qt::CaseInsensitive) == 0)
                state = State::ExpectContinue;
            else
                state = State::ReadingData;
         } else {
            state = State::AllDone;
         }
    }
    return bytes;
}

/*!
    \internal
*/
qsizetype QHttpServerRequestPrivate::sendContinue(QAbstractSocket *socket)
{
    qsizetype ret = socket->write("HTTP/1.1 100 Continue\r\n\r\n");
    state = State::ReadingData;
    return ret;
}

/*!
    \internal
*/
QHttpServerRequestPrivate::QHttpServerRequestPrivate(const QHostAddress &remoteAddress,
                                                     quint16 remotePort,
                                                     const QHostAddress &localAddress,
                                                     quint16 localPort)
    : remoteAddress(remoteAddress),
      remotePort(remotePort),
      localAddress(localAddress),
      localPort(localPort)
{
    clear();
}

/*!
    \internal
*/
bool QHttpServerRequestPrivate::parse(QAbstractSocket *socket)
{
    qsizetype read;

    do {
        switch (state) {
        case State::AllDone:
            clear();
            [[fallthrough]];
        case State::NothingDone:
            state = State::ReadingRequestLine;
            [[fallthrough]];
        case State::ReadingRequestLine:
            read = readRequestLine(socket);
            continue;
        case State::ReadingHeader:
            read = readHeader(socket);
            continue;
        case State::ExpectContinue:
            read = sendContinue(socket);
            continue;
        case State::ReadingData:
            if (chunkedTransferEncoding)
                read = readRequestBodyChunked(socket);
            else
                read = readBodyFast(socket);

            if (state == State::AllDone) {
                body = bodyBuffer.readAll();
                bodyBuffer.clear();
            }

            continue;
        }
        Q_UNREACHABLE(); // fixes GCC -Wmaybe-uninitialized warning on `read`
    } while (state != State::AllDone && read > 0);

    return read != -1;
}

/*!
    \internal
*/
void QHttpServerRequestPrivate::clear()
{
    parser.clear();
    bodyLength = -1;
    contentRead = 0;
    chunkedTransferEncoding = false;
    lastChunkRead = false;
    currentChunkRead = 0;
    currentChunkSize = 0;
    upgrade = false;

    fragment.clear();
    bodyBuffer.clear();
    body.clear();
}

// The body reading functions were mostly copied from QHttpNetworkReplyPrivate

/*!
    \internal
*/
// note this function can only be used for non-chunked, non-compressed with
// known content length
qsizetype QHttpServerRequestPrivate::readBodyFast(QAbstractSocket *socket)
{

    qsizetype toBeRead = qMin(socket->bytesAvailable(), bodyLength - contentRead);
    if (!toBeRead)
        return 0;

    QByteArray bd;
    bd.resize(toBeRead);
    qsizetype haveRead = socket->read(bd.data(), toBeRead);
    if (haveRead == -1) {
        bd.clear();
        return 0; // ### error checking here;
    }
    bd.resize(haveRead);

    bodyBuffer.append(bd);

    contentRead += haveRead;

    if (contentRead == bodyLength)
        state = State::AllDone;

    return haveRead;
}

/*!
    \internal
*/
qsizetype QHttpServerRequestPrivate::readRequestBodyRaw(QAbstractSocket *socket, qsizetype size)
{
    // FIXME get rid of this function and just use readBodyFast and give it socket->bytesAvailable()
    qsizetype bytes = 0;
    Q_ASSERT(socket);

    int toBeRead = qMin<qsizetype>(128 * 1024, qMin<qint64>(size, socket->bytesAvailable()));

    while (toBeRead > 0) {
        QByteArray byteData;
        byteData.resize(toBeRead);
        qsizetype haveRead = socket->read(byteData.data(), byteData.size());
        if (haveRead <= 0) {
            // ### error checking here
            byteData.clear();
            return bytes;
        }

        byteData.resize(haveRead);
        bodyBuffer.append(byteData);
        bytes += haveRead;
        size -= haveRead;

        toBeRead = qMin<qsizetype>(128 * 1024, qMin<qsizetype>(size, socket->bytesAvailable()));
    }
    return bytes;
}

/*!
    \internal
*/
qsizetype QHttpServerRequestPrivate::readRequestBodyChunked(QAbstractSocket *socket)
{
    qsizetype bytes = 0;
    while (socket->bytesAvailable()) {
        if (!lastChunkRead && currentChunkRead >= currentChunkSize) {
            // For the first chunk and when we're done with a chunk
            currentChunkSize = 0;
            currentChunkRead = 0;
            if (bytes) {
                // After a chunk
                char crlf[2];
                // read the "\r\n" after the chunk
                qsizetype haveRead = socket->read(crlf, 2);
                // FIXME: This code is slightly broken and not optimal. What if the 2 bytes are not
                // available yet?! For nice reasons (the toLong in getChunkSize accepting \n at the
                // beginning it right now still works, but we should definitely fix this.

                if (haveRead != 2)
                    return bytes;
                bytes += haveRead;
            }
            // Note that chunk size gets stored in currentChunkSize, what is returned is the bytes
            // read
            bytes += getChunkSize(socket, &currentChunkSize);
            if (currentChunkSize == -1)
                break;
        }
        // if the chunk size is 0, end of the stream
        if (currentChunkSize == 0 || lastChunkRead) {
            lastChunkRead = true;
            // try to read the "\r\n" after the chunk
            char crlf[2];
            qsizetype haveRead = socket->read(crlf, 2);
            if (haveRead > 0)
                bytes += haveRead;

            if ((haveRead == 2 && crlf[0] == '\r' && crlf[1] == '\n')
                || (haveRead == 1 && crlf[0] == '\n')) {
                state = State::AllDone;
            } else if (haveRead == 1 && crlf[0] == '\r') {
                break; // Still waiting for the last \n
            } else if (haveRead > 0) {
                // If we read something else then CRLF, we need to close the channel.
                // FIXME forceConnectionCloseEnabled = true;
                state = State::AllDone;
            }
            break;
        }

        // otherwise, try to begin reading this chunk / to read what is missing for this chunk
        qsizetype haveRead = readRequestBodyRaw(socket, currentChunkSize - currentChunkRead);
        currentChunkRead += haveRead;
        bytes += haveRead;

        // ### error checking here
    }
    return bytes;
}

/*!
    \internal
*/
qsizetype QHttpServerRequestPrivate::getChunkSize(QAbstractSocket *socket, qsizetype *chunkSize)
{
    qsizetype bytes = 0;
    char crlf[2];
    *chunkSize = -1;

    int bytesAvailable = socket->bytesAvailable();
    // FIXME rewrite to permanent loop without bytesAvailable
    while (bytesAvailable > bytes) {
        qsizetype sniffedBytes = socket->peek(crlf, 2);
        int fragmentSize = fragment.size();

        // check the next two bytes for a "\r\n", skip blank lines
        if ((fragmentSize && sniffedBytes == 2 && crlf[0] == '\r' && crlf[1] == '\n')
            || (fragmentSize > 1 && fragment.endsWith('\r') && crlf[0] == '\n')) {
            bytes += socket->read(crlf, 1); // read the \r or \n
            if (crlf[0] == '\r')
                bytes += socket->read(crlf, 1); // read the \n
            bool ok = false;
            // ignore the chunk-extension
            fragment = fragment.mid(0, fragment.indexOf(';')).trimmed();
            *chunkSize = fragment.toLong(&ok, 16);
            fragment.clear();
            break; // size done
        } else {
            // read the fragment to the buffer
            char c = 0;
            qsizetype haveRead = socket->read(&c, 1);
            if (haveRead < 0)
                return -1;

            bytes += haveRead;
            fragment.append(c);
        }
    }

    return bytes;
}

/*!
    \class QHttpServerRequest
    \since 6.4
    \inmodule QtHttpServer
    \brief Encapsulates an HTTP request.

    API for accessing the different parameters of an incoming request.
*/

/*!
    \enum QHttpServerRequest::Method

    This enum type specifies an HTTP request method:

    \value Unknown
        An unknown method.
    \value Get
        HTTP GET method.
    \value Put
        HTTP PUT method.
    \value Delete
        HTTP DELETE method.
    \value Post
        HTTP POST method.
    \value Head
        HTTP HEAD method.
    \value Options
        HTTP OPTIONS method.
    \value Patch
        HTTP PATCH method (\l {https://www.rfc-editor.org/rfc/rfc5789}{RFC 5789}).
    \value Connect
        HTTP CONNECT method.
    \value Trace
        HTTP TRACE method.
    \value AnyKnown
        Combination of all known methods.
*/

/*!
    \internal
*/
QHttpServerRequest::QHttpServerRequest(const QHostAddress &remoteAddress, quint16 remotePort,
                                       const QHostAddress &localAddress, quint16 localPort)
    : d(new QHttpServerRequestPrivate(remoteAddress, remotePort, localAddress, localPort))
{}

/*!
    Destroys a QHttpServerRequest
*/
QHttpServerRequest::~QHttpServerRequest()
{}

/*!
    Returns the combined value of all headers with the named \a key.
*/
QByteArray QHttpServerRequest::value(const QByteArray &key) const
{
    return d->parser.combinedHeaderValue(key);
}

/*!
    Returns the URL the request asked for.
*/
QUrl QHttpServerRequest::url() const
{
    return d->url;
}

/*!
    Returns the query in the request.
*/
QUrlQuery QHttpServerRequest::query() const
{
    return QUrlQuery(d->url.query());
}

/*!
    Returns the method of the request.
*/
QHttpServerRequest::Method QHttpServerRequest::method() const
{
    return d->method;
}

/*!
    Returns all the request headers.
*/
QList<QPair<QByteArray, QByteArray>> QHttpServerRequest::headers() const
{
    return d->parser.headers();
}

/*!
    Returns the body of the request.
*/
QByteArray QHttpServerRequest::body() const
{
    return d->body;
}

/*!
    Returns the address of the origin host of the request.
*/
QHostAddress QHttpServerRequest::remoteAddress() const
{
    return d->remoteAddress;
}

/*!
    Returns the port of the origin host of the request.

    \since 6.5
*/
quint16 QHttpServerRequest::remotePort() const
{
    return d->remotePort;
}

/*!
    Returns the host address of the local socket which received the request.

    \since 6.5
*/
QHostAddress QHttpServerRequest::localAddress() const
{
    return d->localAddress;
}

/*!
    Returns the port of the local socket which received the request.

    \since 6.5
*/
quint16 QHttpServerRequest::localPort() const
{
    return d->localPort;
}

QT_END_NAMESPACE

#include "moc_qhttpserverrequest.cpp"
