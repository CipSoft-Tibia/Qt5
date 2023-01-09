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

#include <private/qsgadaptationlayer_p.h>
#include "qquickcanvasitem_p.h"
#include <private/qquickitem_p.h>
#include <private/qquickcanvascontext_p.h>
#include <private/qquickcontext2d_p.h>
#include <private/qquickcontext2dtexture_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <qsgtextureprovider.h>
#include <QtQuick/private/qquickpixmapcache_p.h>
#include <QtGui/QGuiApplication>
#include <qsgtextureprovider.h>

#include <qqmlinfo.h>
#include <private/qqmlengine_p.h>
#include <QtCore/QBuffer>
#include <QtCore/qdatetime.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

class QQuickCanvasTextureProvider : public QSGTextureProvider
{
public:
    QSGTexture *tex;
    QSGTexture *texture() const override { return tex; }
    void fireTextureChanged() { emit textureChanged(); }
};

QQuickCanvasPixmap::QQuickCanvasPixmap(const QImage& image)
    : m_pixmap(nullptr)
    , m_image(image)
{

}

QQuickCanvasPixmap::QQuickCanvasPixmap(QQuickPixmap *pixmap)
    : m_pixmap(pixmap)
{

}

QQuickCanvasPixmap::~QQuickCanvasPixmap()
{
    delete m_pixmap;
}

qreal QQuickCanvasPixmap::width() const
{
    if (m_pixmap)
        return m_pixmap->width();

    return m_image.width();
}

qreal QQuickCanvasPixmap::height() const
{
    if (m_pixmap)
        return m_pixmap->height();

    return m_image.height();
}

bool QQuickCanvasPixmap::isValid() const
{
    if (m_pixmap)
        return m_pixmap->isReady();
    return !m_image.isNull();
}

QImage QQuickCanvasPixmap::image()
{
    if (m_image.isNull() && m_pixmap)
        m_image = m_pixmap->image();

    return m_image;
}

QHash<QQmlEngine *,QQuickContext2DRenderThread*> QQuickContext2DRenderThread::renderThreads;
QMutex QQuickContext2DRenderThread::renderThreadsMutex;

QQuickContext2DRenderThread::QQuickContext2DRenderThread(QQmlEngine *eng)
    : QThread(eng), m_engine(eng), m_eventLoopQuitHack(nullptr)
{
    Q_ASSERT(eng);
    m_eventLoopQuitHack = new QObject;
    m_eventLoopQuitHack->moveToThread(this);
    connect(m_eventLoopQuitHack, SIGNAL(destroyed(QObject*)), SLOT(quit()), Qt::DirectConnection);
    start(QThread::IdlePriority);
}

QQuickContext2DRenderThread::~QQuickContext2DRenderThread()
{
    renderThreadsMutex.lock();
    renderThreads.remove(m_engine);
    renderThreadsMutex.unlock();

    m_eventLoopQuitHack->deleteLater();
    wait();
}

QQuickContext2DRenderThread *QQuickContext2DRenderThread::instance(QQmlEngine *engine)
{
    QQuickContext2DRenderThread *thread = nullptr;
    renderThreadsMutex.lock();
    if (renderThreads.contains(engine))
        thread = renderThreads.value(engine);
    else {
        thread = new QQuickContext2DRenderThread(engine);
        renderThreads.insert(engine, thread);
    }
    renderThreadsMutex.unlock();
    return thread;
}

class QQuickCanvasItemPrivate : public QQuickItemPrivate
{
public:
    QQuickCanvasItemPrivate();
    ~QQuickCanvasItemPrivate();
    QQuickCanvasContext *context;
    QSizeF canvasSize;
    QSize tileSize;
    QRectF canvasWindow;
    QRectF dirtyRect;
    uint hasCanvasSize :1;
    uint hasTileSize :1;
    uint hasCanvasWindow :1;
    uint available :1;
    QQuickCanvasItem::RenderTarget renderTarget;
    QQuickCanvasItem::RenderStrategy renderStrategy;
    QString contextType;
    QHash<QUrl, QQmlRefPointer<QQuickCanvasPixmap> > pixmaps;
    QUrl baseUrl;
    QMap<int, QV4::PersistentValue> animationCallbacks;
    mutable QQuickCanvasTextureProvider *textureProvider;
    QSGInternalImageNode *node;
    QSGTexture *nodeTexture;
};

QQuickCanvasItemPrivate::QQuickCanvasItemPrivate()
    : QQuickItemPrivate()
    , context(nullptr)
    , canvasSize(1, 1)
    , tileSize(1, 1)
    , hasCanvasSize(false)
    , hasTileSize(false)
    , hasCanvasWindow(false)
    , available(false)
    , renderTarget(QQuickCanvasItem::Image)
    , renderStrategy(QQuickCanvasItem::Immediate)
    , textureProvider(nullptr)
    , node(nullptr)
    , nodeTexture(nullptr)
{
    implicitAntialiasing = true;
}

