// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// Copyright (C) 2016 Svenn-Arne Dragly.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "graphicshelperes3_p.h"
#include <private/attachmentpack_p.h>
#include <qgraphicsutils_p.h>
#include <logging_p.h>
#include <QOpenGLExtraFunctions>

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
#ifndef GL_FLOAT_MAT2x3
#define GL_FLOAT_MAT2x3                   0x8B65
#endif
#ifndef GL_FLOAT_MAT2x4
#define GL_FLOAT_MAT2x4                   0x8B66
#endif
#ifndef GL_FLOAT_MAT3x2
#define GL_FLOAT_MAT3x2                   0x8B67
#endif
#ifndef GL_FLOAT_MAT3x4
#define GL_FLOAT_MAT3x4                   0x8B68
#endif
#ifndef GL_FLOAT_MAT4x2
#define GL_FLOAT_MAT4x2                   0x8B69
#endif
#ifndef GL_FLOAT_MAT4x3
#define GL_FLOAT_MAT4x3                   0x8B6A
#endif
#ifndef GL_UNSIGNED_INT_VEC2
#define GL_UNSIGNED_INT_VEC2              0x8DC6
#endif
#ifndef GL_UNSIGNED_INT_VEC3
#define GL_UNSIGNED_INT_VEC3              0x8DC7
#endif
#ifndef GL_UNSIGNED_INT_VEC4
#define GL_UNSIGNED_INT_VEC4              0x8DC8
#endif
#ifndef GL_INT_SAMPLER_2D
#define GL_INT_SAMPLER_2D                 0x8DCA
#endif
#ifndef GL_INT_SAMPLER_3D
#define GL_INT_SAMPLER_3D                 0x8DCB
#endif
#ifndef GL_INT_SAMPLER_CUBE
#define GL_INT_SAMPLER_CUBE               0x8DCC
#endif
#ifndef GL_INT_SAMPLER_2D_ARRAY
#define GL_INT_SAMPLER_2D_ARRAY           0x8DCF
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_2D
#define GL_UNSIGNED_INT_SAMPLER_2D        0x8DD2
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_3D
#define GL_UNSIGNED_INT_SAMPLER_3D        0x8DD3
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_CUBE
#define GL_UNSIGNED_INT_SAMPLER_CUBE      0x8DD4
#endif
#ifndef GL_UNSIGNED_INT_SAMPLER_2D_ARRAY
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY  0x8DD7
#endif

#ifndef GL_ACTIVE_UNIFORM_BLOCKS
#define GL_ACTIVE_UNIFORM_BLOCKS 0x8A36
#endif
#ifndef GL_UNIFORM_BLOCK_INDEX
#define GL_UNIFORM_BLOCK_INDEX 0x8A3A
#endif
#ifndef GL_UNIFORM_OFFSET
#define GL_UNIFORM_OFFSET 0x8A3B
#endif
#ifndef GL_UNIFORM_ARRAY_STRIDE
#define GL_UNIFORM_ARRAY_STRIDE 0x8A3C
#endif
#ifndef GL_UNIFORM_MATRIX_STRIDE
#define GL_UNIFORM_MATRIX_STRIDE 0x8A3D
#endif
#ifndef GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS 0x8A42
#endif
#ifndef GL_UNIFORM_BLOCK_BINDING
#define GL_UNIFORM_BLOCK_BINDING 0x8A3F
#endif
#ifndef GL_UNIFORM_BLOCK_DATA_SIZE
#define GL_UNIFORM_BLOCK_DATA_SIZE 0x8A40
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER               0x8CA8
#endif

#ifndef GL_SIGNALED
#define GL_SIGNALED                       0x9119
#endif

#ifndef GL_SYNC_STATUS
#define GL_SYNC_STATUS                    0x9114
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED                0xFFFFFFFFFFFFFFFFull
#endif

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE     0x9117
#endif

#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
#define GL_SYNC_FLUSH_COMMANDS_BIT        0x00000001
#endif

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

GraphicsHelperES3::GraphicsHelperES3()
{
}

GraphicsHelperES3::~GraphicsHelperES3()
{
}

void GraphicsHelperES3::initializeHelper(QOpenGLContext *context,
                                         QAbstractOpenGLFunctions *functions)
{
    GraphicsHelperES2::initializeHelper(context, functions);
    m_extraFuncs = context->extraFunctions();
    Q_ASSERT(m_extraFuncs);
}

