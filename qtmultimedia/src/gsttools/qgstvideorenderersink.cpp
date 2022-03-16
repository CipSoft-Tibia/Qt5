/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include <qabstractvideosurface.h>
#include <qvideoframe.h>
#include <QDebug>
#include <QMap>
#include <QThread>
#include <QEvent>
#include <QCoreApplication>

#include <private/qmediapluginloader_p.h>
#include "qgstvideobuffer_p.h"

#include "qgstvideorenderersink_p.h"

#include <gst/video/video.h>

#include "qgstutils_p.h"

//#define DEBUG_VIDEO_SURFACE_SINK

QT_BEGIN_NAMESPACE

QGstDefaultVideoRenderer::QGstDefaultVideoRenderer()
    : m_flushed(true)
{
}

QGstDefaultVideoRenderer::~QGstDefaultVideoRenderer()
{
}

GstCaps *QGstDefaultVideoRenderer::getCaps(QAbstractVideoSurface *surface)
{
    return QGstUtils::capsForFormats(surface->supportedPixelFormats());
}

bool QGstDefaultVideoRenderer::start(QAbstractVideoSurface *surface, GstCaps *caps)
{
    m_flushed = true;
    m_format = QGstUtils::formatForCaps(caps, &m_videoInfo);

    return m_format.isValid() && surface->start(m_format);
}

void QGstDefaultVideoRenderer::stop(QAbstractVideoSurface *surface)
{
    m_flushed = true;
    if (surface)
        surface->stop();
}

bool QGstDefaultVideoRenderer::present(QAbstractVideoSurface *surface, GstBuffer *buffer)
{
    m_flushed = false;
    QVideoFrame frame(
                new QGstVideoBuffer(buffer, m_videoInfo),
                m_format.frameSize(),
                m_format.pixelFormat());
    QGstUtils::setFrameTimeStamps(&frame, buffer);

    return surface->present(frame);
}

void QGstDefaultVideoRenderer::flush(QAbstractVideoSurface *surface)
{
    if (surface && !m_flushed)
        surface->present(QVideoFrame());
    m_flushed = true;
}

bool QGstDefaultVideoRenderer::proposeAllocation(GstQuery *)
{
    return true;
}

Q_GLOBAL_STATIC_WITH_ARGS(QMediaPluginLoader, rendererLoader,
        (QGstVideoRendererInterface_iid, QLatin1String("video/gstvideorenderer"), Qt::CaseInsensitive))

QVideoSurfaceGstDelegate::QVideoSurfaceGstDelegate(QAbstractVideoSurface *surface)
    : m_surface(surface)
    , m_renderer(0)
    , m_activeRenderer(0)
    , m_surfaceCaps(0)
    , m_startCaps(0)
    , m_renderBuffer(0)
    , m_notified(false)
    , m_stop(false)
    , m_flush(false)
{
    const auto instances = rendererLoader()->instances(QGstVideoRendererPluginKey);
    for (QObject *instance : instances) {
        QGstVideoRendererInterface* plugin = qobject_cast<QGstVideoRendererInterface*>(instance);
        if (QGstVideoRenderer *renderer = plugin ? plugin->createRenderer() : 0)
            m_renderers.append(renderer);
    }

    m_renderers.append(new QGstDefaultVideoRenderer);
    updateSupportedFormats();
    connect(m_surface, SIGNAL(supportedFormatsChanged()), this, SLOT(updateSupportedFormats()));
}

QVideoSurfaceGstDelegate::~QVideoSurfaceGstDelegate()
{
    qDeleteAll(m_renderers);

    if (m_surfaceCaps)
        gst_caps_unref(m_surfaceCaps);
    if (m_startCaps)
        gst_caps_unref(m_startCaps);
}

GstCaps *QVideoSurfaceGstDelegate::caps()
{
    QMutexLocker locker(&m_mutex);

    gst_caps_ref(m_surfaceCaps);

    return m_surfaceCaps;
}