QQuickCanvasItemPrivate::~QQuickCanvasItemPrivate()
{
    pixmaps.clear();
}


/*!
    \qmltype Canvas
    \instantiates QQuickCanvasItem
    \inqmlmodule QtQuick
    \since 5.0
    \inherits Item
    \ingroup qtquick-canvas
    \ingroup qtquick-visual
    \brief Provides a 2D canvas item enabling drawing via JavaScript.

    The Canvas item allows drawing of straight and curved lines, simple and
    complex shapes, graphs, and referenced graphic images.  It can also add
    text, colors, shadows, gradients, and patterns, and do low level pixel
    operations. The Canvas output may be saved as an image file or serialized
    to a URL.

    Rendering to the Canvas is done using a Context2D object, usually as a
    result of the \l paint signal.

    To define a drawing area in the Canvas item set the \c width and \c height
    properties.  For example, the following code creates a Canvas item which
    has a drawing area with a height of 100 pixels and width of 200 pixels:
    \qml
    import QtQuick 2.0
    Canvas {
        id: mycanvas
        width: 100
        height: 200
        onPaint: {
            var ctx = getContext("2d");
            ctx.fillStyle = Qt.rgba(1, 0, 0, 1);
            ctx.fillRect(0, 0, width, height);
        }
    }
    \endqml

    Currently the Canvas item only supports the two-dimensional rendering context.

    \section1 Threaded Rendering and Render Target

    The Canvas item supports two render targets: \c Canvas.Image and
    \c Canvas.FramebufferObject.

    The \c Canvas.Image render target is a \a QImage object. This render target
    supports background thread rendering, allowing complex or long running
    painting to be executed without blocking the UI. This is the only render
    target that is supported by all Qt Quick backends.

    The Canvas.FramebufferObject render target utilizes OpenGL hardware
    acceleration rather than rendering into system memory, which in many cases
    results in faster rendering. Canvas.FramebufferObject relies on the OpenGL
    extensions \c GL_EXT_framebuffer_multisample and \c GL_EXT_framebuffer_blit
    for antialiasing. It will also use more graphics memory when rendering
    strategy is anything other than Canvas.Cooperative. Framebuffer objects may
    not be available with Qt Quick backends other than OpenGL.

    The default render target is Canvas.Image and the default renderStrategy is
    Canvas.Immediate.

    \section1 Pixel Operations
    All HTML5 2D context pixel operations are supported. In order to ensure
    improved pixel reading/writing performance the \a Canvas.Image render
    target should be chosen. The \a Canvas.FramebufferObject render target
    requires the pixel data to be exchanged between the system memory and the
    graphic card, which is significantly more expensive.  Rendering may also be
    synchronized with the V-sync signal (to avoid
    \l{http://en.wikipedia.org/wiki/Screen_tearing}{screen tearing}) which will further
    impact pixel operations with \c Canvas.FrambufferObject render target.

    \section1 Tips for Porting Existing HTML5 Canvas Applications

    Although the Canvas item provides an HTML5-like API, HTML5 canvas
    applications need to be modified to run in the Canvas item:
    \list
    \li Replace all DOM API calls with QML property bindings or Canvas item methods.
    \li Replace all HTML event handlers with the MouseArea item.
    \li Change setInterval/setTimeout function calls with the \l Timer item or
       the use of requestAnimationFrame().
    \li Place painting code into the \c onPaint handler and trigger
       painting by calling the markDirty() or requestPaint() methods.
    \li To draw images, load them by calling the Canvas's loadImage() method and then request to paint
       them in the \c onImageLoaded handler.
    \endlist

    Starting Qt 5.4, the Canvas is a
    \l{QSGTextureProvider}{texture provider}
    and can be used directly in \l {ShaderEffect}{ShaderEffects} and other
    classes that consume texture providers.

    \note In general large canvases, frequent updates, and animation should be
    avoided with the Canvas.Image render target. This is because with
    accelerated graphics APIs each update will lead to a texture upload. Also,
    if possible, prefer QQuickPaintedItem and implement drawing in C++ via
    QPainter instead of the more expensive and likely less performing
    JavaScript and Context2D approach.

    \sa Context2D QQuickPaintedItem
*/

QQuickCanvasItem::QQuickCanvasItem(QQuickItem *parent)
    : QQuickItem(*(new QQuickCanvasItemPrivate), parent)
{
    setFlag(ItemHasContents);
}

