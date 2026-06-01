// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QLOTTIESHAPE_P_H
#define QLOTTIESHAPE_P_H

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

#include <QPainterPath>

#include <QtLottie/private/qlottiebase_p.h>
#include <QtLottie/private/qlottieproperty_p.h>

QT_BEGIN_NAMESPACE

class QLottieFill;
class QLottieStroke;
class QLottieTrimPath;

#define LOTTIE_SHAPE_ANY_TYPE_IX    -1
#define LOTTIE_SHAPE_ELLIPSE_IX     0
#define LOTTIE_SHAPE_FILL_IX        1
#define LOTTIE_SHAPE_GFILL_IX       2
#define LOTTIE_SHAPE_GSTROKE_IX     3
#define LOTTIE_SHAPE_GROUP_IX       4
#define LOTTIE_SHAPE_RECT_IX        5
#define LOTTIE_SHAPE_ROUND_IX       6
#define LOTTIE_SHAPE_SHAPE_IX       7
#define LOTTIE_SHAPE_STAR_IX        8
#define LOTTIE_SHAPE_STROKE_IX      9
#define LOTTIE_SHAPE_TRIM_IX        10
#define LOTTIE_SHAPE_TRANS_IX       11
#define LOTTIE_SHAPE_REPEATER_IX    12

class Q_LOTTIE_EXPORT QLottieShape : public QLottieBase
{
public:
    QLottieShape() = default;
    explicit QLottieShape(const QLottieShape &other);

    QLottieBase *clone() const override;

    static QLottieShape *construct(QJsonObject definition, QLottieBase *parent = nullptr);

    virtual const QPainterPath &path() const;
    virtual bool acceptsTrim() const;
    virtual void applyTrim(const QLottieTrimPath& trimmer);
    const QLottieTrimPath *currentTrim() const { return m_appliedTrim; };

    int direction() const;
    bool hasReversedDirection() const { return m_direction == 3; }

protected:
    QPainterPath m_path;
    QLottieTrimPath *m_appliedTrim = nullptr;
    int m_direction = 0;
};

QT_END_NAMESPACE

#endif // QLOTTIESHAPE_P_H
