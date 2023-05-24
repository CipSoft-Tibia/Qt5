// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QUTILS_H
#define QUTILS_H

#include <QtGui/QSurfaceFormat>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

[[maybe_unused]]
static inline QSurfaceFormat qDefaultSurfaceFormat(bool antialias)
{
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setDepthBufferSize(24);
    surfaceFormat.setStencilBufferSize(8);
    surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    surfaceFormat.setRenderableType(QSurfaceFormat::DefaultRenderableType);
    if (antialias)
        surfaceFormat.setSamples(8);
    else
        surfaceFormat.setSamples(0);

    return surfaceFormat;
}

QT_END_NAMESPACE

#endif

