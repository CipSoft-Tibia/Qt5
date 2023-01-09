/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWLCLIENTBUFFER_P_H
#define QWLCLIENTBUFFER_P_H

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

#include <QtCore/QRect>
#include <QtGui/qopengl.h>
#include <QImage>
#include <QAtomicInt>

#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandBufferRef>

#include <wayland-server-core.h>

QT_BEGIN_NAMESPACE

class QWaylandClientBufferIntegration;
class QWaylandBufferRef;
class QWaylandCompositor;
class QOpenGLTexture;

namespace QtWayland {

struct surface_buffer_destroy_listener
{
    struct wl_listener listener;
    class ClientBuffer *surfaceBuffer = nullptr;
};

class Q_WAYLAND_COMPOSITOR_EXPORT ClientBuffer
{
public:
    ClientBuffer(struct ::wl_resource *bufferResource);

    virtual ~ClientBuffer();

    virtual QWaylandBufferRef::BufferFormatEgl bufferFormatEgl() const;
    virtual QSize size() const = 0;
    virtual QWaylandSurface::Origin origin() const = 0;

    virtual quintptr lockNativeBuffer() { return 0; }
    virtual void unlockNativeBuffer(quintptr native_buffer) const { Q_UNUSED(native_buffer); }

    virtual QImage image() const { return QImage(); }

    inline bool isCommitted() const { return m_committed; }
    virtual void setCommitted(QRegion &damage);
    bool isDestroyed() { return m_destroyed; }

    inline struct ::wl_resource *waylandBufferHandle() const { return m_buffer; }

    bool isSharedMemory() const { return wl_shm_buffer_get(m_buffer); }

#if QT_CONFIG(opengl)
    virtual QOpenGLTexture *toOpenGlTexture(int plane = 0) = 0;
#endif

    static bool hasContent(ClientBuffer *buffer) { return buffer && buffer->waylandBufferHandle(); }

protected:
    void ref();
    void deref();
    void sendRelease();
    virtual void setDestroyed();

    struct ::wl_resource *m_buffer = nullptr;
    QRegion m_damage;
    bool m_textureDirty = false;

private:
    bool m_committed = false;
    bool m_destroyed = false;

    QAtomicInt m_refCount;

    friend class ::QWaylandBufferRef;
    friend class BufferManager;
};

class Q_WAYLAND_COMPOSITOR_EXPORT SharedMemoryBuffer : public ClientBuffer
{
public:
    SharedMemoryBuffer(struct ::wl_resource *bufferResource);

    QSize size() const override;
    QWaylandSurface::Origin origin() const  override;
    QImage image() const override;

#if QT_CONFIG(opengl)
    QOpenGLTexture *toOpenGlTexture(int plane = 0) override;

private:
    QOpenGLTexture *m_shmTexture = nullptr;
#endif
};

}

QT_END_NAMESPACE

#endif // QWLCLIENTBUFFER_P_H
