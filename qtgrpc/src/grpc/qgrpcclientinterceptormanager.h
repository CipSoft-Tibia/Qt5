// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCLIENTINTERCEPTORMANAGER_H
#define QGRPCCLIENTINTERCEPTORMANAGER_H

#include <QtCore/qshareddata.h>

#include <QtGrpc/qgrpcclientinterceptor.h>
#include <QtGrpc/qtgrpcglobal.h>

QT_BEGIN_NAMESPACE

class QGrpcClientInterceptor;
class QGrpcClientInterceptorManagerPrivate;

class Q_GRPC_EXPORT QGrpcClientInterceptorManager
{
public:
    QGrpcClientInterceptorManager();
    QGrpcClientInterceptorManager(const QGrpcClientInterceptorManager &other);
    QGrpcClientInterceptorManager(QGrpcClientInterceptorManager &&other);
    QGrpcClientInterceptorManager &operator=(const QGrpcClientInterceptorManager &other);
    QGrpcClientInterceptorManager &operator=(QGrpcClientInterceptorManager &&other);
    ~QGrpcClientInterceptorManager();

    void registerInterceptor(std::shared_ptr<QGrpcClientInterceptor> next);
    void
    registerInterceptors(std::vector<std::shared_ptr<QGrpcClientInterceptor>> nextInterceptors);

    template <typename T>
    void run(QGrpcInterceptorContinuation<T> &finalCall,
             typename QGrpcInterceptorContinuation<T>::ReplyType reponse,
             std::shared_ptr<QGrpcChannelOperation> operation, size_t pos = 0);

private:
    QGrpcClientInterceptorManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QGrpcClientInterceptorManager)
};

QT_END_NAMESPACE

#endif // QGRPCCLIENTINTERCEPTORMANAGER_H
