/****************************************************************************
**
** Copyright (C) 2020 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "renderviewjobutils_p.h"
#include <Qt3DRender/private/renderlogging_p.h>

#include <Qt3DRender/qgraphicsapifilter.h>
#include <Qt3DRender/private/sphere_p.h>
#include <Qt3DRender/qshaderdata.h>

#include <Qt3DRender/private/cameraselectornode_p.h>
#include <Qt3DRender/private/clearbuffers_p.h>
#include <Qt3DRender/private/layerfilternode_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/effect_p.h>
#include <Qt3DRender/private/renderpassfilternode_p.h>
#include <Qt3DRender/private/rendertargetselectornode_p.h>
#include <Qt3DRender/private/sortpolicy_p.h>
#include <Qt3DRender/private/techniquefilternode_p.h>
#include <Qt3DRender/private/viewportnode_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/shaderdata_p.h>
#include <Qt3DRender/private/statesetnode_p.h>
#include <Qt3DRender/private/dispatchcompute_p.h>
#include <Qt3DRender/private/rendersurfaceselector_p.h>
#include <Qt3DRender/private/rendercapture_p.h>
#include <Qt3DRender/private/buffercapture_p.h>
#include <Qt3DRender/private/stringtoint_p.h>
#include <Qt3DRender/private/techniquemanager_p.h>
#include <Qt3DRender/private/memorybarrier_p.h>
#include <Qt3DRender/private/blitframebuffer_p.h>
#include <Qt3DRender/private/waitfence_p.h>
#include <Qt3DRender/private/renderstateset_p.h>
#include <renderview_p.h>
#include <shadervariables_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {
namespace Rhi {

/*!
    \internal
    Walks up the framegraph tree from \a fgLeaf and builds up as much state
    as possible and populates \a rv. For cases where we can't get the specific state
    (e.g. because it depends upon more than just the framegraph) we store the data from
    the framegraph that will be needed to later when the rest of the data becomes available
*/
void setRenderViewConfigFromFrameGraphLeafNode(RenderView *rv, const FrameGraphNode *fgLeaf)
{
    // The specific RenderPass to be used is also dependent upon the Effect and TechniqueFilter
    // which is referenced by the Material which is referenced by the RenderMesh. So we can
    // only store the filter info in the RenderView structure and use it to do the resolving
    // when we build the RenderCommand list.
    const NodeManagers *manager = rv->nodeManagers();
    const FrameGraphNode *node = fgLeaf;

    while (node) {
        FrameGraphNode::FrameGraphNodeType type = node->nodeType();
        if (node->isEnabled())
            switch (type) {
            case FrameGraphNode::InvalidNodeType:
                // A base FrameGraphNode, can be used for grouping purposes
                break;
            case FrameGraphNode::CameraSelector:
                // Can be set only once and we take camera nearest to the leaf node
                if (!rv->renderCameraLens()) {
                    const CameraSelector *cameraSelector =
                            static_cast<const CameraSelector *>(node);
                    Entity *camNode = manager->renderNodesManager()->lookupResource(
                            cameraSelector->cameraUuid());
                    if (camNode) {
                        CameraLens *lens = camNode->renderComponent<CameraLens>();
                        rv->setRenderCameraEntity(camNode);
                        if (lens && lens->isEnabled()) {
                            rv->setRenderCameraLens(lens);
                            // ViewMatrix and ProjectionMatrix are computed
                            // later in updateMatrices()
                            // since at this point the transformation matrices
                            // may not yet have been updated
                        }
                    }
                }
                break;

            case FrameGraphNode::LayerFilter: // Can be set multiple times in the tree
                rv->appendLayerFilter(static_cast<const LayerFilterNode *>(node)->peerId());
                break;

            case FrameGraphNode::ProximityFilter: // Can be set multiple times in the tree
                rv->appendProximityFilterId(node->peerId());
                break;

            case FrameGraphNode::RenderPassFilter:
                // Can be set once
                // TODO: Amalgamate all render pass filters from leaf to root
                if (!rv->renderPassFilter())
                    rv->setRenderPassFilter(static_cast<const RenderPassFilter *>(node));
                break;

            case FrameGraphNode::RenderTarget: {
                // Can be set once and we take render target nearest to the leaf node
                const RenderTargetSelector *targetSelector =
                        static_cast<const RenderTargetSelector *>(node);
                QNodeId renderTargetUid = targetSelector->renderTargetUuid();
                HTarget renderTargetHandle =
                        manager->renderTargetManager()->lookupHandle(renderTargetUid);

                // Add renderTarget Handle and build renderCommand AttachmentPack
                if (!rv->renderTargetId()) {
                    rv->setRenderTargetId(renderTargetUid);

                    RenderTarget *renderTarget =
                            manager->renderTargetManager()->data(renderTargetHandle);
                    if (renderTarget)
                        rv->setAttachmentPack(AttachmentPack(renderTarget,
                                                             manager->attachmentManager(),
                                                             targetSelector->outputs()));
                }
                break;
            }

            case FrameGraphNode::ClearBuffers: {
                const ClearBuffers *cbNode = static_cast<const ClearBuffers *>(node);
                rv->addClearBuffers(cbNode);
                break;
            }

            case FrameGraphNode::TechniqueFilter:
                // Can be set once
                // TODO Amalgamate all technique filters from leaf to root
                if (!rv->techniqueFilter())
                    rv->setTechniqueFilter(static_cast<const TechniqueFilter *>(node));
                break;

            case FrameGraphNode::Viewport: {
                // If the Viewport has already been set in a lower node
                // Make it so that the new viewport is actually
                // a subregion relative to that of the parent viewport
                const ViewportNode *vpNode = static_cast<const ViewportNode *>(node);
                rv->setViewport(ViewportNode::computeViewport(rv->viewport(), vpNode));
                rv->setGamma(vpNode->gamma());
                break;
            }

            case FrameGraphNode::SortMethod: {
                const Render::SortPolicy *sortPolicy =
                        static_cast<const Render::SortPolicy *>(node);
                rv->addSortType(sortPolicy->sortTypes());
                break;
            }

            case FrameGraphNode::SubtreeEnabler:
                // Has no meaning here. SubtreeEnabler was used
                // in a prior step to filter the list of RenderViewJobs
                break;

            case FrameGraphNode::StateSet: {
                const Render::StateSetNode *rStateSet =
                        static_cast<const Render::StateSetNode *>(node);
                // Create global RenderStateSet for renderView if no stateSet was set before
                RenderStateSet *stateSet = rv->stateSet();
                if (stateSet == nullptr && rStateSet->hasRenderStates()) {
                    stateSet = new RenderStateSet();
                    rv->setStateSet(stateSet);
                }

                // Add states from new stateSet we might be missing
                // but don' t override existing states (lower StateSetNode always has priority)
                if (rStateSet->hasRenderStates())
                    addStatesToRenderStateSet(stateSet, rStateSet->renderStates(),
                                              manager->renderStateManager());
                break;
            }

            case FrameGraphNode::NoDraw: {
                rv->setNoDraw(true);
                break;
            }

            case FrameGraphNode::FrustumCulling: {
                rv->setFrustumCulling(true);
                break;
            }

            case FrameGraphNode::ComputeDispatch: {
                const Render::DispatchCompute *dispatchCompute =
                        static_cast<const Render::DispatchCompute *>(node);
                rv->setCompute(true);
                rv->setComputeWorkgroups(dispatchCompute->x(), dispatchCompute->y(),
                                         dispatchCompute->z());
                break;
            }

            case FrameGraphNode::Lighting: {
                // TODO
                break;
            }

            case FrameGraphNode::Surface: {
                // Use the surface closest to leaf node
                if (rv->surface() == nullptr) {
                    const Render::RenderSurfaceSelector *surfaceSelector =
                            static_cast<const Render::RenderSurfaceSelector *>(node);
                    rv->setSurface(surfaceSelector->surface());
                    rv->setSurfaceSize(surfaceSelector->renderTargetSize()
                                       * surfaceSelector->devicePixelRatio());
                    rv->setDevicePixelRatio(surfaceSelector->devicePixelRatio());
                }
                break;
            }
            case FrameGraphNode::RenderCapture: {
                auto *renderCapture = const_cast<Render::RenderCapture *>(
                        static_cast<const Render::RenderCapture *>(node));
                if (rv->renderCaptureNodeId().isNull() && renderCapture->wasCaptureRequested()) {
                    rv->setRenderCaptureNodeId(renderCapture->peerId());
                    rv->setRenderCaptureRequest(renderCapture->takeCaptureRequest());
                }
                break;
            }

            case FrameGraphNode::MemoryBarrier: {
                // Not available in rhi
                break;
            }

            case FrameGraphNode::BufferCapture: {
                auto *bufferCapture = const_cast<Render::BufferCapture *>(
                        static_cast<const Render::BufferCapture *>(node));
                if (bufferCapture != nullptr)
                    rv->setIsDownloadBuffersEnable(bufferCapture->isEnabled());
                break;
            }

            case FrameGraphNode::BlitFramebuffer: {
                const Render::BlitFramebuffer *blitFramebufferNode =
                        static_cast<const Render::BlitFramebuffer *>(node);
                rv->setHasBlitFramebufferInfo(true);
                BlitFramebufferInfo bfbInfo;
                bfbInfo.sourceRenderTargetId = blitFramebufferNode->sourceRenderTargetId();
                bfbInfo.destinationRenderTargetId =
                        blitFramebufferNode->destinationRenderTargetId();
                bfbInfo.sourceRect = blitFramebufferNode->sourceRect();
                bfbInfo.destinationRect = blitFramebufferNode->destinationRect();
                bfbInfo.sourceAttachmentPoint = blitFramebufferNode->sourceAttachmentPoint();
                bfbInfo.destinationAttachmentPoint =
                        blitFramebufferNode->destinationAttachmentPoint();
                bfbInfo.interpolationMethod = blitFramebufferNode->interpolationMethod();
                rv->setBlitFrameBufferInfo(bfbInfo);
                break;
            }

            case FrameGraphNode::WaitFence: {
                // Not available in rhi
                break;
            }

            case FrameGraphNode::SetFence: {
                // Not available in rhi
                break;
            }

            case FrameGraphNode::NoPicking:
                // Nothing to do RenderView wise for NoPicking
                break;

            default:
                // Should never get here
                qCWarning(Backend) << "Unhandled FrameGraphNode type";
            }

        node = node->parent();
    }
}

