// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCALLOPTIONS_H
#define QGRPCALLOPTIONS_H

#include <QtCore/QUrl>
#include <QtGrpc/qgrpcmetadata.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <chrono>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

struct QGrpcCallOptionsPrivate;

class Q_GRPC_EXPORT QGrpcCallOptions final
{
public:
    QGrpcCallOptions();
    ~QGrpcCallOptions();

    QGrpcCallOptions(const QGrpcCallOptions &other);
    QGrpcCallOptions &operator=(const QGrpcCallOptions &other);

    QGrpcCallOptions &withDeadline(std::chrono::milliseconds deadline);
    QGrpcCallOptions &withMaxRetryAttempts(qint64 maxRetryAttempts);
    QGrpcCallOptions &withMetadata(const QGrpcMetadata &metadata);

    std::optional<std::chrono::milliseconds> deadline() const;
    std::optional<qint64> maxRetryAttempts() const;
    QGrpcMetadata metadata() const;

private:
    std::unique_ptr<QGrpcCallOptionsPrivate> dPtr;
};

QT_END_NAMESPACE

#endif // QGRPCALLOPTIONS_H
