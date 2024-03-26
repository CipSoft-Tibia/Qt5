// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "graphicshelperes3_2_p.h"
#include <QOpenGLExtraFunctions>
#include <Qt3DRender/qrendertargetoutput.h>
#include <private/attachmentpack_p.h>

QT_BEGIN_NAMESPACE

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif

#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#endif

#ifndef GL_PATCH_VERTICES
#define GL_PATCH_VERTICES 36466
#endif

#ifndef GL_IMAGE_BUFFER
#define GL_IMAGE_BUFFER                   0x9051
#endif
#ifndef GL_IMAGE_CUBE_MAP_ARRAY
#define GL_IMAGE_CUBE_MAP_ARRAY           0x9054
#endif
#ifndef GL_INT_IMAGE_BUFFER
#define GL_INT_IMAGE_BUFFER               0x905C
#endif
#ifndef GL_INT_IMAGE_CUBE_MAP_ARRAY
#define GL_INT_IMAGE_CUBE_MAP_ARRAY       0x905F
#endif
#ifndef GL_UNSIGNED_INT_IMAGE_BUFFER
#define GL_UNSIGNED_INT_IMAGE_BUFFER      0x9067
#endif
#ifndef GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#endif

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

GraphicsHelperES3_2::GraphicsHelperES3_2()
{
}

GraphicsHelperES3_2::~GraphicsHelperES3_2()
{
}

bool GraphicsHelperES3_2::supportsFeature(GraphicsHelperInterface::Feature feature) const
{
    switch (feature) {
    case GraphicsHelperInterface::Tessellation:
        return true;
    default:
        break;
    }
    return GraphicsHelperES3_1::supportsFeature(feature);
}

bool GraphicsHelperES3_2::frameBufferNeedsRenderBuffer(const Attachment &attachment)
{
    Q_UNUSED(attachment);
    // This is first ES version where we have glFramebufferTexture, so
    // attaching a D24S8 texture to the combined depth-stencil attachment point
    // should work.
    return false;
}

void GraphicsHelperES3_2::bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment)
{
    GLenum attr = GL_COLOR_ATTACHMENT0;

    if (attachment.m_point <= QRenderTargetOutput::Color15)
        attr = GL_COLOR_ATTACHMENT0 + attachment.m_point;
    else if (attachment.m_point == QRenderTargetOutput::Depth)
        attr = GL_DEPTH_ATTACHMENT;
    else if (attachment.m_point == QRenderTargetOutput::Stencil)
        attr = GL_STENCIL_ATTACHMENT;
    else if (attachment.m_point == QRenderTargetOutput::DepthStencil)
        attr = GL_DEPTH_STENCIL_ATTACHMENT;
    else
        qCritical() << "Unsupported FBO attachment OpenGL ES 3.2";

    const QOpenGLTexture::Target target = texture->target();

    texture->bind();
    if (target == QOpenGLTexture::TargetCubeMap && attachment.m_face != QAbstractTexture::AllFaces)
        m_funcs->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attr, attachment.m_face, texture->textureId(), attachment.m_mipLevel);
    else
        m_extraFuncs->glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attr, texture->textureId(), attachment.m_mipLevel);
    texture->release();
}

void GraphicsHelperES3_2::setVerticesPerPatch(GLint verticesPerPatch)
{
    m_extraFuncs->glPatchParameteri(GL_PATCH_VERTICES, verticesPerPatch);
}

void GraphicsHelperES3_2::drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType, GLsizei primitiveCount, GLint indexType, void *indices, GLsizei instances, GLint baseVertex, GLint baseInstance)
{
    if (baseInstance != 0)
        qWarning() << "glDrawElementsInstancedBaseVertexBaseInstance is not supported with OpenGL ES 3.2";

    m_extraFuncs->glDrawElementsInstancedBaseVertex(primitiveType,
                                                    primitiveCount,
                                                    indexType,
                                                    indices,
                                                    instances,
                                                    baseVertex);
}

UniformType GraphicsHelperES3_2::uniformTypeFromGLType(GLenum glType)
{
    switch (glType) {
    case GL_IMAGE_BUFFER:
    case GL_IMAGE_CUBE_MAP_ARRAY:
    case GL_INT_IMAGE_BUFFER:
    case GL_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
        return UniformType::Image;

    default:
       return GraphicsHelperES3_1::uniformTypeFromGLType(glType);
    }
}

uint GraphicsHelperES3_2::uniformByteSize(const ShaderUniform &description)
{
    uint rawByteSize = 0;

    switch (description.m_type) {
    case GL_IMAGE_BUFFER:
    case GL_IMAGE_CUBE_MAP_ARRAY:
    case GL_INT_IMAGE_BUFFER:
    case GL_INT_IMAGE_CUBE_MAP_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
        rawByteSize = 4;
        break;

    default:
        rawByteSize = GraphicsHelperES3_1::uniformByteSize(description);
        break;
    }

    return rawByteSize;
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
