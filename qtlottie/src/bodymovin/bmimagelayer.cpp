// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmimagelayer_p.h"

#include <QJsonObject>
#include <QJsonArray>


#include "bmbase_p.h"
#include "bmimage_p.h"
#include "bmshape_p.h"
#include "bmtrimpath_p.h"
#include "bmbasictransform_p.h"
#include "lottierenderer_p.h"

QT_BEGIN_NAMESPACE

BMImageLayer::BMImageLayer(const BMImageLayer &other)
    : BMLayer(other)
{
    m_maskProperties = other.m_maskProperties;
    m_layerTransform = new BMBasicTransform(*other.m_layerTransform);
    m_appliedTrim = other.m_appliedTrim;
}

BMImageLayer::BMImageLayer(const QJsonObject &definition, const QVersionNumber &version)
{
    m_type = BM_LAYER_IMAGE_IX;
    m_version = version;

    BMLayer::parse(definition);

    BMImage *image = new BMImage(definition, version, this);
    appendChild(image);

    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMImageLayer::BMImageLayer()"
                                       << m_name;

    QJsonArray maskProps = definition.value(QLatin1String("maskProperties")).toArray();
    QJsonArray::const_iterator propIt = maskProps.constBegin();
    while (propIt != maskProps.constEnd()) {
        m_maskProperties.append((*propIt).toVariant().toInt());
        ++propIt;
    }

    QJsonObject trans = definition.value(QLatin1String("ks")).toObject();
    m_layerTransform = new BMBasicTransform(trans, version, this);

    QJsonArray items = definition.value(QLatin1String("shapes")).toArray();
    QJsonArray::const_iterator itemIt = items.constEnd();
    while (itemIt != items.constBegin()) {
        itemIt--;
        BMShape *shape = BMShape::construct((*itemIt).toObject(), version, this);
        if (shape)
            appendChild(shape);
    }

    if (m_maskProperties.size())
        qCWarning(lcLottieQtBodymovinParser)
            << "BM Image Layer: mask properties found, but not supported"
            << m_maskProperties;
}

BMImageLayer::~BMImageLayer()
{
    if (m_layerTransform)
        delete m_layerTransform;
}

BMBase *BMImageLayer::clone() const
{
    return new BMImageLayer(*this);
}

void BMImageLayer::updateProperties(int frame)
{
    BMLayer::updateProperties(frame);

    m_layerTransform->updateProperties(frame);

    for (BMBase *child : children()) {
        if (child->hidden())
            continue;

        BMShape *shape = dynamic_cast<BMShape*>(child);

        if (!shape)
            continue;

        if (shape->type() == BM_SHAPE_TRIM_IX) {
            BMTrimPath *trim = static_cast<BMTrimPath*>(shape);
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

void BMImageLayer::render(LottieRenderer &renderer) const
{
    renderer.saveState();

    renderEffects(renderer);

    // In case there is a linked layer, apply its transform first
    // as it affects transforms of this layer too
    if (BMLayer *ll = linkedLayer())
        renderer.render(*ll->transform());

    renderer.render(*this);

    m_layerTransform->render(renderer);

    for (BMBase *child : children()) {
        if (child->hidden())
            continue;
        child->render(renderer);
    }

    if (m_appliedTrim && !m_appliedTrim->hidden())
        m_appliedTrim->render(renderer);

    renderer.restoreState();}

QT_END_NAMESPACE
