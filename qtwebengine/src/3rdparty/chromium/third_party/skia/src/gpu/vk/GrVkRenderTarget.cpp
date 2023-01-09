/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/vk/GrVkRenderTarget.h"

#include "include/gpu/GrBackendSurface.h"
#include "src/gpu/vk/GrVkCommandBuffer.h"
#include "src/gpu/vk/GrVkDescriptorSet.h"
#include "src/gpu/vk/GrVkFramebuffer.h"
#include "src/gpu/vk/GrVkGpu.h"
#include "src/gpu/vk/GrVkImageView.h"
#include "src/gpu/vk/GrVkResourceProvider.h"
#include "src/gpu/vk/GrVkUtil.h"

#include "include/gpu/vk/GrVkTypes.h"

#define VK_CALL(GPU, X) GR_VK_CALL(GPU->vkInterface(), X)

static int renderpass_features_to_index(
        bool hasStencil, GrVkRenderPass::SelfDependencyFlags selfDepFlags) {
    int index = hasStencil ? 1 : 0;
    if (selfDepFlags & GrVkRenderPass::SelfDependencyFlags::kForInputAttachment) {
        index += 2;
    }
    if (selfDepFlags & GrVkRenderPass::SelfDependencyFlags::kForNonCoherentAdvBlend) {
        index += 4;
    }
    return index;
}

