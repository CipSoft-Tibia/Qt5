// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTGRPCLIENT_H
#define QABSTRACTGRPCLIENT_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>
#include <QtCore/qbytearray.h>
#include <QtGrpc/qabstractgrpcchannel.h>
#include <QtGrpc/qgrpcstatus.h>
#include <QtGrpc/qtgrpcglobal.h>
#include <QtProtobuf/qabstractprotobufserializer.h>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QGrpcOperation;
class QAbstractGrpcChannel;
class QAbstractGrpcClientPrivate;

class Q_GRPC_EXPORT QAbstractGrpcClient : public QObject
{
    Q_OBJECT

public:
    void attachChannel(const std::shared_ptr<QAbstractGrpcChannel> &channel);

Q_SIGNALS:
    void errorOccurred(const QGrpcStatus &status);

protected:
    explicit QAbstractGrpcClient(QLatin1StringView service, QObject *parent = nullptr);
    ~QAbstractGrpcClient() override;

    template <typename ParamType, typename ReturnType>
    QGrpcStatus call(QLatin1StringView method, const QProtobufMessage &arg, ReturnType &ret,
                     const QGrpcCallOptions &options)
    {
        using namespace Qt::StringLiterals;
        QGrpcStatus status{ QGrpcStatus::Unknown,
                            "Serializing failed. Serializer is not ready."_L1 };

        std::optional<QByteArray> argData = trySerialize<ParamType>(arg);
        if (argData) {
            QByteArray retData;
            status = call(method, *argData, retData, options);
            if (status == QGrpcStatus::StatusCode::Ok)
                status = tryDeserialize(ret, retData);
        }
        return status;
    }

    template <typename ParamType>
    std::shared_ptr<QGrpcCallReply> call(QLatin1StringView method, const QProtobufMessage &arg,
                                         const QGrpcCallOptions &options)
    {
        std::optional<QByteArray> argData = trySerialize<ParamType>(arg);
        if (!argData)
            return {};
        return call(method, *argData, options);
    }

    template <typename ParamType>
    std::shared_ptr<QGrpcStream> startStream(QLatin1StringView method, const QProtobufMessage &arg,
                                             const QGrpcCallOptions &options)
    {
        std::optional<QByteArray> argData = trySerialize<ParamType>(arg);
        if (!argData)
            return {};
        return startStream(method, *argData, options);
    }

private:
    QGrpcStatus call(QLatin1StringView method, QByteArrayView arg, QByteArray &ret,
                     const QGrpcCallOptions &options);

    std::shared_ptr<QGrpcCallReply> call(QLatin1StringView method, QByteArrayView arg,
                                         const QGrpcCallOptions &options);

    std::shared_ptr<QGrpcStream> startStream(QLatin1StringView method, QByteArrayView arg,
                                             const QGrpcCallOptions &options);

    template <typename ReturnType>
    QGrpcStatus tryDeserialize(ReturnType *ret, QByteArrayView retData)
    {
        auto _serializer = serializer();
        if (_serializer == nullptr)
            return QGrpcStatus::Unknown;
        if (!_serializer->deserialize(ret, retData))
            return handleDeserializationError(_serializer->deserializationError());
        return QGrpcStatus::Ok;
    }

    template <typename ParamType>
    std::optional<QByteArray> trySerialize(const QProtobufMessage &arg)
    {
        using namespace Qt::StringLiterals;
        auto _serializer = serializer();
        if (_serializer == nullptr) {
            Q_EMIT errorOccurred({ QGrpcStatus::Unknown,
                                   "Serializing failed. Serializer is not ready."_L1 });
            return std::nullopt;
        }
        return _serializer->serialize<ParamType>(&arg);
    }

    std::shared_ptr<QAbstractProtobufSerializer> serializer() const;

    QGrpcStatus handleDeserializationError(
            const QAbstractProtobufSerializer::DeserializationError &err);

    Q_DISABLE_COPY_MOVE(QAbstractGrpcClient)
    Q_DECLARE_PRIVATE(QAbstractGrpcClient)
};

QT_END_NAMESPACE

#endif // QABSTRACTGRPCLIENT_H
