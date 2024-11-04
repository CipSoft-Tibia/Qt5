// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qgrpchttp2channel.h"

#include "qabstractgrpcclient.h"
#include "qgrpcchanneloperation.h"

#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>
#include <QtCore/qendian.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qlist.h>
#include <QtCore/qqueue.h>
#include <QtNetwork/private/hpack_p.h>
#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/private/qhttp2connection_p.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qtcpsocket.h>
#if QT_CONFIG(ssl)
#  include <QtNetwork/qsslsocket.h>
#endif
#include <QtProtobuf/qprotobufserializer.h>
#include <qtgrpcglobal_p.h>

#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcHttp2Channel
    \inmodule QtGrpc

    \brief The QGrpcHttp2Channel class is an HTTP/2-based of
    QAbstractGrpcChannel, based on \l {Qt Network} HTTP/2 implementation.

    Uses \l QGrpcChannelOptions and \l QGrpcCallOptions
    to control the HTTP/2 communication with the server.

    Use \l QGrpcChannelOptions to set the SSL configuration,
    application-specific HTTP/2 headers, and connection timeouts.

    \l QGrpcCallOptions control channel parameters for the
    specific unary call or gRPC stream.

    \sa QGrpcChannelOptions, QGrpcCallOptions, QSslConfiguration
*/

namespace {

constexpr QByteArrayView AuthorityHeader(":authority");
constexpr QByteArrayView MethodHeader(":method");
constexpr QByteArrayView PathHeader(":path");
constexpr QByteArrayView SchemeHeader(":scheme");

constexpr QByteArrayView ContentTypeHeader("content-type");
constexpr QByteArrayView AcceptEncodingHeader("accept-encoding");
constexpr QByteArrayView TEHeader("te");

constexpr QByteArrayView GrpcServiceNameHeader("service-name");
constexpr QByteArrayView GrpcAcceptEncodingHeader("grpc-accept-encoding");
constexpr QByteArrayView GrpcStatusHeader("grpc-status");
constexpr QByteArrayView GrpcStatusMessageHeader("grpc-message");
constexpr qsizetype GrpcMessageSizeHeaderSize = 5;

// This HTTP/2 Error Codes to QGrpcStatus::StatusCode mapping should be kept in sync
// with the following docs:
//     https://www.rfc-editor.org/rfc/rfc7540#section-7
//     https://github.com/grpc/grpc/blob/master/doc/statuscodes.md
//     https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
constexpr QGrpcStatus::StatusCode http2ErrorToStatusCode(const quint32 http2Error)
{
    using StatusCode = QGrpcStatus::StatusCode;
    using Http2Error = Http2::Http2Error;
    switch (http2Error) {
    case Http2Error::HTTP2_NO_ERROR:
    case Http2Error::PROTOCOL_ERROR:
    case Http2Error::INTERNAL_ERROR:
    case Http2Error::FLOW_CONTROL_ERROR:
    case Http2Error::SETTINGS_TIMEOUT:
    case Http2Error::STREAM_CLOSED:
    case Http2Error::FRAME_SIZE_ERROR:
        return StatusCode::Internal;
    case Http2Error::REFUSE_STREAM:
        return StatusCode::Unavailable;
    case Http2Error::CANCEL:
        return StatusCode::Cancelled;
    case Http2Error::COMPRESSION_ERROR:
    case Http2Error::CONNECT_ERROR:
        return StatusCode::Internal;
    case Http2Error::ENHANCE_YOUR_CALM:
        return StatusCode::ResourceExhausted;
    case Http2Error::INADEQUATE_SECURITY:
        return StatusCode::PermissionDenied;
    case Http2Error::HTTP_1_1_REQUIRED:
        return StatusCode::Unknown;
    }
    return StatusCode::Internal;
}

} // namespace

class QGrpcSocketHandler;

struct QGrpcHttp2ChannelPrivate : public QObject
{
    explicit QGrpcHttp2ChannelPrivate(const QGrpcChannelOptions &options);
    ~QGrpcHttp2ChannelPrivate();

    void processOperation(std::shared_ptr<QGrpcChannelOperation> &&channelOperation,
                          bool endStream = false);
private:
    Q_DISABLE_COPY_MOVE(QGrpcHttp2ChannelPrivate)

    enum ConnectionState {
        Connecting = 0,
        Connected,
        Error
    };

