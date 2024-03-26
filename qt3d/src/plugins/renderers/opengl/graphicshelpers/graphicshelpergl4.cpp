// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "graphicshelpergl4_p.h"

#if !QT_CONFIG(opengles2)
#include <QOpenGLFunctions_4_3_Core>
#include <private/attachmentpack_p.h>
#include <qgraphicsutils_p.h>
#include <logging_p.h>

# ifndef QT_OPENGL_4_3
#  ifndef GL_PATCH_VERTICES
#    define GL_PATCH_VERTICES 36466
#  endif
#  define GL_ACTIVE_RESOURCES 0x92F5
#  define GL_BUFFER_BINDING 0x9302
#  define GL_BUFFER_DATA_SIZE 0x9303
#  define GL_NUM_ACTIVE_VARIABLES 0x9304
#  define GL_SHADER_STORAGE_BLOCK 0x92E6
#  define GL_UNIFORM 0x92E1
#  define GL_UNIFORM_BLOCK 0x92E2
#  define GL_UNIFORM_BLOCK_INDEX 0x8A3A
#  define GL_UNIFORM_OFFSET 0x8A3B
#  define GL_UNIFORM_ARRAY_STRIDE 0x8A3C
#  define GL_UNIFORM_MATRIX_STRIDE 0x8A3D
#  define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS 0x8A42
#  define GL_UNIFORM_BLOCK_BINDING 0x8A3F
#  define GL_UNIFORM_BLOCK_DATA_SIZE 0x8A40
#  define GL_ALL_BARRIER_BITS 0xFFFFFFFF
#  define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#  define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#  define GL_UNIFORM_BARRIER_BIT 0x00000004
#  define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#  define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#  define GL_COMMAND_BARRIER_BIT 0x00000040
#  define GL_PIXEL_BUFFER_BARRIER_BIT 0x00000080
#  define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#  define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#  define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#  define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#  define GL_ATOMIC_COUNTER_BARRIER_BIT 0x00001000
#  define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#  define GL_QUERY_BUFFER_BARRIER_BIT 0x00008000
#  define GL_IMAGE_1D                       0x904C
#  define GL_IMAGE_2D                       0x904D
#  define GL_IMAGE_3D                       0x904E
#  define GL_IMAGE_2D_RECT                  0x904F
#  define GL_IMAGE_CUBE                     0x9050
#  define GL_IMAGE_BUFFER                   0x9051
#  define GL_IMAGE_1D_ARRAY                 0x9052
#  define GL_IMAGE_2D_ARRAY                 0x9053
#  define GL_IMAGE_CUBE_MAP_ARRAY           0x9054
#  define GL_IMAGE_2D_MULTISAMPLE           0x9055
#  define GL_IMAGE_2D_MULTISAMPLE_ARRAY     0x9056
#  define GL_INT_IMAGE_1D                   0x9057
#  define GL_INT_IMAGE_2D                   0x9058
#  define GL_INT_IMAGE_3D                   0x9059
#  define GL_INT_IMAGE_2D_RECT              0x905A
#  define GL_INT_IMAGE_CUBE                 0x905B
#  define GL_INT_IMAGE_BUFFER               0x905C
#  define GL_INT_IMAGE_1D_ARRAY             0x905D
#  define GL_INT_IMAGE_2D_ARRAY             0x905E
#  define GL_INT_IMAGE_CUBE_MAP_ARRAY       0x905F
#  define GL_INT_IMAGE_2D_MULTISAMPLE       0x9060
#  define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x9061
#  define GL_UNSIGNED_INT_IMAGE_1D          0x9062
#  define GL_UNSIGNED_INT_IMAGE_2D          0x9063
#  define GL_UNSIGNED_INT_IMAGE_3D          0x9064
#  define GL_UNSIGNED_INT_IMAGE_2D_RECT     0x9065
#  define GL_UNSIGNED_INT_IMAGE_CUBE        0x9066
#  define GL_UNSIGNED_INT_IMAGE_BUFFER      0x9067
#  define GL_UNSIGNED_INT_IMAGE_1D_ARRAY    0x9068
#  define GL_UNSIGNED_INT_IMAGE_2D_ARRAY    0x9069
#  define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#  define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE 0x906B
#  define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x906C
# endif

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

