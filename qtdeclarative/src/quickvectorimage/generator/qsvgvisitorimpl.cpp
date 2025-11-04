// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgvisitorimpl_p.h"
#include "qquickgenerator_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsvgvisitor_p.h>

#include <QString>
#include <QPainter>
#include <QTextDocument>
#include <QTextLayout>
#include <QMatrix4x4>
#include <QQuickItem>

#include <private/qquickshape_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickitem_p.h>

#include <private/qquickimagebase_p_p.h>
#include <private/qquickimage_p.h>
#include <private/qsgcurveprocessor_p.h>

#include <private/qquadpath_p.h>

#include <QtCore/private/qstringiterator_p.h>

#include "utils_p.h"
#include <QtCore/qloggingcategory.h>

#include <QtSvg/private/qsvgstyle_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcVectorImageAnimations, "qt.quick.vectorimage.animations")

using namespace Qt::StringLiterals;

class QSvgStyleResolver
{
public:
    QSvgStyleResolver()
    {
        m_dummyImage = QImage(1, 1, QImage::Format_RGB32);
        m_dummyPainter.begin(&m_dummyImage);
        QPen defaultPen(Qt::NoBrush, 1, Qt::SolidLine, Qt::FlatCap, Qt::SvgMiterJoin);
        defaultPen.setMiterLimit(4);
        m_dummyPainter.setPen(defaultPen);
        m_dummyPainter.setBrush(Qt::black);
    }

    ~QSvgStyleResolver()
    {
        m_dummyPainter.end();
    }

    QPainter& painter() { return m_dummyPainter; }
    QSvgExtraStates& states() { return m_svgState; }

    QColor currentFillColor() const
    {
        if (m_dummyPainter.brush().style() == Qt::NoBrush ||
            m_dummyPainter.brush().color() == QColorConstants::Transparent) {
            return QColor(QColorConstants::Transparent);
        }

        QColor fillColor;
        fillColor = m_dummyPainter.brush().color();
        fillColor.setAlphaF(m_svgState.fillOpacity);

        return fillColor;
    }

    qreal currentFillOpacity() const
    {
        return m_svgState.fillOpacity;
    }

    const QGradient *currentStrokeGradient() const
    {
        QBrush brush = m_dummyPainter.pen().brush();
        if (brush.style() == Qt::LinearGradientPattern
                || brush.style() == Qt::RadialGradientPattern
                || brush.style() == Qt::ConicalGradientPattern) {
            return brush.gradient();
        }
        return nullptr;
    }

    const QGradient *currentFillGradient() const
    {
        if (m_dummyPainter.brush().style() == Qt::LinearGradientPattern || m_dummyPainter.brush().style() == Qt::RadialGradientPattern || m_dummyPainter.brush().style() == Qt::ConicalGradientPattern )
            return m_dummyPainter.brush().gradient();
        return nullptr;
    }

    QTransform currentFillTransform() const
    {
        return m_dummyPainter.brush().transform();
    }

    QColor currentStrokeColor() const
    {
        if (m_dummyPainter.pen().brush().style() == Qt::NoBrush ||
            m_dummyPainter.pen().brush().color() == QColorConstants::Transparent) {
            return QColor(QColorConstants::Transparent);
        }

        QColor strokeColor;
        strokeColor = m_dummyPainter.pen().brush().color();
        strokeColor.setAlphaF(m_svgState.strokeOpacity);

        return strokeColor;
    }

    static QGradient applyOpacityToGradient(const QGradient &gradient, float opacity)
    {
        QGradient grad = gradient;
        QGradientStops stops;
        for (auto &stop : grad.stops()) {
            stop.second.setAlphaF(stop.second.alphaF() * opacity);
            stops.append(stop);
        }

        grad.setStops(stops);

        return grad;
    }

    float currentStrokeWidth() const
    {
        float penWidth = m_dummyPainter.pen().widthF();
        return penWidth ? penWidth : 1;
    }

    QPen currentStroke() const
    {
        return m_dummyPainter.pen();
    }

protected:
    QPainter m_dummyPainter;
    QImage m_dummyImage;
    QSvgExtraStates m_svgState;
};

Q_GLOBAL_STATIC(QSvgStyleResolver, styleResolver)

namespace {
inline bool isPathContainer(const QSvgStructureNode *node)
{
    bool foundPath = false;
    for (const auto *child : node->renderers()) {
        switch (child->type()) {
            // nodes that shouldn't go inside Shape{}
        case QSvgNode::Switch:
        case QSvgNode::Doc:
        case QSvgNode::Group:
        case QSvgNode::AnimateColor:
        case QSvgNode::AnimateTransform:
        case QSvgNode::Use:
        case QSvgNode::Video:
        case QSvgNode::Image:
        case QSvgNode::Textarea:
        case QSvgNode::Text:
        case QSvgNode::Tspan:
            //qCDebug(lcQuickVectorGraphics) << "NOT path container because" << node->typeName() ;
            return false;

            // nodes that could go inside Shape{}
        case QSvgNode::Defs:
            break;

            // nodes that are done as pure ShapePath{}
        case QSvgNode::Rect:
        case QSvgNode::Circle:
        case QSvgNode::Ellipse:
        case QSvgNode::Line:
        case QSvgNode::Path:
        case QSvgNode::Polygon:
        case QSvgNode::Polyline:
        {
            if (!child->style().transform.isDefault()) {
                //qCDebug(lcQuickVectorGraphics) << "NOT path container because local transform";
                return false;
            }
            const QList<QSvgAbstractAnimation *> animations = child->document()->animator()->animationsForNode(child);

            bool hasTransformAnimation = false;
            for (const QSvgAbstractAnimation *animation : animations) {
                const QList<QSvgAbstractAnimatedProperty *> properties = animation->properties();
                for (const QSvgAbstractAnimatedProperty *property : properties) {
                    if (property->type() == QSvgAbstractAnimatedProperty::Transform) {
                        hasTransformAnimation = true;
                        break;
                    }
                }

                if (hasTransformAnimation)
                    break;
            }

            if (hasTransformAnimation) {
                //qCDebug(lcQuickVectorGraphics) << "NOT path container because local transform animation";
                return false;
            }
            foundPath = true;
            break;
        }
        default:
            qCDebug(lcQuickVectorImage) << "Unhandled type in switch" << child->type();
            break;
        }
    }
    //qCDebug(lcQuickVectorGraphics) << "Container" << node->nodeId() << node->typeName()  << "is" << foundPath;
    return foundPath;
}

static QString capStyleName(Qt::PenCapStyle style)
{
    QString styleName;

    switch (style) {
    case Qt::SquareCap:
        styleName = QStringLiteral("squarecap");
        break;
    case Qt::FlatCap:
        styleName = QStringLiteral("flatcap");
        break;
    case Qt::RoundCap:
        styleName = QStringLiteral("roundcap");
        break;
    default:
        break;
    }

    return styleName;
}

static QString joinStyleName(Qt::PenJoinStyle style)
{
    QString styleName;

    switch (style) {
    case Qt::MiterJoin:
        styleName = QStringLiteral("miterjoin");
        break;
    case Qt::BevelJoin:
        styleName = QStringLiteral("beveljoin");
        break;
    case Qt::RoundJoin:
        styleName = QStringLiteral("roundjoin");
        break;
    case Qt::SvgMiterJoin:
        styleName = QStringLiteral("svgmiterjoin");
        break;
    default:
        break;
    }

    return styleName;
}

static QString dashArrayString(QList<qreal> dashArray)
{
    if (dashArray.isEmpty())
        return QString();

    QString dashArrayString;
    QTextStream stream(&dashArrayString);

    for (int i = 0; i < dashArray.length() - 1; i++) {
        qreal value = dashArray[i];
        stream << value << ", ";
    }

    stream << dashArray.last();

    return dashArrayString;
}
};

