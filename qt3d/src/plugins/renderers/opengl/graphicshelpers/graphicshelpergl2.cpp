// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "graphicshelpergl2_p.h"
#if !QT_CONFIG(opengles2)
#include <QOpenGLFunctions_2_0>
#include <QOpenGLExtraFunctions>
#include <private/attachmentpack_p.h>
#include <qgraphicsutils_p.h>
#include <logging_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

GraphicsHelperGL2::GraphicsHelperGL2()
    : m_funcs(nullptr)
{

}

void GraphicsHelperGL2::initializeHelper(QOpenGLContext *context,
                                         QAbstractOpenGLFunctions *functions)
{
    Q_UNUSED(context);
    m_funcs = static_cast<QOpenGLFunctions_2_0*>(functions);
    const bool ok = m_funcs->initializeOpenGLFunctions();
    Q_ASSERT(ok);
    Q_UNUSED(ok);
    m_extraFunctions = context->extraFunctions();
    Q_ASSERT(m_extraFunctions);
}

void GraphicsHelperGL2::drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType,
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

void GraphicsHelperGL2::drawArraysInstanced(GLenum primitiveType,
                                            GLint first,
                                            GLsizei count,
                                            GLsizei instances)
{
    for (GLint i = 0; i < instances; i++)
        drawArrays(primitiveType,
                   first,
                   count);
}

void GraphicsHelperGL2::drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseInstance)
{
    if (baseInstance != 0)
        qWarning() << "glDrawArraysInstancedBaseInstance is not supported with OpenGL 2";
    for (GLint i = 0; i < instances; i++)
        drawArrays(primitiveType,
                   first,
                   count);
}

void GraphicsHelperGL2::drawElements(GLenum primitiveType,
                                     GLsizei primitiveCount,
                                     GLint indexType,
                                     void *indices,
                                     GLint baseVertex)
{
    if (baseVertex != 0)
        qWarning() << "glDrawElementsBaseVertex is not supported with OpenGL 2";

    m_funcs->glDrawElements(primitiveType,
                            primitiveCount,
                            indexType,
                            indices);
}

void GraphicsHelperGL2::drawArrays(GLenum primitiveType,
                                   GLint first,
                                   GLsizei count)
{
    m_funcs->glDrawArrays(primitiveType,
                          first,
                          count);
}

void GraphicsHelperGL2::drawElementsIndirect(GLenum, GLenum, void *)
{
    qWarning() << "Indirect Drawing is not supported with OpenGL 2";
}

void GraphicsHelperGL2::drawArraysIndirect(GLenum , void *)
{
    qWarning() << "Indirect Drawing is not supported with OpenGL 2";
}

void GraphicsHelperGL2::setVerticesPerPatch(GLint verticesPerPatch)
{
    Q_UNUSED(verticesPerPatch);
    qWarning() << "Tessellation not supported with OpenGL 2";
}

void GraphicsHelperGL2::useProgram(GLuint programId)
{
    m_funcs->glUseProgram(programId);
}

std::vector<ShaderUniform> GraphicsHelperGL2::programUniformsAndLocations(GLuint programId)
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

std::vector<ShaderAttribute> GraphicsHelperGL2::programAttributesAndLocations(GLuint programId)
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

std::vector<ShaderUniformBlock> GraphicsHelperGL2::programUniformBlocks(GLuint programId)
{
    Q_UNUSED(programId);
    qWarning() << "UBO are not supported by OpenGL 2.0 (since OpenGL 3.1)";
    return {};
}

std::vector<ShaderStorageBlock> GraphicsHelperGL2::programShaderStorageBlocks(GLuint programId)
{
    Q_UNUSED(programId);
    qWarning() << "SSBO are not supported by OpenGL 2.0 (since OpenGL 4.3)";
    return {};
}

void GraphicsHelperGL2::vertexAttribDivisor(GLuint index,
                                            GLuint divisor)
{
    Q_UNUSED(index);
    Q_UNUSED(divisor);
}

void GraphicsHelperGL2::vertexAttributePointer(GLenum shaderDataType,
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
    case GL_FLOAT_MAT2x3:
    case GL_FLOAT_MAT2x4:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT3x2:
    case GL_FLOAT_MAT3x4:
    case GL_FLOAT_MAT4x2:
    case GL_FLOAT_MAT4x3:
    case GL_FLOAT_MAT4:
        m_funcs->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        break;

    default:
        qCWarning(Rendering) << "vertexAttribPointer: Unhandled type";
        Q_UNREACHABLE();
    }
}

