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

#ifndef QWAYLANDBRCMEGLWINDOW_H
#define QWAYLANDBRCMEGLWINDOW_H

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include "qwaylandbrcmeglintegration.h"

#include <QMutex>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandGLContext;
class QWaylandBrcmBuffer;

class QWaylandBrcmEglWindow : public QWaylandWindow
{
    Q_OBJECT
public:
    QWaylandBrcmEglWindow(QWindow *window);
    ~QWaylandBrcmEglWindow();
    WindowType windowType() const override;
    void setGeometry(const QRect &rect) override;

    QSurfaceFormat format() const override;

    bool makeCurrent(EGLContext context);
    void swapBuffers();

private:
    void createEglSurfaces();
    void destroyEglSurfaces();

    QWaylandBrcmEglIntegration *m_eglIntegration = nullptr;
    struct wl_egl_window *m_waylandEglWindow = nullptr;

    const QWaylandWindow *m_parentWindow = nullptr;

    EGLConfig m_eglConfig = 0;

    EGLint m_globalImages[3*5];
    EGLSurface m_eglSurfaces[3];

    QWaylandBrcmBuffer *m_buffers[3];
    QSurfaceFormat m_format;

    struct wl_event_queue *m_eventQueue = nullptr;

    int m_current = 0;
    int m_count = 0;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDBRCMEGLWINDOW_H
