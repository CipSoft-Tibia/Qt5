// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmpathtrimmer_p.h"

#include "bmtrimpath_p.h"
#include "lottierenderer_p.h"

#include <QPainterPath>

QT_BEGIN_NAMESPACE

BMPathTrimmer::BMPathTrimmer(BMBase *root)
    : m_root(root)
{
    Q_ASSERT(m_root);
}

void BMPathTrimmer::addTrim(BMTrimPath* trim)
{
    if (!trim)
        return;

    m_trimPaths.append(trim);

    if (!m_appliedTrim)
        m_appliedTrim = trim;
    else
        qCWarning(lcLottieQtBodymovinParser)
            << "BM Shape Layer: more than one trim path found on the layer."
            << "Only one (the first encountered) is supported";
}

bool BMPathTrimmer::inUse() const
{
    return !m_trimPaths.isEmpty();
}

void BMPathTrimmer::applyTrim(BMShape *shape)
{
    if (!m_appliedTrim)
        return;
    shape->applyTrim(*m_appliedTrim);
}

void BMPathTrimmer::updateProperties(int frame)
{
    QPainterPath unifiedPath;

    if (m_appliedTrim)
        m_appliedTrim->updateProperties(frame);

//    for (BMBase *child : m_root->children()) {
//        // TODO: Create a better system for recognizing types
//        if (child->type() >= 1000)
//            continue;

//        BMShape *shape = static_cast<BMShape*>(child);

//        // TODO: Get a better way to inherit trimming
//        if (shape->type() == BM_SHAPE_GROUP_IX && m_appliedTrim)
//            shape->applyTrim(*m_appliedTrim);

//        shape->updateProperties(frame);

//        if (m_appliedTrim && shape->acceptsTrim())
//            shape->applyTrim(*m_appliedTrim);
//    }
}

void BMPathTrimmer::render(LottieRenderer &renderer) const
{
    Q_UNUSED(renderer);
//    if (m_appliedTrim) {
//        renderer.render(*m_appliedTrim);
//    }
}

QT_END_NAMESPACE
