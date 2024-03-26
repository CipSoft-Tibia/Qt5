// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "dmabufserverbufferintegration.h"

#include <QtOpenGL/QOpenGLTexture>
#include <QtGui/QOpenGLContext>

#include <drm_fourcc.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

DmaBufServerBuffer::DmaBufServerBuffer(DmaBufServerBufferIntegration *integration, const QImage &qimage, QtWayland::ServerBuffer::Format format)
    : QtWayland::ServerBuffer(qimage.size(),format)
    , m_integration(integration)
{
    m_format = format;

    EGLContext eglContext = eglGetCurrentContext();

    m_texture = new QOpenGLTexture(qimage);

    m_image  = m_integration->eglCreateImageKHR(eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(unsigned long)m_texture->textureId(), nullptr);

    qCDebug(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer created egl image" << m_image;

    int err = eglGetError();
    if (err != EGL_SUCCESS || m_image == EGL_NO_IMAGE_KHR)
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer error creating EGL image" << Qt::hex << err;

    // TODO: formats with more than one plane

    int num_planes = 1;

    if (!m_integration->eglExportDMABUFImageQueryMESA(m_image, &m_fourcc_format, &num_planes, nullptr)) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer: Failed to query egl image";
        qCDebug(qLcWaylandCompositorHardwareIntegration) << "error" << Qt::hex << eglGetError();
    } else {
        qCDebug(qLcWaylandCompositorHardwareIntegration) << "num_planes" << num_planes << "fourcc_format" << m_fourcc_format;
        if (num_planes != 1) {
            qCWarning(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer: multi-plane formats not supported";
            delete m_texture;
            m_texture = nullptr;
            m_integration->eglDestroyImageKHR(m_image);
            m_image = EGL_NO_IMAGE_KHR;
            return;
        }
    }

    if (!m_integration->eglExportDMABUFImageMESA(m_image, &m_fd, &m_stride, &m_offset)) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer: Failed to export egl image. Error code" << Qt::hex << eglGetError();
    } else {
        qCDebug(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer exported egl image: fd" << m_fd << "stride" << m_stride << "offset" << m_offset;
        m_texture->release();
    }
}

DmaBufServerBuffer::~DmaBufServerBuffer()
{
    delete m_texture;

    int err;
    m_integration->eglDestroyImageKHR(m_image);
    if ((err = eglGetError()) != EGL_SUCCESS)
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "~DmaBufServerBuffer: eglDestroyImageKHR error" << Qt::hex << err;

    err = ::close(m_fd);
    if (err)
        perror("~DmaBufServerBuffer:: error closing fd");

}

struct ::wl_resource *DmaBufServerBuffer::resourceForClient(struct ::wl_client *client)
{
    auto *bufferResource = resourceMap().value(client);
    if (!bufferResource) {
        auto integrationResource = m_integration->resourceMap().value(client);
        if (!integrationResource) {
            qCWarning(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer::resourceForClient: Trying to get resource for ServerBuffer. But client is not bound to the qt_dmabuf_server_buffer interface";
            return nullptr;
        }
        struct ::wl_resource *dmabuf_integration_resource = integrationResource->handle;

        Resource *resource = add(client, 1);
        m_integration->send_server_buffer_created(dmabuf_integration_resource, resource->handle, m_fd, m_size.width(), m_size.height(), m_stride, m_offset, m_fourcc_format);
        return resource->handle;
    }
    return bufferResource->handle;
}


QOpenGLTexture *DmaBufServerBuffer::toOpenGlTexture()
{
    if (!m_texture) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "DmaBufServerBuffer::toOpenGlTexture: no texture defined";
    }
    return m_texture;
}

bool DmaBufServerBuffer::bufferInUse()
{
    return resourceMap().size() > 0;
}

DmaBufServerBufferIntegration::DmaBufServerBufferIntegration()
{
}

DmaBufServerBufferIntegration::~DmaBufServerBufferIntegration()
{
}

bool DmaBufServerBufferIntegration::initializeHardware(QWaylandCompositor *compositor)
{
    Q_ASSERT(QGuiApplication::platformNativeInterface());

    m_egl_display = static_cast<EGLDisplay>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("egldisplay"));
    if (!m_egl_display) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "Cannot initialize dmabuf server buffer integration. Missing egl display from platform plugin";
        return false;
    }

    const char *extensionString = eglQueryString(m_egl_display, EGL_EXTENSIONS);
    if (!extensionString || !strstr(extensionString, "EGL_KHR_image")) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "Failed to initialize dmabuf server buffer integration. There is no EGL_KHR_image extension.";
        return false;
    }

    m_egl_create_image = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    m_egl_destroy_image = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    if (!m_egl_create_image || !m_egl_destroy_image) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "Failed to initialize dmabuf server buffer integration. Could not resolve eglCreateImageKHR or eglDestroyImageKHR";
        return false;
    }

    m_gl_egl_image_target_texture_2d = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!m_gl_egl_image_target_texture_2d) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "Failed to initialize dmabuf server buffer integration. Could not find glEGLImageTargetTexture2DOES.";
        return false;
    }

    m_egl_export_dmabuf_image_query = reinterpret_cast<PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC>(eglGetProcAddress("eglExportDMABUFImageQueryMESA"));
    if (!m_egl_export_dmabuf_image_query) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "Failed to initialize dmabuf server buffer integration. Could not find eglExportDMABUFImageQueryMESA.";
        return false;
    }

    m_egl_export_dmabuf_image = reinterpret_cast<PFNEGLEXPORTDMABUFIMAGEMESAPROC>(eglGetProcAddress("eglExportDMABUFImageMESA"));
    if (!m_egl_export_dmabuf_image) {
        qCWarning(qLcWaylandCompositorHardwareIntegration) << "Failed to initialize dmabuf server buffer integration. Could not find eglExportDMABUFImageMESA.";
        return false;
    }

    QtWaylandServer::qt_dmabuf_server_buffer::init(compositor->display(), 1);
    return true;
}

bool DmaBufServerBufferIntegration::supportsFormat(QtWayland::ServerBuffer::Format format) const
{
    // TODO: 8-bit format support
    switch (format) {
    case QtWayland::ServerBuffer::RGBA32:
        return true;
    case QtWayland::ServerBuffer::A8:
        return false;
    default:
        return false;
    }
}

QtWayland::ServerBuffer *DmaBufServerBufferIntegration::createServerBufferFromImage(const QImage &qimage, QtWayland::ServerBuffer::Format format)
{
    return new DmaBufServerBuffer(this, qimage, format);
}

void DmaBufServerBuffer::server_buffer_release(Resource *resource)
{
    qCDebug(qLcWaylandCompositorHardwareIntegration) << "server_buffer RELEASE resource" << resource->handle << wl_resource_get_id(resource->handle) << "for client" << resource->client();
    wl_resource_destroy(resource->handle);
}

QT_END_NAMESPACE