QSvgVisitorImpl::QSvgVisitorImpl(const QString svgFileName, QQuickGenerator *generator)
    : m_svgFileName(svgFileName)
    , m_generator(generator)
{
}

bool QSvgVisitorImpl::traverse()
{
    if (!m_generator) {
        qCDebug(lcQuickVectorImage) << "No valid QQuickGenerator is set. Genration will stop";
        return false;
    }

    auto *doc = QSvgTinyDocument::load(m_svgFileName);
    if (!doc) {
        qCDebug(lcQuickVectorImage) << "Not a valid Svg File : " << m_svgFileName;
        return false;
    }

    QSvgVisitor::traverse(doc);
    return true;
}

void QSvgVisitorImpl::visitNode(const QSvgNode *node)
{
    handleBaseNodeSetup(node);

    NodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    m_generator->generateNode(info);

    handleBaseNodeEnd(node);
}

void QSvgVisitorImpl::visitImageNode(const QSvgImage *node)
{
    // TODO: this requires proper asset management.
    handleBaseNodeSetup(node);

    ImageNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);
    info.image = node->image();
    info.rect = node->rect();
    info.externalFileReference = node->filename();

    m_generator->generateImageNode(info);

    handleBaseNodeEnd(node);
}

void QSvgVisitorImpl::visitRectNode(const QSvgRect *node)
{
    QRectF rect = node->rect();
    QPointF rads = node->radius();
    // This is using Qt::RelativeSize semantics: percentage of half rect size
    qreal x1 = rect.left();
    qreal x2 = rect.right();
    qreal y1 = rect.top();
    qreal y2 = rect.bottom();

    qreal rx =  rads.x() * rect.width() / 200;
    qreal ry = rads.y() * rect.height() / 200;
    QPainterPath p;

    p.moveTo(x1 + rx, y1);
    p.lineTo(x2 - rx, y1);
    // qCDebug(lcQuickVectorGraphics) << "Line1" << x2 - rx << y1;
    p.arcTo(x2 - rx * 2, y1, rx * 2, ry * 2, 90, -90); // ARC to x2, y1 + ry
    // qCDebug(lcQuickVectorGraphics) << "p1" << p;

    p.lineTo(x2, y2 - ry);
    p.arcTo(x2 - rx * 2, y2 - ry * 2, rx * 2, ry * 2, 0, -90); // ARC to x2 - rx, y2

    p.lineTo(x1 + rx, y2);
    p.arcTo(x1, y2 - ry * 2, rx * 2, ry * 2, 270, -90); // ARC to x1, y2 - ry

    p.lineTo(x1, y1 + ry);
    p.arcTo(x1, y1, rx * 2, ry * 2, 180, -90); // ARC to x1 + rx, y1

    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitEllipseNode(const QSvgEllipse *node)
{
    QRectF rect = node->rect();

    QPainterPath p;
    p.addEllipse(rect);

    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPathNode(const QSvgPath *node)
{
    handlePathNode(node, node->path());
}

void QSvgVisitorImpl::visitLineNode(const QSvgLine *node)
{
    QPainterPath p;
    p.moveTo(node->line().p1());
    p.lineTo(node->line().p2());
    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPolygonNode(const QSvgPolygon *node)
{
    QPainterPath p = QQuickVectorImageGenerator::Utils::polygonToPath(node->polygon(), true);
    handlePathNode(node, p);
}

void QSvgVisitorImpl::visitPolylineNode(const QSvgPolyline *node)
{
    QPainterPath p = QQuickVectorImageGenerator::Utils::polygonToPath(node->polygon(), false);
    handlePathNode(node, p);
}

QString QSvgVisitorImpl::gradientCssDescription(const QGradient *gradient)
{
    QString cssDescription;
    if (gradient->type() == QGradient::LinearGradient) {
        const QLinearGradient *linearGradient = static_cast<const QLinearGradient *>(gradient);

        cssDescription += " -qt-foreground: qlineargradient("_L1;
        cssDescription += "x1:"_L1 + QString::number(linearGradient->start().x()) + u',';
        cssDescription += "y1:"_L1 + QString::number(linearGradient->start().y()) + u',';
        cssDescription += "x2:"_L1 + QString::number(linearGradient->finalStop().x()) + u',';
        cssDescription += "y2:"_L1 + QString::number(linearGradient->finalStop().y()) + u',';
    } else if (gradient->type() == QGradient::RadialGradient) {
        const QRadialGradient *radialGradient = static_cast<const QRadialGradient *>(gradient);

        cssDescription += " -qt-foreground: qradialgradient("_L1;
        cssDescription += "cx:"_L1 + QString::number(radialGradient->center().x()) + u',';
        cssDescription += "cy:"_L1 + QString::number(radialGradient->center().y()) + u',';
        cssDescription += "fx:"_L1 + QString::number(radialGradient->focalPoint().x()) + u',';
        cssDescription += "fy:"_L1 + QString::number(radialGradient->focalPoint().y()) + u',';
        cssDescription += "radius:"_L1 + QString::number(radialGradient->radius()) + u',';
    } else {
        const QConicalGradient *conicalGradient = static_cast<const QConicalGradient *>(gradient);

        cssDescription += " -qt-foreground: qconicalgradient("_L1;
        cssDescription += "cx:"_L1 + QString::number(conicalGradient->center().x()) + u',';
        cssDescription += "cy:"_L1 + QString::number(conicalGradient->center().y()) + u',';
        cssDescription += "angle:"_L1 + QString::number(conicalGradient->angle()) + u',';
    }

    const QStringList coordinateModes = { "logical"_L1, "stretchtodevice"_L1, "objectbounding"_L1, "object"_L1 };
    cssDescription += "coordinatemode:"_L1;
    cssDescription += coordinateModes.at(int(gradient->coordinateMode()));
    cssDescription += u',';

    const QStringList spreads = { "pad"_L1, "reflect"_L1, "repeat"_L1 };
    cssDescription += "spread:"_L1;
    cssDescription += spreads.at(int(gradient->spread()));

    for (const QGradientStop &stop : gradient->stops()) {
        cssDescription += ",stop:"_L1;
        cssDescription += QString::number(stop.first);
        cssDescription += u' ';
        cssDescription += stop.second.name(QColor::HexArgb);
    }

    cssDescription += ");"_L1;

    return cssDescription;
}

QString QSvgVisitorImpl::colorCssDescription(QColor color)
{
    QString cssDescription;
    cssDescription += QStringLiteral("rgba(");
    cssDescription += QString::number(color.red()) + QStringLiteral(",");
    cssDescription += QString::number(color.green()) + QStringLiteral(",");
    cssDescription += QString::number(color.blue()) + QStringLiteral(",");
    cssDescription += QString::number(color.alphaF()) + QStringLiteral(")");

    return cssDescription;
}

namespace {

    // Simple class for representing the SVG font as a font engine
    // We use the Proxy font engine type, which is currently unused and does not map to
    // any specific font engine
    // (The QSvgFont object must outlive the engine.)
    class QSvgFontEngine : public QFontEngine
    {
    public:
        QSvgFontEngine(const QSvgFont *font, qreal size);

        QFontEngine *cloneWithSize(qreal size) const override;

        glyph_t glyphIndex(uint ucs4) const override;
        int stringToCMap(const QChar *str,
                         int len,
                         QGlyphLayout *glyphs,
                         int *nglyphs,
                         ShaperFlags flags) const override;

        void addGlyphsToPath(glyph_t *glyphs,
                             QFixedPoint *positions,
                             int nGlyphs,
                             QPainterPath *path,
                             QTextItem::RenderFlags flags) override;

        glyph_metrics_t boundingBox(glyph_t glyph) override;

        void recalcAdvances(QGlyphLayout *, ShaperFlags) const override;
        QFixed ascent() const override;
        QFixed capHeight() const override;
        QFixed descent() const override;
        QFixed leading() const override;
        qreal maxCharWidth() const override;
        qreal minLeftBearing() const override;
        qreal minRightBearing() const override;

        QFixed emSquareSize() const override;

    private:
        const QSvgFont *m_font;
    };

    QSvgFontEngine::QSvgFontEngine(const QSvgFont *font, qreal size)
        : QFontEngine(Proxy)
        , m_font(font)
    {
        fontDef.pixelSize = size;
        fontDef.families = QStringList(m_font->m_familyName);
    }

    QFixed QSvgFontEngine::emSquareSize() const
    {
        return QFixed::fromReal(m_font->m_unitsPerEm);
    }

    glyph_t QSvgFontEngine::glyphIndex(uint ucs4) const
    {
        if (ucs4 < USHRT_MAX && m_font->m_glyphs.contains(QChar(ushort(ucs4))))
            return glyph_t(ucs4);

        return 0;
    }

    int QSvgFontEngine::stringToCMap(const QChar *str,
                                     int len,
                                     QGlyphLayout *glyphs,
                                     int *nglyphs,
                                     ShaperFlags flags) const
    {
        Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
        if (*nglyphs < len) {
            *nglyphs = len;
            return -1;
        }

        int ucs4Length = 0;
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            char32_t ucs4 = it.next();
            glyph_t index = glyphIndex(ucs4);
            glyphs->glyphs[ucs4Length++] = index;
        }

        *nglyphs = ucs4Length;
        glyphs->numGlyphs = ucs4Length;

        if (!(flags & GlyphIndicesOnly))
            recalcAdvances(glyphs, flags);

        return *nglyphs;
    }

    void QSvgFontEngine::addGlyphsToPath(glyph_t *glyphs,
                                         QFixedPoint *positions,
                                         int nGlyphs,
                                         QPainterPath *path,
                                         QTextItem::RenderFlags flags)
    {
        Q_UNUSED(flags);
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        for (int i = 0; i < nGlyphs; ++i) {
            glyph_t index = glyphs[i];
            if (index > 0) {
                QPointF position = positions[i].toPointF();
                QPainterPath glyphPath = m_font->m_glyphs.value(QChar(ushort(index))).m_path;

                QTransform xform;
                xform.translate(position.x(), position.y());
                xform.scale(scale, -scale);
                glyphPath = xform.map(glyphPath);
                path->addPath(glyphPath);
            }
        }
    }

    glyph_metrics_t QSvgFontEngine::boundingBox(glyph_t glyph)
    {
        glyph_metrics_t ret;
        ret.x = 0; // left bearing
        ret.y = -ascent();
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        const QSvgGlyph &svgGlyph = m_font->m_glyphs.value(QChar(ushort(glyph)));
        ret.width = QFixed::fromReal(svgGlyph.m_horizAdvX * scale);
        ret.height = ascent() + descent();
        return ret;
    }

    QFontEngine *QSvgFontEngine::cloneWithSize(qreal size) const
    {
        QSvgFontEngine *otherEngine = new QSvgFontEngine(m_font, size);
        return otherEngine;
    }

    void QSvgFontEngine::recalcAdvances(QGlyphLayout *glyphLayout, ShaperFlags) const
    {
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        for (int i = 0; i < glyphLayout->numGlyphs; i++) {
            glyph_t glyph = glyphLayout->glyphs[i];
            const QSvgGlyph &svgGlyph = m_font->m_glyphs.value(QChar(ushort(glyph)));
            glyphLayout->advances[i] = QFixed::fromReal(svgGlyph.m_horizAdvX * scale);
        }
    }

    QFixed QSvgFontEngine::ascent() const
    {
        return QFixed::fromReal(fontDef.pixelSize);
    }

    QFixed QSvgFontEngine::capHeight() const
    {
        return ascent();
    }
    QFixed QSvgFontEngine::descent() const
    {
        return QFixed{};
    }

    QFixed QSvgFontEngine::leading() const
    {
        return QFixed{};
    }

    qreal QSvgFontEngine::maxCharWidth() const
    {
        const qreal scale = fontDef.pixelSize / m_font->m_unitsPerEm;
        return m_font->m_horizAdvX * scale;
    }

    qreal QSvgFontEngine::minLeftBearing() const
    {
        return 0.0;
    }

    qreal QSvgFontEngine::minRightBearing() const
    {
        return 0.0;
    }
}

void QSvgVisitorImpl::visitTextNode(const QSvgText *node)
{
    handleBaseNodeSetup(node);
    const bool isTextArea = node->type() == QSvgNode::Textarea;

    QString text;
    const QSvgFont *svgFont = styleResolver->states().svgFont;
    bool needsRichText = false;
    bool preserveWhiteSpace = node->whitespaceMode() == QSvgText::Preserve;
    const QGradient *mainGradient = styleResolver->currentFillGradient();

    QFontEngine *fontEngine = nullptr;
    if (svgFont != nullptr) {
        fontEngine = new QSvgFontEngine(svgFont, styleResolver->painter().font().pointSize());
        fontEngine->ref.ref();
    }

#if QT_CONFIG(texthtmlparser)
    bool needsPathNode = mainGradient != nullptr
                           || svgFont != nullptr
                           || styleResolver->currentStrokeGradient() != nullptr;
#endif
    for (const auto *tspan : node->tspans()) {
        if (!tspan) {
            text += QStringLiteral("<br>");
            continue;
        }

        // Note: We cannot get the font directly from the style, since this does
        // not apply the weight, since this is relative and depends on current state.
        handleBaseNodeSetup(tspan);
        QFont font = styleResolver->painter().font();

        QString styleTagContent;

        if ((font.resolveMask() & QFont::FamilyResolved)
            || (font.resolveMask() & QFont::FamiliesResolved)) {
            styleTagContent += QStringLiteral("font-family: %1;").arg(font.family());
        }

        if (font.resolveMask() & QFont::WeightResolved
            && font.weight() != QFont::Normal
            && font.weight() != QFont::Bold) {
            styleTagContent += QStringLiteral("font-weight: %1;").arg(int(font.weight()));
        }

        if (font.resolveMask() & QFont::SizeResolved) {
            // Pixel size stored as point size in SVG parser
            styleTagContent += QStringLiteral("font-size: %1px;").arg(int(font.pointSizeF()));
        }

        if (font.resolveMask() & QFont::CapitalizationResolved
            && font.capitalization() == QFont::SmallCaps) {
            styleTagContent += QStringLiteral("font-variant: small-caps;");
        }

        if (styleResolver->currentFillGradient() != nullptr
            && styleResolver->currentFillGradient() != mainGradient) {
            const QGradient grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());
            styleTagContent += gradientCssDescription(&grad) + u';';
#if QT_CONFIG(texthtmlparser)
            needsPathNode = true;
#endif
        }

        const QColor currentStrokeColor = styleResolver->currentStrokeColor();
        if (currentStrokeColor.alpha() > 0) {
            QString strokeColor = colorCssDescription(currentStrokeColor);
            styleTagContent += QStringLiteral("-qt-stroke-color:%1;").arg(strokeColor);
            styleTagContent += QStringLiteral("-qt-stroke-width:%1px;").arg(styleResolver->currentStrokeWidth());
            styleTagContent += QStringLiteral("-qt-stroke-dasharray:%1;").arg(dashArrayString(styleResolver->currentStroke().dashPattern()));
            styleTagContent += QStringLiteral("-qt-stroke-dashoffset:%1;").arg(styleResolver->currentStroke().dashOffset());
            styleTagContent += QStringLiteral("-qt-stroke-lineCap:%1;").arg(capStyleName(styleResolver->currentStroke().capStyle()));
            styleTagContent += QStringLiteral("-qt-stroke-lineJoin:%1;").arg(joinStyleName(styleResolver->currentStroke().joinStyle()));
            if (styleResolver->currentStroke().joinStyle() == Qt::MiterJoin || styleResolver->currentStroke().joinStyle() == Qt::SvgMiterJoin)
                styleTagContent += QStringLiteral("-qt-stroke-miterlimit:%1;").arg(styleResolver->currentStroke().miterLimit());
#if QT_CONFIG(texthtmlparser)
            needsPathNode = true;
#endif
        }

        if (tspan->whitespaceMode() == QSvgText::Preserve && !preserveWhiteSpace)
            styleTagContent += QStringLiteral("white-space: pre-wrap;");

        QString content = tspan->text().toHtmlEscaped();
        content.replace(QLatin1Char('\t'), QLatin1Char(' '));
        content.replace(QLatin1Char('\n'), QLatin1Char(' '));

        bool fontTag = false;
        if (!tspan->style().fill.isDefault()) {
            auto &b = tspan->style().fill->qbrush();
            qCDebug(lcQuickVectorImage) << "tspan FILL:" << b;
            if (b.style() != Qt::NoBrush)
            {
                if (qFuzzyCompare(b.color().alphaF() + 1.0, 2.0))
                {
                    QString spanColor = b.color().name();
                    fontTag = !spanColor.isEmpty();
                    if (fontTag)
                        text += QStringLiteral("<font color=\"%1\">").arg(spanColor);
                } else {
                    QString spanColor = colorCssDescription(b.color());
                    styleTagContent += QStringLiteral("color:%1").arg(spanColor);
                }
            }
        }

        needsRichText = needsRichText || !styleTagContent.isEmpty();
        if (!styleTagContent.isEmpty())
            text += QStringLiteral("<span style=\"%1\">").arg(styleTagContent);

        if (font.resolveMask() & QFont::WeightResolved && font.bold())
            text += QStringLiteral("<b>");

        if (font.resolveMask() & QFont::StyleResolved && font.italic())
            text += QStringLiteral("<i>");

        if (font.resolveMask() & QFont::CapitalizationResolved) {
            switch (font.capitalization()) {
            case QFont::AllLowercase:
                content = content.toLower();
                break;
            case QFont::AllUppercase:
                content = content.toUpper();
                break;
            case QFont::Capitalize:
                // ### We need to iterate over the string and do the title case conversion,
                // since this is not part of QString.
                qCWarning(lcQuickVectorImage) << "Title case not implemented for tspan";
                break;
            default:
                break;
            }
        }
        text += content;
        if (fontTag)
            text += QStringLiteral("</font>");

        if (font.resolveMask() & QFont::StyleResolved && font.italic())
            text += QStringLiteral("</i>");

        if (font.resolveMask() & QFont::WeightResolved && font.bold())
            text += QStringLiteral("</b>");

        if (!styleTagContent.isEmpty())
            text += QStringLiteral("</span>");

        handleBaseNodeEnd(tspan);
    }

    if (preserveWhiteSpace && (needsRichText || styleResolver->currentFillGradient() != nullptr))
        text = QStringLiteral("<span style=\"white-space: pre-wrap\">") + text + QStringLiteral("</span>");

    QFont font = styleResolver->painter().font();
    if (font.pixelSize() <= 0 && font.pointSize() > 0)
        font.setPixelSize(font.pointSize()); // Pixel size stored as point size by SVG parser

    font.setHintingPreference(QFont::PreferNoHinting);

#if QT_CONFIG(texthtmlparser)
    if (needsPathNode) {
        QTextDocument document;
        document.setHtml(text);
        if (isTextArea && node->size().width() > 0)
            document.setTextWidth(node->size().width());
        document.setDefaultFont(font);
        document.pageCount(); // Force layout

        QTextBlock block = document.firstBlock();
        while (block.isValid()) {
            QTextLayout *lout = block.layout();

            if (lout != nullptr) {
                QRectF boundingRect = lout->boundingRect();

                // If this block has requested the current SVG font, we override it
                // (note that this limits the text to one svg font, but this is also the case
                // in the QPainter at the moment, and needs a more centralized solution in Qt Svg
                // first)
                QFont blockFont = block.charFormat().font();
                if (svgFont != nullptr
                    && blockFont.family() == svgFont->m_familyName) {
                    QRawFont rawFont;
                    QRawFontPrivate *rawFontD = QRawFontPrivate::get(rawFont);
                    rawFontD->setFontEngine(fontEngine->cloneWithSize(blockFont.pixelSize()));

                    lout->setRawFont(rawFont);
                }

                auto addPathForFormat = [&](QPainterPath p, QTextCharFormat fmt) {
                    PathNodeInfo info;
                    fillCommonNodeInfo(node, info);
                    fillAnimationInfo(node, info);
                    auto fillStyle = node->style().fill;
                    if (fillStyle)
                        info.fillRule = fillStyle->fillRule();

                    if (fmt.hasProperty(QTextCharFormat::ForegroundBrush)) {
                        info.fillColor = fmt.foreground().color();
                        if (fmt.foreground().gradient() != nullptr && fmt.foreground().gradient()->type() != QGradient::NoGradient)
                            info.grad = *fmt.foreground().gradient();
                    } else {
                        info.fillColor = styleResolver->currentFillColor();
                    }

                    info.painterPath = p;

                    const QGradient *strokeGradient = styleResolver->currentStrokeGradient();
                    QPen pen;
                    if (fmt.hasProperty(QTextCharFormat::TextOutline)) {
                        pen = fmt.textOutline();
                        if (strokeGradient == nullptr) {
                            info.strokeStyle = StrokeStyle::fromPen(pen);
                            info.strokeStyle.color = pen.color();
                        }
                    } else {
                        pen = styleResolver->currentStroke();
                        if (strokeGradient == nullptr) {
                            info.strokeStyle = StrokeStyle::fromPen(pen);
                            info.strokeStyle.color = styleResolver->currentStrokeColor();
                        }
                    }

                    if (info.grad.type() == QGradient::NoGradient && styleResolver->currentFillGradient() != nullptr)
                        info.grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());

                    info.fillTransform = styleResolver->currentFillTransform();

                    m_generator->generatePath(info, boundingRect);

                    if (strokeGradient != nullptr) {
                        PathNodeInfo strokeInfo;
                        fillCommonNodeInfo(node, strokeInfo);
                        fillAnimationInfo(node, strokeInfo);

                        strokeInfo.grad = *strokeGradient;

                        QPainterPathStroker stroker(pen);
                        strokeInfo.painterPath = stroker.createStroke(p);
                        m_generator->generatePath(strokeInfo, boundingRect);
                    }
                };

                qreal baselineOffset = -QFontMetricsF(font).ascent();
                if (lout->lineCount() > 0 && lout->lineAt(0).isValid())
                    baselineOffset = -lout->lineAt(0).ascent();

                const QPointF baselineTranslation(0.0, baselineOffset);
                auto glyphsToPath = [&](QList<QGlyphRun> glyphRuns, qreal width) {
                    QList<QPainterPath> paths;
                    for (const QGlyphRun &glyphRun : glyphRuns) {
                        QRawFont font = glyphRun.rawFont();
                        QList<quint32> glyphIndexes = glyphRun.glyphIndexes();
                        QList<QPointF> positions = glyphRun.positions();

                        for (qsizetype j = 0; j < glyphIndexes.size(); ++j) {
                            quint32 glyphIndex = glyphIndexes.at(j);
                            const QPointF &pos = positions.at(j);

                            QPainterPath p = font.pathForGlyph(glyphIndex);
                            p.translate(pos + node->position() + baselineTranslation);
                            if (styleResolver->states().textAnchor == Qt::AlignHCenter)
                                p.translate(QPointF(-0.5 * width, 0));
                            else if (styleResolver->states().textAnchor == Qt::AlignRight)
                                p.translate(QPointF(-width, 0));
                            paths.append(p);
                        }
                    }

                    return paths;
                };

                QList<QTextLayout::FormatRange> formats = block.textFormats();
                for (int i = 0; i < formats.size(); ++i) {
                    QTextLayout::FormatRange range = formats.at(i);

                    QList<QGlyphRun> glyphRuns = lout->glyphRuns(range.start, range.length);
                    QList<QPainterPath> paths = glyphsToPath(glyphRuns, lout->minimumWidth());
                    for (const QPainterPath &path : paths)
                        addPathForFormat(path, range.format);
                }
            }

            block = block.next();
        }
    } else
#endif
    {
        TextNodeInfo info;
        fillCommonNodeInfo(node, info);
        fillAnimationInfo(node, info);

        info.position = node->position();
        info.size = node->size();
        info.font = font;
        info.text = text;
        info.isTextArea = isTextArea;
        info.needsRichText = needsRichText;
        info.fillColor = styleResolver->currentFillColor();
        info.alignment = styleResolver->states().textAnchor;
        info.strokeColor = styleResolver->currentStrokeColor();

        m_generator->generateTextNode(info);
    }

    handleBaseNodeEnd(node);

    if (fontEngine != nullptr) {
        fontEngine->ref.deref();
        Q_ASSERT(fontEngine->ref.loadRelaxed() == 0);
        delete fontEngine;
    }
}

