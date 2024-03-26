// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mockclient.h"
#include "mockseat.h"

#include <QElapsedTimer>
#include <QSocketNotifier>

#include <private/qguiapplication_p.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

const struct wl_registry_listener MockClient::registryListener = {
    MockClient::handleGlobal,
    MockClient::handleGlobalRemove
};

MockClient::MockClient()
    : display(wl_display_connect("wayland-qt-test-0"))
{
    if (!display)
        qFatal("MockClient(): wl_display_connect() failed");

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, this);

    fd = wl_display_get_fd(display);

    QSocketNotifier *readNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(readNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(readEvents()));

    QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::eventDispatcher;
    connect(dispatcher, SIGNAL(awake()), this, SLOT(flushDisplay()));

    QElapsedTimer timeout;
    timeout.start();
    do {
        QCoreApplication::processEvents();
    } while (!(compositor && !m_outputs.isEmpty()) && timeout.elapsed() < 1000);

    if (!compositor || m_outputs.empty())
        qFatal("MockClient(): failed to receive globals from display");
}

const wl_output_listener MockClient::outputListener = {
    MockClient::outputGeometryEvent,
    MockClient::outputModeEvent,
    MockClient::outputDone,
    MockClient::outputScale,
    MockClient::outputName,
    MockClient::outputDesc
};

MockClient::~MockClient()
{
    wl_display_disconnect(display);
}

void MockClient::outputGeometryEvent(void *data, wl_output *,
                                     int32_t x, int32_t y,
                                     int32_t width, int32_t height,
                                     int, const char *, const char *,
                                     int32_t )
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    resolve(data)->geometry.moveTopLeft(QPoint(x, y));
}

void MockClient::outputModeEvent(void *data, wl_output *, uint32_t flags,
                                 int w, int h, int refreshRate)
{
    QWaylandOutputMode mode(QSize(w, h), refreshRate);

    if (flags & WL_OUTPUT_MODE_CURRENT) {
        resolve(data)->geometry.setSize(QSize(w, h));
        resolve(data)->resolution = QSize(w, h);
        resolve(data)->refreshRate = refreshRate;
        resolve(data)->currentMode = mode;
    }

    if (flags & WL_OUTPUT_MODE_PREFERRED)
        resolve(data)->preferredMode = mode;

    resolve(data)->modes.append(mode);
}

void MockClient::outputDone(void *, wl_output *)
{

}

void MockClient::outputScale(void *, wl_output *, int)
{

}

void MockClient::outputName(void *, wl_output *, const char *)
{

}

void MockClient::outputDesc(void *, wl_output *, const char *)
{

}

void MockClient::readEvents()
{
    if (error)
        return;
    wl_display_dispatch(display);
}

void MockClient::flushDisplay()
{
    if (error)
        return;

    if (wl_display_prepare_read(display) == 0) {
        wl_display_read_events(display);
    }

    if (wl_display_dispatch_pending(display) < 0) {
        error = wl_display_get_error(display);
        if (error == EPROTO) {
            protocolError.code = wl_display_get_protocol_error(display, &protocolError.interface, &protocolError.id);
            return;
        }
    }

    wl_display_flush(display);
}

void MockClient::handleGlobal(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Q_UNUSED(registry);
    Q_UNUSED(version);
    resolve(data)->handleGlobal(id, QByteArray(interface));
}

void MockClient::handleGlobalRemove(void *data, wl_registry *wl_registry, uint32_t id)
{
    Q_UNUSED(wl_registry);
    resolve(data)->handleGlobalRemove(id);
}