    struct ExpectedData
    {
        qsizetype expectedSize = 0;
        QByteArray container;
        bool updateExpectedSize()
        {
            if (expectedSize == 0) {
                if (container.size() < GrpcMessageSizeHeaderSize)
                    return false;

                const auto src = *reinterpret_cast<const quint32 *>(container.data() + 1);
                expectedSize = qFromBigEndian(src) + GrpcMessageSizeHeaderSize;
            }
            return true;
        }
    };

    struct Http2Handler
    {
        explicit Http2Handler(QHttp2Stream *_stream);
        ~Http2Handler();
        void sendData(QByteArrayView data, bool endStream = false);
        void processQueue();
        void cancel();

        QPointer<QHttp2Stream> stream;
        QBuffer *buffer;
        QQueue<QByteArray> queue;
        ExpectedData expectedData;
        bool cancelled = false;

    private:
        Q_DISABLE_COPY_MOVE(Http2Handler)
    };

    struct ChannelOperation
    {
        ChannelOperation(std::shared_ptr<QGrpcChannelOperation> &&_operation, bool _endStream)
            : operation(_operation), endStream(_endStream)
        {
        }

        ChannelOperation(ChannelOperation &&other) = default;
        ChannelOperation &operator=(ChannelOperation &&other) = default;

        std::shared_ptr<QGrpcChannelOperation> operation;
        bool endStream = false;

    private:
        Q_DISABLE_COPY(ChannelOperation)
    };

    void channelOperationAsyncError(QGrpcChannelOperation *channelOperation,
                                    const QGrpcStatus &status);
    template <typename T>
    void connectErrorHandler(T *socket, QGrpcChannelOperation *channelOperation)
    {
        QObject::connect(socket, &T::errorOccurred, channelOperation,
                         [channelOperationPtr = QPointer(channelOperation)](auto error) {
                             emit channelOperationPtr
                                 ->errorOccurred({ QGrpcStatus::StatusCode::Unavailable,
                                                   QString("Network error occurred %1"_L1)
                                                       .arg(error) });
                             // The errorOccured signal can remove the last channelOperation holder,
                             // and in the same time the last finished signal listener, so we need
                             // to make sure that channelOperationPtr is still valid before
                             if (!channelOperationPtr.isNull())
                                 emit channelOperationPtr->finished();
                         });
    }

    void sendRequest(const ChannelOperation &channelOperation);
    void sendPendingRequests();
    void createHttp2Connection();
    void handleSocketError();

    Http2Handler *createHandler(QHttp2Stream *stream);
    void deleteHandler(Http2Handler *handler);

    template<typename T>
    T* initSocket()
    {
        auto p = std::make_unique<T>();
        T *typedSocket = p.get();
        m_socket = std::move(p);
        return typedSocket;
    }

    QGrpcChannelOptions m_channelOptions;
    std::unique_ptr<QIODevice> m_socket = nullptr;
    QHttp2Connection *m_connection = nullptr;
    std::vector<ChannelOperation> m_operations;
    QList<Http2Handler *> m_activeHandlers;
    bool m_isLocalSocket = false;
    QByteArray m_contentType;
    ConnectionState m_state = Connecting;
    std::function<void()> m_reconnectFunction;
};

QGrpcHttp2ChannelPrivate::Http2Handler::Http2Handler(QHttp2Stream *_stream)
    : stream(_stream), buffer(new QBuffer(_stream))
{
}

QGrpcHttp2ChannelPrivate::Http2Handler::~Http2Handler()
{
    if (stream) {
        QHttp2Stream *streamPtr = stream.get();
        stream.clear();
        delete streamPtr;
    }
}

// Sends the errorOccured and finished signals asynchronously to make sure user connections work
// correctly.
void QGrpcHttp2ChannelPrivate::channelOperationAsyncError(QGrpcChannelOperation *channelOperation,
                                                          const QGrpcStatus &status)
{
    Q_ASSERT_X(channelOperation != nullptr, "QGrpcHttp2ChannelPrivate::channelOperationAsyncError",
               "channelOperation is null");
    QTimer::singleShot(0, channelOperation,
                       [channelOperationPtr = QPointer(channelOperation), status]() {
                           emit channelOperationPtr->errorOccurred(status);

                           // The errorOccured signal can remove the last channelOperation holder,
                           // and in the same time the last finished signal listener, so we need
                           // to make sure that channelOperationPtr is still valid before
                           // emitting the finished signal.
                           if (!channelOperationPtr.isNull())
                               emit channelOperationPtr->finished();
                       });
}

