// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottievisitor_p.h"
#include <private/qquickgenerator_p.h>
#include <private/qquicknodeinfo_p.h>
#include <QtLottie/private/qlottieshape_p.h>
#include <QtLottie/private/qlottiestroke_p.h>
#include <QtLottie/private/qlottierect_p.h>
#include <QtLottie/private/qlottiepolystar_p.h>
#include <QtLottie/private/qlottieellipse_p.h>
#include <QtLottie/private/qlottiefreeformshape_p.h>
#include <QtLottie/private/qlottieshapetransform_p.h>
#include <QtLottie/private/qlottiegfill_p.h>
#include <QtLottie/private/qlottieround_p.h>
#include <QtLottie/private/qlottieroot_p.h>
#include <QtLottie/private/qlottieflatlayers_p.h>
#include <QtLottie/private/qlottieimage_p.h>

#include <QtGui/private/qfixed_p.h>

#include <QFile>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcLottieQtVisitor, "qt.lottieqt.visitor")

using namespace Qt::Literals::StringLiterals;

#define QLOTTIEVISITOR_DEBUG \
    qCDebug(lcLottieQtVisitor).noquote().nospace() \
        << QByteArray().fill(' ', m_savedPaintInfos.size() * 4) \
        << ((trimmingState() == Sequential) ? QByteArray("trimmed") : QByteArray{})

QLottieVisitor::QLottieVisitor(const QString lottieFileName, QQuickGenerator *generator)
    : m_lottieFileName(lottieFileName), m_generator(generator)
{
}

bool QLottieVisitor::nodeIsGraphicElement(const QLottieBase *node)
{
    return (node->type() >= LOTTIE_SHAPE_ELLIPSE_IX && node->type() <= LOTTIE_SHAPE_REPEATER_IX);
}

bool QLottieVisitor::nodeIsShape(const QLottieBase *node)
{
    switch (node->type()) {
    case LOTTIE_SHAPE_ELLIPSE_IX: Q_FALLTHROUGH();
    case LOTTIE_SHAPE_RECT_IX: Q_FALLTHROUGH();
    case LOTTIE_SHAPE_SHAPE_IX: Q_FALLTHROUGH();
    case LOTTIE_SHAPE_STAR_IX:
        return true;
        break;
    default:
        break;
    }
    return false;
}

void QLottieVisitor::render(const QLottieRoot &root)
{
    enumerateLayerChildren(&root);

    StructureNodeInfo info;
    const QJsonObject rootObj = root.definition();
    info.size = QSize(rootObj.value("w"_L1).toInt(), rootObj.value("h"_L1).toInt());

    int frameRate = rootObj.value(QLatin1String("fr")).toVariant().toInt();
    if (frameRate > 0)
        m_frameRate = frameRate;

    m_duration = qRound(1000 * rootObj.value("op"_L1).toDouble(100.0) / m_frameRate);
    info.viewBox = QRectF(QPointF(0, 0), info.size);

    QLOTTIEVISITOR_DEBUG << "[root viewbox=" << info.viewBox << ", frame rate=" << m_frameRate << ", duration=" << m_duration << "ms ]";

    info.stage = StructureNodeStage::Start;
    info.nodeId = u"_q_animation"_s; // # centralize

    m_generator->generateRootNode(info);

    root.render(*this);

    info.stage = StructureNodeStage::End;
    m_generator->generateRootNode(info);
}

QString QLottieVisitor::scrub(const QString &raw)
{
    QString res(raw.left(80));

    if (!res.isEmpty()) {
        constexpr QLatin1StringView legalSymbols("_-.:/[](){}*| "); // No quot. mark or backslash!
        qsizetype i = 0;
        do {
            if (res.at(i).isLetterOrNumber() || legalSymbols.contains(res.at(i)))
                i++;
            else
                res.remove(i, 1);
        } while (i < res.size());
    }

    return res;
}

void QLottieVisitor::fillCommonNodeInfo(const QLottieBase *node, NodeInfo *info)
{
    Q_ASSERT(node);
    Q_ASSERT(info);

    info->typeName = QStringLiteral("Type%1").arg(node->type());
}