void GraphicsHelperGL2::readBuffer(GLenum mode)
{
    m_funcs->glReadBuffer(mode);
}

void GraphicsHelperGL2::drawBuffer(GLenum mode)
{
    m_funcs->glDrawBuffer(mode);
}

void *GraphicsHelperGL2::fenceSync()
{
    qWarning() << "Fences are not supported by OpenGL 2.0 (since OpenGL 3.2)";
    return nullptr;
}

void GraphicsHelperGL2::clientWaitSync(void *, GLuint64 )
{
    qWarning() << "Fences are not supported by OpenGL 2.0 (since OpenGL 3.2)";
}

void GraphicsHelperGL2::waitSync(void *)
{
    qWarning() << "Fences are not supported by OpenGL 2.0 (since OpenGL 3.2)";
}

bool GraphicsHelperGL2::wasSyncSignaled(void *)
{
    qWarning() << "Fences are not supported by OpenGL 2.0 (since OpenGL 3.2)";
    return false;
}

void GraphicsHelperGL2::deleteSync(void *)
{
    qWarning() << "Fences are not supported by OpenGL 2.0 (since OpenGL 3.2)";
}

void GraphicsHelperGL2::rasterMode(GLenum faceMode, GLenum rasterMode)
{
    m_funcs->glPolygonMode(faceMode, rasterMode);
}

void GraphicsHelperGL2::blendEquation(GLenum mode)
{
    m_funcs->glBlendEquation(mode);
}

void GraphicsHelperGL2::blendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
    Q_UNUSED(buf);
    Q_UNUSED(sfactor);
    Q_UNUSED(dfactor);

    qWarning() << "glBlendFunci() not supported by OpenGL 2.0 (since OpenGL 4.0)";
}

void GraphicsHelperGL2::blendFuncSeparatei(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sAlpha, GLenum dAlpha)
{
    Q_UNUSED(buf);
    Q_UNUSED(sRGB);
    Q_UNUSED(dRGB);
    Q_UNUSED(sAlpha);
    Q_UNUSED(dAlpha);

    qWarning() << "glBlendFuncSeparatei() not supported by OpenGL 2.0 (since OpenGL 4.0)";
}

void GraphicsHelperGL2::alphaTest(GLenum mode1, GLenum mode2)
{
    m_funcs->glEnable(GL_ALPHA_TEST);
    m_funcs->glAlphaFunc(mode1, mode2);
}

void GraphicsHelperGL2::depthTest(GLenum mode)
{
    m_funcs->glEnable(GL_DEPTH_TEST);
    m_funcs->glDepthFunc(mode);
}

void GraphicsHelperGL2::depthMask(GLenum mode)
{
    m_funcs->glDepthMask(mode);
}

void GraphicsHelperGL2::depthRange(GLdouble nearValue, GLdouble farValue)
{
    m_funcs->glDepthRange(nearValue, farValue);
}

void GraphicsHelperGL2::frontFace(GLenum mode)
{
    m_funcs->glFrontFace(mode);
}

void GraphicsHelperGL2::setMSAAEnabled(bool enabled)
{
    enabled ? m_funcs->glEnable(GL_MULTISAMPLE)
            : m_funcs->glDisable(GL_MULTISAMPLE);
}

void GraphicsHelperGL2::setAlphaCoverageEnabled(bool enabled)
{
    enabled ? m_funcs->glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)
            : m_funcs->glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

GLuint GraphicsHelperGL2::createFrameBufferObject()
{
    GLuint id;
    m_extraFunctions->glGenFramebuffers(1, &id);
    return id;
}

void GraphicsHelperGL2::releaseFrameBufferObject(GLuint frameBufferId)
{
    m_extraFunctions->glDeleteFramebuffers(1, &frameBufferId);
}

