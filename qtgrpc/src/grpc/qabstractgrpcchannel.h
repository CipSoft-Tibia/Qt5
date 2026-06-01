// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTGRPCCHANNEL_H
#define QABSTRACTGRPCCHANNEL_H

#include <QtGrpc/qtgrpcglobal.h>

#include <QtCore/qmetatype.h>
#include <QtCore/qstringfwd.h>
#include <QtCore/qtclasshelpermacros.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractProtobufSerializer;
class QGrpcBidiStream;
class QGrpcCallReply;
class QGrpcOperationContext;
class QGrpcChannelOptions;
class QGrpcCallOptions;
class QGrpcClientBase;
class QGrpcClientStream;
class QGrpcServerStream;

class QAbstractGrpcChannelPrivate;
class Q_GRPC_EXPORT QAbstractGrpcChannel
{
public:
    virtual ~QAbstractGrpcChannel();

    [[nodiscard]] virtual std::shared_ptr<QAbstractProtobufSerializer> serializer() const = 0;

    [[nodiscard]] const QGrpcChannelOptions &channelOptions() const & noexcept;
    void channelOptions() const && = delete;

    void setChannelOptions(const QGrpcChannelOptions &options);
    void setChannelOptions(QGrpcChannelOptions &&options);

protected:
    QAbstractGrpcChannel();
    explicit QAbstractGrpcChannel(QAbstractGrpcChannelPrivate &dd);
    explicit QAbstractGrpcChannel(const QGrpcChannelOptions &options);

private:
    virtual void call(std::shared_ptr<QGrpcOperationContext> operationContext) = 0;
    virtual void serverStream(std::shared_ptr<QGrpcOperationContext> operationContext) = 0;
    virtual void clientStream(std::shared_ptr<QGrpcOperationContext> operationContext) = 0;
    virtual void bidiStream(std::shared_ptr<QGrpcOperationContext> operationContext) = 0;

private:
    std::unique_ptr<QGrpcCallReply> call(QLatin1StringView method, QLatin1StringView service,
                                         QByteArrayView arg, const QGrpcCallOptions &options);
    std::unique_ptr<QGrpcServerStream> serverStream(QLatin1StringView method,
                                                    QLatin1StringView service, QByteArrayView arg,
                                                    const QGrpcCallOptions &options);
    std::unique_ptr<QGrpcClientStream> clientStream(QLatin1StringView method,
                                                    QLatin1StringView service, QByteArrayView arg,
                                                    const QGrpcCallOptions &options);
    std::unique_ptr<QGrpcBidiStream> bidiStream(QLatin1StringView method, QLatin1StringView service,
                                                QByteArrayView arg,
                                                const QGrpcCallOptions &options);

private:
    friend class QGrpcClientBase;
    friend class QGrpcClientBasePrivate;

    std::unique_ptr<QAbstractGrpcChannelPrivate> d_ptr;

    Q_DISABLE_COPY_MOVE(QAbstractGrpcChannel)
    Q_DECLARE_PRIVATE(QAbstractGrpcChannel)
};

QT_END_NAMESPACE

#endif // QABSTRACTGRPCCHANNEL_H
