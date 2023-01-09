/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWAYLANDXCOMPOSITEGLXINTEGRATION_H
#define QWAYLANDXCOMPOSITEGLXINTEGRATION_H

#include <QtWaylandClient/private/qwaylandclientbufferintegration_p.h>
#include <wayland-client-core.h>

#include <QtCore/QTextStream>
#include <QtCore/QDataStream>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>
#include <QtGui/QWindow>

#include <X11/Xlib.h>

// avoid clashes with Qt::CursorShape
#ifdef CursorShape
#   define X_CursorShape CursorShape
#   undef CursorShape
#endif

struct qt_xcomposite;
struct qt_xcomposite_listener;
struct wl_registry;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandXCompositeGLXIntegration : public QWaylandClientBufferIntegration
{
public:
    QWaylandXCompositeGLXIntegration();
    ~QWaylandXCompositeGLXIntegration() override;

    void initialize(QWaylandDisplay *display) override;

    QWaylandWindow *createEglWindow(QWindow *window) override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share) const override;

    QWaylandDisplay *waylandDisplay() const;
    struct qt_xcomposite *waylandXComposite() const;

    Display *xDisplay() const;
    int screen() const;
    Window rootWindow() const;

    bool supportsThreadedOpenGL() const override { return false; }
    bool supportsWindowDecoration() const override { return false; }

private:
    QWaylandDisplay *mWaylandDisplay = nullptr;
    struct qt_xcomposite *mWaylandComposite = nullptr;

    Display *mDisplay = nullptr;
    int mScreen = 0;
    Window mRootWindow = 0;

    static void wlDisplayHandleGlobal(void *data, struct ::wl_registry *registry, uint32_t id,
                                      const QString &interface, uint32_t version);

    static const struct qt_xcomposite_listener xcomposite_listener;
    static void rootInformation(void *data,
                 struct qt_xcomposite *xcomposite,
                 const char *display_name,
                 uint32_t root_window);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDXCOMPOSITEGLXINTEGRATION_H
