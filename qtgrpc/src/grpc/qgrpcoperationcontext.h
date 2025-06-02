// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCOPERATIONCONTEXT_H
#define QGRPCOPERATIONCONTEXT_H

#include <QtGrpc/qtgrpcglobal.h>

#include <QtProtobuf/qabstractprotobufserializer.h>

#include <QtCore/qhash.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringfwd.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QGrpcCallOptions;
class QGrpcOperationContextPrivate;
class QGrpcStatus;

class Q_GRPC_EXPORT QGrpcOperationContext final : public QObject
{
    Q_OBJECT
    QT_DEFINE_TAG_STRUCT(PrivateConstructor);

public:
    explicit QGrpcOperationContext(QLatin1StringView method, QLatin1StringView service,
                                   QByteArrayView argument, const QGrpcCallOptions &options,
                                   std::shared_ptr<QAbstractProtobufSerializer> serializer,
                                   PrivateConstructor);
    ~QGrpcOperationContext() override;

    [[nodiscard]] QLatin1StringView method() const noexcept;
    [[nodiscard]] QLatin1StringView service() const noexcept;
    [[nodiscard]] QByteArrayView argument() const noexcept;

    void callOptions() && = delete;
    [[nodiscard]] const QGrpcCallOptions &callOptions() const & noexcept;

    void serverMetadata() && = delete;
    [[nodiscard]] const QHash<QByteArray, QByteArray> &serverMetadata() const & noexcept;
    void setServerMetadata(const QHash<QByteArray, QByteArray> &metadata);
    void setServerMetadata(QHash<QByteArray, QByteArray> &&metadata);

    [[nodiscard]] std::shared_ptr<const QAbstractProtobufSerializer> serializer() const;

Q_SIGNALS:
    // Outgoing signals of the channel.
    void finished(const QGrpcStatus &status);
    void messageReceived(const QByteArray &data);
    // Icoming signals from the client.
    void cancelRequested();
    void writeMessageRequested(const QByteArray &data);
    void writesDoneRequested();

private:
    Q_DISABLE_COPY_MOVE(QGrpcOperationContext)
    Q_DECLARE_PRIVATE(QGrpcOperationContext)

    friend class QAbstractGrpcChannel;

public:
    bool event(QEvent *event) override;
};

QT_END_NAMESPACE

#endif // QGRPCOPERATIONCONTEXT_H
