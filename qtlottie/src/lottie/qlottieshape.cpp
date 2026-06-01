// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieshape_p.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

#include "qlottiegroup_p.h"
#include "qlottiefill_p.h"
#include "qlottiegfill_p.h"
#include "qlottiestroke_p.h"
#include "qlottierect_p.h"
#include "qlottieellipse_p.h"
#include "qlottiepolystar_p.h"
#include "qlottieround_p.h"
#include "qlottietrimpath_p.h"
#include "qlottieshapetransform_p.h"
#include "qlottiefreeformshape_p.h"
#include "qlottierepeater_p.h"
#include "qlottieconstants_p.h"

QT_BEGIN_NAMESPACE

QLottieShape::QLottieShape(const QLottieShape &other)
    : QLottieBase(other)
{
    m_direction = other.m_direction;
    m_path = other.m_path;
    m_appliedTrim = other.m_appliedTrim;
}

QLottieBase *QLottieShape::clone() const
{
    return new QLottieShape(*this);
}

QLottieShape *QLottieShape::construct(QJsonObject definition, QLottieBase *parent)
{
    qCDebug(lcLottieQtLottieParser) << "QLottieShape::construct()";

    QLottieShape *shape = nullptr;
    const QByteArray type = definition.value(QLatin1String("ty")).toString().toLatin1();

    if (Q_UNLIKELY(type.size() != 2)) {
        qCInfo(lcLottieQtLottieParser) << "Unsupported shape type:"
                                             << type;
        return shape;
    }

#define LOTTIE_SHAPE_TAG(c1, c2) int((quint32(c1)<<8) | quint32(c2))

    int typeToBuild = LOTTIE_SHAPE_TAG(type[0], type[1]);

    switch (typeToBuild) {
    case LOTTIE_SHAPE_TAG('g', 'r'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse group";
        shape = new QLottieGroup(definition, parent);
        shape->setType(LOTTIE_SHAPE_GROUP_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('r', 'c'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse m_rect";
        shape = new QLottieRect(definition, parent);
        shape->setType(LOTTIE_SHAPE_RECT_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('f', 'l'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse fill";
        shape = new QLottieFill(definition, parent);
        shape->setType(LOTTIE_SHAPE_FILL_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('g', 'f'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse group fill";
        shape = new QLottieGFill(definition, parent);
        shape->setType(LOTTIE_SHAPE_GFILL_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('s', 't'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse stroke";
        shape = new QLottieStroke(definition, parent);
        shape->setType(LOTTIE_SHAPE_STROKE_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('t', 'r'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse shape transform";
        shape = new QLottieShapeTransform(definition, parent);
        shape->setType(LOTTIE_SHAPE_TRANS_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('e', 'l'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse ellipse";
        shape = new QLottieEllipse(definition, parent);
        shape->setType(LOTTIE_SHAPE_ELLIPSE_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('s', 'r'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse polystar";
        shape = new QLottiePolyStar(definition, parent);
        shape->setType(LOTTIE_SHAPE_STAR_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('r', 'd'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse round";
        shape = new QLottieRound(definition, parent);
        shape->setType(LOTTIE_SHAPE_ROUND_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('s', 'h'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse shape";
        shape = new QLottieFreeFormShape(definition, parent);
        shape->setType(LOTTIE_SHAPE_SHAPE_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('t', 'm'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse trim path";
        shape = new QLottieTrimPath(definition, parent);
        shape->setType(LOTTIE_SHAPE_TRIM_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('r', 'p'):
    {
        qCDebug(lcLottieQtLottieParser) << "Parse trim path";
        shape = new QLottieRepeater(definition, parent);
        shape->setType(LOTTIE_SHAPE_REPEATER_IX);
        break;
    }
    case LOTTIE_SHAPE_TAG('g', 's'): // ### LOTTIE_SHAPE_GSTROKE_IX
        // fall through
    default:
        qCInfo(lcLottieQtLottieParser) << "Unsupported shape type:"
                                             << type;
    }

#undef LOTTIE_SHAPE_TAG

    return shape;
}

bool QLottieShape::acceptsTrim() const
{
    return false;
}

void QLottieShape::applyTrim(const QLottieTrimPath &trimmer)
{
    if (trimmer.isParallel())
        m_path = trimmer.trim(m_path);
}

int QLottieShape::direction() const
{
    return m_direction;
}

const QPainterPath &QLottieShape::path() const
{
    return m_path;
}

QT_END_NAMESPACE
