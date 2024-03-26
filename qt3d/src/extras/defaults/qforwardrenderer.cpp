// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qforwardrenderer.h"
#include "qforwardrenderer_p.h"

#include <Qt3DCore/qentity.h>
#include <Qt3DRender/qviewport.h>
#include <Qt3DRender/qcameraselector.h>
#include <Qt3DRender/qclearbuffers.h>
#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qfrustumculling.h>
#include <Qt3DRender/qrendersurfaceselector.h>
#include <Qt3DRender/qdebugoverlay.h>

static void initResources()
{
#ifdef QT_STATIC
    Q_INIT_RESOURCE(extras);
#endif
}

QT_BEGIN_NAMESPACE

using namespace Qt3DRender;

namespace Qt3DExtras {

QForwardRendererPrivate::QForwardRendererPrivate()
    : QTechniqueFilterPrivate()
    , m_surfaceSelector(new QRenderSurfaceSelector)
    , m_viewport(new QViewport())
    , m_cameraSelector(new QCameraSelector())
    , m_clearBuffer(new QClearBuffers())
    , m_frustumCulling(new QFrustumCulling())
    , m_debugOverlay(new QDebugOverlay())
{
}

void QForwardRendererPrivate::init()
{
    Q_Q(QForwardRenderer);

    initResources();

    m_debugOverlay->setParent(m_frustumCulling);
    m_debugOverlay->setEnabled(false);
    m_frustumCulling->setParent(m_clearBuffer);
    m_clearBuffer->setParent(m_cameraSelector);
    m_cameraSelector->setParent(m_viewport);
    m_viewport->setParent(m_surfaceSelector);
    m_surfaceSelector->setParent(q);

    m_viewport->setNormalizedRect(QRectF(0.0, 0.0, 1.0, 1.0));
    m_clearBuffer->setClearColor(Qt::white);
    m_clearBuffer->setBuffers(QClearBuffers::ColorDepthBuffer);

    QFilterKey *forwardRenderingStyle = new QFilterKey(q);
    forwardRenderingStyle->setName(QStringLiteral("renderingStyle"));
    forwardRenderingStyle->setValue(QStringLiteral("forward"));
    q->addMatch(forwardRenderingStyle);
}

/*!
    \class Qt3DExtras::QForwardRenderer
    \brief The QForwardRenderer provides a default \l{Qt 3D Render Framegraph}{FrameGraph}
    implementation of a forward renderer.
    \inmodule Qt3DExtras
    \since 5.7
    \inherits Qt3DRender::QTechniqueFilter

    Forward rendering is what OpenGL traditionally uses. It renders directly to the backbuffer
    one object at a time shading each one as it goes.

    QForwardRenderer is a single leaf \l{Qt 3D Render Framegraph}{FrameGraph} tree which contains
    a Qt3DRender::QViewport, a Qt3DRender::QCameraSelector, and a Qt3DRender::QClearBuffers.
    The QForwardRenderer has a default requirement filter key whose name is "renderingStyle" and
    value "forward".
    If you need to filter out your techniques, you should do so based on that filter key.

    By default the viewport occupies the whole screen and the clear color is white.
    Frustum culling is also enabled.
*/
/*!
    \qmltype ForwardRenderer
    \brief The ForwardRenderer provides a default \l{Qt 3D Render Framegraph}{FrameGraph}
    implementation of a forward renderer.
    \since 5.7
    \inqmlmodule Qt3D.Extras
    \instantiates Qt3DExtras::QForwardRenderer

    Forward rendering is what OpenGL traditionally uses. It renders directly to the backbuffer
    one object at a time shading each one as it goes.

    ForwardRenderer is a single leaf \l{Qt 3D Render Framegraph}{FrameGraph} tree which contains
    a Viewport, a CameraSelector, and a ClearBuffers.
    The ForwardRenderer has a default requirement filter key whose name is "renderingStyle" and
    value "forward".
    If you need to filter out your techniques, you should do so based on that filter key.

    By default the viewport occupies the whole screen and the clear color is white.
    Frustum culling is also enabled.
 */

QForwardRenderer::QForwardRenderer(QNode *parent)
    : QTechniqueFilter(*new QForwardRendererPrivate, parent)
{
    Q_D(QForwardRenderer);
    QObject::connect(d->m_clearBuffer, &QClearBuffers::clearColorChanged, this, &QForwardRenderer::clearColorChanged);
    QObject::connect(d->m_clearBuffer, &QClearBuffers::buffersChanged, this, &QForwardRenderer::buffersToClearChanged);
    QObject::connect(d->m_viewport, &QViewport::normalizedRectChanged, this, &QForwardRenderer::viewportRectChanged);
    QObject::connect(d->m_cameraSelector, &QCameraSelector::cameraChanged, this, &QForwardRenderer::cameraChanged);
    QObject::connect(d->m_surfaceSelector, &QRenderSurfaceSelector::surfaceChanged, this, &QForwardRenderer::surfaceChanged);
    QObject::connect(d->m_surfaceSelector, &QRenderSurfaceSelector::externalRenderTargetSizeChanged, this, &QForwardRenderer::externalRenderTargetSizeChanged);
    QObject::connect(d->m_frustumCulling, &QFrustumCulling::enabledChanged, this, &QForwardRenderer::frustumCullingEnabledChanged);
    QObject::connect(d->m_viewport, &QViewport::gammaChanged, this, &QForwardRenderer::gammaChanged);
    QObject::connect(d->m_debugOverlay, &QDebugOverlay::enabledChanged, this, &QForwardRenderer::showDebugOverlayChanged);
    d->init();
}

QForwardRenderer::~QForwardRenderer()
{
}

void QForwardRenderer::setViewportRect(const QRectF &viewportRect)
{
    Q_D(QForwardRenderer);
    d->m_viewport->setNormalizedRect(viewportRect);
}

void QForwardRenderer::setClearColor(const QColor &clearColor)
{
    Q_D(QForwardRenderer);
    d->m_clearBuffer->setClearColor(clearColor);
}

void QForwardRenderer::setBuffersToClear(QClearBuffers::BufferType buffers)
{
    Q_D(QForwardRenderer);
    d->m_clearBuffer->setBuffers(buffers);
}

void QForwardRenderer::setCamera(Qt3DCore::QEntity *camera)
{
    Q_D(QForwardRenderer);
    d->m_cameraSelector->setCamera(camera);
}

void QForwardRenderer::setSurface(QObject *surface)
{
    Q_D(QForwardRenderer);
    d->m_surfaceSelector->setSurface(surface);
}

void QForwardRenderer::setExternalRenderTargetSize(const QSize &size)
{
    Q_D(QForwardRenderer);
    d->m_surfaceSelector->setExternalRenderTargetSize(size);
}

void QForwardRenderer::setFrustumCullingEnabled(bool enabled)
{
    Q_D(QForwardRenderer);
    d->m_frustumCulling->setEnabled(enabled);
}

void QForwardRenderer::setGamma(float gamma)
{
    Q_D(QForwardRenderer);
    d->m_viewport->setGamma(gamma);
}

void QForwardRenderer::setShowDebugOverlay(bool showDebugOverlay)
{
    Q_D(QForwardRenderer);
    d->m_debugOverlay->setEnabled(showDebugOverlay);
}

/*!
    \qmlproperty rect ForwardRenderer::viewportRect

    Holds the current normalized viewport rectangle.
*/
/*!
    \property QForwardRenderer::viewportRect

    Holds the current normalized viewport rectangle.
*/
QRectF QForwardRenderer::viewportRect() const
{
    Q_D(const QForwardRenderer);
    return d->m_viewport->normalizedRect();
}

/*!
    \qmlproperty color ForwardRenderer::clearColor

    Holds the current clear color of the scene. The frame buffer is initialized to the clear color
    before rendering.
*/
/*!
    \property QForwardRenderer::clearColor

    Holds the current clear color of the scene. The frame buffer is initialized to the clear color
    before rendering.
*/
QColor QForwardRenderer::clearColor() const
{
    Q_D(const QForwardRenderer);
    return d->m_clearBuffer->clearColor();
}

/*!
    \qmlproperty color ForwardRenderer::buffersToClear

    Holds the current buffers to be cleared. Default value is ColorDepthBuffer
    \since 5.14
*/
/*!
    \property QForwardRenderer::buffersToClear

    Holds the current buffers to be cleared. Default value is ColorDepthBuffer
    \since 5.14
*/
QClearBuffers::BufferType QForwardRenderer::buffersToClear() const
{
    Q_D(const QForwardRenderer);
    return d->m_clearBuffer->buffers();
}

/*!
    \qmlproperty Entity ForwardRenderer::camera

    Holds the current camera entity used to render the scene.

    \note A camera is an Entity that has a CameraLens as one of its components.
*/
/*!
    \property QForwardRenderer::camera

    Holds the current camera entity used to render the scene.

    \note A camera is a QEntity that has a QCameraLens as one of its components.
*/
Qt3DCore::QEntity *QForwardRenderer::camera() const
{
    Q_D(const QForwardRenderer);
    return d->m_cameraSelector->camera();
}

/*!
    \qmlproperty QtObject ForwardRenderer::window

    Holds the current render surface.

    \deprecated
*/
/*!
    \property QForwardRenderer::window

    Holds the current render surface.

    \deprecated
*/

/*!
    \qmlproperty QtObject ForwardRenderer::surface

    Holds the current render surface.
*/
/*!
    \property QForwardRenderer::surface

    Holds the current render surface.
*/
QObject *QForwardRenderer::surface() const
{
    Q_D(const QForwardRenderer);
    return d->m_surfaceSelector->surface();
}

/*!
    \qmlproperty QSize ForwardRenderer::externalRenderTargetSize

    Contains the size of the external render target. External render
    targets are relevant when rendering does not target a window
    surface (as set in \l {surface}).
*/
/*!
    \property QForwardRenderer::externalRenderTargetSize

    Contains the size of the external render target. External render
    targets are relevant when rendering does not target a window
    surface (as set in \l {surface}).
*/
QSize QForwardRenderer::externalRenderTargetSize() const
{
    Q_D(const QForwardRenderer);
    return d->m_surfaceSelector->externalRenderTargetSize();
}

/*!
    \qmlproperty bool ForwardRenderer::frustumCulling

    Indicates if the renderer applies frustum culling to the scene.
*/
/*!
    \property QForwardRenderer::frustumCulling

    Indicates if the renderer applies frustum culling to the scene.
*/
bool QForwardRenderer::isFrustumCullingEnabled() const
{
    Q_D(const QForwardRenderer);
    return d->m_frustumCulling->isEnabled();
}

/*!
    \qmlproperty real ForwardRenderer::gamma

    Holds the gamma value the renderer applies to the scene.
*/
/*!
    \property QForwardRenderer::gamma

    Holds the gamma value the renderer applies to the scene.
*/
float QForwardRenderer::gamma() const
{
    Q_D(const QForwardRenderer);
    return d->m_viewport->gamma();
}

/*!
    \qmlproperty bool ForwardRenderer::showDebugOverlay

    If true, a debug overlay will be rendered over the scene. It will show
    detailed information about the runtime rendering state, let the user
    turn logging on and off, etc.

    \since 5.15
*/
/*!
    \property QForwardRenderer::showDebugOverlay

    If true, a debug overlay will be rendered over the scene. It will show
    detailed information about the runtime rendering state, let the user
    turn logging on and off, etc.

    \since 5.15
*/
bool QForwardRenderer::showDebugOverlay() const
{
    Q_D(const QForwardRenderer);
    return d->m_debugOverlay->isEnabled();
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qforwardrenderer.cpp"
