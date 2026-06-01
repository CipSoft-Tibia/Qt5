// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottielayer_p.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QtCore/QScopedValueRollback>

#include "qlottieflatlayers_p.h"
#include "qlottieshapelayer_p.h"
#include "qlottieprecomplayer_p.h"
#include "qlottiefilleffect_p.h"
#include "qlottiebasictransform_p.h"
#include "qlottierenderer_p.h"

QT_BEGIN_NAMESPACE

QLottieLayer::QLottieLayer(const QLottieLayer &other)
    : QLottieBase(other)
{
    m_layerIndex = other.m_layerIndex;
    m_startFrame = other.m_startFrame;
    m_endFrame = other.m_endFrame;
    m_startTime = other.m_startTime;
    m_blendMode = other.m_blendMode;
    m_3dLayer = other.m_3dLayer;
    m_stretch = other.m_stretch;
    m_hasLinkedLayer = other.m_hasLinkedLayer;
    m_linkedLayerId = other.m_linkedLayerId;
    m_td = other.m_td;
    m_clipMode = other.m_clipMode;
    if (other.m_layerTransform) {
        m_layerTransform = new QLottieBasicTransform(*other.m_layerTransform);
        m_layerTransform->setParent(this);
    }
    m_size = other.m_size;
    if (other.m_effects) {
        m_effects = new QLottieBase;
        for (QLottieBase *effect : other.m_effects->children())
            m_effects->appendChild(effect->clone());
    }
    //m_transformAtFirstFrame = other.m_transformAtFirstFrame;
}

QLottieLayer::~QLottieLayer()
{
    if (m_layerTransform)
        delete m_layerTransform;
    if (m_effects)
        delete m_effects;
}

QLottieBase *QLottieLayer::clone() const
{
    return new QLottieLayer(*this);
}

QLottieLayer *QLottieLayer::construct(QJsonObject definition, const QMap<QString, QJsonObject> &assets)
{
    qCDebug(lcLottieQtLottieParser) << "QLottieLayer::construct()";

    QLottieLayer *layer = nullptr;
    int type = definition.value(QLatin1String("ty")).toInt();
    switch (type) {
    case 0:
        qCDebug(lcLottieQtLottieParser) << "Parse precomp layer";
        layer = new QLottiePrecompLayer(definition, assets);
        break;
    case 1:
        qCDebug(lcLottieQtLottieParser) << "Parse solid layer";
        layer = new QLottieSolidLayer(definition);
        break;
    case 2:
        qCDebug(lcLottieQtLottieParser) << "Parse image layer";
        layer = new QLottieImageLayer(definition);
        break;
    case 3:
        qCDebug(lcLottieQtLottieParser) << "Parse null layer";
        layer = new QLottieNullLayer(definition);
        break;
    case 4:
        qCDebug(lcLottieQtLottieParser) << "Parse shape layer";
        layer = new QLottieShapeLayer(definition);
        break;
    default:
        qCInfo(lcLottieQtLottieParser) << "Unsupported layer type:" << type;
    }
    return layer;
}

// Take the content of a lottie layers tag and construct the corresponding layer objects
// Also adds them as children to given parent
int QLottieLayer::constructLayers(QJsonArray jsonLayers, QLottieBase *parent,
                                  const QMap<QString, QJsonObject> &assets)
{
    int layersAdded = 0;
    QJsonArray::const_iterator jsonLayerIt = jsonLayers.constEnd();
    while (jsonLayerIt != jsonLayers.constBegin()) {
        jsonLayerIt--;
        QJsonObject jsonLayer = (*jsonLayerIt).toObject();
        if (jsonLayer.value(QLatin1String("ty")).toInt() == 2) {
            QString refId = jsonLayer.value(QLatin1String("refId")).toString();
            jsonLayer.insert(QLatin1String("asset"), assets.value(refId));
        }
        QLottieLayer *layer = QLottieLayer::construct(jsonLayer, assets);
        if (layer) {
            layer->setParent(parent);
            // Mask layers must be rendered before the layers they affect to
            // although they appear after in layer hierarchy. For this reason
            // move a mask in front of the affected layer, so it will be rendered first
            if (layer->isMaskLayer())
                parent->insertChildBeforeLast(layer);
            else
                parent->appendChild(layer);
            layersAdded++;
        }
    }
    return layersAdded;
}

bool QLottieLayer::active(int frame) const
{
    return (!m_hidden && ((frame >= m_startFrame && frame <= m_endFrame) || isStructureDumping()));
}

void QLottieLayer::parse(const QJsonObject &definition)
{
    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieLayer::parse():" << m_name;

    m_layerIndex = definition.value(QLatin1String("ind")).toVariant().toInt();
    m_startFrame = definition.value(QLatin1String("ip")).toVariant().toInt();
    m_endFrame = definition.value(QLatin1String("op")).toVariant().toInt();
    m_blendMode = definition.value(QLatin1String("lottie")).toVariant().toInt();
    m_autoOrient = definition.value(QLatin1String("ao")).toBool();
    m_3dLayer = definition.value(QLatin1String("ddd")).toBool();
    m_stretch = definition.value(QLatin1String("sr")).toVariant().toReal();
    m_linkedLayerId = definition.value(QLatin1String("parent")).toVariant().toInt(&m_hasLinkedLayer);
    m_td = definition.value(QLatin1String("td")).toInt();
    int clipMode = definition.value(QLatin1String("tt")).toInt(-1);
    if (clipMode > -1 && clipMode < 5)
        m_clipMode = static_cast<MatteClipMode>(clipMode);

    QJsonObject trans = definition.value(QLatin1String("ks")).toObject();
    m_layerTransform = new QLottieBasicTransform(trans, this);

    QJsonArray effects = definition.value(QLatin1String("ef")).toArray();
    parseEffects(effects);

    if (m_clipMode > 2)
        qCInfo(lcLottieQtLottieParser)
                << "Lottie Layer: Only alpha mask layer supported:" << m_clipMode;
    if (m_blendMode > 0)
        qCInfo(lcLottieQtLottieParser)
                << "Lottie Layer: Unsupported blend mode" << m_blendMode;
    if (m_stretch > 1)
        qCInfo(lcLottieQtLottieParser)
                << "Lottie Layer: stretch not supported" << m_stretch;
    if (m_autoOrient)
        qCInfo(lcLottieQtLottieParser)
                << "Lottie Layer: auto-orient not supported";
    if (m_3dLayer)
        qCInfo(lcLottieQtLottieParser)
                << "Lottie Layer: is a 3D layer, but not handled";
}

