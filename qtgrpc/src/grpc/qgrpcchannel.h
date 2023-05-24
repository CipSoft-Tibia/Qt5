// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Giulio Girardi <giulio.girardi@protechgroup.it>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCHANNEL_H
#define QGRPCCHANNEL_H

#include <QtGrpc/qabstractgrpcclient.h>
#include <QtGrpc/qgrpcchanneloptions.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct QGrpcChannelPrivate;

class Q_GRPC_EXPORT QGrpcChannel final : public QAbstractGrpcChannel
{
public:
    enum NativeGrpcChannelCredentials : uint8_t {
        InsecureChannelCredentials = 0,
        GoogleDefaultCredentials,
        SslDefaultCredentials,
    };

    explicit QGrpcChannel(const QGrpcChannelOptions &options,
                          NativeGrpcChannelCredentials credentialsType);

    ~QGrpcChannel() override;

    QGrpcStatus call(QLatin1StringView method, QLatin1StringView service, QByteArrayView args,
                     QByteArray &ret,
                     const QGrpcCallOptions &options = QGrpcCallOptions()) override;
    std::shared_ptr<QGrpcCallReply> call(
            QLatin1StringView method, QLatin1StringView service, QByteArrayView args,
            const QGrpcCallOptions &options = QGrpcCallOptions()) override;
    std::shared_ptr<QGrpcStream> startStream(
            QLatin1StringView method, QLatin1StringView service, QByteArrayView arg,
            const QGrpcCallOptions &options = QGrpcCallOptions()) override;
    std::shared_ptr<QAbstractProtobufSerializer> serializer() const override;

private:
    Q_DISABLE_COPY_MOVE(QGrpcChannel)

    std::unique_ptr<QGrpcChannelPrivate> dPtr;
};

QT_END_NAMESPACE

#endif // QGRPCCHANNEL_H
