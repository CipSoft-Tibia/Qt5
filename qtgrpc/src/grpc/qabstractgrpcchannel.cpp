// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/private/qabstractgrpcchannel_p.h>
#include <QtGrpc/qabstractgrpcchannel.h>
#include <QtGrpc/qgrpccalloptions.h>
#include <QtGrpc/qgrpccallreply.h>
#include <QtGrpc/qgrpcoperationcontext.h>
#include <QtGrpc/qgrpcstream.h>

#include <QtCore/qbytearrayview.h>
#include <QtCore/qlatin1stringview.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractGrpcChannel
    \inmodule QtGrpc
    \brief The QAbstractGrpcChannel class provides an interface for
    implementing the transport layer of \gRPC operations.

    Implement this interface to create a custom channel for \gRPC
    transportation. The QGrpcHttp2Channel class is provided as a fully featured
    implementation of QAbstractGrpcChannel for HTTP/2 communication.

    \sa QGrpcChannelOptions, QGrpcHttp2Channel
*/

/*!
    \fn virtual std::shared_ptr<QAbstractProtobufSerializer> QAbstractGrpcChannel::serializer() const = 0

    This pure virtual function retrieves the QAbstractProtobufSerializer used
    for the serialization and deserialization of messages.
*/

/*!
    \fn virtual void QAbstractGrpcChannel::call(std::shared_ptr<QGrpcOperationContext> operationContext) = 0
    \since 6.7

//! [abstract-rpc-desc]
    This pure virtual function is called when a user starts a new RPC through
    the generated client interface. The \a operationContext should be used to
    communicate with the corresponding RPC handler, which is a derived type of
    the QGrpcOperation object.

    This function should start the corresponding RPC on the channel side. The
    implementation must be asynchronous and must not block the calling thread.

    \note It is the channel's responsibility to support and restrict the subset
    of features that its RPC type allows.
//! [abstract-rpc-desc]
*/

/*!
    \fn virtual void QAbstractGrpcChannel::serverStream(std::shared_ptr<QGrpcOperationContext> operationContext) = 0
    \since 6.7

    \include qabstractgrpcchannel.cpp abstract-rpc-desc
*/

/*!
    \fn virtual void QAbstractGrpcChannel::clientStream(std::shared_ptr<QGrpcOperationContext> operationContext) = 0
    \since 6.7

    \include qabstractgrpcchannel.cpp abstract-rpc-desc
*/

/*!
    \fn virtual void QAbstractGrpcChannel::bidiStream(std::shared_ptr<QGrpcOperationContext> operationContext) = 0
    \since 6.7

    \include qabstractgrpcchannel.cpp abstract-rpc-desc
*/

/*!
    Default-constructs the QAbstractGrpcChannel.
*/
QAbstractGrpcChannel::QAbstractGrpcChannel()
    : d_ptr(std::make_unique<QAbstractGrpcChannelPrivate>(QGrpcChannelOptions{}))
{
}

/*!
    \since 6.8
    \internal

    Constructs the QAbstractGrpcChannel using the Private implementation to
    reuse the d_ptr.
*/
QAbstractGrpcChannel::QAbstractGrpcChannel(QAbstractGrpcChannelPrivate &dd) : d_ptr(&dd)
{
}

/*!
    Constructs the QAbstractGrpcChannel using the specified \a options.
*/
QAbstractGrpcChannel::QAbstractGrpcChannel(const QGrpcChannelOptions &options)
    : d_ptr(std::make_unique<QAbstractGrpcChannelPrivate>(options))
{
}

/*!
    Destroys the QAbstractGrpcChannel.
*/
QAbstractGrpcChannel::~QAbstractGrpcChannel() = default;

/*!
    Returns the options utilized by the channel.

    \sa setChannelOptions
*/
const QGrpcChannelOptions &QAbstractGrpcChannel::channelOptions() const & noexcept
{
    Q_D(const QAbstractGrpcChannel);
    return d->channelOptions;
}

