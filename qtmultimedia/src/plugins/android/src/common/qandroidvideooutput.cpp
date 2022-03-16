/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "qandroidvideooutput.h"

#include "androidsurfacetexture.h"
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <qevent.h>
#include <qcoreapplication.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <qopenglshaderprogram.h>
#include <qopenglframebufferobject.h>
#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

static const GLfloat g_vertex_data[] = {
    -1.f, 1.f,
    1.f, 1.f,
    1.f, -1.f,
    -1.f, -1.f
};

static const GLfloat g_texture_data[] = {
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f,
    0.f, 1.f
};

void OpenGLResourcesDeleter::deleteTextureHelper(quint32 id)
{
    if (id != 0)
        glDeleteTextures(1, &id);
}

void OpenGLResourcesDeleter::deleteFboHelper(void *fbo)
{
    delete reinterpret_cast<QOpenGLFramebufferObject *>(fbo);
}

void OpenGLResourcesDeleter::deleteShaderProgramHelper(void *prog)
{
    delete reinterpret_cast<QOpenGLShaderProgram *>(prog);
}

void OpenGLResourcesDeleter::deleteThisHelper()
{
    delete this;
}

class AndroidTextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    AndroidTextureVideoBuffer(QAndroidTextureVideoOutput *output, const QSize &size)
        : QAbstractVideoBuffer(GLTextureHandle)
        , m_mapMode(NotMapped)
        , m_output(output)
        , m_size(size)
        , m_textureUpdated(false)
    {
    }

    virtual ~AndroidTextureVideoBuffer() {}

    MapMode mapMode() const { return m_mapMode; }

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine)
    {
        if (m_mapMode == NotMapped && mode == ReadOnly && updateFrame()) {
            m_mapMode = mode;
            m_image = m_output->m_fbo->toImage();

            if (numBytes)
                *numBytes = m_image.byteCount();

            if (bytesPerLine)
                *bytesPerLine = m_image.bytesPerLine();

            return m_image.bits();
        } else {
            return 0;
        }
    }

    void unmap()
    {
        m_image = QImage();
        m_mapMode = NotMapped;
    }

    QVariant handle() const
    {
        AndroidTextureVideoBuffer *that = const_cast<AndroidTextureVideoBuffer*>(this);
        if (!that->updateFrame())
            return QVariant();

        return m_output->m_fbo->texture();
    }

private:
    bool updateFrame()
    {
        // Even though the texture was updated in a previous call, we need to re-check
        // that this has not become a stale buffer, e.g., if the output size changed or
        // has since became invalid.
        if (!m_output->m_nativeSize.isValid())
            return false;

        // Size changed
        if (m_output->m_nativeSize != m_size)
            return false;

        // In the unlikely event that we don't have a valid fbo, but have a valid size,
        // force an update.
        const bool forceUpdate = !m_output->m_fbo;

        if (m_textureUpdated && !forceUpdate)
            return true;

        // update the video texture (called from the render thread)
        return (m_textureUpdated = m_output->renderFrameToFbo());
    }

    MapMode m_mapMode;
    QAndroidTextureVideoOutput *m_output;
    QImage m_image;
    QSize m_size;
    bool m_textureUpdated;
};

QAndroidTextureVideoOutput::QAndroidTextureVideoOutput(QObject *parent)
    : QAndroidVideoOutput(parent)
    , m_surface(0)
    , m_surfaceTexture(0)
    , m_externalTex(0)
    , m_fbo(0)
    , m_program(0)
    , m_glDeleter(0)
    , m_surfaceTextureCanAttachToContext(QtAndroidPrivate::androidSdkVersion() >= 16)
{

}

QAndroidTextureVideoOutput::~QAndroidTextureVideoOutput()
{
    clearSurfaceTexture();

    if (m_glDeleter) { // Make sure all of these are deleted on the render thread.
        m_glDeleter->deleteFbo(m_fbo);
        m_glDeleter->deleteShaderProgram(m_program);
        m_glDeleter->deleteTexture(m_externalTex);
        m_glDeleter->deleteThis();
    }
}

QAbstractVideoSurface *QAndroidTextureVideoOutput::surface() const
{
    return m_surface;
}

