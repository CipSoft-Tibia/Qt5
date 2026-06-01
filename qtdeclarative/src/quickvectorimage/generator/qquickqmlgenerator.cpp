// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickqmlgenerator_p.h"
#include "qquicknodeinfo_p.h"
#include "utils_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickimage_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

static QString sanitizeString(const QString &input)
{
    QString s = input;
    s.replace(QLatin1Char('"'), QLatin1String("\\\""));
    return s;
}

QQuickQmlGenerator::QQuickQmlGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, const QString &outFileName)
    : QQuickGenerator(fileName, flags)
    , outputFileName(outFileName)
{
    m_result.open(QIODevice::ReadWrite);
}

QQuickQmlGenerator::~QQuickQmlGenerator()
{
}

bool QQuickQmlGenerator::save()
{
    bool res = true;
    if (!outputFileName.isEmpty()) {
        QFileInfo fileInfo(outputFileName);
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            qCWarning(lcQuickVectorImage) << "Failed to create path" << dir.absolutePath();
            res = false;
        } else {
            QFile outFile(outputFileName);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(m_result.data());
                outFile.close();
            } else {
                qCWarning(lcQuickVectorImage) << "Failed to write to file" << outFile.fileName();
                res = false;
            }
        }
    }

    if (lcQuickVectorImage().isDebugEnabled())
        qCDebug(lcQuickVectorImage).noquote() << m_result.data().left(300);

    return res;
}

void QQuickQmlGenerator::setShapeTypeName(const QString &name)
{
    m_shapeTypeName = name.toLatin1();
}

QString QQuickQmlGenerator::shapeTypeName() const
{
    return QString::fromLatin1(m_shapeTypeName);
}

void QQuickQmlGenerator::setCommentString(const QString commentString)
{
    m_commentString = commentString;
}

QString QQuickQmlGenerator::commentString() const
{
    return m_commentString;
}

QString QQuickQmlGenerator::generateNodeBase(const NodeInfo &info)
{
    auto layerIdString = [](int layerId) {
        return QStringLiteral("_qt_layer%1").arg(layerId);
    };

    if (!info.nodeId.isEmpty())
        stream() << "objectName: \"" << info.nodeId << "\"";

    static int counter = 0;
    QString idString;
    if (info.layerNum >= 0)
        idString = layerIdString(info.layerNum);
    else
        idString = QStringLiteral("_qt_node%1").arg(counter++);
    stream() << "id: " << idString;

    if (!info.isDefaultOpacity)
        stream() << "opacity: " << info.opacity.defaultValue().toReal();

    const bool hasTransformReference = (info.transformReferenceLayerNum >= 0);
    const bool hasTransform = info.transform.isAnimated() || !info.isDefaultTransform || hasTransformReference;
    if (hasTransform) {
        stream() << "transform: TransformGroup {";
        m_indentLevel++;
        stream() << "id: " << idString << "_transform_base_group";

        if (info.transform.isAnimated()) {
            for (int groupIndex = 0; groupIndex < info.transform.animationGroupCount(); ++groupIndex) {
                stream() << "TransformGroup {";
                m_indentLevel++;
                stream() << "id: " << idString << "_transform_group_" << groupIndex;

                int animationStart = info.transform.animationGroup(groupIndex);
                int nextAnimationStart = groupIndex + 1 < info.transform.animationGroupCount()
                    ? info.transform.animationGroup(groupIndex + 1)
                    : info.transform.animationCount();

                for (int i = nextAnimationStart - 1; i >= animationStart; --i) {
                    const QQuickAnimatedProperty::PropertyAnimation &animation = info.transform.animation(i);
                    if (animation.frames.isEmpty())
                        continue;

                    const QVariantList &parameters = animation.frames.first().value<QVariantList>();
                    switch (animation.subtype) {
                    case QTransform::TxTranslate:
                        if (animation.isConstant()) {
                            const QPointF translation = parameters.value(0).value<QPointF>();
                            if (!translation.isNull())
                                stream() << "Translate { x: " << translation.x() << "; y: " << translation.y() << " }";
                        } else {
                            stream() << "Translate { id: " << idString << "_transform_" << groupIndex << "_" << i << " }";
                        }
                        break;
                    case QTransform::TxScale:
                        if (animation.isConstant()) {
                            const QPointF scale = parameters.value(0).value<QPointF>();
                            if (scale != QPointF(1, 1))
                                stream() << "Scale { xScale: " << scale.x() << "; yScale: " << scale.y() << " }";
                        } else {
                            stream() << "Scale { id: " << idString << "_transform_" << groupIndex << "_" << i << "}";
                        }
                        break;
                    case QTransform::TxRotate:
                        if (animation.isConstant()) {
                            const QPointF center = parameters.value(0).value<QPointF>();
                            const qreal angle = parameters.value(1).toReal();
                            if (!qFuzzyIsNull(angle))
                                stream() << "Rotation { angle: " << angle << "; origin.x: " << center.x() << "; origin.y: " << center.y() << " }"; //### center relative to what?
                        } else {
                            stream() << "Rotation { id: " << idString << "_transform_" << groupIndex << "_" << i << " }";
                        }
                        break;
                    case QTransform::TxShear:
                        if (animation.isConstant()) {
                            const QPointF skew = parameters.value(0).value<QPointF>();
                            if (!skew.isNull())
                                stream() << "Shear { xAngle: " << skew.x() << "; yAngle: " << skew.y() << " }";
                        } else {
                            stream() << "Shear { id: " << idString << "_transform_" << groupIndex << "_" << i << " }";
                        }
                        break;
                    default:
                        Q_UNREACHABLE();
                    }
                }

                m_indentLevel--;
                stream() << "}";
            }
        }

        if (!info.isDefaultTransform) {
            QTransform xf = info.transform.defaultValue().value<QTransform>();
            if (xf.type() <= QTransform::TxTranslate) {
                stream() << "Translate { x: " << xf.dx() << "; y: " << xf.dy() << "}";
            } else {
                stream() << "Matrix4x4 { matrix: ";
                generateTransform(xf);
                stream(SameLine) << "}";
            }
        }

        if (hasTransformReference) {
            stream() << "Matrix4x4 { matrix: " << layerIdString(info.transformReferenceLayerNum)
                     << ".transformMatrix }";
        }

        m_indentLevel--;
        stream() << "}";

        generateAnimateTransform(idString, info);
    }

    if (info.opacity.isAnimated())
        generatePropertyAnimation(info.opacity, idString, QStringLiteral("opacity"));

    generatePropertyAnimation(info.visibility, idString, QStringLiteral("visible"));

    return idString;
}

