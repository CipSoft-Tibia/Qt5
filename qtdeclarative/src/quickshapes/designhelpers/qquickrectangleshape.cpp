// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickrectangleshape_p.h"
#include "qquickrectangleshape_p_p.h"

QT_BEGIN_NAMESPACE

void QQuickRectangleShapePrivate::updateStrokeAdjustment()
{
    Q_Q(QQuickRectangleShape);
    switch (borderMode) {
    case QQuickRectangleShape::Inside:
        pathRectangle->setStrokeAdjustment(q->strokeWidth());
        break;
    case QQuickRectangleShape::Middle:
        pathRectangle->setStrokeAdjustment(0);
        break;
    case QQuickRectangleShape::Outside:
        pathRectangle->setStrokeAdjustment(-q->strokeWidth());
        break;
    }
    q->polish();
}

QQuickRectangleShape::QQuickRectangleShape(QQuickItem *parent)
    : QQuickShape(*(new QQuickRectangleShapePrivate), parent)
{
    Q_D(QQuickRectangleShape);

    setPreferredRendererType(CurveRenderer);

    d->shapePath = new QQuickShapePath(this);
    d->shapePath->setObjectName("rectangleShapeShapePath");
    d->shapePath->setAsynchronous(true);
    d->shapePath->setStrokeWidth(4);
    d->shapePath->setStrokeColor(QColorConstants::Black);

    connect(d->shapePath, &QQuickShapePath::strokeColorChanged, this, &QQuickRectangleShape::strokeColorChanged);
    connect(d->shapePath, &QQuickShapePath::strokeWidthChanged, this, &QQuickRectangleShape::strokeWidthChanged);
    connect(d->shapePath, &QQuickShapePath::fillColorChanged, this, &QQuickRectangleShape::fillColorChanged);
    connect(d->shapePath, &QQuickShapePath::joinStyleChanged, this, &QQuickRectangleShape::joinStyleChanged);
    connect(d->shapePath, &QQuickShapePath::capStyleChanged, this, &QQuickRectangleShape::capStyleChanged);
    connect(d->shapePath, &QQuickShapePath::strokeStyleChanged, this, &QQuickRectangleShape::strokeStyleChanged);
    connect(d->shapePath, &QQuickShapePath::dashOffsetChanged, this, &QQuickRectangleShape::dashOffsetChanged);
    connect(d->shapePath, &QQuickShapePath::dashPatternChanged, this, &QQuickRectangleShape::dashPatternChanged);
    // QQuickShapePath has no change signal for fillGradient.

    // Construct and append the PathRectangle to the ShapePath.
    d->pathRectangle = new QQuickPathRectangle(d->shapePath);
    d->pathRectangle->setObjectName("topRightPathArc");
    d->pathRectangle->setRadius(10);
    connect(d->pathRectangle, &QQuickPathRectangle::radiusChanged, this, &QQuickRectangleShape::radiusChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::topLeftRadiusChanged, this, &QQuickRectangleShape::topLeftRadiusChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::topRightRadiusChanged, this, &QQuickRectangleShape::topRightRadiusChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::bottomLeftRadiusChanged, this, &QQuickRectangleShape::bottomLeftRadiusChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::bottomRightRadiusChanged, this, &QQuickRectangleShape::bottomRightRadiusChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::bevelChanged, this, &QQuickRectangleShape::bevelChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::topLeftBevelChanged, this, &QQuickRectangleShape::topLeftBevelChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::topRightBevelChanged, this, &QQuickRectangleShape::topRightBevelChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::bottomLeftBevelChanged, this, &QQuickRectangleShape::bottomLeftBevelChanged);
    connect(d->pathRectangle, &QQuickPathRectangle::bottomRightBevelChanged, this, &QQuickRectangleShape::bottomRightBevelChanged);

    // geometryChange sets PathRectangle's size to ours, so do this after we've created it.
    setWidth(200);
    setHeight(150);
    d->updateStrokeAdjustment();

    QQuickPathPrivate::get(d->shapePath)->appendPathElement(d->pathRectangle);

    // Now that we've constructed the individual path elements and added them to the shape path,
    // add that path as a child of us.
    // Do what vpe_append in qquickshape.cpp does except without the overhead of the QQmlListProperty stuff.
    d->sp.append(d->shapePath);
    // Similar, but for QQuickItemPrivate::data_append...
    d->shapePath->setParent(this);
    // ... which calls QQuickItemPrivate::resources_append.
    d->extra.value().resourcesList.append(d->shapePath);
}

