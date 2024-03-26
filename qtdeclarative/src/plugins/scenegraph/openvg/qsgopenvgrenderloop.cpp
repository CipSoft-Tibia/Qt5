// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgrenderloop_p.h"
#include "qsgopenvgcontext_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>

#include <private/qquickanimatorcontroller_p.h>
#include <private/qquickwindow_p.h>
#include <private/qquickprofiler_p.h>

#include <qtquick_tracepoints_p.h>

#include "qopenvgcontext_p.h"

QT_BEGIN_NAMESPACE

QSGOpenVGRenderLoop::QSGOpenVGRenderLoop()
    : vg(nullptr)
{
    sg = QSGContext::createDefaultContext();
    rc = sg->createRenderContext();
}

QSGOpenVGRenderLoop::~QSGOpenVGRenderLoop()
{
    delete rc;
    delete sg;
}

void QSGOpenVGRenderLoop::show(QQuickWindow *window)
{
    WindowData data;
    data.updatePending = false;
    data.grabOnly = false;
    m_windows[window] = data;

    maybeUpdate(window);
}

void QSGOpenVGRenderLoop::hide(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->fireAboutToStop();
}

void QSGOpenVGRenderLoop::windowDestroyed(QQuickWindow *window)
{
    m_windows.remove(window);
    hide(window);

    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
    d->cleanupNodesOnShutdown();

    if (m_windows.size() == 0) {
        rc->invalidate();
        delete vg;
        vg = nullptr;
    } else if (vg && window == vg->window()) {
        vg->doneCurrent();
    }

    d->animationController.reset();
}

void QSGOpenVGRenderLoop::exposureChanged(QQuickWindow *window)
{
    if (window->isExposed()) {
        m_windows[window].updatePending = true;
        renderWindow(window);
    }
}

QImage QSGOpenVGRenderLoop::grab(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return QImage();

    m_windows[window].grabOnly = true;

    renderWindow(window);

    QImage grabbed = grabContent;
    grabContent = QImage();
    return grabbed;
}

void QSGOpenVGRenderLoop::update(QQuickWindow *window)
{
    maybeUpdate(window);
}

void QSGOpenVGRenderLoop::handleUpdateRequest(QQuickWindow *window)
{
    renderWindow(window);
}

void QSGOpenVGRenderLoop::maybeUpdate(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows[window].updatePending = true;
    window->requestUpdate();
}

QAnimationDriver *QSGOpenVGRenderLoop::animationDriver() const
{
    return nullptr;
}

QSGContext *QSGOpenVGRenderLoop::sceneGraphContext() const
{
    return sg;
}

QSGRenderContext *QSGOpenVGRenderLoop::createRenderContext(QSGContext *) const
{
    return rc;
}

void QSGOpenVGRenderLoop::releaseResources(QQuickWindow *window)
{
    Q_UNUSED(window);
}

QSurface::SurfaceType QSGOpenVGRenderLoop::windowSurfaceType() const
{
    return QSurface::OpenVGSurface;
}

void QSGOpenVGRenderLoop::renderWindow(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!cd->isRenderable() || !m_windows.contains(window))
        return;

    Q_TRACE_SCOPE(QSG_renderWindow);

    WindowData &data = const_cast<WindowData &>(m_windows[window]);

    if (vg == nullptr) {
        vg = new QOpenVGContext(window);
        vg->makeCurrent();
        QSGOpenVGRenderContext::InitParams params;
        params.context = vg;
        cd->context->initialize(&params);
    } else {
        vg->makeCurrent();
    }

    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    if (!data.grabOnly) {
        cd->deliveryAgentPrivate()->flushFrameSynchronousEvents(window);
        // Event delivery/processing triggered the window to be deleted or stop rendering.
        if (!m_windows.contains(window))
            return;
    }
    QElapsedTimer renderTimer;
    qint64 renderTime = 0, syncTime = 0, polishTime = 0;
    bool profileFrames = QSG_OPENVG_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames)
        renderTimer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphPolishFrame);
    Q_TRACE(QSG_polishItems_entry);

    cd->polishItems();

    if (profileFrames)
        polishTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_polishItems_exit);
    Q_QUICK_SG_PROFILE_SWITCH(QQuickProfiler::SceneGraphPolishFrame,
                              QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphPolishPolish);
    Q_TRACE(QSG_sync_entry);

    emit window->afterAnimating();

    cd->syncSceneGraph();
    rc->endSync();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_sync_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopSync);
    Q_TRACE(QSG_render_entry);

    // setup coordinate system for window
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity();
    vgTranslate(0.0f, window->size().height());
    vgScale(1.0, -1.0);

    cd->renderSceneGraph();

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_render_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopRender);
    Q_TRACE(QSG_swap_entry);

    if (data.grabOnly) {
        grabContent = vg->readFramebuffer(window->size() * window->effectiveDevicePixelRatio());
        data.grabOnly = false;
    }

    if (alsoSwap && window->isVisible()) {
        vg->swapBuffers();
        cd->fireFrameSwapped();
    }

    qint64 swapTime = 0;
    if (profileFrames)
        swapTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_swap_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRenderLoopFrame,
                           QQuickProfiler::SceneGraphRenderLoopSwap);

    if (QSG_OPENVG_LOG_TIME_RENDERLOOP().isDebugEnabled()) {
        static QTime lastFrameTime = QTime::currentTime();
        qCDebug(QSG_OPENVG_LOG_TIME_RENDERLOOP,
                "Frame rendered with 'basic' renderloop in %dms, polish=%d, sync=%d, render=%d, swap=%d, frameDelta=%d",
                int(swapTime / 1000000),
                int(polishTime / 1000000),
                int((syncTime - polishTime) / 1000000),
                int((renderTime - syncTime) / 1000000),
                int((swapTime - renderTime) / 10000000),
                int(lastFrameTime.msecsTo(QTime::currentTime())));
        lastFrameTime = QTime::currentTime();
    }

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

QT_END_NAMESPACE
