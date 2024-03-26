// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgglyphnode_p.h"
#include "qopenvgcontext_p.h"
#include "qsgopenvgcontext_p.h"
#include "qsgopenvghelpers.h"
#include "qsgopenvgfontglyphcache.h"
#include "qopenvgoffscreensurface.h"
#include <cmath>

QT_BEGIN_NAMESPACE

QSGOpenVGGlyphNode::QSGOpenVGGlyphNode(QSGRenderContext *rc)
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
    , m_glyphCache(nullptr)
{
    // Set Dummy material to avoid asserts
    setMaterial((QSGMaterial*)1);
    setGeometry(&m_geometry);
    m_fontColorPaint = vgCreatePaint();
    m_styleColorPaint = vgCreatePaint();

    // Get handle to Glyph Cache
    m_renderContext = static_cast<QSGOpenVGRenderContext*>(rc);
}

QSGOpenVGGlyphNode::~QSGOpenVGGlyphNode()
{
    if (m_glyphCache)
        m_glyphCache->release(m_glyphRun.glyphIndexes());

    vgDestroyPaint(m_fontColorPaint);
    vgDestroyPaint(m_styleColorPaint);
}

void QSGOpenVGGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    // Obtain glyph cache for font
    auto oldGlyphCache = m_glyphCache;
    m_glyphCache = m_renderContext->glyphCache(glyphs.rawFont());
    if (m_glyphCache != oldGlyphCache) {
        if (oldGlyphCache)
            oldGlyphCache->release(m_glyphRun.glyphIndexes());
    }
    m_glyphCache->populate(glyphs.glyphIndexes());

    m_position = position;
    m_glyphRun = glyphs;
    m_bounding_rect = glyphs.boundingRect().translated(m_position - QPointF(0.0, glyphs.rawFont().ascent()));

    // Recreate ajustments
    m_xAdjustments.clear();
    m_yAdjustments.clear();

    for (int i = 1; i < glyphs.positions().count(); ++i) {
        m_xAdjustments.append(glyphs.positions().at(i).x() - glyphs.positions().at(i-1).x());
        m_yAdjustments.append(glyphs.positions().at(i).y() - glyphs.positions().at(i-1).y());
    }
}

void QSGOpenVGGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    vgSetParameteri(m_fontColorPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv(m_fontColorPaint, VG_PAINT_COLOR, 4, QSGOpenVGHelpers::qColorToVGColor(m_color, opacity()).constData());
}

void QSGOpenVGGlyphNode::setStyle(QQuickText::TextStyle style)
{
    m_style = style;
}

void QSGOpenVGGlyphNode::setStyleColor(const QColor &color)
{
    m_styleColor = color;
    vgSetParameteri(m_styleColorPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetParameterfv(m_styleColorPaint, VG_PAINT_COLOR, 4, QSGOpenVGHelpers::qColorToVGColor(m_styleColor, opacity()).constData());
}

QPointF QSGOpenVGGlyphNode::baseLine() const
{
    return QPointF();
}

void QSGOpenVGGlyphNode::setPreferredAntialiasingMode(QSGGlyphNode::AntialiasingMode)
{
}

void QSGOpenVGGlyphNode::update()
{
}

void QSGOpenVGGlyphNode::render()
{
    if (m_glyphRun.positions().count() == 0)
        return;

    // Rendering Style
    qreal offset = 1.0;

    QOpenVGOffscreenSurface *offscreenSurface = nullptr;

    // Set Transform
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
    if (transform().isAffine()) {
        vgLoadMatrix(transform().constData());
    } else {
        vgLoadIdentity();
        offscreenSurface = new QOpenVGOffscreenSurface(QSize(std::ceil(m_bounding_rect.width()), std::ceil(m_bounding_rect.height())));
        offscreenSurface->makeCurrent();
    }

    // Set Quality
    vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);


    switch (m_style) {
    case QQuickText::Normal: break;
    case QQuickText::Outline:
        // Set the correct fill state
        vgSetPaint(m_styleColorPaint, VG_FILL_PATH);
        drawGlyphsAtOffset(QPointF(0, offset));
        drawGlyphsAtOffset(QPointF(0, -offset));
        drawGlyphsAtOffset(QPointF(offset, 0));
        drawGlyphsAtOffset(QPointF(-offset, 0));
        break;
    case QQuickText::Raised:
        vgSetPaint(m_styleColorPaint, VG_FILL_PATH);
        drawGlyphsAtOffset(QPointF(0, offset));
        break;
    case QQuickText::Sunken:
        vgSetPaint(m_styleColorPaint, VG_FILL_PATH);
        drawGlyphsAtOffset(QPointF(0, -offset));
        break;
    }

    // Set the correct fill state
    vgSetPaint(m_fontColorPaint, VG_FILL_PATH);
    drawGlyphsAtOffset(QPointF(0.0, 0.0));

    if (!transform().isAffine()) {
        vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
        vgLoadMatrix(transform().constData());
        offscreenSurface->doneCurrent();
        vgDrawImage(offscreenSurface->image());
        delete offscreenSurface;
    }
}

void QSGOpenVGGlyphNode::setOpacity(float opacity)
{
    if (QSGOpenVGRenderable::opacity() != opacity) {
        QSGOpenVGRenderable::setOpacity(opacity);
        // Update Colors
        setColor(m_color);
        setStyleColor(m_styleColor);
    }
}

void QSGOpenVGGlyphNode::drawGlyphsAtOffset(const QPointF &offset)
{
    QPointF firstPosition = m_glyphRun.positions()[0] + (m_position - QPointF(0, m_glyphRun.rawFont().ascent()));
    VGfloat origin[2];
    origin[0] = firstPosition.x() + offset.x();
    origin[1] = firstPosition.y() + offset.y();
    vgSetfv(VG_GLYPH_ORIGIN, 2, origin);

    vgDrawGlyphs(m_glyphCache->font(),
                 m_glyphRun.glyphIndexes().count(),
                 (VGuint*)m_glyphRun.glyphIndexes().constData(),
                 m_xAdjustments.constData(),
                 m_yAdjustments.constData(),
                 VG_FILL_PATH,
                 VG_TRUE);
}

QT_END_NAMESPACE