// Do not send the data immediately, but put it to message queue, for further processing.
// The data for cancelled stream is ignored.
void QGrpcHttp2ChannelPrivate::Http2Handler::sendData(QByteArrayView data, bool endStream)
{
    if (cancelled || stream->state() == QHttp2Stream::State::HalfClosedLocal
        || stream->state() == QHttp2Stream::State::Closed) {
        qGrpcDebug("Attempt sending data in ended operation");
        return;
    }

    QByteArray msg(GrpcMessageSizeHeaderSize, '\0');
    // Args must be 4-byte unsigned int to fit into 4-byte big endian
    qToBigEndian(static_cast<quint32>(data.size()), msg.data() + 1);
    msg += data;

    queue.enqueue(msg);

    // If the operation is the single-request operation such as unary call or server stream
    // we should send the empty DATA frame with the END_STREAM flag as the end of transmission
    // indicator.
    if (endStream)
        queue.enqueue({});

    processQueue();
}

// Once steam is ready to upload more data, send it.
void QGrpcHttp2ChannelPrivate::Http2Handler::processQueue()
{
    if (stream->isUploadingDATA())
        return;

    if (queue.isEmpty())
        return;

    QByteArray data = queue.dequeue();

    buffer->close();
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);
    stream->sendDATA(buffer, data.isEmpty());
}

// gRPC cancellation happens by sending empty DATA frame with the END_STREAM bit
void QGrpcHttp2ChannelPrivate::Http2Handler::cancel()
{
    if (cancelled)
        return;

    cancelled = true;

    // Stream is already is (half)closed, skip sending the cancellation DATA frame.
    if (stream->state() == QHttp2Stream::State::HalfClosedLocal
        || stream->state() == QHttp2Stream::State::Closed)
        return;

    // Clear the existing queue and enqeue empty data. Data should contains at least the payload
    // size, even if payload is 0. Empty data is the explicit indicator for stream cancellation.
    queue.clear();
    queue.enqueue({});
    processQueue();
}

QGrpcHttp2ChannelPrivate::QGrpcHttp2ChannelPrivate(const QGrpcChannelOptions &options)
    : m_channelOptions(options)
{
    QUrl url = m_channelOptions.host();
    if (url.scheme() == "unix"_L1) {
        auto *localSocket = initSocket<QLocalSocket>();
        m_isLocalSocket = true;

        QObject::connect(localSocket, &QLocalSocket::connected, this,
                         &QGrpcHttp2ChannelPrivate::createHttp2Connection);
        QObject::connect(localSocket, &QLocalSocket::errorOccurred, this,
                         [this, url](QLocalSocket::LocalSocketError error) {
                             qGrpcDebug()
                                 << "Error occurred(" << error << "):"
                                 << static_cast<QLocalSocket *>(m_socket.get())->errorString()
                                 << url;
                             handleSocketError();
                         });
        m_reconnectFunction = [localSocket, url] {
            localSocket->connectToServer(url.host() + url.path());
        };
    } else
#if QT_CONFIG(ssl)
    if (url.scheme() == "https"_L1 || options.sslConfiguration()) {
        auto *sslSocket = initSocket<QSslSocket>();
        if (url.port() < 0) {
            url.setPort(443);
        }

        if (const auto userSslConfig = options.sslConfiguration(); userSslConfig) {
            sslSocket->setSslConfiguration(*userSslConfig);
        } else {
            static const QByteArray h2NexProtocol = "h2"_ba;
            auto defautlSslConfig = QSslConfiguration::defaultConfiguration();
            auto allowedNextProtocols = defautlSslConfig.allowedNextProtocols();
            if (!allowedNextProtocols.contains(h2NexProtocol))
                allowedNextProtocols.append(h2NexProtocol);
            defautlSslConfig.setAllowedNextProtocols(allowedNextProtocols);
            sslSocket->setSslConfiguration(defautlSslConfig);
        }

        QObject::connect(sslSocket, &QSslSocket::encrypted, this,
                         &QGrpcHttp2ChannelPrivate::createHttp2Connection);
        QObject::connect(sslSocket, &QAbstractSocket::errorOccurred, this,
                         [this, url](QAbstractSocket::SocketError error) {
                             qDebug()
                                 << "Error occurred(" << error << "):"
                                 << static_cast<QAbstractSocket *>(m_socket.get())->errorString()
                                 << url;
                             handleSocketError();
                         });
        m_reconnectFunction = [sslSocket, url] {
            sslSocket->connectToHostEncrypted(url.host(), static_cast<quint16>(url.port()));
        };
    } else
#endif
    {
        auto *httpSocket = initSocket<QTcpSocket>();
        if (url.port() < 0) {
            url.setPort(80);
        }

        QObject::connect(httpSocket, &QAbstractSocket::connected, this,
                         &QGrpcHttp2ChannelPrivate::createHttp2Connection);
        QObject::connect(httpSocket, &QAbstractSocket::errorOccurred, this,
                         [this, url](QAbstractSocket::SocketError error) {
                             qGrpcDebug()
                                 << "Error occurred(" << error << "):"
                                 << static_cast<QAbstractSocket *>(m_socket.get())->errorString()
                                 << url;
                             handleSocketError();
                         });
        m_reconnectFunction = [httpSocket, url] {
            httpSocket->connectToHost(url.host(), static_cast<quint16>(url.port()));
        };
    }
    m_reconnectFunction();
}

