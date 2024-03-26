// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "graphicshelperes2_p.h"
#include <private/attachmentpack_p.h>
#include <qgraphicsutils_p.h>
#include <renderbuffer_p.h>
#include <logging_p.h>
#include <QtGui/private/qopenglextensions_p.h>


QT_BEGIN_NAMESPACE

// ES 3.0+
#ifndef GL_SAMPLER_3D
#define GL_SAMPLER_3D                     0x8B5F
#endif
#ifndef GL_SAMPLER_2D_SHADOW
#define GL_SAMPLER_2D_SHADOW              0x8B62
#endif
#ifndef GL_SAMPLER_CUBE_SHADOW
#define GL_SAMPLER_CUBE_SHADOW            0x8DC5
#endif
#ifndef GL_SAMPLER_2D_ARRAY
#define GL_SAMPLER_2D_ARRAY               0x8DC1
#endif
#ifndef GL_SAMPLER_2D_ARRAY_SHADOW
#define GL_SAMPLER_2D_ARRAY_SHADOW        0x8DC4
#endif

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

GraphicsHelperES2::GraphicsHelperES2()
    : m_funcs(0)
    , m_supportFramebufferBlit(false)
{
}

GraphicsHelperES2::~GraphicsHelperES2()
{
}

void GraphicsHelperES2::initializeHelper(QOpenGLContext *context,
                                         QAbstractOpenGLFunctions *)
{
    Q_ASSERT(context);
    m_funcs = context->functions();
    Q_ASSERT(m_funcs);
    m_ext.reset(new QOpenGLExtensions(context));
    if (m_ext->hasOpenGLExtension(QOpenGLExtensions::FramebufferBlit))
        m_supportFramebufferBlit = true;
}

void GraphicsHelperES2::drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType,
                                                                    GLsizei primitiveCount,
                                                                    GLint indexType,
                                                                    void *indices,
                                                                    GLsizei instances,
                                                                    GLint baseVertex,
                                                                    GLint baseInstance)
{
    if (baseInstance != 0)
        qWarning() << "glDrawElementsInstancedBaseVertexBaseInstance is not supported with OpenGL ES 2";

    if (baseVertex != 0)
        qWarning() << "glDrawElementsInstancedBaseVertex is not supported with OpenGL ES 2";

    for (GLint i = 0; i < instances; i++)
        drawElements(primitiveType,
                     primitiveCount,
                     indexType,
                     indices);
}

void GraphicsHelperES2::drawArraysInstanced(GLenum primitiveType,
                                             GLint first,
                                             GLsizei count,
                                             GLsizei instances)
{
    for (GLint i = 0; i < instances; i++)
        drawArrays(primitiveType,
                   first,
                   count);
}

void GraphicsHelperES2::drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseInstance)
{
    if (baseInstance != 0)
        qWarning() << "glDrawArraysInstancedBaseInstance is not supported with OpenGL ES 2";
    for (GLint i = 0; i < instances; i++)
        drawArrays(primitiveType,
                   first,
                   count);
}

void GraphicsHelperES2::drawElements(GLenum primitiveType,
                                      GLsizei primitiveCount,
                                      GLint indexType,
                                      void *indices,
                                      GLint baseVertex)
{
    if (baseVertex != 0)
        qWarning() << "glDrawElementsBaseVertex is not supported with OpenGL ES 2";
    QOpenGLExtensions *xfuncs = static_cast<QOpenGLExtensions *>(m_funcs);
    if (indexType == GL_UNSIGNED_INT && !xfuncs->hasOpenGLExtension(QOpenGLExtensions::ElementIndexUint)) {
        static bool warnShown = false;
        if (!warnShown) {
            warnShown = true;
            qWarning("GL_UNSIGNED_INT index type not supported on this system, skipping draw call.");
        }
        return;
    }
    m_funcs->glDrawElements(primitiveType,
                            primitiveCount,
                            indexType,
                            indices);
}

