// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMGLOBAL_H
#define BMGLOBAL_H

#include <QtCore/qglobal.h>

#if defined(BODYMOVIN_LIBRARY)
#  define BODYMOVIN_EXPORT Q_DECL_EXPORT
#else
#  define BODYMOVIN_EXPORT Q_DECL_IMPORT
#endif

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

#endif // BMGLOBAL_H
