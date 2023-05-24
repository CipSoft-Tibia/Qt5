// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Giulio Girardi <giulio.girardi@protechgroup.it>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCHANNEL_P_H
#define QGRPCCHANNEL_P_H

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

#include <QtGrpc/qabstractgrpcclient.h>
#include <QtGrpc/qgrpccallreply.h>
#include <QtGrpc/qgrpcchannel.h>
#include <QtGrpc/qgrpcstream.h>
#include <QtGrpc/private/qtgrpcglobal_p.h>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/qbytearray.h>

QT_REQUIRE_CONFIG(native_grpc);
#include <grpcpp/channel.h>
#include <grpcpp/impl/codegen/byte_buffer.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/sync_stream.h>
#include <grpcpp/security/credentials.h>

QT_BEGIN_NAMESPACE

class QGrpcChannelStream : public QObject
{
    Q_OBJECT

public:
    explicit QGrpcChannelStream(grpc::Channel *channel, QLatin1StringView method,
                                QByteArrayView data);
    ~QGrpcChannelStream() override;

    void cancel();
    void start();

Q_SIGNALS:
    void dataReady(const QByteArray &data);
    void finished();

public:
    QGrpcStatus status;

private:
    QThread *thread;
    grpc::ClientContext context;
    grpc::ClientReader<grpc::ByteBuffer> *reader = nullptr;
};

class QGrpcChannelCall : public QObject
{
    Q_OBJECT

public:
    explicit QGrpcChannelCall(grpc::Channel *channel, QLatin1StringView method,
                              QByteArrayView data);
    ~QGrpcChannelCall() override;

    void cancel();
    void start();
    void waitForFinished(const QDeadlineTimer &deadline = QDeadlineTimer(QDeadlineTimer::Forever));

Q_SIGNALS:
    void finished();

public:
    QGrpcStatus status;
    QByteArray response;

private:
    QThread *thread;
    grpc::ClientContext context;
};

struct QGrpcChannelPrivate
{
    std::shared_ptr<grpc::Channel> m_channel;
    std::shared_ptr<grpc::ChannelCredentials> m_credentials;
    QGrpcChannelOptions m_channelOptions;

    explicit QGrpcChannelPrivate(const QGrpcChannelOptions &channelOptions,
                                 QGrpcChannel::NativeGrpcChannelCredentials credentialsType);
    ~QGrpcChannelPrivate();

    std::shared_ptr<QGrpcCallReply> call(QLatin1StringView method, QLatin1StringView service,
                                         QByteArrayView args, const QGrpcCallOptions &options);
    QGrpcStatus call(QLatin1StringView method, QLatin1StringView service, QByteArrayView args,
                     QByteArray &ret, const QGrpcCallOptions &options);
    std::shared_ptr<QGrpcStream> startStream(QLatin1StringView method, QLatin1StringView service,
                                             QByteArrayView arg, const QGrpcCallOptions &options);
    std::shared_ptr<QAbstractProtobufSerializer> serializer() const;
};

QT_END_NAMESPACE

#endif // QGRPCCHANNEL_P_H
