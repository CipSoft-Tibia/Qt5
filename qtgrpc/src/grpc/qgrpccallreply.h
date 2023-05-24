// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCALLREPLY_H
#define QGRPCCALLREPLY_H

#include <QtGrpc/qgrpcoperation.h>
#include <QtGrpc/qtgrpcglobal.h>

QT_BEGIN_NAMESPACE

class Q_GRPC_EXPORT QGrpcCallReply final : public QGrpcOperation
{
    Q_OBJECT

public:
    explicit QGrpcCallReply(std::shared_ptr<QAbstractProtobufSerializer> serializer);
    ~QGrpcCallReply() override;

    void abort() override;

    template<typename Func1, typename Func2>
    void subscribe(QObject *receiver, Func1 &&finishCallback, Func2 &&errorCallback,
                   Qt::ConnectionType type = Qt::AutoConnection)
    {
        QObject::connect(this, &QGrpcCallReply::finished, receiver,
                         std::forward<Func1>(finishCallback), type);
        QObject::connect(this, &QGrpcCallReply::errorOccurred, receiver,
                         std::forward<Func2>(errorCallback), type);
    }

    template<typename Func1>
    void subscribe(QObject *receiver, Func1 &&finishCallback,
                   Qt::ConnectionType type = Qt::AutoConnection)
    {
        QObject::connect(this, &QGrpcCallReply::finished, receiver,
                         std::forward<Func1>(finishCallback), type);
    }

private:
    QGrpcCallReply();
    Q_DISABLE_COPY_MOVE(QGrpcCallReply)
};

QT_END_NAMESPACE

#endif // QGRPCCALLREPLY_H