/*!
    \internal
    Searches the best matching Technique from \a effect specified.
*/
Technique *findTechniqueForEffect(NodeManagers *manager, const TechniqueFilter *techniqueFilter,
                                  Effect *effect)
{
    if (!effect)
        return nullptr;

    QVector<Technique *> matchingTechniques;
    const bool hasInvalidTechniqueFilter =
            (techniqueFilter == nullptr || techniqueFilter->filters().isEmpty());

    // Iterate through the techniques in the effect
    const auto techniqueIds = effect->techniques();
    for (const QNodeId techniqueId : techniqueIds) {
        Technique *technique = manager->techniqueManager()->lookupResource(techniqueId);

        // Should be valid, if not there likely a problem with node addition/destruction changes
        Q_ASSERT(technique);

        // Check if the technique is compatible with the rendering API
        // If no techniqueFilter is present, we return the technique as it satisfies OpenGL version
        if (technique->isCompatibleWithRenderer()
            && (hasInvalidTechniqueFilter
                || technique->isCompatibleWithFilters(techniqueFilter->filters())))
            matchingTechniques.append(technique);
    }

    if (matchingTechniques.size() == 0) // We failed to find a suitable technique to use :(
        return nullptr;

    if (matchingTechniques.size() == 1)
        return matchingTechniques.first();

    // Several compatible techniques, return technique with highest major and minor version
    Technique *highest = matchingTechniques.first();
    GraphicsApiFilterData filter = *highest->graphicsApiFilter();
    for (auto it = matchingTechniques.cbegin() + 1; it < matchingTechniques.cend(); ++it) {
        if (filter < *(*it)->graphicsApiFilter()) {
            filter = *(*it)->graphicsApiFilter();
            highest = *it;
        }
    }
    return highest;
}

