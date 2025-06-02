// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCSTREAM_H
#define QGRPCSTREAM_H

#include <QtGrpc/qgrpcoperation.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <memory>

QT_BEGIN_NAMESPACE

class Q_GRPC_EXPORT QGrpcServerStream final : public QGrpcOperation
{
    Q_OBJECT

public:
    explicit QGrpcServerStream(std::shared_ptr<QGrpcOperationContext> operationContext,
                               QObject *parent = nullptr);
    ~QGrpcServerStream() override;

Q_SIGNALS:
    void messageReceived();

private:
    Q_DISABLE_COPY_MOVE(QGrpcServerStream)

public:
    bool event(QEvent *event) override;
};

class Q_GRPC_EXPORT QGrpcClientStream final : public QGrpcOperation
{
    Q_OBJECT

public:
    explicit QGrpcClientStream(std::shared_ptr<QGrpcOperationContext> operationContext,
                               QObject *parent = nullptr);
    ~QGrpcClientStream() override;

    void writeMessage(const QProtobufMessage &message);
    void writesDone();

private:
    Q_DISABLE_COPY_MOVE(QGrpcClientStream)

public:
    bool event(QEvent *event) override;
};

class Q_GRPC_EXPORT QGrpcBidiStream final : public QGrpcOperation
{
    Q_OBJECT

public:
    explicit QGrpcBidiStream(std::shared_ptr<QGrpcOperationContext> operationContext,
                             QObject *parent = nullptr);
    ~QGrpcBidiStream() override;

    void writeMessage(const QProtobufMessage &message);
    void writesDone();

Q_SIGNALS:
    void messageReceived();

private:
    Q_DISABLE_COPY_MOVE(QGrpcBidiStream)

public:
    bool event(QEvent *event) override;
};

QT_END_NAMESPACE

#endif // QGRPCSTREAM_H