namespace {

GLbitfield memoryBarrierGL4Bitfield(QMemoryBarrier::Operations barriers)
{
    GLbitfield bits = 0;

    if (barriers.testFlag(QMemoryBarrier::All)) {
        bits |= GL_ALL_BARRIER_BITS;
        return bits;
    }

    if (barriers.testFlag(QMemoryBarrier::VertexAttributeArray))
        bits |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::ElementArray))
        bits |= GL_ELEMENT_ARRAY_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::Uniform))
        bits |= GL_UNIFORM_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::TextureFetch))
        bits |= GL_TEXTURE_FETCH_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::ShaderImageAccess))
        bits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::Command))
        bits |= GL_COMMAND_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::PixelBuffer))
        bits |= GL_PIXEL_BUFFER_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::TextureUpdate))
        bits |= GL_TEXTURE_UPDATE_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::BufferUpdate))
        bits |= GL_BUFFER_UPDATE_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::FrameBuffer))
        bits |= GL_FRAMEBUFFER_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::TransformFeedback))
        bits |= GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::AtomicCounter))
        bits |= GL_ATOMIC_COUNTER_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::ShaderStorage))
        bits |= GL_SHADER_STORAGE_BARRIER_BIT;
    if (barriers.testFlag(QMemoryBarrier::QueryBuffer))
        bits |= GL_QUERY_BUFFER_BARRIER_BIT;

    return bits;
}

}

GraphicsHelperGL4::GraphicsHelperGL4()
    : m_funcs(nullptr)
{
}

void GraphicsHelperGL4::initializeHelper(QOpenGLContext *context,
                                         QAbstractOpenGLFunctions *functions)
{
    Q_UNUSED(context);
    m_funcs = static_cast<QOpenGLFunctions_4_3_Core*>(functions);
    const bool ok = m_funcs->initializeOpenGLFunctions();
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void GraphicsHelperGL4::drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType,
                                                                    GLsizei primitiveCount,
                                                                    GLint indexType,
                                                                    void *indices,
                                                                    GLsizei instances,
                                                                    GLint baseVertex,
                                                                    GLint baseInstance)
{
    m_funcs->glDrawElementsInstancedBaseVertexBaseInstance(primitiveType,
                                               primitiveCount,
                                               indexType,
                                               indices,
                                               instances,
                                               baseVertex,
                                               baseInstance);
}

void GraphicsHelperGL4::drawArraysInstanced(GLenum primitiveType,
                                            GLint first,
                                            GLsizei count,
                                            GLsizei instances)
{
    // glDrawArraysInstanced OpenGL 3.1 or greater
    m_funcs->glDrawArraysInstanced(primitiveType,
                                   first,
                                   count,
                                   instances);
}

void GraphicsHelperGL4::drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseInstance)
{
    m_funcs->glDrawArraysInstancedBaseInstance(primitiveType,
                                               first,
                                               count,
                                               instances,
                                               baseInstance);
}

void GraphicsHelperGL4::drawElements(GLenum primitiveType,
                                     GLsizei primitiveCount,
                                     GLint indexType,
                                     void *indices,
                                     GLint baseVertex)
{
    m_funcs->glDrawElementsBaseVertex(primitiveType,
                                      primitiveCount,
                                      indexType,
                                      indices,
                                      baseVertex);
}

void GraphicsHelperGL4::drawElementsIndirect(GLenum mode,
                                             GLenum type,
                                             void *indirect)
{
    m_funcs->glDrawElementsIndirect(mode, type, indirect);
}

void GraphicsHelperGL4::drawArrays(GLenum primitiveType,
                                   GLint first,
                                   GLsizei count)
{
    m_funcs->glDrawArrays(primitiveType,
                          first,
                          count);
}

void GraphicsHelperGL4::drawArraysIndirect(GLenum mode, void *indirect)
{
    m_funcs->glDrawArraysIndirect(mode, indirect);
}

void GraphicsHelperGL4::setVerticesPerPatch(GLint verticesPerPatch)
{
    m_funcs->glPatchParameteri(GL_PATCH_VERTICES, verticesPerPatch);
}

void GraphicsHelperGL4::useProgram(GLuint programId)
{
    m_funcs->glUseProgram(programId);
}