void QSvgVisitorImpl::visitUseNode(const QSvgUse *node)
{
    QSvgNode *link = node->link();
    if (!link)
        return;

    handleBaseNodeSetup(node);
    UseNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    info.stage = StructureNodeStage::Start;
    info.startPos = node->start();

    m_generator->generateUseNode(info);

    QSvgVisitor::traverse(link);

    info.stage = StructureNodeStage::End;
    m_generator->generateUseNode(info);
    handleBaseNodeEnd(node);
}

bool QSvgVisitorImpl::visitSwitchNodeStart(const QSvgSwitch *node)
{
    QSvgNode *link = node->childToRender();
    if (!link)
        return false;

    QSvgVisitor::traverse(link);

    return false;
}

void QSvgVisitorImpl::visitSwitchNodeEnd(const QSvgSwitch *node)
{
    Q_UNUSED(node);
}

bool QSvgVisitorImpl::visitDefsNodeStart(const QSvgDefs *node)
{
    Q_UNUSED(node)

    return m_generator->generateDefsNode(NodeInfo{});
}

bool QSvgVisitorImpl::visitStructureNodeStart(const QSvgStructureNode *node)
{
    constexpr bool forceSeparatePaths = false;
    handleBaseNodeSetup(node);

    StructureNodeInfo info;

    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);
    info.forceSeparatePaths = forceSeparatePaths;
    info.isPathContainer = isPathContainer(node);
    info.stage = StructureNodeStage::Start;

    return m_generator->generateStructureNode(info);
}