bool QVideoSurfaceGstDelegate::start(GstCaps *caps)
{
    QMutexLocker locker(&m_mutex);

    if (m_activeRenderer) {
        m_flush = true;
        m_stop = true;
    }

    if (m_startCaps)
        gst_caps_unref(m_startCaps);
    m_startCaps = caps;
    gst_caps_ref(m_startCaps);

    /*
    Waiting for start() to be invoked in the main thread may block
    if gstreamer blocks the main thread until this call is finished.
    This situation is rare and usually caused by setState(Null)
    while pipeline is being prerolled.

    The proper solution to this involves controlling gstreamer pipeline from
    other thread than video surface.

    Currently start() fails if wait() timed out.
    */
    if (!waitForAsyncEvent(&locker, &m_setupCondition, 1000) && m_startCaps) {
        qWarning() << "Failed to start video surface due to main thread blocked.";
        gst_caps_unref(m_startCaps);
        m_startCaps = 0;
    }

    return m_activeRenderer != 0;
}

void QVideoSurfaceGstDelegate::stop()
{
    QMutexLocker locker(&m_mutex);

    if (!m_activeRenderer)
        return;

    m_flush = true;
    m_stop = true;

    if (m_startCaps) {
        gst_caps_unref(m_startCaps);
        m_startCaps = 0;
    }

    waitForAsyncEvent(&locker, &m_setupCondition, 500);
}

void QVideoSurfaceGstDelegate::unlock()
{
    QMutexLocker locker(&m_mutex);

    m_setupCondition.wakeAll();
    m_renderCondition.wakeAll();
}

bool QVideoSurfaceGstDelegate::proposeAllocation(GstQuery *query)
{
    QMutexLocker locker(&m_mutex);

    if (QGstVideoRenderer *pool = m_activeRenderer) {
        locker.unlock();

        return pool->proposeAllocation(query);
    } else {
        return false;
    }
}

void QVideoSurfaceGstDelegate::flush()
{
    QMutexLocker locker(&m_mutex);

    m_flush = true;
    m_renderBuffer = 0;
    m_renderCondition.wakeAll();

    notify();
}

GstFlowReturn QVideoSurfaceGstDelegate::render(GstBuffer *buffer)
{
    QMutexLocker locker(&m_mutex);

    m_renderReturn = GST_FLOW_OK;
    m_renderBuffer = buffer;

    waitForAsyncEvent(&locker, &m_renderCondition, 300);

    m_renderBuffer = 0;

    return m_renderReturn;
}

bool QVideoSurfaceGstDelegate::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        QMutexLocker locker(&m_mutex);

        if (m_notified) {
            while (handleEvent(&locker)) {}
            m_notified = false;
        }
        return true;
    } else {
        return QObject::event(event);
    }
}

bool QVideoSurfaceGstDelegate::handleEvent(QMutexLocker *locker)
{
    if (m_flush) {
        m_flush = false;
        if (m_activeRenderer) {
            locker->unlock();

            m_activeRenderer->flush(m_surface);
        }
    } else if (m_stop) {
        m_stop = false;

        if (QGstVideoRenderer * const activePool = m_activeRenderer) {
            m_activeRenderer = 0;
            locker->unlock();

            activePool->stop(m_surface);

            locker->relock();
        }
    } else if (m_startCaps) {
        Q_ASSERT(!m_activeRenderer);

        GstCaps * const startCaps = m_startCaps;
        m_startCaps = 0;

        if (m_renderer && m_surface) {
            locker->unlock();

            const bool started = m_renderer->start(m_surface, startCaps);

            locker->relock();

            m_activeRenderer = started
                    ? m_renderer
                    : 0;
        } else if (QGstVideoRenderer * const activePool = m_activeRenderer) {
            m_activeRenderer = 0;
            locker->unlock();

            activePool->stop(m_surface);

            locker->relock();
        }

        gst_caps_unref(startCaps);
    } else if (m_renderBuffer) {
        GstBuffer *buffer = m_renderBuffer;
        m_renderBuffer = 0;
        m_renderReturn = GST_FLOW_ERROR;

        if (m_activeRenderer && m_surface) {
            gst_buffer_ref(buffer);

            locker->unlock();

            const bool rendered = m_activeRenderer->present(m_surface, buffer);

            gst_buffer_unref(buffer);

            locker->relock();

            if (rendered)
                m_renderReturn = GST_FLOW_OK;
        }

        m_renderCondition.wakeAll();
    } else {
        m_setupCondition.wakeAll();

        return false;
    }
    return true;
}