QQuickCanvasItem::~QQuickCanvasItem()
{
    Q_D(QQuickCanvasItem);
    delete d->context;
    if (d->textureProvider)
        QQuickWindowQObjectCleanupJob::schedule(window(), d->textureProvider);
}

/*!
    \qmlproperty bool QtQuick::Canvas::available

    Indicates when Canvas is able to provide a drawing context to operate on.
*/

bool QQuickCanvasItem::isAvailable() const
{
    return d_func()->available;
}

/*!
    \qmlproperty string QtQuick::Canvas::contextType
    The type of drawing context to use.

    This property is set to the name of the active context type.

    If set explicitly the canvas will attempt to create a context of the
    named type after becoming available.

    The type name is the same as used in the getContext() call, for the 2d
    canvas the value will be "2d".

    \sa getContext(), available
*/

QString QQuickCanvasItem::contextType() const
{
    return d_func()->contextType;
}

void QQuickCanvasItem::setContextType(const QString &contextType)
{
    Q_D(QQuickCanvasItem);

    if (contextType.compare(d->contextType, Qt::CaseInsensitive) == 0)
        return;

    if (d->context) {
        qmlWarning(this) << "Canvas already initialized with a different context type";
        return;
    }

    d->contextType = contextType;

    if (d->available)
        createContext(contextType);

    emit contextTypeChanged();
}

/*!
    \qmlproperty object QtQuick::Canvas::context
    Holds the active drawing context.

    If the canvas is ready and there has been a successful call to getContext()
    or the contextType property has been set with a supported context type,
    this property will contain the current drawing context, otherwise null.
*/

QJSValue QQuickCanvasItem::context() const
{
    Q_D(const QQuickCanvasItem);
    return d->context ? QJSValue(d->context->v4Engine(), d->context->v4value()) : QJSValue();
}

/*!
    \qmlproperty size QtQuick::Canvas::canvasSize
    Holds the logical canvas size that the context paints on.

    By default, the canvas size is the same size as the current canvas item
    size.

    By setting the canvasSize, tileSize and canvasWindow, the Canvas item can
    act as a large virtual canvas with many separately rendered tile rectangles.
    Only those tiles within the current canvas window are painted by the Canvas
    render engine.

    \sa tileSize, canvasWindow
*/
QSizeF QQuickCanvasItem::canvasSize() const
{
    Q_D(const QQuickCanvasItem);
    return d->canvasSize;
}

void QQuickCanvasItem::setCanvasSize(const QSizeF & size)
{
    Q_D(QQuickCanvasItem);
    if (d->canvasSize != size) {
        d->hasCanvasSize = true;
        d->canvasSize = size;
        emit canvasSizeChanged();

        if (d->context)
            polish();
    }
}

/*!
    \qmlproperty size QtQuick::Canvas::tileSize
    Holds the canvas rendering tile size.

    The Canvas item enters tiled mode by setting canvasSize, tileSize and the
    canvasWindow. This can improve rendering performance by rendering and
    caching tiles instead of rendering the whole canvas every time.

    Memory will be consumed only by those tiles within the current visible
    region.

    By default the tileSize is the same as the canvasSize.

    \obsolete This feature is incomplete. For details, see QTBUG-33129.

    \sa canvasSize, canvasWindow
*/
QSize QQuickCanvasItem::tileSize() const
{
    Q_D(const QQuickCanvasItem);
    return d->tileSize;
}

void QQuickCanvasItem::setTileSize(const QSize & size)
{
    Q_D(QQuickCanvasItem);
    if (d->tileSize != size) {
        d->hasTileSize = true;
        d->tileSize = size;

        emit tileSizeChanged();

        if (d->context)
            polish();
    }
}

/*!
    \qmlproperty rect QtQuick::Canvas::canvasWindow
     Holds the current canvas visible window.

     By default the canvasWindow size is the same as the Canvas item size with
     the top-left point as (0, 0).

     If the canvasSize is different to the Canvas item size, the Canvas item
     can display different visible areas by changing the canvas windowSize
     and/or position.

    \obsolete This feature is incomplete. For details, see QTBUG-33129

    \sa canvasSize, tileSize
*/
QRectF QQuickCanvasItem::canvasWindow() const
{
    Q_D(const QQuickCanvasItem);
    return d->canvasWindow;
}

void QQuickCanvasItem::setCanvasWindow(const QRectF& rect)
{
    Q_D(QQuickCanvasItem);
    if (d->canvasWindow != rect) {
        d->canvasWindow = rect;

        d->hasCanvasWindow = true;
        emit canvasWindowChanged();

        if (d->context)
            polish();
    }
}