void QLottieVisitor::fillLayerAnimationInfo(const QLottieLayer *node, NodeInfo *info)
{
    int endTime = qRound(1000 * node->endFrame() / qreal(m_frameRate));
    if (node->startFrame() == 0 && endTime >= m_duration)
        return;

    QQuickAnimatedProperty::PropertyAnimation animation;
    if (node->startFrame() > 0)
        animation.frames[0] = QVariant::fromValue(false);
    animation.frames[qRound(1000 * node->startFrame() / qreal(m_frameRate))] = QVariant::fromValue(true);
    if (endTime < m_duration)
        animation.frames[endTime] = QVariant::fromValue(false);
    animation.frames[m_duration] = animation.frames.last();
    animation.flags |= QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;

    info->visibility.addAnimation(animation);
}

void QLottieVisitor::fillAnimationNodeInfo(const QLottieBase *node, NodeInfo *info)
{
    Q_UNUSED(node);
    for (const PaintInfo::TransformAnimationInfo &animInfo : std::as_const(m_currentPaintInfo.transformAnimations)) {
        QQuickAnimatedProperty::PropertyAnimation animation;
        animation.subtype = animInfo.animationType;
        animation.frames = animInfo.frames;
        animation.flags |= QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;

        if (!animation.frames.isEmpty()) {
            animation.frames[0] = animation.frames.first();
            animation.frames[m_duration] = animation.frames.last();

            animation.easingPerFrame = animInfo.easingPerFrame;

            if (animInfo.animationType == QTransform::TxNone)
                info->opacity.addAnimation(animation);
            else
                info->transform.addAnimation(animation);
        }
    }
}

void QLottieVisitor::saveState()
{
    m_savedPaintInfos.append(m_currentPaintInfo);
}

void QLottieVisitor::restoreState()
{
    Q_ASSERT(!m_savedPaintInfos.isEmpty());
    m_currentPaintInfo = m_savedPaintInfos.takeLast();
}

void QLottieVisitor::render(const QLottieLayer &layer)
{
    QLOTTIEVISITOR_DEBUG << "[layer '" << layer.name() << "' type " << Qt::hex << layer.type() << "]";

    m_frameOffset += layer.frameOffset();

    StructureNodeInfo info;
    info.stage = StructureNodeStage::Start;
    info.nodeId = scrub(layer.name());
    info.transform.setDefaultValue(QVariant::fromValue(m_currentPaintInfo.transform));
    info.isDefaultTransform = m_currentPaintInfo.transform.isIdentity();

    fillCommonNodeInfo(&layer, &info);
    fillAnimationNodeInfo(&layer, &info);
    fillLayerAnimationInfo(&layer, &info);
    info.layerNum = m_layers.indexOf(&layer);
    if (layer.hasLinkedLayer() && layer.parent()) {
        for (const QLottieBase *sibling : layer.parent()->children()) {
            if (auto siblingLayer = QLottieLayer::checkedCast(sibling)) {
                if (siblingLayer != &layer && siblingLayer->layerId() == layer.linkedLayerId())
                    info.transformReferenceLayerNum = m_layers.indexOf(siblingLayer);
            }
        }
        QLOTTIEVISITOR_DEBUG << "  xf link resolved to layernum" << info.transformReferenceLayerNum;
    }

    m_generator->generateStructureNode(info);

    m_currentPaintInfo = {};
}

void QLottieVisitor::render(const QLottieSolidLayer &layer)
{
    render(static_cast<const QLottieLayer &>(layer));
    if (!layer.size().isEmpty()) {
        m_currentPaintInfo.fill = layer.color();
        QPainterPath layerRect;
        layerRect.addRect(QRect(QPoint(), layer.size()));
        processShape(nullptr, layerRect);
    }
}

