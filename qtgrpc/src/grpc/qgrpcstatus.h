// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCSTATUS_H
#define QGRPCSTATUS_H

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/qobjectdefs.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QGrpcStatusPrivate;

class Q_GRPC_EXPORT QGrpcStatus final
{
    Q_GADGET
    Q_PROPERTY(StatusCode code READ code CONSTANT)
    Q_PROPERTY(QString message READ message CONSTANT)

public:
    enum StatusCode : uint8_t {
        Ok = 0,
        Cancelled = 1,
        Unknown = 2,
        InvalidArgument = 3,
        DeadlineExceeded = 4,
        NotFound = 5,
        AlreadyExists = 6,
        PermissionDenied = 7,
        Unauthenticated = 16,
        ResourceExhausted = 8,
        FailedPrecondition = 9,
        Aborted = 10,
        OutOfRange = 11,
        Unimplemented = 12,
        Internal = 13,
        Unavailable = 14,
        DataLoss = 15,
    };

    Q_ENUM(StatusCode)

    QGrpcStatus(StatusCode code = StatusCode::Ok, const QString &message = QString());
    ~QGrpcStatus();

    StatusCode code() const;
    QString message() const;

    QGrpcStatus(const QGrpcStatus &other);
    QGrpcStatus &operator=(const QGrpcStatus &other);

    QGrpcStatus(QGrpcStatus &&other);
    QGrpcStatus &operator=(QGrpcStatus &&other);

private:
    friend bool operator==(const QGrpcStatus &lhs, QGrpcStatus::StatusCode code)
    {
        return lhs.code() == code;
    }
    friend bool operator!=(const QGrpcStatus &lhs, QGrpcStatus::StatusCode code)
    {
        return lhs.code() != code;
    }
    friend bool operator==(const QGrpcStatus &lhs, const QGrpcStatus &rhs)
    {
        return lhs.code() == rhs.code();
    }
    friend bool operator!=(const QGrpcStatus &lhs, const QGrpcStatus &rhs)
    {
        return lhs.code() == rhs.code();
    }

    std::unique_ptr<QGrpcStatusPrivate> dPtr;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QGrpcStatus)

#endif // QGRPCSTATUS_H