void QLottieLayer::updateProperties(int frame)
{
    if (m_hasLinkedLayer)
        resolveLinkedLayer();

    int adjFrame = frame - m_startTime;
    m_isActive = active(adjFrame);
    if (!m_isActive)
        return;

    // Update first effects, as they are not children of the layer
    if (m_effects) {
        for (QLottieBase* effect : m_effects->children())
            effect->updateProperties(adjFrame);
    }

    m_layerTransform->updateProperties(adjFrame);

    QLottieBase::updateProperties(adjFrame);
}

void QLottieLayer::render(QLottieRenderer &renderer) const
{
    if (!m_isActive)
        return;

    // Render first effects, as they affect the children
    renderEffects(renderer);

    // In case there is a linked layer, apply its transform first
    // as it affects tranforms of this layer too
    applyLayerTransform(renderer);

    renderer.render(*this);

    for (QLottieBase *child : children()) {
        if (child->hidden())
            continue;
        child->render(renderer);
    }
}

QLottieBase *QLottieLayer::findChild(const QString &childName)
{
    QLottieBase *child = nullptr;

    if (m_effects)
        child = m_effects->findChild(childName);

    if (child)
        return child;
    else
        return QLottieBase::findChild(childName);
}

QLottieLayer *QLottieLayer::resolveLinkedLayer()
{
    if (m_linkedLayer)
        return m_linkedLayer;

    Q_ASSERT(parent());

    for (QLottieBase *child : parent()->children()) {
        QLottieLayer *layer = static_cast<QLottieLayer*>(child);
        if (layer->layerId() == m_linkedLayerId) {
            m_linkedLayer = layer;
            break;
        }
    }
    return m_linkedLayer;
}

QLottieLayer *QLottieLayer::linkedLayer() const
{
    return m_linkedLayer;
}

bool QLottieLayer::isClippedLayer() const
{
    return m_clipMode != NoClip;
}

bool QLottieLayer::isMaskLayer() const
{
    return m_td > 0;
}

QLottieLayer::MatteClipMode QLottieLayer::clipMode() const
{
    return m_clipMode;
}

int QLottieLayer::layerId() const
{
    return m_layerIndex;
}

QLottieBasicTransform *QLottieLayer::transform() const
{
    return m_layerTransform;
}

void QLottieLayer::renderEffects(QLottieRenderer &renderer) const
{
    if (!m_effects)
        return;

    for (QLottieBase* effect : m_effects->children()) {
        if (effect->hidden())
            continue;
        effect->render(renderer);
    }
}

void QLottieLayer::applyLayerTransform(QLottieRenderer &renderer) const
{
    if (m_applyingLayerTransform)
        return;
    QScopedValueRollback<bool> recursionGuard(m_applyingLayerTransform, true);

    if (!isStructureDumping()) {
        if (QLottieLayer *ll = linkedLayer())
            ll->applyLayerTransform(renderer);
    }
    if (m_layerTransform)
        m_layerTransform->render(renderer); // TBD: except opacity
}

QSize QLottieLayer::size() const
{
    return m_size;
}

const QLottieLayer *QLottieLayer::checkedCast(const QLottieBase *node)
{
    const QLottieLayer *res = nullptr;
    if (node && node->type() >= LOTTIE_LAYER_PRECOMP_IX && node->type() <= LOTTIE_LAYER_TEXT_IX)
        res = static_cast<const QLottieLayer *>(node);
    return res;
}

void QLottieLayer::parseEffects(const QJsonArray &definition, QLottieBase *effectRoot)
{
    QJsonArray::const_iterator it = definition.constEnd();
    while (it != definition.constBegin()) {
        // Create effects container if at least one effect found
        if (!m_effects) {
            m_effects = new QLottieBase;
            effectRoot = m_effects;
        }
        it--;
        QJsonObject effect = (*it).toObject();
        int type = effect.value(QLatin1String("ty")).toInt();
        switch (type) {
        case 0:
        {
            QLottieBase *slider = new QLottieBase;
            slider->parse(effect);
            effectRoot->appendChild(slider);
            break;
        }
        case 5:
        {
            if (effect.value(QLatin1String("en")).toInt()) {
                QLottieBase *group = new QLottieBase;
                group->parse(effect);
                effectRoot->appendChild(group);
                parseEffects(effect.value(QLatin1String("ef")).toArray(), group);
            }
            break;
        }
        case 21:
        {
            QLottieFillEffect *fill = new QLottieFillEffect;
            fill->construct(effect);
            effectRoot->appendChild(fill);
            break;
        }
        default:
            qCInfo(lcLottieQtLottieParser)
                << "QLottieLayer: Unsupported effect" << type;
        }
    }
}

QT_END_NAMESPACE