std::vector<ShaderUniform> GraphicsHelperGL4::programUniformsAndLocations(GLuint programId)
{
    std::vector<ShaderUniform> uniforms;

    GLint nbrActiveUniforms = 0;
    m_funcs->glGetProgramInterfaceiv(programId, GL_UNIFORM, GL_ACTIVE_RESOURCES, &nbrActiveUniforms);
    uniforms.reserve(nbrActiveUniforms);
    char uniformName[256];
    for (GLint i = 0; i < nbrActiveUniforms; ++i) {
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
        m_funcs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_BLOCK_INDEX, &uniform.m_blockIndex);
        m_funcs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_OFFSET, &uniform.m_offset);
        m_funcs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_ARRAY_STRIDE, &uniform.m_arrayStride);
        m_funcs->glGetActiveUniformsiv(programId, 1, (GLuint*)&i, GL_UNIFORM_MATRIX_STRIDE, &uniform.m_matrixStride);
        uniform.m_rawByteSize = uniformByteSize(uniform);
        uniforms.push_back(uniform);
        qCDebug(Rendering) << uniform.m_name << "size" << uniform.m_size
                           << " offset" << uniform.m_offset
                           << " rawSize" << uniform.m_rawByteSize;
    }

    return uniforms;
}

std::vector<ShaderAttribute> GraphicsHelperGL4::programAttributesAndLocations(GLuint programId)
{
    std::vector<ShaderAttribute> attributes;
    GLint nbrActiveAttributes = 0;
    m_funcs->glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTES, &nbrActiveAttributes);
    attributes.reserve(nbrActiveAttributes);
    char attributeName[256];
    for (GLint i = 0; i < nbrActiveAttributes; ++i) {
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

std::vector<ShaderUniformBlock> GraphicsHelperGL4::programUniformBlocks(GLuint programId)
{
    std::vector<ShaderUniformBlock> blocks;
    GLint nbrActiveUniformsBlocks = 0;
    m_funcs->glGetProgramInterfaceiv(programId, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &nbrActiveUniformsBlocks);
    blocks.reserve(nbrActiveUniformsBlocks);
    for (GLint i = 0; i < nbrActiveUniformsBlocks; ++i) {
        QByteArray uniformBlockName(256, '\0');
        GLsizei length = 0;
        ShaderUniformBlock uniformBlock;
        m_funcs->glGetProgramResourceName(programId, GL_UNIFORM_BLOCK, i, 256, &length, uniformBlockName.data());
        uniformBlock.m_name = QString::fromUtf8(uniformBlockName.left(length));
        uniformBlock.m_index = i;
        GLenum prop = GL_BUFFER_BINDING;
        m_funcs->glGetProgramResourceiv(programId, GL_UNIFORM_BLOCK, i, 1, &prop, 4, NULL, &uniformBlock.m_binding);
        prop = GL_BUFFER_DATA_SIZE;
        m_funcs->glGetProgramResourceiv(programId, GL_UNIFORM_BLOCK, i, 1, &prop, 4, NULL, &uniformBlock.m_size);
        prop = GL_NUM_ACTIVE_VARIABLES;
        m_funcs->glGetProgramResourceiv(programId, GL_UNIFORM_BLOCK, i, 1, &prop, 4, NULL, &uniformBlock.m_activeUniformsCount);
        blocks.push_back(uniformBlock);
    }
    return blocks;
}

std::vector<ShaderStorageBlock> GraphicsHelperGL4::programShaderStorageBlocks(GLuint programId)
{
    std::vector<ShaderStorageBlock> blocks;
    GLint nbrActiveShaderStorageBlocks = 0;
    m_funcs->glGetProgramInterfaceiv(programId, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &nbrActiveShaderStorageBlocks);
    blocks.reserve(nbrActiveShaderStorageBlocks);
    for (GLint i = 0; i < nbrActiveShaderStorageBlocks; ++i) {
        QByteArray storageBlockName(256, '\0');
        GLsizei length = 0;
        ShaderStorageBlock storageBlock;
        m_funcs->glGetProgramResourceName(programId, GL_SHADER_STORAGE_BLOCK, i, 256, &length, storageBlockName.data());
        storageBlock.m_index = i;
        storageBlock.m_name = QString::fromUtf8(storageBlockName.left(length));
        GLenum prop = GL_BUFFER_BINDING;
        m_funcs->glGetProgramResourceiv(programId, GL_SHADER_STORAGE_BLOCK, i, 1, &prop, 4, NULL, &storageBlock.m_binding);
        prop = GL_BUFFER_DATA_SIZE;
        m_funcs->glGetProgramResourceiv(programId, GL_SHADER_STORAGE_BLOCK, i, 1, &prop, 4, NULL, &storageBlock.m_size);
        prop = GL_NUM_ACTIVE_VARIABLES;
        m_funcs->glGetProgramResourceiv(programId, GL_SHADER_STORAGE_BLOCK, i, 1, &prop, 4, NULL, &storageBlock.m_activeVariablesCount);
        blocks.push_back(storageBlock);
    }
    return blocks;
}

void GraphicsHelperGL4::vertexAttribDivisor(GLuint index, GLuint divisor)
{
    m_funcs->glVertexAttribDivisor(index, divisor);
}

void GraphicsHelperGL4::vertexAttributePointer(GLenum shaderDataType,
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
        m_funcs->glVertexAttribIPointer(index, size, type, stride, pointer);
        break;

    case GL_DOUBLE:
    case GL_DOUBLE_VEC2:
    case GL_DOUBLE_VEC3:
    case GL_DOUBLE_VEC4:
        m_funcs->glVertexAttribLPointer(index, size, type, stride, pointer);
        break;

    default:
        qCWarning(Rendering) << "vertexAttribPointer: Unhandled type";
        Q_UNREACHABLE();
    }
}