void QLottieVisitor::render(const QLottieGroup &group)
{
    QLOTTIEVISITOR_DEBUG << "[group '" << group.name() << "' #children " << group.children().size() << "]";

    bool hasGroups = false;
    for (const QLottieBase *child : group.children()) {
        if (child->type() == LOTTIE_SHAPE_GROUP_IX) {
            hasGroups = true;
            break;
        }
    }

    // Child groups have their own transforms, so we need a structure node for correct xf ordering.
    if (hasGroups) {
        // We must apply the group xf already here, so it will become the structure node's xf.
        // If non-identity, m_currentStructElements is used to avoid re-applying it on normal visit.
        const QLottieBase *groupXf = group.children().first();
        if (groupXf->type() == LOTTIE_SHAPE_TRANS_IX) // Always true in wellformed lottie file
            render(*static_cast<const QLottieShapeTransform *>(groupXf));
        const bool groupHasTransform = !m_currentPaintInfo.transform.isIdentity()
                                       || m_currentPaintInfo.transformAnimations.size() > 0;
        if (groupHasTransform) {
            StructureNodeInfo info;
            info.stage = StructureNodeStage::Start;
            info.nodeId = scrub(group.name());
            info.transform.setDefaultValue(QVariant::fromValue(m_currentPaintInfo.transform));
            info.isDefaultTransform = m_currentPaintInfo.transform.isIdentity();

            fillCommonNodeInfo(&group, &info);
            fillAnimationNodeInfo(&group, &info);

            m_generator->generateStructureNode(info);
            m_currentStructElements.push(&group);

            m_currentPaintInfo.transform.reset();
            m_currentPaintInfo.transformAnimations.clear();
        }
    }
}

void QLottieVisitor::finish(const QLottieLayer &layer)
{
    QLOTTIEVISITOR_DEBUG << "[layer '" << layer.name() << "' finish]";

    m_frameOffset -= layer.frameOffset();

    StructureNodeInfo info;
    info.stage = StructureNodeStage::End;

    fillCommonNodeInfo(&layer, &info);
    m_generator->generateStructureNode(info);
}

void QLottieVisitor::finish(const QLottieGroup &group)
{
    QLOTTIEVISITOR_DEBUG << "[group '" << group.name() << "' finish]";

    if (!m_currentStructElements.isEmpty() && m_currentStructElements.top() == &group) {
        StructureNodeInfo info;
        info.stage = StructureNodeStage::End;

        fillCommonNodeInfo(&group, &info);
        m_generator->generateStructureNode(info);
        m_currentStructElements.pop();
    }
}

void QLottieVisitor::render(const QLottieRect &rect)
{
    QLOTTIEVISITOR_DEBUG << "[rect]";

    processShape(&rect);
}

void QLottieVisitor::render(const QLottieEllipse &ellipse)
{
    QLOTTIEVISITOR_DEBUG << "[ellipse]";

    processShape(&ellipse);
}

void QLottieVisitor::render(const QLottiePolyStar &star)
{
    QLOTTIEVISITOR_DEBUG << "[star]";

    processShape(&star);
}

void QLottieVisitor::render(const QLottieRound &round)
{
    QLOTTIEVISITOR_DEBUG << "[round]";

    processShape(&round);
}

void QLottieVisitor::render(const QLottieFill &fill)
{
    QLOTTIEVISITOR_DEBUG << "[fill color=" << fill.color() << ", opacity=" << fill.opacity() << "]";

    QColor color = fill.color();
    color.setAlphaF(color.alphaF() * (fill.opacity() / 100.0));
    m_currentPaintInfo.fill = color;

    bool isEvenOdd = (fill.definition().value(QLatin1String("r")).toInt() == 2);
    m_currentPaintInfo.fillRule = isEvenOdd ? Qt::OddEvenFill : Qt::WindingFill;
}

void QLottieVisitor::render(const QLottieGFill &gradient)
{
    QLOTTIEVISITOR_DEBUG << "[fill gradient]";

    if (gradient.value() != nullptr)
        m_currentPaintInfo.fill = *gradient.value();
    bool isEvenOdd = (gradient.definition().value(QLatin1String("r")).toInt() == 2);
    m_currentPaintInfo.fillRule = isEvenOdd ? Qt::OddEvenFill : Qt::WindingFill;
}