void QVideoSurfaceGstDelegate::notify()
{
    if (!m_notified) {
        m_notified = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool QVideoSurfaceGstDelegate::waitForAsyncEvent(
        QMutexLocker *locker, QWaitCondition *condition, unsigned long time)
{
    if (QThread::currentThread() == thread()) {
        while (handleEvent(locker)) {}
        m_notified = false;

        return true;
    } else {
        notify();

        return condition->wait(&m_mutex, time);
    }
}

void QVideoSurfaceGstDelegate::updateSupportedFormats()
{
    if (m_surfaceCaps) {
        gst_caps_unref(m_surfaceCaps);
        m_surfaceCaps = 0;
    }

    for (QGstVideoRenderer *pool : qAsConst(m_renderers)) {
        if (GstCaps *caps = pool->getCaps(m_surface)) {
            if (gst_caps_is_empty(caps)) {
                gst_caps_unref(caps);
                continue;
            }

            if (m_surfaceCaps)
                gst_caps_unref(m_surfaceCaps);

            m_renderer = pool;
            m_surfaceCaps = caps;
            break;
        } else {
            gst_caps_unref(caps);
        }
    }
}

static GstVideoSinkClass *sink_parent_class;
static QAbstractVideoSurface *current_surface;

#define VO_SINK(s) QGstVideoRendererSink *sink(reinterpret_cast<QGstVideoRendererSink *>(s))

QGstVideoRendererSink *QGstVideoRendererSink::createSink(QAbstractVideoSurface *surface)
{
    setSurface(surface);
    QGstVideoRendererSink *sink = reinterpret_cast<QGstVideoRendererSink *>(
            g_object_new(QGstVideoRendererSink::get_type(), 0));

    g_signal_connect(G_OBJECT(sink), "notify::show-preroll-frame", G_CALLBACK(handleShowPrerollChange), sink);

    return sink;
}

void QGstVideoRendererSink::setSurface(QAbstractVideoSurface *surface)
{
    current_surface = surface;
    get_type();
}

GType QGstVideoRendererSink::get_type()
{
    static GType type = 0;

    if (type == 0) {
        static const GTypeInfo info =
        {
            sizeof(QGstVideoRendererSinkClass),                    // class_size
            base_init,                                         // base_init
            NULL,                                              // base_finalize
            class_init,                                        // class_init
            NULL,                                              // class_finalize
            NULL,                                              // class_data
            sizeof(QGstVideoRendererSink),                         // instance_size
            0,                                                 // n_preallocs
            instance_init,                                     // instance_init
            0                                                  // value_table
        };

        type = g_type_register_static(
                GST_TYPE_VIDEO_SINK, "QGstVideoRendererSink", &info, GTypeFlags(0));

        // Register the sink type to be used in custom piplines.
        // When surface is ready the sink can be used.
        gst_element_register(nullptr, "qtvideosink", GST_RANK_PRIMARY, type);
    }

    return type;
}

void QGstVideoRendererSink::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    sink_parent_class = reinterpret_cast<GstVideoSinkClass *>(g_type_class_peek_parent(g_class));

    GstVideoSinkClass *video_sink_class = reinterpret_cast<GstVideoSinkClass *>(g_class);
    video_sink_class->show_frame = QGstVideoRendererSink::show_frame;

    GstBaseSinkClass *base_sink_class = reinterpret_cast<GstBaseSinkClass *>(g_class);
    base_sink_class->get_caps = QGstVideoRendererSink::get_caps;
    base_sink_class->set_caps = QGstVideoRendererSink::set_caps;
    base_sink_class->propose_allocation = QGstVideoRendererSink::propose_allocation;
    base_sink_class->stop = QGstVideoRendererSink::stop;
    base_sink_class->unlock = QGstVideoRendererSink::unlock;

    GstElementClass *element_class = reinterpret_cast<GstElementClass *>(g_class);
    element_class->change_state = QGstVideoRendererSink::change_state;
    gst_element_class_set_metadata(element_class,
        "Qt built-in video renderer sink",
        "Sink/Video",
        "Qt default built-in video renderer sink",
        "The Qt Company");

    GObjectClass *object_class = reinterpret_cast<GObjectClass *>(g_class);
    object_class->finalize = QGstVideoRendererSink::finalize;
}

void QGstVideoRendererSink::base_init(gpointer g_class)
{
    static GstStaticPadTemplate sink_pad_template = GST_STATIC_PAD_TEMPLATE(
            "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(
                    "video/x-raw, "
                    "framerate = (fraction) [ 0, MAX ], "
                    "width = (int) [ 1, MAX ], "
                    "height = (int) [ 1, MAX ]"));

    gst_element_class_add_pad_template(
            GST_ELEMENT_CLASS(g_class), gst_static_pad_template_get(&sink_pad_template));
}