void QSvgVisitorImpl::visitStructureNodeEnd(const QSvgStructureNode *node)
{
    handleBaseNodeEnd(node);
    //    qCDebug(lcQuickVectorGraphics) << "REVERT" << node->nodeId() << node->type() << (m_styleResolver->painter().pen().style() != Qt::NoPen) << m_styleResolver->painter().pen().color().name()
    //             << (m_styleResolver->painter().pen().brush().style() != Qt::NoBrush) << m_styleResolver->painter().pen().brush().color().name();

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    info.stage = StructureNodeStage::End;

    m_generator->generateStructureNode(info);
}

bool QSvgVisitorImpl::visitDocumentNodeStart(const QSvgTinyDocument *node)
{
    handleBaseNodeSetup(node);

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);

    const QSvgTinyDocument *doc = static_cast<const QSvgTinyDocument *>(node);
    info.size = doc->size();
    info.viewBox = doc->viewBox();
    info.isPathContainer = isPathContainer(node);
    info.forceSeparatePaths = false;
    info.stage = StructureNodeStage::Start;

    return m_generator->generateRootNode(info);
}

void QSvgVisitorImpl::visitDocumentNodeEnd(const QSvgTinyDocument *node)
{
    handleBaseNodeEnd(node);
    qCDebug(lcQuickVectorImage) << "REVERT" << node->nodeId() << node->type() << (styleResolver->painter().pen().style() != Qt::NoPen)
                                   << styleResolver->painter().pen().color().name() << (styleResolver->painter().pen().brush().style() != Qt::NoBrush)
                                   << styleResolver->painter().pen().brush().color().name();

    StructureNodeInfo info;
    fillCommonNodeInfo(node, info);
    info.stage = StructureNodeStage::End;

    m_generator->generateRootNode(info);
}

