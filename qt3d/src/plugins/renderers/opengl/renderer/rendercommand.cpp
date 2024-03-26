// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "rendercommand_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

RenderCommand::RenderCommand()
    : m_glShader(nullptr)
    , m_stateSet(nullptr)
    , m_depth(0.0f)
    , m_changeCost(0)
    , m_type(RenderCommand::Draw)
    , m_primitiveCount(0)
    , m_primitiveType(QGeometryRenderer::Triangles)
    , m_restartIndexValue(-1)
    , m_firstInstance(0)
    , m_firstVertex(0)
    , m_verticesPerPatch(0)
    , m_instanceCount(0)
    , m_indexOffset(0)
    , m_indexAttributeByteOffset(0)
    , m_indexAttributeDataType(GL_UNSIGNED_SHORT)
    , m_indirectAttributeByteOffset(0)
    , m_drawIndexed(false)
    , m_drawIndirect(false)
    , m_primitiveRestartEnabled(false)
    , m_isValid(false)
{
   m_workGroups[0] = 0;
   m_workGroups[1] = 0;
   m_workGroups[2] = 0;
}

bool operator==(const RenderCommand &a, const RenderCommand &b) noexcept
{
    return (a.m_vao == b.m_vao && a.m_glShader == b.m_glShader && a.m_material == b.m_material &&
            a.m_stateSet == b.m_stateSet && a.m_geometry == b.m_geometry && a.m_geometryRenderer == b.m_geometryRenderer &&
            a.m_indirectDrawBuffer == b.m_indirectDrawBuffer && a.m_activeAttributes == b.m_activeAttributes &&
            a.m_depth == b.m_depth && a.m_changeCost == b.m_changeCost && a.m_shaderId == b.m_shaderId &&
            a.m_workGroups[0] == b.m_workGroups[0] && a.m_workGroups[1] == b.m_workGroups[1] && a.m_workGroups[2] == b.m_workGroups[2] &&
            a.m_primitiveCount == b.m_primitiveCount && a.m_primitiveType == b.m_primitiveType && a.m_restartIndexValue == b.m_restartIndexValue &&
            a.m_firstInstance == b.m_firstInstance && a.m_firstVertex == b.m_firstVertex && a.m_verticesPerPatch == b.m_verticesPerPatch &&
            a.m_instanceCount == b.m_instanceCount && a.m_indexOffset == b.m_indexOffset && a.m_indexAttributeByteOffset == b.m_indexAttributeByteOffset &&
            a.m_drawIndexed == b.m_drawIndexed && a.m_drawIndirect == b.m_drawIndirect && a.m_primitiveRestartEnabled == b.m_primitiveRestartEnabled &&
            a.m_isValid == b.m_isValid && a.m_computeCommand == b.m_computeCommand);
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
