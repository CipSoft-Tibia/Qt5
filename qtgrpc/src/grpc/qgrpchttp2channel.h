// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCHTTP2CHANNEL_H
#define QGRPCHTTP2CHANNEL_H

#include <QtGrpc/qabstractgrpcchannel.h>
#include <QtGrpc/qgrpcchanneloptions.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct QGrpcHttp2ChannelPrivate;

class Q_GRPC_EXPORT QGrpcHttp2Channel final : public QAbstractGrpcChannel
{
public:
    explicit QGrpcHttp2Channel(const QGrpcChannelOptions &options);
    ~QGrpcHttp2Channel() override;

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
    Q_DISABLE_COPY_MOVE(QGrpcHttp2Channel)

    std::unique_ptr<QGrpcHttp2ChannelPrivate> dPtr;
};

QT_END_NAMESPACE

#endif // QGRPCHTTP2CHANNEL_H
