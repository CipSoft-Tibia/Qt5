// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPE_P_P_H
#define QQUICKSHAPE_P_P_H

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

#include <QtQuickShapes/private/qquickshapesglobal_p.h>
#include <QtQuickShapes/private/qquickshape_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QElapsedTimer>
#if QT_CONFIG(opengl)
# include <private/qopenglcontext_p.h>
#endif
QT_BEGIN_NAMESPACE

class QSGPlainTexture;
class QRhi;

class QQuickAbstractPathRenderer
{
public:
    enum Flag {
        SupportsAsync = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum FillGradientType { NoGradient = 0, LinearGradient, RadialGradient, ConicalGradient };
    struct GradientDesc { // can fully describe a linear/radial/conical gradient
        QGradientStops stops;
        QQuickShapeGradient::SpreadMode spread = QQuickShapeGradient::PadSpread;
        QPointF a; // start (L) or center point (R/C)
        QPointF b; // end (L) or focal point (R)
        qreal v0; // center radius (R) or start angle (C)
        qreal v1; // focal radius (R)
    };

    virtual ~QQuickAbstractPathRenderer() { }

    // Gui thread
    virtual void beginSync(int totalCount, bool *countChanged) = 0;
    virtual void endSync(bool async) = 0;
    virtual void setAsyncCallback(void (*)(void *), void *) { }
    virtual Flags flags() const { return {}; }
    virtual void setPath(int index, const QQuickPath *path) = 0;
    virtual void setStrokeColor(int index, const QColor &color) = 0;
    virtual void setStrokeWidth(int index, qreal w) = 0;
    virtual void setFillColor(int index, const QColor &color) = 0;
    virtual void setFillRule(int index, QQuickShapePath::FillRule fillRule) = 0;
    virtual void setJoinStyle(int index, QQuickShapePath::JoinStyle joinStyle, int miterLimit) = 0;
    virtual void setCapStyle(int index, QQuickShapePath::CapStyle capStyle) = 0;
    virtual void setStrokeStyle(int index, QQuickShapePath::StrokeStyle strokeStyle,
                                qreal dashOffset, const QVector<qreal> &dashPattern) = 0;
    virtual void setFillGradient(int index, QQuickShapeGradient *gradient) = 0;
    virtual void setTriangulationScale(qreal) { }

    // Render thread, with gui blocked
    virtual void updateNode() = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickAbstractPathRenderer::Flags)

struct QQuickShapeStrokeFillParams
{
    QQuickShapeStrokeFillParams();

    QColor strokeColor;
    qreal strokeWidth;
    QColor fillColor;
    QQuickShapePath::FillRule fillRule;
    QQuickShapePath::JoinStyle joinStyle;
    int miterLimit;
    QQuickShapePath::CapStyle capStyle;
    QQuickShapePath::StrokeStyle strokeStyle;
    qreal dashOffset;
    QVector<qreal> dashPattern;
    QQuickShapeGradient *fillGradient;
};

class Q_QUICKSHAPES_PRIVATE_EXPORT QQuickShapePathPrivate : public QQuickPathPrivate
{
    Q_DECLARE_PUBLIC(QQuickShapePath)

public:
    enum Dirty {
        DirtyPath = 0x01,
        DirtyStrokeColor = 0x02,
        DirtyStrokeWidth = 0x04,
        DirtyFillColor = 0x08,
        DirtyFillRule = 0x10,
        DirtyStyle = 0x20,
        DirtyDash = 0x40,
        DirtyFillGradient = 0x80,

        DirtyAll = 0xFF
    };

    QQuickShapePathPrivate();

    void _q_pathChanged();
    void _q_fillGradientChanged();

    static QQuickShapePathPrivate *get(QQuickShapePath *p) { return p->d_func(); }

    int dirty;
    QQuickShapeStrokeFillParams sfp;
};

class QQuickShapePrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickShape)

public:
    QQuickShapePrivate();
    ~QQuickShapePrivate();

    QQuickShape::RendererType selectRendererType();
    void createRenderer();
    QSGNode *createNode();
    void sync();

    void _q_shapePathChanged();
    void setStatus(QQuickShape::Status newStatus);

    static QQuickShapePrivate *get(QQuickShape *item) { return item->d_func(); }

    static void asyncShapeReady(void *data);

    int effectRefCount;
    QVector<QQuickShapePath *> sp;
    QElapsedTimer syncTimer;
    QQuickAbstractPathRenderer *renderer = nullptr;
    int syncTimingTotalDirty = 0;
    int syncTimeCounter = 0;
    QQuickShape::Status status = QQuickShape::Null;
    QQuickShape::RendererType rendererType = QQuickShape::UnknownRenderer;
    QQuickShape::RendererType preferredType = QQuickShape::UnknownRenderer;
    QQuickShape::ContainsMode containsMode = QQuickShape::BoundingRectContains;
    bool spChanged = false;
    bool rendererChanged = false;
    bool async = false;
    bool enableVendorExts = false;
    bool syncTimingActive = false;
    qreal triangulationScale = 1.0;
};

struct QQuickShapeGradientCacheKey
{
    QQuickShapeGradientCacheKey(const QGradientStops &stops, QQuickShapeGradient::SpreadMode spread)
        : stops(stops), spread(spread)
    { }
    QGradientStops stops;
    QQuickShapeGradient::SpreadMode spread;
    bool operator==(const QQuickShapeGradientCacheKey &other) const
    {
        return spread == other.spread && stops == other.stops;
    }
};

inline size_t qHash(const QQuickShapeGradientCacheKey &v, size_t seed = 0)
{
    size_t h = seed + v.spread;
    for (int i = 0; i < 3 && i < v.stops.size(); ++i)
        h += v.stops[i].second.rgba();
    return h;
}

class QQuickShapeGradientCache
{
public:
    ~QQuickShapeGradientCache();
    static QQuickShapeGradientCache *cacheForRhi(QRhi *rhi);
    QSGTexture *get(const QQuickShapeGradientCacheKey &grad);

private:
    QHash<QQuickShapeGradientCacheKey, QSGPlainTexture *> m_textures;
};

QT_END_NAMESPACE

#endif