bool GraphicsHelperGL2::checkFrameBufferComplete()
{
    return m_extraFunctions->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool GraphicsHelperGL2::frameBufferNeedsRenderBuffer(const Attachment &attachment)
{
    Q_UNUSED(attachment);
    return false;
}

void GraphicsHelperGL2::bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment)
{
    GLenum attr = GL_DEPTH_STENCIL_ATTACHMENT;

    if (attachment.m_point <= QRenderTargetOutput::Color15)
        attr = GL_COLOR_ATTACHMENT0 + attachment.m_point;
    else if (attachment.m_point == QRenderTargetOutput::Depth)
        attr = GL_DEPTH_ATTACHMENT;
    else if (attachment.m_point == QRenderTargetOutput::Stencil)
        attr = GL_STENCIL_ATTACHMENT;
    else
        qCritical() << "DepthStencil Attachment not supported on OpenGL 2.0";

    const QOpenGLTexture::Target target = texture->target();

    if (target == QOpenGLTexture::TargetCubeMap && attachment.m_face == QAbstractTexture::AllFaces) {
        qWarning() << "OpenGL 2.0 doesn't handle attaching all the faces of a cube map texture at once to an FBO";
        return;
    }

    texture->bind();
    if (target == QOpenGLTexture::TargetCubeMap)
        m_extraFunctions->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attr, attachment.m_face, texture->textureId(), attachment.m_mipLevel);
    else if (target == QOpenGLTexture::Target2D || target == QOpenGLTexture::TargetRectangle)
        m_extraFunctions->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attr, target, texture->textureId(), attachment.m_mipLevel);
    else
        qCritical() << "Texture format not supported for Attachment on OpenGL 2.0";
    texture->release();
}

void GraphicsHelperGL2::bindFrameBufferAttachment(RenderBuffer *renderBuffer, const Attachment &attachment)
{
    Q_UNUSED(renderBuffer);
    Q_UNUSED(attachment);
    Q_UNREACHABLE();
}

bool GraphicsHelperGL2::supportsFeature(GraphicsHelperInterface::Feature feature) const
{
    switch (feature) {
    case MRT:
    case TextureDimensionRetrieval:
    case MapBuffer:
        return true;
    default:
        return false;
    }
}

void GraphicsHelperGL2::drawBuffers(GLsizei n, const int *bufs)
{
    QVarLengthArray<GLenum, 16> drawBufs(n);

    for (int i = 0; i < n; i++)
        drawBufs[i] = GL_COLOR_ATTACHMENT0 + bufs[i];
    m_extraFunctions->glDrawBuffers(n, drawBufs.constData());
}

void GraphicsHelperGL2::bindFragDataLocation(GLuint, const QHash<QString, int> &)
{
    qCritical() << "bindFragDataLocation is not supported by GL 2.0";
}

void GraphicsHelperGL2::bindFrameBufferObject(GLuint frameBufferId, FBOBindMode mode)
{
    switch (mode) {
    case FBODraw:
        m_extraFunctions->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferId);
        return;
    case FBORead:
        m_extraFunctions->glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferId);
        return;
    case FBOReadAndDraw:
    default:
        m_extraFunctions->glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId);
        return;
    }
}

void GraphicsHelperGL2::bindImageTexture(GLuint imageUnit, GLuint texture,
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
    qWarning() << "Shader Images are not supported by OpenGL 2.0 (since OpenGL 4.2)";

}

GLuint GraphicsHelperGL2::boundFrameBufferObject()
{
    GLint id = 0;
    m_extraFunctions->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &id);
    return id;
}

void GraphicsHelperGL2::bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    Q_UNUSED(programId);
    Q_UNUSED(uniformBlockIndex);
    Q_UNUSED(uniformBlockBinding);
    qWarning() << "UBO are not supported by OpenGL 2.0 (since OpenGL 3.1)";
}

void GraphicsHelperGL2::bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding)
{
    Q_UNUSED(programId);
    Q_UNUSED(shaderStorageBlockIndex);
    Q_UNUSED(shaderStorageBlockBinding);
    qWarning() << "SSBO are not supported by OpenGL 2.0 (since OpenGL 4.3)";
}

void GraphicsHelperGL2::bindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    Q_UNUSED(target);
    Q_UNUSED(index);
    Q_UNUSED(buffer);
    qWarning() << "bindBufferBase is not supported by OpenGL 2.0 (since OpenGL 3.0)";
}

void GraphicsHelperGL2::buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer)
{
    Q_UNUSED(v);
    Q_UNUSED(description);
    Q_UNUSED(buffer);
    qWarning() << "UBO are not supported by OpenGL 2.0 (since OpenGL 3.1)";
}

