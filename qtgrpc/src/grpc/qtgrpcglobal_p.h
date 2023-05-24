// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTGRPCGLOBAL_P_H
#define QTGRPCGLOBAL_P_H

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

#include <QtGrpc/qtgrpcglobal.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(Grpc)
QT_END_NAMESPACE

#define qGrpcDebug(...) qCDebug(Grpc, __VA_ARGS__)
#define qGrpcInfo(...) qCInfo(Grpc, __VA_ARGS__)
#define qGrpcWarning(...) qCWarning(Grpc, __VA_ARGS__)
#define qGrpcCritical(...) qCCritical(Grpc, __VA_ARGS__)

#endif // QTGRPCGLOBAL_P_H