void GraphicsHelperGL4::readBuffer(GLenum mode)
{
    m_funcs->glReadBuffer(mode);
}

void GraphicsHelperGL4::drawBuffer(GLenum mode)
{
    m_funcs->glDrawBuffer(mode);
}

void *GraphicsHelperGL4::fenceSync()
{
    return m_funcs->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void GraphicsHelperGL4::clientWaitSync(void *sync, GLuint64 nanoSecTimeout)
{
    qDebug() << Q_FUNC_INFO << sync << static_cast<GLsync>(sync);
    GLenum e = m_funcs->glClientWaitSync(static_cast<GLsync>(sync), GL_SYNC_FLUSH_COMMANDS_BIT, nanoSecTimeout);
    qDebug() << e;
}

void GraphicsHelperGL4::waitSync(void *sync)
{
    m_funcs->glWaitSync(static_cast<GLsync>(sync), 0, GL_TIMEOUT_IGNORED);
}

bool GraphicsHelperGL4::wasSyncSignaled(void *sync)
{
    GLint v = 0;
    m_funcs->glGetSynciv(static_cast<GLsync>(sync),
                         GL_SYNC_STATUS,
                         sizeof(v),
                         nullptr,
                         &v);
    return v == GL_SIGNALED;
}

void GraphicsHelperGL4::deleteSync(void *sync)
{
    m_funcs->glDeleteSync(static_cast<GLsync>(sync));
}

void GraphicsHelperGL4::rasterMode(GLenum faceMode, GLenum rasterMode)
{
    m_funcs->glPolygonMode(faceMode, rasterMode);
}

void GraphicsHelperGL4::glUniform1fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform1fv(location, count, values);
}

void GraphicsHelperGL4::glUniform2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform2fv(location, count, values);
}

void GraphicsHelperGL4::glUniform3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform3fv(location, count, values);
}

void GraphicsHelperGL4::glUniform4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniform4fv(location, count, values);
}

void GraphicsHelperGL4::glUniform1iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform1iv(location, count, values);
}

void GraphicsHelperGL4::glUniform2iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform2iv(location, count, values);
}

void GraphicsHelperGL4::glUniform3iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform3iv(location, count, values);
}

void GraphicsHelperGL4::glUniform4iv(GLint location, GLsizei count, const GLint *values)
{
    m_funcs->glUniform4iv(location, count, values);
}

void GraphicsHelperGL4::glUniform1uiv(GLint location, GLsizei count, const GLuint *values)
{
    m_funcs->glUniform1uiv(location, count, values);
}

void GraphicsHelperGL4::glUniform2uiv(GLint location, GLsizei count, const GLuint *values)
{
    m_funcs->glUniform2uiv(location, count, values);
}

void GraphicsHelperGL4::glUniform3uiv(GLint location, GLsizei count, const GLuint *values)
{
    m_funcs->glUniform3uiv(location, count, values);
}

void GraphicsHelperGL4::glUniform4uiv(GLint location, GLsizei count, const GLuint *values)
{
    m_funcs->glUniform4uiv(location, count, values);
}

void GraphicsHelperGL4::glUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix2fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix3fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix4fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix2x3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix2x3fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix3x2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix3x2fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix2x4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix2x4fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix4x2fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix4x2fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix3x4fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix3x4fv(location, count, false, values);
}

