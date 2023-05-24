// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtgrpcglobal_p.h>

#include <QtCore/qpointer.h>
#include <QtCore/private/qobject_p.h>

#include "qgrpcoperation.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGrpcOperation
    \inmodule QtGrpc
    \brief The QGrpcOperation class implements common logic to
           handle communication in Grpc channel.
*/

/*!
    \fn template <typename T> T QGrpcOperation::read() const

    Reads message from raw byte array stored in QGrpcCallReply.

    Returns a copy of the deserialized message or, on failure,
    a default-constructed message.
*/

/*!
    \fn void QGrpcOperation::finished()

    This signal indicates the end of communication for this call.

    If signal emitted by stream this means that stream is successfully
    closed either by client or server.
*/

/*!
    \fn void QGrpcOperation::errorOccurred(const QGrpcStatus &status)

    This signal indicates the error occurred during serialization.

    This signal is emitted when error with \a status occurs in channel
    or during serialization.

    \sa QAbstractGrpcClient::errorOccurred
*/

class QGrpcOperationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGrpcOperation)
public:
    QGrpcOperationPrivate(std::shared_ptr<QAbstractProtobufSerializer> _serializer)
        : serializer(std::move(_serializer))
    {
    }

    QByteArray data;
    QGrpcMetadata metadata;
    std::shared_ptr<QAbstractProtobufSerializer> serializer;
};

QGrpcOperation::QGrpcOperation(std::shared_ptr<QAbstractProtobufSerializer> serializer)
    : QObject(*new QGrpcOperationPrivate(std::move(serializer)))
{
}

QGrpcOperation::~QGrpcOperation() = default;

/*!
    Interface for implementation of QAbstractGrpcChannel.

    Should be used to write raw data from channel to reply \a data raw data
    received from channel.
 */
void QGrpcOperation::setData(const QByteArray &data)
{
    Q_D(QGrpcOperation);
    d->data = data;
}

/*!
    Interface for implementation of QAbstractGrpcChannel.

    Should be used to write raw data from channel to reply \a data raw data
    received from channel.
*/
void QGrpcOperation::setData(QByteArray &&data)
{
    Q_D(QGrpcOperation);
    d->data = std::move(data);
}

/*!
    \internal
    Getter of the data received from the channel.
*/
QByteArray QGrpcOperation::data() const
{
    return d_func()->data;
}

/*!
    Interface for implementation of QAbstractGrpcChannel.

    Should be used to write metadata from channel to reply \a metadata received
    from channel. For the HTTP2 channels it usually contains the HTTP
    headers received from the server.
*/
void QGrpcOperation::setMetadata(const QGrpcMetadata &metadata)
{
    Q_D(QGrpcOperation);
    d->metadata = metadata;
}

/*!
    Interface for implementation of QAbstractGrpcChannel.

    Should be used to write metadata from channel to reply \a metadata received
    from channel. For the HTTP2 channels it usually contains the HTTP
    headers received from the server.
*/
void QGrpcOperation::setMetadata(QGrpcMetadata &&metadata)
{
    Q_D(QGrpcOperation);
    d->metadata = std::move(metadata);
}

/*!
    Getter of the metadata received from the channel. For the HTTP2 channels it
    usually contains the HTTP headers received from the server.
*/
QGrpcMetadata QGrpcOperation::metadata() const
{
    return d_func()->metadata;
}

/*!
    \internal
    Getter of the serializer that QGrpcOperation was constructed with.
*/
std::shared_ptr<QAbstractProtobufSerializer> QGrpcOperation::serializer() const
{
    return d_func()->serializer;
}

QT_END_NAMESPACE

#include "moc_qgrpcoperation.cpp"
