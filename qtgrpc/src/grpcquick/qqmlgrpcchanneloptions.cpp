// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmlgrpcchanneloptions_p.h"
#include "qqmlgrpcmetadata_p.h"
#include <QtCore/private/qobject_p.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QQmlGrpcChannelOptionsPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlGrpcChannelOptions)

public:
    QQmlGrpcChannelOptionsPrivate();

    QGrpcChannelOptions m_options;
    QQmlGrpcMetadata *m_metadata = nullptr;
#if QT_CONFIG(ssl)
    QQmlSslConfiguration m_configuration;
#endif // QT_CONFIG(ssl)
};

QQmlGrpcChannelOptionsPrivate::QQmlGrpcChannelOptionsPrivate()
    : QObjectPrivate(),
      m_options(QUrl())
{
}

QQmlGrpcChannelOptions::QQmlGrpcChannelOptions(QObject *parent)
    : QObject(*(new QQmlGrpcChannelOptionsPrivate()), parent)
{
}

QUrl QQmlGrpcChannelOptions::host() const
{
    return d_func()->m_options.host();
}

void QQmlGrpcChannelOptions::setHost(const QUrl &newUrl)
{
    Q_D(QQmlGrpcChannelOptions);
    d->m_options.withHost(newUrl);
    emit hostChanged();
}

qint64 QQmlGrpcChannelOptions::deadline() const
{
    std::chrono::milliseconds ms
            = d_func()->m_options.deadline().value_or(std::chrono::milliseconds(0));
    return ms.count();
}

void QQmlGrpcChannelOptions::setDeadline(qint64 value)
{
    Q_D(QQmlGrpcChannelOptions);
    std::chrono::milliseconds ms(value);
    d->m_options.withDeadline(ms);
    emit deadlineChanged();
}

const QGrpcChannelOptions &QQmlGrpcChannelOptions::options() const
{
    return d_func()->m_options;
}

QQmlGrpcMetadata *QQmlGrpcChannelOptions::metadata() const
{
    return d_func()->m_metadata;
}

void QQmlGrpcChannelOptions::setMetadata(QQmlGrpcMetadata *value)
{
    Q_D(QQmlGrpcChannelOptions);
    if (d->m_metadata != value) {
        d->m_metadata = value;
        if (d->m_metadata)
            d->m_options.withMetadata(d->m_metadata->metadata());
        else
            d->m_options.withMetadata(QGrpcMetadata());
        emit metadataChanged();
    }
}

#if QT_CONFIG(ssl)
QQmlSslConfiguration QQmlGrpcChannelOptions::sslConfiguration() const
{
    return d_func()->m_configuration;
}

void QQmlGrpcChannelOptions::setSslConfiguration(const QQmlSslConfiguration &config)
{
    Q_D(QQmlGrpcChannelOptions);
    if (d->m_configuration != config) {
        d->m_configuration = config;
        d->m_options.withSslConfiguration(d->m_configuration.configuration());
        emit sslConfigurationChanged();
    }
}
#endif // QT_CONFIG(ssl)

QT_END_NAMESPACE