void GraphicsHelperGL4::glUniformMatrix4x3fv(GLint location, GLsizei count, const GLfloat *values)
{
    m_funcs->glUniformMatrix4x3fv(location, count, false, values);
}

UniformType GraphicsHelperGL4::uniformTypeFromGLType(GLenum type)
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
    case GL_FLOAT_MAT2x3:
        return UniformType::Mat2x3;
    case GL_FLOAT_MAT3x2:
        return UniformType::Mat3x2;
    case GL_FLOAT_MAT2x4:
        return UniformType::Mat2x4;
    case GL_FLOAT_MAT4x2:
        return UniformType::Mat4x2;
    case GL_FLOAT_MAT3x4:
        return UniformType::Mat3x4;
    case GL_FLOAT_MAT4x3:
        return UniformType::Mat4x3;
    case GL_INT:
        return UniformType::Int;
    case GL_INT_VEC2:
        return UniformType::IVec2;
    case GL_INT_VEC3:
        return UniformType::IVec3;
    case GL_INT_VEC4:
        return UniformType::IVec4;
    case GL_UNSIGNED_INT:
        return UniformType::UInt;
    case GL_UNSIGNED_INT_VEC2:
        return UniformType::UIVec2;
    case GL_UNSIGNED_INT_VEC3:
        return UniformType::UIVec3;
    case GL_UNSIGNED_INT_VEC4:
        return UniformType::UIVec4;
    case GL_BOOL:
        return UniformType::Bool;
    case GL_BOOL_VEC2:
        return UniformType::BVec2;
    case GL_BOOL_VEC3:
        return UniformType::BVec3;
    case GL_BOOL_VEC4:
        return UniformType::BVec4;

    case GL_SAMPLER_1D:
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_1D_ARRAY_SHADOW:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_RECT:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_SHADOW:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_CUBE_MAP_ARRAY:
    case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_MULTISAMPLE:
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_BUFFER:
    case GL_INT_SAMPLER_1D:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_BUFFER:
    case GL_INT_SAMPLER_2D_RECT:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
    case GL_INT_SAMPLER_1D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_1D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
        return UniformType::Sampler;

    case GL_IMAGE_1D:
    case GL_IMAGE_2D:
    case GL_IMAGE_3D:
    case GL_IMAGE_2D_RECT:
    case GL_IMAGE_CUBE:
    case GL_IMAGE_BUFFER:
    case GL_IMAGE_1D_ARRAY:
    case GL_IMAGE_2D_ARRAY:
    case GL_IMAGE_CUBE_MAP_ARRAY:
    case GL_IMAGE_2D_MULTISAMPLE:
    case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_INT_IMAGE_1D:
    case GL_INT_IMAGE_2D:
    case GL_INT_IMAGE_3D:
    case GL_INT_IMAGE_2D_RECT:
    case GL_INT_IMAGE_CUBE:
    case GL_INT_IMAGE_BUFFER:
    case GL_INT_IMAGE_1D_ARRAY:
    case GL_INT_IMAGE_2D_ARRAY:
    case GL_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_INT_IMAGE_2D_MULTISAMPLE:
    case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_1D:
    case GL_UNSIGNED_INT_IMAGE_2D:
    case GL_UNSIGNED_INT_IMAGE_3D:
    case GL_UNSIGNED_INT_IMAGE_2D_RECT:
    case GL_UNSIGNED_INT_IMAGE_CUBE:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
        return UniformType::Image;

    default:
        // TO DO: Add support for Doubles and Images
        Q_UNREACHABLE_RETURN(UniformType::Float);
    }
}

void GraphicsHelperGL4::blendEquation(GLenum mode)
{
    m_funcs->glBlendEquation(mode);
}

void GraphicsHelperGL4::blendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
    m_funcs->glBlendFunci(buf, sfactor, dfactor);
}

void GraphicsHelperGL4::blendFuncSeparatei(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sAlpha, GLenum dAlpha)
{
    m_funcs->glBlendFuncSeparatei(buf, sRGB, dRGB, sAlpha, dAlpha);
}

void GraphicsHelperGL4::alphaTest(GLenum, GLenum)
{
    qCWarning(Rendering) << "AlphaTest not available with OpenGL 3.2 core";
}

void GraphicsHelperGL4::depthTest(GLenum mode)
{
    m_funcs->glEnable(GL_DEPTH_TEST);
    m_funcs->glDepthFunc(mode);
}

void GraphicsHelperGL4::depthMask(GLenum mode)
{
    m_funcs->glDepthMask(mode);
}

