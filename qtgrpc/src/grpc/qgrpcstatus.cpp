// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qgrpcstatus.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGrpcStatus
    \inmodule QtGrpc

    \brief The QGrpcStatus class contains information about last gRPC operation.

    In case of error in call/stream processing QGrpcStatus will contain code
    any of non-Ok QGrpcStatus::StatusCode.
    This class combines QGrpcStatus::StatusCode and message returned from
    channel or QGrpc framework.
*/

/*!
    \enum QGrpcStatus::StatusCode

    \brief Channel's status codes.

    \value Ok No error
    \value Cancelled The operation was cancelled, typically by the caller.
    \omitvalue Unknown
    \value InvalidArgument The client specified an invalid argument,
    \value DeadlineExceeded The deadline expired before the operation
    could complete,
    \value NotFound Some requested entity (e.g., file or directory) was
    not found.
    \value AlreadyExists The entity that a client attempted to create
    (e.g., file or directory) already exists.
    \value PermissionDenied  The caller does not have permission to execute
    the specified operation.
    \c PermissionDenied must not be used for rejections caused by exhausting
    some resource (use \c ResourceExhausted instead for those errors).
    \c PermissionDenied must not be used if the caller can not be identified
    (use \c Unauthenticated instead for those errors).
    This error code does not imply the request is valid or the requested
    entity exists or satisfies other pre-conditions.
    \value ResourceExhausted Some resource has been exhausted, perhaps
    a per-user quota, or perhaps the entire file system is out of space.
    \value FailedPrecondition The operation was rejected because the system
    is not in a state required for the operation's execution.
    \value Aborted The operation was aborted, typically due to
    a concurrency issue such as a sequencer check failure or transaction abort.
    \value OutOfRange The operation was attempted past the valid range.
    \value Unimplemented The operation is not implemented or is
    not supported/enabled in this service.
    \value Internal This means that some invariants expected by
    the underlying system have been broken.
    \value Unavailable The service is currently unavailable.
    This is most likely a transient condition, which can be corrected
    by retrying with a backoff. Note that it is not always safe
    to retry non-idempotent operations.
    \value DataLoss Unrecoverable data loss or corruption.
    \value Unauthenticated The request does not have valid authentication
    credentials for the operation.

    \sa{https://github.com/grpc/grpc/blob/master/doc/statuscodes.md}{gRPC status codes}
*/

/*!
    \fn bool QGrpcStatus::operator==(const QGrpcStatus &lhs, QGrpcStatus::StatusCode code)
    Returns \c true if \a lhs status code and \a code are equal.
*/

/*!
    \fn bool QGrpcStatus::operator!=(const QGrpcStatus &lhs, QGrpcStatus::StatusCode code)
    Returns \c true if \a lhs status code and \a code are not equal.
*/

/*!
    \fn bool QGrpcStatus::operator==(const QGrpcStatus &lhs, const QGrpcStatus &rhs)
    Returns \c true if \a lhs status code and \a rhs status code are equal.
*/

/*!
    \fn bool QGrpcStatus::operator!=(const QGrpcStatus &lhs, const QGrpcStatus &rhs)
    Returns \c true if \a lhs status code and \a rhs status code are not equal.
*/

class QGrpcStatusPrivate
{
public:
    QGrpcStatusPrivate(QGrpcStatus::StatusCode code, const QString &message)
        : m_code(code), m_message(message)
    {
    }

    ~QGrpcStatusPrivate() = default;

    QGrpcStatus::StatusCode m_code;
    QString m_message;
};

/*!
    Creates an instance of QGrpcStatus with a status \a code and a \a message.
*/
QGrpcStatus::QGrpcStatus(StatusCode code, const QString &message)
    : dPtr(std::make_unique<QGrpcStatusPrivate>(code, message))
{
}

/*!
    Copies the \a other QGrpcStatus to this QGrpcStatus.
*/
QGrpcStatus::QGrpcStatus(const QGrpcStatus &other)
    : dPtr(std::make_unique<QGrpcStatusPrivate>(other.dPtr->m_code, other.dPtr->m_message))
{
}

/*!
    Moves \a other into new instance of QGrpcStatus.
*/
QGrpcStatus::QGrpcStatus(QGrpcStatus &&other) : dPtr(std::move(other.dPtr))
{
}

/*!
    Assigns the \a other QGrpcStatus into this QGrpcStatus.
*/
QGrpcStatus &QGrpcStatus::operator=(const QGrpcStatus &other)
{
    dPtr->m_code = other.dPtr->m_code;
    dPtr->m_message = other.dPtr->m_message;
    return *this;
}

/*!
    Move assigns \a other into new instance of QGrpcStatus.
*/
QGrpcStatus &QGrpcStatus::operator=(QGrpcStatus &&other)
{
    dPtr = std::move(other.dPtr);
    return *this;
}

/*!
    Destroys the QGrpcStatus.
*/
QGrpcStatus::~QGrpcStatus() = default;

/*!
    \property QGrpcStatus::code
    \brief QGrpcStatus::StatusCode received for prior gRPC call.
*/
QGrpcStatus::StatusCode QGrpcStatus::code() const
{
    return dPtr->m_code;
}

/*!
    \property QGrpcStatus::message
    \brief Status message received for prior gRPC call.
*/
QString QGrpcStatus::message() const
{
    return dPtr->m_message;
}

QT_END_NAMESPACE

#include "moc_qgrpcstatus.cpp"
