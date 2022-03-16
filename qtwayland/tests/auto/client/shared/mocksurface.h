/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKSURFACE_H
#define MOCKSURFACE_H

#include <qglobal.h>

#include "qwayland-server-wayland.h"

#include "mockcompositor.h"

namespace Impl {

class XdgToplevelV6;
class WlShellSurface;

class Surface : public QtWaylandServer::wl_surface
{
public:
    Surface(wl_client *client, uint32_t id, int v, Compositor *compositor);
    ~Surface();

    Compositor *compositor() const { return m_compositor; }
    static Surface *fromResource(struct ::wl_resource *resource);
    void map();
    bool isMapped() const;
    XdgSurfaceV6 *xdgSurfaceV6() const { return m_xdgSurfaceV6; }
    XdgToplevelV6 *xdgToplevelV6() const { return m_xdgSurfaceV6 ? m_xdgSurfaceV6->toplevel() : nullptr; }
    WlShellSurface *wlShellSurface() const { return m_wlShellSurface; }

    QSharedPointer<MockSurface> mockSurface() const { return m_mockSurface; }

protected:

    void surface_destroy_resource(Resource *resource) override;

    void surface_destroy(Resource *resource) override;
    void surface_attach(Resource *resource,
                        struct wl_resource *buffer, int x, int y) override;
    void surface_damage(Resource *resource,
                        int32_t x, int32_t y, int32_t width, int32_t height) override;
    void surface_frame(Resource *resource,
                       uint32_t callback) override;
    void surface_commit(Resource *resource) override;
private:
    wl_resource *m_buffer = nullptr;
    XdgSurfaceV6 *m_xdgSurfaceV6 = nullptr;
    WlShellSurface *m_wlShellSurface = nullptr;

    Compositor *m_compositor = nullptr;
    QSharedPointer<MockSurface> m_mockSurface;
    QList<wl_resource *> m_frameCallbackList;
    bool m_mapped = false;

    friend class XdgSurfaceV6;
    friend class WlShellSurface;
};

}

#endif // MOCKSURFACE_H