void GraphicsHelperES3::drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType,
                                                                    GLsizei primitiveCount,
                                                                    GLint indexType,
                                                                    void *indices,
                                                                    GLsizei instances,
                                                                    GLint baseVertex,
                                                                    GLint baseInstance)
{
    if (baseInstance != 0)
        qWarning() << "glDrawElementsInstancedBaseVertexBaseInstance is not supported with OpenGL ES 3";

    if (baseVertex != 0)
        qWarning() << "glDrawElementsInstancedBaseVertex is not supported with OpenGL ES 3";

    m_extraFuncs->glDrawElementsInstanced(primitiveType,
                                          primitiveCount,
                                          indexType,
                                          indices,
                                          instances);
}

void GraphicsHelperES3::vertexAttribDivisor(GLuint index, GLuint divisor)
{
    m_extraFuncs->glVertexAttribDivisor(index, divisor);
}

void GraphicsHelperES3::vertexAttributePointer(GLenum shaderDataType,
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

    case GL_INT:
    case GL_INT_VEC2:
    case GL_INT_VEC3:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_VEC2:
    case GL_UNSIGNED_INT_VEC3:
    case GL_UNSIGNED_INT_VEC4:
        m_extraFuncs->glVertexAttribIPointer(index, size, type, stride, pointer);
        break;

    default:
        qCWarning(Rendering) << "vertexAttribPointer: Unhandled type";
        Q_UNREACHABLE();
    }
}

void GraphicsHelperES3::drawArraysInstanced(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances)
{
    m_extraFuncs->glDrawArraysInstanced(primitiveType,
                                        first,
                                        count,
                                        instances);
}

void GraphicsHelperES3::drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseInstance)
{
    if (baseInstance != 0)
        qWarning() << "glDrawElementsInstancedBaseVertexBaseInstance is not supported with OpenGL ES 3";

    m_extraFuncs->glDrawArraysInstanced(primitiveType,
                                        first,
                                        count,
                                        instances);
}

void GraphicsHelperES3::readBuffer(GLenum mode)
{
    m_extraFuncs->glReadBuffer(mode);
}

void GraphicsHelperES3::drawBuffer(GLenum mode)
{
    Q_UNUSED(mode);
    qWarning() << "glDrawBuffer is not supported with OpenGL ES 3";
}

void GraphicsHelperES3::bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment)
{
    GLenum attr = GL_COLOR_ATTACHMENT0;

    if (attachment.m_point <= QRenderTargetOutput::Color15)
        attr = GL_COLOR_ATTACHMENT0 + attachment.m_point;
    else if (attachment.m_point == QRenderTargetOutput::Depth)
        attr = GL_DEPTH_ATTACHMENT;
    else if (attachment.m_point == QRenderTargetOutput::Stencil)
        attr = GL_STENCIL_ATTACHMENT;
    else
        qCritical() << "Unsupported FBO attachment OpenGL ES 3.0";

    const QOpenGLTexture::Target target = texture->target();

    if (target == QOpenGLTexture::TargetCubeMap && attachment.m_face == QAbstractTexture::AllFaces) {
        qWarning() << "OpenGL ES 3.0 doesn't handle attaching all the faces of a cube map texture at once to an FBO";
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

void GraphicsHelperES3::bindFrameBufferObject(GLuint frameBufferId, GraphicsHelperInterface::FBOBindMode mode)
{
    switch (mode) {
    case FBODraw:
        m_funcs->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferId);
        return;
    case FBORead:
        m_funcs->glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferId);
        return;
    case FBOReadAndDraw:
    default:
        m_funcs->glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId);
        return;
    }
}

bool GraphicsHelperES3::supportsFeature(GraphicsHelperInterface::Feature feature) const
{
    switch (feature) {
    case RenderBufferDimensionRetrieval:
    case MRT:
    case BlitFramebuffer:
    case UniformBufferObject:
    case MapBuffer:
    case Fences:
        return true;
    default:
        return false;
    }
}

void GraphicsHelperES3::drawBuffers(GLsizei n, const int *bufs)
{
    QVarLengthArray<GLenum, 16> drawBufs(n);

    for (int i = 0; i < n; i++)
        drawBufs[i] = GL_COLOR_ATTACHMENT0 + bufs[i];
    m_extraFuncs->glDrawBuffers(n, drawBufs.constData());
}

UniformType GraphicsHelperES3::uniformTypeFromGLType(GLenum glType)
{
    switch (glType) {
    case GL_SAMPLER_3D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
        return UniformType::Sampler;
    default:
       return GraphicsHelperES2::uniformTypeFromGLType(glType);
    }
}