/*!
    \fn void QAbstractGrpcChannel::setChannelOptions(const QGrpcChannelOptions &options) noexcept
    \fn void QAbstractGrpcChannel::setChannelOptions(QGrpcChannelOptions &&options) noexcept
    \since 6.8

    Sets the channel \a options.

    \note The updated channel options do not affect currently active calls or streams.
    The revised options will apply only to new RPCs made through this channel.

    \sa channelOptions
*/
void QAbstractGrpcChannel::setChannelOptions(const QGrpcChannelOptions &options)
{
    Q_D(QAbstractGrpcChannel);
    d->channelOptions = options;
}

void QAbstractGrpcChannel::setChannelOptions(QGrpcChannelOptions &&options)
{
    Q_D(QAbstractGrpcChannel);
    d->channelOptions = std::move(options);
}

/*!
    \internal

//! [private-rpc-desc]
    This function is called when a user initiates a new RPC through the
    generated client interface via QGrpcClientBase. It creates the
    QGrpcOperationContext and the corresponding RPC handler, establishing the
    required connections between the two.
//! [private-rpc-desc]
*/
std::unique_ptr<QGrpcCallReply> QAbstractGrpcChannel::call(QLatin1StringView method,
                                                           QLatin1StringView service,
                                                           QByteArrayView arg,
                                                           const QGrpcCallOptions &options)
{
    auto operationContext = std::make_shared<
        QGrpcOperationContext>(method, service, arg, options, serializer(),
                               QGrpcOperationContext::PrivateConstructor());

    QObject::connect(operationContext.get(), &QGrpcOperationContext::writeMessageRequested,
                     operationContext.get(), []() {
                         Q_ASSERT_X(false, "QAbstractGrpcChannel::call",
                                    "QAbstractGrpcChannel::call disallows the "
                                    "'writeMessageRequested' signal from "
                                    "QGrpcOperationContext");
                     });

    auto reply = std::make_unique<QGrpcCallReply>(operationContext);
    call(operationContext);

    return reply;
}

/*!
    \internal
    \include qabstractgrpcchannel.cpp private-rpc-desc
*/
std::unique_ptr<QGrpcServerStream>
QAbstractGrpcChannel::serverStream(QLatin1StringView method, QLatin1StringView service,
                                   QByteArrayView arg, const QGrpcCallOptions &options)
{
    auto operationContext = std::make_shared<
        QGrpcOperationContext>(method, service, arg, options, serializer(),
                               QGrpcOperationContext::PrivateConstructor());

    QObject::connect(operationContext.get(), &QGrpcOperationContext::writeMessageRequested,
                     operationContext.get(), []() {
                         Q_ASSERT_X(false, "QAbstractGrpcChannel::serverStream",
                                    "QAbstractGrpcChannel::serverStream disallows "
                                    "the 'writeMessageRequested' signal from "
                                    "QGrpcOperationContext");
                     });

    auto stream = std::make_unique<QGrpcServerStream>(operationContext);
    serverStream(operationContext);

    return stream;
}

/*!
    \internal
    \include qabstractgrpcchannel.cpp private-rpc-desc
*/
std::unique_ptr<QGrpcClientStream>
QAbstractGrpcChannel::clientStream(QLatin1StringView method, QLatin1StringView service,
                                   QByteArrayView arg, const QGrpcCallOptions &options)
{
    auto operationContext = std::make_shared<
        QGrpcOperationContext>(method, service, arg, options, serializer(),
                               QGrpcOperationContext::PrivateConstructor());

    auto stream = std::make_unique<QGrpcClientStream>(operationContext);
    clientStream(operationContext);

    return stream;
}

/*!
    \internal
    \include qabstractgrpcchannel.cpp private-rpc-desc
*/
std::unique_ptr<QGrpcBidiStream> QAbstractGrpcChannel::bidiStream(QLatin1StringView method,
                                                                  QLatin1StringView service,
                                                                  QByteArrayView arg,
                                                                  const QGrpcCallOptions &options)
{
    auto operationContext = std::make_shared<
        QGrpcOperationContext>(method, service, arg, options, serializer(),
                               QGrpcOperationContext::PrivateConstructor());

    auto stream = std::make_unique<QGrpcBidiStream>(operationContext);
    bidiStream(operationContext);

    return stream;
}

QT_END_NAMESPACE