struct NullSurface : QAbstractVideoSurface
{
    NullSurface(QObject *parent = nullptr) : QAbstractVideoSurface(parent) { }

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const override
    {
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
    }

    bool present(const QVideoFrame &) override
    {
        return true;
    }
};

void QGstVideoRendererSink::instance_init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(g_class);
    VO_SINK(instance);

    if (!current_surface) {
        qWarning() << "Using qtvideosink element without video surface";
        static NullSurface nullSurface;
        current_surface = &nullSurface;
    }

    sink->delegate = new QVideoSurfaceGstDelegate(current_surface);
    sink->delegate->moveToThread(current_surface->thread());
    current_surface = nullptr;
}

void QGstVideoRendererSink::finalize(GObject *object)
{
    VO_SINK(object);

    delete sink->delegate;

    // Chain up
    G_OBJECT_CLASS(sink_parent_class)->finalize(object);
}

void QGstVideoRendererSink::handleShowPrerollChange(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(o);
    Q_UNUSED(p);
    QGstVideoRendererSink *sink = reinterpret_cast<QGstVideoRendererSink *>(d);

    gboolean showPrerollFrame = true; // "show-preroll-frame" property is true by default
    g_object_get(G_OBJECT(sink), "show-preroll-frame", &showPrerollFrame, NULL);

    if (!showPrerollFrame) {
        GstState state = GST_STATE_VOID_PENDING;
        GstClockTime timeout = 10000000; // 10 ms
        gst_element_get_state(GST_ELEMENT(sink), &state, NULL, timeout);
        // show-preroll-frame being set to 'false' while in GST_STATE_PAUSED means
        // the QMediaPlayer was stopped from the paused state.
        // We need to flush the current frame.
        if (state == GST_STATE_PAUSED)
            sink->delegate->flush();
    }
}

GstStateChangeReturn QGstVideoRendererSink::change_state(
        GstElement *element, GstStateChange transition)
{
    QGstVideoRendererSink *sink = reinterpret_cast<QGstVideoRendererSink *>(element);

    gboolean showPrerollFrame = true; // "show-preroll-frame" property is true by default
    g_object_get(G_OBJECT(element), "show-preroll-frame", &showPrerollFrame, NULL);

    // If show-preroll-frame is 'false' when transitioning from GST_STATE_PLAYING to
    // GST_STATE_PAUSED, it means the QMediaPlayer was stopped.
    // We need to flush the current frame.
    if (transition == GST_STATE_CHANGE_PLAYING_TO_PAUSED && !showPrerollFrame)
        sink->delegate->flush();

    return GST_ELEMENT_CLASS(sink_parent_class)->change_state(element, transition);
}

GstCaps *QGstVideoRendererSink::get_caps(GstBaseSink *base, GstCaps *filter)
{
    VO_SINK(base);

    GstCaps *caps = sink->delegate->caps();
    GstCaps *unfiltered = caps;
    if (filter) {
        caps = gst_caps_intersect(unfiltered, filter);
        gst_caps_unref(unfiltered);
    }

    return caps;
}

gboolean QGstVideoRendererSink::set_caps(GstBaseSink *base, GstCaps *caps)
{
    VO_SINK(base);

#ifdef DEBUG_VIDEO_SURFACE_SINK
    qDebug() << "set_caps:";
    qDebug() << caps;
#endif

    if (!caps) {
        sink->delegate->stop();

        return TRUE;
    } else if (sink->delegate->start(caps)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean QGstVideoRendererSink::propose_allocation(GstBaseSink *base, GstQuery *query)
{
    VO_SINK(base);
    return sink->delegate->proposeAllocation(query);
}

gboolean QGstVideoRendererSink::stop(GstBaseSink *base)
{
    VO_SINK(base);
    sink->delegate->stop();
    return TRUE;
}

gboolean QGstVideoRendererSink::unlock(GstBaseSink *base)
{
    VO_SINK(base);
    sink->delegate->unlock();
    return TRUE;
}

GstFlowReturn QGstVideoRendererSink::show_frame(GstVideoSink *base, GstBuffer *buffer)
{
    VO_SINK(base);
    return sink->delegate->render(buffer);
}

QT_END_NAMESPACE