bool QQuickQmlGenerator::generateDefsNode(const NodeInfo &info)
{
    Q_UNUSED(info)

    return false;
}

void QQuickQmlGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    const QFileInfo outputFileInfo(outputFileName);
    const QDir outputDir(outputFileInfo.absolutePath());

    QString filePath;

    if (!m_retainFilePaths || info.externalFileReference.isEmpty()) {
        filePath = m_assetFileDirectory;
        if (filePath.isEmpty())
            filePath = outputDir.absolutePath();

        if (!filePath.isEmpty() && !filePath.endsWith(u'/'))
            filePath += u'/';

        QDir fileDir(filePath);
        if (!fileDir.exists()) {
            if (!fileDir.mkpath(QStringLiteral(".")))
                qCWarning(lcQuickVectorImage) << "Failed to create image resource directory:" << filePath;
        }

        filePath += QStringLiteral("%1%2.png").arg(m_assetFilePrefix.isEmpty()
                                                   ? QStringLiteral("svg_asset_")
                                                   : m_assetFilePrefix)
                                              .arg(info.image.cacheKey());

        if (!info.image.save(filePath))
            qCWarning(lcQuickVectorImage) << "Unabled to save image resource" << filePath;
        qCDebug(lcQuickVectorImage) << "Saving copy of IMAGE" << filePath;
    } else {
        filePath = info.externalFileReference;
    }

    const QFileInfo assetFileInfo(filePath);

    stream() << "Image {";

    m_indentLevel++;
    generateNodeBase(info);
    stream() << "x: " << info.rect.x();
    stream() << "y: " << info.rect.y();
    stream() << "width: " << info.rect.width();
    stream() << "height: " << info.rect.height();
    stream() << "source: \"" << m_urlPrefix << outputDir.relativeFilePath(assetFileInfo.absoluteFilePath()) <<"\"";

    m_indentLevel--;

    stream() << "}";
}

void QQuickQmlGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (!isNodeVisible(info))
        return;

    if (m_inShapeItemLevel > 0) {
        if (!info.isDefaultTransform)
            qWarning() << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info, overrideBoundingRect);
    } else {
        m_inShapeItemLevel++;
        stream() << shapeName() << " {";

        m_indentLevel++;
        generateNodeBase(info);

        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
            stream() << "preferredRendererType: Shape.CurveRenderer";
        optimizePaths(info, overrideBoundingRect);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        m_indentLevel--;
        stream() << "}";
        m_inShapeItemLevel--;
    }
}