/*!
    \qmlproperty enumeration QtQuick::Canvas::renderTarget
    Holds the current canvas render target.

    \list
    \li Canvas.Image  - render to an in memory image buffer.
    \li Canvas.FramebufferObject - render to an OpenGL frame buffer
    \endlist

    This hint is supplied along with renderStrategy to the graphics context to
    determine the method of rendering. A renderStrategy, renderTarget or a
    combination may not be supported by a graphics context, in which case the
    context will choose appropriate options and Canvas will signal the change
    to the properties.

    The default render target is \c Canvas.Image.
*/
QQuickCanvasItem::RenderTarget QQuickCanvasItem::renderTarget() const
{
    Q_D(const QQuickCanvasItem);
    return d->renderTarget;
}

void QQuickCanvasItem::setRenderTarget(QQuickCanvasItem::RenderTarget target)
{
    Q_D(QQuickCanvasItem);
    if (d->renderTarget != target) {
        if (d->context) {
            qmlWarning(this) << "Canvas:renderTarget not changeble once context is active.";
            return;
        }

        d->renderTarget = target;
        emit renderTargetChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick::Canvas::renderStrategy
    Holds the current canvas rendering strategy.

    \list
    \li Canvas.Immediate - context will perform graphics commands immediately in the main UI thread.
    \li Canvas.Threaded - context will defer graphics commands to a private rendering thread.
    \li Canvas.Cooperative - context will defer graphics commands to the applications global render thread.
    \endlist

    This hint is supplied along with renderTarget to the graphics context to
    determine the method of rendering. A renderStrategy, renderTarget or a
    combination may not be supported by a graphics context, in which case the
    context will choose appropriate options and Canvas will signal the change
    to the properties.

    Configuration or runtime tests may cause the QML Scene Graph to render in
    the GUI thread.  Selecting \c Canvas.Cooperative, does not guarantee
    rendering will occur on a thread separate from the GUI thread.

    The default value is \c Canvas.Immediate.

    \sa renderTarget
*/

QQuickCanvasItem::RenderStrategy QQuickCanvasItem::renderStrategy() const
{
    return d_func()->renderStrategy;
}

void QQuickCanvasItem::setRenderStrategy(QQuickCanvasItem::RenderStrategy strategy)
{
    Q_D(QQuickCanvasItem);
    if (d->renderStrategy != strategy) {
        if (d->context) {
            qmlWarning(this) << "Canvas:renderStrategy not changeable once context is active.";
            return;
        }
        d->renderStrategy = strategy;
        emit renderStrategyChanged();
    }
}

QQuickCanvasContext* QQuickCanvasItem::rawContext() const
{
    return d_func()->context;
}

bool QQuickCanvasItem::isPaintConnected()
{
    IS_SIGNAL_CONNECTED(this, QQuickCanvasItem, paint, (const QRect &));
}

void QQuickCanvasItem::sceneGraphInitialized()
{
    Q_D(QQuickCanvasItem);

    d->available = true;
    connect(this, SIGNAL(visibleChanged()), SLOT(checkAnimationCallbacks()));
    QMetaObject::invokeMethod(this, "availableChanged", Qt::QueuedConnection);

    if (!d->contextType.isNull())
        QMetaObject::invokeMethod(this, "delayedCreate", Qt::QueuedConnection);
    else if (isPaintConnected())
        QMetaObject::invokeMethod(this, "requestPaint", Qt::QueuedConnection);
}

void QQuickCanvasItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickCanvasItem);

    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    // Due to indirect recursion, newGeometry may be outdated
    // after this call, so we use width and height instead.
    QSizeF newSize = QSizeF(width(), height());
    if (!d->hasCanvasSize && d->canvasSize != newSize) {
        d->canvasSize = newSize;
        emit canvasSizeChanged();
    }

    if (!d->hasTileSize && d->tileSize != newSize) {
        d->tileSize = newSize.toSize();
        emit tileSizeChanged();
    }

    const QRectF rect = QRectF(QPointF(0, 0), newSize);

    if (!d->hasCanvasWindow && d->canvasWindow != rect) {
        d->canvasWindow = rect;
        emit canvasWindowChanged();
    }

    if (d->available && newSize != oldGeometry.size()) {
        if (isVisible() || (d->extra.isAllocated() && d->extra->effectRefCount > 0))
            requestPaint();
    }
}

void QQuickCanvasItem::releaseResources()
{
    Q_D(QQuickCanvasItem);

    if (d->context) {
        delete d->context;
        d->context = nullptr;
    }
    d->node = nullptr; // managed by the scene graph, just reset the pointer
    if (d->textureProvider) {
        QQuickWindowQObjectCleanupJob::schedule(window(), d->textureProvider);
        d->textureProvider = nullptr;
    }
    if (d->nodeTexture) {
        QQuickWindowQObjectCleanupJob::schedule(window(), d->nodeTexture);
        d->nodeTexture = nullptr;
    }
}