void QLottieVisitor::render(const QLottieImage &image)
{
    QLOTTIEVISITOR_DEBUG << "[image size=" << image.size() << "]";

    ImageNodeInfo info;
    fillCommonNodeInfo(&image, &info);
    info.image = image.image();
    info.rect = QRectF(QPointF(), image.size());
    info.externalFileReference = image.url().toLocalFile();

    m_generator->generateImageNode(info);
}

void QLottieVisitor::render(const QLottieStroke &stroke)
{
    QLOTTIEVISITOR_DEBUG << "[stroke color=" << stroke.pen().color()
                         << ", opacity=" << stroke.opacity() << "]";

    const QPen pen = stroke.pen();
    m_currentPaintInfo.stroke = pen;

    QColor color = pen.color();
    color.setAlphaF(color.alphaF() * (stroke.opacity() / 100.0));
    m_currentPaintInfo.stroke.setColor(color);
}

void QLottieVisitor::render(const QLottieBasicTransform &transform)
{
    QLOTTIEVISITOR_DEBUG << "[basic transform s=" << transform.scale()
                         << ", r=" << transform.rotation()
                         << ", o=" << transform.opacity() << "]";
    if (hasAnimations(&transform))
        collectTransformAnimations(&transform);
    else
        applyTransform(&m_currentPaintInfo.transform, transform, false);

    m_currentPaintInfo.opacity *= transform.opacity();
}

namespace {
    template<typename T>
    QLottieVisitor::PaintInfo::TransformAnimationInfo
        collectAnimations(const T &property,
                          QTransform::TransformationType type,
                          std::function<void(qreal,
                                             const QVariant &,
                                             QLottieVisitor::PaintInfo::TransformAnimationInfo *,
                                             std::optional<QBezier>)> storeAnimationFrame,
                          std::function<QVariantList(const QVariant &)> createParams)
    {
        const auto easingCurves = property.easingCurves();
        QLottieVisitor::PaintInfo::TransformAnimationInfo info;
        info.animationType = type;
        bool firstFrame = true;
        if (easingCurves.isEmpty()) {
            const QVariantList params = createParams(QVariant::fromValue(property.value()));
            storeAnimationFrame(0, params, &info, std::nullopt);
        } else {
            for (const auto &curve : easingCurves) {
                if (firstFrame) {
                    const auto startValue = curve.startValue;
                    const QVariantList startParams = createParams(QVariant::fromValue(startValue));
                    storeAnimationFrame(0, startParams, &info, std::nullopt);
                    if (curve.startFrame > 0)
                        storeAnimationFrame(curve.startFrame, startParams, &info, std::nullopt);
                    firstFrame = false;
                }
                const auto endValue = curve.endValue;
                const QVariantList endParams = createParams(endValue);
                storeAnimationFrame(curve.endFrame, endParams, &info, curve.easing.bezier());
            }
        }

        return info;
    }
}