void QAndroidTextureVideoOutput::setSurface(QAbstractVideoSurface *surface)
{
    if (surface == m_surface)
        return;

    if (m_surface) {
        if (m_surface->isActive())
            m_surface->stop();

        if (!m_surfaceTextureCanAttachToContext)
            m_surface->setProperty("_q_GLThreadCallback", QVariant());
    }

    m_surface = surface;

    if (m_surface && !m_surfaceTextureCanAttachToContext) {
        m_surface->setProperty("_q_GLThreadCallback",
                               QVariant::fromValue<QObject*>(this));
    }
}

bool QAndroidTextureVideoOutput::isReady()
{
    return m_surfaceTextureCanAttachToContext || QOpenGLContext::currentContext() || m_externalTex;
}

bool QAndroidTextureVideoOutput::initSurfaceTexture()
{
    if (m_surfaceTexture)
        return true;

    if (!m_surface)
        return false;

    if (!m_surfaceTextureCanAttachToContext) {
        // if we have an OpenGL context in the current thread, create a texture. Otherwise, wait
        // for the GL render thread to call us back to do it.
        if (QOpenGLContext::currentContext()) {
            glGenTextures(1, &m_externalTex);
            if (!m_glDeleter)
                m_glDeleter = new OpenGLResourcesDeleter;
        } else if (!m_externalTex) {
            return false;
        }
    }

    QMutexLocker locker(&m_mutex);

    m_surfaceTexture = new AndroidSurfaceTexture(m_externalTex);

    if (m_surfaceTexture->surfaceTexture() != 0) {
        connect(m_surfaceTexture, SIGNAL(frameAvailable()), this, SLOT(onFrameAvailable()));
    } else {
        delete m_surfaceTexture;
        m_surfaceTexture = 0;
        if (m_glDeleter)
            m_glDeleter->deleteTexture(m_externalTex);
        m_externalTex = 0;
    }

    return m_surfaceTexture != 0;
}

void QAndroidTextureVideoOutput::clearSurfaceTexture()
{
    QMutexLocker locker(&m_mutex);
    if (m_surfaceTexture) {
        delete m_surfaceTexture;
        m_surfaceTexture = 0;
    }

    // Also reset the attached OpenGL texture
    // Note: The Android SurfaceTexture class does not release the texture on deletion,
    // only if detachFromGLContext() called (API level >= 16), so we'll do it manually,
    // on the render thread.
    if (m_surfaceTextureCanAttachToContext) {
        if (m_glDeleter)
            m_glDeleter->deleteTexture(m_externalTex);
        m_externalTex = 0;
    }
}

AndroidSurfaceTexture *QAndroidTextureVideoOutput::surfaceTexture()
{
    if (!initSurfaceTexture())
        return 0;

    return m_surfaceTexture;
}

void QAndroidTextureVideoOutput::setVideoSize(const QSize &size)
{
     QMutexLocker locker(&m_mutex);
    if (m_nativeSize == size)
        return;

    stop();

    m_nativeSize = size;
}

void QAndroidTextureVideoOutput::stop()
{
    if (m_surface && m_surface->isActive())
        m_surface->stop();
    m_nativeSize = QSize();
}

void QAndroidTextureVideoOutput::reset()
{
    // flush pending frame
    if (m_surface)
        m_surface->present(QVideoFrame());

    clearSurfaceTexture();
}

void QAndroidTextureVideoOutput::onFrameAvailable()
{
    if (!m_nativeSize.isValid() || !m_surface)
        return;

    QAbstractVideoBuffer *buffer = new AndroidTextureVideoBuffer(this, m_nativeSize);
    QVideoFrame frame(buffer, m_nativeSize, QVideoFrame::Format_BGR32);

    if (m_surface->isActive() && (m_surface->surfaceFormat().pixelFormat() != frame.pixelFormat()
                                  || m_surface->surfaceFormat().frameSize() != frame.size())) {
        m_surface->stop();
    }

    if (!m_surface->isActive()) {
        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(),
                                   QAbstractVideoBuffer::GLTextureHandle);

        m_surface->start(format);
    }

    if (m_surface->isActive())
        m_surface->present(frame);
}

