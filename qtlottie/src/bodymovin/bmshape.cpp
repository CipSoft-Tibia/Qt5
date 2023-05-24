// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmshape_p.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

#include "bmgroup_p.h"
#include "bmfill_p.h"
#include "bmgfill_p.h"
#include "bmstroke_p.h"
#include "bmrect_p.h"
#include "bmellipse_p.h"
#include "bmround_p.h"
#include "bmtrimpath_p.h"
#include "bmshapetransform_p.h"
#include "bmfreeformshape_p.h"
#include "bmrepeater_p.h"
#include "bmconstants_p.h"

QT_BEGIN_NAMESPACE

BMShape::BMShape(const BMShape &other)
    : BMBase(other)
{
    m_direction = other.m_direction;
    m_path = other.m_path;
    m_appliedTrim = other.m_appliedTrim;
}

BMBase *BMShape::clone() const
{
    return new BMShape(*this);
}

BMShape *BMShape::construct(QJsonObject definition, const QVersionNumber &version, BMBase *parent)
{
    qCDebug(lcLottieQtBodymovinParser) << "BMShape::construct()";

    BMShape *shape = nullptr;
    const QByteArray type = definition.value(QLatin1String("ty")).toString().toLatin1();

    if (Q_UNLIKELY(type.size() != 2)) {
        qCWarning(lcLottieQtBodymovinParser) << "Unsupported shape type:"
                                             << type;
        return shape;
    }

#define BM_SHAPE_TAG(c1, c2) int((quint32(c1)<<8) | quint32(c2))

    int typeToBuild = BM_SHAPE_TAG(type[0], type[1]);

    switch (typeToBuild) {
    case BM_SHAPE_TAG('g', 'r'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse group";
        shape = new BMGroup(definition, version, parent);
        shape->setType(BM_SHAPE_GROUP_IX);
        break;
    }
    case BM_SHAPE_TAG('r', 'c'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse m_rect";
        shape = new BMRect(definition, version, parent);
        shape->setType(BM_SHAPE_RECT_IX);
        break;
    }
    case BM_SHAPE_TAG('f', 'l'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse fill";
        shape = new BMFill(definition, version, parent);
        shape->setType(BM_SHAPE_FILL_IX);
        break;
    }
    case BM_SHAPE_TAG('g', 'f'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse group fill";
        shape = new BMGFill(definition, version, parent);
        shape->setType(BM_SHAPE_GFILL_IX);
        break;
    }
    case BM_SHAPE_TAG('s', 't'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse stroke";
        shape = new BMStroke(definition, version, parent);
        shape->setType(BM_SHAPE_STROKE_IX);
        break;
    }
    case BM_SHAPE_TAG('t', 'r'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse shape transform";
        shape = new BMShapeTransform(definition, version, parent);
        shape->setType(BM_SHAPE_TRANS_IX);
        break;
    }
    case BM_SHAPE_TAG('e', 'l'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse ellipse";
        shape = new BMEllipse(definition, version, parent);
        shape->setType(BM_SHAPE_ELLIPSE_IX);
        break;
    }
    case BM_SHAPE_TAG('r', 'd'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse round";
        shape = new BMRound(definition, version, parent);
        shape->setType(BM_SHAPE_ROUND_IX);
        break;
    }
    case BM_SHAPE_TAG('s', 'h'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse shape";
        shape = new BMFreeFormShape(definition, version, parent);
        shape->setType(BM_SHAPE_SHAPE_IX);
        break;
    }
    case BM_SHAPE_TAG('t', 'm'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse trim path";
        shape = new BMTrimPath(definition, version, parent);
        shape->setType(BM_SHAPE_TRIM_IX);
        break;
    }
    case BM_SHAPE_TAG('r', 'p'):
    {
        qCDebug(lcLottieQtBodymovinParser) << "Parse trim path";
        shape = new BMRepeater(definition, version, parent);
        shape->setType(BM_SHAPE_REPEATER_IX);
        break;
    }
    case BM_SHAPE_TAG('g', 's'): // ### BM_SHAPE_GSTROKE_IX
    case BM_SHAPE_TAG('s', 'r'): // ### BM_SHAPE_STAR_IX
        // fall through
    default:
        qCWarning(lcLottieQtBodymovinParser) << "Unsupported shape type:"
                                             << type;
    }

#undef BM_SHAPE_TAG

    return shape;
}

bool BMShape::acceptsTrim() const
{
    return false;
}

void BMShape::applyTrim(const BMTrimPath &trimmer)
{
    if (trimmer.simultaneous())
        m_path = trimmer.trim(m_path);
}

int BMShape::direction() const
{
    return m_direction;
}

const QPainterPath &BMShape::path() const
{
    return m_path;
}

QT_END_NAMESPACE