void QLottieVisitor::collectTransformAnimations(const QLottieBasicTransform *transform,
                                                bool isShapeTransform)
{
    Q_UNUSED(isShapeTransform);
    const QLottieProperty<QPointF> anchorPoints = transform->anchorPointProperty();
    const QLottieProperty<qreal> rotations = transform->rotationProperty();
    const QLottieProperty<QPointF> scales = transform->scaleProperty();
    const QLottieSpatialProperty positions = transform->positionProperty();
    const QLottieProperty<qreal> opacities = transform->opacityProperty();
    const QLottieProperty<qreal> xPositions = transform->xPosProperty();
    const QLottieProperty<qreal> yPositions = transform->yPosProperty();

    auto storeAnimationFrame = [&](qreal lottieFrameNumber,
                                   const QVariant &propertyValue,
                                   PaintInfo::TransformAnimationInfo *info,
                                   std::optional<QBezier> easingBezier = std::nullopt) {
        const int timePointMs = qRound(1000 * (m_frameOffset + lottieFrameNumber) / m_frameRate);
        info->frames[timePointMs] = propertyValue;
        if (easingBezier)
            info->easingPerFrame[timePointMs] = *easingBezier;
    };

    QLottieVisitor::PaintInfo::TransformAnimationInfo info;
    if (!transform->splitPosition()) {
        info = collectAnimations(positions,
                                 QTransform::TxTranslate,
                                 storeAnimationFrame,
                                 [](const QVariant &v)
                                 {
                                     return QVariantList{ v };
                                 });
        m_currentPaintInfo.transformAnimations.append(info);
    } else {
        info = collectAnimations(xPositions,
                                 QTransform::TxTranslate,
                                 storeAnimationFrame,
                                 [](const QVariant &v)
                                 {
                                     return QVariantList{ QVariant::fromValue(QPointF(v.toReal(), 0.0)) };
                                 });
        m_currentPaintInfo.transformAnimations.append(info);

        info = collectAnimations(yPositions,
                                 QTransform::TxTranslate,
                                 storeAnimationFrame,
                                 [](const QVariant &v)
                                 {
                                     return QVariantList{ QVariant::fromValue(QPointF(0.0, v.toReal())) };
                                 });
        m_currentPaintInfo.transformAnimations.append(info);
    }

    auto storeRotationParameter = [](const QVariant &v) {
        return QVariantList{ QVariant::fromValue(QPointF(0, 0)), v };
    };
    info = collectAnimations(rotations,
                             QTransform::TxRotate,
                             storeAnimationFrame,
                             storeRotationParameter);
    m_currentPaintInfo.transformAnimations.append(info);

    if (isShapeTransform) {
        const QLottieShapeTransform *shapeTransform =
            static_cast<const QLottieShapeTransform *>(transform);

        const QLottieProperty<qreal> skews = shapeTransform->skewProperty();
        const QLottieProperty<qreal> skewAxes = shapeTransform->skewAxisProperty();

        // Lottie shear transforms work by first rotating by skew axis angle, then applying
        // the skewAngle as the shear along the X-axis, and then rotating back.
        info = collectAnimations(skewAxes,
                                 QTransform::TxRotate,
                                 storeAnimationFrame,
                                 [](const QVariant &v) {
                                     return QVariantList{ QVariant::fromValue(QPointF(0, 0)),
                                                          QVariant::fromValue(-1.0 * v.toReal()) };
                                 });
        m_currentPaintInfo.transformAnimations.append(info);

        info = collectAnimations(skews,
                                 QTransform::TxShear,
                                 storeAnimationFrame,
                                 [](const QVariant &v) {
                                     return QVariantList{ QVariant::fromValue(QPointF(-1.0 * v.toReal(), 0.0)) };
                                 });
        m_currentPaintInfo.transformAnimations.append(info);

        info = collectAnimations(skewAxes,
                                 QTransform::TxRotate,
                                 storeAnimationFrame,
                                 storeRotationParameter);
        m_currentPaintInfo.transformAnimations.append(info);
    }

    info = collectAnimations(scales,
                             QTransform::TxScale,
                             storeAnimationFrame,
                             [](const QVariant &v) {
                                 return QVariantList{ QVariant::fromValue(v.toPointF() / 100.0) };
                             });
    m_currentPaintInfo.transformAnimations.append(info);

    info = collectAnimations(anchorPoints,
                             QTransform::TxTranslate,
                             storeAnimationFrame,
                             [](const QVariant &v) {
                                 return QVariantList{ QVariant::fromValue(v.toPointF() * -1.0) };
                             });
    m_currentPaintInfo.transformAnimations.append(info);

    {
        const QList<EasingSegment<qreal> > easingCurves = opacities.easingCurves();
        PaintInfo::TransformAnimationInfo info;
        info.animationType = QTransform::TxNone;
        bool firstFrame = true;
        for (const auto &curve : easingCurves) {
            if (firstFrame) {
                const qreal startValue = curve.startValue / 100;
                const QVariant startParams = QVariant::fromValue(startValue);
                storeAnimationFrame(0, startParams, &info, std::nullopt);
                if (curve.startFrame > 0)
                    storeAnimationFrame(curve.startFrame, startParams, &info);
                firstFrame = false;
            }

            const qreal endValue = curve.endValue / 100;
            const QVariant endParams = QVariant::fromValue(endValue);
            storeAnimationFrame(curve.endFrame, endParams, &info, curve.easing.bezier());
        }
        m_currentPaintInfo.transformAnimations.append(info);
    }
}

