// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKPAINTEDITEM_P_P_H
#define QQUICKPAINTEDITEM_P_P_H

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

#include "qquickpainteditem.h"
#include "qquickitem_p.h"
#include "qquickpainteditem.h"
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QQuickPaintedItemTextureProvider;
class QSGPainterNode;

class Q_QUICK_PRIVATE_EXPORT QQuickPaintedItemPrivate : public QQuickItemPrivate
{
public:
    QQuickPaintedItemPrivate();

    QSize contentsSize;
    qreal contentsScale;
    QColor fillColor;
    QQuickPaintedItem::RenderTarget renderTarget;
    QQuickPaintedItem::PerformanceHints performanceHints;
    QSize textureSize;

    QRect dirtyRect;

    bool opaquePainting: 1;
    bool antialiasing: 1;
    bool mipmap: 1;

    mutable QQuickPaintedItemTextureProvider *textureProvider;
    QSGPainterNode *node;
};

QT_END_NAMESPACE

#endif // QQUICKPAINTEDITEM_P_P_H
