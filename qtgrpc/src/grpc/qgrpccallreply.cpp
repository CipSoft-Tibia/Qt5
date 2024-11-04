// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qgrpccallreply.h"
#include "qgrpcchanneloperation.h"

#include <QtCore/qthread.h>
#include <QtCore/qeventloop.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcCallReply
    \inmodule QtGrpc

    \brief The QGrpcCallReply class implements logic to handle gRPC calls
    from the gRPC client side.

    The QGrpcCallReply object is owned by the client object that created it.
*/

/*!
    \fn template <typename Func1, typename Func2> void QGrpcCallReply::subscribe(QObject *receiver,
    Func1 &&finishCallback, Func2 &&errorCallback, Qt::ConnectionType type = Qt::AutoConnection);

    Convenience function to connect the \a finishCallback and
    \a errorCallback of \a receiver to the QGrpcCallReply::finished and
    the QGrpcCallReply::errorOccurred signals with the given connection \a type.

    Calling this function is equivalent to the following:
    \code
        QObject::connect(this, &QGrpcCallReply::finished, receiver,
                         std::forward<Func1>(finishCallback), type);
        QObject::connect(this, &QGrpcCallReply::errorOccurred, receiver,
                         std::forward<Func2>(errorCallback), type);
    \endcode
*/

/*!
    \fn template <typename Func1> void QGrpcCallReply::subscribe(QObject *receiver,
    Func1 &&finishCallback, Qt::ConnectionType type = Qt::AutoConnection);

    Convenience function to connect the \a finishCallback of \a receiver to
    the QGrpcCallReply::finished signal with given connection \a type.

    Calling this function is equivalent to the following:
    \code
        QObject::connect(this, &QGrpcCallReply::finished, receiver,
                         std::forward<Func1>(finishCallback), type);
    \endcode
*/

QGrpcCallReply::QGrpcCallReply(std::shared_ptr<QGrpcChannelOperation> channelOperation)
    : QGrpcOperation(std::move(channelOperation))
{
}

QGrpcCallReply::~QGrpcCallReply() = default;

QT_END_NAMESPACE

#include "moc_qgrpccallreply.cpp"