QGrpcHttp2ChannelPrivate::~QGrpcHttp2ChannelPrivate()
{
    qDeleteAll(m_activeHandlers);
}

void QGrpcHttp2ChannelPrivate::processOperation(std::shared_ptr<QGrpcChannelOperation>
                                                    &&channelOperation,
                                                bool endStream)
{
    auto *channelOperationPtr = channelOperation.get();
    Q_ASSERT_X(channelOperationPtr != nullptr, "QGrpcHttp2ChannelPrivate::processOperation",
               "Channel operation is nullptr.");

    if (!m_socket->isWritable()) {
        channelOperationAsyncError(channelOperationPtr,
                                   { QGrpcStatus::StatusCode::Unavailable,
                                     m_socket->errorString() });
        return;
    }

    if (m_isLocalSocket) {
        connectErrorHandler<QLocalSocket>(static_cast<QLocalSocket *>(m_socket.get()),
                                          channelOperationPtr);
    } else {
        connectErrorHandler<QAbstractSocket>(static_cast<QAbstractSocket *>(m_socket.get()),
                                             channelOperationPtr);
    }

    ChannelOperation operation(std::move(channelOperation), endStream);
    if (m_connection == nullptr)
        m_operations.emplace_back(std::move(operation));
    else
        sendRequest(operation);

    if (m_state == ConnectionState::Error) {
        Q_ASSERT_X(m_reconnectFunction, "QGrpcHttp2ChannelPrivate::processOperation",
                   "Socket reconnection function is not defined.");
        m_reconnectFunction();
        m_state = ConnectionState::Connecting;
    }
}

void QGrpcHttp2ChannelPrivate::createHttp2Connection()
{
    Q_ASSERT_X(m_connection == nullptr, "QGrpcHttp2ChannelPrivate::createHttp2Connection",
               "Attempt to create the HTTP/2 connection, but it already exists. This situation is "
               "exceptional.");
    m_connection = QHttp2Connection::createDirectConnection(m_socket.get(), {});

    if (m_connection) {
        QObject::connect(m_socket.get(), &QAbstractSocket::readyRead, m_connection,
                         &QHttp2Connection::handleReadyRead);
        m_state = ConnectionState::Connected;
    }

    for (const auto &operation : m_operations)
        sendRequest(operation);

    m_operations.clear();
}

void QGrpcHttp2ChannelPrivate::handleSocketError()
{
    qDeleteAll(m_activeHandlers);
    m_activeHandlers.clear();
    delete m_connection;
    m_connection = nullptr;
    m_state = ConnectionState::Error;
    m_operations.clear();
}

