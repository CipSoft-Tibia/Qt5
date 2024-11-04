// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef INTERCEPTORMOCKS_H
#define INTERCEPTORMOCKS_H

#include <QDebug>
#include <QObject>

#include <QtGrpc/qgrpccallreply.h>
#include <QtGrpc/qgrpcclientinterceptor.h>
#include <QtGrpc/qgrpcstream.h>
#include <QtGrpc/qtgrpcglobal.h>

class CallInterceptor : public QGrpcClientInterceptor
{
public:
    using funcType = std::function<
        void(std::shared_ptr<QGrpcChannelOperation>, std::shared_ptr<QGrpcCallReply>,
             QGrpcInterceptorContinuation<QGrpcCallReply> &, QLatin1StringView)>;

    CallInterceptor(QByteArrayView _id, funcType _func) : id(_id.begin(), _id.size()), func(_func)
    {
    }

    void interceptCall(std::shared_ptr<QGrpcChannelOperation> operation,
                       std::shared_ptr<QGrpcCallReply> response,
                       QGrpcInterceptorContinuation<QGrpcCallReply> &continuation) override
    {
        qDebug() << id << "called interceptCall()";
        func(operation, response, continuation, QLatin1StringView(id));
    }

private:
    QByteArray id;
    funcType func;
};

class ServerStreamInterceptor : public QGrpcClientInterceptor
{
public:
    using funcType = std::function<
        void(std::shared_ptr<QGrpcChannelOperation>, std::shared_ptr<QGrpcServerStream>,
             QGrpcInterceptorContinuation<QGrpcServerStream> &, QLatin1StringView)>;

    ServerStreamInterceptor(QByteArrayView _id, funcType _func)
        : id(_id.begin(), _id.size()), func(_func)
    {
    }

    void
    interceptServerStream(std::shared_ptr<QGrpcChannelOperation> operation,
                          std::shared_ptr<QGrpcServerStream> response,
                          QGrpcInterceptorContinuation<QGrpcServerStream> &continuation) override
    {
        qDebug() << id << "called interceptCall()";
        func(operation, response, continuation, QLatin1StringView(id));
    }

private:
    QByteArray id;
    funcType func;
};

class MockInterceptor : public QGrpcClientInterceptor
{
public:
    MockInterceptor(QByteArrayView _id,
                    std::function<void(std::shared_ptr<QGrpcChannelOperation>, QLatin1StringView)>
                        _func = nullptr)
        : id(_id.begin(), _id.size()), func(_func)
    {
    }

protected:
    void interceptCall(std::shared_ptr<QGrpcChannelOperation> operation,
                       std::shared_ptr<QGrpcCallReply> response,
                       QGrpcInterceptorContinuation<QGrpcCallReply> &continuation) override
    {
        qDebug() << id << "called interceptCall()";
        if (func)
            func(operation, QLatin1StringView(id));
        continuation(std::move(response), std::move(operation));
    }
    void
    interceptServerStream(std::shared_ptr<QGrpcChannelOperation> operation,
                          std::shared_ptr<QGrpcServerStream> response,
                          QGrpcInterceptorContinuation<QGrpcServerStream> &continuation) override
    {
        qDebug() << id << "called interceptServerStream()";
        if (func)
            func(operation, QLatin1StringView(id));
        continuation(std::move(response), std::move(operation));
    }
    void
    interceptClientStream(std::shared_ptr<QGrpcChannelOperation> operation,
                          std::shared_ptr<QGrpcClientStream> response,
                          QGrpcInterceptorContinuation<QGrpcClientStream> &continuation) override
    {
        qDebug() << id << "called interceptClientStream()";
        func(operation, QLatin1StringView(id));
        continuation(std::move(response), std::move(operation));
    }
    void interceptBidirStream(std::shared_ptr<QGrpcChannelOperation> operation,
                              std::shared_ptr<QGrpcBidirStream> response,
                              QGrpcInterceptorContinuation<QGrpcBidirStream> &continuation) override
    {
        qDebug() << id << "called interceptBidirStream()";
        if (func)
            func(operation, QLatin1StringView(id));
        continuation(std::move(response), std::move(operation));
    }

private:
    QByteArray id;
    std::function<void(std::shared_ptr<QGrpcChannelOperation>, QLatin1StringView)> func;
};

class NoContinuationInterceptor : public QGrpcClientInterceptor
{
public:
    NoContinuationInterceptor(QLatin1StringView _id,
                              std::function<void(std::shared_ptr<QGrpcChannelOperation>,
                                                 QLatin1StringView)>
                                  _func = nullptr)
        : id(_id.begin(), _id.size()), func(_func)
    {
    }

protected:
    void interceptCall(std::shared_ptr<QGrpcChannelOperation> operation,
                       std::shared_ptr<QGrpcCallReply> response,
                       QGrpcInterceptorContinuation<QGrpcCallReply> &) override
    {
        qDebug() << id << "called interceptCall(), but continuation won't be called.";
        if (func)
            func(operation, QLatin1StringView(id));
        response->cancel();
    }
    void interceptServerStream(std::shared_ptr<QGrpcChannelOperation> operation,
                               std::shared_ptr<QGrpcServerStream> response,
                               QGrpcInterceptorContinuation<QGrpcServerStream> &) override
    {
        qDebug() << id << "called interceptServerStream(), but continuation won't be called.";
        if (func)
            func(operation, QLatin1StringView(id));
        response->cancel();
    }
    void interceptClientStream(std::shared_ptr<QGrpcChannelOperation> operation,
                               std::shared_ptr<QGrpcClientStream> response,
                               QGrpcInterceptorContinuation<QGrpcClientStream> &) override
    {
        qDebug() << id << "called interceptClientStream(), but continuation won't be called.";
        if (func)
            func(operation, QLatin1StringView(id));
        response->cancel();
    }
    void interceptBidirStream(std::shared_ptr<QGrpcChannelOperation> operation,
                              std::shared_ptr<QGrpcBidirStream> response,
                              QGrpcInterceptorContinuation<QGrpcBidirStream> &) override
    {
        qDebug() << id << "called interceptBidirStream(), but continuation won't be called.";
        if (func)
            func(operation, QLatin1StringView(id));
        response->cancel();
    }

private:
    QByteArray id;
    std::function<void(std::shared_ptr<QGrpcChannelOperation>, QLatin1StringView)> func;
};

#endif // INTERCEPTORMOCKS_H
