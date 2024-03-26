// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKIMAGEBASE_P_P_H
#define QQUICKIMAGEBASE_P_P_H

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

#include "qquickimplicitsizeitem_p_p.h"
#include "qquickimagebase_p.h"

#include <QtQuick/private/qquickpixmapcache_p.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;
class Q_QUICK_PRIVATE_EXPORT QQuickImageBasePrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickImageBase)

public:
    QQuickImageBasePrivate()
      : status(QQuickImageBase::Null),
        progress(0.0),
        devicePixelRatio(1.0),
        currentFrame(0),
        frameCount(0),
        async(false),
        cache(true),
        mirrorHorizontally(false),
        mirrorVertically(false),
        oldAutoTransform(false)
    {
    }

    virtual bool updateDevicePixelRatio(qreal targetDevicePixelRatio);

    void setStatus(QQuickImageBase::Status value);
    void setProgress(qreal value);

    QQuickPixmap pix;
    QQuickImageBase::Status status;
    QUrl url;
    qreal progress;
    QSize sourcesize;
    QSize oldSourceSize;
    qreal devicePixelRatio;
    QRectF sourceClipRect;
    QQuickImageProviderOptions providerOptions;
    QColorSpace colorSpace;
    int currentFrame;
    int frameCount;
    bool async : 1;
    bool cache : 1;
    bool mirrorHorizontally: 1;
    bool mirrorVertically : 1;
    bool oldAutoTransform : 1;
};

QT_END_NAMESPACE

#endif // QQUICKIMAGEBASE_P_P_H
