// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/qgrpccalloptions.h>
#include <QtGrpc/qgrpcoperationcontext.h>
#include <QtGrpc/qgrpcstatus.h>

#include <QtProtobuf/qprotobufserializer.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qlatin1stringview.h>

#include <utility>

QT_BEGIN_NAMESPACE

/*!
    \class QGrpcOperationContext
    \inmodule QtGrpc
    \since 6.7
    \brief The QGrpcOperationContext class provides context for communication
    between an individual \gRPC operation and a channel.

    QGrpcOperationContext is constructed internally when a remote procedure call
    (RPC) is requested, mediating interaction between the client's operation
    request and the channel's operation outcome.

    RPCs requested on the client interface return specializations of
    QGrpcOperation, such as QGrpcCallReply, QGrpcServerStream,
    QGrpcClientStream, or QGrpcBidiStream. These classes are implicitly
    integrated with the underlying QGrpcOperationContext counterpart.

    \note Don't use this class directly unless implementing a custom channel.

    The signals contained within this class are used to build the communication
    between the client-facing QGrpcOperation and the QAbstractGrpcChannel
    implementation. These operations are asynchronous in nature, and multiple
    RPCs can operate on the same channel concurrently. Channels typically treat
    all operations the same, and it is the channel's responsibility to
    correctly support and restrict the subset of features its RPC type
    supports.

    Signals which should only be emitted by the channel's implementation are:
    \list
        \li finished()
        \li messageReceived()
    \endlist

    Signals which will be emitted by QGrpcOperation and its specializations are:
    \list
        \li cancelRequested()
        \li writeMessageRequested()
        \li writesDoneRequested()
    \endlist

    Custom implementations of QAbstractGrpcChannel must handle the client's
    signals, as no default signal handling is provided, and emit their own
    signals accordingly.
*/

/*!
    \fn void finished(const QGrpcStatus &status)

    This signal should be emitted by the channel when an RPC finishes.

    It usually means that the server sent a \a status for the requested RPC and
    closed the connection. Implementations of QAbstractGrpcChannel should
    detect this situation and emit the signal.

    After receiving this signal, no further communication should occur through
    this operation context. The client side may then safely delete the
    corresponding RPC object.

    \note This signal is implicitly connected to the QGrpcOperation counterpart.

    \sa QGrpcOperation::finished
*/

/*!
    \fn void QGrpcOperationContext::messageReceived(const QByteArray &data)

    This signal should be emitted by the channel when a new chunk of \a data is
    received from the server.

    For client streams and unary calls, this means that the single and final
    response has arrived and communication is about to finish.

    For server and bidirectional streams, this signal should be continuously
    emitted by the channel upon receiving each new messages.

    \note This signal is implicitly connected to the QGrpcOperation
    counterpart.

    \sa QGrpcServerStream::messageReceived
    \sa QGrpcBidiStream::messageReceived
*/

/*!
    \fn void QGrpcOperationContext::cancelRequested()

    This signal is emitted by QGrpcOperation when requesting cancellation of the
    communication.

    The channel is expected to connect its cancellation logic to this signal and
    attempt to cancel the RPC and return immediately. Successful cancellation
    cannot be guaranteed. Further processing of the data received from a
    channel is not required and should be avoided.

    \sa QGrpcOperation::cancel
*/

/*!
    \fn void QGrpcOperationContext::writeMessageRequested(const QByteArray &data)

    This signal is emitted by QGrpcClientStream or QGrpcBidiStream when it has
    serialized \a data ready for a channel to deliver.

    The channel is expected to connect its writing logic to this signal and wrap
    the serialized data in channel-related headers before writing it to the
    wire.

    \sa QGrpcClientStream::writeMessage
    \sa QGrpcBidiStream::writeMessage
*/

/*!
    \fn void QGrpcOperationContext::writesDoneRequested()

    This signal is emitted by QGrpcClientStream or QGrpcBidiStream to indicate
    that it's done writing messages. The channel should respond to this by
    half-closing the stream.

    \note After receiving this signal no more write operations are permitted
    for the streaming RPCs. The server can still send messages, which should be
    forwarded with messageReceived().

    \sa QGrpcClientStream::writesDone
    \sa QGrpcBidiStream::writesDone
*/

class QGrpcOperationContextPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGrpcOperationContext)
public:
    QGrpcOperationContextPrivate(QLatin1StringView method_, QLatin1StringView service_,
                                 QByteArrayView argument_, QGrpcCallOptions options_,
                                 std::shared_ptr<QAbstractProtobufSerializer> &&serializer_)
        : method(method_), service(service_), argument(argument_.toByteArray()),
          options(std::move(options_)), serializer(std::move(serializer_))
    {
    }

    QLatin1StringView method;
    QLatin1StringView service;
    QByteArray argument;
    QGrpcCallOptions options;
    std::shared_ptr<QAbstractProtobufSerializer> serializer;
    QHash<QByteArray, QByteArray> serverMetadata;
};

/*!
    \internal

    Constructs an operation context with \a method and \a service name. The
    initial serialized message \a arg is used to start a call with the \a
    options and the selected \a serializer used for the RPC.

    \note This class can only be constructed by QAbstractGrpcChannel.
*/
QGrpcOperationContext::QGrpcOperationContext(QLatin1StringView method, QLatin1StringView service,
                                             QByteArrayView arg, const QGrpcCallOptions &options,
                                             std::shared_ptr<QAbstractProtobufSerializer>
                                                 serializer,
                                             PrivateConstructor /*unused*/)
    : QObject(*new QGrpcOperationContextPrivate(method, service, arg, options,
                                                std::move(serializer)))
{
}

/*!
    Destroys the operation-context.
*/
QGrpcOperationContext::~QGrpcOperationContext() = default;

/*!
    Returns the method name of the service associated with this
    operation-context.
*/
QLatin1StringView QGrpcOperationContext::method() const noexcept
{
    Q_D(const QGrpcOperationContext);
    return d->method;
}

/*!
    Returns the service name associated with this operation-context.
*/
QLatin1StringView QGrpcOperationContext::service() const noexcept
{
    Q_D(const QGrpcOperationContext);
    return d->service;
}

/*!
    Returns the serialized argument that is utilized by this operation-context.
*/
QByteArrayView QGrpcOperationContext::argument() const noexcept
{
    Q_D(const QGrpcOperationContext);
    return d->argument;
}

/*!
    Returns the call options that is utilized by this operation-context.

    For channel-wide options, see QGrpcChannelOptions.
*/
const QGrpcCallOptions &QGrpcOperationContext::callOptions() const & noexcept
{
    Q_D(const QGrpcOperationContext);
    return d->options;
}

/*!
    Returns the serializer of this operation-context
*/
std::shared_ptr<const QAbstractProtobufSerializer>
QGrpcOperationContext::serializer() const
{
    Q_D(const QGrpcOperationContext);
    return d->serializer;
}

/*!
    Returns the metadata received from the server.

    \note This method is used implicitly by the QGrpcOperation counterpart.
*/
const QHash<QByteArray, QByteArray> &QGrpcOperationContext::serverMetadata() const & noexcept
{
    Q_D(const QGrpcOperationContext);
    return d->serverMetadata;
}

/*!
    \fn void QGrpcOperationContext::setServerMetadata(const QHash<QByteArray, QByteArray> &metadata)
    \fn void QGrpcOperationContext::setServerMetadata(QHash<QByteArray, QByteArray> &&metadata)

    Sets the server \a metadata received from the service.
*/
void QGrpcOperationContext::setServerMetadata(const QHash<QByteArray, QByteArray> &metadata)
{
    Q_D(QGrpcOperationContext);
    d->serverMetadata = metadata;
}

void QGrpcOperationContext::setServerMetadata(QHash<QByteArray, QByteArray> &&metadata)
{
    Q_D(QGrpcOperationContext);
    d->serverMetadata = std::move(metadata);
}

// For future extensions
bool QGrpcOperationContext::event(QEvent *event)
{
    return QObject::event(event);
}

QT_END_NAMESPACE

#include "moc_qgrpcoperationcontext.cpp"