void GraphicsHelperES2::drawArrays(GLenum primitiveType,
                                    GLint first,
                                    GLsizei count)
{
    m_funcs->glDrawArrays(primitiveType,
                          first,
                          count);
}

void GraphicsHelperES2::drawElementsIndirect(GLenum, GLenum, void *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "Indirect Drawing is not supported with OpenGL ES 2";
}

void GraphicsHelperES2::drawArraysIndirect(GLenum , void *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "Indirect Drawing is not supported with OpenGL ES 2";
}

void GraphicsHelperES2::setVerticesPerPatch(GLint verticesPerPatch)
{
    Q_UNUSED(verticesPerPatch);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "Tessellation not supported with OpenGL ES 2";
}

void GraphicsHelperES2::useProgram(GLuint programId)
{
    m_funcs->glUseProgram(programId);
}

std::vector<ShaderUniform> GraphicsHelperES2::programUniformsAndLocations(GLuint programId)
{
    std::vector<ShaderUniform> uniforms;

    GLint nbrActiveUniforms = 0;
    m_funcs->glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &nbrActiveUniforms);
    uniforms.reserve(nbrActiveUniforms);
    char uniformName[256];
    for (GLint i = 0; i < nbrActiveUniforms; i++) {
        ShaderUniform uniform;
        GLsizei uniformNameLength = 0;
        // Size is 1 for scalar and more for struct or arrays
        // Type is the GL Type
        m_funcs->glGetActiveUniform(programId, i, sizeof(uniformName) - 1, &uniformNameLength,
                                    &uniform.m_size, &uniform.m_type, uniformName);
        uniformName[sizeof(uniformName) - 1] = '\0';
        uniform.m_location = m_funcs->glGetUniformLocation(programId, uniformName);
        uniform.m_name = QString::fromUtf8(uniformName, uniformNameLength);
        // Work around for uniform array names that aren't returned with [0] by some drivers
        if (uniform.m_size > 1 && !uniform.m_name.endsWith(QLatin1String("[0]")))
            uniform.m_name.append(QLatin1String("[0]"));
        uniform.m_rawByteSize = uniformByteSize(uniform);
        uniforms.push_back(uniform);
    }
    return uniforms;
}

std::vector<ShaderAttribute> GraphicsHelperES2::programAttributesAndLocations(GLuint programId)
{
    std::vector<ShaderAttribute> attributes;
    GLint nbrActiveAttributes = 0;
    m_funcs->glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTES, &nbrActiveAttributes);
    attributes.reserve(nbrActiveAttributes);
    char attributeName[256];
    for (GLint i = 0; i < nbrActiveAttributes; i++) {
        ShaderAttribute attribute;
        GLsizei attributeNameLength = 0;
        // Size is 1 for scalar and more for struct or arrays
        // Type is the GL Type
        m_funcs->glGetActiveAttrib(programId, i, sizeof(attributeName) - 1, &attributeNameLength,
                                   &attribute.m_size, &attribute.m_type, attributeName);
        attributeName[sizeof(attributeName) - 1] = '\0';
        attribute.m_location = m_funcs->glGetAttribLocation(programId, attributeName);
        attribute.m_name = QString::fromUtf8(attributeName, attributeNameLength);
        attributes.push_back(attribute);
    }
    return attributes;
}

std::vector<ShaderUniformBlock> GraphicsHelperES2::programUniformBlocks(GLuint programId)
{
    Q_UNUSED(programId);
    static bool showWarning = true;
    if (!showWarning)
        return {};
    showWarning = false;
    qWarning() << "UBO are not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
    return {};
}

std::vector<ShaderStorageBlock> GraphicsHelperES2::programShaderStorageBlocks(GLuint programId)
{
    Q_UNUSED(programId);
    static bool showWarning = true;
    if (!showWarning)
        return {};
    showWarning = false;
    qWarning() << "SSBO are not supported by OpenGL ES 2.0 (since OpenGL ES 3.1)";
    return {};
}

