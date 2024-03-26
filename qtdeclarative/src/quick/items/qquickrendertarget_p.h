// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRENDERTARGET_P_H
#define QQUICKRENDERTARGET_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include "qquickrendertarget.h"
#include <QAtomicInt>

QT_BEGIN_NAMESPACE

class QRhi;
class QQuickWindowRenderTarget;

class Q_QUICK_PRIVATE_EXPORT QQuickRenderTargetPrivate
{
public:
    static QQuickRenderTargetPrivate *get(QQuickRenderTarget *rt) { return rt->d; }
    static const QQuickRenderTargetPrivate *get(const QQuickRenderTarget *rt) { return rt->d; }
    QQuickRenderTargetPrivate();
    QQuickRenderTargetPrivate(const QQuickRenderTargetPrivate &other);
    bool resolve(QRhi *rhi, QQuickWindowRenderTarget *dst);

    enum class Type {
        Null,
        NativeTexture,
        NativeRenderbuffer,
        RhiRenderTarget,
        PaintDevice
    };

    QAtomicInt ref;
    Type type = Type::Null;
    QSize pixelSize;
    qreal devicePixelRatio = 1.0;
    int sampleCount = 1;
    struct NativeTexture {
        quint64 object;
        int layoutOrState;
        uint rhiFormat;
        uint rhiFlags;
    };
    union {
        NativeTexture nativeTexture;
        quint64 nativeRenderbufferObject;
        QRhiRenderTarget *rhiRt;
        QPaintDevice *paintDevice;
    } u;

    bool mirrorVertically = false;
};

QT_END_NAMESPACE

#endif // QQUICKRENDERTARGET_P_H
