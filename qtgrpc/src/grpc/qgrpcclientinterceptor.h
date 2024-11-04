// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCLIENTINTERCEPTOR_H
#define QGRPCCLIENTINTERCEPTOR_H

#include <QtCore/qtmetamacros.h>

#include <QtGrpc/qabstractgrpcchannel.h>
#include <QtGrpc/qgrpcchanneloperation.h>
#include <QtGrpc/qgrpcoperation.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <memory>

QT_BEGIN_NAMESPACE

template <typename T>
struct QGrpcRpcInfo
{
    using ReplyType = std::shared_ptr<T>;
};

template <typename T>
class Q_GRPC_EXPORT QGrpcInterceptorContinuation final
{
public:
    using ReplyType = typename QGrpcRpcInfo<T>::ReplyType;
    using ParamType = std::shared_ptr<QGrpcChannelOperation>;

    QGrpcInterceptorContinuation(std::function<void(ReplyType, ParamType)> _func)
        : func(std::move(_func))
    {
    }

    void operator()(ReplyType response, ParamType param) { func(response, param); }

private:
    std::function<void(ReplyType, ParamType)> func;
};

class Q_GRPC_EXPORT QGrpcClientInterceptor
{
public:
    virtual ~QGrpcClientInterceptor();

#ifdef Q_QDOC
    template <typename T>
#else
    template <typename T, std::enable_if_t<std::is_base_of_v<QGrpcOperation, T>, bool> = true>
#endif
    void intercept(std::shared_ptr<QGrpcChannelOperation> operation,
                   typename QGrpcInterceptorContinuation<T>::ReplyType response,
                   QGrpcInterceptorContinuation<T> &continuation)
    {
        if constexpr (std::is_same_v<T, QGrpcCallReply>) {
            interceptCall(operation, response, continuation);
            return;
        }
        if constexpr (std::is_same_v<T, QGrpcServerStream>) {
            interceptServerStream(operation, response, continuation);
            return;
        }
        if constexpr (std::is_same_v<T, QGrpcClientStream>) {
            interceptClientStream(operation, response, continuation);
            return;
        }
        if constexpr (std::is_same_v<T, QGrpcBidirStream>) {
            interceptBidirStream(operation, response, continuation);
            return;
        }
        Q_ASSERT_X(false, "QGrpcClientInterceptor<T>::intercept()",
                   "Unknown QGrpcChannelOperation.");
    }

protected:
    virtual void interceptCall(std::shared_ptr<QGrpcChannelOperation> operation,
                               std::shared_ptr<QGrpcCallReply> response,
                               QGrpcInterceptorContinuation<QGrpcCallReply> &continuation);
    virtual void
    interceptServerStream(std::shared_ptr<QGrpcChannelOperation> operation,
                          std::shared_ptr<QGrpcServerStream> response,
                          QGrpcInterceptorContinuation<QGrpcServerStream> &continuation);
    virtual void
    interceptClientStream(std::shared_ptr<QGrpcChannelOperation> operation,
                          std::shared_ptr<QGrpcClientStream> response,
                          QGrpcInterceptorContinuation<QGrpcClientStream> &continuation);
    virtual void interceptBidirStream(std::shared_ptr<QGrpcChannelOperation> operation,
                                      std::shared_ptr<QGrpcBidirStream> response,
                                      QGrpcInterceptorContinuation<QGrpcBidirStream> &continuation);
};

QT_END_NAMESPACE

#endif // QGRPCCLIENTINTERCEPTOR_H
