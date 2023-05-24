// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LOTTIERENDERER_H
#define LOTTIERENDERER_H

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

#include <QStack>
#include <private/qglobal_p.h>

#include "bmglobal.h"

QT_BEGIN_NAMESPACE

class BMBase;
class BMLayer;
class BMRect;
class BMFill;
class BMGFill;
class BMImage;
class BMStroke;
class BMBasicTransform;
class BMLayerTransform;
class BMShapeTransform;
class BMRepeaterTransform;
class BMShapeLayer;
class BMEllipse;
class BMRound;
class BMFreeFormShape;
class BMTrimPath;
class BMFillEffect;
class BMRepeater;

class BODYMOVIN_EXPORT LottieRenderer
{
public:
    enum TrimmingState{Off = 0, Simultaneous, Individual};

    virtual ~LottieRenderer() = default;

    virtual void saveState() = 0;
    virtual void restoreState() = 0;

    virtual void setTrimmingState(TrimmingState state);
    virtual TrimmingState trimmingState() const;

    virtual void render(const BMLayer &layer) = 0;
    virtual void render(const BMRect &rect) = 0;
    virtual void render(const BMEllipse &ellipse) = 0;
    virtual void render(const BMRound &round) = 0;
    virtual void render(const BMFill &fill) = 0;
    virtual void render(const BMGFill &fill) = 0;
    virtual void render(const BMImage &image) = 0;
    virtual void render(const BMStroke &stroke) = 0;
    virtual void render(const BMBasicTransform &trans) = 0;
    virtual void render(const BMShapeTransform &trans) = 0;
    virtual void render(const BMFreeFormShape &shape) = 0;
    virtual void render(const BMTrimPath &trans) = 0;
    virtual void render(const BMFillEffect &effect) = 0;
    virtual void render(const BMRepeater &repeater) = 0;

protected:
    void saveTrimmingState();
    void restoreTrimmingState();

    TrimmingState m_trimmingState = Off;

private:
    QStack<LottieRenderer::TrimmingState> m_trimStateStack;
};

QT_END_NAMESPACE

#endif // LOTTIERENDERER_H