uint GraphicsHelperES3::uniformByteSize(const ShaderUniform &description)
{
    uint rawByteSize = 0;
    int arrayStride = qMax(description.m_arrayStride, 0);
    int matrixStride = qMax(description.m_matrixStride, 0);

    switch (description.m_type) {

    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
        rawByteSize = 8;
        break;

    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
        rawByteSize = 12;
        break;

    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
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
    case GL_UNSIGNED_INT:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
        rawByteSize = 4;
        break;
    }

    return arrayStride ? rawByteSize * arrayStride : rawByteSize;
}

void *GraphicsHelperES3::fenceSync()
{
    return m_extraFuncs->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void GraphicsHelperES3::clientWaitSync(void *sync, GLuint64 nanoSecTimeout)
{
    m_extraFuncs->glClientWaitSync(static_cast<GLsync>(sync), GL_SYNC_FLUSH_COMMANDS_BIT, nanoSecTimeout);
}

void GraphicsHelperES3::waitSync(void *sync)
{
    m_extraFuncs->glWaitSync(static_cast<GLsync>(sync), 0, GL_TIMEOUT_IGNORED);
}

bool GraphicsHelperES3::wasSyncSignaled(void *sync)
{
    GLint v;
    m_extraFuncs->glGetSynciv(static_cast<GLsync>(sync),
                         GL_SYNC_STATUS,
                         sizeof(v),
                         nullptr,
                         &v);
    return v == GL_SIGNALED;
}

void GraphicsHelperES3::deleteSync(void *sync)
{
    m_extraFuncs->glDeleteSync(static_cast<GLsync>(sync));
}

void GraphicsHelperES3::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    m_extraFuncs->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void GraphicsHelperES3::bindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    m_extraFuncs->glBindBufferBase(target, index, buffer);
}

bool GraphicsHelperES3::frameBufferNeedsRenderBuffer(const Attachment &attachment)
{
    // Use a renderbuffer for combined depth+stencil attachments since this is
    // problematic before GLES 3.2. Keep using textures for everything else.
    return attachment.m_point == QRenderTargetOutput::DepthStencil;
}

void GraphicsHelperES3::bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    m_extraFuncs->glUniformBlockBinding(programId, uniformBlockIndex, uniformBlockBinding);
}

void GraphicsHelperES3::buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer)
{
    char *bufferData = buffer.data();

    switch (description.m_type) {

    case GL_FLOAT: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 1);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 1);
        break;
    }

    case GL_FLOAT_VEC2: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 2);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 2);
        break;
    }

    case GL_FLOAT_VEC3: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 3);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 3);
        break;
    }

    case GL_FLOAT_VEC4: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 4);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 4);
        break;
    }

    case GL_FLOAT_MAT2: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 4);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 2, 2);
        break;
    }

    case GL_FLOAT_MAT2x3: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 6);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 2, 3);
        break;
    }

    case GL_FLOAT_MAT2x4: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 8);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 2, 4);
        break;
    }

    case GL_FLOAT_MAT3: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 9);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 3, 3);
        break;
    }

    case GL_FLOAT_MAT3x2: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 6);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 3, 2);
        break;
    }

    case GL_FLOAT_MAT3x4: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 12);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 3, 4);
        break;
    }

    case GL_FLOAT_MAT4: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 16);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 4, 4);
        break;
    }

    case GL_FLOAT_MAT4x2: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 8);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 4, 2);
        break;
    }

    case GL_FLOAT_MAT4x3: {
        const GLfloat *data = QGraphicsUtils::valueArrayFromVariant<GLfloat>(v, description.m_size, 12);
        QGraphicsUtils::fillDataMatrixArray(bufferData, data, description, 4, 3);
        break;
    }

    case GL_INT: {
        const GLint *data = QGraphicsUtils::valueArrayFromVariant<GLint>(v, description.m_size, 1);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 1);
        break;
    }

    case GL_INT_VEC2: {
        const GLint *data = QGraphicsUtils::valueArrayFromVariant<GLint>(v, description.m_size, 2);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 2);
        break;
    }

    case GL_INT_VEC3: {
        const GLint *data = QGraphicsUtils::valueArrayFromVariant<GLint>(v, description.m_size, 3);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 3);
        break;
    }

    case GL_INT_VEC4: {
        const GLint *data = QGraphicsUtils::valueArrayFromVariant<GLint>(v, description.m_size, 4);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 4);
        break;
    }

    case GL_UNSIGNED_INT: {
        const GLuint *data = QGraphicsUtils::valueArrayFromVariant<GLuint>(v, description.m_size, 1);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 1);
        break;
    }

    case GL_UNSIGNED_INT_VEC2: {
        const GLuint *data = QGraphicsUtils::valueArrayFromVariant<GLuint>(v, description.m_size, 2);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 2);
        break;
    }

    case GL_UNSIGNED_INT_VEC3: {
        const GLuint *data = QGraphicsUtils::valueArrayFromVariant<GLuint>(v, description.m_size, 3);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 3);
        break;
    }

    case GL_UNSIGNED_INT_VEC4: {
        const GLuint *data = QGraphicsUtils::valueArrayFromVariant<GLuint>(v, description.m_size, 4);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 4);
        break;
    }

    case GL_BOOL: {
        const GLboolean *data = QGraphicsUtils::valueArrayFromVariant<GLboolean>(v, description.m_size, 1);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 1);
        break;
    }

    case GL_BOOL_VEC2: {
        const GLboolean *data = QGraphicsUtils::valueArrayFromVariant<GLboolean>(v, description.m_size, 2);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 2);
        break;
    }

    case GL_BOOL_VEC3: {
        const GLboolean *data = QGraphicsUtils::valueArrayFromVariant<GLboolean>(v, description.m_size, 3);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 3);
        break;
    }

    case GL_BOOL_VEC4: {
        const GLboolean *data = QGraphicsUtils::valueArrayFromVariant<GLboolean>(v, description.m_size, 4);
        QGraphicsUtils::fillDataArray(bufferData, data, description, 4);
        break;
    }

    // note: only GLES 3.0 supported types, not the same as OpenGL proper
    // (also, no MS samplers before ES 3.1)
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    {
        Q_ASSERT(description.m_size == 1);
        int value = v.toInt();
        QGraphicsUtils::fillDataArray<GLint>(bufferData, &value, description, 1);
        break;
    }

    default:
        qWarning() << Q_FUNC_INFO << "unsupported uniform type:" << description.m_type << "for " << description.m_name;
        break;
    }
}

