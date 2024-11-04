// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qabstractgrpcchannel.h"
#include "qabstractgrpcchannel_p.h"
#include "qgrpccallreply.h"
#include "qgrpcchanneloperation.h"
#include "qgrpcclientinterceptor.h"
#include "qgrpcstream.h"

#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractGrpcChannel
    \inmodule QtGrpc
    \brief The QAbstractGrpcChannel class is an interface that represents common
    gRPC channel functionality.

    Implement this interface to create your own custom channel for gRPC
    transportation. We provide the QGrpcHttp2Channel, which is a fully featured
    implementation of the QAbstractGrpcChannel for HTTP/2 communication.
*/

/*!
    \fn virtual std::shared_ptr<QAbstractProtobufSerializer> QAbstractGrpcChannel::serializer() const = 0

    This pure virtual function shall return a shared pointer
    to QAbstractProtobufSerializer.

    This function is called to obtain the QAbstractProtobufSerializer used
    to perform serialization and deserialization of the message.
*/

/*!
    \fn virtual void QAbstractGrpcChannel::call(std::shared_ptr<QGrpcChannelOperation> channelOperation) = 0
    \since 6.7

    This pure virtual function is called by public QAbstractGrpcChannel::call
    method when making unary gRPC call. The \a channelOperation is the
    pointer to a channel side \l QGrpcChannelOperation primitive that is
    connected with \l QGrpcCallReply primitive, that is used in
    \l QAbstractGrpcClient implementations.

    The function should implement the channel-side logic of unary call. The
    implementation must be asynchronous and must not block the thread where
    the function was called.
*/

/*!
    \fn virtual void QAbstractGrpcChannel::startServerStream(std::shared_ptr<QGrpcChannelOperation> channelOperation) = 0
    \since 6.7

    This pure virtual function that the starts of the server-side stream. The
    \a channelOperation is the pointer to a channel side
    \l QGrpcChannelOperation primitive that is connected with \l QGrpcServerStream
    primitive, that is used in \l QAbstractGrpcClient implementations.

    The function should implement the channel-side logic of server-side stream.
    The implementation must be asynchronous and must not block the thread where
    the function was called.
*/

/*!
    \fn virtual void QAbstractGrpcChannel::startClientStream(std::shared_ptr<QGrpcChannelOperation> channelOperation) = 0
    \since 6.7

    This pure virtual function that the starts of the client-side stream. The
    \a channelOperation is the pointer to a channel side
    \l QGrpcChannelOperation primitive that is connected with
    \l QGrpcClientStream primitive, that is used in \l QAbstractGrpcClient.

    The function should implement the channel-side logic of client-side stream.
    The implementation must be asynchronous and must not block the thread where
    the function was called.
*/

/*!
    \fn virtual void QAbstractGrpcChannel::startBidirStream(std::shared_ptr<QGrpcChannelOperation> channelOperation) = 0
    \since 6.7

    This pure virtual function that the starts of the bidirectional stream. The
    \a channelOperation is the pointer to a channel side
    \l QGrpcChannelOperation primitive that is connected with
    \l QGrpcBidirStream primitive, that is used in \l QAbstractGrpcClient.

    The function should implement the channel-side logic of bidirectional
    stream. The implementation must be asynchronous and must not block the
    thread where the function was called.
*/

static std::optional<std::chrono::milliseconds>
deadlineForCall(const QGrpcChannelOptions &channelOptions, const QGrpcCallOptions &callOptions)
{
    if (callOptions.deadline())
        return *callOptions.deadline();
    if (channelOptions.deadline())
        return *channelOptions.deadline();
    return std::nullopt;
}

QAbstractGrpcChannel::QAbstractGrpcChannel(const QGrpcChannelOptions &options)
    : dPtr(std::make_unique<QAbstractGrpcChannelPrivate>(options))
{
}
QAbstractGrpcChannel::~QAbstractGrpcChannel() = default;

/*!
    Sets the interceptor \a manager for the channel.
*/
void QAbstractGrpcChannel::addInterceptorManager(const QGrpcClientInterceptorManager &manager)
{
    dPtr->interceptorManager = manager;
}

/*!
    \internal
    Returns QGrpcChannelOptions used by the channel.
*/
const QGrpcChannelOptions &QAbstractGrpcChannel::channelOptions() const noexcept
{
    return dPtr->channelOptions;
}

/*!
    \internal
    Function constructs \l QGrpcCallReply and \l QGrpcChannelOperation
    primitives and makes the required for unary gRPC call connections
    between them.

    The function should not be called directly, but only by
    \l QAbstractGrpcClient implementations.
*/
std::shared_ptr<QGrpcCallReply> QAbstractGrpcChannel::call(QLatin1StringView method,
                                                           QLatin1StringView service,
                                                           QByteArrayView arg,
                                                           const QGrpcCallOptions &options)
{
    auto channelOperation = std::make_shared<QGrpcChannelOperation>(method, service, arg, options,
                                                                    serializer());
    auto reply = std::make_shared<QGrpcCallReply>(channelOperation);

    QTimer::singleShot(0, channelOperation.get(), [this, reply, channelOperation]() mutable {
        using Continuation = QGrpcInterceptorContinuation<QGrpcCallReply>;
        Continuation finalCall([this](Continuation::ReplyType response,
                                      Continuation::ParamType operation) {
            QObject::
                connect(operation.get(), &QGrpcChannelOperation::sendData, operation.get(), []() {
                    Q_ASSERT_X(false, "QAbstractGrpcChannel::call",
                               "QAbstractGrpcChannel::call disallows sendData signal from "
                               "QGrpcChannelOperation");
                });

            call(operation);
            return response;
        });
        dPtr->interceptorManager.run(finalCall, reply, channelOperation);
    });

    if (auto deadline = deadlineForCall(dPtr->channelOptions, channelOperation->options()))
        QTimer::singleShot(*deadline, reply.get(), &QGrpcCallReply::cancel);
    return reply;
}