void GraphicsHelperGL4::depthRange(GLdouble nearValue, GLdouble farValue)
{
    m_funcs->glDepthRange(nearValue, farValue);
}

void GraphicsHelperGL4::frontFace(GLenum mode)
{
    m_funcs->glFrontFace(mode);

}

void GraphicsHelperGL4::setMSAAEnabled(bool enabled)
{
    enabled ? m_funcs->glEnable(GL_MULTISAMPLE)
            : m_funcs->glDisable(GL_MULTISAMPLE);
}

void GraphicsHelperGL4::setAlphaCoverageEnabled(bool enabled)
{
    enabled ? m_funcs->glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)
            : m_funcs->glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

GLuint GraphicsHelperGL4::createFrameBufferObject()
{
    GLuint id;
    m_funcs->glGenFramebuffers(1, &id);
    return id;
}

void GraphicsHelperGL4::releaseFrameBufferObject(GLuint frameBufferId)
{
    m_funcs->glDeleteFramebuffers(1, &frameBufferId);
}

void GraphicsHelperGL4::bindFrameBufferObject(GLuint frameBufferId, FBOBindMode mode)
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

void GraphicsHelperGL4::bindImageTexture(GLuint imageUnit, GLuint texture,
                                         GLint mipLevel, GLboolean layered,
                                         GLint layer, GLenum access, GLenum format)
{
    m_funcs->glBindImageTexture(imageUnit,
                                texture,
                                mipLevel,
                                layered,
                                layer,
                                access,
                                format);
}

GLuint GraphicsHelperGL4::boundFrameBufferObject()
{
    GLint id = 0;
    m_funcs->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &id);
    return id;
}

bool GraphicsHelperGL4::checkFrameBufferComplete()
{
    return (m_funcs->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

bool GraphicsHelperGL4::frameBufferNeedsRenderBuffer(const Attachment &attachment)
{
    Q_UNUSED(attachment);
    return false;
}

void GraphicsHelperGL4::bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment)
{
    GLenum attr = GL_DEPTH_STENCIL_ATTACHMENT;

    if (attachment.m_point <= QRenderTargetOutput::Color15)
        attr = GL_COLOR_ATTACHMENT0 + attachment.m_point;
    else if (attachment.m_point == QRenderTargetOutput::Depth)
        attr = GL_DEPTH_ATTACHMENT;
    else if (attachment.m_point == QRenderTargetOutput::Stencil)
        attr = GL_STENCIL_ATTACHMENT;

    texture->bind();
    QOpenGLTexture::Target target = texture->target();
    if (target == QOpenGLTexture::Target1DArray || target == QOpenGLTexture::Target2DArray ||
            target == QOpenGLTexture::Target2DMultisampleArray || target == QOpenGLTexture::Target3D)
        m_funcs->glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attr, texture->textureId(), attachment.m_mipLevel, attachment.m_layer);
    else if (target == QOpenGLTexture::TargetCubeMapArray && attachment.m_face != QAbstractTexture::AllFaces)
        m_funcs->glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, attr, texture->textureId(), attachment.m_mipLevel, attachment.m_layer * 6 + (attachment.m_face - QAbstractTexture::CubeMapPositiveX));
    else if (target == QOpenGLTexture::TargetCubeMap && attachment.m_face != QAbstractTexture::AllFaces)
        m_funcs->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attr, attachment.m_face, texture->textureId(), attachment.m_mipLevel);
    else
        m_funcs->glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attr, texture->textureId(), attachment.m_mipLevel);
    texture->release();
}

void GraphicsHelperGL4::bindFrameBufferAttachment(RenderBuffer *renderBuffer, const Attachment &attachment)
{
    Q_UNUSED(renderBuffer);
    Q_UNUSED(attachment);
    Q_UNREACHABLE();
}

bool GraphicsHelperGL4::supportsFeature(GraphicsHelperInterface::Feature feature) const
{
    switch (feature) {
    case MRT:
    case Tessellation:
    case UniformBufferObject:
    case BindableFragmentOutputs:
    case PrimitiveRestart:
    case RenderBufferDimensionRetrieval:
    case TextureDimensionRetrieval:
    case ShaderStorageObject:
    case Compute:
    case DrawBuffersBlend:
    case BlitFramebuffer:
    case IndirectDrawing:
    case MapBuffer:
    case Fences:
    case ShaderImage:
        return true;
    default:
        return false;
    }
}

