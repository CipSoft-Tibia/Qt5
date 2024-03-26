// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshape_p.h"
#include "qquickshape_p_p.h"
#include "qquickshapegenericrenderer_p.h"
#include "qquickshapesoftwarerenderer_p.h"
#include "qquickshapecurverenderer_p.h"
#include <private/qsgplaintexture_p.h>
#include <private/qquicksvgparser_p.h>
#include <QtGui/private/qdrawhelper_p.h>
#include <QOpenGLFunctions>
#include <QLoggingCategory>
#include <rhi/qrhi.h>

static void initResources()
{
#if defined(QT_STATIC)
    Q_INIT_RESOURCE(qtquickshapes_shaders);
#endif
}

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QQSHAPE_LOG_TIME_DIRTY_SYNC, "qt.shape.time.sync")

/*!
    \qmlmodule QtQuick.Shapes 1.\QtMinorVersion
    \title Qt Quick Shapes QML Types
    \ingroup qmlmodules
    \brief Provides QML types for drawing stroked and filled shapes.

    To use the types in this module, import the module with the following line:

    \qml
    import QtQuick.Shapes
    \endqml

    Qt Quick Shapes provides tools for drawing arbitrary shapes in a Qt Quick scene.
    \l{Shape}{Shapes} can be constructed from basic building blocks like \l{PathLine}{lines} and
    \l{PathCubic}{curves} that define sub-shapes. The sub-shapes can then be filled with solid
    colors or gradients, and an outline stroke can be defined.

    Qt Quick Shapes also supports higher level path element types, such as \l{PathText}{text} and
    \l{PathSvg}{SVG path descriptions}. The currently supported element types is: PathMove,
    PathLine, PathQuad, PathCubic, PathArc, PathText and PathSvg.

    Qt Quick Shapes triangulates the shapes and renders the corresponding triangles on the GPU.
    Therefore, altering the control points of elements will lead to re-triangulation of the
    affected paths, at some performance cost. In addition, curves are flattened before they are
    rendered, so applying a very high scale to the shape may show artifacts where it is visible
    that the curves are represented by a sequence of smaller, straight lines.

    \note Qt Quick Shapes relies on multi-sampling for anti-aliasing. This can be enabled for the
    entire application or window using the corresponding settings in QSurfaceFormat. It can also
    be enabled for only the shape, by setting its \l{Item::layer.enabled}{layer.enabled} property to
    true and then adjusting the \l{Item::layer.samples}{layer.samples} property. In the latter case,
    multi-sampling will not be applied to the entire scene, but the shape will be rendered via an
    intermediate off-screen buffer.

    For further information, the \l{Qt Quick Examples - Shapes}{Shapes example} shows how to
    implement different types of shapes, fills and strokes.
*/

void QQuickShapes_initializeModule()
{
    QQuickShapesModule::defineModule();
}

Q_CONSTRUCTOR_FUNCTION(QQuickShapes_initializeModule)

void QQuickShapesModule::defineModule()
{
    initResources();
}

QQuickShapeStrokeFillParams::QQuickShapeStrokeFillParams()
    : strokeColor(Qt::white),
      strokeWidth(1),
      fillColor(Qt::white),
      fillRule(QQuickShapePath::OddEvenFill),
      joinStyle(QQuickShapePath::BevelJoin),
      miterLimit(2),
      capStyle(QQuickShapePath::SquareCap),
      strokeStyle(QQuickShapePath::SolidLine),
      dashOffset(0),
      fillGradient(nullptr)
{
    dashPattern << 4 << 2; // 4 * strokeWidth dash followed by 2 * strokeWidth space
}

/*!
    \qmltype ShapePath
    //! \instantiates QQuickShapePath
    \inqmlmodule QtQuick.Shapes
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits Path
    \brief Describes a Path and associated properties for stroking and filling.
    \since 5.10

    A \l Shape contains one or more ShapePath elements. At least one ShapePath is
    necessary in order to have a Shape output anything visible. A ShapePath
    itself is a \l Path with additional properties describing the stroking and
    filling parameters, such as the stroke width and color, the fill color or
    gradient, join and cap styles, and so on. As with ordinary \l Path objects,
    ShapePath also contains a list of path elements like \l PathMove, \l PathLine,
    \l PathCubic, \l PathQuad, \l PathArc, together with a starting position.

    Any property changes in these data sets will be bubble up and change the
    output of the Shape. This means that it is simple and easy to change, or
    even animate, the starting and ending position, control points, or any
    stroke or fill parameters using the usual QML bindings and animation types
    like NumberAnimation.

    In the following example the line join style changes automatically based on
    the value of joinStyleIndex:

    \qml
    ShapePath {
        strokeColor: "black"
        strokeWidth: 16
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap

        property int joinStyleIndex: 0

        property variant styles: [
            ShapePath.BevelJoin,
            ShapePath.MiterJoin,
            ShapePath.RoundJoin
        ]

        joinStyle: styles[joinStyleIndex]

        startX: 30
        startY: 30
        PathLine { x: 100; y: 100 }
        PathLine { x: 30; y: 100 }
    }
    \endqml

    Once associated with a Shape, here is the output with a joinStyleIndex
    of 2 (ShapePath.RoundJoin):

    \image visualpath-code-example.png

    \sa {Qt Quick Examples - Shapes}, Shape
 */

QQuickShapePathPrivate::QQuickShapePathPrivate()
    : dirty(DirtyAll)
{
    // Set this QQuickPath to be a ShapePath
    isShapePath = true;
}

QQuickShapePath::QQuickShapePath(QObject *parent)
    : QQuickPath(*(new QQuickShapePathPrivate), parent)
{
    // The inherited changed() and the shapePathChanged() signals remain
    // distinct, and this is intentional. Combining the two is not possible due
    // to the difference in semantics and the need to act (see dirty flag
    // below) differently on QQuickPath-related changes.

    connect(this, &QQuickPath::changed, [this]() {
        Q_D(QQuickShapePath);
        d->dirty |= QQuickShapePathPrivate::DirtyPath;
        emit shapePathChanged();
    });
}

QQuickShapePath::~QQuickShapePath()
{
}

/*!
    \qmlproperty color QtQuick.Shapes::ShapePath::strokeColor

    This property holds the stroking color.

    When set to \c transparent, no stroking occurs.

    The default value is \c white.
 */

