// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTGRPCNAMESPACE_H
#define QTGRPCNAMESPACE_H

#if 0
#pragma qt_class(QtGrpcNamespace)
#endif

#include <QtGrpc/qtgrpcexports.h>

#include <QtCore/qtmetamacros.h>

QT_BEGIN_NAMESPACE

struct QMetaObject;

namespace QtGrpc {
Q_NAMESPACE_EXPORT(Q_GRPC_EXPORT)

enum class SerializationFormat : quint8 {
    Default,
    Protobuf,
    Json,
};
Q_ENUM_NS(SerializationFormat)

enum class StatusCode : quint8 {
    Ok = 0,
    Cancelled = 1,
    Unknown = 2,
    InvalidArgument = 3,
    DeadlineExceeded = 4,
    NotFound = 5,
    AlreadyExists = 6,
    PermissionDenied = 7,
    ResourceExhausted = 8,
    FailedPrecondition = 9,
    Aborted = 10,
    OutOfRange = 11,
    Unimplemented = 12,
    Internal = 13,
    Unavailable = 14,
    DataLoss = 15,
    Unauthenticated = 16,
};
Q_ENUM_NS(StatusCode)

// ### Qt7: remove QHash metadata interfaces.
inline QT_DEFINE_TAG(MultiValue);

Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
} // namespace QtGrpc

QT_END_NAMESPACE

#endif // QTGRPCNAMESPACE_H
