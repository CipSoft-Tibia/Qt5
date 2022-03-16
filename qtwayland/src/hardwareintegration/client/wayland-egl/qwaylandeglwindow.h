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

#ifndef QWAYLANDEGLWINDOW_H
#define QWAYLANDEGLWINDOW_H

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include "qwaylandeglinclude.h"
#include "qwaylandeglclientbufferintegration.h"

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;

namespace QtWaylandClient {

class QWaylandGLContext;

class QWaylandEglWindow : public QWaylandWindow
{
    Q_OBJECT
public:
    QWaylandEglWindow(QWindow *window);
    ~QWaylandEglWindow();
    WindowType windowType() const override;
    void ensureSize() override;

    void updateSurface(bool create);
    virtual void setGeometry(const QRect &rect) override;
    QRect contentsRect() const;

    EGLSurface eglSurface() const;
    GLuint contentFBO() const;
    GLuint contentTexture() const;
    bool needToUpdateContentFBO() const { return decoration() && (m_resize || !m_contentFBO); }

    QSurfaceFormat format() const override;

    void bindContentFBO();

    void invalidateSurface() override;
    void setVisible(bool visible) override;

private:
    QWaylandEglClientBufferIntegration *m_clientBufferIntegration = nullptr;
    struct wl_egl_window *m_waylandEglWindow = nullptr;

    const QWaylandWindow *m_parentWindow = nullptr;

    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLConfig m_eglConfig;
    mutable bool m_resize = false;
    mutable QOpenGLFramebufferObject *m_contentFBO = nullptr;

    QSurfaceFormat m_format;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDEGLWINDOW_H
