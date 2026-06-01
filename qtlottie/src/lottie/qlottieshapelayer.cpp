// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieshapelayer_p.h"

#include <QJsonObject>
#include <QJsonArray>


#include "qlottieconstants_p.h"
#include "qlottiebase_p.h"
#include "qlottieshape_p.h"
#include "qlottietrimpath_p.h"
#include "qlottiebasictransform_p.h"
#include "qlottierenderer_p.h"

QT_BEGIN_NAMESPACE

QLottieShapeLayer::QLottieShapeLayer(const QLottieShapeLayer &other)
    : QLottieLayer(other)
{
    m_appliedTrim = other.m_appliedTrim;
}

QLottieShapeLayer::QLottieShapeLayer(const QJsonObject &definition)
{
    m_type = LOTTIE_LAYER_SHAPE_IX;

    QLottieLayer::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieShapeLayer::QLottieShapeLayer()" << m_name;

    QJsonArray items = definition.value(QLatin1String("shapes")).toArray();
    QJsonArray::const_iterator itemIt = items.constEnd();
    while (itemIt != items.constBegin()) {
        itemIt--;
        QLottieShape *shape = QLottieShape::construct((*itemIt).toObject(), this);
        if (shape)
            appendChild(shape);
    }
}

QLottieBase *QLottieShapeLayer::clone() const
{
    return new QLottieShapeLayer(*this);
}

void QLottieShapeLayer::updateProperties(int frame)
{
    QLottieLayer::updateProperties(frame);

    for (QLottieBase *child : children()) {
        if (child->hidden())
            continue;

        QLottieShape *shape = dynamic_cast<QLottieShape*>(child);

        if (!shape)
            continue;

        if (shape->type() == LOTTIE_SHAPE_TRIM_IX) {
            QLottieTrimPath *trim = static_cast<QLottieTrimPath*>(shape);
            if (m_appliedTrim)
                m_appliedTrim->applyTrim(*trim);
            else
                m_appliedTrim = trim;
        } else if (m_appliedTrim) {
            if (shape->acceptsTrim())
                shape->applyTrim(*m_appliedTrim);
        }
    }
}

void QLottieShapeLayer::render(QLottieRenderer &renderer) const
{
    if (!m_isActive)
        return;
    renderer.saveState();

    QLottieLayer::render(renderer);

    if (m_appliedTrim && !m_appliedTrim->hidden())
        m_appliedTrim->render(renderer);

    renderer.finish(*this);
    renderer.restoreState();
}

QT_END_NAMESPACE