uint GraphicsHelperGL2::uniformByteSize(const ShaderUniform &description)
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

    case GL_FLOAT_MAT2x4:
        rawByteSize = matrixStride ? 2 * matrixStride : 32;
        break;

    case GL_FLOAT_MAT4x2:
        rawByteSize = matrixStride ? 4 * matrixStride : 32;
        break;

    case GL_FLOAT_MAT3:
        rawByteSize = matrixStride ? 3 * matrixStride : 36;
        break;

    case GL_FLOAT_MAT2x3:
        rawByteSize = matrixStride ? 2 * matrixStride : 24;
        break;

    case GL_FLOAT_MAT3x2:
        rawByteSize = matrixStride ? 3 * matrixStride : 24;
        break;

    case GL_FLOAT_MAT4:
        rawByteSize = matrixStride ? 4 * matrixStride : 64;
        break;

    case GL_FLOAT_MAT4x3:
        rawByteSize = matrixStride ? 4 * matrixStride : 48;
        break;

    case GL_FLOAT_MAT3x4:
        rawByteSize = matrixStride ? 3 * matrixStride : 48;
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
    case GL_SAMPLER_1D:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
        rawByteSize = 4;
        break;

    default:
        Q_UNREACHABLE();
    }

    return arrayStride ? rawByteSize * arrayStride : rawByteSize;
}

void GraphicsHelperGL2::enableClipPlane(int clipPlane)
{
    m_funcs->glEnable(GL_CLIP_DISTANCE0 + clipPlane);
}

void GraphicsHelperGL2::disableClipPlane(int clipPlane)
{
    m_funcs->glDisable(GL_CLIP_DISTANCE0 + clipPlane);
}

void GraphicsHelperGL2::setClipPlane(int clipPlane, const QVector3D &normal, float distance)
{
    double plane[4];
    plane[0] = normal.x();
    plane[1] = normal.y();
    plane[2] = normal.z();
    plane[3] = distance;

    m_funcs->glClipPlane(GL_CLIP_PLANE0 + clipPlane, plane);
}

GLint GraphicsHelperGL2::maxClipPlaneCount()
{
    GLint max = 0;
    m_funcs->glGetIntegerv(GL_MAX_CLIP_DISTANCES, &max);
    return max;
}

void GraphicsHelperGL2::memoryBarrier(QMemoryBarrier::Operations barriers)
{
    Q_UNUSED(barriers);
    qWarning() << "memory barrier is not supported by OpenGL 2.0 (since 4.3)";
}

void GraphicsHelperGL2::enablePrimitiveRestart(int)
{
}

void GraphicsHelperGL2::enableVertexAttributeArray(int location)
{
    m_funcs->glEnableVertexAttribArray(location);
}

void GraphicsHelperGL2::disablePrimitiveRestart()
{
}

void GraphicsHelperGL2::clearBufferf(GLint drawbuffer, const QVector4D &values)
{
    Q_UNUSED(drawbuffer);
    Q_UNUSED(values);
    qWarning() << "glClearBuffer*() not supported by OpenGL 2.0";
}

void GraphicsHelperGL2::pointSize(bool programmable, GLfloat value)
{
    m_funcs->glEnable(GL_POINT_SPRITE);
    if (programmable)
        m_funcs->glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    else
        m_funcs->glPointSize(value);
}

void GraphicsHelperGL2::enablei(GLenum cap, GLuint index)
{
    Q_UNUSED(cap);
    Q_UNUSED(index);
    qWarning() << "glEnablei() not supported by OpenGL 2.0 (since 3.0)";
}

void GraphicsHelperGL2::disablei(GLenum cap, GLuint index)
{
    Q_UNUSED(cap);
    Q_UNUSED(index);
    qWarning() << "glDisablei() not supported by OpenGL 2.0 (since 3.0)";
}

void GraphicsHelperGL2::setSeamlessCubemap(bool enable)
{
    Q_UNUSED(enable);
    qWarning() << "GL_TEXTURE_CUBE_MAP_SEAMLESS not supported by OpenGL 2.0 (since 3.2)";
}

QSize GraphicsHelperGL2::getRenderBufferDimensions(GLuint renderBufferId)
{
    Q_UNUSED(renderBufferId);
    qCritical() << "RenderBuffer dimensions retrival not supported on OpenGL 2.0";
    return QSize(0,0);
}

QSize GraphicsHelperGL2::getTextureDimensions(GLuint textureId, GLenum target, uint level)
{
    GLint width = 0;
    GLint height = 0;

    m_funcs->glBindTexture(target, textureId);
    m_funcs->glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    m_funcs->glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);
    m_funcs->glBindTexture(target, 0);

    return QSize(width, height);
}

