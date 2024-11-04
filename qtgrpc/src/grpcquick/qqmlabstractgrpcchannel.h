// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLABSTRACTGRPCCHANNEL_H
#define QQMLABSTRACTGRPCCHANNEL_H

#include <QtGrpc/qabstractgrpcchannel.h>
#include <QtGrpcQuick/qtgrpcquickexports.h>

#include <QtCore/qobject.h>

#include <memory>

QT_BEGIN_NAMESPACE

class Q_GRPCQUICK_EXPORT QQmlAbstractGrpcChannel : public QObject
{
    Q_OBJECT
public:
    QQmlAbstractGrpcChannel(QObject *parent = nullptr) : QObject(parent) {}
    ~QQmlAbstractGrpcChannel() = default;

    virtual std::shared_ptr<QAbstractGrpcChannel> getChannel() = 0;
};

QT_END_NAMESPACE

#endif // QQMLABSTRACTGRPCCHANNEL_H
