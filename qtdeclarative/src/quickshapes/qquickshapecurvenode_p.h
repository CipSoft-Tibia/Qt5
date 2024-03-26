// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPECURVENODE_P_H
#define QQUICKSHAPECURVENODE_P_H

#include <QtQuick/qsgnode.h>

#include "qquickshapeabstractcurvenode_p.h"
#include "qquickshapegenericrenderer_p.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQuickShapeCurveNode : public QQuickShapeAbstractCurveNode
{
public:
    QQuickShapeCurveNode();

    void setColor(QColor col) override
    {
        if (m_color == col)
            return;
        m_color = col;
        updateMaterial();
    }

    QColor color() const
    {
        return m_color;
    }

    void setStrokeColor(QColor col)
    {
        const bool hadStroke = hasStroke();
        m_strokeColor = col;
        if (hadStroke != hasStroke())
            updateMaterial();
    }

    QColor strokeColor() const
    {
        return m_strokeColor;
    }

    void setStrokeWidth(float width)
    {
        const bool hadStroke = hasStroke();
        m_strokeWidth = width;
        if (hadStroke != hasStroke())
            updateMaterial();
    }

    float strokeWidth() const
    {
        return m_strokeWidth;
    }

    void setFillGradient(const QQuickAbstractPathRenderer::GradientDesc &fillGradient)
    {
        m_fillGradient = fillGradient;
    }

    QQuickAbstractPathRenderer::GradientDesc fillGradient() const
    {
        return m_fillGradient;
    }

    void setGradientType(QQuickAbstractPathRenderer::FillGradientType type)
    {
        if (m_gradientType != type) {
            m_gradientType = type;
            updateMaterial();
        }
    }

    float debug() const
    {
        return m_debug;
    }

    void setDebug(float newDebug)
    {
        m_debug = newDebug;
    }

    QQuickAbstractPathRenderer::FillGradientType gradientType() const
    {
        return m_gradientType;
    }

    bool hasStroke() const
    {
        return m_strokeWidth > 0.0f && m_strokeColor.alpha() > 0;
    }

    void appendTriangle(const std::array<QVector2D, 3> &v, // triangle vertices
                        const std::array<QVector2D, 3> &n, // vertex normals
                        std::function<QVector3D(QVector2D)> uvForPoint
                        )
    {
        QVector3D uv1 = uvForPoint(v[0]);
        QVector3D uv2 = uvForPoint(v[1]);
        QVector3D uv3 = uvForPoint(v[2]);

        QVector2D duvdx = QVector2D(uvForPoint(v[0] + QVector2D(1, 0))) - QVector2D(uv1);
        QVector2D duvdy = QVector2D(uvForPoint(v[0] + QVector2D(0, 1))) - QVector2D(uv1);

        m_uncookedIndexes.append(m_uncookedVertexes.size());
        m_uncookedVertexes.append( { v[0].x(), v[0].y(),
            uv1.x(), uv1.y(), uv1.z(),
            duvdx.x(), duvdx.y(),
            duvdy.x(), duvdy.y(),
            n[0].x(), n[0].y()
        });

        m_uncookedIndexes.append(m_uncookedVertexes.size());
        m_uncookedVertexes.append( { v[1].x(), v[1].y(),
            uv2.x(), uv2.y(), uv2.z(),
            duvdx.x(), duvdx.y(),
            duvdy.x(), duvdy.y(),
            n[1].x(), n[1].y()
        });

        m_uncookedIndexes.append(m_uncookedVertexes.size());
        m_uncookedVertexes.append( { v[2].x(), v[2].y(),
            uv3.x(), uv3.y(), uv3.z(),
            duvdx.x(), duvdx.y(),
            duvdy.x(), duvdy.y(),
            n[2].x(), n[2].y()
        });

    }

    void appendTriangle(const QVector2D &v1,
                        const QVector2D &v2,
                        const QVector2D &v3,
                        std::function<QVector3D(QVector2D)> uvForPoint)
    {
        appendTriangle({v1, v2, v3}, {}, uvForPoint);
    }

    void appendIndex(quint32 index)
    {
        m_uncookedIndexes.append(index);
    }

    void appendIndexes(QVector<quint32> indexes)
    {
        m_uncookedIndexes.append(indexes);
    }

    QVector<quint32> uncookedIndexes() const
    {
        return m_uncookedIndexes;
    }

    void cookGeometry();

private:
    struct CurveNodeVertex
    {
        float x, y, u, v, w;
        float dudx, dvdx, dudy, dvdy; // Size of pixel in curve space (must be same for all vertices in triangle)
        float nx, ny; // normal vector describing the direction to shift the vertex for AA
    };

    void updateMaterial();
    static const QSGGeometry::AttributeSet &attributes();

    QColor m_color = Qt::white;
    QColor m_strokeColor = Qt::transparent;
    float m_strokeWidth = 0.0f;
    float m_debug = 0.0f;
    QQuickAbstractPathRenderer::GradientDesc m_fillGradient;
    QQuickAbstractPathRenderer::FillGradientType m_gradientType = QQuickAbstractPathRenderer::NoGradient;

    QScopedPointer<QSGMaterial> m_material;

    QVector<CurveNodeVertex> m_uncookedVertexes;
    QVector<quint32> m_uncookedIndexes;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPECURVENODE_P_H
