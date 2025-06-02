// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCALLREPLY_H
#define QGRPCCALLREPLY_H

#include <QtGrpc/qgrpcoperation.h>
#include <QtGrpc/qgrpcoperationcontext.h>
#include <QtGrpc/qgrpcstatus.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <QtCore/qobject.h>

#include <memory>

QT_BEGIN_NAMESPACE

class Q_GRPC_EXPORT QGrpcCallReply final : public QGrpcOperation
{
    Q_OBJECT

public:
    explicit QGrpcCallReply(std::shared_ptr<QGrpcOperationContext> operationContext);
    ~QGrpcCallReply() override;

private:
    Q_DISABLE_COPY_MOVE(QGrpcCallReply)

public:
    bool event(QEvent *event) override;
};

QT_END_NAMESPACE

#endif // QGRPCCALLREPLY_H