void QQuickQmlGenerator::generateGradient(const QGradient *grad)
{
    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);
        stream() << "fillGradient: LinearGradient {";
        m_indentLevel++;

        QRectF gradRect(linGrad->start(), linGrad->finalStop());

        stream() << "x1: " << gradRect.left();
        stream() << "y1: " << gradRect.top();
        stream() << "x2: " << gradRect.right();
        stream() << "y2: " << gradRect.bottom();
        for (auto &stop : linGrad->stops())
            stream() << "GradientStop { position: " << QString::number(stop.first, 'g', 7)
                     << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        m_indentLevel--;
        stream() << "}";
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        stream() << "fillGradient: RadialGradient {";
        m_indentLevel++;

        stream() << "centerX: " << radGrad->center().x();
        stream() << "centerY: " << radGrad->center().y();
        stream() << "centerRadius: " << radGrad->radius();
        stream() << "focalX:" << radGrad->focalPoint().x();
        stream() << "focalY:" << radGrad->focalPoint().y();
        for (auto &stop : radGrad->stops())
            stream() << "GradientStop { position: " << QString::number(stop.first, 'g', 7)
                     << "; color: \"" << stop.second.name(QColor::HexArgb) << "\" }";
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::generateAnimationBindings()
{
    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");

    stream() << "loops: " << m_topLevelIdString << prefix << ".loops";
    stream() << "paused: " << m_topLevelIdString << prefix << ".paused";
    stream() << "running: true";

    // We need to reset the animation when the loop count changes
    stream() << "onLoopsChanged: { if (running) { restart() } }";
}

void QQuickQmlGenerator::generateEasing(const QQuickAnimatedProperty::PropertyAnimation &animation, int time)
{
    if (animation.easingPerFrame.contains(time)) {
        QBezier bezier = animation.easingPerFrame.value(time);
        QPointF c1 = bezier.pt2();
        QPointF c2 = bezier.pt3();

        bool isLinear = (c1 == c1.transposed() && c2 == c2.transposed());
        if (!isLinear) {
            stream() << "easing.type: Easing.BezierSpline";
            stream() << "easing.bezierCurve: [ " << c1.x() << ", " << c1.y()
                     << ", " << c2.x() << ", " << c2.y() << ", 1, 1 ]";
        }
    }
}