RenderPassList findRenderPassesForTechnique(NodeManagers *manager,
                                            const RenderPassFilter *passFilter,
                                            Technique *technique)
{
    Q_ASSERT(manager);
    Q_ASSERT(technique);

    RenderPassList passes;
    const auto passIds = technique->renderPasses();
    for (const QNodeId passId : passIds) {
        RenderPass *renderPass = manager->renderPassManager()->lookupResource(passId);

        if (renderPass && renderPass->isEnabled()) {
            bool foundMatch = (!passFilter || passFilter->filters().size() == 0);

            // A pass filter is present so we need to check for matching criteria
            if (!foundMatch && renderPass->filterKeys().size() >= passFilter->filters().size()) {

                // Iterate through the filter criteria and look for render passes with criteria that
                // satisfy them
                const auto filterKeyIds = passFilter->filters();
                for (const QNodeId filterKeyId : filterKeyIds) {
                    foundMatch = false;
                    FilterKey *filterFilterKey =
                            manager->filterKeyManager()->lookupResource(filterKeyId);

                    const auto passFilterKeyIds = renderPass->filterKeys();
                    for (const QNodeId passFilterKeyId : passFilterKeyIds) {
                        FilterKey *passFilterKey =
                                manager->filterKeyManager()->lookupResource(passFilterKeyId);
                        if ((foundMatch = (*passFilterKey == *filterFilterKey)))
                            break;
                    }

                    if (!foundMatch) {
                        // No match for criterion in any of the render pass' criteria
                        break;
                    }
                }
            }

            if (foundMatch) {
                // Found a renderpass that satisfies our needs. Add it in order
                passes << renderPass;
            }
        }
    }

    return passes;
}

