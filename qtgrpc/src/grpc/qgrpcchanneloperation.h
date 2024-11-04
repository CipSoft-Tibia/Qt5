// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCHANNELOPERATION_H
#define QGRPCCHANNELOPERATION_H

#include <QtCore/qobject.h>
#include <QtCore/qlatin1stringview.h>
#include <QtCore/qbytearrayview.h>
#include <QtGrpc/qgrpcmetadata.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractProtobufSerializer;
class QGrpcCallOptions;
class QGrpcChannelOperationPrivate;
class QGrpcStatus;

class Q_GRPC_EXPORT QGrpcChannelOperation : public QObject
{
    Q_OBJECT
public:
    explicit QGrpcChannelOperation(QLatin1StringView method, QLatin1StringView service,
                                   QByteArrayView arg, const QGrpcCallOptions &options,
                                   std::shared_ptr<QAbstractProtobufSerializer> serializer);
    ~QGrpcChannelOperation() override;

    [[nodiscard]] QLatin1StringView method() const noexcept;
    [[nodiscard]] QLatin1StringView service() const noexcept;
    [[nodiscard]] QByteArrayView argument() const noexcept;
    [[nodiscard]] const QGrpcCallOptions &options() const noexcept;
    [[nodiscard]] std::shared_ptr<const QAbstractProtobufSerializer> serializer() const noexcept;

    [[nodiscard]] const QGrpcMetadata &clientMetadata() const noexcept;
    [[nodiscard]] const QGrpcMetadata &serverMetadata() const noexcept;

    void setArgument(QByteArrayView arg);
    void setOptions(QGrpcCallOptions &options);

    void setClientMetadata(const QGrpcMetadata &metadata);
    void setClientMetadata(QGrpcMetadata &&metadata);
    void setServerMetadata(const QGrpcMetadata &metadata);
    void setServerMetadata(QGrpcMetadata &&metadata);

Q_SIGNALS:
    void dataReady(const QByteArray &data);
    void sendData(const QByteArray &data) const;
    void errorOccurred(const QGrpcStatus &status);
    void finished();

    void cancelled();

private:
    Q_DISABLE_COPY_MOVE(QGrpcChannelOperation)
    Q_DECLARE_PRIVATE(QGrpcChannelOperation)
};

QT_END_NAMESPACE

#endif // QGRPCCHANNELOPERATION_H
