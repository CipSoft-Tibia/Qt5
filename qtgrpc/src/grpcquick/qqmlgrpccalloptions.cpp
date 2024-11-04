// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmlgrpccalloptions_p.h"

#include <chrono>

QT_BEGIN_NAMESPACE

QQmlGrpcCallOptions::QQmlGrpcCallOptions(QObject *parent)
    : QObject(parent),
      m_metadata(nullptr)
{
}

QQmlGrpcCallOptions::~QQmlGrpcCallOptions() = default;

qint64 QQmlGrpcCallOptions::deadline() const
{
    std::chrono::milliseconds ms = m_options.deadline().value_or(std::chrono::milliseconds(0));
    return ms.count();
}

void QQmlGrpcCallOptions::setDeadline(qint64 value)
{
    std::chrono::milliseconds ms(value);
    m_options.withDeadline(ms);
    emit deadlineChanged();
}

QGrpcCallOptions QQmlGrpcCallOptions::options() const
{
    return m_options;
}

QQmlGrpcMetadata *QQmlGrpcCallOptions::metadata() const
{
    return m_metadata;
}

void QQmlGrpcCallOptions::setMetadata(QQmlGrpcMetadata *value)
{
    if (m_metadata != value) {
        m_metadata = value;
        if (m_metadata)
            m_options.withMetadata(m_metadata->metadata());
        else
            m_options.withMetadata(QGrpcMetadata());
        emit metadataChanged();
    }
}

QT_END_NAMESPACE