QColor QQuickShapePath::strokeColor() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.strokeColor;
}

void QQuickShapePath::setStrokeColor(const QColor &color)
{
    Q_D(QQuickShapePath);
    if (d->sfp.strokeColor != color) {
        d->sfp.strokeColor = color;
        d->dirty |= QQuickShapePathPrivate::DirtyStrokeColor;
        emit strokeColorChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty real QtQuick.Shapes::ShapePath::strokeWidth

    This property holds the stroke width.

    When set to a negative value, no stroking occurs.

    The default value is 1.
 */

qreal QQuickShapePath::strokeWidth() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.strokeWidth;
}

void QQuickShapePath::setStrokeWidth(qreal w)
{
    Q_D(QQuickShapePath);
    if (d->sfp.strokeWidth != w) {
        d->sfp.strokeWidth = w;
        d->dirty |= QQuickShapePathPrivate::DirtyStrokeWidth;
        emit strokeWidthChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty color QtQuick.Shapes::ShapePath::fillColor

    This property holds the fill color.

    When set to \c transparent, no filling occurs.

    The default value is \c white.
 */

QColor QQuickShapePath::fillColor() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.fillColor;
}

void QQuickShapePath::setFillColor(const QColor &color)
{
    Q_D(QQuickShapePath);
    if (d->sfp.fillColor != color) {
        d->sfp.fillColor = color;
        d->dirty |= QQuickShapePathPrivate::DirtyFillColor;
        emit fillColorChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::ShapePath::fillRule

    This property holds the fill rule. The default value is
    \c ShapePath.OddEvenFill. For an explanation on fill rules, see
    QPainterPath::setFillRule().

    \value ShapePath.OddEvenFill
           Odd-even fill rule.

    \value ShapePath.WindingFill
           Non-zero winding fill rule.
 */

QQuickShapePath::FillRule QQuickShapePath::fillRule() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.fillRule;
}

void QQuickShapePath::setFillRule(FillRule fillRule)
{
    Q_D(QQuickShapePath);
    if (d->sfp.fillRule != fillRule) {
        d->sfp.fillRule = fillRule;
        d->dirty |= QQuickShapePathPrivate::DirtyFillRule;
        emit fillRuleChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::ShapePath::joinStyle

    This property defines how joins between two connected lines are drawn. The
    default value is \c ShapePath.BevelJoin.

    \value ShapePath.MiterJoin
           The outer edges of the lines are extended to meet at an angle, and
           this area is filled.

    \value ShapePath.BevelJoin
           The triangular notch between the two lines is filled.

    \value ShapePath.RoundJoin
           A circular arc between the two lines is filled.
 */

QQuickShapePath::JoinStyle QQuickShapePath::joinStyle() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.joinStyle;
}

void QQuickShapePath::setJoinStyle(JoinStyle style)
{
    Q_D(QQuickShapePath);
    if (d->sfp.joinStyle != style) {
        d->sfp.joinStyle = style;
        d->dirty |= QQuickShapePathPrivate::DirtyStyle;
        emit joinStyleChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty int QtQuick.Shapes::ShapePath::miterLimit

    When joinStyle is set to \c ShapePath.MiterJoin, this property
    specifies how far the miter join can extend from the join point.

    The default value is 2.
 */

int QQuickShapePath::miterLimit() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.miterLimit;
}

void QQuickShapePath::setMiterLimit(int limit)
{
    Q_D(QQuickShapePath);
    if (d->sfp.miterLimit != limit) {
        d->sfp.miterLimit = limit;
        d->dirty |= QQuickShapePathPrivate::DirtyStyle;
        emit miterLimitChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::ShapePath::capStyle

    This property defines how the end points of lines are drawn. The
    default value is \c ShapePath.SquareCap.

    \value ShapePath.FlatCap
           A square line end that does not cover the end point of the line.

    \value ShapePath.SquareCap
           A square line end that covers the end point and extends beyond it
           by half the line width.

    \value ShapePath.RoundCap
           A rounded line end.
 */

QQuickShapePath::CapStyle QQuickShapePath::capStyle() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.capStyle;
}

void QQuickShapePath::setCapStyle(CapStyle style)
{
    Q_D(QQuickShapePath);
    if (d->sfp.capStyle != style) {
        d->sfp.capStyle = style;
        d->dirty |= QQuickShapePathPrivate::DirtyStyle;
        emit capStyleChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::ShapePath::strokeStyle

    This property defines the style of stroking. The default value is
    ShapePath.SolidLine.

    \value ShapePath.SolidLine  A plain line.
    \value ShapePath.DashLine   Dashes separated by a few pixels.
 */

QQuickShapePath::StrokeStyle QQuickShapePath::strokeStyle() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.strokeStyle;
}

void QQuickShapePath::setStrokeStyle(StrokeStyle style)
{
    Q_D(QQuickShapePath);
    if (d->sfp.strokeStyle != style) {
        d->sfp.strokeStyle = style;
        d->dirty |= QQuickShapePathPrivate::DirtyDash;
        emit strokeStyleChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty real QtQuick.Shapes::ShapePath::dashOffset

    This property defines the starting point on the dash pattern, measured in
    units used to specify the dash pattern.

    The default value is 0.

    \sa QPen::setDashOffset()
 */

qreal QQuickShapePath::dashOffset() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.dashOffset;
}

void QQuickShapePath::setDashOffset(qreal offset)
{
    Q_D(QQuickShapePath);
    if (d->sfp.dashOffset != offset) {
        d->sfp.dashOffset = offset;
        d->dirty |= QQuickShapePathPrivate::DirtyDash;
        emit dashOffsetChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty list<real> QtQuick.Shapes::ShapePath::dashPattern

    This property defines the dash pattern when ShapePath.strokeStyle is set
    to ShapePath.DashLine. The pattern must be specified as an even number of
    positive entries where the entries 1, 3, 5... are the dashes and 2, 4,
    6... are the spaces. The pattern is specified in units of the pen's width.

    The default value is (4, 2), meaning a dash of 4 * ShapePath.strokeWidth
    pixels followed by a space of 2 * ShapePath.strokeWidth pixels.

    \sa QPen::setDashPattern()
 */

QVector<qreal> QQuickShapePath::dashPattern() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.dashPattern;
}

void QQuickShapePath::setDashPattern(const QVector<qreal> &array)
{
    Q_D(QQuickShapePath);
    if (d->sfp.dashPattern != array) {
        d->sfp.dashPattern = array;
        d->dirty |= QQuickShapePathPrivate::DirtyDash;
        emit dashPatternChanged();
        emit shapePathChanged();
    }
}

/*!
    \qmlproperty ShapeGradient QtQuick.Shapes::ShapePath::fillGradient

    This property defines the fill gradient. By default no gradient is enabled
    and the value is \c null. In this case the fill uses a solid color based
    on the value of ShapePath.fillColor.

    When set, ShapePath.fillColor is ignored and filling is done using one of
    the ShapeGradient subtypes.

    \note The Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like LinearGradient.
 */

QQuickShapeGradient *QQuickShapePath::fillGradient() const
{
    Q_D(const QQuickShapePath);
    return d->sfp.fillGradient;
}

void QQuickShapePath::setFillGradient(QQuickShapeGradient *gradient)
{
    Q_D(QQuickShapePath);
    if (d->sfp.fillGradient != gradient) {
        if (d->sfp.fillGradient)
            qmlobject_disconnect(d->sfp.fillGradient, QQuickShapeGradient, SIGNAL(updated()),
                                 this, QQuickShapePath, SLOT(_q_fillGradientChanged()));
        d->sfp.fillGradient = gradient;
        if (d->sfp.fillGradient)
            qmlobject_connect(d->sfp.fillGradient, QQuickShapeGradient, SIGNAL(updated()),
                              this, QQuickShapePath, SLOT(_q_fillGradientChanged()));
        d->dirty |= QQuickShapePathPrivate::DirtyFillGradient;
        emit shapePathChanged();
    }
}

void QQuickShapePathPrivate::_q_fillGradientChanged()
{
    Q_Q(QQuickShapePath);
    dirty |= DirtyFillGradient;
    emit q->shapePathChanged();
}

void QQuickShapePath::resetFillGradient()
{
    setFillGradient(nullptr);
}

/*!
    \qmltype Shape
    //! \instantiates QQuickShape
    \inqmlmodule QtQuick.Shapes
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits Item
    \brief Renders a path.
    \since 5.10

    Renders a path by triangulating geometry from a QPainterPath.

    This approach is different from rendering shapes via QQuickPaintedItem or
    the 2D Canvas because the path never gets rasterized in software.
    Therefore Shape is suitable for creating shapes spreading over larger
    areas of the screen, avoiding the performance penalty for texture uploads
    or framebuffer blits. In addition, the declarative API allows manipulating,
    binding to, and even animating the path element properties like starting
    and ending position, the control points, and so on.

    The types for specifying path elements are shared between \l PathView and
    Shape. However, not all Shape implementations support all path
    element types, while some may not make sense for PathView. Shape's
    currently supported subset is: PathMove, PathLine, PathQuad, PathCubic,
    PathArc, PathText and PathSvg.

    See \l Path for a detailed overview of the supported path elements.

    \qml
    Shape {
        width: 200
        height: 150
        anchors.centerIn: parent
        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            fillGradient: LinearGradient {
                x1: 20; y1: 20
                x2: 180; y2: 130
                GradientStop { position: 0; color: "blue" }
                GradientStop { position: 0.2; color: "green" }
                GradientStop { position: 0.4; color: "red" }
                GradientStop { position: 0.6; color: "yellow" }
                GradientStop { position: 1; color: "cyan" }
            }
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 20; startY: 20
            PathLine { x: 180; y: 130 }
            PathLine { x: 20; y: 130 }
            PathLine { x: 20; y: 20 }
        }
    }
    \endqml

    \image pathitem-code-example.png

    Like \l Item, Shape also allows any visual or non-visual objects to be
    declared as children. ShapePath objects are handled specially. This is
    useful since it allows adding visual items, like \l Rectangle or \l Image,
    and non-visual objects, like \l Timer directly as children of Shape.

    The following list summarizes the available Shape rendering approaches:

    \list

    \li When Qt Quick is running with the default, hardware-accelerated backend (RHI),
    the generic shape renderer will be used. This converts the shapes into triangles
    which are passed to the renderer.

    \li The \c software backend is fully supported. The path is rendered via
    QPainter::strokePath() and QPainter::fillPath() in this case.

    \li The OpenVG backend is not currently supported.

    \endlist

    When using Shape, it is important to be aware of potential performance
    implications:

    \list

    \li When the application is running with the generic, triangulation-based
    Shape implementation, the geometry generation happens entirely on the
    CPU. This is potentially expensive. Changing the set of path elements,
    changing the properties of these elements, or changing certain properties
    of the Shape itself all lead to retriangulation of the affected paths on
    every change. Therefore, applying animation to such properties can affect
    performance on less powerful systems.

    \li However, the data-driven, declarative nature of the Shape API often
    means better cacheability for the underlying CPU and GPU resources. A
    property change in one ShapePath will only lead to reprocessing the
    affected ShapePath, leaving other parts of the Shape unchanged. Therefore,
    a frequently changing property can still result in a lower overall system
    load than with imperative painting approaches (for example, QPainter).

    \li At the same time, attention must be paid to the number of Shape
    elements in the scene. The way such a Shape item is represented in
    the scene graph is different from an ordinary geometry-based item,
    and incurs a certain cost when it comes to OpenGL state changes.

    \li As a general rule, scenes should avoid using separate Shape items when
    it is not absolutely necessary. Prefer using one Shape item with multiple
    ShapePath elements over multiple Shape items.

    \endlist

    \sa {Qt Quick Examples - Shapes}, Path, PathMove, PathLine, PathQuad, PathCubic, PathArc, PathSvg
*/

QQuickShapePrivate::QQuickShapePrivate()
      : effectRefCount(0)
{
}

QQuickShapePrivate::~QQuickShapePrivate()
{
    delete renderer;
}

void QQuickShapePrivate::_q_shapePathChanged()
{
    Q_Q(QQuickShape);
    spChanged = true;
    q->polish();
    emit q->boundingRectChanged();
}

void QQuickShapePrivate::setStatus(QQuickShape::Status newStatus)
{
    Q_Q(QQuickShape);
    if (status != newStatus) {
        status = newStatus;
        emit q->statusChanged();
    }
}

QQuickShape::QQuickShape(QQuickItem *parent)
  : QQuickItem(*(new QQuickShapePrivate), parent)
{
    setFlag(ItemHasContents);
}

QQuickShape::~QQuickShape()
{
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::Shape::rendererType
    \readonly

    This property determines which path rendering backend is active.

    \value Shape.UnknownRenderer
           The renderer is unknown.

    \value Shape.GeometryRenderer
           The generic, driver independent solution for GPU rendering. Uses the same
           CPU-based triangulation approach as QPainter's OpenGL 2 paint
           engine. This is the default when the RHI-based Qt Quick scenegraph
           backend is in use.

    \value Shape.SoftwareRenderer
           Pure QPainter drawing using the raster paint engine. This is the
           default, and only, option when the Qt Quick scenegraph is running
           with the \c software backend.

    \value Shape.CurveRenderer
           Experimental GPU-based renderer, added as technology preview in Qt 6.6.
           In contrast to \c Shape.GeometryRenderer, curves are not approximated by short straight
           lines. Instead, curves are rendered using a specialized fragment shader. This improves
           visual quality and avoids re-tesselation performance hit when zooming. Also,
           \c Shape.CurveRenderer provides native, high-quality anti-aliasing, without the
           performance cost of multi- or supersampling.

    By default, \c Shape.GeometryRenderer will be selected unless the Qt Quick scenegraph is running
    with the \c software backend. In that case, \c Shape.SoftwareRenderer will be used.
    \c Shape.CurveRenderer may be requested using the \l preferredRendererType property.

    Note that \c Shape.CurveRenderer is currently regarded as experimental. The enum name of
    this renderer may change in future versions of Qt, and some shapes may render incorrectly.
    Among the known limitations are:
    \list 1
      \li Only quadratic curves are inherently supported. Cubic curves will be approximated by
          quadratic curves.
      \li Shapes where elements intersect are not rendered correctly. The \l [QML] {Path::simplify}
          {Path.simplify} property may be used to remove self-intersections from such shapes, but
          may incur a performance cost and reduced visual quality.
      \li Shapes that span a large numerical range, such as a long string of text, may have
          issues. Consider splitting these shapes into multiple ones, for instance by making
          a \l PathText for each individual word.
      \li If the shape is being rendered into a Qt Quick 3D scene, the
          \c GL_OES_standard_derivatives extension to OpenGL is required when the OpenGL
          RHI backend is in use (this is available by default on OpenGL ES 3 and later, but
          optional in OpenGL ES 2).
    \endlist
*/

QQuickShape::RendererType QQuickShape::rendererType() const
{
    Q_D(const QQuickShape);
    return d->rendererType;
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::Shape::preferredRendererType
    \since 6.6

    Requests a specific backend to use for rendering the shape. The possible values are the same as
    for \l rendererType. The default is \c Shape.UnknownRenderer, indicating no particular preference.

    If the requested renderer type is not supported for the current Qt Quick backend, the default
    renderer for that backend will be used instead. This will be reflected in the \l rendererType
    when the backend is initialized.

    \c Shape.SoftwareRenderer can currently not be selected without running the scenegraph with
    the \c software backend, in which case it will be selected regardless of the
    \c preferredRendererType.

    \note This API is considered tech preview and may change or be removed in future versions of
    Qt.

    See \l rendererType for more information on the implications.
*/

QQuickShape::RendererType QQuickShape::preferredRendererType() const
{
    Q_D(const QQuickShape);
    return d->preferredType;
}

void QQuickShape::setPreferredRendererType(QQuickShape::RendererType preferredType)
{
    Q_D(QQuickShape);
    if (d->preferredType == preferredType)
        return;

    d->preferredType = preferredType;
    // (could bail out here if selectRenderType shows no change?)

    for (int i = 0; i < d->sp.size(); ++i) {
        QQuickShapePath *p = d->sp[i];
        QQuickShapePathPrivate *pp = QQuickShapePathPrivate::get(p);
        pp->dirty |= QQuickShapePathPrivate::DirtyAll;
    }
    d->spChanged = true;
    d->_q_shapePathChanged();
    polish();
    update();

    emit preferredRendererTypeChanged();
}


/*!
    \qmlproperty bool QtQuick.Shapes::Shape::asynchronous

    When rendererType is \c Shape.GeometryRenderer, the input path is
    triangulated on the CPU during the polishing phase of the Shape. This is
    potentially expensive. To offload this work to separate worker threads,
    set this property to \c true.

    When enabled, making a Shape visible will not wait for the content to
    become available. Instead, the GUI/main thread is not blocked and the
    results of the path rendering are shown only when all the asynchronous
    work has been finished.

    The default value is \c false.
 */

bool QQuickShape::asynchronous() const
{
    Q_D(const QQuickShape);
    return d->async;
}

void QQuickShape::setAsynchronous(bool async)
{
    Q_D(QQuickShape);
    if (d->async != async) {
        d->async = async;
        emit asynchronousChanged();
        if (d->componentComplete)
            d->_q_shapePathChanged();
    }
}

/*!
    \qmlproperty rect QtQuick.Shapes::Shape::boundingRect
    \since 6.6

    Contains the united bounding rect of all sub paths in the shape.
 */
QRectF QQuickShape::boundingRect() const
{
    Q_D(const QQuickShape);
    QRectF brect;
    for (QQuickShapePath *path : d->sp) {
        brect = brect.united(path->path().boundingRect());
    }

    return brect;
}

/*!
    \qmlproperty bool QtQuick.Shapes::Shape::vendorExtensionsEnabled

    This property controls the usage of non-standard OpenGL extensions.

    The default value is \c false.

    As of Qt 6.0 there are no vendor-specific rendering paths implemented.
 */

bool QQuickShape::vendorExtensionsEnabled() const
{
    Q_D(const QQuickShape);
    return d->enableVendorExts;
}

void QQuickShape::setVendorExtensionsEnabled(bool enable)
{
    Q_D(QQuickShape);
    if (d->enableVendorExts != enable) {
        d->enableVendorExts = enable;
        emit vendorExtensionsEnabledChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::Shape::status
    \readonly

    This property determines the status of the Shape and is relevant when
    Shape.asynchronous is set to \c true.

    \value Shape.Null
           Not yet initialized.

    \value Shape.Ready
           The Shape has finished processing.

    \value Shape.Processing
           The path is being processed.
 */

QQuickShape::Status QQuickShape::status() const
{
    Q_D(const QQuickShape);
    return d->status;
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::Shape::containsMode
    \since QtQuick.Shapes 1.11

    This property determines the definition of \l {QQuickItem::contains()}{contains()}
    for the Shape. It is useful in case you add \l {Qt Quick Input Handlers} and you want to
    react only when the mouse or touchpoint is fully inside the Shape.

    \value Shape.BoundingRectContains
        The default implementation of \l QQuickItem::contains() checks only
        whether the given point is inside the rectangular bounding box. This is
        the most efficient implementation, which is why it's the default.

    \value Shape.FillContains
        Check whether the interior (the part that would be filled if you are
        rendering it with fill) of any \l ShapePath that makes up this Shape
        contains the given point. The more complex and numerous ShapePaths you
        add, the less efficient this is to check, which can potentially slow
        down event delivery in your application. So it should be used with care.

    One way to speed up the \c FillContains check is to generate an approximate
    outline with as few points as possible, place that in a transparent Shape
    on top, and add your Pointer Handlers to that, so that the containment
    check is cheaper during event delivery.
*/
QQuickShape::ContainsMode QQuickShape::containsMode() const
{
    Q_D(const QQuickShape);
    return d->containsMode;
}

void QQuickShape::setContainsMode(QQuickShape::ContainsMode containsMode)
{
    Q_D(QQuickShape);
    if (d->containsMode == containsMode)
        return;

    d->containsMode = containsMode;
    emit containsModeChanged();
}

bool QQuickShape::contains(const QPointF &point) const
{
    Q_D(const QQuickShape);
    switch (d->containsMode) {
    case BoundingRectContains:
        return QQuickItem::contains(point);
    case FillContains:
        for (QQuickShapePath *path : d->sp) {
            if (path->path().contains(point))
                return true;
        }
    }
    return false;
}

static void vpe_append(QQmlListProperty<QObject> *property, QObject *obj)
{
    QQuickShape *item = static_cast<QQuickShape *>(property->object);
    QQuickShapePrivate *d = QQuickShapePrivate::get(item);
    QQuickShapePath *path = qobject_cast<QQuickShapePath *>(obj);
    if (path)
        d->sp.append(path);

    QQuickItemPrivate::data_append(property, obj);

    if (path && d->componentComplete) {
        QObject::connect(path, SIGNAL(shapePathChanged()), item, SLOT(_q_shapePathChanged()));
        d->_q_shapePathChanged();
    }
}

static void vpe_clear(QQmlListProperty<QObject> *property)
{
    QQuickShape *item = static_cast<QQuickShape *>(property->object);
    QQuickShapePrivate *d = QQuickShapePrivate::get(item);

    for (QQuickShapePath *p : d->sp)
        QObject::disconnect(p, SIGNAL(shapePathChanged()), item, SLOT(_q_shapePathChanged()));

    d->sp.clear();

    QQuickItemPrivate::data_clear(property);

    if (d->componentComplete)
        d->_q_shapePathChanged();
}

/*!
    \qmlproperty list<Object> QtQuick.Shapes::Shape::data

    This property holds the ShapePath objects that define the contents of the
    Shape. It can also contain any other type of objects, since Shape, like
    Item, allows adding any visual or non-visual objects as children.

    \qmldefault
 */

QQmlListProperty<QObject> QQuickShape::data()
{
    return QQmlListProperty<QObject>(this,
                                     nullptr,
                                     vpe_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     vpe_clear);
}

void QQuickShape::classBegin()
{
    QQuickItem::classBegin();
}

void QQuickShape::componentComplete()
{
    Q_D(QQuickShape);

    QQuickItem::componentComplete();

    for (QQuickShapePath *p : d->sp)
        connect(p, SIGNAL(shapePathChanged()), this, SLOT(_q_shapePathChanged()));

    d->_q_shapePathChanged();
}

void QQuickShape::updatePolish()
{
    Q_D(QQuickShape);

    const int currentEffectRefCount = d->extra.isAllocated() ? d->extra->recursiveEffectRefCount : 0;
    if (!d->spChanged && currentEffectRefCount <= d->effectRefCount)
        return;

    d->spChanged = false;
    d->effectRefCount = currentEffectRefCount;

    QQuickShape::RendererType expectedRenderer = d->selectRendererType();
    if (d->rendererType != expectedRenderer) {
        delete d->renderer;
        d->renderer = nullptr;
    }

    if (!d->renderer) {
        d->createRenderer();
        if (!d->renderer)
            return;
        emit rendererChanged();
    }

    // endSync() is where expensive calculations may happen (or get kicked off
    // on worker threads), depending on the backend. Therefore do this only
    // when the item is visible.
    if (isVisible() || d->effectRefCount > 0)
        d->sync();
}

void QQuickShape::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickShape);

    // sync may have been deferred; do it now if the item became visible
    if (change == ItemVisibleHasChanged && data.boolValue)
        d->_q_shapePathChanged();
    else if (change == QQuickItem::ItemSceneChange) {
        for (int i = 0; i < d->sp.size(); ++i)
            QQuickShapePathPrivate::get(d->sp[i])->dirty = QQuickShapePathPrivate::DirtyAll;
        d->_q_shapePathChanged();
    }

    QQuickItem::itemChange(change, data);
}

QSGNode *QQuickShape::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    // Called on the render thread, with the gui thread blocked. We can now
    // safely access gui thread data.
    Q_D(QQuickShape);

    if (d->renderer || d->rendererChanged) {
        if (!node || d->rendererChanged) {
            d->rendererChanged = false;
            delete node;
            node = d->createNode();
        }
        if (d->renderer)
            d->renderer->updateNode();
    }
    return node;
}

QQuickShape::RendererType QQuickShapePrivate::selectRendererType()
{
    QQuickShape::RendererType res = QQuickShape::UnknownRenderer;
    Q_Q(QQuickShape);
    QSGRendererInterface *ri = q->window()->rendererInterface();
    if (!ri)
        return res;

    static const bool environmentPreferCurve =
        qEnvironmentVariable("QT_QUICKSHAPES_BACKEND").toLower() == QLatin1String("curverenderer");

    switch (ri->graphicsApi()) {
    case QSGRendererInterface::Software:
        res = QQuickShape::SoftwareRenderer;
        break;
    default:
        if (QSGRendererInterface::isApiRhiBased(ri->graphicsApi())) {
            if (preferredType == QQuickShape::CurveRenderer || environmentPreferCurve) {
                res = QQuickShape::CurveRenderer;
            } else {
                res = QQuickShape::GeometryRenderer;
            }
        } else {
            qWarning("No path backend for this graphics API yet");
        }
        break;
    }

    return res;
}

// the renderer object lives on the gui thread
void QQuickShapePrivate::createRenderer()
{
    Q_Q(QQuickShape);
    QQuickShape::RendererType selectedType = selectRendererType();
    if (selectedType == QQuickShape::UnknownRenderer)
        return;

    rendererType = selectedType;
    rendererChanged = true;

    switch (selectedType) {
    case QQuickShape::SoftwareRenderer:
        renderer = new QQuickShapeSoftwareRenderer;
        break;
    case QQuickShape::GeometryRenderer:
        renderer = new QQuickShapeGenericRenderer(q);
        break;
    case QQuickShape::CurveRenderer:
        renderer = new QQuickShapeCurveRenderer(q);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

// the node lives on the render thread
QSGNode *QQuickShapePrivate::createNode()
{
    Q_Q(QQuickShape);
    QSGNode *node = nullptr;
    if (!q->window() || !renderer)
        return node;
    QSGRendererInterface *ri = q->window()->rendererInterface();
    if (!ri)
        return node;

    switch (ri->graphicsApi()) {
    case QSGRendererInterface::Software:
        node = new QQuickShapeSoftwareRenderNode(q);
        static_cast<QQuickShapeSoftwareRenderer *>(renderer)->setNode(
                    static_cast<QQuickShapeSoftwareRenderNode *>(node));
        break;
    default:
        if (QSGRendererInterface::isApiRhiBased(ri->graphicsApi())) {
            if (rendererType == QQuickShape::CurveRenderer) {
                node = new QSGNode;
                static_cast<QQuickShapeCurveRenderer *>(renderer)->setRootNode(node);
            } else {
                node = new QQuickShapeGenericNode;
                static_cast<QQuickShapeGenericRenderer *>(renderer)->setRootNode(
                    static_cast<QQuickShapeGenericNode *>(node));
            }
        } else {
            qWarning("No path backend for this graphics API yet");
        }
        break;
    }

    return node;
}

void QQuickShapePrivate::asyncShapeReady(void *data)
{
    QQuickShapePrivate *self = static_cast<QQuickShapePrivate *>(data);
    self->setStatus(QQuickShape::Ready);
    if (self->syncTimingActive)
        qDebug("[Shape %p] [%d] [dirty=0x%x] async update took %lld ms",
               self->q_func(), self->syncTimeCounter, self->syncTimingTotalDirty, self->syncTimer.elapsed());
}

void QQuickShapePrivate::sync()
{
    int totalDirty = 0;
    syncTimingActive = QQSHAPE_LOG_TIME_DIRTY_SYNC().isDebugEnabled();
    if (syncTimingActive)
        syncTimer.start();

    const bool useAsync = async && renderer->flags().testFlag(QQuickAbstractPathRenderer::SupportsAsync);
    if (useAsync) {
        setStatus(QQuickShape::Processing);
        renderer->setAsyncCallback(asyncShapeReady, this);
    }

    const int count = sp.size();
    bool countChanged = false;
    renderer->beginSync(count, &countChanged);
    renderer->setTriangulationScale(triangulationScale);

    for (int i = 0; i < count; ++i) {
        QQuickShapePath *p = sp[i];
        int &dirty(QQuickShapePathPrivate::get(p)->dirty);
        totalDirty |= dirty;

        if (dirty & QQuickShapePathPrivate::DirtyPath)
            renderer->setPath(i, p);
        if (dirty & QQuickShapePathPrivate::DirtyStrokeColor)
            renderer->setStrokeColor(i, p->strokeColor());
        if (dirty & QQuickShapePathPrivate::DirtyStrokeWidth)
            renderer->setStrokeWidth(i, p->strokeWidth());
        if (dirty & QQuickShapePathPrivate::DirtyFillColor)
            renderer->setFillColor(i, p->fillColor());
        if (dirty & QQuickShapePathPrivate::DirtyFillRule)
            renderer->setFillRule(i, p->fillRule());
        if (dirty & QQuickShapePathPrivate::DirtyStyle) {
            renderer->setJoinStyle(i, p->joinStyle(), p->miterLimit());
            renderer->setCapStyle(i, p->capStyle());
        }
        if (dirty & QQuickShapePathPrivate::DirtyDash)
            renderer->setStrokeStyle(i, p->strokeStyle(), p->dashOffset(), p->dashPattern());
        if (dirty & QQuickShapePathPrivate::DirtyFillGradient)
            renderer->setFillGradient(i, p->fillGradient());

        dirty = 0;
    }

    syncTimingTotalDirty = totalDirty;
    if (syncTimingTotalDirty)
        ++syncTimeCounter;
    else
        syncTimingActive = false;

    renderer->endSync(useAsync);

    if (!useAsync) {
        setStatus(QQuickShape::Ready);
        if (syncTimingActive)
            qDebug("[Shape %p] [%d] [dirty=0x%x] update took %lld ms",
                   q_func(), syncTimeCounter, syncTimingTotalDirty, syncTimer.elapsed());
    }

    // Must dirty the QQuickItem if something got changed, nothing
    // else does this for us.
    Q_Q(QQuickShape);
    if (totalDirty || countChanged)
        q->update();
}

// ***** gradient support *****

/*!
    \qmltype ShapeGradient
    //! \instantiates QQuickShapeGradient
    \inqmlmodule QtQuick.Shapes
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits Gradient
    \brief Base type of Shape fill gradients.
    \since 5.10

    This is an abstract base class for gradients like LinearGradient and
    cannot be created directly. It extends \l Gradient with properties like the
    spread mode.
 */

QQuickShapeGradient::QQuickShapeGradient(QObject *parent)
    : QQuickGradient(parent),
      m_spread(PadSpread)
{
}

/*!
    \qmlproperty enumeration QtQuick.Shapes::ShapeGradient::spread

    Specifies how the area outside the gradient area should be filled. The
    default value is \c ShapeGradient.PadSpread.

    \value ShapeGradient.PadSpread
           The area is filled with the closest stop color.

    \value ShapeGradient.RepeatSpread
           The gradient is repeated outside the gradient area.

    \value ShapeGradient.ReflectSpread
           The gradient is reflected outside the gradient area.
 */

QQuickShapeGradient::SpreadMode QQuickShapeGradient::spread() const
{
    return m_spread;
}

void QQuickShapeGradient::setSpread(SpreadMode mode)
{
    if (m_spread != mode) {
        m_spread = mode;
        emit spreadChanged();
        emit updated();
    }
}

/*!
    \qmltype LinearGradient
    //! \instantiates QQuickShapeLinearGradient
    \inqmlmodule QtQuick.Shapes
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits ShapeGradient
    \brief Linear gradient.
    \since 5.10

    Linear gradients interpolate colors between start and end points in Shape
    items. Outside these points the gradient is either padded, reflected or
    repeated depending on the spread type.

    \note LinearGradient is only supported in combination with Shape items. It
    is not compatible with \l Rectangle, as that only supports \l Gradient.

    \sa QLinearGradient
 */

QQuickShapeLinearGradient::QQuickShapeLinearGradient(QObject *parent)
    : QQuickShapeGradient(parent)
{
}

/*!
    \qmlproperty real QtQuick.Shapes::LinearGradient::x1
    \qmlproperty real QtQuick.Shapes::LinearGradient::y1
    \qmlproperty real QtQuick.Shapes::LinearGradient::x2
    \qmlproperty real QtQuick.Shapes::LinearGradient::y2

    These properties define the start and end points between which color
    interpolation occurs. By default both points are set to (0, 0).
 */

qreal QQuickShapeLinearGradient::x1() const
{
    return m_start.x();
}

void QQuickShapeLinearGradient::setX1(qreal v)
{
    if (m_start.x() != v) {
        m_start.setX(v);
        emit x1Changed();
        emit updated();
    }
}

qreal QQuickShapeLinearGradient::y1() const
{
    return m_start.y();
}

void QQuickShapeLinearGradient::setY1(qreal v)
{
    if (m_start.y() != v) {
        m_start.setY(v);
        emit y1Changed();
        emit updated();
    }
}

qreal QQuickShapeLinearGradient::x2() const
{
    return m_end.x();
}

void QQuickShapeLinearGradient::setX2(qreal v)
{
    if (m_end.x() != v) {
        m_end.setX(v);
        emit x2Changed();
        emit updated();
    }
}

qreal QQuickShapeLinearGradient::y2() const
{
    return m_end.y();
}

void QQuickShapeLinearGradient::setY2(qreal v)
{
    if (m_end.y() != v) {
        m_end.setY(v);
        emit y2Changed();
        emit updated();
    }
}

/*!
    \qmltype RadialGradient
    //! \instantiates QQuickShapeRadialGradient
    \inqmlmodule QtQuick.Shapes
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits ShapeGradient
    \brief Radial gradient.
    \since 5.10

    Radial gradients interpolate colors between a focal circle and a center
    circle in Shape items. Points outside the cone defined by the two circles
    will be transparent.

    Outside the end points the gradient is either padded, reflected or repeated
    depending on the spread type.

    Below is an example of a simple radial gradient. Here the colors are
    interpolated between the specified point and the end points on a circle
    specified by the radius:

    \code
        fillGradient: RadialGradient {
            centerX: 50; centerY: 50
            centerRadius: 100
            focalX: centerX; focalY: centerY
            GradientStop { position: 0; color: "blue" }
            GradientStop { position: 0.2; color: "green" }
            GradientStop { position: 0.4; color: "red" }
            GradientStop { position: 0.6; color: "yellow" }
            GradientStop { position: 1; color: "cyan" }
        }
    \endcode

    \image shape-radial-gradient.png

    Extended radial gradients, where a separate focal circle is specified, are
    also supported.

    \note RadialGradient is only supported in combination with Shape items. It
    is not compatible with \l Rectangle, as that only supports \l Gradient.

    \sa QRadialGradient
 */

QQuickShapeRadialGradient::QQuickShapeRadialGradient(QObject *parent)
    : QQuickShapeGradient(parent)
{
}

/*!
    \qmlproperty real QtQuick.Shapes::RadialGradient::centerX
    \qmlproperty real QtQuick.Shapes::RadialGradient::centerY
    \qmlproperty real QtQuick.Shapes::RadialGradient::focalX
    \qmlproperty real QtQuick.Shapes::RadialGradient::focalY

    These properties define the center and focal points. To specify a simple
    radial gradient, set focalX and focalY to the value of centerX and
    centerY, respectively.
 */

qreal QQuickShapeRadialGradient::centerX() const
{
    return m_centerPoint.x();
}

void QQuickShapeRadialGradient::setCenterX(qreal v)
{
    if (m_centerPoint.x() != v) {
        m_centerPoint.setX(v);
        emit centerXChanged();
        emit updated();
    }
}

qreal QQuickShapeRadialGradient::centerY() const
{
    return m_centerPoint.y();
}

void QQuickShapeRadialGradient::setCenterY(qreal v)
{
    if (m_centerPoint.y() != v) {
        m_centerPoint.setY(v);
        emit centerYChanged();
        emit updated();
    }
}

/*!
    \qmlproperty real QtQuick.Shapes::RadialGradient::centerRadius
    \qmlproperty real QtQuick.Shapes::RadialGradient::focalRadius

    These properties define the center and focal radius. For simple radial
    gradients, focalRadius should be set to \c 0 (the default value).
 */

qreal QQuickShapeRadialGradient::centerRadius() const
{
    return m_centerRadius;
}

void QQuickShapeRadialGradient::setCenterRadius(qreal v)
{
    if (m_centerRadius != v) {
        m_centerRadius = v;
        emit centerRadiusChanged();
        emit updated();
    }
}

qreal QQuickShapeRadialGradient::focalX() const
{
    return m_focalPoint.x();
}

void QQuickShapeRadialGradient::setFocalX(qreal v)
{
    if (m_focalPoint.x() != v) {
        m_focalPoint.setX(v);
        emit focalXChanged();
        emit updated();
    }
}

qreal QQuickShapeRadialGradient::focalY() const
{
    return m_focalPoint.y();
}

void QQuickShapeRadialGradient::setFocalY(qreal v)
{
    if (m_focalPoint.y() != v) {
        m_focalPoint.setY(v);
        emit focalYChanged();
        emit updated();
    }
}

qreal QQuickShapeRadialGradient::focalRadius() const
{
    return m_focalRadius;
}

void QQuickShapeRadialGradient::setFocalRadius(qreal v)
{
    if (m_focalRadius != v) {
        m_focalRadius = v;
        emit focalRadiusChanged();
        emit updated();
    }
}

/*!
    \qmltype ConicalGradient
    //! \instantiates QQuickShapeConicalGradient
    \inqmlmodule QtQuick.Shapes
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits ShapeGradient
    \brief Conical gradient.
    \since 5.10

    Conical gradients interpolate colors counter-clockwise around a center
    point in Shape items.

    \note The \l{ShapeGradient::spread}{spread mode} setting has no effect for
    conical gradients.

    \note ConicalGradient is only supported in combination with Shape items. It
    is not compatible with \l Rectangle, as that only supports \l Gradient.

    \sa QConicalGradient
 */

QQuickShapeConicalGradient::QQuickShapeConicalGradient(QObject *parent)
    : QQuickShapeGradient(parent)
{
}

/*!
    \qmlproperty real QtQuick.Shapes::ConicalGradient::centerX
    \qmlproperty real QtQuick.Shapes::ConicalGradient::centerY

    These properties define the center point of the conical gradient.
 */

qreal QQuickShapeConicalGradient::centerX() const
{
    return m_centerPoint.x();
}

void QQuickShapeConicalGradient::setCenterX(qreal v)
{
    if (m_centerPoint.x() != v) {
        m_centerPoint.setX(v);
        emit centerXChanged();
        emit updated();
    }
}

qreal QQuickShapeConicalGradient::centerY() const
{
    return m_centerPoint.y();
}

void QQuickShapeConicalGradient::setCenterY(qreal v)
{
    if (m_centerPoint.y() != v) {
        m_centerPoint.setY(v);
        emit centerYChanged();
        emit updated();
    }
}

/*!
    \qmlproperty real QtQuick.Shapes::ConicalGradient::angle

    This property defines the start angle for the conical gradient. The value
    is in degrees (0-360).
 */

qreal QQuickShapeConicalGradient::angle() const
{
    return m_angle;
}

void QQuickShapeConicalGradient::setAngle(qreal v)
{
    if (m_angle != v) {
        m_angle = v;
        emit angleChanged();
        emit updated();
    }
}

static void generateGradientColorTable(const QQuickShapeGradientCacheKey &gradient,
                                       uint *colorTable, int size, float opacity)
{
    int pos = 0;
    const QGradientStops &s = gradient.stops;
    Q_ASSERT(!s.isEmpty());
    const bool colorInterpolation = true;

    uint alpha = qRound(opacity * 256);
    uint current_color = ARGB_COMBINE_ALPHA(s[0].second.rgba(), alpha);
    qreal incr = 1.0 / qreal(size);
    qreal fpos = 1.5 * incr;
    colorTable[pos++] = ARGB2RGBA(qPremultiply(current_color));

    while (fpos <= s.first().first) {
        colorTable[pos] = colorTable[pos - 1];
        pos++;
        fpos += incr;
    }

    if (colorInterpolation)
        current_color = qPremultiply(current_color);

    const int sLast = s.size() - 1;
    for (int i = 0; i < sLast; ++i) {
        qreal delta = 1/(s[i+1].first - s[i].first);
        uint next_color = ARGB_COMBINE_ALPHA(s[i + 1].second.rgba(), alpha);
        if (colorInterpolation)
            next_color = qPremultiply(next_color);

        while (fpos < s[i+1].first && pos < size) {
            int dist = int(256 * ((fpos - s[i].first) * delta));
            int idist = 256 - dist;
            if (colorInterpolation)
                colorTable[pos] = ARGB2RGBA(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist));
            else
                colorTable[pos] = ARGB2RGBA(qPremultiply(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist)));
            ++pos;
            fpos += incr;
        }
        current_color = next_color;
    }

    uint last_color = ARGB2RGBA(qPremultiply(ARGB_COMBINE_ALPHA(s[sLast].second.rgba(), alpha)));
    for ( ; pos < size; ++pos)
        colorTable[pos] = last_color;

    colorTable[size-1] = last_color;
}

QQuickShapeGradientCache::~QQuickShapeGradientCache()
{
    qDeleteAll(m_textures);
}

QQuickShapeGradientCache *QQuickShapeGradientCache::cacheForRhi(QRhi *rhi)
{
    static QHash<QRhi *, QQuickShapeGradientCache *> caches;
    auto it = caches.constFind(rhi);
    if (it != caches.constEnd())
        return *it;

    QQuickShapeGradientCache *cache = new QQuickShapeGradientCache;
    rhi->addCleanupCallback([cache](QRhi *rhi) {
        caches.remove(rhi);
        delete cache;
    });
    caches.insert(rhi, cache);
    return cache;
}

QSGTexture *QQuickShapeGradientCache::get(const QQuickShapeGradientCacheKey &grad)
{
    QSGPlainTexture *tx = m_textures[grad];
    if (!tx) {
        static const int W = 1024; // texture size is 1024x1
        QImage gradTab(W, 1, QImage::Format_RGBA8888_Premultiplied);
        if (!grad.stops.isEmpty())
            generateGradientColorTable(grad, reinterpret_cast<uint *>(gradTab.bits()), W, 1.0f);
        else
            gradTab.fill(Qt::black);
        tx = new QSGPlainTexture;
        tx->setImage(gradTab);
        switch (grad.spread) {
        case QQuickShapeGradient::PadSpread:
            tx->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            tx->setVerticalWrapMode(QSGTexture::ClampToEdge);
            break;
        case QQuickShapeGradient::RepeatSpread:
            tx->setHorizontalWrapMode(QSGTexture::Repeat);
            tx->setVerticalWrapMode(QSGTexture::Repeat);
            break;
        case QQuickShapeGradient::ReflectSpread:
            tx->setHorizontalWrapMode(QSGTexture::MirroredRepeat);
            tx->setVerticalWrapMode(QSGTexture::MirroredRepeat);
            break;
        default:
            qWarning("Unknown gradient spread mode %d", grad.spread);
            break;
        }
        tx->setFiltering(QSGTexture::Linear);
        m_textures[grad] = tx;
    }
    return tx;
}

QT_END_NAMESPACE

#include "moc_qquickshape_p.cpp"