/*!
    \qmltype RectangleShape
    \inqmlmodule QtQuick.Shapes.DesignHelpers
    \brief A filled rectangle with an optional border.
    \since QtQuick 6.10

    RectangleShape is used to fill areas with solid color or gradients and to
    provide a rectangular border.

    Each Rectangle item is painted using either a solid fill color, specified
    using the \l fillColor property, or a gradient, defined using one of the \l
    ShapeGradient subtypes and set using the \l gradient property. If both a
    color and a gradient are specified, the gradient is used.

    An optional border can be added to a rectangle with its own color and
    thickness by setting the \l strokeColor and \l strokeWidth properties.
    Setting the color to \c transparent creates a border without a fill color.

    Rounded rectangles can be drawn using the \l radius property. The radius
    can also be specified separately for each corner. Additionally, \l bevel
    can be applied on any corner to cut it off sharply.

    RectangleShape's default value for \l {QtQuick.Shapes::Shape::preferredRendererType} is
    \c Shape.CurveRenderer.

    \section1 Example Usage

    \snippet rectangleshape-bevel.qml rectangleShape

    \image pathrectangle-bevel.png
*/

QQuickRectangleShape::~QQuickRectangleShape()
{
}

/*!
    \include pathrectangle.qdocinc {radius-property}
        {QtQuick.Shapes.DesignHelpers::RectangleShape}

    The default value is \c 10.
*/

int QQuickRectangleShape::radius() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->radius();
}

void QQuickRectangleShape::setRadius(int radius)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setRadius(radius);
}

/*!
    \include pathrectangle.qdocinc {radius-properties}
        {QtQuick.Shapes.DesignHelpers::RectangleShape} {rectangleshape.qml} {rectangleShape}
*/

int QQuickRectangleShape::topLeftRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->topLeftRadius();
}

void QQuickRectangleShape::setTopLeftRadius(int topLeftRadius)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setTopLeftRadius(topLeftRadius);
}

void QQuickRectangleShape::resetTopLeftRadius()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetTopLeftRadius();
}

int QQuickRectangleShape::topRightRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->topRightRadius();
}

void QQuickRectangleShape::setTopRightRadius(int topRightRadius)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setTopRightRadius(topRightRadius);
}

void QQuickRectangleShape::resetTopRightRadius()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetTopRightRadius();
}

int QQuickRectangleShape::bottomLeftRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->bottomLeftRadius();
}

void QQuickRectangleShape::setBottomLeftRadius(int bottomLeftRadius)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setBottomLeftRadius(bottomLeftRadius);
}

void QQuickRectangleShape::resetBottomLeftRadius()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetBottomLeftRadius();
}

int QQuickRectangleShape::bottomRightRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->bottomRightRadius();
}

void QQuickRectangleShape::setBottomRightRadius(int bottomRightRadius)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setBottomRightRadius(bottomRightRadius);
}

void QQuickRectangleShape::resetBottomRightRadius()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetBottomRightRadius();
}

/*!
    \include pathrectangle.qdocinc {bevel-property}
        {QtQuick.Shapes.DesignHelpers::RectangleShape}
        {rectangleshape-bevel.qml}{rectangleShape}
*/

bool QQuickRectangleShape::hasBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->hasBevel();
}

void QQuickRectangleShape::setBevel(bool bevel)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setBevel(bevel);
}

/*!
    \include pathrectangle.qdocinc {bevel-properties}
        {QtQuick.Shapes.DesignHelpers::RectangleShape}
        {rectangleshape.qml} {rectangleShape}
*/

bool QQuickRectangleShape::hasTopLeftBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->hasTopLeftBevel();
}

void QQuickRectangleShape::setTopLeftBevel(bool topLeftBevel)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setTopLeftBevel(topLeftBevel);
}

void QQuickRectangleShape::resetTopLeftBevel()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetTopLeftBevel();
}

bool QQuickRectangleShape::hasTopRightBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->hasTopRightBevel();
}

void QQuickRectangleShape::setTopRightBevel(bool topRightBevel)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setTopRightBevel(topRightBevel);
}

void QQuickRectangleShape::resetTopRightBevel()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetTopRightBevel();
}

bool QQuickRectangleShape::hasBottomLeftBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->hasBottomLeftBevel();
}

void QQuickRectangleShape::setBottomLeftBevel(bool bottomLeftBevel)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setBottomLeftBevel(bottomLeftBevel);
}

void QQuickRectangleShape::resetBottomLeftBevel()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetBottomLeftBevel();
}

bool QQuickRectangleShape::hasBottomRightBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->pathRectangle->hasBottomRightBevel();
}

void QQuickRectangleShape::setBottomRightBevel(bool bottomRightBevel)
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->setBottomRightBevel(bottomRightBevel);
}

void QQuickRectangleShape::resetBottomRightBevel()
{
    Q_D(QQuickRectangleShape);
    d->pathRectangle->resetBottomRightBevel();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::RectangleShape::strokeColor

    This property holds the stroking color.

    When set to \c transparent, no stroking occurs.

    The default value is \c "black".
*/

QColor QQuickRectangleShape::strokeColor() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->strokeColor();
}

void QQuickRectangleShape::setStrokeColor(const QColor &color)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setStrokeColor(color);
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::RectangleShape::strokeWidth

    This property holds the stroke width.

    When set to a negative value, no stroking occurs.

    The default value is \c 1.