ParameterInfoList::const_iterator findParamInfo(ParameterInfoList *params, const int nameId)
{
    const ParameterInfoList::const_iterator end = params->cend();
    ParameterInfoList::const_iterator it = std::lower_bound(params->cbegin(), end, nameId);
    if (it != end && it->nameId != nameId)
        return end;
    return it;
}

void addParametersForIds(ParameterInfoList *params, ParameterManager *manager,
                         const Qt3DCore::QNodeIdVector &parameterIds)
{
    for (const QNodeId paramId : parameterIds) {
        const HParameter parameterHandle = manager->lookupHandle(paramId);
        const Parameter *param = manager->data(parameterHandle);
        ParameterInfoList::iterator it =
                std::lower_bound(params->begin(), params->end(), param->nameId());
        if (it == params->end() || it->nameId != param->nameId())
            params->insert(it, ParameterInfo(param->nameId(), parameterHandle));
    }
}

void parametersFromMaterialEffectTechnique(ParameterInfoList *infoList, ParameterManager *manager,
                                           Material *material, Effect *effect, Technique *technique)
{
    // The parameters are taken in the following priority order:
    //
    // 1) Material
    // 2) Effect
    // 3) Technique
    //
    // That way a user can override defaults in Effect's and Techniques on a
    // object manner and a Technique can override global defaults from the Effect.
    parametersFromParametersProvider(infoList, manager, material);
    parametersFromParametersProvider(infoList, manager, effect);
    parametersFromParametersProvider(infoList, manager, technique);
}

// Only add states with types we don't already have
void addStatesToRenderStateSet(RenderStateSet *stateSet, const QVector<Qt3DCore::QNodeId> &stateIds,
                               RenderStateManager *manager)
{
    for (const Qt3DCore::QNodeId &stateId : stateIds) {
        RenderStateNode *node = manager->lookupResource(stateId);
        if (node->isEnabled() && stateSet->canAddStateOfType(node->type())) {
            stateSet->addState(node->impl());
        }
    }
}

namespace {

const QString blockArray = QStringLiteral("[%1]");
const int qNodeIdTypeId = qMetaTypeId<QNodeId>();

}

UniformBlockValueBuilder::UniformBlockValueBuilder()
    : updatedPropertiesOnly(false), shaderDataManager(nullptr), textureManager(nullptr)
{
}

UniformBlockValueBuilder::~UniformBlockValueBuilder() { }

