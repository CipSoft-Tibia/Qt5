// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/private/qtgrpclogging_p.h>
#include <QtGrpc/qgrpcoperation.h>
#include <QtGrpc/qgrpcoperationcontext.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcOperation
    \inmodule QtGrpc
    \brief The QGrpcOperation class provides common operations to handle the
    \gRPC communication from the client side.

    QGrpcOperation serves as the base class for the four \gRPC method types:
    QGrpcCallReply (unary calls), QGrpcServerStream (server streaming),
    QGrpcClientStream (client streaming), and QGrpcBidiStream (bidirectional
    streaming). It provides a common interface for interacting with these
    remote procedure calls (RPCs).

    Each QGrpcOperation corresponds to a specific RPC requested through the
    generated client interface.

    For a high-level overview, refer to the \l {clientguide} {Qt GRPC
    Client Guide}.
*/

/*!
    \fn void QGrpcOperation::finished(const QGrpcStatus &status)

    This signal is emitted when a previously started RPC has finished. The \a
    status provides additional information about the outcome of the RPC.

    After this signal is received, no further write or read operations should
    be performed on the operation object. At this point, it is safe to reuse or
    destroy the RPC object.

    \note This signal is emitted only once, and in most cases, you will want to
    disconnect right after receiving it to avoid issues, such as lambda
    captures not being destroyed after receiving the signal. An easy way to
    achieve this is by using the \l {Qt::} {SingleShotConnection} connection
    type. See \l {Single Shot RPCs} for further details.
*/

class QGrpcOperationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGrpcOperation)
public:
    explicit QGrpcOperationPrivate(std::shared_ptr<QGrpcOperationContext> &&operationContext_)
        : operationContext(operationContext_)
    {
    }

    QByteArray data;
    std::shared_ptr<QGrpcOperationContext> operationContext;
    QAtomicInteger<bool> isFinished{ false };
};

/*!
    \internal

    Constructs a QGrpcOperation using \a operationContext to communicate
    with the underlying channel and sets \a parent as the owner.
*/
QGrpcOperation::QGrpcOperation(std::shared_ptr<QGrpcOperationContext> operationContext,
                               QObject *parent)
    : QObject(*new QGrpcOperationPrivate(std::move(operationContext)), parent)
{
    Q_D(QGrpcOperation);
    [[maybe_unused]] bool valid = QObject::connect(d->operationContext.get(),
                                                   &QGrpcOperationContext::messageReceived, this,
                                                   [this](const QByteArray &data) {
                                                       Q_D(QGrpcOperation);
                                                       d->data = data;
                                                   });
    Q_ASSERT_X(valid, "QGrpcOperation::QGrpcOperation",
               "Unable to make connection to the 'messageReceived' signal");

    valid = QObject::connect(d->operationContext.get(), &QGrpcOperationContext::finished, this,
                             [this](const QGrpcStatus &status) {
                                 if (!isFinished()) {
                                     Q_D(QGrpcOperation);
                                     d->isFinished.storeRelaxed(true);
                                     emit this->finished(status);
                                 }
                             });
    Q_ASSERT_X(valid, "QGrpcOperation::QGrpcOperation",
               "Unable to make connection to the 'finished' signal");
}

/*!
    Destroys the QGrpcOperation.
*/
QGrpcOperation::~QGrpcOperation() = default;

/*!
    \fn template <typename T, QtProtobuf::if_protobuf_message<T> = true> std::optional<T> QGrpcOperation::read() const

    Reads a message from a raw byte array stored within this operation object.

    Returns an optional deserialized message. On failure, \c {std::nullopt} is
    returned.

    \note This function only participates in overload resolution if \c T is a
    subclass of QProtobufMessage.

    \sa read(QProtobufMessage *)
*/

/*!
    \since 6.8

    Reads a message from a raw byte array stored within this operation object.

    The function writes the deserialized value to the \a message pointer.

    If the deserialization is successful, this function returns \c true.
    Otherwise, it returns \c false.

    \sa read()
*/
bool QGrpcOperation::read(QProtobufMessage *message) const
{
    Q_ASSERT_X(message != nullptr, "QGrpcOperation::read",
               "Can't read to nullptr QProtobufMessage");
    Q_D(const QGrpcOperation);
    const auto ser = d->operationContext->serializer();
    Q_ASSERT_X(ser, "QGrpcOperation", "The serializer is null");
    if (!ser->deserialize(message, d->data)) {
        qGrpcWarning() << "Unable to deserialize message(" << qToUnderlying(ser->lastError()) <<"): "
                       << ser->lastErrorString();
        return false;
    }
    return true;
}

/*!
    Tries to cancel the RPC immediately. Successful cancellation cannot be
    guaranteed. Emits the \l finished signal with a \l {QtGrpc::StatusCode::}
    {Cancelled} status code.

    \sa QGrpcOperationContext::cancelRequested
*/
void QGrpcOperation::cancel()
{
    if (!isFinished()) {
        Q_D(QGrpcOperation);
        d->isFinished.storeRelaxed(true);
        emit d->operationContext->cancelRequested();
        Q_EMIT finished(QGrpcStatus{ QtGrpc::StatusCode::Cancelled,
                                     tr("Operation is cancelled by client") });
    }
}

/*!
    Returns the server metadata received from the channel.

    \note For \l {QGrpcHttp2Channel} {HTTP/2 channels} it usually includes the
    HTTP headers received from the server.
*/
const QHash<QByteArray, QByteArray> &QGrpcOperation::metadata() const & noexcept
{
    Q_D(const QGrpcOperation);
    return d->operationContext->serverMetadata();
}

/*!
    Returns the method name associated with this RPC operation.
*/
QLatin1StringView QGrpcOperation::method() const noexcept
{
    Q_D(const QGrpcOperation);
    return d->operationContext->method();
}

/*!
    Returns true if this operation has finished, meaning that no more
    operations can happen on the corresponding RPC, otherwise returns false.
*/
bool QGrpcOperation::isFinished() const noexcept
{
    Q_D(const QGrpcOperation);
    return d->isFinished.loadRelaxed();
}

/*!
    \internal
    \fn const QGrpcOperationContext &QGrpcOperation::context() const &
    \fn QGrpcOperationContext &QGrpcOperation::context() &

    Returns a reference to the internal operation context.
*/
const QGrpcOperationContext &QGrpcOperation::context() const & noexcept
{
    Q_D(const QGrpcOperation);
    return *d->operationContext;
}

bool QGrpcOperation::event(QEvent *event)
{
    return QObject::event(event);
}

QT_END_NAMESPACE

#include "moc_qgrpcoperation.cpp"
