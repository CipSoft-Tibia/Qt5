// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCCLIENTINTERCEPTORMANAGER_P_H
#define QGRPCCLIENTINTERCEPTORMANAGER_P_H

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
#include <QtCore/qshareddata.h>

#include <QtGrpc/qgrpcclientinterceptor.h>
#include <QtGrpc/qtgrpcglobal.h>

QT_BEGIN_NAMESPACE

class QGrpcClientInterceptorManagerPrivate
{
public:
    std::vector<std::shared_ptr<QGrpcClientInterceptor>> interceptors;
};

QT_END_NAMESPACE

#endif // QGRPCCLIENTINTERCEPTORMANAGER_P_H