void QSvgVisitorImpl::fillCommonNodeInfo(const QSvgNode *node, NodeInfo &info)
{
    info.nodeId = node->nodeId();
    info.typeName = node->typeName();
    info.isDefaultTransform = node->style().transform.isDefault();
    info.transform = !info.isDefaultTransform ? node->style().transform->qtransform() : QTransform();
    info.isDefaultOpacity = node->style().opacity.isDefault();
    info.opacity = !info.isDefaultOpacity ? node->style().opacity->opacity() : 1.0;
    info.isVisible = node->isVisible();
    info.isDisplayed = node->displayMode() != QSvgNode::DisplayMode::NoneMode;
}

void QSvgVisitorImpl::fillColorAnimationInfo(const QSvgNode *node, NodeInfo &info)
{
    const QList<QSvgAbstractAnimation *> animations = node->document()->animator()->animationsForNode(node);
    for (const QSvgAbstractAnimation *animation : animations) {
        const QList<QSvgAbstractAnimatedProperty *> properties = animation->properties();
        for (const QSvgAbstractAnimatedProperty *property : properties) {
            if (property->type() == QSvgAbstractAnimatedProperty::Color) {
                const QSvgAnimatedPropertyColor *colorProperty = static_cast<const QSvgAnimatedPropertyColor *>(property);
                const QList<qreal> keyFrames = colorProperty->keyFrames();

                NodeInfo::AnimateColor animateColor;
                animateColor.start = animation->start();
                animateColor.fill = colorProperty->propertyName() == QStringLiteral("fill");
                animateColor.repeatCount = animation->iterationCount();
                animateColor.freeze = animation->animationType() == QSvgAbstractAnimation::SMIL
                                          ? static_cast<const QSvgAnimateNode *>(animation)->fill() == QSvgAnimateNode::Freeze
                                          : true;

                const QList<QColor> colors = colorProperty->colors();
                Q_ASSERT(colors.size() == keyFrames.size());

                for (int i = 0; i < keyFrames.size(); ++i) {
                    qreal timeCode = keyFrames.at(i) * animation->duration();
                    QColor color = colors.at(i);
                    animateColor.keyFrames.append(qMakePair(timeCode, color));
                }

                if (!animateColor.keyFrames.isEmpty())
                    info.animateColors.append(animateColor);
            }
        }
    }
}

