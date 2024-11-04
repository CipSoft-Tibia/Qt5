// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLGRPCHTTP2CHANNEL_H
#define QQMLGRPCHTTP2CHANNEL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGrpcQuick/qtgrpcquickexports.h>
#include <QtGrpcQuick/qqmlabstractgrpcchannel.h>
#include <QtGrpcQuick/private/qqmlgrpcchanneloptions_p.h>

#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE

class Q_GRPCQUICK_EXPORT QQmlGrpcHttp2Channel : public QQmlAbstractGrpcChannel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GrpcHttp2Channel)
    QML_ADDED_IN_VERSION(6, 7)
    Q_PROPERTY(QQmlGrpcChannelOptions *options READ options
                       WRITE setOptions NOTIFY optionsChanged REQUIRED)
    Q_PROPERTY(std::shared_ptr<QAbstractGrpcChannel> channel READ getChannel NOTIFY channelUpdated)

public:
    QQmlGrpcHttp2Channel(QObject *parent = nullptr);
    ~QQmlGrpcHttp2Channel();
    std::shared_ptr<QAbstractGrpcChannel> getChannel() final;
    QQmlGrpcChannelOptions *options() const { return m_options; }
    void setOptions(QQmlGrpcChannelOptions *options);

signals:
    void optionsChanged();
    void channelUpdated();

private:
    void updateChannel();

    QQmlGrpcChannelOptions *m_options;
    std::shared_ptr<QAbstractGrpcChannel> m_channel;
};

QT_END_NAMESPACE

#endif // QQMLGRPCHTTP2CHANNEL_H