bool QQuickCanvasItem::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::PolishRequest:
        polish();
        return true;
    default:
        return QQuickItem::event(event);
    }
}

void QQuickCanvasItem::invalidateSceneGraph()
{
    Q_D(QQuickCanvasItem);
    if (d->context)
        d->context->deleteLater();
    d->context = nullptr;
    d->node = nullptr; // managed by the scene graph, just reset the pointer
    delete d->textureProvider;
    d->textureProvider = nullptr;
    delete d->nodeTexture;
    d->nodeTexture = nullptr;
}

void QQuickCanvasItem::schedulePolish()
{
    auto polishRequestEvent = new QEvent(QEvent::PolishRequest);
    QCoreApplication::postEvent(this, polishRequestEvent);
}

void QQuickCanvasItem::componentComplete()
{
    QQuickItem::componentComplete();

    Q_D(QQuickCanvasItem);
    d->baseUrl = qmlEngine(this)->contextForObject(this)->baseUrl();
}

void QQuickCanvasItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    if (change != QQuickItem::ItemSceneChange)
        return;

    Q_D(QQuickCanvasItem);
    if (d->available) {
        if (d->dirtyAttributes & QQuickItemPrivate::ContentUpdateMask)
            requestPaint();
        return;
    }

    if (value.window== nullptr)
        return;

    d->window = value.window;
    QSGRenderContext *context = QQuickWindowPrivate::get(d->window)->context;

    // Rendering to FramebufferObject needs a valid OpenGL context.
    if (context != nullptr && (d->renderTarget != FramebufferObject || context->isValid())) {
        // Defer the call. In some (arguably incorrect) cases we get here due
        // to ItemSceneChange with the user-supplied property values not yet
        // set. Work this around by a deferred invoke. (QTBUG-49692)
        QMetaObject::invokeMethod(this, "sceneGraphInitialized", Qt::QueuedConnection);
    } else {
        connect(d->window, SIGNAL(sceneGraphInitialized()), SLOT(sceneGraphInitialized()));
    }
}

void QQuickCanvasItem::updatePolish()
{
    QQuickItem::updatePolish();

    Q_D(QQuickCanvasItem);
    if (d->context && d->renderStrategy != QQuickCanvasItem::Cooperative)
        d->context->prepare(d->canvasSize.toSize(), d->tileSize, d->canvasWindow.toRect(), d->dirtyRect.toRect(), d->smooth, antialiasing());

    if (d->animationCallbacks.size() > 0 && isVisible()) {
        QMap<int, QV4::PersistentValue> animationCallbacks = d->animationCallbacks;
        d->animationCallbacks.clear();

        QV4::ExecutionEngine *v4 = qmlEngine(this)->handle();
        QV4::Scope scope(v4);
        QV4::ScopedFunctionObject function(scope);
        QV4::JSCallData jsCall(scope, 1);
        *jsCall->thisObject = QV4::QObjectWrapper::wrap(v4, this);

        for (auto it = animationCallbacks.cbegin(), end = animationCallbacks.cend(); it != end; ++it) {
            function = it.value().value();
            jsCall->args[0] = QV4::Value::fromUInt32(QDateTime::currentMSecsSinceEpoch());
            function->call(jsCall);
        }
    }
    else {
        if (d->dirtyRect.isValid()) {
            if (d->hasTileSize && d->hasCanvasWindow)
                emit paint(tiledRect(d->canvasWindow.intersected(d->dirtyRect.toAlignedRect()), d->tileSize));
            else
                emit paint(d->dirtyRect.toRect());
            d->dirtyRect = QRectF();
        }
    }

    if (d->context) {
        if (d->renderStrategy == QQuickCanvasItem::Cooperative)
            update();
        else
            d->context->flush();
    }
}

