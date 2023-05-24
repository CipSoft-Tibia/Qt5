// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCOPERATION_H
#define QGRPCOPERATION_H

#include <QtCore/QObject>
#include <QtGrpc/qgrpcstatus.h>
#include <QtGrpc/qtgrpcglobal.h>
#include <QtGrpc/qgrpcmetadata.h>
#include <QtProtobuf/qabstractprotobufserializer.h>

QT_BEGIN_NAMESPACE

class QGrpcOperationPrivate;
class Q_GRPC_EXPORT QGrpcOperation : public QObject
{
    Q_OBJECT

public:
    template <typename T>
    T read() const
    {
        T value;
        if (auto ser = serializer(); ser)
            ser->deserialize(&value, data());
        return value;
    }

    void setData(const QByteArray &data);
    void setData(QByteArray &&data);

    virtual void abort() = 0;

    void setMetadata(const QGrpcMetadata &metadata);
    void setMetadata(QGrpcMetadata &&metadata);
    QGrpcMetadata metadata() const;
Q_SIGNALS:
    void finished();
    void errorOccurred(const QGrpcStatus &status);

protected:
    explicit QGrpcOperation(std::shared_ptr<QAbstractProtobufSerializer> serializer);
    ~QGrpcOperation() override;

private:
    Q_DISABLE_COPY_MOVE(QGrpcOperation)

    QByteArray data() const;
    std::shared_ptr<QAbstractProtobufSerializer> serializer() const;

    Q_DECLARE_PRIVATE(QGrpcOperation)
};

QT_END_NAMESPACE

#endif // QGRPCOPERATION_H
