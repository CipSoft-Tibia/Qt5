// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/private/qtgrpclogging_p.h>
#include <QtGrpc/qgrpccalloptions.h>
#include <QtGrpc/qgrpcchanneloptions.h>
#include <QtGrpc/qgrpchttp2channel.h>
#include <QtGrpc/qgrpcoperationcontext.h>
#include <QtGrpc/qgrpcserializationformat.h>
#include <QtGrpc/qgrpcstatus.h>

#include <QtProtobuf/qprotobufjsonserializer.h>
#include <QtProtobuf/qprotobufserializer.h>

#include <QtNetwork/private/hpack_p.h>
#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/private/qhttp2connection_p.h>
#if QT_CONFIG(localserver)
#  include <QtNetwork/qlocalsocket.h>
#endif
#include <QtNetwork/qtcpsocket.h>
#if QT_CONFIG(ssl)
#  include <QtNetwork/qsslsocket.h>
#endif

#include <QtCore/private/qnoncontiguousbytedevice_p.h>
#include <QtCore/qalgorithms.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qendian.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qqueue.h>
#include <QtCore/qtimer.h>

#include <functional>
#include <optional>
#include <utility>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtGrpc;

/*!
    \class QGrpcHttp2Channel
    \inmodule QtGrpc
    \brief The QGrpcHttp2Channel class provides a HTTP/2 transport layer
    for \gRPC communication.

    The QGrpcHttp2Channel class implements QAbstractGrpcChannel, enabling \gRPC
    communication carried over \l{https://datatracker.ietf.org/doc/html/rfc7540}
    {HTTP/2 framing}.

    HTTP/2 introduces several advantages over its predecessor, HTTP/1.1, making
    QGrpcHttp2Channel well-suited for high-performance, real-time applications
    that require efficient communication, without sacrificing security or
    reliability, by using multiplexed TCP connections.

    The channel can be customized with \l{Secure Sockets Layer (SSL)
    Classes}{SSL} support, a custom \l{QGrpcChannelOptions::}
    {serializationFormat}, or other options by constructing it with a
    QGrpcChannelOptions containing the required customizations.

    \section2 Transportation scheme

    The QGrpcHttp2Channel implementation prefers different transportation
    methods based on the provided \c{hostUri}, \l{QUrl::}{scheme} and options.
    The following criteria applies:

    \table
    \header
        \li Scheme
        \li Description
        \li Default Port
        \li Requirements
        \li Example
    \row
        \li \c{http}
        \li Unencrypted HTTP/2 over TCP
        \li 80
        \li None
        \li \c{http://localhost}
    \row
        \li \c{https}
        \li TLS-encrypted HTTP/2 over TCP
        \li 443
        \li QSslSocket support \b{AND} (scheme \b{OR} \l{QGrpcChannelOptions::}{sslConfiguration})
        \li \c{https://localhost}
    \row
        \li \c{unix}
        \li Unix domain socket in filesystem path
        \li ✗
        \li QLocalSocket support \b{AND} scheme
        \li \c{unix:///tmp/grpc.socket}
    \endtable

    \section2 Content-Type

    The \e{content-type} in \gRPC over HTTP/2 determines the message
    serialization format. It must start with \c{application/grpc} and can
    include a suffix. The format follows this scheme:

    \code
        "content-type": "application/grpc" [("+proto" / "+json" / {custom})]
    \endcode

    For example:
    \list
        \li \c{application/grpc+proto} specifies Protobuf encoding.
        \li \c{application/grpc+json} specifies JSON encoding.
    \endlist

    The serialization format can be configured either by specifying the \c
    {content-type} inside the metadata or by setting the \l{QGrpcChannelOptions::}
    {serializationFormat} directly. By default, the \c {application/grpc}
    content-type is used.

    To configure QGrpcHttp2Channel with the JSON serialization format using
    \c {content-type} metadata:

    \code
        auto jsonChannel = std::make_shared<QGrpcHttp2Channel>(
            QUrl("http://localhost:50051"_L1),
            QGrpcChannelOptions().setMetadata({
                { "content-type"_ba, "application/grpc+json"_ba },
            })
        );
    \endcode

    For a custom serializer and \c {content-type}, you can directly set the
    serialization format:

    \include qgrpcserializationformat.cpp custom-serializer-code

    \code
        auto dummyChannel = std::make_shared<QGrpcHttp2Channel>(
            QUrl("http://localhost:50051"_L1),
            QGrpcChannelOptions().setSerializationFormat(dummyFormat)
        );
    \endcode

    \include qgrpcserializationformat.cpp custom-serializer-desc

    \sa QAbstractGrpcChannel, QGrpcChannelOptions, QGrpcSerializationFormat
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
constexpr QByteArrayView DefaultContentType = "application/grpc";

// This HTTP/2 Error Codes to QGrpcStatus::StatusCode mapping should be kept in sync
// with the following docs:
//     https://www.rfc-editor.org/rfc/rfc7540#section-7
//     https://github.com/grpc/grpc/blob/master/doc/statuscodes.md
//     https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
constexpr StatusCode http2ErrorToStatusCode(const quint32 http2Error)
{
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

// Sends the errorOccured and finished signals asynchronously to make sure user
// connections work correctly.
void operationContextAsyncError(QGrpcOperationContext *operationContext, const QGrpcStatus &status)
{
    Q_ASSERT_X(operationContext != nullptr, "QGrpcHttp2ChannelPrivate::operationContextAsyncError",
               "operationContext is null");
    QTimer::singleShot(0, operationContext,
                       [operationContext, status]() { emit operationContext->finished(status); });
}

} // namespace

struct ExpectedData
{
    qsizetype expectedSize = 0;
    QByteArray container;

    bool updateExpectedSize()
    {
        if (expectedSize == 0) {
            if (container.size() < GrpcMessageSizeHeaderSize)
                return false;
            expectedSize = qFromBigEndian<quint32>(container.data() + 1)
                + GrpcMessageSizeHeaderSize;
        }
        return true;
    }
};

// The Http2Handler manages an individual RPC over the HTTP/2 channel.
// Each instance corresponds to an RPC initiated by the user.
class Http2Handler : public QObject
{
    // Q_OBJECT macro is not needed and adds unwanted overhead.

public:
    enum State : uint8_t { Active, Cancelled, Finished };

    explicit Http2Handler(const std::shared_ptr<QGrpcOperationContext> &operation,
                          QGrpcHttp2ChannelPrivate *parent, bool endStream);
    ~Http2Handler() override;

    void sendInitialRequest();
    void attachStream(QHttp2Stream *stream_);
    void processQueue();

    [[nodiscard]] QGrpcOperationContext *operation() const;
    [[nodiscard]] bool expired() const { return m_operation.expired(); }

    [[nodiscard]] bool isStreamClosedForSending() const
    {
        // If stream pointer is nullptr this means we never opened it and should collect
        // the incoming messages in queue until the stream is opened or the error occurred.
        return m_stream != nullptr
            && (m_stream->state() == QHttp2Stream::State::HalfClosedLocal
                || m_stream->state() == QHttp2Stream::State::Closed);
    }

// context slot handlers:
    bool cancel();
    void writesDone();
    void writeMessage(QByteArrayView data);
    void deadlineTimeout();

private:
    void prepareInitialRequest(QGrpcOperationContext *operationContext,
                               QGrpcHttp2ChannelPrivate *channel);

    HPack::HttpHeader m_initialHeaders;
    std::weak_ptr<QGrpcOperationContext> m_operation;
    QQueue<QByteArray> m_queue;
    QPointer<QHttp2Stream> m_stream;
    ExpectedData m_expectedData;
    State m_handlerState = Active;
    const bool m_endStreamAtFirstData;
    QTimer m_deadlineTimer;

    Q_DISABLE_COPY_MOVE(Http2Handler)
};

class QGrpcHttp2ChannelPrivate : public QObject
{
    Q_OBJECT
public:
    explicit QGrpcHttp2ChannelPrivate(const QUrl &uri, QGrpcHttp2Channel *q);
    ~QGrpcHttp2ChannelPrivate() override = default;

    void processOperation(const std::shared_ptr<QGrpcOperationContext> &operationContext,
                          bool endStream = false);

    void deleteHandler(Http2Handler *handler);
    [[nodiscard]] bool isLocalSocket() const
    {
#if QT_CONFIG(localserver)
        return m_isLocalSocket;
#else
        return false;
#endif
    }
    [[nodiscard]] const QByteArray &contentType() const { return m_contentType; }

    [[nodiscard]] const QByteArray &authorityHeader() const { return m_authorityHeader; }

    std::shared_ptr<QAbstractProtobufSerializer> serializer;
    QUrl hostUri;
    QGrpcHttp2Channel *q_ptr = nullptr;

private:
    enum ConnectionState { Connecting = 0, Connected, Error };

    template <typename T>
    void connectErrorHandler(T *socket, QGrpcOperationContext *operationContext)
    {
        QObject::connect(socket, &T::errorOccurred, operationContext,
                         [operationContextPtr = QPointer(operationContext), this](auto error) {
                             if (m_isInsideSocketErrorOccurred) {
                                 qGrpcCritical("Socket errorOccurred signal triggered while "
                                               "already handling an error");
                                 return;
                             }
                             m_isInsideSocketErrorOccurred = true;
                             auto reset = qScopeGuard([this]() {
                                 m_isInsideSocketErrorOccurred = false;
                             });
                             emit operationContextPtr->finished(QGrpcStatus{
                                 StatusCode::Unavailable,
                                 QGrpcHttp2ChannelPrivate::tr("Network error occurred: %1")
                                     .arg(error) });
                         });
    }

    void sendInitialRequest(Http2Handler *handler);
    void createHttp2Connection();
    void handleSocketError();

    template <typename T>
    T *initSocket()
    {
        auto p = std::make_unique<T>();
        T *typedSocket = p.get();
        m_socket = std::move(p);
        return typedSocket;
    }

    std::unique_ptr<QIODevice> m_socket = nullptr;
    bool m_isInsideSocketErrorOccurred = false;
    QHttp2Connection *m_connection = nullptr;
    QList<Http2Handler *> m_activeHandlers;
    QList<Http2Handler *> m_pendingHandlers;
#if QT_CONFIG(localserver)
    bool m_isLocalSocket = false;
#endif
    QByteArray m_contentType;
    ConnectionState m_state = Connecting;
    std::function<void()> m_reconnectFunction;

    QByteArray m_authorityHeader;
    Q_DISABLE_COPY_MOVE(QGrpcHttp2ChannelPrivate)
};

///
/// ## Http2Handler Implementations
///

Http2Handler::Http2Handler(const std::shared_ptr<QGrpcOperationContext> &operation,
                           QGrpcHttp2ChannelPrivate *parent, bool endStream)
    : QObject(parent), m_operation(operation), m_endStreamAtFirstData(endStream)
{
    auto *channelOpPtr = operation.get();
    QObject::connect(channelOpPtr, &QGrpcOperationContext::cancelRequested, this,
                     &Http2Handler::cancel);
    QObject::connect(channelOpPtr, &QGrpcOperationContext::writesDoneRequested, this,
                     &Http2Handler::writesDone);
    if (!m_endStreamAtFirstData) {
        QObject::connect(channelOpPtr, &QGrpcOperationContext::writeMessageRequested, this,
                         &Http2Handler::writeMessage);
    }
    QObject::connect(channelOpPtr, &QGrpcOperationContext::finished, &m_deadlineTimer,
                     &QTimer::stop);
    prepareInitialRequest(channelOpPtr, parent);
}

Http2Handler::~Http2Handler()
{
    if (m_stream) {
        QHttp2Stream *streamPtr = m_stream.get();
        m_stream.clear();
        delete streamPtr;
    }
}

// Attaches the HTTP/2 stream and sets up the necessary connections and
// preconditions. The HTTP/2 connection is established, and the transport
// is now ready for communication.
void Http2Handler::attachStream(QHttp2Stream *stream_)
{
    Q_ASSERT(m_stream == nullptr);
    Q_ASSERT(stream_ != nullptr);

    auto *channelOpPtr = operation();
    m_stream = stream_;

    auto *parentChannel = qobject_cast<QGrpcHttp2ChannelPrivate *>(parent());
    QObject::connect(m_stream.get(), &QHttp2Stream::headersReceived, channelOpPtr,
                     [channelOpPtr, parentChannel, this](const HPack::HttpHeader &headers,
                                                         bool endStream) {
                         auto md = channelOpPtr->serverMetadata();
                         QtGrpc::StatusCode statusCode = StatusCode::Ok;
                         QString statusMessage;
                         for (const auto &header : headers) {
                             md.insert(header.name, header.value);
                             if (header.name == GrpcStatusHeader) {
                                 statusCode = static_cast<
                                     StatusCode>(QString::fromLatin1(header.value).toShort());
                             } else if (header.name == GrpcStatusMessageHeader) {
                                 statusMessage = QString::fromUtf8(header.value);
                             }
                         }

                         channelOpPtr->setServerMetadata(std::move(md));

                         if (endStream) {
                             if (m_handlerState != Cancelled) {
                                 emit channelOpPtr->finished(
                                     QGrpcStatus{ statusCode,statusMessage });
                             }
                             parentChannel->deleteHandler(this);
                         }
                     });

    Q_ASSERT(parentChannel != nullptr);
    auto errorConnection = std::make_shared<QMetaObject::Connection>();
    *errorConnection = QObject::connect(
        m_stream.get(), &QHttp2Stream::errorOccurred, parentChannel,
        [parentChannel, errorConnection, this](quint32 http2ErrorCode, const QString &errorString) {
            if (!m_operation.expired()) {
                auto channelOp = m_operation.lock();
                emit channelOp->finished(QGrpcStatus{ http2ErrorToStatusCode(http2ErrorCode),
                                                      errorString });
            }
            parentChannel->deleteHandler(this);
            QObject::disconnect(*errorConnection);
        });

    QObject::connect(m_stream.get(), &QHttp2Stream::dataReceived, channelOpPtr,
                     [channelOpPtr, parentChannel, this](const QByteArray &data, bool endStream) {
                         if (m_handlerState != Cancelled) {
                             m_expectedData.container.append(data);

                             if (!m_expectedData.updateExpectedSize())
                                 return;

                             while (m_expectedData.container.size()
                                    >= m_expectedData.expectedSize) {
                                 qGrpcDebug() << "Full data received:" << data.size()
                                              << "dataContainer:" << m_expectedData.container.size()
                                              << "capacity:" << m_expectedData.expectedSize;
                                 emit channelOpPtr
                                     ->messageReceived(m_expectedData.container
                                                           .mid(GrpcMessageSizeHeaderSize,
                                                                m_expectedData.expectedSize
                                                                    - GrpcMessageSizeHeaderSize));
                                 m_expectedData.container.remove(0, m_expectedData.expectedSize);
                                 m_expectedData.expectedSize = 0;
                                 if (!m_expectedData.updateExpectedSize())
                                     return;
                             }
                             if (endStream) {
                                 m_handlerState = Finished;
                                 emit channelOpPtr->finished({});
                                 parentChannel->deleteHandler(this);
                             }
                         }
                     });

    QObject::connect(m_stream.get(), &QHttp2Stream::uploadFinished, this,
                     &Http2Handler::processQueue);

    std::optional<std::chrono::milliseconds> deadline;
    if (channelOpPtr->callOptions().deadlineTimeout())
        deadline = channelOpPtr->callOptions().deadlineTimeout();
    else if (parentChannel->q_ptr->channelOptions().deadlineTimeout())
        deadline = parentChannel->q_ptr->channelOptions().deadlineTimeout();
    if (deadline) {
        // We have an active stream and a deadline. It's time to start the timer.
        QObject::connect(&m_deadlineTimer, &QTimer::timeout, this, &Http2Handler::deadlineTimeout);
        m_deadlineTimer.start(*deadline);
    }
}

QGrpcOperationContext *Http2Handler::operation() const
{
    Q_ASSERT(!m_operation.expired());

    return m_operation.lock().get();
}

// Prepares the initial headers and enqueues the initial message.
// The headers are sent once the HTTP/2 connection is established.
void Http2Handler::prepareInitialRequest(QGrpcOperationContext *operationContext,
                                         QGrpcHttp2ChannelPrivate *channel)
{
    const auto &channelOptions = channel->q_ptr->channelOptions();
    QByteArray service{ operationContext->service().data(), operationContext->service().size() };
    QByteArray method{ operationContext->method().data(), operationContext->method().size() };
    m_initialHeaders = HPack::HttpHeader{
        { AuthorityHeader.toByteArray(),          channel->authorityHeader()               },
        { MethodHeader.toByteArray(),             "POST"_ba                                },
        { PathHeader.toByteArray(),               QByteArray('/' + service + '/' + method) },
        { SchemeHeader.toByteArray(),
         channel->isLocalSocket() ? "http"_ba : channel->hostUri.scheme().toLatin1()       },
        { ContentTypeHeader.toByteArray(),        channel->contentType()                   },
        { GrpcServiceNameHeader.toByteArray(),    { service }                              },
        { GrpcAcceptEncodingHeader.toByteArray(), "identity,deflate,gzip"_ba               },
        { AcceptEncodingHeader.toByteArray(),     "identity,gzip"_ba                       },
        { TEHeader.toByteArray(),                 "trailers"_ba                            },
    };

    auto iterateMetadata = [this](const auto &metadata) {
        for (const auto &[key, value] : metadata.asKeyValueRange()) {
            const auto lowerKey = key.toLower();
            if (lowerKey == AuthorityHeader || lowerKey == MethodHeader || lowerKey == PathHeader
                || lowerKey == SchemeHeader || lowerKey == ContentTypeHeader) {
                continue;
            }
            m_initialHeaders.emplace_back(lowerKey, value);
        }
    };

    iterateMetadata(channelOptions.metadata());
    iterateMetadata(operationContext->callOptions().metadata());

    writeMessage(operationContext->argument());
}

// Slot to enqueue a writeMessage request, either from the initial message
// or from the user in client/bidirectional streaming RPCs.
void Http2Handler::writeMessage(QByteArrayView data)
{
    if (m_handlerState != Active || isStreamClosedForSending()) {
        qGrpcDebug("Attempt sending data to the ended stream");
        return;
    }

    QByteArray msg(GrpcMessageSizeHeaderSize + data.size(), '\0');
    // Args must be 4-byte unsigned int to fit into 4-byte big endian
    qToBigEndian(static_cast<quint32>(data.size()), msg.data() + 1);

    // protect against nullptr data.
    if (!data.isEmpty()) {
        std::memcpy(msg.begin() + GrpcMessageSizeHeaderSize, data.begin(),
                    static_cast<size_t>(data.size()));
    }

    m_queue.enqueue(msg);
    processQueue();
}

// Sends the initial headers and processes the message queue containing the
// initial message. At this point, the HTTP/2 connection is established, and
// the stream is attached.
void Http2Handler::sendInitialRequest()
{
    Q_ASSERT(!m_initialHeaders.empty());
    Q_ASSERT(m_stream);

    if (!m_stream->sendHEADERS(m_initialHeaders, false)) {
        operationContextAsyncError(operation(),
                                   QGrpcStatus{
                                       StatusCode::Unavailable,
                                       tr("Unable to send initial headers to an HTTP/2 stream") });
        return;
    }
    m_initialHeaders.clear();
    processQueue();
}

// The core logic for sending the already serialized data through the HTTP/2 stream.
// This function is invoked either by the user via writeMessageRequested() or
// writesDoneRequested(), or it is continuously polled after the previous uploadFinished()
void Http2Handler::processQueue()
{
    if (!m_stream)
        return;

    if (m_stream->isUploadingDATA())
        return;

    if (m_queue.isEmpty())
        return;

    // Take ownership of the byte device.
    auto *device = QNonContiguousByteDeviceFactory::create(m_queue.dequeue());
    device->setParent(m_stream);

    m_stream->sendDATA(device, device->atEnd() || m_endStreamAtFirstData);
    // Manage the lifetime through the uploadFinished signal (or this
    // Http2Handler). Don't use QObject::deleteLater here as this function is
    // expensive and blocking. Delete the byte device directly.
    //              This is fine in this context.
    connect(m_stream.get(), &QHttp2Stream::uploadFinished, device, [device] { delete device; });
}

bool Http2Handler::cancel()
{
    if (m_handlerState != Active || !m_stream)
        return false;
    m_handlerState = Cancelled;

    // Client cancelled the stream before the deadline exceeded.
    m_deadlineTimer.stop();

    // Immediate cancellation by sending the RST_STREAM frame.
    return m_stream->sendRST_STREAM(Http2::Http2Error::CANCEL);
}

void Http2Handler::writesDone()
{
    if (m_handlerState != Active)
        return;

    m_handlerState = Finished;

    // Stream is already (half)closed, skip sending the DATA frame with the end-of-stream flag.
    if (isStreamClosedForSending())
        return;

    m_queue.enqueue({});
    processQueue();
}

void Http2Handler::deadlineTimeout()
{
    Q_ASSERT_X(m_stream, "onDeadlineTimeout", "stream is not available");

    if (m_operation.expired()) {
        qGrpcWarning("Operation expired on deadline timeout");
        return;
    }
    // cancel the stream by sending the RST_FRAME and report
    // the status back to our user.
    if (cancel()) {
        emit m_operation.lock()->finished({ StatusCode::DeadlineExceeded,
                                            "Deadline Exceeded" });
    } else {
        qGrpcWarning("Cancellation failed on deadline timeout.");
    }
}

///
/// ## QGrpcHttp2ChannelPrivate Implementations
///

QGrpcHttp2ChannelPrivate::QGrpcHttp2ChannelPrivate(const QUrl &uri, QGrpcHttp2Channel *q)
    : hostUri(uri), q_ptr(q)
{
    auto channelOptions = q_ptr->channelOptions();
    auto formatSuffix = channelOptions.serializationFormat().suffix();
    const QByteArray defaultContentType = DefaultContentType.toByteArray();
    const QByteArray contentTypeFromOptions = !formatSuffix.isEmpty()
        ? defaultContentType + '+' + formatSuffix
        : defaultContentType;
    bool warnAboutFormatConflict = !formatSuffix.isEmpty();

    const auto it = channelOptions.metadata().constFind(ContentTypeHeader.data());
    if (it != channelOptions.metadata().cend()) {
        if (formatSuffix.isEmpty() && it.value() != DefaultContentType) {
            if (it.value() == "application/grpc+json") {
                channelOptions.setSerializationFormat(SerializationFormat::Json);
            } else if (it.value() == "application/grpc+proto" || it.value() == DefaultContentType) {
                channelOptions.setSerializationFormat(SerializationFormat::Protobuf);
            } else {
                qGrpcWarning() << "Cannot choose the serializer for " << ContentTypeHeader
                               << it.value() << ". Using protobuf format as the default one.";
                channelOptions.setSerializationFormat(SerializationFormat::Default);
            }
            q_ptr->setChannelOptions(channelOptions);
        } else if (it.value() != contentTypeFromOptions) {
            warnAboutFormatConflict = true;
        } else {
            warnAboutFormatConflict = false;
        }
    } else {
        warnAboutFormatConflict = false;
    }

    if (formatSuffix == channelOptions.serializationFormat().suffix()) { // no change
        m_contentType = contentTypeFromOptions;
    } else { // format has changed, update content type
        m_contentType = !channelOptions.serializationFormat().suffix().isEmpty()
            ? defaultContentType + '+' + channelOptions.serializationFormat().suffix()
            : defaultContentType;
    }

    if (warnAboutFormatConflict) {
        qGrpcWarning()
            << "Manually specified serialization format '%1' doesn't match the %2 header value "
               "'%3'"_L1.arg(QString::fromLatin1(contentTypeFromOptions),
                             QString::fromLatin1(ContentTypeHeader),
                             QString::fromLatin1(it.value()));
    }

    bool nonDefaultPort = false;
#if QT_CONFIG(localserver)
    if (hostUri.scheme() == "unix"_L1) {
        auto *localSocket = initSocket<QLocalSocket>();
        m_isLocalSocket = true;

        QObject::connect(localSocket, &QLocalSocket::connected, this,
                         &QGrpcHttp2ChannelPrivate::createHttp2Connection);
        QObject::connect(localSocket, &QLocalSocket::errorOccurred, this,
                         [this](QLocalSocket::LocalSocketError error) {
                             qGrpcDebug()
                                 << "Error occurred(" << error << "):"
                                 << static_cast<QLocalSocket *>(m_socket.get())->errorString()
                                 << hostUri;
                             handleSocketError();
                         });
        m_reconnectFunction = [localSocket, this] {
            localSocket->connectToServer(hostUri.host() + hostUri.path());
        };
    } else
#endif
#if QT_CONFIG(ssl)
        if (hostUri.scheme() == "https"_L1 || channelOptions.sslConfiguration()) {
        auto *sslSocket = initSocket<QSslSocket>();
        if (hostUri.port() < 0) {
            hostUri.setPort(443);
        } else {
            nonDefaultPort = hostUri.port() != 443;
        }

        if (const auto userSslConfig = channelOptions.sslConfiguration(); userSslConfig) {
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
                         [this](QAbstractSocket::SocketError error) {
                             qDebug()
                                 << "Error occurred(" << error << "):"
                                 << static_cast<QAbstractSocket *>(m_socket.get())->errorString()
                                 << hostUri;
                             handleSocketError();
                         });
        m_reconnectFunction = [sslSocket, this] {
            sslSocket->connectToHostEncrypted(hostUri.host(), static_cast<quint16>(hostUri.port()));
        };
    } else
#endif
    {
        if (hostUri.scheme() != "http"_L1) {
            qGrpcWarning() << "Unsupported transport protocol scheme '" << hostUri.scheme()
                           << "'. Fall back to 'http'.";
        }

        auto *httpSocket = initSocket<QTcpSocket>();
        if (hostUri.port() < 0) {
            hostUri.setPort(80);
        } else {
            nonDefaultPort = hostUri.port() != 80;
        }

        QObject::connect(httpSocket, &QAbstractSocket::connected, this,
                         &QGrpcHttp2ChannelPrivate::createHttp2Connection);
        QObject::connect(httpSocket, &QAbstractSocket::errorOccurred, this,
                         [this](QAbstractSocket::SocketError error) {
                             qGrpcDebug()
                                 << "Error occurred(" << error << "):"
                                 << static_cast<QAbstractSocket *>(m_socket.get())->errorString()
                                 << hostUri;
                             handleSocketError();
                         });
        m_reconnectFunction = [httpSocket, this] {
            httpSocket->connectToHost(hostUri.host(), static_cast<quint16>(hostUri.port()));
        };
    }

    m_authorityHeader = hostUri.host().toLatin1();
    if (nonDefaultPort) {
        m_authorityHeader += ':';
        m_authorityHeader += QByteArray::number(hostUri.port());
    }

    m_reconnectFunction();
}

void QGrpcHttp2ChannelPrivate::processOperation(const std::shared_ptr<QGrpcOperationContext>
                                                    &operationContext,
                                                bool endStream)
{
    auto *operationContextPtr = operationContext.get();
    Q_ASSERT_X(operationContextPtr != nullptr, "QGrpcHttp2ChannelPrivate::processOperation",
               "operation context is nullptr.");

    if (!m_socket->isWritable() && m_state == ConnectionState::Connected) {
        operationContextAsyncError(operationContextPtr,
                                   QGrpcStatus{ StatusCode::Unavailable,
                                                m_socket->errorString() });
        return;
    }

#if QT_CONFIG(localserver)
    if (m_isLocalSocket) {
        connectErrorHandler<QLocalSocket>(static_cast<QLocalSocket *>(m_socket.get()),
                                          operationContextPtr);
    } else
#endif
    {
        connectErrorHandler<QAbstractSocket>(static_cast<QAbstractSocket *>(m_socket.get()),
                                             operationContextPtr);
    }

    auto *handler = new Http2Handler(operationContext, this, endStream);
    if (m_connection == nullptr) {
        m_pendingHandlers.push_back(handler);
    } else {
        sendInitialRequest(handler);
        m_activeHandlers.push_back(handler);
    }

    if (m_state == ConnectionState::Error) {
        Q_ASSERT_X(m_reconnectFunction, "QGrpcHttp2ChannelPrivate::processOperation",
                   "Socket reconnection function is not defined.");
        if (m_isInsideSocketErrorOccurred) {
            qGrpcWarning("Inside socket error handler. Reconnect deferred to event loop.");
            QTimer::singleShot(0, [this]{ m_reconnectFunction(); });
        } else {
            m_reconnectFunction();
        }
        m_state = ConnectionState::Connecting;
    }
}

void QGrpcHttp2ChannelPrivate::createHttp2Connection()
{
    Q_ASSERT_X(m_connection == nullptr, "QGrpcHttp2ChannelPrivate::createHttp2Connection",
               "Attempt to create the HTTP/2 connection, but it already exists. This situation is "
               "exceptional.");

    // Nagle's algorithm slows down gRPC communication when frequently sending small utility
    // HTTP/2 frames. Since an ACK is not sent until a predefined timeout if the TCP frame is
    // not full enough, communication hangs. In our case, this results in a 40ms delay when
    // WINDOW_UPDATE or PING frames are sent in a separate TCP frame.
    //
    // TODO: We should probably allow users to opt out of this using QGrpcChannelOptions,
    // see QTBUG-134428.
    if (QAbstractSocket *abstractSocket = qobject_cast<QAbstractSocket *>(m_socket.get()))
        abstractSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    m_connection = QHttp2Connection::createDirectConnection(m_socket.get(), {});

    if (m_connection) {
        QObject::connect(m_socket.get(), &QAbstractSocket::readyRead, m_connection,
                         &QHttp2Connection::handleReadyRead);
        m_state = ConnectionState::Connected;
    }

    for (const auto &handler : m_pendingHandlers) {
        if (handler->expired()) {
            delete handler;
            continue;
        }
        sendInitialRequest(handler);
    }
    m_activeHandlers.append(m_pendingHandlers);
    m_pendingHandlers.clear();
}

void QGrpcHttp2ChannelPrivate::handleSocketError()
{
    qDeleteAll(m_activeHandlers);
    m_activeHandlers.clear();
    qDeleteAll(m_pendingHandlers);
    m_pendingHandlers.clear();
    delete m_connection;
    m_connection = nullptr;
    m_state = ConnectionState::Error;
}

void QGrpcHttp2ChannelPrivate::sendInitialRequest(Http2Handler *handler)
{
    Q_ASSERT(handler != nullptr);
    auto *channelOpPtr = handler->operation();
    if (!m_connection) {
        operationContextAsyncError(channelOpPtr,
                                   QGrpcStatus{
                                       StatusCode::Unavailable,
                                       tr("Unable to establish an HTTP/2 connection") });
        return;
    }

    const auto streamAttempt = m_connection->createStream();
    if (!streamAttempt.ok()) {
        operationContextAsyncError(channelOpPtr,
                                   QGrpcStatus{
                                       StatusCode::Unavailable,
                                       tr("Unable to create an HTTP/2 stream (%1)")
                                           .arg(QDebug::toString(streamAttempt.error())) });
        return;
    }
    handler->attachStream(streamAttempt.unwrap());
    handler->sendInitialRequest();
}

void QGrpcHttp2ChannelPrivate::deleteHandler(Http2Handler *handler)
{
    const auto it = std::find(m_activeHandlers.constBegin(), m_activeHandlers.constEnd(), handler);
    if (it == m_activeHandlers.constEnd())
        return;
    handler->deleteLater();
    m_activeHandlers.erase(it);
}

///
/// ## QGrpcHttp2Channel Implementations
///

/*!
    Constructs QGrpcHttp2Channel with \a hostUri. Please see the
    \l{Transportation scheme} section for more information.
*/
QGrpcHttp2Channel::QGrpcHttp2Channel(const QUrl &hostUri)
    : d_ptr(std::make_unique<QGrpcHttp2ChannelPrivate>(hostUri, this))
{
}

