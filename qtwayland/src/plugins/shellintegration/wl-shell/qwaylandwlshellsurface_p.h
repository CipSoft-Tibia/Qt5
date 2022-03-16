/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QWAYLANDWLSHELLSURFACE_H
#define QWAYLANDWLSHELLSURFACE_H

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

#include <QtCore/QSize>

#include <wayland-client.h>

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

QT_BEGIN_NAMESPACE

class QWindow;

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandInputDevice;
class QWaylandExtendedSurface;

class Q_WAYLAND_CLIENT_EXPORT QWaylandWlShellSurface : public QWaylandShellSurface
    , public QtWayland::wl_shell_surface
{
    Q_OBJECT
public:
    QWaylandWlShellSurface(struct ::wl_shell_surface *shell_surface, QWaylandWindow *window);
    ~QWaylandWlShellSurface() override;

    using QtWayland::wl_shell_surface::resize;
    void resize(QWaylandInputDevice *inputDevice, enum wl_shell_surface_resize edges) override;

    using QtWayland::wl_shell_surface::move;
    bool move(QWaylandInputDevice *inputDevice) override;

    void setTitle(const QString & title) override;
    void setAppId(const QString &appId) override;

    void raise() override;
    void lower() override;
    void setContentOrientationMask(Qt::ScreenOrientations orientation) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void sendProperty(const QString &name, const QVariant &value) override;

    void applyConfigure() override;
    bool wantsDecorations() const override;

protected:
    void requestWindowStates(Qt::WindowStates states) override;

private:
    void setTopLevel();
    void updateTransientParent(QWindow *parent);
    void setPopup(QWaylandWindow *parent, QWaylandInputDevice *device, uint serial);

    QWaylandWindow *m_window = nullptr;
    struct {
        Qt::WindowStates states = Qt::WindowNoState;
        QSize size;
        enum resize edges = resize_none;
    } m_applied, m_pending;
    QSize m_normalSize;
    // There's really no need to have pending and applied state on wl-shell, but we do it just to
    // keep the different shell implementations more similar.
    QWaylandExtendedSurface *m_extendedWindow = nullptr;

    void shell_surface_ping(uint32_t serial) override;
    void shell_surface_configure(uint32_t edges,
                                 int32_t width,
                                 int32_t height) override;
    void shell_surface_popup_done() override;

    friend class QWaylandWindow;
};

QT_END_NAMESPACE

}

#endif // QWAYLANDSHELLSURFACE_H
