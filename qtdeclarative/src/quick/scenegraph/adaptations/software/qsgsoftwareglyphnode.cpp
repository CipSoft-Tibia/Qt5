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
    constexpr QPointF STARTING_POINT_IN_PIXMAP{1, 1}; // Leave a 1px top/left spacing for outline
    if (m_Dirty) {
        constexpr int OFFSET_FOR_OUTLINE_PIXELS{2}; // Needs 1px on top/left and 1px on bottom/right for outline
        m_CachedPixmap = QPixmap{
            static_cast<int>(m_bounding_rect.x() + m_bounding_rect.width() + OFFSET_FOR_OUTLINE_PIXELS),
            static_cast<int>(m_bounding_rect.y() + m_bounding_rect.height() + OFFSET_FOR_OUTLINE_PIXELS)};
        const QColor Transparent{0, 0, 0, 0};
        m_CachedPixmap.fill(Transparent);

        QPainter Painter(&m_CachedPixmap);
        Painter.setBrush(QBrush());
        Painter.setBackgroundMode(Qt::TransparentMode);

        qreal offset = 1.0;
        if (painter->device()->devicePixelRatio() > 0.0)
            offset = 1.0 / painter->device()->devicePixelRatio();

        switch (m_style) {
        case QQuickText::Normal: break;
        case QQuickText::Outline:
            Painter.setPen(m_styleColor);
            Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP + QPointF(0, offset), m_glyphRun);
            Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP + QPointF(0, -offset), m_glyphRun);
            Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP + QPointF(offset, 0), m_glyphRun);
            Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP + QPointF(-offset, 0), m_glyphRun);
            break;

        // XXX: raised and sunken text was never tested, so it might not work as expected
        case QQuickText::Raised:
            Painter.setPen(m_styleColor);
            Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP + QPointF(0, offset), m_glyphRun);
            break;
        case QQuickText::Sunken:
            Painter.setPen(m_styleColor);
            Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP + QPointF(0, -offset), m_glyphRun);
            break;
        }

        Painter.setPen(m_color);
        Painter.drawGlyphRun(STARTING_POINT_IN_PIXMAP, m_glyphRun);

        m_Dirty = false;
    }

    QPointF AbsolutePos = m_position - QPointF(0, m_glyphRun.rawFont().ascent()) - STARTING_POINT_IN_PIXMAP;
    painter->drawPixmap(AbsolutePos, m_CachedPixmap);
}

QT_END_NAMESPACE
