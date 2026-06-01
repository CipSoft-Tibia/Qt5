// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpcQuick/qqmlgrpccalloptions.h>

#include <QtGrpcQuick/private/qqmlgrpcmetadata_p.h>

#include <QtCore/private/qobject_p.h>

#include <chrono>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

namespace QtGrpcQuickPrivate {

class QQmlGrpcCallOptionsPrivate : public QObjectPrivate
{
public:
    QGrpcCallOptions options;
    QQmlGrpcMetadata *metadata = nullptr;
    QMetaObject::Connection metadataUpdateConnection;
};

/*!
    \class QQmlGrpcCallOptions
    \internal
*/

QQmlGrpcCallOptions::QQmlGrpcCallOptions(QObject *parent)
    : QObject(*(new QQmlGrpcCallOptionsPrivate), parent)
{
}

QQmlGrpcCallOptions::~QQmlGrpcCallOptions() = default;

/*!
    \property QtGrpcQuickPrivate::QQmlGrpcCallOptions::deadlineTimeout

    This property holds the deadline timeout for the gRPC call in milliseconds.
    The deadline specifies how long the client is willing to wait for a reply
    from the server. If the deadline is exceeded, the call will be canceled.

    The default value is \c 0, which means no deadline is set.
*/
qint64 QQmlGrpcCallOptions::deadlineTimeout() const
{
    Q_D(const QQmlGrpcCallOptions);
    std::chrono::milliseconds ms = d->options.deadlineTimeout().value_or(0ms);
    return ms.count();
}

void QQmlGrpcCallOptions::setDeadlineTimeout(qint64 value)
{
    std::chrono::milliseconds ms(value);
    Q_D(QQmlGrpcCallOptions);
    d->options.setDeadlineTimeout(ms);
    emit deadlineTimeoutChanged();
}

const QGrpcCallOptions &QQmlGrpcCallOptions::options() const & noexcept
{
    Q_D(const QQmlGrpcCallOptions);
    return d->options;
}

/*!
    \property QtGrpcQuickPrivate::QQmlGrpcCallOptions::metadata

    This property holds the metadata to be sent with the gRPC call. Metadata
    consists of key-value pairs that are sent as HTTP/2 headers in the gRPC
    request.
*/
QQmlGrpcMetadata *QQmlGrpcCallOptions::metadata() const
{
    Q_D(const QQmlGrpcCallOptions);
    return d->metadata;
}

void QQmlGrpcCallOptions::setMetadata(QQmlGrpcMetadata *value)
{
    Q_D(QQmlGrpcCallOptions);
    if (d->metadata != value) {
        if (d->metadataUpdateConnection) {
            disconnect(d->metadataUpdateConnection);
            d->metadataUpdateConnection = {};
        }

        d->metadata = value;
        if (d->metadata) {
            const auto updateMetadata = [this]() {
                Q_D(QQmlGrpcCallOptions);
                d->options.setMetadata(d->metadata->metadata());
                emit metadataChanged();
            };
            d->metadataUpdateConnection = connect(d->metadata, &QQmlGrpcMetadata::dataChanged, this,
                                                  updateMetadata);
            updateMetadata();
        }
    }
}

}

QT_END_NAMESPACE

#include "moc_qqmlgrpccalloptions.cpp"
