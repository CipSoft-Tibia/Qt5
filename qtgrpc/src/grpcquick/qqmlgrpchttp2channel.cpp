// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmlgrpchttp2channel_p.h"
#include <QtGrpc/qabstractgrpcclient.h>
#include <QtGrpc/qgrpchttp2channel.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQmlGrpcHttp2Channel::QQmlGrpcHttp2Channel(QObject *parent)
    : QQmlAbstractGrpcChannel(parent),
      m_options(nullptr)
{
    connect(this, &QQmlGrpcHttp2Channel::optionsChanged,
            this, &QQmlGrpcHttp2Channel::updateChannel);
}

QQmlGrpcHttp2Channel::~QQmlGrpcHttp2Channel() = default;

std::shared_ptr<QAbstractGrpcChannel> QQmlGrpcHttp2Channel::getChannel()
{
    return m_channel;
}

void QQmlGrpcHttp2Channel::setOptions(QQmlGrpcChannelOptions *options)
{
    if (options == nullptr || m_options == options)
        return;

    m_options = options;
    emit optionsChanged();
}

void QQmlGrpcHttp2Channel::updateChannel()
{
    if (m_channel && m_options)
        m_channel.reset();
    m_channel = std::make_shared<QGrpcHttp2Channel>(m_options->options());
    emit channelUpdated();
}

QT_END_NAMESPACE
