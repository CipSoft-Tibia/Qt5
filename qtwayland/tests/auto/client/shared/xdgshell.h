// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MOCKCOMPOSITOR_XDGSHELL_H
#define MOCKCOMPOSITOR_XDGSHELL_H

#include "coreprotocol.h"
#include <qwayland-server-xdg-shell.h>

namespace MockCompositor {

class XdgSurface;
class XdgToplevel;
class XdgPopup;
using XdgPositioner = QtWaylandServer::xdg_positioner;

class XdgWmBase : public Global, public QtWaylandServer::xdg_wm_base
{
    Q_OBJECT
public:
    explicit XdgWmBase(CoreCompositor *compositor, int version = 4);
    using QtWaylandServer::xdg_wm_base::send_ping;
    void send_ping(uint32_t) = delete; // It's a global, use resource specific instead
    bool isClean() override { return m_xdgSurfaces.empty(); }
    QString dirtyMessage() override { return m_xdgSurfaces.empty() ? "clean" : "remaining xdg surfaces"; }
    QList<XdgSurface *> m_xdgSurfaces;
    XdgToplevel *toplevel(int i = 0);
    XdgPopup *popup(int i = 0);
    XdgPopup *m_topmostGrabbingPopup = nullptr;
    CoreCompositor *m_compositor = nullptr;

signals:
    void pong(uint serial);
    void xdgSurfaceCreated(XdgSurface *xdgSurface);
    void toplevelCreated(XdgToplevel *toplevel);

protected:
    void xdg_wm_base_get_xdg_surface(Resource *resource, uint32_t id, ::wl_resource *surface) override;
    void xdg_wm_base_pong(Resource *resource, uint32_t serial) override;
    void xdg_wm_base_create_positioner(Resource *resource, uint32_t id) override
    {
        new XdgPositioner(resource->client(), id, resource->version());
    }
};

class XdgSurface : public QObject, public QtWaylandServer::xdg_surface
{
    Q_OBJECT
public:
    explicit XdgSurface(XdgWmBase *xdgWmBase, Surface *surface, wl_client *client, int id, int version);
    void send_configure(uint serial) = delete; // Use the one below instead, as it tracks state
    void sendConfigure(uint serial);
    uint sendConfigure();
    bool isTopmostGrabbingPopup() const { return m_popup && m_xdgWmBase->m_topmostGrabbingPopup == m_popup; }
    bool isValidPopupGrabParent() const { return isTopmostGrabbingPopup() || (m_toplevel && !m_xdgWmBase->m_topmostGrabbingPopup); }

    // Role objects
    XdgToplevel *m_toplevel = nullptr;
    XdgPopup *m_popup = nullptr;

    XdgWmBase *m_xdgWmBase = nullptr;
    Surface *m_surface = nullptr;
    bool m_configureSent = false;
    QList<uint> m_pendingConfigureSerials;
    uint m_ackedConfigureSerial = 0;
    uint m_committedConfigureSerial = 0;
    struct DoubleBufferedState {
        QRect windowGeometry = {0, 0, 0, 0};
    } m_pending, m_committed;
    QList<XdgPopup *> m_popups;

public slots:
    void verifyConfigured() { QVERIFY(m_configureSent); }

signals:
    void configureCommitted(uint);
    void toplevelCreated(XdgToplevel *toplevel);

protected:
    void xdg_surface_get_toplevel(Resource *resource, uint32_t id) override;
    void xdg_surface_get_popup(Resource *resource, uint32_t id, ::wl_resource *parent, ::wl_resource *positioner) override;
    void xdg_surface_destroy_resource(Resource *resource) override;
    void xdg_surface_destroy(Resource *resource) override;
    void xdg_surface_set_window_geometry(Resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) override;
    void xdg_surface_ack_configure(Resource *resource, uint32_t serial) override;
};

class XdgToplevel : public QObject, public QtWaylandServer::xdg_toplevel
{
    Q_OBJECT
public:
    explicit XdgToplevel(XdgSurface *xdgSurface, int id, int version = 1);
    void sendConfigureBounds(const QSize &size);
    void sendConfigure(const QSize &size = {0, 0}, const QList<uint> &states = {});
    uint sendCompleteConfigure(const QSize &size = {0, 0}, const QList<uint> &states = {});
    Surface *surface() { return m_xdgSurface->m_surface; }

    XdgSurface *m_xdgSurface = nullptr;
    struct DoubleBufferedState {
        QSize minSize = {0, 0};
        QSize maxSize = {0, 0};
    } m_pending, m_committed;

protected:
    void xdg_toplevel_set_max_size(Resource *resource, int32_t width, int32_t height) override;
    void xdg_toplevel_set_min_size(Resource *resource, int32_t width, int32_t height) override;
};

class XdgPopup : public QObject, public QtWaylandServer::xdg_popup
{
    Q_OBJECT
public:
    explicit XdgPopup(XdgSurface *xdgSurface, XdgSurface *parent, int id, int version = 1);
    void sendConfigure(const QRect &geometry);
    uint sendCompleteConfigure(const QRect &geometry);
    Surface *surface() { return m_xdgSurface->m_surface; }
    XdgSurface *m_xdgSurface = nullptr;
    XdgSurface *m_parentXdgSurface = nullptr;
    bool m_grabbed = false;
    uint m_grabSerial = 0;
signals:
    void destroyRequested();
protected:
    void xdg_popup_grab(Resource *resource, ::wl_resource *seat, uint32_t serial) override;
    void xdg_popup_destroy(Resource *resource) override;
};

} // namespace MockCompositor

#endif // MOCKCOMPOSITOR_XDGSHELL_H