bool QAndroidTextureVideoOutput::renderFrameToFbo()
{
    QMutexLocker locker(&m_mutex);

    if (!m_nativeSize.isValid() || !m_surfaceTexture)
        return false;

    createGLResources();

    m_surfaceTexture->updateTexImage();

    // save current render states
    GLboolean stencilTestEnabled;
    GLboolean depthTestEnabled;
    GLboolean scissorTestEnabled;
    GLboolean blendEnabled;
    glGetBooleanv(GL_STENCIL_TEST, &stencilTestEnabled);
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glGetBooleanv(GL_SCISSOR_TEST, &scissorTestEnabled);
    glGetBooleanv(GL_BLEND, &blendEnabled);

    if (stencilTestEnabled)
        glDisable(GL_STENCIL_TEST);
    if (depthTestEnabled)
        glDisable(GL_DEPTH_TEST);
    if (scissorTestEnabled)
        glDisable(GL_SCISSOR_TEST);
    if (blendEnabled)
        glDisable(GL_BLEND);

    m_fbo->bind();

    glViewport(0, 0, m_nativeSize.width(), m_nativeSize.height());

    m_program->bind();
    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    m_program->setUniformValue("frameTexture", GLuint(0));
    m_program->setUniformValue("texMatrix", m_surfaceTexture->getTransformMatrix());

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, g_vertex_data);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, g_texture_data);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->disableAttributeArray(0);
    m_program->disableAttributeArray(1);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    m_fbo->release();

    // restore render states
    if (stencilTestEnabled)
        glEnable(GL_STENCIL_TEST);
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    if (scissorTestEnabled)
        glEnable(GL_SCISSOR_TEST);
    if (blendEnabled)
        glEnable(GL_BLEND);

    return true;
}

void QAndroidTextureVideoOutput::createGLResources()
{
    Q_ASSERT(QOpenGLContext::currentContext() != NULL);

    if (!m_glDeleter)
        m_glDeleter = new OpenGLResourcesDeleter;

    if (m_surfaceTextureCanAttachToContext && !m_externalTex) {
        m_surfaceTexture->detachFromGLContext();
        glGenTextures(1, &m_externalTex);
        m_surfaceTexture->attachToGLContext(m_externalTex);
    }

    if (!m_fbo || m_fbo->size() != m_nativeSize) {
        delete m_fbo;
        m_fbo = new QOpenGLFramebufferObject(m_nativeSize);
    }

    if (!m_program) {
        m_program = new QOpenGLShaderProgram;

        QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, m_program);
        vertexShader->compileSourceCode("attribute highp vec4 vertexCoordsArray; \n" \
                                        "attribute highp vec2 textureCoordArray; \n" \
                                        "uniform   highp mat4 texMatrix; \n" \
                                        "varying   highp vec2 textureCoords; \n" \
                                        "void main(void) \n" \
                                        "{ \n" \
                                        "    gl_Position = vertexCoordsArray; \n" \
                                        "    textureCoords = (texMatrix * vec4(textureCoordArray, 0.0, 1.0)).xy; \n" \
                                        "}\n");
        m_program->addShader(vertexShader);

        QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, m_program);
        fragmentShader->compileSourceCode("#extension GL_OES_EGL_image_external : require \n" \
                                          "varying highp vec2         textureCoords; \n" \
                                          "uniform samplerExternalOES frameTexture; \n" \
                                          "void main() \n" \
                                          "{ \n" \
                                          "    gl_FragColor = texture2D(frameTexture, textureCoords); \n" \
                                          "}\n");
        m_program->addShader(fragmentShader);

        m_program->bindAttributeLocation("vertexCoordsArray", 0);
        m_program->bindAttributeLocation("textureCoordArray", 1);
        m_program->link();
    }
}

void QAndroidTextureVideoOutput::customEvent(QEvent *e)
{
    if (e->type() == QEvent::User) {
        // This is running in the render thread (OpenGL enabled)
        if (!m_surfaceTextureCanAttachToContext && !m_externalTex) {
            glGenTextures(1, &m_externalTex);
            if (!m_glDeleter) // We'll use this to cleanup GL resources in the correct thread
                m_glDeleter = new OpenGLResourcesDeleter;
            emit readyChanged(true);
        }
    }
}

QT_END_NAMESPACE