void UniformBlockValueBuilder::buildActiveUniformNameValueMapHelper(
        const ShaderData *currentShaderData, const QString &blockName,
        const QString &qmlPropertyName, const QVariant &value)
{
    // In the end, values are either scalar or a scalar array
    // Composed elements (structs, structs array) are simplified into simple scalars
    if (value.userType() == QMetaType::QVariantList) { // Array
        QVariantList list = value.value<QVariantList>();
        if (list.at(0).userType()
            == qNodeIdTypeId) { // Array of struct qmlPropertyName[i].structMember
            for (int i = 0; i < list.size(); ++i) {
                const QVariant &variantElement = list.at(i);
                if (list.at(i).userType() == qNodeIdTypeId) {
                    const auto nodeId = variantElement.value<QNodeId>();
                    ShaderData *subShaderData = shaderDataManager->lookupResource(nodeId);
                    if (subShaderData) {
                        buildActiveUniformNameValueMapStructHelper(
                                subShaderData,
                                blockName + QLatin1Char('.') + qmlPropertyName + blockArray.arg(i),
                                QLatin1String(""));
                    }
                    // Note: we only handle ShaderData as nested container nodes here
                }
            }
        } else { // Array of scalar/vec  qmlPropertyName[0]
            QString varName;
            varName.reserve(blockName.length() + 1 + qmlPropertyName.length() + 3);
            varName.append(blockName);
            varName.append(QLatin1String("."));
            varName.append(qmlPropertyName);
            varName.append(QLatin1String("[0]"));
            if (uniforms.contains(varName)) {
                qCDebug(Shaders) << "UBO array member " << varName << " set for update";
                activeUniformNamesToValue.insert(StringToInt::lookupId(varName), value);
            }
        }
    } else if (value.userType() == qNodeIdTypeId) { // Struct qmlPropertyName.structMember
        const auto nodeId = value.value<QNodeId>();
        ShaderData *rSubShaderData = shaderDataManager->lookupResource(nodeId);
        if (rSubShaderData) {
            buildActiveUniformNameValueMapStructHelper(rSubShaderData, blockName, qmlPropertyName);
        } else if (textureManager->contains(nodeId)) {
            const auto varId =
                    StringToInt::lookupId(blockName + QLatin1Char('.') + qmlPropertyName);
            activeUniformNamesToValue.insert(varId, value);
        }
    } else { // Scalar / Vec
        QString varName;
        varName.reserve(blockName.length() + 1 + qmlPropertyName.length());
        varName.append(blockName);
        varName.append(QLatin1String("."));
        varName.append(qmlPropertyName);
        if (uniforms.contains(varName)) {
            qCDebug(Shaders) << "UBO scalar member " << varName << " set for update";

            // If the property needs to be transformed, we transform it here as
            // the shaderdata cannot hold transformed properties for multiple
            // thread contexts at once
            activeUniformNamesToValue.insert(
                    StringToInt::lookupId(varName),
                    currentShaderData->getTransformedProperty(qmlPropertyName, viewMatrix));
        }
    }
}

void UniformBlockValueBuilder::buildActiveUniformNameValueMapStructHelper(
        const ShaderData *rShaderData, const QString &blockName, const QString &qmlPropertyName)
{
    const QHash<QString, ShaderData::PropertyValue> &properties = rShaderData->properties();
    auto it = properties.begin();
    const auto end = properties.end();

    while (it != end) {
        QString fullBlockName;
        fullBlockName.reserve(blockName.length() + 1 + qmlPropertyName.length());
        fullBlockName.append(blockName);
        if (!qmlPropertyName.isEmpty()) {
            fullBlockName.append(QLatin1String("."));
            fullBlockName.append(qmlPropertyName);
        }
        buildActiveUniformNameValueMapHelper(rShaderData, fullBlockName, it.key(),
                                             it.value().value);
        ++it;
    }
}

ParameterInfo::ParameterInfo(const int nameId, const HParameter &handle)
    : nameId(nameId), handle(handle)
{
}

bool ParameterInfo::operator<(const ParameterInfo &other) const Q_DECL_NOEXCEPT
{
    return nameId < other.nameId;
}

bool ParameterInfo::operator<(const int otherNameId) const Q_DECL_NOEXCEPT
{
    return nameId < otherNameId;
}

} // namespace Rhi
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