void QLottieVisitor::enumerateLayerChildren(const QLottieBase *node)
{
    if (auto layer = QLottieLayer::checkedCast(node))
        m_layers.append(layer);
    for (const QLottieBase *child : node->children())
        enumerateLayerChildren(child);
}

bool QLottieVisitor::hasAnimations(const QLottieBasicTransform *transform, bool isShapeTransform)
{
    bool hasAnimations = transform->rotationProperty().startFrame() < transform->rotationProperty().endFrame()
                         || transform->positionProperty().startFrame() < transform->positionProperty().endFrame()
                         || transform->scaleProperty().startFrame() < transform->scaleProperty().endFrame()
                         || transform->opacityProperty().startFrame() < transform->opacityProperty().endFrame();

    if (transform->splitPosition() && !hasAnimations) {
        hasAnimations = transform->xPosProperty().startFrame() < transform->xPosProperty().endFrame()
                        || transform->yPosProperty().startFrame() < transform->yPosProperty().endFrame();
    }

    if (isShapeTransform && !hasAnimations) {
        const QLottieShapeTransform *shapeTransform = static_cast<const QLottieShapeTransform *>(transform);
        hasAnimations = shapeTransform->skewProperty().startFrame() < shapeTransform->skewProperty().endFrame()
            || shapeTransform->skewAxisProperty().startFrame() < shapeTransform->skewAxisProperty().endFrame();
    }

    if (qEnvironmentVariableIntValue("QLOTTIEVISITOR_DISABLE_ANIMATIONS"))
        return false;

    return hasAnimations;
}

void QLottieVisitor::render(const QLottieShapeTransform &transform)
{
    if (!m_currentStructElements.isEmpty() && transform.parent() == m_currentStructElements.top()) {
        // This transform was already applied as part of a group structure node
        return;
    }

    QLOTTIEVISITOR_DEBUG << "[shape transform s=" << transform.scale()
        << ", r=" << transform.rotation()
        << ", o=" << transform.opacity() << "]";

    if (hasAnimations(&transform, true))
        collectTransformAnimations(&transform, true);
    else
        applyTransform(&m_currentPaintInfo.transform, transform, true);

    m_currentPaintInfo.opacity *= transform.opacity();
}

void QLottieVisitor::render(const QLottieFreeFormShape &shape)
{
    QLOTTIEVISITOR_DEBUG << "[freeform]";

    processShape(&shape);
}