void GraphicsHelperGL2::dispatchCompute(GLuint wx, GLuint wy, GLuint wz)
{
    Q_UNUSED(wx);
    Q_UNUSED(wy);
    Q_UNUSED(wz);
    qWarning() << "Compute Shaders are not supported by OpenGL 2.0 (since OpenGL 4.3)";
}

char *GraphicsHelperGL2::mapBuffer(GLenum target, GLsizeiptr size)
{
    Q_UNUSED(size);
    return static_cast<char*>(m_funcs->glMapBuffer(target, GL_READ_WRITE));
}

GLboolean GraphicsHelperGL2::unmapBuffer(GLenum target)
{
    return m_funcs->glUnmapBuffer(target);
}

void GraphicsHelperGL2::glUniform1fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform1fv(location, count, values);
}

void GraphicsHelperGL2::glUniform2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform2fv(location, count, values);
}

void GraphicsHelperGL2::glUniform3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform3fv(location, count, values);
}

void GraphicsHelperGL2::glUniform4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform4fv(location, count, values);
}

void GraphicsHelperGL2::glUniform1iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform1iv(location, count, values);
}

void GraphicsHelperGL2::glUniform2iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform2iv(location, count, values);
}

void GraphicsHelperGL2::glUniform3iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform3iv(location, count, values);
}

void GraphicsHelperGL2::glUniform4iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform4iv(location, count, values);
}

void GraphicsHelperGL2::glUniform1uiv(GLint , GLsizei , const GLuint *)
{
    qWarning() << "glUniform1uiv not supported by GL 2";
}

void GraphicsHelperGL2::glUniform2uiv(GLint , GLsizei , const GLuint *)
{
    qWarning() << "glUniform2uiv not supported by GL 2";
}

void GraphicsHelperGL2::glUniform3uiv(GLint , GLsizei , const GLuint *)
{
    qWarning() << "glUniform3uiv not supported by GL 2";
}

void GraphicsHelperGL2::glUniform4uiv(GLint , GLsizei , const GLuint *)
{
    qWarning() << "glUniform4uiv not supported by GL 2";
}

void GraphicsHelperGL2::glUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix2fv(location, count, false, values);
}

void GraphicsHelperGL2::glUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix3fv(location, count, false, values);
}

void GraphicsHelperGL2::glUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix4fv(location, count, false, values);
}

void GraphicsHelperGL2::glUniformMatrix2x3fv(GLint , GLsizei , const GLfloat *)
{
    qWarning() << "glUniformMatrix2x3fv not supported by GL 2";
}

void GraphicsHelperGL2::glUniformMatrix3x2fv(GLint , GLsizei , const GLfloat *)
{
    qWarning() << "glUniformMatrix3x2fv not supported by GL 2";
}

void GraphicsHelperGL2::glUniformMatrix2x4fv(GLint , GLsizei , const GLfloat *)
{
    qWarning() << "glUniformMatrix2x4fv not supported by GL 2";
}

void GraphicsHelperGL2::glUniformMatrix4x2fv(GLint , GLsizei , const GLfloat *)
{
    qWarning() << "glUniformMatrix4x2fv not supported by GL 2";
}

void GraphicsHelperGL2::glUniformMatrix3x4fv(GLint , GLsizei , const GLfloat *)
{
    qWarning() << "glUniformMatrix3x4fv not supported by GL 2";
}

void GraphicsHelperGL2::glUniformMatrix4x3fv(GLint , GLsizei , const GLfloat *)
{
    qWarning() << "glUniformMatrix4x3fv not supported by GL 2";
}

UniformType GraphicsHelperGL2::uniformTypeFromGLType(GLenum type)
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

    case GL_SAMPLER_1D:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_3D:
        return UniformType::Sampler;

    default:
        Q_UNREACHABLE_RETURN(UniformType::Float);
    }
}

void GraphicsHelperGL2::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    Q_UNUSED(srcX0);
    Q_UNUSED(srcX1);
    Q_UNUSED(srcY0);
    Q_UNUSED(srcY1);
    Q_UNUSED(dstX0);
    Q_UNUSED(dstX1);
    Q_UNUSED(dstY0);
    Q_UNUSED(dstY1);
    Q_UNUSED(mask);
    Q_UNUSED(filter);
    qWarning() << "Framebuffer blits are not supported by ES 2.0 (since ES 3.1)";
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // !QT_OPENGL_ES_2
