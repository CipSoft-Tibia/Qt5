// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qgrpcoperation.h"

#include "qtgrpcglobal_p.h"
#include "qgrpcchanneloperation.h"

#include <QtCore/qatomic.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/qpointer.h>
#include <QtCore/qeventloop.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcOperation
    \inmodule QtGrpc
    \brief The QGrpcOperation class implements common logic to
           handle the gRPC communication from the client side.
*/

/*!
    \fn template <typename T> T QGrpcOperation::read() const

    Reads message from raw byte array stored in QGrpcOperation.

    Returns a deserialized message or, on failure, a default-constructed
    message.
    If deserialization is not successful the \l QGrpcOperation::errorOccurred
    signal is emitted.
*/

/*!
    \fn void QGrpcOperation::finished()

    This signal indicates the end of communication for this call.

    If this signal is emitted by the stream then this stream is successfully
    closed either by client or server.
*/

/*!
    \fn void QGrpcOperation::errorOccurred(const QGrpcStatus &status) const

    This signal indicates the error occurred during serialization.

    This signal is emitted when error with \a status occurs in channel
    or during serialization.

    \sa QAbstractGrpcClient::errorOccurred
*/

class QGrpcOperationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGrpcOperation)
public:
    QGrpcOperationPrivate(std::shared_ptr<QGrpcChannelOperation> _channelOperation)
        : channelOperation(std::move(_channelOperation))
    {
    }

    QByteArray data;
    std::shared_ptr<QGrpcChannelOperation> channelOperation;
    QAtomicInteger<bool> isFinished{ false };
};

QGrpcOperation::QGrpcOperation(std::shared_ptr<QGrpcChannelOperation> channelOperation)
    : QObject(*new QGrpcOperationPrivate(std::move(channelOperation)))
{
    [[maybe_unused]] bool valid =
            QObject::connect(d_func()->channelOperation.get(), &QGrpcChannelOperation::dataReady,
                             this, [this](const QByteArray &data) {
                                 Q_D(QGrpcOperation);
                                 d->data = data;
                             });
    Q_ASSERT_X(valid, "QGrpcOperation::QGrpcOperation",
               "Unable to make connection to the 'dataReady' signal");

    valid = QObject::connect(d_func()->channelOperation.get(),
                             &QGrpcChannelOperation::errorOccurred, this,
                             [this](const auto &status) {
                                 d_func()->isFinished.storeRelaxed(true);
                                 emit this->errorOccurred(status);
                             });
    Q_ASSERT_X(valid, "QGrpcOperation::QGrpcOperation",
               "Unable to make connection to the 'errorOccurred' signal");

    valid = QObject::connect(d_func()->channelOperation.get(), &QGrpcChannelOperation::finished,
                             this, [this]() {
                                 d_func()->isFinished.storeRelaxed(true);
                                 emit this->finished();
                             });
    Q_ASSERT_X(valid, "QGrpcOperation::QGrpcOperation",
               "Unable to make connection to the 'finished' signal");
}

QGrpcOperation::~QGrpcOperation() = default;

/*!
    \internal
    Getter of the data received from the channel.
*/
QByteArray QGrpcOperation::data() const noexcept
{
    return d_func()->data;
}

/*!
    Getter of the metadata received from the channel. For the HTTP2 channels it
    usually contains the HTTP headers received from the server.
*/
QGrpcMetadata QGrpcOperation::metadata() const noexcept
{
    return d_func()->channelOperation->serverMetadata();
}

/*!
    Getter of the method that this operation was intialized with.
*/
QLatin1StringView QGrpcOperation::method() const noexcept
{
    return d_func()->channelOperation->method();
}

/*!
    \internal
    Returns a pointer to the assigned channel-side QGrpcChannelOperation.
*/
const QGrpcChannelOperation *QGrpcOperation::channelOperation() const noexcept
{
    return d_func()->channelOperation.get();
}

/*!
    \internal
    Getter of the serializer that QGrpcOperation was constructed with.
*/
std::shared_ptr<const QAbstractProtobufSerializer> QGrpcOperation::serializer() const noexcept
{
    return d_func()->channelOperation->serializer();
}

/*!
    Attempts to cancel the operation in a channel and immediately emits
    \l{QGrpcOperation::errorOccurred} with the \l{QGrpcStatus::Cancelled}
    status code.

    Any manipulation of the operation after this call has no effect.
*/
void QGrpcOperation::cancel()
{
    d_func()->isFinished.storeRelaxed(true);
    emit d_func()->channelOperation->cancelled();
    emit errorOccurred({ QGrpcStatus::Cancelled, "Operation is cancelled by client"_L1 });
}

/*!
    Returns true when QGrpcOperation finished its workflow,
    meaning it was finished, canceled, or error occurred, otherwise returns false.
*/
bool QGrpcOperation::isFinished() const noexcept
{
    return d_func()->isFinished.loadRelaxed();
}

QGrpcStatus QGrpcOperation::deserializationError() const
{
    QGrpcStatus status;
    switch (serializer()->deserializationError()) {
    case QAbstractProtobufSerializer::InvalidHeaderError: {
        const QLatin1StringView errStr("Response deserialization failed: invalid field found.");
        status = { QGrpcStatus::InvalidArgument, errStr };
        qGrpcWarning() << errStr;
        emit errorOccurred(status);
    } break;
    case QAbstractProtobufSerializer::NoDeserializerError: {
        const QLatin1StringView errStr("No deserializer was found for a given type.");
        status = { QGrpcStatus::InvalidArgument, errStr };
        qGrpcWarning() << errStr;
        emit errorOccurred(status);
    } break;
    case QAbstractProtobufSerializer::UnexpectedEndOfStreamError: {
        const QLatin1StringView errStr("Invalid size of received buffer.");
        status = { QGrpcStatus::OutOfRange, errStr };
        qGrpcWarning() << errStr;
        emit errorOccurred(status);
    } break;
    case QAbstractProtobufSerializer::NoError:
        Q_FALLTHROUGH();
    default:
        const QLatin1StringView errStr("Deserializing failed, but no error was set.");
        status = { QGrpcStatus::InvalidArgument, errStr };
        qGrpcWarning() << errStr;
        emit errorOccurred(status);
    }
    return status;
}

QT_END_NAMESPACE

#include "moc_qgrpcoperation.cpp"