void QGrpcHttp2ChannelPrivate::sendRequest(const ChannelOperation &channelOperation)
{
    Q_ASSERT_X(channelOperation.operation != nullptr, "QGrpcHttp2ChannelPrivate::sendRequest",
               "channelOperation is null");

    auto channelOpPtr = channelOperation.operation.get();
    if (!m_connection) {
        channelOperationAsyncError(channelOpPtr,
                                   { QGrpcStatus::Unavailable,
                                     "Unable to establish a HTTP/2 connection"_L1 });
        return;
    }

    const auto streamAttempt = m_connection->createStream();
    if (!streamAttempt.ok()) {
        channelOperationAsyncError(channelOpPtr,
                                   { QGrpcStatus::Unavailable,
                                     "Unable to create a HTTP/2 stream"_L1 });
        return;
    }
    Http2Handler *handler = createHandler(streamAttempt.unwrap());

    QObject::connect(handler->stream.get(), &QHttp2Stream::headersReceived, channelOpPtr,
                     [handler,
                      channelOpInnerPtr = QPointer(channelOpPtr)](const HPack::HttpHeader &headers,
                                                                  bool endStream) {
                         QGrpcMetadata md = channelOpInnerPtr->serverMetadata();
                         QGrpcStatus::StatusCode statusCode = QGrpcStatus::StatusCode::Ok;
                         QString statusMessage;
                         for (const auto &header : headers) {
                             md.insert(std::pair<QByteArray, QByteArray>(header.name,
                                                                         header.value));
                             if (header.name == GrpcStatusHeader)
                                 statusCode = static_cast<
                                     QGrpcStatus::StatusCode>(QString::fromLatin1(header.value)
                                                                  .toShort());
                             else if (header.name == GrpcStatusMessageHeader)
                                 statusMessage = QString::fromUtf8(header.value);
                         }

                         channelOpInnerPtr->setServerMetadata(md);

                         if (statusCode != QGrpcStatus::StatusCode::Ok)
                             emit channelOpInnerPtr->errorOccurred({ statusCode, statusMessage });

                         // The errorOccured signal can remove the last channelOperation holder,
                         // and in the same time the last finished signal listener, so we need
                         // to make sure that channelOpInnerPtr is still valid before
                         // emitting the finished signal.
                         if (endStream && !handler->cancelled && !channelOpInnerPtr.isNull())
                             emit channelOpInnerPtr->finished();
                     });

    QObject::connect(handler->stream.get(), &QHttp2Stream::errorOccurred, channelOpPtr,
                     [this, channelOpPtr, handler](quint32 http2ErrorCode, const QString &errorString) {
                         deleteHandler(handler);
                         // Check for HTTP2_NO_ERROR to avoid invalid error emission.
                         // This is fixed in Qt6.8
                         if (http2ErrorCode == 0) {
                             return;
                         }
                         QGrpcStatus::StatusCode code = http2ErrorToStatusCode(http2ErrorCode);
                         emit channelOpPtr->errorOccurred({ code, errorString });
                     });

    QObject::connect(handler->stream.get(), &QHttp2Stream::dataReceived, channelOpPtr,
                     [channelOpPtr, handler](const QByteArray &data, bool endStream) {
                         if (!handler->cancelled) {
                             handler->expectedData.container.append(data);

                             if (!handler->expectedData.updateExpectedSize())
                                 return;

                             while (handler->expectedData.container.size()
                                    >= handler->expectedData.expectedSize) {
                                 qGrpcDebug()
                                     << "Full data received:" << data.size()
                                     << "dataContainer:" << handler->expectedData.container.size()
                                     << "capacity:" << handler->expectedData.expectedSize;
                                 emit channelOpPtr
                                     ->dataReady(handler->expectedData.container
                                                     .mid(GrpcMessageSizeHeaderSize,
                                                          handler->expectedData.expectedSize
                                                              - GrpcMessageSizeHeaderSize));
                                 handler->expectedData.container
                                     .remove(0, handler->expectedData.expectedSize);
                                 handler->expectedData.expectedSize = 0;
                                 if (!handler->expectedData.updateExpectedSize())
                                     return;
                             }
                         }

                         if (endStream)
                             emit channelOpPtr->finished();
                     });

    QObject::connect(channelOpPtr, &QGrpcChannelOperation::cancelled, handler->stream.get(),
                     [handler]() { handler->cancel(); });

    QObject::connect(channelOpPtr, &QGrpcChannelOperation::sendData, handler->stream.get(),
                     [handler](const QByteArray &data) { handler->sendData(data); });

    QObject::connect(handler->stream.get(), &QHttp2Stream::uploadFinished, handler->stream.get(),
                     [handler]() { handler->processQueue(); });

    QByteArray service{ channelOpPtr->service().data(), channelOpPtr->service().size() };
    QByteArray method{ channelOpPtr->method().data(), channelOpPtr->method().size() };

    HPack::HttpHeader requestHeaders = HPack::HttpHeader{
        { AuthorityHeader.toByteArray(), m_channelOptions.host().host().toLatin1() },
        { MethodHeader.toByteArray(), "POST"_ba },
        { PathHeader.toByteArray(), QByteArray('/' + service + '/' + method) },
        { SchemeHeader.toByteArray(),
          m_isLocalSocket ? "http"_ba : m_channelOptions.host().scheme().toLatin1() },
        { ContentTypeHeader.toByteArray(), "application/grpc"_ba },
        { GrpcServiceNameHeader.toByteArray(), { service } },
        { GrpcAcceptEncodingHeader.toByteArray(), "identity,deflate,gzip"_ba },
        { AcceptEncodingHeader.toByteArray(), "identity,gzip"_ba },
        { TEHeader.toByteArray(), "trailers"_ba },
    };

    auto iterateMetadata = [&requestHeaders](const auto &metadata) {
        for (const auto &[key, value] : std::as_const(metadata)) {
            auto lowerKey = key.toLower();
            if (lowerKey == AuthorityHeader ||
                lowerKey == MethodHeader ||
                lowerKey == PathHeader ||
                lowerKey == SchemeHeader)
                continue;
            requestHeaders.push_back({ lowerKey, value });
        }
    };

    iterateMetadata(m_channelOptions.metadata());
    iterateMetadata(channelOpPtr->options().metadata());

    if (!handler->stream->sendHEADERS(requestHeaders, false)) {
        channelOperationAsyncError(channelOpPtr,
                                   { QGrpcStatus::Unavailable,
                                     "Unable to create HTTP2 stream"_L1 });
        return;
    }
    handler->sendData(channelOpPtr->argument(), channelOperation.endStream);
}