void QQuickQmlGenerator::generatePropertyAnimation(const QQuickAnimatedProperty &property,
                                                   const QString &targetName,
                                                   const QString &propertyName,
                                                   AnimationType animationType)
{
    if (!property.isAnimated())
        return;

    QString mainAnimationId = targetName
                              + QStringLiteral("_")
                              + propertyName
                              + QStringLiteral("_animation");
    mainAnimationId.replace(QLatin1Char('.'), QLatin1Char('_'));

    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");

    stream() << "Connections { target: " << m_topLevelIdString << prefix << "; function onRestart() {" << mainAnimationId << ".restart() } }";

    stream() << "ParallelAnimation {";
    m_indentLevel++;

    stream() << "id: " << mainAnimationId;

    generateAnimationBindings();

    for (int i = 0; i < property.animationCount(); ++i) {
        const QQuickAnimatedProperty::PropertyAnimation &animation = property.animation(i);

        stream() << "SequentialAnimation {";
        m_indentLevel++;

        const int startOffset = animation.startOffset;
        if (startOffset > 0)
            stream() << "PauseAnimation { duration: " << startOffset << " }";

        stream() << "SequentialAnimation {";
        m_indentLevel++;

        const int repeatCount = animation.repeatCount;
        if (repeatCount < 0)
            stream() << "loops: Animation.Infinite";
        else
            stream() << "loops: " << repeatCount;

        int previousTime = 0;
        QVariant previousValue;
        for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
            const int time = it.key();
            const int frameTime = time - previousTime;
            const QVariant &value = it.value();

            if (previousValue.isValid() && previousValue == value) {
                if (frameTime > 0)
                    stream() << "PauseAnimation { duration: " << frameTime << " }";
            } else if (animationType == AnimationType::Auto && value.typeId() == QMetaType::Bool) {
                // We special case bools, with PauseAnimation and then a setter at the end
                if (frameTime > 0)
                    stream() << "PauseAnimation { duration: " << frameTime << " }";
                stream() << "ScriptAction {";
                m_indentLevel++;

                stream() << "script:" << targetName << "." << propertyName << " = " << value.toString();

                m_indentLevel--;
                stream() << "}";
            } else {
                generateAnimatedPropertySetter(targetName,
                                               propertyName,
                                               value,
                                               animation,
                                               frameTime,
                                               time,
                                               animationType);
            }

            previousTime = time;
            previousValue = value;
        }

        if (!(animation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd)) {
            stream() << "ScriptAction {";
            m_indentLevel++;
            stream() << "script: ";

            switch (animationType) {
            case AnimationType::Auto:
                stream(SameLine) << targetName << "." << propertyName << " = ";
                break;
            case AnimationType::ColorOpacity:
                stream(SameLine) << targetName << "." << propertyName << ".a = ";
                break;
            };

            QVariant value = property.defaultValue();
            if (value.typeId() == QMetaType::QColor)
                stream(SameLine) << "\"" << value.toString() << "\"";
            else
                stream(SameLine) << value.toReal();

            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";

        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateTransform(const QTransform &xf)
{
    if (xf.isAffine()) {
        stream(SameLine) << "PlanarTransform.fromAffineMatrix("
                         << xf.m11() << ", " << xf.m12() << ", "
                         << xf.m21() << ", " << xf.m22() << ", "
                         << xf.dx() << ", " << xf.dy() << ")";
    } else {
        QMatrix4x4 m(xf);
        stream(SameLine) << "Qt.matrix4x4(";
        m_indentLevel += 3;
        const auto *data = m.data();
        for (int i = 0; i < 4; i++) {
            stream() << data[i] << ", " << data[i+4] << ", " << data[i+8] << ", " << data[i+12];
            if (i < 3)
                stream(SameLine) << ", ";
        }
        stream(SameLine) << ")";
        m_indentLevel -= 3;
    }
}

void QQuickQmlGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *painterPath, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    static int counter = 0;

    const QColor strokeColor = info.strokeStyle.color.defaultValue().value<QColor>();
    const bool noPen = strokeColor == QColorConstants::Transparent
                       && !info.strokeStyle.color.isAnimated()
                       && !info.strokeStyle.opacity.isAnimated();
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const QColor fillColor = info.fillColor.defaultValue().value<QColor>();
    const bool noFill = info.grad.type() == QGradient::NoGradient
                        && fillColor == QColorConstants::Transparent
                        && !info.fillColor.isAnimated()
                        && !info.fillOpacity.isAnimated();
    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
        return;

    if (noPen && noFill)
        return;

    auto fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());
    stream() << "ShapePath {";
    m_indentLevel++;

    const QString shapePathId = QStringLiteral("_qt_shapePath_%1").arg(counter);
    stream() << "id: " << shapePathId;

    if (!info.nodeId.isEmpty()) {
        switch (pathSelector) {
        case QQuickVectorImageGenerator::FillPath:
            stream() << "objectName: \"svg_fill_path:" << info.nodeId << "\"";
            break;
        case QQuickVectorImageGenerator::StrokePath:
            stream() << "objectName: \"svg_stroke_path:" << info.nodeId << "\"";
            break;
        case QQuickVectorImageGenerator::FillAndStroke:
            stream() << "objectName: \"svg_path:" << info.nodeId << "\"";
            break;
        }
    }

    if (noPen || !(pathSelector & QQuickVectorImageGenerator::StrokePath)) {
        stream() << "strokeColor: \"transparent\"";
    } else {
        stream() << "strokeColor: \"" << strokeColor.name(QColor::HexArgb) << "\"";
        stream() << "strokeWidth: " << info.strokeStyle.width;
        stream() << "capStyle: " << QQuickVectorImageGenerator::Utils::strokeCapStyleString(info.strokeStyle.lineCapStyle);
        stream() << "joinStyle: " << QQuickVectorImageGenerator::Utils::strokeJoinStyleString(info.strokeStyle.lineJoinStyle);
        stream() << "miterLimit: " << info.strokeStyle.miterLimit;
        if (info.strokeStyle.dashArray.length() != 0) {
            stream() << "strokeStyle: " << "ShapePath.DashLine";
            stream() << "dashPattern: " << QQuickVectorImageGenerator::Utils::listString(info.strokeStyle.dashArray);
            stream() << "dashOffset: " << info.strokeStyle.dashOffset;
        }
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        stream() << "fillColor: \"transparent\"";
    } else if (info.grad.type() != QGradient::NoGradient) {
        generateGradient(&info.grad);
        if (info.grad.coordinateMode() == QGradient::ObjectMode) {
            QTransform objectToUserSpace;
            objectToUserSpace.translate(boundingRect.x(), boundingRect.y());
            objectToUserSpace.scale(boundingRect.width(), boundingRect.height());
            fillTransform *= objectToUserSpace;
        }
    } else {
        stream() << "fillColor: \"" << fillColor.name(QColor::HexArgb) << "\"";
    }

    if (!fillTransform.isIdentity()) {
        const QTransform &xf = fillTransform;
        stream() << "fillTransform: ";
        if (info.fillTransform.type() == QTransform::TxTranslate)
            stream(SameLine) << "PlanarTransform.fromTranslate(" << xf.dx() << ", " << xf.dy() << ")";
        else if (info.fillTransform.type() == QTransform::TxScale && !xf.dx() && !xf.dy())
            stream(SameLine) << "PlanarTransform.fromScale(" << xf.m11() << ", " << xf.m22() << ")";
        else
            generateTransform(xf);
    }

    if (info.trim.enabled) {
        stream() << "trim.start: " << info.trim.start.defaultValue().toReal();
        stream() << "trim.end: " << info.trim.end.defaultValue().toReal();
        stream() << "trim.offset: " << info.trim.offset.defaultValue().toReal();

    }

    if (fillRule == QQuickShapePath::WindingFill)
        stream() << "fillRule: ShapePath.WindingFill";
    else
        stream() << "fillRule: ShapePath.OddEvenFill";

    QString hintStr;
    if (quadPath)
        hintStr = QQuickVectorImageGenerator::Utils::pathHintString(*quadPath);
    if (!hintStr.isEmpty())
        stream() << hintStr;

    QString svgPathString = painterPath ? QQuickVectorImageGenerator::Utils::toSvgString(*painterPath) : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);
    stream() <<   "PathSvg { path: \"" << svgPathString << "\" }";

    m_indentLevel--;
    stream() << "}";

    if (info.trim.enabled) {
        generatePropertyAnimation(info.trim.start, shapePathId + QStringLiteral(".trim"), QStringLiteral("start"));
        generatePropertyAnimation(info.trim.end, shapePathId + QStringLiteral(".trim"), QStringLiteral("end"));
        generatePropertyAnimation(info.trim.offset, shapePathId + QStringLiteral(".trim"), QStringLiteral("offset"));
    }

    generatePropertyAnimation(info.strokeStyle.color, shapePathId, QStringLiteral("strokeColor"));
    generatePropertyAnimation(info.strokeStyle.opacity, shapePathId, QStringLiteral("strokeColor"), AnimationType::ColorOpacity);
    generatePropertyAnimation(info.fillColor, shapePathId, QStringLiteral("fillColor"));
    generatePropertyAnimation(info.fillOpacity, shapePathId, QStringLiteral("fillColor"), AnimationType::ColorOpacity);

    counter++;
}