char *GraphicsHelperES3::mapBuffer(GLenum target, GLsizeiptr size)
{
    return static_cast<char*>(m_extraFuncs->glMapBufferRange(target, 0, size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
}

GLboolean GraphicsHelperES3::unmapBuffer(GLenum target)
{
    return m_extraFuncs->glUnmapBuffer(target);
}

std::vector<ShaderUniform> GraphicsHelperES3::programUniformsAndLocations(GLuint programId)
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
        m_extraFuncs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_BLOCK_INDEX, &uniform.m_blockIndex);
        m_extraFuncs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_OFFSET, &uniform.m_offset);
        m_extraFuncs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_ARRAY_STRIDE, &uniform.m_arrayStride);
        m_extraFuncs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_MATRIX_STRIDE, &uniform.m_matrixStride);
        uniform.m_rawByteSize = uniformByteSize(uniform);
        uniforms.push_back(uniform);
        qCDebug(Rendering) << uniform.m_name << "size" << uniform.m_size
                                   << " offset" << uniform.m_offset
                                   << " rawSize" << uniform.m_rawByteSize;
    }

    return uniforms;
}

std::vector<ShaderUniformBlock> GraphicsHelperES3::programUniformBlocks(GLuint programId)
{
    std::vector<ShaderUniformBlock> blocks;
    GLint nbrActiveUniformsBlocks = 0;
    m_extraFuncs->glGetProgramiv(programId, GL_ACTIVE_UNIFORM_BLOCKS, &nbrActiveUniformsBlocks);
    blocks.reserve(nbrActiveUniformsBlocks);
    for (GLint i = 0; i < nbrActiveUniformsBlocks; i++) {
        QByteArray uniformBlockName(256, '\0');
        GLsizei length = 0;
        ShaderUniformBlock uniformBlock;
        m_extraFuncs->glGetActiveUniformBlockName(programId, i, 256, &length, uniformBlockName.data());
        uniformBlock.m_name = QString::fromUtf8(uniformBlockName.left(length));
        uniformBlock.m_index = i;
        m_extraFuncs->glGetActiveUniformBlockiv(programId, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniformBlock.m_activeUniformsCount);
        m_extraFuncs->glGetActiveUniformBlockiv(programId, i, GL_UNIFORM_BLOCK_BINDING, &uniformBlock.m_binding);
        m_extraFuncs->glGetActiveUniformBlockiv(programId, i, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlock.m_size);
        blocks.push_back(uniformBlock);
    }
    return blocks;
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