void GraphicsHelperES2::vertexAttribDivisor(GLuint index, GLuint divisor)
{
    Q_UNUSED(index);
    Q_UNUSED(divisor);
}

void GraphicsHelperES2::vertexAttributePointer(GLenum shaderDataType,
                                               GLuint index,
                                               GLint size,
                                               GLenum type,
                                               GLboolean normalized,
                                               GLsizei stride,
                                               const GLvoid *pointer)
{
    switch (shaderDataType) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
    case GL_FLOAT_MAT2:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT4:
        m_funcs->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        break;

    default:
        qCWarning(Rendering) << "vertexAttribPointer: Unhandled type";
        Q_UNREACHABLE();
    }
}

void GraphicsHelperES2::readBuffer(GLenum mode)
{
    Q_UNUSED(mode);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glReadBuffer not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
}

void GraphicsHelperES2::drawBuffer(GLenum mode)
{
    Q_UNUSED(mode);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glDrawBuffer is not supported with OpenGL ES 2";
}

void *GraphicsHelperES2::fenceSync()
{
    qWarning() << "Fences are not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
    return nullptr;
}

void GraphicsHelperES2::clientWaitSync(void *, GLuint64 )
{
    qWarning() << "Fences are not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
}

void GraphicsHelperES2::waitSync(void *)
{
    qWarning() << "Fences are not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
}

bool GraphicsHelperES2::wasSyncSignaled(void *)
{
    qWarning() << "Fences are not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
    return false;
}

void GraphicsHelperES2::deleteSync(void *)
{
    qWarning() << "Fences are not supported by OpenGL ES 2.0 (since OpenGL ES 3.0)";
}

void GraphicsHelperES2::rasterMode(GLenum faceMode, GLenum rasterMode)
{
    Q_UNUSED(faceMode);
    Q_UNUSED(rasterMode);
    qWarning() << "glPolyonMode is not supported with OpenGL ES";
}

void GraphicsHelperES2::blendEquation(GLenum mode)
{
    m_funcs->glBlendEquation(mode);
}

void GraphicsHelperES2::blendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
    Q_UNUSED(buf);
    Q_UNUSED(sfactor);
    Q_UNUSED(dfactor);

    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glBlendFunci() not supported by OpenGL ES 2.0";
}

void GraphicsHelperES2::blendFuncSeparatei(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sAlpha, GLenum dAlpha)
{
    Q_UNUSED(buf);
    Q_UNUSED(sRGB);
    Q_UNUSED(dRGB);
    Q_UNUSED(sAlpha);
    Q_UNUSED(dAlpha);

    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glBlendFuncSeparatei() not supported by OpenGL ES 2.0";
}

void GraphicsHelperES2::alphaTest(GLenum, GLenum)
{
    qCWarning(Rendering) << Q_FUNC_INFO << "AlphaTest not available with OpenGL ES 2.0";
}

void GraphicsHelperES2::depthTest(GLenum mode)
{
    m_funcs->glEnable(GL_DEPTH_TEST);
    m_funcs->glDepthFunc(mode);
}

void GraphicsHelperES2::depthMask(GLenum mode)
{
    m_funcs->glDepthMask(mode);
}

void GraphicsHelperES2::depthRange(GLdouble nearValue, GLdouble farValue)
{
    m_funcs->glDepthRangef(static_cast<float>(nearValue), static_cast<float>(farValue));
}

void GraphicsHelperES2::frontFace(GLenum mode)
{
    m_funcs->glFrontFace(mode);
}

void GraphicsHelperES2::setMSAAEnabled(bool enabled)
{
    Q_UNUSED(enabled);
    static bool showWarning = true;
    if (!showWarning)
        return;
    if (!enabled) {
        showWarning = false;
        qWarning() << "MSAA cannot be disabled with OpenGL ES 2.0";
    }
}

