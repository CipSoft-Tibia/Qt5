// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QSGBASICINTERNALRECTANGLENODE_P_H
#define QSGBASICINTERNALRECTANGLENODE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QSGBasicInternalRectangleNode : public QSGInternalRectangleNode
{
public:
    QSGBasicInternalRectangleNode();

    void setRect(const QRectF &rect) override;
    void setColor(const QColor &color) override;
    void setPenColor(const QColor &color) override;
    void setPenWidth(qreal width) override;
    void setGradientStops(const QGradientStops &stops) override;
    void setGradientVertical(bool vertical) override;
    void setRadius(qreal radius) override;
    void setAntialiasing(bool antialiasing) override;
    void setAligned(bool aligned) override;
    void update() override;

protected:
    virtual bool supportsAntialiasing() const { return true; }
    virtual void updateMaterialAntialiasing() = 0;
    virtual void updateMaterialBlending(QSGNode::DirtyState *state) = 0;

    void updateGeometry();
    void updateGradientTexture();

    QRectF m_rect;
    QGradientStops m_gradient_stops;
    QColor m_color;
    QColor m_border_color;
    qreal m_radius;
    qreal m_pen_width;

    uint m_aligned : 1;
    uint m_antialiasing : 1;
    uint m_gradient_is_opaque : 1;
    uint m_dirty_geometry : 1;
    uint m_gradient_is_vertical : 1;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

#endif
