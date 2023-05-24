// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRPCMETADATA_H
#define QGRPCMETADATA_H

#include <QtGrpc/qtgrpcglobal.h>

#include <unordered_map>

#if 0
// Create a forwarding header
#pragma qt_class(QGrpcMetadata)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

using QGrpcMetadata = std::unordered_multimap<QByteArray, QByteArray>;

QT_END_NAMESPACE

#endif // QGRPCMETADATA_H