void GraphicsHelperES2::setAlphaCoverageEnabled(bool enabled)
{
    enabled ? m_funcs->glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)
            : m_funcs->glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

GLuint GraphicsHelperES2::createFrameBufferObject()
{
    GLuint id;
    m_funcs->glGenFramebuffers(1, &id);
    return id;
}

void GraphicsHelperES2::releaseFrameBufferObject(GLuint frameBufferId)
{
    m_funcs->glDeleteFramebuffers(1, &frameBufferId);
}

void GraphicsHelperES2::bindFrameBufferObject(GLuint frameBufferId, FBOBindMode mode)
{
    Q_UNUSED(mode);
    // For ES2 the spec states for target: The symbolic constant must be GL_FRAMEBUFFER
    // so mode is ignored and is always set to GL_FRAMEBUFFER
    m_funcs->glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId);
}

void GraphicsHelperES2::bindImageTexture(GLuint imageUnit, GLuint texture,
                                         GLint mipLevel, GLboolean layered,
                                         GLint layer, GLenum access, GLenum format)
{
    Q_UNUSED(imageUnit);
    Q_UNUSED(texture);
    Q_UNUSED(mipLevel);
    Q_UNUSED(layered);
    Q_UNUSED(layer);
    Q_UNUSED(access);
    Q_UNUSED(format);
    qWarning() << "Shader Images are not supported by ES 2.0 (since ES 3.1)";

}

GLuint GraphicsHelperES2::boundFrameBufferObject()
{
    GLint id = 0;
    m_funcs->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);
    return id;
}

bool GraphicsHelperES2::checkFrameBufferComplete()
{
    return (m_funcs->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

bool GraphicsHelperES2::frameBufferNeedsRenderBuffer(const Attachment &attachment)
{
    // Use a renderbuffer for depth or stencil attachments since this is
    // problematic before GLES 3.2. Keep using textures for everything else.
    // For ES2 individual Depth and Stencil buffers need to be an option because
    // DepthStencil is an extension.
    return attachment.m_point == QRenderTargetOutput::DepthStencil ||
           attachment.m_point == QRenderTargetOutput::Depth ||
           attachment.m_point == QRenderTargetOutput::Stencil;
}

void GraphicsHelperES2::bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment)
{
    GLenum attr = GL_COLOR_ATTACHMENT0;

    if (attachment.m_point == QRenderTargetOutput::Color0)
        attr = GL_COLOR_ATTACHMENT0;
    else if (attachment.m_point == QRenderTargetOutput::Depth)
        attr = GL_DEPTH_ATTACHMENT;
    else if (attachment.m_point == QRenderTargetOutput::Stencil)
        attr = GL_STENCIL_ATTACHMENT;
    else
        qCritical() << "Unsupported FBO attachment OpenGL ES 2.0";

    const QOpenGLTexture::Target target = texture->target();

    if (target == QOpenGLTexture::TargetCubeMap && attachment.m_face == QAbstractTexture::AllFaces) {
        qWarning() << "OpenGL ES 2.0 doesn't handle attaching all the faces of a cube map texture at once to an FBO";
        return;
    }

    texture->bind();
    if (target == QOpenGLTexture::Target2D)
        m_funcs->glFramebufferTexture2D(GL_FRAMEBUFFER, attr, target, texture->textureId(), attachment.m_mipLevel);
    else if (target == QOpenGLTexture::TargetCubeMap)
        m_funcs->glFramebufferTexture2D(GL_FRAMEBUFFER, attr, attachment.m_face, texture->textureId(), attachment.m_mipLevel);
    else
        qCritical() << "Unsupported Texture FBO attachment format";
    texture->release();
}

void GraphicsHelperES2::bindFrameBufferAttachment(RenderBuffer *renderBuffer, const Attachment &attachment)
{
    if (attachment.m_point != QRenderTargetOutput::DepthStencil &&
        attachment.m_point != QRenderTargetOutput::Depth &&
        attachment.m_point != QRenderTargetOutput::Stencil) {
        qCritical() << "Renderbuffers only supported for combined depth-stencil, depth, or stencil, but got attachment point"
                    << attachment.m_point;
        return;
    }

    renderBuffer->bind();
    if (attachment.m_point == QRenderTargetOutput::DepthStencil ||
        attachment.m_point == QRenderTargetOutput::Depth)
        m_funcs->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer->renderBufferId());
    if (attachment.m_point == QRenderTargetOutput::DepthStencil ||
        attachment.m_point == QRenderTargetOutput::Stencil)
        m_funcs->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer->renderBufferId());
    renderBuffer->release();
}

