/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
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

#include "drmeglserverbufferintegration.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTexture>

QT_BEGIN_NAMESPACE

DrmEglServerBuffer::DrmEglServerBuffer(DrmEglServerBufferIntegration *integration, const QImage &qimage, QtWayland::ServerBuffer::Format format)
    : QtWayland::ServerBuffer(qimage.size(),format)
    , m_integration(integration)
{
    m_format = format;

    EGLint egl_format;
    switch (m_format) {
        case RGBA32:
            m_drm_format = QtWaylandServer::qt_drm_egl_server_buffer::format_RGBA32;
            egl_format = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
            break;
#ifdef EGL_DRM_BUFFER_FORMAT_A8_MESA
        case A8:
            m_drm_format = QtWaylandServer::qt_drm_egl_server_buffer::format_A8;
            egl_format = EGL_DRM_BUFFER_FORMAT_A8_MESA;
            break;
#endif
        default:
            qWarning("DrmEglServerBuffer: unsupported format");
            m_drm_format = QtWaylandServer::qt_drm_egl_server_buffer::format_RGBA32;
            egl_format = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
            break;
    }
    EGLint imageAttribs[] = {
        EGL_WIDTH, m_size.width(),
        EGL_HEIGHT, m_size.height(),
        EGL_DRM_BUFFER_FORMAT_MESA, egl_format,
        EGL_DRM_BUFFER_USE_MESA, EGL_DRM_BUFFER_USE_SHARE_MESA,
        EGL_NONE
    };

    m_image = m_integration->eglCreateDRMImageMESA(imageAttribs);

    EGLint handle;
    if (!m_integration->eglExportDRMImageMESA(m_image, &m_name, &handle, &m_stride)) {
        qWarning("DrmEglServerBuffer: Failed to export egl image");
    }

    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->create();
    m_texture->bind();

    m_integration->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_image);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, qimage.width(), qimage.height(), GL_RGBA, GL_UNSIGNED_BYTE, qimage.constBits());

    m_texture->release();
    m_texture->setSize(m_size.width(), m_size.height());
}

struct ::wl_resource *DrmEglServerBuffer::resourceForClient(struct ::wl_client *client)
{
    auto *bufferResource = resourceMap().value(client);
    if (!bufferResource) {
        auto integrationResource = m_integration->resourceMap().value(client);
        if (!integrationResource) {
            qWarning("DrmEglServerBuffer::resourceForClient: Trying to get resource for ServerBuffer. But client is not bound to the drm_egl interface");
            return nullptr;
        }
        struct ::wl_resource *drm_egl_integration_resource = integrationResource->handle;
        Resource *resource = add(client, 1);
        m_integration->send_server_buffer_created(drm_egl_integration_resource, resource->handle, m_name, m_size.width(), m_size.height(), m_stride, m_drm_format);
        return resource->handle;
    }
    return bufferResource->handle;
}


QOpenGLTexture *DrmEglServerBuffer::toOpenGlTexture()
{
    if (!m_texture) {
        qWarning("DrmEglServerBuffer::toOpenGlTexture: no texture defined");
    }
    return m_texture;
}

bool DrmEglServerBuffer::bufferInUse()
{
    return resourceMap().count() > 0;
}

DrmEglServerBufferIntegration::DrmEglServerBufferIntegration()
{
}

DrmEglServerBufferIntegration::~DrmEglServerBufferIntegration()
{
}

void DrmEglServerBufferIntegration::initializeHardware(QWaylandCompositor *compositor)
{
    Q_ASSERT(QGuiApplication::platformNativeInterface());

    m_egl_display = static_cast<EGLDisplay>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("egldisplay"));
    if (!m_egl_display) {
        qWarning("Can't initialize drm egl server buffer integration. Missing egl display from platform plugin");
        return;
    }

    const char *extensionString = eglQueryString(m_egl_display, EGL_EXTENSIONS);
    if (!extensionString || !strstr(extensionString, "EGL_KHR_image")) {
        qWarning("Failed to initialize drm egl server buffer integration. There is no EGL_KHR_image extension.\n");
        return;
    }
    m_egl_create_image = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    m_egl_destroy_image = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    if (!m_egl_create_image || !m_egl_destroy_image) {
        qWarning("Failed to initialize drm egl server buffer integration. Could not resolve eglCreateImageKHR or eglDestroyImageKHR");
        return;
    }

    if (!extensionString || !strstr(extensionString, "EGL_MESA_drm_image")) {
        qWarning("Failed to initialize drm egl server buffer integration. There is no EGL_MESA_drm_image extension.\n");
        return;
    }

    m_egl_create_drm_image = reinterpret_cast<PFNEGLCREATEDRMIMAGEMESAPROC>(eglGetProcAddress("eglCreateDRMImageMESA"));
    m_egl_export_drm_image = reinterpret_cast<PFNEGLEXPORTDRMIMAGEMESAPROC>(eglGetProcAddress("eglExportDRMImageMESA"));
    if (!m_egl_create_drm_image || !m_egl_export_drm_image) {
        qWarning("Failed to initialize drm egl server buffer integration. Could not find eglCreateDRMImageMESA or eglExportDRMImageMESA.\n");
        return;
    }

    m_gl_egl_image_target_texture_2d = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!m_gl_egl_image_target_texture_2d) {
        qWarning("Failed to initialize drm egl server buffer integration. Could not find glEGLImageTargetTexture2DOES.\n");
        return;
    }

    QtWaylandServer::qt_drm_egl_server_buffer::init(compositor->display(), 1);
}

bool DrmEglServerBufferIntegration::supportsFormat(QtWayland::ServerBuffer::Format format) const
{
    switch (format) {
        case QtWayland::ServerBuffer::RGBA32:
            return true;
        case QtWayland::ServerBuffer::A8:
#ifdef EGL_DRM_BUFFER_FORMAT_A8_MESA
            return true;
#else
            return false;
#endif
        default:
            return false;
    }
}

QtWayland::ServerBuffer *DrmEglServerBufferIntegration::createServerBufferFromImage(const QImage &qimage, QtWayland::ServerBuffer::Format format)
{
    return new DrmEglServerBuffer(this, qimage, format);
}

QT_END_NAMESPACE