/*!
    \internal
    Function constructs \l QGrpcServerStream and \l QGrpcChannelOperation
    primitives and makes the required for server-side gRPC stream connections
    between them.

    The function should not be called directly, but only by
    \l QAbstractGrpcClient implementations.
*/
std::shared_ptr<QGrpcServerStream>
QAbstractGrpcChannel::startServerStream(QLatin1StringView method, QLatin1StringView service,
                                        QByteArrayView arg, const QGrpcCallOptions &options)
{
    auto channelOperation = std::make_shared<QGrpcChannelOperation>(method, service, arg, options,
                                                                    serializer());
    auto stream = std::make_shared<QGrpcServerStream>(channelOperation);

    QTimer::singleShot(0, channelOperation.get(), [this, stream, channelOperation]() mutable {
        using Continuation = QGrpcInterceptorContinuation<QGrpcServerStream>;
        Continuation finalStream([this](Continuation::ReplyType response,
                                        Continuation::ParamType operation) {
            QObject::connect(operation.get(), &QGrpcChannelOperation::sendData, operation.get(),
                             []() {
                                 Q_ASSERT_X(false, "QAbstractGrpcChannel::startServerStream",
                                            "QAbstractGrpcChannel::startServerStream disallows "
                                            "sendData signal from "
                                            "QGrpcChannelOperation");
                             });
            startServerStream(operation);
            return response;
        });
        dPtr->interceptorManager.run(finalStream, stream, channelOperation);
    });

    if (auto deadline = deadlineForCall(dPtr->channelOptions, channelOperation->options()))
        QTimer::singleShot(*deadline, stream.get(), &QGrpcServerStream::cancel);
    return stream;
}

/*!
    \internal
    Function constructs \l QGrpcClientStream and \l QGrpcChannelOperation
    primitives and makes the required for client-side gRPC stream connections
    between them.

    The function should not be called directly, but only by
    \l QAbstractGrpcClient.
*/
std::shared_ptr<QGrpcClientStream>
QAbstractGrpcChannel::startClientStream(QLatin1StringView method, QLatin1StringView service,
                                        QByteArrayView arg, const QGrpcCallOptions &options)
{
    auto channelOperation = std::make_shared<QGrpcChannelOperation>(method, service, arg, options,
                                                                    serializer());
    auto stream = std::make_shared<QGrpcClientStream>(channelOperation);

    QTimer::singleShot(0, channelOperation.get(), [this, stream, channelOperation]() mutable {
        using Continuation = QGrpcInterceptorContinuation<QGrpcClientStream>;
        Continuation finalStream([this](Continuation::ReplyType response,
                                        Continuation::ParamType operation) {
            startClientStream(operation);
            return response;
        });
        dPtr->interceptorManager.run(finalStream, stream, channelOperation);
    });

    if (auto deadline = deadlineForCall(dPtr->channelOptions, channelOperation->options()))
        QTimer::singleShot(*deadline, stream.get(), &QGrpcClientStream::cancel);
    return stream;
}

/*!
    \internal
    Function constructs \l QGrpcBidirStream and \l QGrpcChannelOperation
    primitives and makes the required for bidirectional gRPC stream connections
    between them.

    The function should not be called directly, but only by
    \l QAbstractGrpcClient.
*/
std::shared_ptr<QGrpcBidirStream>
QAbstractGrpcChannel::startBidirStream(QLatin1StringView method, QLatin1StringView service,
                                       QByteArrayView arg, const QGrpcCallOptions &options)
{
    auto channelOperation = std::make_shared<QGrpcChannelOperation>(method, service, arg, options,
                                                                    serializer());
    auto stream = std::make_shared<QGrpcBidirStream>(channelOperation);

    QTimer::singleShot(0, channelOperation.get(), [this, stream, channelOperation]() mutable {
        using Continuation = QGrpcInterceptorContinuation<QGrpcBidirStream>;
        Continuation finalStream([this](Continuation::ReplyType response,
                                        Continuation::ParamType operation) {
            startBidirStream(operation);
            return response;
        });
        dPtr->interceptorManager.run(finalStream, stream, channelOperation);
    });

    if (auto deadline = deadlineForCall(dPtr->channelOptions, channelOperation->options()))
        QTimer::singleShot(*deadline, stream.get(), &QGrpcBidirStream::cancel);
    return stream;
}

QT_END_NAMESPACE