bool GraphicsHelperES2::supportsFeature(GraphicsHelperInterface::Feature feature) const
{
    switch (feature) {
    case RenderBufferDimensionRetrieval:
        return true;
    case BlitFramebuffer:
        return m_supportFramebufferBlit;
    default:
        return false;
    }
}

void GraphicsHelperES2::drawBuffers(GLsizei, const int *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "drawBuffers is not supported by ES 2.0";
}

void GraphicsHelperES2::bindFragDataLocation(GLuint , const QHash<QString, int> &)
{
    qCritical() << "bindFragDataLocation is not supported by ES 2.0";
}

void GraphicsHelperES2::bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    Q_UNUSED(programId);
    Q_UNUSED(uniformBlockIndex);
    Q_UNUSED(uniformBlockBinding);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "UBO are not supported by ES 2.0 (since ES 3.0)";
}

void GraphicsHelperES2::bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding)
{
    Q_UNUSED(programId);
    Q_UNUSED(shaderStorageBlockIndex);
    Q_UNUSED(shaderStorageBlockBinding);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "SSBO are not supported by ES 2.0 (since ES 3.1)";
}

void GraphicsHelperES2::bindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    Q_UNUSED(target);
    Q_UNUSED(index);
    Q_UNUSED(buffer);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "bindBufferBase is not supported by ES 2.0 (since ES 3.0)";
}

void GraphicsHelperES2::buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer)
{
    Q_UNUSED(v);
    Q_UNUSED(description);
    Q_UNUSED(buffer);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "UBO are not supported by ES 2.0 (since ES 3.0)";
}

uint GraphicsHelperES2::uniformByteSize(const ShaderUniform &description)
{
    uint rawByteSize = 0;
    int arrayStride = qMax(description.m_arrayStride, 0);
    int matrixStride = qMax(description.m_matrixStride, 0);

    switch (description.m_type) {

    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
        rawByteSize = 8;
        break;

    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
        rawByteSize = 12;
        break;

    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
        rawByteSize = 16;
        break;

    case GL_FLOAT_MAT2:
        rawByteSize = matrixStride ? 2 * matrixStride : 16;
        break;

    case GL_FLOAT_MAT3:
        rawByteSize = matrixStride ? 3 * matrixStride : 36;
        break;

    case GL_FLOAT_MAT4:
        rawByteSize = matrixStride ? 4 * matrixStride : 64;
        break;

    case GL_BOOL:
        rawByteSize = 1;
        break;

    case GL_BOOL_VEC2:
        rawByteSize = 2;
        break;

    case GL_BOOL_VEC3:
        rawByteSize = 3;
        break;

    case GL_BOOL_VEC4:
        rawByteSize = 4;
        break;

    case GL_INT:
    case GL_FLOAT:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE:
        rawByteSize = 4;
        break;
    }

    return arrayStride ? rawByteSize * arrayStride : rawByteSize;
}

void GraphicsHelperES2::enableClipPlane(int)
{
}

void GraphicsHelperES2::disableClipPlane(int)
{
}

void GraphicsHelperES2::setClipPlane(int, const QVector3D &, float)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "Clip planes not supported by OpenGL ES 2.0";
}

