// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTGRPCCHANNEL_P_H
#define QABSTRACTGRPCCHANNEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGrpc/qgrpcchanneloptions.h>

#include <QtCore/qnamespace.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

class QAbstractGrpcChannelPrivate
{
public:
    explicit QAbstractGrpcChannelPrivate(const QGrpcChannelOptions &options)
        : threadId(QThread::currentThreadId()), channelOptions(options)
    {
    }
    const Qt::HANDLE threadId;
    QGrpcChannelOptions channelOptions;
};

QT_END_NAMESPACE

#endif // QABSTRACTGRPCCHANNEL_P_H
