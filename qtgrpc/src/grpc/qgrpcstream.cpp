// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QThread>

#include "qgrpcstream.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGrpcStream
    \inmodule QtGrpc

    \brief The QGrpcStream class implements logic to handle stream communication
    in the Grpc channel.
*/

/*!
    \fn void QGrpcStream::messageReceived()

    The signal is emitted when the stream receives an updated value from server.
*/

QGrpcStream::QGrpcStream(QLatin1StringView method, QByteArrayView arg,
                         std::shared_ptr<QAbstractProtobufSerializer> serializer)
    : QGrpcOperation(std::move(serializer)), m_method(method.data(), method.size()),
      m_arg(arg.toByteArray())
{
}

/*!
    Destroys the QGrpcStream object.
*/
QGrpcStream::~QGrpcStream() = default;

/*!
    Cancel this stream and try to abort any call active on any channel
    in the stream.
*/
void QGrpcStream::abort()
{
    if (thread() != QThread::currentThread())
        QMetaObject::invokeMethod(this, &QGrpcStream::finished, Qt::BlockingQueuedConnection);
    else
        emit finished();
}

/*!
    Returns the method for this stream.
*/
QLatin1StringView QGrpcStream::method() const
{
    return QLatin1StringView(m_method);
}

/*!
    Returns serialized arguments for this stream.
*/
QByteArrayView QGrpcStream::arg() const
{
    return m_arg;
}

/*!
    Sets underlying data field with \a data and emits QGrpcStream::messageReceived signal.

    Should be used by QAbstractGrpcChannel implementations,
    to update data in a stream and notify clients about stream updates.
*/
void QGrpcStream::updateData(const QByteArray &data)
{
    setData(QByteArray(data));
    emit messageReceived();
}

QT_END_NAMESPACE

#include "moc_qgrpcstream.cpp"
