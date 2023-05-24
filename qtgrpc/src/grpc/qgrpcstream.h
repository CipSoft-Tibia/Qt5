// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCSTREAM_H
#define QGRPCSTREAM_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGrpc/qgrpcoperation.h>
#include <QtGrpc/qtgrpcglobal.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QAbstractGrpcClient;

class Q_GRPC_EXPORT QGrpcStream final : public QGrpcOperation
{
    Q_OBJECT

public:
    explicit QGrpcStream(QLatin1StringView method, QByteArrayView arg,
                         std::shared_ptr<QAbstractProtobufSerializer> serializer);
    ~QGrpcStream() override;

    void abort() override;

    QLatin1StringView method() const;
    QByteArrayView arg() const;
    void updateData(const QByteArray &data);

Q_SIGNALS:
    void messageReceived();

private:
    const std::string m_method;
    const QByteArray m_arg;
};

QT_END_NAMESPACE

#endif // QGRPCSTREAM_H