void QQuickQmlGenerator::generateNode(const NodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    stream() << "// Missing Implementation for SVG Node: " << info.typeName;
    stream() << "// Adding an empty Item and skipping";
    stream() << "Item {";
    m_indentLevel++;
    generateNodeBase(info);
    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    static int counter = 0;
    stream() << "Item {";
    m_indentLevel++;
    generateNodeBase(info);

    if (!info.isTextArea)
        stream() << "Item { id: textAlignItem_" << counter << "; x: " << info.position.x() << "; y: " << info.position.y() << "}";

    stream() << "Text {";

    m_indentLevel++;

    const QString textItemId = QStringLiteral("_qt_textItem_%1").arg(counter);
    stream() << "id: " << textItemId;

    generatePropertyAnimation(info.fillColor, textItemId, QStringLiteral("color"));
    generatePropertyAnimation(info.fillOpacity, textItemId, QStringLiteral("color"), AnimationType::ColorOpacity);
    generatePropertyAnimation(info.strokeColor, textItemId, QStringLiteral("styleColor"));
    generatePropertyAnimation(info.strokeOpacity, textItemId, QStringLiteral("styleColor"), AnimationType::ColorOpacity);

    if (info.isTextArea) {
        stream() << "x: " << info.position.x();
        stream() << "y: " << info.position.y();
        if (info.size.width() > 0)
            stream() << "width: " << info.size.width();
        if (info.size.height() > 0)
            stream() << "height: " << info.size.height();
        stream() << "wrapMode: Text.Wrap"; // ### WordWrap? verify with SVG standard
        stream() << "clip: true"; //### Not exactly correct: should clip on the text level, not the pixel level
    } else {
        QString hAlign = QStringLiteral("left");
        stream() << "anchors.baseline: textAlignItem_" << counter << ".top";
        switch (info.alignment) {
        case Qt::AlignHCenter:
            hAlign = QStringLiteral("horizontalCenter");
            break;
        case Qt::AlignRight:
            hAlign = QStringLiteral("right");
            break;
        default:
            qCDebug(lcQuickVectorImage) << "Unexpected text alignment" << info.alignment;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            break;
        }
        stream() << "anchors." << hAlign << ": textAlignItem_" << counter << ".left";
    }
    counter++;

    stream() << "color: \"" << info.fillColor.defaultValue().value<QColor>().name(QColor::HexArgb) << "\"";
    stream() << "textFormat:" << (info.needsRichText ? "Text.RichText" : "Text.StyledText");

    stream() << "text: \"" << sanitizeString(info.text) << "\"";
    stream() << "font.family: \"" << sanitizeString(info.font.family()) << "\"";
    if (info.font.pixelSize() > 0)
        stream() << "font.pixelSize:" << info.font.pixelSize();
    else if (info.font.pointSize() > 0)
        stream() << "font.pixelSize:" << info.font.pointSizeF();
    if (info.font.underline())
        stream() << "font.underline: true";
    if (info.font.weight() != QFont::Normal)
        stream() << "font.weight: " << int(info.font.weight());
    if (info.font.italic())
        stream() << "font.italic: true";
    switch (info.font.hintingPreference()) {
    case QFont::PreferFullHinting:
        stream() << "font.hintingPreference: Font.PreferFullHinting";
        break;
    case QFont::PreferVerticalHinting:
        stream() << "font.hintingPreference: Font.PreferVerticalHinting";
        break;
    case QFont::PreferNoHinting:
        stream() << "font.hintingPreference: Font.PreferNoHinting";
        break;
    case QFont::PreferDefaultHinting:
        stream() << "font.hintingPreference: Font.PreferDefaultHinting";
        break;
    };

    const QColor strokeColor = info.strokeColor.defaultValue().value<QColor>();
    if (strokeColor != QColorConstants::Transparent || info.strokeColor.isAnimated()) {
        stream() << "styleColor: \"" << strokeColor.name(QColor::HexArgb) << "\"";
        stream() << "style: Text.Outline";
    }

    m_indentLevel--;
    stream() << "}";

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    if (info.stage == StructureNodeStage::Start) {
        stream() << "Item {";
        m_indentLevel++;
        generateNodeBase(info);
    } else {
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::generatePathContainer(const StructureNodeInfo &info)
{
    Q_UNUSED(info);
    stream() << shapeName() <<" {";
    m_indentLevel++;
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
        stream() << "preferredRendererType: Shape.CurveRenderer";
    m_indentLevel--;

    m_inShapeItemLevel++;
}

void QQuickQmlGenerator::generateAnimatedPropertySetter(const QString &targetName,
                                                        const QString &propertyName,
                                                        const QVariant &value,
                                                        const QQuickAnimatedProperty::PropertyAnimation &animation,
                                                        int frameTime,
                                                        int time,
                                                        AnimationType animationType)
{
    if (frameTime > 0) {
        switch (animationType) {
        case AnimationType::Auto:
            if (value.typeId() == QMetaType::QColor)
                stream() << "ColorAnimation {";
            else
                stream() << "PropertyAnimation {";
            break;
        case AnimationType::ColorOpacity:
            stream() << "ColorOpacityAnimation {";
            break;
        };
        m_indentLevel++;

        stream() << "duration: " << frameTime;
        stream() << "target: " << targetName;
        stream() << "property: \"" << propertyName << "\"";
        stream() << "to: ";
        if (value.typeId() == QMetaType::QVector3D) {
            const QVector3D &v = value.value<QVector3D>();
            stream(SameLine) << "Qt.vector3d(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        } else if (value.typeId() == QMetaType::QColor) {
            stream(SameLine) << "\"" << value.toString() << "\"";
        } else {
            stream(SameLine) << value.toReal();
        }
        generateEasing(animation, time);
        m_indentLevel--;
        stream() << "}";
    } else {
        stream() << "ScriptAction {";
        m_indentLevel++;
        stream() << "script:" << targetName << "." << propertyName;
        if (animationType == AnimationType::ColorOpacity)
            stream(SameLine) << ".a";

        stream(SameLine) << " = ";
        if (value.typeId() == QMetaType::QVector3D) {
            const QVector3D &v = value.value<QVector3D>();
            stream(SameLine) << "Qt.vector3d(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
        } else if (value.typeId() == QMetaType::QColor) {
            stream(SameLine) << "\"" << value.toString() << "\"";
        } else {
            stream(SameLine) << value.toReal();
        }
        m_indentLevel--;
        stream() << "}";
    }
}

void QQuickQmlGenerator::generateAnimateTransform(const QString &targetName, const NodeInfo &info)
{
    if (!info.transform.isAnimated())
        return;

    const QString mainAnimationId = targetName
                                    + QStringLiteral("_transform_animation");

    QString prefix;
    if (Q_UNLIKELY(!isRuntimeGenerator()))
        prefix = QStringLiteral(".animations");
    stream() << "Connections { target: " << m_topLevelIdString << prefix << "; function onRestart() {" << mainAnimationId << ".restart() } }";

    stream() << "ParallelAnimation {";
    m_indentLevel++;

    stream() << "id:" << mainAnimationId;

    generateAnimationBindings();
    for (int groupIndex = 0; groupIndex < info.transform.animationGroupCount(); ++groupIndex) {
        int animationStart = info.transform.animationGroup(groupIndex);
        int nextAnimationStart = groupIndex + 1 < info.transform.animationGroupCount()
                                     ? info.transform.animationGroup(groupIndex + 1)
                                     : info.transform.animationCount();

        // The first animation in the group holds the shared properties for the whole group
        const QQuickAnimatedProperty::PropertyAnimation &firstAnimation = info.transform.animation(animationStart);
        const bool freeze = firstAnimation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd;
        const bool replace = firstAnimation.flags & QQuickAnimatedProperty::PropertyAnimation::ReplacePreviousAnimations;

        stream() << "SequentialAnimation {";
        m_indentLevel++;

        const int startOffset = firstAnimation.startOffset;
        if (startOffset > 0)
            stream() << "PauseAnimation { duration: " << startOffset << " }";

        const int repeatCount = firstAnimation.repeatCount;
        if (repeatCount < 0)
            stream() << "loops: Animation.Infinite";
        else
            stream() << "loops: " << repeatCount;

        if (replace) {
            stream() << "ScriptAction {";
            m_indentLevel++;

            stream() << "script: " << targetName << "_transform_base_group"
                     << ".activateOverride(" << targetName << "_transform_group_" << groupIndex << ")";

            m_indentLevel--;
            stream() << "}";
        }

        stream() << "ParallelAnimation {";
        m_indentLevel++;

        for (int i = animationStart; i < nextAnimationStart; ++i) {
            const QQuickAnimatedProperty::PropertyAnimation &animation = info.transform.animation(i);
            if (animation.isConstant())
                continue;
            bool hasRotationCenter = false;
            if (animation.subtype == QTransform::TxRotate) {
                for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
                    const QPointF center = it->value<QVariantList>().value(0).value<QPointF>();
                    if (!center.isNull()) {
                        hasRotationCenter = true;
                        break;
                    }
                }
            }

            stream() << "SequentialAnimation {";
            m_indentLevel++;

            int previousTime = 0;
            QVariantList previousParameters;
            for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
                const int time = it.key();
                const int frameTime = time - previousTime;
                const QVariantList &parameters = it.value().value<QVariantList>();
                if (parameters.isEmpty())
                    continue;

                if (parameters == previousParameters) {
                    if (frameTime > 0)
                        stream() << "PauseAnimation { duration: " << frameTime << " }";
                } else {
                    stream() << "ParallelAnimation {";
                    m_indentLevel++;

                    const QString propertyTargetName = targetName
                                                       + QStringLiteral("_transform_")
                                                       + QString::number(groupIndex)
                                                       + QStringLiteral("_")
                                                       + QString::number(i);

                    switch (animation.subtype) {
                    case QTransform::TxTranslate:
                    {
                        const QPointF translation = parameters.first().value<QPointF>();

                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("x"),
                                                       translation.x(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("y"),
                                                       translation.y(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    case QTransform::TxScale:
                    {
                        const QPointF scale = parameters.first().value<QPointF>();
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("xScale"),
                                                       scale.x(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("yScale"),
                                                       scale.y(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    case QTransform::TxRotate:
                    {
                        Q_ASSERT(parameters.size() == 2);
                        const qreal angle = parameters.value(1).toReal();
                        if (hasRotationCenter) {
                            const QPointF center = parameters.value(0).value<QPointF>();
                            generateAnimatedPropertySetter(propertyTargetName,
                                                           QStringLiteral("origin"),
                                                           QVector3D(center.x(), center.y(), 0.0),
                                                           animation,
                                                           frameTime,
                                                           time);
                        }
                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("angle"),
                                                       angle,
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    case QTransform::TxShear:
                    {
                        const QPointF skew = parameters.first().value<QPointF>();

                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("xAngle"),
                                                       skew.x(),
                                                       animation,
                                                       frameTime,
                                                       time);

                        generateAnimatedPropertySetter(propertyTargetName,
                                                       QStringLiteral("yAngle"),
                                                       skew.y(),
                                                       animation,
                                                       frameTime,
                                                       time);
                        break;
                    }
                    default:
                        Q_UNREACHABLE();
                    }

                    m_indentLevel--;
                    stream() << "}"; // Parallel key frame animation
                }

                previousTime = time;
                previousParameters = parameters;
            }

            m_indentLevel--;
            stream() << "}"; // Parallel key frame animation
        }

        m_indentLevel--;
        stream() << "}"; // Parallel key frame animation

        // If the animation ever finishes, then we add an action on the end that handles itsr
        // freeze state
        if (firstAnimation.repeatCount >= 0) {
            stream() << "ScriptAction {";
            m_indentLevel++;

            stream() << "script: {";
            m_indentLevel++;

            if (!freeze) {
                stream() << targetName << "_transform_base_group.deactivate("
                         << targetName << "_transform_group_" << groupIndex << ")";
            } else if (!replace) {
                stream() << targetName << "_transform_base_group.deactivateOverride("
                         << targetName << "_transform_group_" << groupIndex << ")";
            }

            m_indentLevel--;
            stream() << "}";

            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}";
}

bool QQuickQmlGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (!isNodeVisible(info))
        return false;

    const bool isPathContainer = !info.forceSeparatePaths && info.isPathContainer;
    if (info.stage == StructureNodeStage::Start) {
        if (isPathContainer) {
            generatePathContainer(info);
        } else  if (info.layerNum >= 0) {
            stream() << "LayerItem {";
        } else {
            stream() << "Item { // Structure node";
        }

        if (!info.viewBox.isEmpty()) {
            m_indentLevel++;
            stream() << "transform: [";
            m_indentLevel++;
            bool translate = !qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y());
            if (translate)
                stream() << "Translate { x: " << -info.viewBox.x() << "; y: " << -info.viewBox.y() << " },";
            stream() << "Scale { xScale: width / " << info.viewBox.width() << "; yScale: height / " << info.viewBox.height() << " }";
            m_indentLevel--;
            stream() << "]";
            m_indentLevel--;
        }

        m_indentLevel++;
        generateNodeBase(info);
    } else {
        m_indentLevel--;
        stream() << "}";
        if (isPathContainer)
            m_inShapeItemLevel--;
    }

    return true;
}

bool QQuickQmlGenerator::generateRootNode(const StructureNodeInfo &info)
{
    const QStringList comments = m_commentString.split(u'\n');

    if (!isNodeVisible(info)) {
        m_indentLevel = 0;

        if (comments.isEmpty()) {
            stream() << "// Generated from SVG";
        } else {
            for (const auto &comment : comments)
                stream() << "// " << comment;
        }

        stream() << "import QtQuick";
        stream() << "import QtQuick.Shapes" << Qt::endl;
        stream() << "Item {";
        m_indentLevel++;

        double w = info.size.width();
        double h = info.size.height();
        if (w > 0)
            stream() << "implicitWidth: " << w;
        if (h > 0)
            stream() << "implicitHeight: " << h;

        m_indentLevel--;
        stream() << "}";

        return false;
    }

    if (info.stage == StructureNodeStage::Start) {
        m_indentLevel = 0;

        if (comments.isEmpty())
            stream() << "// Generated from SVG";
        else
            for (const auto &comment : comments)
                stream() << "// " << comment;

        stream() << "import QtQuick";
        stream() << "import QtQuick.VectorImage";
        stream() << "import QtQuick.VectorImage.Helpers";
        stream() << "import QtQuick.Shapes";
        for (const auto &import : std::as_const(m_extraImports))
            stream() << "import " << import;

        stream() << Qt::endl << "Item {";
        m_indentLevel++;

        double w = info.size.width();
        double h = info.size.height();
        if (w > 0)
            stream() << "implicitWidth: " << w;
        if (h > 0)
            stream() << "implicitHeight: " << h;

        if (Q_UNLIKELY(!isRuntimeGenerator())) {
            stream() << "component AnimationsInfo : QtObject";
            stream() << "{";
            m_indentLevel++;
        }

        stream() << "property bool paused: false";
        stream() << "property int loops: 1";
        stream() << "signal restart()";

        if (Q_UNLIKELY(!isRuntimeGenerator())) {
            m_indentLevel--;
            stream() << "}";
            stream() << "property AnimationsInfo animations : AnimationsInfo {}";
        }

        if (!info.viewBox.isEmpty()) {
            stream() << "transform: [";
            m_indentLevel++;
            bool translate = !qFuzzyIsNull(info.viewBox.x()) || !qFuzzyIsNull(info.viewBox.y());
            if (translate)
                stream() << "Translate { x: " << -info.viewBox.x() << "; y: " << -info.viewBox.y() << " },";
            stream() << "Scale { xScale: width / " << info.viewBox.width() << "; yScale: height / " << info.viewBox.height() << " }";
            m_indentLevel--;
            stream() << "]";
        }

        if (!info.forceSeparatePaths && info.isPathContainer) {
            m_topLevelIdString = QStringLiteral("__qt_toplevel");
            stream() << "id: " << m_topLevelIdString;

            generatePathContainer(info);
            m_indentLevel++;

            generateNodeBase(info);
        } else {
            m_topLevelIdString = generateNodeBase(info);
        }
    } else {
        if (m_inShapeItemLevel > 0) {
            m_inShapeItemLevel--;
            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";
        stream().flush();
    }

    return true;
}

QStringView QQuickQmlGenerator::indent()
{
    static QString indentString;
    int indentWidth = m_indentLevel * 4;
    if (indentWidth > indentString.size())
        indentString.fill(QLatin1Char(' '), indentWidth * 2);
    return QStringView(indentString).first(indentWidth);
}

QTextStream &QQuickQmlGenerator::stream(int flags)
{
    if (m_stream.device() == nullptr)
        m_stream.setDevice(&m_result);
    else if (!(flags & StreamFlags::SameLine))
        m_stream << Qt::endl << indent();
    return m_stream;
}

const char *QQuickQmlGenerator::shapeName() const
{
    return m_shapeTypeName.isEmpty() ? "Shape" : m_shapeTypeName.constData();
}

QT_END_NAMESPACE