void MockClient::handleGlobal(uint32_t id, const QByteArray &interface)
{
    if (interface == "wl_compositor") {
        compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, id, &wl_compositor_interface, 4));
    } else if (interface == "wl_output") {
        auto output = static_cast<wl_output *>(wl_registry_bind(registry, id, &wl_output_interface, 2));
        m_outputs.insert(id, output);
        wl_output_add_listener(output, &outputListener, this);
    } else if (interface == "wl_shm") {
        shm = static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
    } else if (interface == "wp_viewporter") {
        viewporter = static_cast<wp_viewporter *>(wl_registry_bind(registry, id, &wp_viewporter_interface, 1));
    } else if (interface == "wl_shell") {
        wlshell = static_cast<wl_shell *>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
    } else if (interface == "xdg_wm_base") {
        xdgWmBase = static_cast<xdg_wm_base *>(wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
    } else if (interface == "ivi_application") {
        iviApplication = static_cast<ivi_application *>(wl_registry_bind(registry, id, &ivi_application_interface, 1));
    } else if (interface == "wl_seat") {
        wl_seat *s = static_cast<wl_seat *>(wl_registry_bind(registry, id, &wl_seat_interface, 1));
        m_seats << new MockSeat(s);
    } else if (interface == "zwp_idle_inhibit_manager_v1") {
        idleInhibitManager = static_cast<zwp_idle_inhibit_manager_v1 *>(wl_registry_bind(registry, id, &zwp_idle_inhibit_manager_v1_interface, 1));
    } else if (interface == "zxdg_output_manager_v1") {
        xdgOutputManager = new QtWayland::zxdg_output_manager_v1(registry, id, 2);
    }
}

void MockClient::handleGlobalRemove(uint32_t id)
{
    auto *output = m_outputs[id];
    if (m_xdgOutputs.contains(output))
        delete m_xdgOutputs.take(output);

    m_outputs.remove(id);
}

wl_surface *MockClient::createSurface()
{
    flushDisplay();
    return wl_compositor_create_surface(compositor);
}

wl_shell_surface *MockClient::createShellSurface(wl_surface *surface)
{
    flushDisplay();
    return wl_shell_get_shell_surface(wlshell, surface);
}

xdg_surface *MockClient::createXdgSurface(wl_surface *surface)
{
    flushDisplay();
    return xdg_wm_base_get_xdg_surface(xdgWmBase, surface);
}

xdg_toplevel *MockClient::createXdgToplevel(xdg_surface *xdgSurface)
{
    flushDisplay();
    return xdg_surface_get_toplevel(xdgSurface);
}

ivi_surface *MockClient::createIviSurface(wl_surface *surface, uint iviId)
{
    flushDisplay();
    return ivi_application_surface_create(iviApplication, iviId, surface);
}

zwp_idle_inhibitor_v1 *MockClient::createIdleInhibitor(wl_surface *surface)
{
    flushDisplay();

    auto *idleInhibitor = zwp_idle_inhibit_manager_v1_create_inhibitor(
                idleInhibitManager, surface);
    zwp_idle_inhibitor_v1_set_user_data(idleInhibitor, this);
    return idleInhibitor;
}

MockXdgOutputV1 *MockClient::createXdgOutput(wl_output *output)
{
    auto *xdgOutput = new MockXdgOutputV1(xdgOutputManager->get_xdg_output(output));
    m_xdgOutputs[output] = xdgOutput;
    return xdgOutput;
}

ShmBuffer::ShmBuffer(const QSize &size, wl_shm *shm)
{
    int stride = size.width() * 4;
    int alloc = stride * size.height();

    char filename[] = "/tmp/wayland-shm-XXXXXX";

    int fd = mkstemp(filename);
    if (fd < 0) {
        qWarning("open %s failed: %s", filename, strerror(errno));
        return;
    }

    if (ftruncate(fd, alloc) < 0) {
        qWarning("ftruncate failed: %s", strerror(errno));
        close(fd);
        return;
    }

    void *data = mmap(nullptr, alloc, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unlink(filename);

    if (data == MAP_FAILED) {
        qWarning("mmap failed: %s", strerror(errno));
        close(fd);
        return;
    }

    image = QImage(static_cast<uchar *>(data), size.width(), size.height(), stride, QImage::Format_ARGB32_Premultiplied);
    shm_pool = wl_shm_create_pool(shm,fd,alloc);
    handle = wl_shm_pool_create_buffer(shm_pool,0, size.width(), size.height(),
                                   stride, WL_SHM_FORMAT_ARGB8888);
    close(fd);
}

ShmBuffer::~ShmBuffer()
{
    munmap(image.bits(), image.sizeInBytes());
    wl_buffer_destroy(handle);
    wl_shm_pool_destroy(shm_pool);
}