void QLottieVisitor::render(const QLottieTrimPath &trim)
{
    QLOTTIEVISITOR_DEBUG << "[trim]";

    m_currentPaintInfo.trim.enabled = true;
    m_currentPaintInfo.trim.start.setDefaultValue(trim.start() / 100.0);
    m_currentPaintInfo.trim.end.setDefaultValue(trim.end() / 100.0);
    m_currentPaintInfo.trim.offset.setDefaultValue(trim.offset() / 360.0);

    auto registerAnimations = [&](QQuickAnimatedProperty *outProperty,
                                  const QLottieProperty<qreal> &inProperty,
                                  qreal scale) {
        const QList<EasingSegment<qreal> > easingCurves = inProperty.easingCurves();

        QQuickAnimatedProperty::PropertyAnimation animation;
        animation.flags |= QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;

        for (const auto &curve : easingCurves) {
            const qreal startValue = curve.startValue * scale;
            const qreal endValue = curve.endValue * scale;

            int startFrameTime = QFixed::fromReal(1000 * curve.startFrame / qreal(m_frameRate)).round().toInt();
            int endFrameTime = QFixed::fromReal(1000 * curve.endFrame / qreal(m_frameRate)).round().toInt();

            animation.frames[startFrameTime] = startValue;
            animation.frames[endFrameTime] = endValue;

            animation.easingPerFrame[endFrameTime] = curve.easing.bezier();
        }
        if (!animation.frames.isEmpty()) {
            animation.frames[0] = outProperty->defaultValue();
            animation.frames[m_duration] = animation.frames.last();
            outProperty->addAnimation(animation);
        }
    };

    registerAnimations(&m_currentPaintInfo.trim.start, trim.startProperty(), 1.0 / 100.0);
    registerAnimations(&m_currentPaintInfo.trim.end, trim.endProperty(), 1.0 / 100.0);
    registerAnimations(&m_currentPaintInfo.trim.offset, trim.offsetProperty(), 1.0 / 360.0);

    if (!trim.isParallel())
        processShape(&trim, m_currentPaintInfo.unitedPath);
}

void QLottieVisitor::render(const QLottieFillEffect &effect)
{
    QLOTTIEVISITOR_DEBUG << "[effect]";

    // ### What are you?
    Q_UNUSED(effect);
}

void QLottieVisitor::render(const QLottieRepeater &repeater)
{
    QLOTTIEVISITOR_DEBUG << "[repeater]";

    // ### Repeats the following shapes N times with different transforms
    Q_UNUSED(repeater);
}

void QLottieVisitor::processShape(const QLottieShape *shape, const QPainterPath &path)
{
    QLOTTIEVISITOR_DEBUG << "[drawing shape with"
                         << " stroke=" << m_currentPaintInfo.stroke
                         << ", fill=" << m_currentPaintInfo.fill;

    if (path.isEmpty())
        return;
    StructureNodeInfo info;
    info.stage = StructureNodeStage::Start;
    info.isPathContainer = true;

    info.transform.setDefaultValue(QVariant::fromValue(m_currentPaintInfo.transform));
    info.isDefaultTransform = m_currentPaintInfo.transform.isIdentity();
    info.opacity.setDefaultValue(m_currentPaintInfo.opacity);
    info.isDefaultOpacity = qFuzzyCompare(m_currentPaintInfo.opacity, 1.0);

    if (shape) {
        fillCommonNodeInfo(shape, &info);
        fillAnimationNodeInfo(shape, &info);
    }

    m_generator->generateStructureNode(info);

    PathNodeInfo pathInfo;

    pathInfo.painterPath = path;
    pathInfo.fillRule = m_currentPaintInfo.fillRule;
    pathInfo.fillColor.setDefaultValue(QVariant::fromValue(m_currentPaintInfo.fill.color()));
    pathInfo.strokeStyle = StrokeStyle::fromPen(m_currentPaintInfo.stroke);
    pathInfo.strokeStyle.color.setDefaultValue(QVariant::fromValue(m_currentPaintInfo.stroke.color()));
    if (m_currentPaintInfo.fill.gradient() != nullptr)
        pathInfo.grad = *m_currentPaintInfo.fill.gradient();
    if (trimmingState() != TrimmingState::Off)
        pathInfo.trim = m_currentPaintInfo.trim;
    m_generator->generatePath(pathInfo);

    info.stage = StructureNodeStage::End;
    m_generator->generateStructureNode(info);
}

void QLottieVisitor::processShape(const QLottieShape *shape)
{
    QLOTTIEVISITOR_DEBUG << "[shape bounds=" << shape->path().controlPointRect() << "]";

    if (trimmingState() == Sequential) {
        QPainterPath p = m_currentPaintInfo.transform.map(shape->path());
        p.addPath(m_currentPaintInfo.unitedPath);
        m_currentPaintInfo.unitedPath = p;
    // } else if (m_buildingClipRegion) {     ###
    } else {
        processShape(shape, shape->path());
    }
}

QT_END_NAMESPACE
