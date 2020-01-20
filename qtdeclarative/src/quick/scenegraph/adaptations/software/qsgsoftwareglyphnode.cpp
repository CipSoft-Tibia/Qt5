// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwareglyphnode_p.h"
#include <QtGui/private/qrawfont_p.h>

QT_BEGIN_NAMESPACE

QSGSoftwareGlyphNode::QSGSoftwareGlyphNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
    , m_Dirty(true)
{
    setMaterial((QSGMaterial*)1);
    setGeometry(&m_geometry);
}

namespace {
QRectF calculateBoundingRect(const QPointF &position, const QGlyphRun &glyphs)
{
    QFixed minX;
    QFixed minY;
    QFixed maxX;
    QFixed maxY;

    QRawFontPrivate *rawFontD = QRawFontPrivate::get(glyphs.rawFont());
    QFontEngine *fontEngine = rawFontD->fontEngine;

    QFontEngine::GlyphFormat glyphFormat = fontEngine->glyphFormat != QFontEngine::Format_None ? fontEngine->glyphFormat : QFontEngine::Format_A32;

    int margin = fontEngine->glyphMargin(glyphFormat);

    const QVector<uint> glyphIndexes = glyphs.glyphIndexes();
    const QVector<QPointF> glyphPositions = glyphs.positions();
    for (int i = 0, n = qMin(glyphIndexes.size(), glyphPositions.size()); i < n; ++i) {
        glyph_metrics_t gm = fontEngine->alphaMapBoundingBox(glyphIndexes.at(i), QFixedPoint(), QTransform(), glyphFormat);

        gm.x += QFixed::fromReal(glyphPositions.at(i).x()) - margin;
        gm.y += QFixed::fromReal(glyphPositions.at(i).y()) - margin;

        if (i == 0) {
            minX = gm.x;
            minY = gm.y;
            maxX = gm.x + gm.width;
            maxY = gm.y + gm.height;
        } else {
            minX = qMin(gm.x, minX);
            minY = qMin(gm.y, minY);
            maxX = qMax(gm.x + gm.width, maxX);
            maxY = qMax(gm.y + gm.height, maxY);
        }
    }

    QRectF boundingRect(QPointF(minX.toReal(), minY.toReal()), QPointF(maxX.toReal(), maxY.toReal()));
    return boundingRect.translated(position - QPointF(0.0, glyphs.rawFont().ascent()));
}
}

void QSGSoftwareGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    m_position = position;
    m_glyphRun = glyphs;
    // Decorations handled by text node
    m_glyphRun.setOverline(false);
    m_glyphRun.setStrikeOut(false);
    m_glyphRun.setUnderline(false);
    m_bounding_rect = calculateBoundingRect(position, glyphs);
    m_Dirty = true;
}

void QSGSoftwareGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    m_Dirty = true;
}

void QSGSoftwareGlyphNode::setStyle(QQuickText::TextStyle style)
{
    m_style = style;

    // Resize the bounding rectangle to allow for a 1px outline
    if (m_style == QQuickText::Outline) {
        m_bounding_rect.setX(m_bounding_rect.x() - 1);
        m_bounding_rect.setY(m_bounding_rect.y() - 1);
        m_bounding_rect.setWidth(m_bounding_rect.width() + 2);
        m_bounding_rect.setHeight(m_bounding_rect.height() + 2);
    }

    m_Dirty = true;
}

void QSGSoftwareGlyphNode::setStyleColor(const QColor &color)
{
    m_styleColor = color;
    m_Dirty = true;
}

QPointF QSGSoftwareGlyphNode::baseLine() const
{
    return QPointF();
}

void QSGSoftwareGlyphNode::setPreferredAntialiasingMode(QSGGlyphNode::AntialiasingMode)
{
}

void QSGSoftwareGlyphNode::update()
{
}

void QSGSoftwareGlyphNode::paint(QPainter *painter)
{
    constexpr QPointF OUTLINE_STARTING_POINT_IN_PIXMAP{1, 1}; // Leave a 1px top/left spacing for outline

    if (m_Dirty) {
        const int PixmapWidth = static_cast<int>(std::abs(std::ceil(m_bounding_rect.x())) + m_bounding_rect.width());
        // Make sure that the resulting bitmap is, at the minimum, large enough to contain the highest font symbol (via ascent)
        const int PixmapHeight = qMax(static_cast<int>(std::abs(std::ceil(m_bounding_rect.y())) + m_bounding_rect.height()),
                                      static_cast<int>(m_glyphRun.rawFont().ascent()));
        m_CachedPixmap = QPixmap{PixmapWidth, PixmapHeight};
        const QColor Transparent{0, 0, 0, 0};
        m_CachedPixmap.fill(Transparent);

        QPainter Painter(&m_CachedPixmap);
        Painter.setBrush(QBrush());
        Painter.setBackgroundMode(Qt::TransparentMode);

        qreal offset = 1.0;
        if (painter->device()->devicePixelRatio() > 0.0)
            offset = 1.0 / painter->device()->devicePixelRatio();

        switch (m_style) {
        case QQuickText::Normal:
        case QQuickText::Sunken:
        case QQuickText::Raised:
            // XXX: raised and sunken text aren't implemented
            Painter.setPen(m_color);
            Painter.drawGlyphRun(QPointF{}, m_glyphRun);
        break;
        case QQuickText::Outline:
            Painter.setPen(m_styleColor);
            Painter.drawGlyphRun(OUTLINE_STARTING_POINT_IN_PIXMAP + QPointF(0, offset), m_glyphRun);
            Painter.drawGlyphRun(OUTLINE_STARTING_POINT_IN_PIXMAP + QPointF(0, -offset), m_glyphRun);
            Painter.drawGlyphRun(OUTLINE_STARTING_POINT_IN_PIXMAP + QPointF(offset, 0), m_glyphRun);
            Painter.drawGlyphRun(OUTLINE_STARTING_POINT_IN_PIXMAP + QPointF(-offset, 0), m_glyphRun);
            Painter.setPen(m_color);
            Painter.drawGlyphRun(OUTLINE_STARTING_POINT_IN_PIXMAP, m_glyphRun);
            break;
        }

        m_Dirty = false;
    }

    QPointF AbsolutePos = m_position - QPointF(0, m_glyphRun.rawFont().ascent()) - (m_style == QQuickText::Outline ? OUTLINE_STARTING_POINT_IN_PIXMAP : QPointF{});
    painter->drawPixmap(AbsolutePos, m_CachedPixmap);
}

QT_END_NAMESPACE
