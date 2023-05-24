// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMSHAPE_P_H
#define BMSHAPE_P_H

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

#include <QtBodymovin/private/bmbase_p.h>
#include <QtBodymovin/private/bmproperty_p.h>

QT_BEGIN_NAMESPACE

class BMFill;
class BMStroke;
class BMTrimPath;

#define BM_SHAPE_ANY_TYPE_IX    -1
#define BM_SHAPE_ELLIPSE_IX     0
#define BM_SHAPE_FILL_IX        1
#define BM_SHAPE_GFILL_IX       2
#define BM_SHAPE_GSTROKE_IX     3
#define BM_SHAPE_GROUP_IX       4
#define BM_SHAPE_RECT_IX        5
#define BM_SHAPE_ROUND_IX       6
#define BM_SHAPE_SHAPE_IX       7
#define BM_SHAPE_STAR_IX        8
#define BM_SHAPE_STROKE_IX      9
#define BM_SHAPE_TRIM_IX        10
#define BM_SHAPE_TRANS_IX       11
#define BM_SHAPE_REPEATER_IX    12

class BODYMOVIN_EXPORT BMShape : public BMBase
{
public:
    BMShape() = default;
    explicit BMShape(const BMShape &other);

    BMBase *clone() const override;

    static BMShape *construct(QJsonObject definition, const QVersionNumber &version,
                              BMBase *parent = nullptr);

    virtual const QPainterPath &path() const;
    virtual bool acceptsTrim() const;
    virtual void applyTrim(const BMTrimPath& trimmer);

    int direction() const;

protected:
    QPainterPath m_path;
    BMTrimPath *m_appliedTrim = nullptr;
    int m_direction = 0;
};

QT_END_NAMESPACE

#endif // BMSHAPE_P_H