QGrpcHttp2ChannelPrivate::Http2Handler *QGrpcHttp2ChannelPrivate::createHandler(QHttp2Stream *stream)
{
    Http2Handler *handler = new Http2Handler(stream);
    m_activeHandlers.append(handler);
    return handler;
}

void QGrpcHttp2ChannelPrivate::deleteHandler(Http2Handler *handler)
{
    const auto it = std::find(m_activeHandlers.constBegin(), m_activeHandlers.constEnd(), handler);
    Q_ASSERT_X(it != m_activeHandlers.constEnd(), "QGrpcHttp2ChannelPrivate::deleteHandler",
               "Attempt to delete unregistered Http2Handler");
    delete *it;
    m_activeHandlers.erase(it);
}

/*!
    Constructs QGrpcHttp2Channel with \a options.
*/
QGrpcHttp2Channel::QGrpcHttp2Channel(const QGrpcChannelOptions &options)
    : QAbstractGrpcChannel(options), dPtr(std::make_unique<QGrpcHttp2ChannelPrivate>(options))
{
}

/*!
    Destroys the QGrpcHttp2Channel object.
*/
QGrpcHttp2Channel::~QGrpcHttp2Channel() = default;

/*!
    \internal
    Implementation of unary gRPC call based on \l QNetworkAccessManager.
*/
void QGrpcHttp2Channel::call(std::shared_ptr<QGrpcChannelOperation> channelOperation)
{
    dPtr->processOperation(std::move(channelOperation), true);
}

/*!
    \internal
    Implementation of server-side gRPC stream based on \l QNetworkAccessManager.
*/
void QGrpcHttp2Channel::startServerStream(std::shared_ptr<QGrpcChannelOperation> channelOperation)
{
    dPtr->processOperation(std::move(channelOperation), true);
}

/*!
    \internal
    Implementation of client-side gRPC stream based on \l QNetworkAccessManager.
*/
void QGrpcHttp2Channel::startClientStream(std::shared_ptr<QGrpcChannelOperation> channelOperation)
{
    dPtr->processOperation(std::move(channelOperation));
}

/*!
    \internal
    Implementation of bidirectional gRPC stream based on \l QNetworkAccessManager.
*/
void QGrpcHttp2Channel::startBidirStream(std::shared_ptr<QGrpcChannelOperation> channelOperation)
{
    dPtr->processOperation(std::move(channelOperation));
}

/*!
    Returns the newly created QProtobufSerializer shared pointer.
*/
std::shared_ptr<QAbstractProtobufSerializer> QGrpcHttp2Channel::serializer() const noexcept
{
    // TODO: make selection based on credentials or channel settings
    return std::make_shared<QProtobufSerializer>();
}

QT_END_NAMESPACE