QSGNode *QQuickCanvasItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QQuickCanvasItem);

    if (!d->context || d->canvasWindow.size().isEmpty()) {
        if (d->textureProvider) {
            d->textureProvider->tex = nullptr;
            d->textureProvider->fireTextureChanged();
        }
        delete oldNode;
        return nullptr;
    }

    QSGInternalImageNode *node = static_cast<QSGInternalImageNode *>(oldNode);
    if (!node) {
        QSGRenderContext *rc = QQuickWindowPrivate::get(window())->context;
        node = rc->sceneGraphContext()->createInternalImageNode(rc);
        d->node = node;
    }


    if (d->smooth)
        node->setFiltering(QSGTexture::Linear);
    else
        node->setFiltering(QSGTexture::Nearest);

    if (d->renderStrategy == QQuickCanvasItem::Cooperative) {
        d->context->prepare(d->canvasSize.toSize(), d->tileSize, d->canvasWindow.toRect(), d->dirtyRect.toRect(), d->smooth, antialiasing());
        d->context->flush();
    }

    QQuickContext2D *ctx = qobject_cast<QQuickContext2D *>(d->context);
    QQuickContext2DTexture *factory = ctx->texture();
    QSGTexture *texture = factory->textureForNextFrame(d->nodeTexture, window());
    if (!texture) {
        delete node;
        d->node = nullptr;
        d->nodeTexture = nullptr;
        if (d->textureProvider) {
            d->textureProvider->tex = nullptr;
            d->textureProvider->fireTextureChanged();
        }
        return nullptr;
    }

    d->nodeTexture = texture;
    node->setTexture(texture);
    node->setTargetRect(QRectF(QPoint(0, 0), d->canvasWindow.size()));
    node->setInnerTargetRect(QRectF(QPoint(0, 0), d->canvasWindow.size()));
    node->update();

    if (d->textureProvider) {
        d->textureProvider->tex = d->nodeTexture;
        d->textureProvider->fireTextureChanged();
    }
    return node;
}

bool QQuickCanvasItem::isTextureProvider() const
{
    return true;
}