void GraphicsHelperGL4::drawBuffers(GLsizei n, const int *bufs)
{
    // Use QVarLengthArray here
    QVarLengthArray<GLenum, 16> drawBufs(n);

    for (int i = 0; i < n; i++)
        drawBufs[i] = GL_COLOR_ATTACHMENT0 + bufs[i];
    m_funcs->glDrawBuffers(n, drawBufs.constData());
}

void GraphicsHelperGL4::bindFragDataLocation(GLuint shader, const QHash<QString, int> &outputs)
{
    for (auto it = outputs.begin(), end = outputs.end(); it != end; ++it)
        m_funcs->glBindFragDataLocation(shader, it.value(), it.key().toStdString().c_str());
}

void GraphicsHelperGL4::bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    m_funcs->glUniformBlockBinding(programId, uniformBlockIndex, uniformBlockBinding);
}

void GraphicsHelperGL4::bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding)
{
    m_funcs->glShaderStorageBlockBinding(programId, shaderStorageBlockIndex, shaderStorageBlockBinding);
}

void GraphicsHelperGL4::bindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    m_funcs->glBindBufferBase(target, index, buffer);
}

void GraphicsHelperGL4::buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer)
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

    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_BUFFER:
    case GL_SAMPLER_2D_RECT:
    case GL_INT_SAMPLER_1D:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
    case GL_INT_SAMPLER_BUFFER:
    case GL_INT_SAMPLER_2D_RECT:
    case GL_UNSIGNED_INT_SAMPLER_1D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_CUBE_MAP_ARRAY:
    case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_1D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_1D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_RECT_SHADOW:
    case GL_SAMPLER_2D_MULTISAMPLE:
    case GL_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_IMAGE_1D:
    case GL_IMAGE_2D:
    case GL_IMAGE_3D:
    case GL_IMAGE_2D_RECT:
    case GL_IMAGE_CUBE:
    case GL_IMAGE_BUFFER:
    case GL_IMAGE_1D_ARRAY:
    case GL_IMAGE_2D_ARRAY:
    case GL_IMAGE_CUBE_MAP_ARRAY:
    case GL_IMAGE_2D_MULTISAMPLE:
    case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_INT_IMAGE_1D:
    case GL_INT_IMAGE_2D:
    case GL_INT_IMAGE_3D:
    case GL_INT_IMAGE_2D_RECT:
    case GL_INT_IMAGE_CUBE:
    case GL_INT_IMAGE_BUFFER:
    case GL_INT_IMAGE_1D_ARRAY:
    case GL_INT_IMAGE_2D_ARRAY:
    case GL_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_INT_IMAGE_2D_MULTISAMPLE:
    case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_1D:
    case GL_UNSIGNED_INT_IMAGE_2D:
    case GL_UNSIGNED_INT_IMAGE_3D:
    case GL_UNSIGNED_INT_IMAGE_2D_RECT:
    case GL_UNSIGNED_INT_IMAGE_CUBE:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY: {
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

uint GraphicsHelperGL4::uniformByteSize(const ShaderUniform &description)
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
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_BUFFER:
    case GL_SAMPLER_2D_RECT:
    case GL_INT_SAMPLER_1D:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_BUFFER:
    case GL_INT_SAMPLER_2D_RECT:
    case GL_UNSIGNED_INT_SAMPLER_1D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_CUBE_MAP_ARRAY:
    case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
    case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_1D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_1D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_RECT_SHADOW:
    case GL_SAMPLER_2D_MULTISAMPLE:
    case GL_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_IMAGE_1D:
    case GL_IMAGE_2D:
    case GL_IMAGE_3D:
    case GL_IMAGE_2D_RECT:
    case GL_IMAGE_CUBE:
    case GL_IMAGE_BUFFER:
    case GL_IMAGE_1D_ARRAY:
    case GL_IMAGE_2D_ARRAY:
    case GL_IMAGE_CUBE_MAP_ARRAY:
    case GL_IMAGE_2D_MULTISAMPLE:
    case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_INT_IMAGE_1D:
    case GL_INT_IMAGE_2D:
    case GL_INT_IMAGE_3D:
    case GL_INT_IMAGE_2D_RECT:
    case GL_INT_IMAGE_CUBE:
    case GL_INT_IMAGE_BUFFER:
    case GL_INT_IMAGE_1D_ARRAY:
    case GL_INT_IMAGE_2D_ARRAY:
    case GL_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_INT_IMAGE_2D_MULTISAMPLE:
    case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_1D:
    case GL_UNSIGNED_INT_IMAGE_2D:
    case GL_UNSIGNED_INT_IMAGE_3D:
    case GL_UNSIGNED_INT_IMAGE_2D_RECT:
    case GL_UNSIGNED_INT_IMAGE_CUBE:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
        rawByteSize = 4;
        break;

    default: {
        qWarning() << Q_FUNC_INFO << "unable to deduce rawByteSize for uniform type:" << description.m_type << "for uniform" << description.m_name;
        break;
    }

    }

    return arrayStride ? rawByteSize * arrayStride : rawByteSize;
}