// We're virtually derived from GrSurface (via GrRenderTarget) so its
// constructor must be explicitly called.
GrVkRenderTarget::GrVkRenderTarget(GrVkGpu* gpu,
                                   SkISize dimensions,
                                   int sampleCnt,
                                   const GrVkImageInfo& info,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> mutableState,
                                   const GrVkImageInfo& msaaInfo,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> msaaMutableState,
                                   sk_sp<const GrVkImageView> colorAttachmentView,
                                   sk_sp<const GrVkImageView> resolveAttachmentView)
        : GrSurface(gpu, dimensions, info.fProtected)
        , GrVkImage(gpu, info, std::move(mutableState), GrBackendObjectOwnership::kBorrowed)
        // for the moment we only support 1:1 color to stencil
        , GrRenderTarget(gpu, dimensions, sampleCnt, info.fProtected)
        , fColorAttachmentView(std::move(colorAttachmentView))
        , fMSAAImage(new GrVkImage(gpu, msaaInfo, std::move(msaaMutableState),
                                   GrBackendObjectOwnership::kOwned))
        , fResolveAttachmentView(std::move(resolveAttachmentView))
        , fCachedFramebuffers()
        , fCachedRenderPasses() {
    SkASSERT(info.fProtected == msaaInfo.fProtected);
    SkASSERT(sampleCnt > 1);
    SkASSERT(SkToBool(info.fImageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
    this->setFlags(info);
    this->registerWithCacheWrapped(GrWrapCacheable::kNo);
}

// We're virtually derived from GrSurface (via GrRenderTarget) so its
// constructor must be explicitly called.
GrVkRenderTarget::GrVkRenderTarget(GrVkGpu* gpu,
                                   SkISize dimensions,
                                   int sampleCnt,
                                   const GrVkImageInfo& info,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> mutableState,
                                   const GrVkImageInfo& msaaInfo,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> msaaMutableState,
                                   sk_sp<const GrVkImageView> colorAttachmentView,
                                   sk_sp<const GrVkImageView> resolveAttachmentView,
                                   GrBackendObjectOwnership ownership)
        : GrSurface(gpu, dimensions, info.fProtected)
        , GrVkImage(gpu, info, std::move(mutableState), ownership)
        // for the moment we only support 1:1 color to stencil
        , GrRenderTarget(gpu, dimensions, sampleCnt, info.fProtected)
        , fColorAttachmentView(std::move(colorAttachmentView))
        , fMSAAImage(new GrVkImage(gpu, msaaInfo, std::move(msaaMutableState),
                                   GrBackendObjectOwnership::kOwned))
        , fResolveAttachmentView(std::move(resolveAttachmentView))
        , fCachedFramebuffers()
        , fCachedRenderPasses() {
    SkASSERT(info.fProtected == msaaInfo.fProtected);
    SkASSERT(sampleCnt > 1);
    SkASSERT(SkToBool(info.fImageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
    this->setFlags(info);
}

// We're virtually derived from GrSurface (via GrRenderTarget) so its
// constructor must be explicitly called.
GrVkRenderTarget::GrVkRenderTarget(GrVkGpu* gpu,
                                   SkISize dimensions,
                                   const GrVkImageInfo& info,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> mutableState,
                                   sk_sp<const GrVkImageView> colorAttachmentView)
        : GrSurface(gpu, dimensions, info.fProtected)
        , GrVkImage(gpu, info, std::move(mutableState), GrBackendObjectOwnership::kBorrowed)
        , GrRenderTarget(gpu, dimensions, 1, info.fProtected)
        , fColorAttachmentView(std::move(colorAttachmentView))
        , fMSAAImage(nullptr)
        , fCachedFramebuffers()
        , fCachedRenderPasses() {
    SkASSERT(SkToBool(info.fImageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
    this->setFlags(info);
    this->registerWithCacheWrapped(GrWrapCacheable::kNo);
}

// We're virtually derived from GrSurface (via GrRenderTarget) so its
// constructor must be explicitly called.
GrVkRenderTarget::GrVkRenderTarget(GrVkGpu* gpu,
                                   SkISize dimensions,
                                   const GrVkImageInfo& info,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> mutableState,
                                   sk_sp<const GrVkImageView> colorAttachmentView,
                                   GrBackendObjectOwnership ownership)
        : GrSurface(gpu, dimensions, info.fProtected)
        , GrVkImage(gpu, info, std::move(mutableState), ownership)
        , GrRenderTarget(gpu, dimensions, 1, info.fProtected)
        , fColorAttachmentView(std::move(colorAttachmentView))
        , fMSAAImage(nullptr)
        , fCachedFramebuffers()
        , fCachedRenderPasses() {
    SkASSERT(SkToBool(info.fImageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
    this->setFlags(info);
}

GrVkRenderTarget::GrVkRenderTarget(GrVkGpu* gpu,
                                   SkISize dimensions,
                                   const GrVkImageInfo& info,
                                   sk_sp<GrBackendSurfaceMutableStateImpl> mutableState,
                                   const GrVkRenderPass* renderPass,
                                   VkCommandBuffer secondaryCommandBuffer)
        : GrSurface(gpu, dimensions, info.fProtected)
        , GrVkImage(gpu, info, std::move(mutableState), GrBackendObjectOwnership::kBorrowed, true)
        , GrRenderTarget(gpu, dimensions, 1, info.fProtected)
        , fMSAAImage(nullptr)
        , fCachedFramebuffers()
        , fCachedRenderPasses()
        , fSecondaryCommandBuffer(secondaryCommandBuffer) {
    SkASSERT(fSecondaryCommandBuffer != VK_NULL_HANDLE);
    SkASSERT(SkToBool(info.fImageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
    this->setFlags(info);
    this->registerWithCacheWrapped(GrWrapCacheable::kNo);
    // We use the cached renderpass with no stencil and no extra dependencies to hold the external
    // render pass.
    int exteralRPIndex = renderpass_features_to_index(false, SelfDependencyFlags::kNone);
    fCachedRenderPasses[exteralRPIndex] = renderPass;
}

void GrVkRenderTarget::setFlags(const GrVkImageInfo& info) {
    if (info.fImageUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
        this->setVkRTSupportsInputAttachment();
    }
}

sk_sp<GrVkRenderTarget> GrVkRenderTarget::MakeWrappedRenderTarget(
        GrVkGpu* gpu, SkISize dimensions, int sampleCnt, const GrVkImageInfo& info,
        sk_sp<GrBackendSurfaceMutableStateImpl> mutableState) {
    SkASSERT(VK_NULL_HANDLE != info.fImage);

    SkASSERT(1 == info.fLevelCount);
    VkFormat pixelFormat = info.fFormat;

    VkImage colorImage;

    // create msaa surface if necessary
    GrVkImageInfo msInfo;
    sk_sp<GrBackendSurfaceMutableStateImpl> msMutableState;
    sk_sp<const GrVkImageView> resolveAttachmentView;
    if (sampleCnt > 1) {
        GrVkImage::ImageDesc msImageDesc;
        msImageDesc.fImageType = VK_IMAGE_TYPE_2D;
        msImageDesc.fFormat = pixelFormat;
        msImageDesc.fWidth = dimensions.fWidth;
        msImageDesc.fHeight = dimensions.fHeight;
        msImageDesc.fLevels = 1;
        msImageDesc.fSamples = sampleCnt;
        msImageDesc.fImageTiling = VK_IMAGE_TILING_OPTIMAL;
        msImageDesc.fUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        msImageDesc.fMemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        msImageDesc.fIsProtected = info.fProtected;

        if (!GrVkImage::InitImageInfo(gpu, msImageDesc, &msInfo)) {
            return nullptr;
        }

        // Set color attachment image
        colorImage = msInfo.fImage;

        // Create Resolve attachment view
        resolveAttachmentView = GrVkImageView::Make(gpu, info.fImage, pixelFormat,
                                                      GrVkImageView::kColor_Type, 1,
                                                      GrVkYcbcrConversionInfo());
        if (!resolveAttachmentView) {
            GrVkImage::DestroyImageInfo(gpu, &msInfo);
            return nullptr;
        }
        msMutableState.reset(new GrBackendSurfaceMutableStateImpl(msInfo.fImageLayout,
                                                           msInfo.fCurrentQueueFamily));
    } else {
        // Set color attachment image
        colorImage = info.fImage;
    }

    // Get color attachment view
    sk_sp<const GrVkImageView> colorAttachmentView = GrVkImageView::Make(
            gpu, colorImage, pixelFormat, GrVkImageView::kColor_Type, 1, GrVkYcbcrConversionInfo());
    if (!colorAttachmentView) {
        if (sampleCnt > 1) {
            resolveAttachmentView->unref();
            GrVkImage::DestroyImageInfo(gpu, &msInfo);
        }
        return nullptr;
    }

    GrVkRenderTarget* vkRT;
    if (sampleCnt > 1) {
        vkRT = new GrVkRenderTarget(gpu, dimensions, sampleCnt, info, std::move(mutableState),
                                    msInfo, std::move(msMutableState),
                                    std::move(colorAttachmentView),
                                    std::move(resolveAttachmentView));
    } else {
        vkRT = new GrVkRenderTarget(gpu, dimensions, info, std::move(mutableState),
                                    std::move(colorAttachmentView));
    }

    return sk_sp<GrVkRenderTarget>(vkRT);
}

sk_sp<GrVkRenderTarget> GrVkRenderTarget::MakeSecondaryCBRenderTarget(
        GrVkGpu* gpu, SkISize dimensions, const GrVkDrawableInfo& vkInfo) {
    const GrVkRenderPass* rp =
            gpu->resourceProvider().findCompatibleExternalRenderPass(vkInfo.fCompatibleRenderPass,
                                                                     vkInfo.fColorAttachmentIndex);
    if (!rp) {
        return nullptr;
    }

    if (vkInfo.fSecondaryCommandBuffer == VK_NULL_HANDLE) {
        return nullptr;
    }

    // We only set the few properties of the GrVkImageInfo that we know like layout and format. The
    // others we keep at the default "null" values.
    GrVkImageInfo info;
    info.fImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    info.fFormat = vkInfo.fFormat;
    info.fImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    sk_sp<GrBackendSurfaceMutableStateImpl> mutableState(new GrBackendSurfaceMutableStateImpl(
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_QUEUE_FAMILY_IGNORED));

    GrVkRenderTarget* vkRT = new GrVkRenderTarget(gpu, dimensions, info, std::move(mutableState),
                                                  rp, vkInfo.fSecondaryCommandBuffer);

    return sk_sp<GrVkRenderTarget>(vkRT);
}

bool GrVkRenderTarget::completeStencilAttachment() {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());
    return true;
}

const GrVkRenderPass* GrVkRenderTarget::externalRenderPass() const {
    SkASSERT(this->wrapsSecondaryCommandBuffer());
    // We use the cached render pass with no attachments or self dependencies to hold the
    // external render pass.
    int exteralRPIndex = renderpass_features_to_index(false, SelfDependencyFlags::kNone);
    return fCachedRenderPasses[exteralRPIndex];
}

GrVkResourceProvider::CompatibleRPHandle GrVkRenderTarget::compatibleRenderPassHandle(
        bool withStencil, SelfDependencyFlags selfDepFlags) {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());

    int cacheIndex = renderpass_features_to_index(withStencil, selfDepFlags);
    SkASSERT(cacheIndex < GrVkRenderTarget::kNumCachedRenderPasses);

    GrVkResourceProvider::CompatibleRPHandle* pRPHandle;
    pRPHandle = &fCompatibleRPHandles[cacheIndex];

    if (!pRPHandle->isValid()) {
        this->createSimpleRenderPass(withStencil, selfDepFlags);
    }

#ifdef SK_DEBUG
    const GrVkRenderPass* rp = fCachedRenderPasses[cacheIndex];
    SkASSERT(pRPHandle->isValid() == SkToBool(rp));
    if (rp) {
        SkASSERT(selfDepFlags == rp->selfDependencyFlags());
    }
#endif

    return *pRPHandle;
}

const GrVkRenderPass* GrVkRenderTarget::getSimpleRenderPass(bool withStencil,
                                                            SelfDependencyFlags selfDepFlags) {
    int cacheIndex = renderpass_features_to_index(withStencil, selfDepFlags);
    SkASSERT(cacheIndex < GrVkRenderTarget::kNumCachedRenderPasses);
    if (const GrVkRenderPass* rp = fCachedRenderPasses[cacheIndex]) {
        return rp;
    }

    return this->createSimpleRenderPass(withStencil, selfDepFlags);
}

const GrVkRenderPass* GrVkRenderTarget::createSimpleRenderPass(bool withStencil,
                                                               SelfDependencyFlags selfDepFlags) {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());

    GrVkResourceProvider& rp = this->getVkGpu()->resourceProvider();
    int cacheIndex = renderpass_features_to_index(withStencil, selfDepFlags);
    SkASSERT(cacheIndex < GrVkRenderTarget::kNumCachedRenderPasses);
    SkASSERT(!fCachedRenderPasses[cacheIndex]);
    fCachedRenderPasses[cacheIndex] = rp.findCompatibleRenderPass(
            *this, &fCompatibleRPHandles[cacheIndex], withStencil, selfDepFlags);
    return fCachedRenderPasses[cacheIndex];
}

const GrVkFramebuffer* GrVkRenderTarget::getFramebuffer(bool withStencil,
                                                        SelfDependencyFlags selfDepFlags) {
    int cacheIndex = renderpass_features_to_index(withStencil, selfDepFlags);
    SkASSERT(cacheIndex < GrVkRenderTarget::kNumCachedRenderPasses);
    if (auto fb = fCachedFramebuffers[cacheIndex]) {
        return fb;
    }

    return this->createFramebuffer(withStencil, selfDepFlags);
}

const GrVkFramebuffer* GrVkRenderTarget::createFramebuffer(bool withStencil,
                                                           SelfDependencyFlags selfDepFlags) {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());
    GrVkGpu* gpu = this->getVkGpu();

    const GrVkRenderPass* renderPass = this->getSimpleRenderPass(withStencil, selfDepFlags);
    if (!renderPass) {
        return nullptr;
    }

    int cacheIndex = renderpass_features_to_index(withStencil, selfDepFlags);
    SkASSERT(cacheIndex < GrVkRenderTarget::kNumCachedRenderPasses);

    // Stencil attachment view is stored in the base RT stencil attachment
    const GrVkImageView* stencilView = withStencil ? this->stencilAttachmentView() : nullptr;
    fCachedFramebuffers[cacheIndex] = GrVkFramebuffer::Create(gpu, this->width(), this->height(),
                                                              renderPass,
                                                              fColorAttachmentView.get(),
                                                              stencilView);

    return fCachedFramebuffers[cacheIndex];
}

void GrVkRenderTarget::getAttachmentsDescriptor(GrVkRenderPass::AttachmentsDescriptor* desc,
                                                GrVkRenderPass::AttachmentFlags* attachmentFlags,
                                                bool withStencil) const {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());
    desc->fColor.fFormat = this->imageFormat();
    desc->fColor.fSamples = this->numSamples();
    *attachmentFlags = GrVkRenderPass::kColor_AttachmentFlag;
    uint32_t attachmentCount = 1;

    if (withStencil) {
        const GrStencilAttachment* stencil = this->getStencilAttachment();
        SkASSERT(stencil);
        const GrVkStencilAttachment* vkStencil = static_cast<const GrVkStencilAttachment*>(stencil);
        desc->fStencil.fFormat = vkStencil->imageFormat();
        desc->fStencil.fSamples = vkStencil->numSamples();
#ifdef SK_DEBUG
        if (this->getVkGpu()->caps()->mixedSamplesSupport()) {
            SkASSERT(desc->fStencil.fSamples >= desc->fColor.fSamples);
        } else {
            SkASSERT(desc->fStencil.fSamples == desc->fColor.fSamples);
        }
#endif
        *attachmentFlags |= GrVkRenderPass::kStencil_AttachmentFlag;
        ++attachmentCount;
    }
    desc->fAttachmentCount = attachmentCount;
}

void GrVkRenderTarget::ReconstructAttachmentsDescriptor(const GrVkCaps& vkCaps,
                                                        const GrProgramInfo& programInfo,
                                                        GrVkRenderPass::AttachmentsDescriptor* desc,
                                                        GrVkRenderPass::AttachmentFlags* flags) {
    VkFormat format;
    SkAssertResult(programInfo.backendFormat().asVkFormat(&format));

    desc->fColor.fFormat = format;
    desc->fColor.fSamples = programInfo.numSamples();
    *flags = GrVkRenderPass::kColor_AttachmentFlag;
    uint32_t attachmentCount = 1;

    SkASSERT(!programInfo.isStencilEnabled() || programInfo.numStencilSamples());
    if (programInfo.numStencilSamples()) {
        const GrVkCaps::StencilFormat& stencilFormat = vkCaps.preferredStencilFormat();
        desc->fStencil.fFormat = stencilFormat.fInternalFormat;
        desc->fStencil.fSamples = programInfo.numStencilSamples();
#ifdef SK_DEBUG
        if (vkCaps.mixedSamplesSupport()) {
            SkASSERT(desc->fStencil.fSamples >= desc->fColor.fSamples);
        } else {
            SkASSERT(desc->fStencil.fSamples == desc->fColor.fSamples);
        }
#endif
        *flags |= GrVkRenderPass::kStencil_AttachmentFlag;
        ++attachmentCount;
    }
    desc->fAttachmentCount = attachmentCount;
}

const GrVkDescriptorSet* GrVkRenderTarget::inputDescSet(GrVkGpu* gpu) {
    SkASSERT(this->supportsInputAttachmentUsage());
    SkASSERT(this->numSamples() <= 1);
    if (fCachedInputDescriptorSet) {
        return fCachedInputDescriptorSet;
    }
    fCachedInputDescriptorSet = gpu->resourceProvider().getInputDescriptorSet();

    if (!fCachedInputDescriptorSet) {
        return nullptr;
    }

    VkDescriptorImageInfo imageInfo;
    memset(&imageInfo, 0, sizeof(VkDescriptorImageInfo));
    imageInfo.sampler = VK_NULL_HANDLE;
    imageInfo.imageView = this->colorAttachmentView()->imageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writeInfo;
    memset(&writeInfo, 0, sizeof(VkWriteDescriptorSet));
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.pNext = nullptr;
    writeInfo.dstSet = *fCachedInputDescriptorSet->descriptorSet();
    writeInfo.dstBinding = GrVkUniformHandler::kInputBinding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writeInfo.pImageInfo = &imageInfo;
    writeInfo.pBufferInfo = nullptr;
    writeInfo.pTexelBufferView = nullptr;

    GR_VK_CALL(gpu->vkInterface(), UpdateDescriptorSets(gpu->device(), 1, &writeInfo, 0, nullptr));

    return fCachedInputDescriptorSet;
}


GrVkRenderTarget::~GrVkRenderTarget() {
    // either release or abandon should have been called by the owner of this object.
    SkASSERT(!fMSAAImage);
    SkASSERT(!fResolveAttachmentView);
    SkASSERT(!fColorAttachmentView);

    for (int i = 0; i < kNumCachedRenderPasses; ++i) {
        SkASSERT(!fCachedFramebuffers[i]);
        SkASSERT(!fCachedRenderPasses[i]);
    }

    SkASSERT(!fCachedInputDescriptorSet);
}

void GrVkRenderTarget::addResources(GrVkCommandBuffer& commandBuffer, bool withStencil,
                                    SelfDependencyFlags selfDepFlags) {
    commandBuffer.addResource(this->getFramebuffer(withStencil, selfDepFlags));
    commandBuffer.addResource(this->colorAttachmentView());
    commandBuffer.addResource(this->msaaImageResource() ? this->msaaImageResource()
                                                        : this->resource());
    if (this->stencilImageResource()) {
        commandBuffer.addResource(this->stencilImageResource());
        commandBuffer.addResource(this->stencilAttachmentView());
    }
}

void GrVkRenderTarget::releaseInternalObjects() {
    if (fMSAAImage) {
        fMSAAImage->releaseImage();
        fMSAAImage.reset();
    }

    if (fResolveAttachmentView) {
        fResolveAttachmentView.reset();
    }
    if (fColorAttachmentView) {
        fColorAttachmentView.reset();
    }

    for (int i = 0; i < kNumCachedRenderPasses; ++i) {
        if (fCachedFramebuffers[i]) {
            fCachedFramebuffers[i]->unref();
            fCachedFramebuffers[i] = nullptr;
        }
        if (fCachedRenderPasses[i]) {
            fCachedRenderPasses[i]->unref();
            fCachedRenderPasses[i] = nullptr;
        }
    }

    if (fCachedInputDescriptorSet) {
        fCachedInputDescriptorSet->recycle();
        fCachedInputDescriptorSet = nullptr;
    }

    for (int i = 0; i < fGrSecondaryCommandBuffers.count(); ++i) {
        SkASSERT(fGrSecondaryCommandBuffers[i]);
        fGrSecondaryCommandBuffers[i]->releaseResources();
    }
    fGrSecondaryCommandBuffers.reset();
}

void GrVkRenderTarget::onRelease() {
    this->releaseInternalObjects();
    this->releaseImage();
    GrRenderTarget::onRelease();
}

void GrVkRenderTarget::onAbandon() {
    this->releaseInternalObjects();
    this->releaseImage();
    GrRenderTarget::onAbandon();
}

GrBackendRenderTarget GrVkRenderTarget::getBackendRenderTarget() const {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());
    return GrBackendRenderTarget(this->width(), this->height(), fInfo, this->getMutableState());
}

const GrManagedResource* GrVkRenderTarget::stencilImageResource() const {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());
    const GrStencilAttachment* stencil = this->getStencilAttachment();
    if (stencil) {
        const GrVkStencilAttachment* vkStencil = static_cast<const GrVkStencilAttachment*>(stencil);
        return vkStencil->imageResource();
    }

    return nullptr;
}

const GrVkImageView* GrVkRenderTarget::stencilAttachmentView() const {
    SkASSERT(!this->wrapsSecondaryCommandBuffer());
    const GrStencilAttachment* stencil = this->getStencilAttachment();
    if (stencil) {
        const GrVkStencilAttachment* vkStencil = static_cast<const GrVkStencilAttachment*>(stencil);
        return vkStencil->stencilView();
    }

    return nullptr;
}

GrVkGpu* GrVkRenderTarget::getVkGpu() const {
    SkASSERT(!this->wasDestroyed());
    return static_cast<GrVkGpu*>(this->getGpu());
}