GLint GraphicsHelperES2::maxClipPlaneCount()
{
    return 0;
}

void GraphicsHelperES2::memoryBarrier(QMemoryBarrier::Operations barriers)
{
    Q_UNUSED(barriers);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "memory barrier is not supported by OpenGL ES 2.0 (since 4.3)";
}

void GraphicsHelperES2::enablePrimitiveRestart(int)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "primitive restart is not supported by OpenGL ES 2.0 (since GL 3.1, ES 3.0)";
}

void GraphicsHelperES2::enableVertexAttributeArray(int location)
{
    m_funcs->glEnableVertexAttribArray(location);
}

void GraphicsHelperES2::disablePrimitiveRestart()
{
}

void GraphicsHelperES2::clearBufferf(GLint drawbuffer, const QVector4D &values)
{
    Q_UNUSED(drawbuffer);
    Q_UNUSED(values);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glClearBuffer*() not supported by OpenGL ES 2.0";
}

void GraphicsHelperES2::pointSize(bool programmable, GLfloat value)
{
    // If this is not a reset to default values, print a warning
    if (programmable || !qFuzzyCompare(value, 1.0f)) {
        static bool warned = false;
        if (!warned) {
            qWarning() << "glPointSize() and GL_PROGRAM_POINT_SIZE are not supported by ES 2.0";
            warned = true;
        }
    }
}

void GraphicsHelperES2::enablei(GLenum cap, GLuint index)
{
    Q_UNUSED(cap);
    Q_UNUSED(index);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glEnablei() not supported by OpenGL ES 2.0";
}

void GraphicsHelperES2::disablei(GLenum cap, GLuint index)
{
    Q_UNUSED(cap);
    Q_UNUSED(index);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glDisablei() not supported by OpenGL ES 2.0";
}

void GraphicsHelperES2::setSeamlessCubemap(bool enable)
{
    Q_UNUSED(enable);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "GL_TEXTURE_CUBE_MAP_SEAMLESS not supported by OpenGL ES 2.0";
}

QSize GraphicsHelperES2::getRenderBufferDimensions(GLuint renderBufferId)
{
    GLint width = 0;
    GLint height = 0;

    m_funcs->glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
    m_funcs->glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    m_funcs->glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    m_funcs->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return QSize(width, height);
}

QSize GraphicsHelperES2::getTextureDimensions(GLuint textureId, GLenum target, uint level)
{
    Q_UNUSED(textureId);
    Q_UNUSED(target);
    Q_UNUSED(level);
    qCritical() << "getTextureDimensions is not supported by ES 2.0";
    return QSize(0, 0);
}

void GraphicsHelperES2::dispatchCompute(GLuint wx, GLuint wy, GLuint wz)
{
    Q_UNUSED(wx);
    Q_UNUSED(wy);
    Q_UNUSED(wz);
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "Compute Shaders are not supported by ES 2.0 (since ES 3.1)";
}

char *GraphicsHelperES2::mapBuffer(GLenum target, GLsizeiptr size)
{
    Q_UNUSED(target);
    Q_UNUSED(size);
    static bool showWarning = true;
    if (!showWarning)
        return nullptr;
    showWarning = false;
    qWarning() << "Map buffer is not a core requirement for ES 2.0";
    return nullptr;
}

GLboolean GraphicsHelperES2::unmapBuffer(GLenum target)
{
    Q_UNUSED(target);
    static bool showWarning = true;
    if (!showWarning)
        return false;
    showWarning = false;
    qWarning() << "unMap buffer is not a core requirement for ES 2.0";
    return false;
}

void GraphicsHelperES2::glUniform1fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform1fv(location, count, values);
}

void GraphicsHelperES2::glUniform2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform2fv(location, count, values);
}

void GraphicsHelperES2::glUniform3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform3fv(location, count, values);
}

void GraphicsHelperES2::glUniform4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform4fv(location, count, values);
}