QSGTextureProvider *QQuickCanvasItem::textureProvider() const
{
    // When Item::layer::enabled == true, QQuickItem will be a texture
    // provider. In this case we should prefer to return the layer rather
    // than the canvas itself.
    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    Q_D(const QQuickCanvasItem);
#if QT_CONFIG(opengl)
    QQuickWindow *w = window();
    if (!w || !w->isSceneGraphInitialized()
            || QThread::currentThread() != QQuickWindowPrivate::get(w)->context->thread()) {
        qWarning("QQuickCanvasItem::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }
#endif
    if (!d->textureProvider)
        d->textureProvider = new QQuickCanvasTextureProvider;
    d->textureProvider->tex = d->nodeTexture;
    return d->textureProvider;
}

/*!
    \qmlmethod object QtQuick::Canvas::getContext(string contextId, ... args)

    Returns a drawing context, or \c null if no context is available.

    The \a contextId parameter names the required context. The Canvas item
    will return a context that implements the required drawing mode. After the
    first call to getContext, any subsequent call to getContext with the same
    contextId will return the same context object. Any additional arguments
    (\a args) are currently ignored.

    If the context type is not supported or the canvas has previously been
    requested to provide a different and incompatible context type, \c null
    will be returned.

    Canvas only supports a 2d context.

*/

void QQuickCanvasItem::getContext(QQmlV4Function *args)
{
    Q_D(QQuickCanvasItem);

    QV4::Scope scope(args->v4engine());
    QV4::ScopedString str(scope, (*args)[0]);
    if (!str) {
        qmlWarning(this) << "getContext should be called with a string naming the required context type";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    if (!d->available) {
        qmlWarning(this) << "Unable to use getContext() at this time, please wait for available: true";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    QString contextId = str->toQString();

    if (d->context != nullptr) {
        if (d->context->contextNames().contains(contextId, Qt::CaseInsensitive)) {
            args->setReturnValue(d->context->v4value());
            return;
        }

        qmlWarning(this) << "Canvas already initialized with a different context type";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    if (createContext(contextId))
        args->setReturnValue(d->context->v4value());
    else
        args->setReturnValue(QV4::Encode::null());
}

/*!
    \qmlmethod int QtQuick::Canvas::requestAnimationFrame(callback)

    This function schedules \a callback to be invoked before composing the Qt Quick
    scene.
*/

void QQuickCanvasItem::requestAnimationFrame(QQmlV4Function *args)
{
    QV4::Scope scope(args->v4engine());
    QV4::ScopedFunctionObject f(scope, (*args)[0]);
    if (!f) {
        qmlWarning(this) << "requestAnimationFrame should be called with an animation callback function";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    Q_D(QQuickCanvasItem);

    static int id = 0;

    d->animationCallbacks.insert(++id, QV4::PersistentValue(scope.engine, f->asReturnedValue()));

    // QTBUG-55778: Calling polish directly here can lead to a polish loop
    if (isVisible())
        schedulePolish();

    args->setReturnValue(QV4::Encode(id));
}

/*!
    \qmlmethod QtQuick::Canvas::cancelRequestAnimationFrame(int handle)

    This function will cancel the animation callback referenced by \a handle.
*/

void QQuickCanvasItem::cancelRequestAnimationFrame(QQmlV4Function *args)
{
    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[0]);
    if (!v->isInteger()) {
        qmlWarning(this) << "cancelRequestAnimationFrame should be called with an animation callback id";
        args->setReturnValue(QV4::Encode::null());
        return;
    }

    d_func()->animationCallbacks.remove(v->integerValue());
}


/*!
    \qmlmethod QtQuick::Canvas::requestPaint()

    Request the entire visible region be re-drawn.

    \sa markDirty()
*/

void QQuickCanvasItem::requestPaint()
{
    markDirty(d_func()->canvasWindow);
}

/*!
    \qmlmethod QtQuick::Canvas::markDirty(rect area)

    Marks the given \a area as dirty, so that when this area is visible the
    canvas renderer will redraw it. This will trigger the \c paint signal.

    \sa paint, requestPaint()
*/

void QQuickCanvasItem::markDirty(const QRectF& rect)
{
    Q_D(QQuickCanvasItem);
    if (!d->available)
        return;

    d->dirtyRect |= rect;

    polish();
}

void QQuickCanvasItem::checkAnimationCallbacks()
{
    if (d_func()->animationCallbacks.size() > 0 && isVisible())
        polish();
}

/*!
    \qmlmethod bool QtQuick::Canvas::save(string filename)

    Saves the current canvas content into an image file \a filename.
    The saved image format is automatically decided by the \a filename's
    suffix. Returns \c true on success.

    \note Calling this method will force painting the whole canvas, not just the
    current canvas visible window.

    \sa canvasWindow, canvasSize, toDataURL()
*/
bool QQuickCanvasItem::save(const QString &filename) const
{
    Q_D(const QQuickCanvasItem);
    QUrl url = d->baseUrl.resolved(QUrl::fromLocalFile(filename));
    return toImage().save(url.toLocalFile());
}

QQmlRefPointer<QQuickCanvasPixmap> QQuickCanvasItem::loadedPixmap(const QUrl& url)
{
    Q_D(QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    if (!d->pixmaps.contains(fullPathUrl)) {
        loadImage(url);
    }
    return d->pixmaps.value(fullPathUrl);
}

/*!
    \qmlsignal QtQuick::Canvas::imageLoaded()

    This signal is emitted when an image has been loaded.

    \sa loadImage()
*/

/*!
    \qmlmethod QtQuick::Canvas::loadImage(url image)

    Loads the given \a image asynchronously.

    Once the image is ready, imageLoaded() signal will be emitted.
    The loaded image can be unloaded with the unloadImage() method.

    \note Only loaded images can be painted on the Canvas item.

    \sa unloadImage(), imageLoaded(), isImageLoaded(),
        Context2D::createImageData(), Context2D::drawImage()
*/
void QQuickCanvasItem::loadImage(const QUrl& url)
{
    Q_D(QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    if (!d->pixmaps.contains(fullPathUrl)) {
        QQuickPixmap* pix = new QQuickPixmap();
        QQmlRefPointer<QQuickCanvasPixmap> canvasPix;
        canvasPix.adopt(new QQuickCanvasPixmap(pix));
        d->pixmaps.insert(fullPathUrl, canvasPix);

        pix->load(qmlEngine(this)
                , fullPathUrl
                , QQuickPixmap::Cache | QQuickPixmap::Asynchronous);
        if (pix->isLoading())
            pix->connectFinished(this, SIGNAL(imageLoaded()));
    }
}
/*!
    \qmlmethod QtQuick::Canvas::unloadImage(url image)

    Unloads the \a image.

    Once an image is unloaded, it cannot be painted by the canvas context
    unless it is loaded again.

    \sa loadImage(), imageLoaded(), isImageLoaded(),
        Context2D::createImageData(), Context2D::drawImage
*/
void QQuickCanvasItem::unloadImage(const QUrl& url)
{
    Q_D(QQuickCanvasItem);
    d->pixmaps.remove(d->baseUrl.resolved(url));
}

/*!
    \qmlmethod QtQuick::Canvas::isImageError(url image)

    Returns \c true if the \a image failed to load, \c false otherwise.

    \sa loadImage()
*/
bool QQuickCanvasItem::isImageError(const QUrl& url) const
{
    Q_D(const QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->pixmaps.contains(fullPathUrl)
        && d->pixmaps.value(fullPathUrl)->pixmap()->isError();
}

/*!
  \qmlmethod QtQuick::Canvas::isImageLoading(url image)
  Returns true if the \a image is currently loading.

  \sa loadImage()
*/
bool QQuickCanvasItem::isImageLoading(const QUrl& url) const
{
    Q_D(const QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->pixmaps.contains(fullPathUrl)
        && d->pixmaps.value(fullPathUrl)->pixmap()->isLoading();
}
/*!
  \qmlmethod QtQuick::Canvas::isImageLoaded(url image)
  Returns true if the \a image is successfully loaded and ready to use.

  \sa loadImage()
*/
bool QQuickCanvasItem::isImageLoaded(const QUrl& url) const
{
    Q_D(const QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->pixmaps.contains(fullPathUrl)
        && d->pixmaps.value(fullPathUrl)->pixmap()->isReady();
}

QImage QQuickCanvasItem::toImage(const QRectF& rect) const
{
    Q_D(const QQuickCanvasItem);

    if (!d->context)
        return QImage();

    const QRectF &rectSource = rect.isEmpty() ? canvasWindow() : rect;
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : qreal(1);
    const QRectF rectScaled(rectSource.topLeft() * dpr, rectSource.size() * dpr);

    QImage image = d->context->toImage(rectScaled);
    image.setDevicePixelRatio(dpr);
    return image;
}

static const char* mimeToType(const QString &mime)
{
    const QLatin1String imagePrefix("image/");
    if (!mime.startsWith(imagePrefix))
        return nullptr;
    const QStringRef mimeExt = mime.midRef(imagePrefix.size());
    if (mimeExt == QLatin1String("png"))
        return "png";
    else if (mimeExt == QLatin1String("bmp"))
        return "bmp";
    else if (mimeExt == QLatin1String("jpeg"))
        return "jpeg";
    else if (mimeExt == QLatin1String("x-portable-pixmap"))
        return "ppm";
    else if (mimeExt == QLatin1String("tiff"))
        return "tiff";
    else if (mimeExt == QLatin1String("xpm"))
        return "xpm";
    return nullptr;
}

/*!
  \qmlmethod string QtQuick::Canvas::toDataURL(string mimeType)

   Returns a data URL for the image in the canvas.

   The default \a mimeType is "image/png".

   \sa save()
*/
QString QQuickCanvasItem::toDataURL(const QString& mimeType) const
{
    QImage image = toImage();

    if (!image.isNull()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        const QString mime = mimeType.toLower();
        const char* type = mimeToType(mime);
        if (!type)
            return QStringLiteral("data:,");

        image.save(&buffer, type);
        buffer.close();
        return QLatin1String("data:") + mime + QLatin1String(";base64,") + QLatin1String(ba.toBase64().constData());
    }
    return QStringLiteral("data:,");
}

void QQuickCanvasItem::delayedCreate()
{
    Q_D(QQuickCanvasItem);

    if (!d->context && !d->contextType.isNull())
        createContext(d->contextType);

    requestPaint();
}

bool QQuickCanvasItem::createContext(const QString &contextType)
{
    Q_D(QQuickCanvasItem);

    if (!window())
        return false;

    if (contextType == QLatin1String("2d")) {
        if (d->contextType.compare(QLatin1String("2d"), Qt::CaseInsensitive) != 0)  {
            d->contextType = QLatin1String("2d");
            emit contextTypeChanged(); // XXX: can't be in setContextType()
        }
        initializeContext(new QQuickContext2D(this));
        return true;
    }

    return false;
}

void QQuickCanvasItem::initializeContext(QQuickCanvasContext *context, const QVariantMap &args)
{
    Q_D(QQuickCanvasItem);

    d->context = context;
    d->context->init(this, args);
    d->context->setV4Engine(qmlEngine(this)->handle());
    connect(d->context, SIGNAL(textureChanged()), SLOT(update()));
    connect(d->context, SIGNAL(textureChanged()), SIGNAL(painted()));
    emit contextChanged();
}

QRect QQuickCanvasItem::tiledRect(const QRectF &window, const QSize &tileSize)
{
    if (window.isEmpty())
        return QRect();

    const int tw = tileSize.width();
    const int th = tileSize.height();
    const int h1 = window.left() / tw;
    const int v1 = window.top() / th;

    const int htiles = ((window.right() - h1 * tw) + tw - 1)/tw;
    const int vtiles = ((window.bottom() - v1 * th) + th - 1)/th;

    return QRect(h1 * tw, v1 * th, htiles * tw, vtiles * th);
}

/*!
    \qmlsignal QtQuick::Canvas::paint(rect region)

    This signal is emitted when the \a region needs to be rendered. If a context
    is active it can be referenced from the context property.

    This signal can be triggered by markdirty(), requestPaint() or by changing
    the current canvas window.
*/

/*!
    \qmlsignal QtQuick::Canvas::painted()

    This signal is emitted after all context painting commands are executed and
    the Canvas has been rendered.
*/

QT_END_NAMESPACE

#include "moc_qquickcanvasitem_p.cpp"