void QSvgVisitorImpl::fillTransformAnimationInfo(const QSvgNode *node, NodeInfo &info)
{
    // We convert transform animations into key frames ahead of time, resolving things like
    // freeze, repeat, replace etc. to avoid having to do this in the generators.
    // One complexity here is if some animations repeat indefinitely and others do not.
    // For these, we need to first have the finite animation and then have this be replaced by
    // an infinite animation afterwards.

    // First, we collect all animated properties. We assume that each QSvgAbstractAnimatedProperty
    // only modifies a single property each in the following code.
    QList<QPair<const QSvgAbstractAnimation *, const QSvgAnimatedPropertyTransform *> > animateTransforms;
    const QList<QSvgAbstractAnimation *> animations = node->document()->animator()->animationsForNode(node);
    for (const QSvgAbstractAnimation *animation : animations) {
        const QList<QSvgAbstractAnimatedProperty *> properties = animation->properties();
        for (const QSvgAbstractAnimatedProperty *property : properties) {
            if (property->type() == QSvgAbstractAnimatedProperty::Transform) {
                auto v = qMakePair(animation, static_cast<const QSvgAnimatedPropertyTransform *>(property));
                animateTransforms.append(v);
            }
        }
    }

    if (!animateTransforms.isEmpty()) {
        // If the animation has some animations with a finite repeat count and some that loop
        // infinitely, we split the duration into two: First one part with the duration of the
        // longest finite animation. Then we add an infinitely looping tail at the end.
        // We record the longest finite animation as maxRunningTime and the looping tail duration as
        // infiniteAnimationTail
        int maxRunningTime = 0;
        int infiniteAnimationTail = 0;

        auto &keyFrames = info.transformAnimation.keyFrames;
        for (int i = 0; i < animateTransforms.size(); ++i) {
            const QSvgAbstractAnimation *animation = animateTransforms.at(i).first;
            const QSvgAnimatedPropertyTransform *property = animateTransforms.at(i).second;

            const int start = animation->start();
            const int duration = animation->duration();
            const int iterationCount = animation->iterationCount();
            const int repeatCount = qMax(iterationCount, 1);
            const int runningTime = start + duration * repeatCount;

            const qsizetype translationCount = property->translations().size();
            const qsizetype scaleCount = property->scales().size();
            const qsizetype rotationCount = property->rotations().size();
            const qsizetype skewCount = property->skews().size();

            if (translationCount > 0)
                info.transformAnimation.animationTypes.append(QTransform::TxTranslate);
            else if (scaleCount > 0)
                info.transformAnimation.animationTypes.append(QTransform::TxScale);
            else if (rotationCount > 0)
                info.transformAnimation.animationTypes.append(QTransform::TxRotate);
            else if (skewCount > 0)
                info.transformAnimation.animationTypes.append(QTransform::TxShear);

            maxRunningTime = qMax(maxRunningTime, runningTime);

            // If this animation is looping infinitely, we need to make sure the duration of
            // the infinitely looping tail animation is divisible by its duration, so that it
            // will be able to finish a whole number of repeats before looping. We do this
            // by multiplying the current tail by the duration.
            // (So if there is an infinitely looping animation of 2s and another of 3s then we
            // make the looping part 6s, so that the first loops 3 times and the second 2 times
            // during the length of the animation.)
            if (iterationCount < 0) {
                if (infiniteAnimationTail == 0)
                    infiniteAnimationTail = duration;
                else if (duration == 0 || (infiniteAnimationTail % duration) != 0) {
                    if (duration <= 0 || infiniteAnimationTail >= INT_MAX / duration) {
                        qCWarning(lcVectorImageAnimations)
                            << "Error adding indefinite animation of duration"
                            << duration
                            << "to tail of length"
                            << infiniteAnimationTail;
                    } else {
                        infiniteAnimationTail *= duration;
                    }
                }
            }
        }

        qCDebug(lcVectorImageAnimations) << "Finite running time" << maxRunningTime << "infinite tail" << infiniteAnimationTail;

        // Then we record the key frames. We determine specific positions in the animations where we
        // need to know the state and record all the time codes for these up-front.
        for (int i = 0; i < animateTransforms.size(); ++i) {
            const QSvgAbstractAnimation *animation = animateTransforms.at(i).first;
            const QSvgAnimatedPropertyTransform *property = animateTransforms.at(i).second;

            const int repeatCount = animation->iterationCount();
            const int start = animation->start();
            const int duration = animation->duration();
            const int runningTime = repeatCount > 0 ? start + duration * repeatCount : maxRunningTime;
            const qreal frameLength = qreal(duration) / property->keyFrames().size();

            if (repeatCount > 0) {
                // For animations with a finite number of loops, we record the state right before the
                // animation, at all key frames of the animation for each loop, right before the
                // end of the loop, and at the end of the whole thing
                qreal currentFrameTime = start;
                if (currentFrameTime > 0)
                    keyFrames[QFixed::fromReal(currentFrameTime) - 1] = NodeInfo::TransformAnimation::TransformKeyFrame{};
                for (int j = 0; j < repeatCount; ++j) {
                    for (int k = 0; k < property->keyFrames().size(); ++k) {
                        auto keyFrame = NodeInfo::TransformAnimation::TransformKeyFrame{};
                        keyFrames[QFixed::fromReal(currentFrameTime)] = keyFrame;
                        currentFrameTime += frameLength;
                    }

                    keyFrames[QFixed::fromReal(currentFrameTime) - 1] = NodeInfo::TransformAnimation::TransformKeyFrame{};
                }

                keyFrames[QFixed::fromReal(currentFrameTime)] = NodeInfo::TransformAnimation::TransformKeyFrame{};

                // For animations that end before the full running time, we also add a key frame
                // right after to record the state when it has stopped.
                if (currentFrameTime < maxRunningTime)
                    keyFrames[QFixed::fromReal(currentFrameTime) + 1] = NodeInfo::TransformAnimation::TransformKeyFrame{};
            } else {
                // For animations with infinite repeats, we first do the same as for finite
                // animations during the finite part, and then we add key frames for the infinite
                // tail
                qreal currentFrameTime = start;
                while (currentFrameTime < runningTime) {
                    for (int k = 0; k < property->keyFrames().size(); ++k) {
                        auto keyFrame = NodeInfo::TransformAnimation::TransformKeyFrame{};
                        keyFrames[QFixed::fromReal(currentFrameTime)] = keyFrame;
                        currentFrameTime += frameLength;
                    }
                }

                keyFrames[QFixed::fromReal(currentFrameTime) - 1] = NodeInfo::TransformAnimation::TransformKeyFrame{};

                // For animations that end before the full running time, we also add a key frame
                // right after to record the state when it has stopped.
                if (currentFrameTime < maxRunningTime)
                    keyFrames[QFixed::fromReal(currentFrameTime) + 1] = NodeInfo::TransformAnimation::TransformKeyFrame{};

                // Start infinite portion at 1ms after finite part to make sure we
                // reset the animation to the correct position
                while (currentFrameTime <= runningTime + infiniteAnimationTail) {
                    for (int k = 0; k < property->keyFrames().size(); ++k) {
                        auto keyFrame = NodeInfo::TransformAnimation::TransformKeyFrame{};
                        keyFrames[QFixed::fromReal(currentFrameTime)] = keyFrame;
                        currentFrameTime += frameLength;
                    }

                    keyFrames[QFixed::fromReal(currentFrameTime) - 1] = NodeInfo::TransformAnimation::TransformKeyFrame{};
                }
            }
        }

        // For each keyframe, we iterate over all animations to see if they affect the frame.
        // We record whether a finite animation touches the frame or not. If no finite animation
        // touches the frame, it means we are in the "tail" period after all finite animations
        // have finished and which should be looped indefinitely.
        QTransform baseTransform = info.transform;
        for (auto it = keyFrames.begin(); it != keyFrames.end(); ++it) {
            QFixed timecode = it.key();
            qCDebug(lcVectorImageAnimations) << "Frame at" << timecode;

            if (timecode >= maxRunningTime && infiniteAnimationTail > 0) {
                qCDebug(lcVectorImageAnimations) << "    -> Infinite repeats";
                it.value().indefiniteAnimation = true;
            }

            // The base matrix is the matrix set on the item ahead of time. This will be
            // kept unless a replace animation is active.
            it.value().baseMatrix = baseTransform;

            // Initialize values to default all animations to inactive
            Q_ASSERT(animateTransforms.size() == info.transformAnimation.animationTypes.size());
            for (int i = 0; i < info.transformAnimation.animationTypes.size(); ++i) {
                if (info.transformAnimation.animationTypes.at(i) == QTransform::TxScale)
                    it.value().values.append({ 1.0, 1.0, 0.0 });
                else
                    it.value().values.append({ 0.0, 0.0, 0.0 });
            }

            // For debugging purposes
            QPointF accumulatedScale = QPointF(1.0, 1.0);
            QPointF accumulatedTranslation;
            QPointF accumulatedSkew;
            qreal accumulatedRotation = 0.0;

            // We count backwards so that we only evaluate up until the last active animation
            // that is set to additive==replace
            for (int i = animateTransforms.size() - 1; i >= 0; --i) {
                qCDebug(lcVectorImageAnimations) << "       -> Checking animation" << i;
                const QSvgAbstractAnimation *animation = animateTransforms.at(i).first;
                const QSvgAnimatedPropertyTransform *property = animateTransforms.at(i).second;
                const int start = animation->start();
                const int repeatCount = animation->iterationCount();
                const int duration = animation->duration();
                const int end = start + duration * qMax(1, repeatCount);
                QTransform::TransformationType type = info.transformAnimation.animationTypes.at(i);

                // Does this animation replace all other animations, then we need to clear
                // the base transform
                bool replacesOtherTransforms = true;
                bool freeze = false;
                if (animation->animationType() == QSvgAbstractAnimation::SMIL) {
                    const QSvgAnimateNode *animateNode = static_cast<const QSvgAnimateNode *>(animation);
                    replacesOtherTransforms = animateNode->additiveType() == QSvgAnimateNode::Replace;
                    freeze = animateNode->fill() == QSvgAnimateNode::Freeze;
                }

                qCDebug(lcVectorImageAnimations) << "       -> Start:" << start
                                                 << ", repeatCount:" << repeatCount
                                                 << ", end:" << end
                                                 << ", freeze:" << freeze
                                                 << ", replacesOtherTransforms:" << replacesOtherTransforms;

                // Does it apply to this time code? If not, we skip this animation
                if (QFixed(start) > timecode
                    || (repeatCount > 0 && QFixed(end) < timecode && !freeze)) {
                    qCDebug(lcVectorImageAnimations) << "       -> Skipping" << i;
                    continue;
                }

                QFixed relativeTimeCode = timecode - QFixed(start);
                while (it.value().indefiniteAnimation && relativeTimeCode > duration) {
                    relativeTimeCode -= duration;
                }

                qreal fractionOfTotalTime = relativeTimeCode.toReal() / duration;
                qreal fractionOfCurrentIterationTime = fractionOfTotalTime - std::trunc(fractionOfTotalTime);
                if (timecode >= end && !it.value().indefiniteAnimation)
                    fractionOfCurrentIterationTime = 1.0;

                qCDebug(lcVectorImageAnimations) << "    -> Checking frame at"
                                                 << relativeTimeCode
                                                 << "(fraction of total:"
                                                 << fractionOfTotalTime
                                                 << ", of current iteration:"
                                                 << fractionOfCurrentIterationTime << ")"
                                                 << "animation index:" << i;

                const QList<qreal> propertyKeyFrames = property->keyFrames();

                if (replacesOtherTransforms) {
                    baseTransform = QTransform{};
                    it.value().baseMatrix = QTransform{};
                }

                for (int j = 1; j < propertyKeyFrames.size(); ++j) {
                    qreal from = propertyKeyFrames.at(j - 1);
                    qreal to = propertyKeyFrames.at(j);

                    if (fractionOfCurrentIterationTime >= from && (fractionOfCurrentIterationTime < to || freeze)) {
                        qreal currFraction = (fractionOfCurrentIterationTime - from) / (to - from);

                        if (type == QTransform::TxTranslate) {
                            const QPointF trans = property->interpolatedTranslation(j, currFraction);
                            it.value().values[i * 3] = trans.x();
                            it.value().values[i * 3 + 1] = trans.y();

                            accumulatedTranslation += trans;

                            qCDebug(lcVectorImageAnimations) << "       -> Adding translation of" << trans;
                        } else if (type == QTransform::TxScale) {
                            const QPointF scale = property->interpolatedScale(j, currFraction);

                            it.value().values[i * 3] = scale.x();
                            it.value().values[i * 3 + 1] = scale.y();

                            accumulatedScale.rx() *= scale.x();
                            accumulatedScale.ry() *= scale.y();

                            qCDebug(lcVectorImageAnimations) << "       -> Adding scale of" << scale;
                        } else if (type == QTransform::TxRotate) {
                            const QPointF origin = property->interpolatedCenterOfRotation(j, currFraction);
                            const qreal rotation = property->interpolatedRotation(j, currFraction);

                            it.value().values[i * 3] = origin.x();
                            it.value().values[i * 3 + 1] = origin.y();
                            it.value().values[i * 3 + 2] = rotation;

                            accumulatedRotation += rotation;

                            qCDebug(lcVectorImageAnimations) << "       -> Adding rotation of" << rotation << "around" << origin;
                        } else if (type == QTransform::TxShear) {
                            const QPointF skew = property->interpolatedSkew(j, currFraction);

                            it.value().values[i * 3] = skew.x();
                            it.value().values[i * 3 + 1] = skew.y();
                            accumulatedSkew += skew;

                            qCDebug(lcVectorImageAnimations) << "       -> Adding skew of" << skew;
                        }
                    }
                }

                // This animation replaces all animations further down the stack, so we just
                // escape here
                if (replacesOtherTransforms)
                    break;
            }

            qCDebug(lcVectorImageAnimations) << "  -> Transform: "
                                             << "translation == " << accumulatedTranslation
                                             << "| scales == " << accumulatedScale
                                             << "| rotation == " << accumulatedRotation
                                             << "| skew == " << accumulatedSkew;
        }
    }
}