/*!
    Constructs QGrpcHttp2Channel with \a hostUri and \a options. Please see the
    \l{Transportation scheme} section for more information.
*/
QGrpcHttp2Channel::QGrpcHttp2Channel(const QUrl &hostUri, const QGrpcChannelOptions &options)
    : QAbstractGrpcChannel(options),
      d_ptr(std::make_unique<QGrpcHttp2ChannelPrivate>(hostUri, this))
{
}

/*!
    Destroys the QGrpcHttp2Channel object.
*/
QGrpcHttp2Channel::~QGrpcHttp2Channel() = default;

/*!
    Returns the host URI for this channel.
*/
QUrl QGrpcHttp2Channel::hostUri() const
{
    return d_ptr->hostUri;
}

/*!
    \internal
    Initiates a unary \gRPC call.
*/
void QGrpcHttp2Channel::call(std::shared_ptr<QGrpcOperationContext> operationContext)
{
    d_ptr->processOperation(operationContext, true);
}

/*!
    \internal
    Initiates a server-side \gRPC stream.
*/
void QGrpcHttp2Channel::serverStream(std::shared_ptr<QGrpcOperationContext> operationContext)
{
    d_ptr->processOperation(operationContext, true);
}

/*!
    \internal
    Initiates a client-side \gRPC stream.
*/
void QGrpcHttp2Channel::clientStream(std::shared_ptr<QGrpcOperationContext> operationContext)
{
    d_ptr->processOperation(operationContext);
}

/*!
    \internal
    Initiates a bidirectional \gRPC stream.
*/
void QGrpcHttp2Channel::bidiStream(std::shared_ptr<QGrpcOperationContext> operationContext)
{
    d_ptr->processOperation(operationContext);
}

/*!
    \internal
    Returns the serializer of the channel.
*/
std::shared_ptr<QAbstractProtobufSerializer> QGrpcHttp2Channel::serializer() const
{
    return channelOptions().serializationFormat().serializer();
}

QT_END_NAMESPACE

#include "qgrpchttp2channel.moc"
