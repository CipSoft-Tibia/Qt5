// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCOPERATION_H
#define QGRPCOPERATION_H

#include <QtCore/qobject.h>
#include <QtGrpc/qgrpcstatus.h>
#include <QtGrpc/qtgrpcglobal.h>
#include <QtGrpc/qgrpcmetadata.h>
#include <QtProtobuf/qabstractprotobufserializer.h>

QT_BEGIN_NAMESPACE

class QGrpcChannelOperation;
class QGrpcOperationPrivate;

class Q_GRPC_EXPORT QGrpcOperation : public QObject
{
    Q_OBJECT

public:
    ~QGrpcOperation() override;

    template <typename T>
    T read() const
    {
        T value;
        if (auto ser = serializer(); ser) {
            if (!ser->deserialize(&value, data()))
                errorOccurred(deserializationError());
        }
        return value;
    }

    [[nodiscard]] QGrpcMetadata metadata() const noexcept;
    [[nodiscard]] QLatin1StringView method() const noexcept;

    void cancel();
    [[nodiscard]] bool isFinished() const noexcept;

Q_SIGNALS:
    void finished();
    void errorOccurred(const QGrpcStatus &status) const;

protected:
    explicit QGrpcOperation(std::shared_ptr<QGrpcChannelOperation> channelOperation);

    [[nodiscard]] const QGrpcChannelOperation *channelOperation() const noexcept;
    [[nodiscard]] std::shared_ptr<const QAbstractProtobufSerializer> serializer() const noexcept;

private:
    Q_DISABLE_COPY_MOVE(QGrpcOperation)

    [[nodiscard]] QByteArray data() const noexcept;
    [[nodiscard]] QGrpcStatus deserializationError() const;

    Q_DECLARE_PRIVATE(QGrpcOperation)
};

QT_END_NAMESPACE

#endif // QGRPCOPERATION_H