void GraphicsHelperGL4::enableClipPlane(int clipPlane)
{
    m_funcs->glEnable(GL_CLIP_DISTANCE0 + clipPlane);
}

void GraphicsHelperGL4::disableClipPlane(int clipPlane)
{
    m_funcs->glDisable(GL_CLIP_DISTANCE0 + clipPlane);
}

void GraphicsHelperGL4::setClipPlane(int, const QVector3D &, float)
{
    // deprecated
}

GLint GraphicsHelperGL4::maxClipPlaneCount()
{
    GLint max = 0;
    m_funcs->glGetIntegerv(GL_MAX_CLIP_DISTANCES, &max);
    return max;
}

void GraphicsHelperGL4::memoryBarrier(QMemoryBarrier::Operations barriers)
{
    m_funcs->glMemoryBarrier(memoryBarrierGL4Bitfield(barriers));
}

void GraphicsHelperGL4::enablePrimitiveRestart(int primitiveRestartIndex)
{
    m_funcs->glPrimitiveRestartIndex(primitiveRestartIndex);
    m_funcs->glEnable(GL_PRIMITIVE_RESTART);
}

void GraphicsHelperGL4::enableVertexAttributeArray(int location)
{
    m_funcs->glEnableVertexAttribArray(location);
}

void GraphicsHelperGL4::disablePrimitiveRestart()
{
    m_funcs->glDisable(GL_PRIMITIVE_RESTART);
}

void GraphicsHelperGL4::clearBufferf(GLint drawbuffer, const QVector4D &values)
{
    GLfloat vec[4] = {values[0], values[1], values[2], values[3]};
    m_funcs->glClearBufferfv(GL_COLOR, drawbuffer, vec);
}

void GraphicsHelperGL4::pointSize(bool programmable, GLfloat value)
{
    if (programmable) {
        m_funcs->glEnable(GL_PROGRAM_POINT_SIZE);
    } else {
        m_funcs->glDisable(GL_PROGRAM_POINT_SIZE);
        m_funcs->glPointSize(value);
    }
}

void GraphicsHelperGL4::enablei(GLenum cap, GLuint index)
{
    m_funcs->glEnablei(cap, index);
}

void GraphicsHelperGL4::disablei(GLenum cap, GLuint index)
{
    m_funcs->glDisablei(cap, index);
}

void GraphicsHelperGL4::setSeamlessCubemap(bool enable)
{
    if (enable)
        m_funcs->glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    else
        m_funcs->glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

QSize GraphicsHelperGL4::getRenderBufferDimensions(GLuint renderBufferId)
{
    GLint width = 0;
    GLint height = 0;

    m_funcs->glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
    m_funcs->glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    m_funcs->glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    m_funcs->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return QSize(width, height);
}

QSize GraphicsHelperGL4::getTextureDimensions(GLuint textureId, GLenum target, uint level)
{
    GLint width = 0;
    GLint height = 0;

    m_funcs->glBindTexture(target, textureId);
    m_funcs->glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    m_funcs->glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);
    m_funcs->glBindTexture(target, 0);

    return QSize(width, height);
}

void GraphicsHelperGL4::dispatchCompute(GLuint wx, GLuint wy, GLuint wz)
{
    m_funcs->glDispatchCompute(wx, wy, wz);
}

char *GraphicsHelperGL4::mapBuffer(GLenum target, GLsizeiptr size)
{
    return static_cast<char*>(m_funcs->glMapBufferRange(target, 0, size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
}

GLboolean GraphicsHelperGL4::unmapBuffer(GLenum target)
{
    return m_funcs->glUnmapBuffer(target);
}

void GraphicsHelperGL4::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    m_funcs->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // !QT_OPENGL_ES_2