void QSvgVisitorImpl::fillAnimationInfo(const QSvgNode *node, NodeInfo &info)
{
    fillColorAnimationInfo(node, info);
    fillTransformAnimationInfo(node, info);
}

void QSvgVisitorImpl::handleBaseNodeSetup(const QSvgNode *node)
{
    qCDebug(lcQuickVectorImage) << "Before SETUP" << node << "fill" << styleResolver->currentFillColor()
                                   << "stroke" << styleResolver->currentStrokeColor() << styleResolver->currentStrokeWidth()
                                   << node->nodeId() << " type: " << node->typeName()  << " " << node->type();

    node->applyStyle(&styleResolver->painter(), styleResolver->states());

    qCDebug(lcQuickVectorImage) << "After SETUP" << node << "fill" << styleResolver->currentFillColor()
                                   << "stroke" << styleResolver->currentStrokeColor()
                                   << styleResolver->currentStrokeWidth() << node->nodeId();
}

void QSvgVisitorImpl::handleBaseNode(const QSvgNode *node)
{
    NodeInfo info;
    fillCommonNodeInfo(node, info);

    m_generator->generateNodeBase(info);
}

void QSvgVisitorImpl::handleBaseNodeEnd(const QSvgNode *node)
{
    node->revertStyle(&styleResolver->painter(), styleResolver->states());

    qCDebug(lcQuickVectorImage) << "After END" << node << "fill" << styleResolver->currentFillColor()
                                   << "stroke" << styleResolver->currentStrokeColor() << styleResolver->currentStrokeWidth()
                                   << node->nodeId();
}

