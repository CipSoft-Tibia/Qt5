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
            stream().flush(); // Add a final newline and flush the stream to m_result
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

void QQuickQmlGenerator::generateNodeBase(const NodeInfo &info)
{
    if (!info.nodeId.isEmpty())
        stream() << "objectName: \"" << info.nodeId << "\"";

    static int counter = 0;
    const QString idString = QStringLiteral("_qt_node%1").arg(counter++);
    stream() << "id: " << idString;

    if (!info.isDefaultOpacity)
        stream() << "opacity: " << info.opacity;

    if (!info.transformAnimation.animationTypes.isEmpty()) {
        stream() << "transform: [";
        m_indentLevel++;
        for (int i = info.transformAnimation.animationTypes.size() - 1; i >= 0; --i) {
            switch (info.transformAnimation.animationTypes.at(i)) {
            case QTransform::TxTranslate:
                stream() << "Translate { id: " << idString << "_transform_" << i << " }";
                break;
            case QTransform::TxScale:
                stream() << "Scale { id: " << idString << "_transform_" << i << "}";
                break;
            case QTransform::TxRotate:
                stream() << "Rotation { id: " << idString << "_transform_" << i << "; origin.x: " << idString << ".width / 2.0; origin.y: " << idString << ".height / 2.0 }";
                break;
            case QTransform::TxShear:
                stream() << "Shear { id: " << idString << "_transform_" << i << " }";
                break;
            default:
                Q_UNREACHABLE();
            }

            if (i > 0)
                stream(SameLine) << ",";
        }

        if (!info.isDefaultTransform)
            stream() << ", Matrix4x4 { id: " << idString << "_transform_base }";

        m_indentLevel--;
        stream() << "]";

        generateAnimateTransform(idString, info);
    } else if (!info.isDefaultTransform) {
        stream() << "transform: Matrix4x4 { matrix: ";
        generateTransform(info.transform);
        stream(SameLine) << "}";
    }
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
    stream() << "source: \"" << outputDir.relativeFilePath(assetFileInfo.absoluteFilePath()) <<"\"";

    m_indentLevel--;

    stream() << "}";
}

void QQuickQmlGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (!isNodeVisible(info))
        return;

    if (m_inShapeItem) {
        if (!info.isDefaultTransform)
            qWarning() << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info, overrideBoundingRect);
    } else {
        m_inShapeItem = true;
        stream() << shapeName() << " {";

        m_indentLevel++;
        generateNodeBase(info);

        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
            stream() << "preferredRendererType: Shape.CurveRenderer";
        optimizePaths(info, overrideBoundingRect);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        m_indentLevel--;
        stream() << "}";
        m_inShapeItem = false;
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

    const bool noPen = info.strokeStyle.color == QColorConstants::Transparent;
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const bool noFill = info.grad.type() == QGradient::NoGradient && info.fillColor == QColorConstants::Transparent;

    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
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
        stream() << "strokeColor: \"" << info.strokeStyle.color.name(QColor::HexArgb) << "\"";
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
        stream() << "fillColor: \"" << info.fillColor.name(QColor::HexArgb) << "\"";
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

    for (int i = 0; i < info.animateColors.size(); ++i) {
        const NodeInfo::AnimateColor &animateColor = info.animateColors.at(i);
        generateAnimateColor(shapePathId,
                             animateColor.fill
                                 ? QStringLiteral("fillColor")
                                 : QStringLiteral("strokeColor"),
                             animateColor,
                             animateColor.fill
                                 ? info.fillColor
                                 : info.strokeStyle.color);
    }

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

    for (int i = 0; i < info.animateColors.size(); ++i) {
        const NodeInfo::AnimateColor &animateColor = info.animateColors.at(i);
        generateAnimateColor(textItemId,
                             animateColor.fill ? QStringLiteral("color") : QStringLiteral("styleColor"),
                             animateColor,
                             animateColor.fill ? info.fillColor : info.strokeColor);
    }

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

    stream() << "color: \"" << info.fillColor.name(QColor::HexArgb) << "\"";
    stream() << "textFormat:" << (info.needsRichText ? "Text.RichText" : "Text.StyledText");

    QString s = info.text;
    s.replace(QLatin1Char('"'), QLatin1String("\\\""));
    stream() << "text: \"" << s << "\"";
    stream() << "font.family: \"" << info.font.family() << "\"";
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

    if (info.strokeColor != QColorConstants::Transparent) {
        stream() << "styleColor: \"" << info.strokeColor.name(QColor::HexArgb) << "\"";
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
        stream() << "x: " << info.startPos.x();
        stream() << "y: " << info.startPos.y();
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

    m_inShapeItem = true;
}

void QQuickQmlGenerator::generateAnimateTransform(const QString &targetName, const NodeInfo &info)
{
    // Main animation which contains one animation for the finite part and optionally
    // one animation for the infinite part
    stream() << "SequentialAnimation {";
    m_indentLevel++;
    stream() << "running: true";

    stream() << "SequentialAnimation {";
    m_indentLevel++;

    const auto &keyFrames = info.transformAnimation.keyFrames;
    qreal previousTimeCode = 0.0;

    bool inFinitePart = true;
    for (auto it = keyFrames.constBegin(); it != keyFrames.constEnd(); ++it) {
        const auto &keyFrame = it.value();
        const qreal timeCode = it.key().toReal();
        const qreal frameTime = timeCode - previousTimeCode;
        previousTimeCode = timeCode;

        if (keyFrame.indefiniteAnimation && inFinitePart) {
            m_indentLevel--;
            stream() << "}";
            stream() << "SequentialAnimation {";
            m_indentLevel++;
            stream() << "loops: Animation.Infinite";

            inFinitePart = false;
        }

        stream() << "ParallelAnimation {";
        m_indentLevel++;

        for (int i = 0; i < info.transformAnimation.animationTypes.size(); ++i) {
            switch (info.transformAnimation.animationTypes.at(i)) {
            case QTransform::TxTranslate:
                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"x\"";
                stream() << "to: " << keyFrame.values.at(i * 3);
                m_indentLevel--;
                stream() << "}";

                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"y\"";
                stream() << "to: " << keyFrame.values.at(i * 3 + 1);
                m_indentLevel--;
                stream() << "}";
                break;
            case QTransform::TxScale:
                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"xScale\"";
                stream() << "to: " << keyFrame.values.at(i * 3);
                m_indentLevel--;
                stream() << "}";

                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"yScale\"";
                stream() << "to: " << keyFrame.values.at(i * 3 + 1);
                m_indentLevel--;
                stream() << "}";
                break;
            case QTransform::TxRotate:
                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"origin\"";
                stream() << "to: Qt.vector3d(" << keyFrame.values.at(i * 3) << ", " << keyFrame.values.at(i * 3 + 1) << ", 0.0)";
                m_indentLevel--;
                stream() << "}";

                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"angle\"";
                stream() << "to: " << keyFrame.values.at(i * 3 + 2);
                m_indentLevel--;
                stream() << "}";
                break;
            case QTransform::TxShear:
                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"xAngle\"";
                stream() << "to: " << keyFrame.values.at(i * 3);
                m_indentLevel--;
                stream() << "}";

                stream() << "PropertyAnimation {";
                m_indentLevel++;
                stream() << "duration: " << frameTime;
                stream() << "target: " << targetName << "_transform_" << i;
                stream() << "property: \"yAngle\"";
                stream() << "to: " << keyFrame.values.at(i * 3 + 1);
                m_indentLevel--;
                stream() << "}";
                break;
            default:
                Q_UNREACHABLE();
            }
        }

        if (!info.isDefaultTransform) {
            stream() << "PropertyAction {";
            m_indentLevel++;
            stream() << "target: " << targetName << "_transform_base";
            stream() << "property: \"matrix\"";
            stream() << "value: ";
            generateTransform(keyFrame.baseMatrix);

            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}";

    m_indentLevel--;
    stream() << "}";
}

void QQuickQmlGenerator::generateAnimateColor(const QString &targetName,
                                              const QString &propertyName,
                                              const NodeInfo::AnimateColor &animateColor,
                                              const QColor &resetColor)
{
    stream() << "SequentialAnimation {";
    m_indentLevel++;
    stream() << "running: true";

    if (animateColor.start > 0.0) {
        stream() << "PauseAnimation {";
        m_indentLevel++;
        stream() << "duration: " << animateColor.start;
        m_indentLevel--;
        stream() << "}";
    }

    // Sequential animation for key frames
    stream() << "SequentialAnimation {";
    if (animateColor.repeatCount < 0)
        stream() << "loops: Animation.Infinite";
    else
        stream() << "loops: " << animateColor.repeatCount;

    m_indentLevel++;

    for (const auto &keyFrame : animateColor.keyFrames) {
        stream() << "ColorAnimation {";
        m_indentLevel++;

        stream() << "target: " << targetName;
        stream() << "property: \"" << propertyName << "\"";
        stream() << "to: \"" << keyFrame.second.name(QColor::HexArgb) << '"';
        stream() << "duration: " << keyFrame.first;

        m_indentLevel--;
        stream() << "}";
    }

    m_indentLevel--;
    stream() << "}"; // SequentialAnimation

    if (!animateColor.freeze) {
        stream() << "ScriptAction {";
        m_indentLevel++;
        stream() << "script: " << targetName << "." << propertyName << " = \"";
        stream(SameLine) << resetColor.name(QColor::HexArgb);
        stream(SameLine) << "\"";
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

    if (info.stage == StructureNodeStage::Start) {
        if (!info.forceSeparatePaths && info.isPathContainer) {
            generatePathContainer(info);
        } else {
            stream() << "Item {";
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
        m_inShapeItem = false;
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
        stream() << "import QtQuick.Shapes" << Qt::endl;
        stream() << "Item {";
        m_indentLevel++;

        double w = info.size.width();
        double h = info.size.height();
        if (w > 0)
            stream() << "implicitWidth: " << w;
        if (h > 0)
            stream() << "implicitHeight: " << h;

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
            generatePathContainer(info);
            m_indentLevel++;
        }

        generateNodeBase(info);
    } else {
        if (m_inShapeItem) {
            m_inShapeItem = false;
            m_indentLevel--;
            stream() << "}";
        }

        m_indentLevel--;
        stream() << "}";
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
