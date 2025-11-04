// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTGRPCOPERATION_P_H
#define QTGRPCOPERATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGrpc/qgrpcoperation.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

class QGrpcOperationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGrpcOperation)
public:
    explicit QGrpcOperationPrivate(std::shared_ptr<QGrpcOperationContext> &&operationContext_)
        : operationContext(std::move(operationContext_))
    {
    }
    ~QGrpcOperationPrivate() override;

    QByteArray data;
    std::shared_ptr<QGrpcOperationContext> operationContext;
    QAtomicInteger<bool> isFinished{ false };

    static const QGrpcOperationPrivate *get(const QGrpcOperation *op) { return op->d_func(); }
};

QT_END_NAMESPACE

#endif // QTGRPCOPERATION_P_H
