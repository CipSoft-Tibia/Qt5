// Copyright (C) 2017 Jolla Ltd, author: <giulio.camuffo@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "libhybriseglserverbufferintegration.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTexture>
#include <hybris/eglplatformcommon/hybris_nativebufferext.h>
#include <wayland-server-core.h>

QT_BEGIN_NAMESPACE
LibHybrisEglServerBuffer::LibHybrisEglServerBuffer(LibHybrisEglServerBufferIntegration *integration, const QImage &qimage, QtWayland::ServerBuffer::Format format)
    : QtWayland::ServerBuffer(qimage.size(),format)
    , m_integration(integration)
{
    m_format = format;

    EGLint egl_format;
    switch (m_format) {
        case RGBA32:
            m_hybris_format = QtWaylandServer::qt_libhybris_egl_server_buffer::format_RGBA32;
            egl_format = HYBRIS_PIXEL_FORMAT_RGBA_8888;
            break;
        default:
            qWarning("LibHybrisEglServerBuffer: unsupported format");
            m_hybris_format = QtWaylandServer::qt_libhybris_egl_server_buffer::format_RGBA32;
            egl_format = HYBRIS_PIXEL_FORMAT_RGBA_8888;
            break;
    }

    if (!m_integration->eglHybrisCreateNativeBuffer(m_size.width(), m_size.height(), HYBRIS_USAGE_HW_TEXTURE, egl_format, &m_stride, &m_buffer)) {
        qWarning("LibHybrisEglServerBuffer: Failed to create egl buffer");
        return;
    }

    int numInts;
    int numFds;
    m_integration->eglHybrisGetNativeBufferInfo(m_buffer, &numInts, &numFds);
    m_ints.resize(numInts);
    m_fds.resize(numFds);
    m_integration->eglHybrisSerializeNativeBuffer(m_buffer, m_ints.data(), m_fds.data());

    m_image = m_integration->eglCreateImageKHR(EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, m_buffer, 0);

    if (!QOpenGLContext::currentContext()) {
        qWarning("LibHybrisEglServerBuffer: No current context when creating buffer. Texture loading will fail");
        return;
    }

    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->create();

    m_texture->bind();

    m_integration->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_image);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, qimage.width(), qimage.height(), GL_RGBA, GL_UNSIGNED_BYTE, qimage.constBits());

    m_texture->release();
    m_texture->setSize(m_size.width(), m_size.height());
}

struct ::wl_resource *LibHybrisEglServerBuffer::resourceForClient(struct ::wl_client *client)
{
    auto *bufferResource = resourceMap().value(client);
    if (!bufferResource) {
        auto integrationResource = m_integration->resourceMap().value(client);
        if (!integrationResource) {
            qWarning("LibHybrisEglServerBuffer::resourceForClient: Trying to get resource for ServerBuffer. But client is not bound to the libhybris_egl interface");
            return 0;
        }
        struct ::wl_resource *egl_integration_resource = integrationResource->handle;
        Resource *resource = add(client, 1);
        wl_resource *bufRes = wl_resource_create(client, &qt_libhybris_buffer_interface,-1, 0);

        m_integration->send_server_buffer_created(egl_integration_resource, resource->handle, bufRes, m_fds.size(), QByteArray((char *)m_ints.data(), m_ints.size() * sizeof(int32_t)),
                                    m_name, m_size.width(), m_size.height(), m_stride, m_format);

        for (int i = 0; i < m_fds.size(); ++i) {
            send_add_fd(resource->handle, m_fds.at(i));
        }

        return resource->handle;
    }
    return bufferResource->handle;
}

QOpenGLTexture *LibHybrisEglServerBuffer::toOpenGlTexture()
{
    return m_texture;
}

LibHybrisEglServerBufferIntegration::LibHybrisEglServerBufferIntegration()
{
}

LibHybrisEglServerBufferIntegration::~LibHybrisEglServerBufferIntegration()
{
}

bool LibHybrisEglServerBufferIntegration::initializeHardware(QWaylandCompositor *compositor)
{
    Q_ASSERT(QGuiApplication::platformNativeInterface());

    m_egl_display = static_cast<EGLDisplay>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("egldisplay"));
    if (!m_egl_display) {
        qWarning("Can't initialize libhybris egl server buffer integration. Missing egl display from platform plugin");
        return false;
    }

    m_egl_create_buffer = reinterpret_cast<PFNEGLHYBRISCREATENATIVEBUFFERPROC>(eglGetProcAddress("eglHybrisCreateNativeBuffer"));
    if (!m_egl_create_buffer) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not find eglHybrisCreateNativeBuffer.\n");
        return false;
    }
    m_egl_get_buffer_info = reinterpret_cast<PFNEGLHYBRISGETNATIVEBUFFERINFOPROC>(eglGetProcAddress("eglHybrisGetNativeBufferInfo"));
    if (!m_egl_get_buffer_info) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not find eglHybrisGetNativeBufferInfo.\n");
        return false;
    }
    m_egl_serialize_buffer = reinterpret_cast<PFNEGLHYBRISSERIALIZENATIVEBUFFERPROC>(eglGetProcAddress("eglHybrisSerializeNativeBuffer"));
    if (!m_egl_serialize_buffer) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not find eglHybrisSerializeNativeBuffer.\n");
        return false;
    }

    const char *extensionString = eglQueryString(m_egl_display, EGL_EXTENSIONS);
    if (!extensionString || !strstr(extensionString, "EGL_KHR_image")) {
        qWarning("Failed to initialize libhybris egl server buffer integration. There is no EGL_KHR_image extension.\n");
        return false;
    }
    m_egl_create_image = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    m_egl_destroy_image = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    if (!m_egl_create_image || !m_egl_destroy_image) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not resolve eglCreateImageKHR or eglDestroyImageKHR");
        return false;
    }

    m_gl_egl_image_target_texture_2d = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!m_gl_egl_image_target_texture_2d) {
        qWarning("Failed to initialize libhybris egl server buffer integration. Could not find glEGLImageTargetTexture2DOES.\n");
        return false;
    }

    QtWaylandServer::qt_libhybris_egl_server_buffer::init(compositor->display(), 1);
    return true;
}

bool LibHybrisEglServerBufferIntegration::supportsFormat(QtWayland::ServerBuffer::Format format) const
{
    switch (format) {
        case QtWayland::ServerBuffer::RGBA32:
            return true;
        case QtWayland::ServerBuffer::A8:
            return false;
        default:
            return false;
    }
}

QtWayland::ServerBuffer *LibHybrisEglServerBufferIntegration::createServerBufferFromImage(const QImage &qimage, QtWayland::ServerBuffer::Format format)
{
    return new LibHybrisEglServerBuffer(this, qimage, format);
}

QT_END_NAMESPACE