void QSvgVisitorImpl::handlePathNode(const QSvgNode *node, const QPainterPath &path)
{
    handleBaseNodeSetup(node);

    PathNodeInfo info;
    fillCommonNodeInfo(node, info);
    fillAnimationInfo(node, info);
    auto fillStyle = node->style().fill;
    if (fillStyle)
        info.fillRule = fillStyle->fillRule();

    const QGradient *strokeGradient = styleResolver->currentStrokeGradient();

    info.painterPath = path;
    info.fillColor = styleResolver->currentFillColor();
    if (strokeGradient == nullptr) {
        info.strokeStyle = StrokeStyle::fromPen(styleResolver->currentStroke());
        info.strokeStyle.color = styleResolver->currentStrokeColor();
    }
    if (styleResolver->currentFillGradient() != nullptr)
        info.grad = styleResolver->applyOpacityToGradient(*styleResolver->currentFillGradient(), styleResolver->currentFillOpacity());
    info.fillTransform = styleResolver->currentFillTransform();

    m_generator->generatePath(info);

    if (strokeGradient != nullptr) {
        PathNodeInfo strokeInfo;
        fillCommonNodeInfo(node, strokeInfo);

        strokeInfo.grad = *strokeGradient;

        QPainterPathStroker stroker(styleResolver->currentStroke());
        strokeInfo.painterPath = stroker.createStroke(path);
        m_generator->generatePath(strokeInfo);
    }

    handleBaseNodeEnd(node);
}

QT_END_NAMESPACE
