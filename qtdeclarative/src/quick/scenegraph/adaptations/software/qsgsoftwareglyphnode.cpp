/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgsoftwareglyphnode_p.h"

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
    qreal minX = 0;
    qreal minY = 0;
    qreal maxX = 0;
    qreal maxY = 0;

    for (int i = 0, n = qMin(glyphs.glyphIndexes().size(), glyphs.positions().size()); i < n; ++i) {
        QRectF glyphRect = glyphs.rawFont().boundingRect(glyphs.glyphIndexes()[i]);
        glyphRect.translate(glyphs.positions()[i]);

        if (i == 0) {
            minX = glyphRect.left();
            minY = glyphRect.top();
            maxX = glyphRect.right();
            maxY = glyphRect.bottom();
        } else {
            minX = qMin(glyphRect.left(), minX);
            minY = qMin(glyphRect.top(), minY);
            maxX = qMax(glyphRect.right(),maxX);
            maxY = qMax(glyphRect.bottom(), maxY);
        }
    }
    QRectF boundingRect(QPointF(minX, minY), QPointF(maxX, maxY));
    return boundingRect.translated(position - QPointF(0.0, glyphs.rawFont().ascent()));
}
}

void QSGSoftwareGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    m_position = position;
    m_glyphRun = glyphs;
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
        if (painter->device()->devicePixelRatioF() > 0.0)
            offset = 1.0 / painter->device()->devicePixelRatioF();

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