void GraphicsHelperES2::glUniform1iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform1iv(location, count, values);
}

void GraphicsHelperES2::glUniform2iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform2iv(location, count, values);
}

void GraphicsHelperES2::glUniform3iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform3iv(location, count, values);
}

void GraphicsHelperES2::glUniform4iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform4iv(location, count, values);
}

void GraphicsHelperES2::glUniform1uiv(GLint , GLsizei , const GLuint *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniform1uiv not supported by ES 2";
}

void GraphicsHelperES2::glUniform2uiv(GLint , GLsizei , const GLuint *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniform2uiv not supported by ES 2";
}

void GraphicsHelperES2::glUniform3uiv(GLint , GLsizei , const GLuint *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniform3uiv not supported by ES 2";
}

void GraphicsHelperES2::glUniform4uiv(GLint , GLsizei , const GLuint *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniform4uiv not supported by ES 2";
}

void GraphicsHelperES2::glUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix2fv(location, count, false, values);
}

void GraphicsHelperES2::glUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix3fv(location, count, false, values);
}

void GraphicsHelperES2::glUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix4fv(location, count, false, values);
}

void GraphicsHelperES2::glUniformMatrix2x3fv(GLint , GLsizei , const GLfloat *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniformMatrix2x3fv not supported by ES 2";
}

void GraphicsHelperES2::glUniformMatrix3x2fv(GLint , GLsizei , const GLfloat *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniformMatrix3x2fv not supported by ES 2";
}

void GraphicsHelperES2::glUniformMatrix2x4fv(GLint , GLsizei , const GLfloat *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniformMatrix2x4fv not supported by ES 2";
}

void GraphicsHelperES2::glUniformMatrix4x2fv(GLint , GLsizei , const GLfloat *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniformMatrix4x2fv not supported by ES 2";
}

void GraphicsHelperES2::glUniformMatrix3x4fv(GLint , GLsizei , const GLfloat *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniformMatrix3x4fv not supported by ES 2";
}

void GraphicsHelperES2::glUniformMatrix4x3fv(GLint , GLsizei , const GLfloat *)
{
    static bool showWarning = true;
    if (!showWarning)
        return;
    showWarning = false;
    qWarning() << "glUniformMatrix4x3fv not supported by ES 2";
}

UniformType GraphicsHelperES2::uniformTypeFromGLType(GLenum type)
{
    switch (type) {
    case GL_FLOAT:
        return UniformType::Float;
    case GL_FLOAT_VEC2:
        return UniformType::Vec2;
    case GL_FLOAT_VEC3:
        return UniformType::Vec3;
    case GL_FLOAT_VEC4:
        return UniformType::Vec4;
    case GL_FLOAT_MAT2:
        return UniformType::Mat2;
    case GL_FLOAT_MAT3:
        return UniformType::Mat3;
    case GL_FLOAT_MAT4:
        return UniformType::Mat4;
    case GL_INT:
        return UniformType::Int;
    case GL_INT_VEC2:
        return UniformType::IVec2;
    case GL_INT_VEC3:
        return UniformType::IVec3;
    case GL_INT_VEC4:
        return UniformType::IVec4;
    case GL_BOOL:
        return UniformType::Bool;
    case GL_BOOL_VEC2:
        return UniformType::BVec2;
    case GL_BOOL_VEC3:
        return UniformType::BVec3;
    case GL_BOOL_VEC4:
        return UniformType::BVec4;

    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE:
        return UniformType::Sampler;
    default:
        Q_UNREACHABLE_RETURN(UniformType::Float);
    }
}

void GraphicsHelperES2::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    if (!m_supportFramebufferBlit) {
        static bool showWarning = true;
        if (!showWarning)
            return;
        showWarning = false;
        qWarning() << "Framebuffer blits are not supported by ES 2.0 (since ES 3.1)";
    } else
        m_ext->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
