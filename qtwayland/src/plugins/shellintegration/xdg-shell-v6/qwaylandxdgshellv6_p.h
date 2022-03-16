/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2017 Eurogiciel, author: <philippe.coval@eurogiciel.fr>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the config.tests of the Qt Toolkit.
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

#ifndef QWAYLANDXDGSHELLV6_H
#define QWAYLANDXDGSHELLV6_H

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

#include "qwayland-xdg-shell-unstable-v6.h"

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include <QtCore/QSize>
#include <QtGui/QRegion>

#include <wayland-client.h>

QT_BEGIN_NAMESPACE

class QWindow;

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandInputDevice;
class QWaylandXdgShellV6;

class Q_WAYLAND_CLIENT_EXPORT QWaylandXdgSurfaceV6 : public QWaylandShellSurface, public QtWayland::zxdg_surface_v6
{
    Q_OBJECT
public:
    QWaylandXdgSurfaceV6(QWaylandXdgShellV6 *shell, ::zxdg_surface_v6 *surface, QWaylandWindow *window);
    ~QWaylandXdgSurfaceV6() override;

    void resize(QWaylandInputDevice *inputDevice, enum zxdg_toplevel_v6_resize_edge edges);
    void resize(QWaylandInputDevice *inputDevice, enum wl_shell_surface_resize edges) override;
    bool move(QWaylandInputDevice *inputDevice) override;
    void setTitle(const QString &title) override;
    void setAppId(const QString &appId) override;

    bool isExposed() const override;
    bool handleExpose(const QRegion &) override;
    bool handlesActiveState() const { return m_toplevel; }
    void applyConfigure() override;
    bool wantsDecorations() const override;

protected:
    void requestWindowStates(Qt::WindowStates states) override;
    void zxdg_surface_v6_configure(uint32_t serial) override;

private:
    class Toplevel: public QtWayland::zxdg_toplevel_v6
    {
    public:
        Toplevel(QWaylandXdgSurfaceV6 *xdgSurface);
        ~Toplevel() override;

        void applyConfigure();

        void zxdg_toplevel_v6_configure(int32_t width, int32_t height, wl_array *states) override;
        void zxdg_toplevel_v6_close() override;

        void requestWindowStates(Qt::WindowStates states);
        struct {
            QSize size = {0, 0};
            Qt::WindowStates states = Qt::WindowNoState;
        }  m_pending, m_applied;
        QSize m_normalSize;

        QWaylandXdgSurfaceV6 *m_xdgSurface = nullptr;
    };

    class Popup : public QtWayland::zxdg_popup_v6 {
    public:
        Popup(QWaylandXdgSurfaceV6 *xdgSurface, QWaylandXdgSurfaceV6 *parent, QtWayland::zxdg_positioner_v6 *positioner);
        ~Popup() override;

        void grab(QWaylandInputDevice *seat, uint serial);
        void zxdg_popup_v6_popup_done() override;

        QWaylandXdgSurfaceV6 *m_xdgSurface = nullptr;
        QWaylandXdgSurfaceV6 *m_parent = nullptr;
        bool m_grabbing = false;
    };

    void setToplevel();
    void setPopup(QWaylandWindow *parent);
    void setGrabPopup(QWaylandWindow *parent, QWaylandInputDevice *device, int serial);

    QWaylandXdgShellV6 *m_shell = nullptr;
    QWaylandWindow *m_window = nullptr;
    Toplevel *m_toplevel = nullptr;
    Popup *m_popup = nullptr;
    bool m_configured = false;
    QRegion m_exposeRegion;
    uint m_pendingConfigureSerial = 0;

    friend class QWaylandXdgShellV6;
};

class Q_WAYLAND_CLIENT_EXPORT QWaylandXdgShellV6 : public QtWayland::zxdg_shell_v6
{
public:
    QWaylandXdgShellV6(struct ::wl_registry *registry, uint32_t id, uint32_t availableVersion);

    QWaylandXdgSurfaceV6 *getXdgSurface(QWaylandWindow *window);

    ~QWaylandXdgShellV6() override;

private:
    void zxdg_shell_v6_ping(uint32_t serial) override;
    QWaylandXdgSurfaceV6::Popup *m_topmostGrabbingPopup = nullptr;

    friend class QWaylandXdgSurfaceV6;
};

QT_END_NAMESPACE

}

#endif // QWAYLANDXDGSHELLV6_H