*/

qreal QQuickRectangleShape::strokeWidth() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->strokeWidth();
}

void QQuickRectangleShape::setStrokeWidth(qreal width)
{
    Q_D(QQuickRectangleShape);
    const qreal oldStrokeWidth = d->shapePath->strokeWidth();
    d->shapePath->setStrokeWidth(width);
    // setStrokeWidth doesn't use qFuzzyCompare, so neither do we.
    if (oldStrokeWidth != d->shapePath->strokeWidth())
        d->updateStrokeAdjustment();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::RectangleShape::fillColor

    This property holds the fill color.

    When set to \c transparent, no filling occurs.

    The default value is \c "white".

    \note If either \l fillGradient is set to something other than \c null, it
    will be used instead of \c fillColor.
*/

QColor QQuickRectangleShape::fillColor() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->fillColor();
}

void QQuickRectangleShape::setFillColor(const QColor &color)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setFillColor(color);
}

/*!
    \include shapepath.qdocinc {fillRule-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::FillRule QQuickRectangleShape::fillRule() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->fillRule();
}

void QQuickRectangleShape::setFillRule(QQuickShapePath::FillRule fillRule)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setFillRule(fillRule);
}

/*!
    \include shapepath.qdocinc {joinStyle-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::JoinStyle QQuickRectangleShape::joinStyle() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->joinStyle();
}

void QQuickRectangleShape::setJoinStyle(QQuickShapePath::JoinStyle style)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setJoinStyle(style);
}

/*!
    \include shapepath.qdocinc {capStyle-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::CapStyle QQuickRectangleShape::capStyle() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->capStyle();
}

void QQuickRectangleShape::setCapStyle(QQuickShapePath::CapStyle style)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setCapStyle(style);
}

/*!
    \include shapepath.qdocinc {strokeStyle-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::StrokeStyle QQuickRectangleShape::strokeStyle() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->strokeStyle();
}

void QQuickRectangleShape::setStrokeStyle(QQuickShapePath::StrokeStyle style)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setStrokeStyle(style);
}

/*!
    \include shapepath.qdocinc {dashOffset-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

qreal QQuickRectangleShape::dashOffset() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->dashOffset();
}

void QQuickRectangleShape::setDashOffset(qreal offset)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setDashOffset(offset);
}

/*!
    \include shapepath.qdocinc {dashPattern-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QVector<qreal> QQuickRectangleShape::dashPattern() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->dashPattern();
}

void QQuickRectangleShape::setDashPattern(const QVector<qreal> &array)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setDashPattern(array);
}

/*!
    \qmlproperty ShapeGradient QtQuick.Shapes.DesignHelpers::RectangleShape::fillGradient

    The fillGradient of the rectangle fill color.

    By default, no fillGradient is enabled and the value is null. In this case, the
    fill uses a solid color based on the value of \l fillColor.

    When set, \l fillColor is ignored and filling is done using one of the
    \l ShapeGradient subtypes.

    \note The \l Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like \l LinearGradient.
*/
QQuickShapeGradient *QQuickRectangleShape::fillGradient() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->fillGradient();
}

void QQuickRectangleShape::setFillGradient(QQuickShapeGradient *fillGradient)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setFillGradient(fillGradient);
}

void QQuickRectangleShape::resetFillGradient()
{
    setFillGradient(nullptr);
}

/*!
    \qmlproperty enumeration QtQuick.Shapes.DesignHelpers::RectangleShape::borderMode

    The \l borderMode property determines where the border is drawn along the
    edge of the rectangle.

    \value RectangleShape.Inside
        The border is drawn along the inside edge of the item and does not
        affect the item width.

        This is the default value.
    \value RectangleShape.Middle
        The border is drawn over the edge of the item and does not
        affect the item width.
    \value RectangleShape.Outside
        The border is drawn along the outside edge of the item and increases
        the item width by the value of \l strokeWidth.

    \sa strokeWidth
*/
QQuickRectangleShape::BorderMode QQuickRectangleShape::borderMode() const
{
    Q_D(const QQuickRectangleShape);
    return d->borderMode;
}

void QQuickRectangleShape::setBorderMode(BorderMode borderMode)
{
    Q_D(QQuickRectangleShape);
    if (borderMode == d->borderMode)
        return;

    d->borderMode = borderMode;
    d->updateStrokeAdjustment();
    emit borderModeChanged();
}

void QQuickRectangleShape::resetBorderMode()
{
    setBorderMode(Inside);
}

void QQuickRectangleShape::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickRectangleShape);
    QQuickShape::geometryChange(newGeometry, oldGeometry);
    d->pathRectangle->setWidth(newGeometry.width());
    d->pathRectangle->setHeight(newGeometry.height());
}

QT_END_NAMESPACE

#include "moc_qquickrectangleshape_p.cpp"
