//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferVk.cpp:
//    Implements the class methods for FramebufferVk.
//

#include "libANGLE/renderer/vulkan/FramebufferVk.h"

#include <array>

#include "common/debug.h"
#include "common/vulkan/vk_headers.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/ResourceVk.h"
#include "libANGLE/renderer/vulkan/SurfaceVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/trace.h"

namespace rx
{

namespace
{
constexpr size_t kMinReadPixelsBufferSize = 128000;

// Alignment value to accommodate the largest known, for now, uncompressed Vulkan format
// VK_FORMAT_R64G64B64A64_SFLOAT, while supporting 3-component types such as
// VK_FORMAT_R16G16B16_SFLOAT.
constexpr size_t kReadPixelsBufferAlignment = 32 * 3;

// Clear values are only used when loadOp=Clear is set in clearWithRenderPassOp.  When starting a
// new render pass, the clear value is set to an unlikely value (bright pink) to stand out better
// in case of a bug.
constexpr VkClearValue kUninitializedClearValue = {{{0.95, 0.05, 0.95, 0.95}}};

// The value to assign an alpha channel that's emulated.  The type is unsigned int, though it will
// automatically convert to the actual data type.
constexpr unsigned int kEmulatedAlphaValue = 1;

bool HasSrcBlitFeature(RendererVk *renderer, RenderTargetVk *srcRenderTarget)
{
    const VkFormat srcFormat = srcRenderTarget->getImageFormat().vkImageFormat;
    return renderer->hasImageFormatFeatureBits(srcFormat, VK_FORMAT_FEATURE_BLIT_SRC_BIT);
}

bool HasDstBlitFeature(RendererVk *renderer, RenderTargetVk *dstRenderTarget)
{
    const VkFormat dstFormat = dstRenderTarget->getImageFormat().vkImageFormat;
    return renderer->hasImageFormatFeatureBits(dstFormat, VK_FORMAT_FEATURE_BLIT_DST_BIT);
}

// Returns false if destination has any channel the source doesn't.  This means that channel was
// emulated and using the Vulkan blit command would overwrite that emulated channel.
bool AreSrcAndDstColorChannelsBlitCompatible(RenderTargetVk *srcRenderTarget,
                                             RenderTargetVk *dstRenderTarget)
{
    const angle::Format &srcFormat = srcRenderTarget->getImageFormat().intendedFormat();
    const angle::Format &dstFormat = dstRenderTarget->getImageFormat().intendedFormat();

    // Luminance/alpha formats are not renderable, so they can't have ended up in a framebuffer to
    // participate in a blit.
    ASSERT(!dstFormat.isLUMA() && !srcFormat.isLUMA());

    // All color formats have the red channel.
    ASSERT(dstFormat.redBits > 0 && srcFormat.redBits > 0);

    return (dstFormat.greenBits > 0 || srcFormat.greenBits == 0) &&
           (dstFormat.blueBits > 0 || srcFormat.blueBits == 0) &&
           (dstFormat.alphaBits > 0 || srcFormat.alphaBits == 0);
}

// Returns false if formats are not identical.  vkCmdResolveImage and resolve attachments both
// require identical formats between source and destination.  vkCmdBlitImage additionally requires
// the same for depth/stencil formats.
bool AreSrcAndDstFormatsIdentical(RenderTargetVk *srcRenderTarget, RenderTargetVk *dstRenderTarget)
{
    const vk::Format &srcFormat = srcRenderTarget->getImageFormat();
    const vk::Format &dstFormat = dstRenderTarget->getImageFormat();

    return srcFormat.vkImageFormat == dstFormat.vkImageFormat;
}

bool AreSrcAndDstDepthStencilChannelsBlitCompatible(RenderTargetVk *srcRenderTarget,
                                                    RenderTargetVk *dstRenderTarget)
{
    const angle::Format &srcFormat = srcRenderTarget->getImageFormat().intendedFormat();
    const angle::Format &dstFormat = dstRenderTarget->getImageFormat().intendedFormat();

    return (dstFormat.depthBits > 0 || srcFormat.depthBits == 0) &&
           (dstFormat.stencilBits > 0 || srcFormat.stencilBits == 0);
}

void EarlyAdjustFlipYForPreRotation(SurfaceRotation blitAngleIn,
                                    SurfaceRotation *blitAngleOut,
                                    bool *blitFlipYOut)
{
    switch (blitAngleIn)
    {
        case SurfaceRotation::Identity:
            // No adjustments needed
            break;
        case SurfaceRotation::Rotated90Degrees:
            *blitAngleOut = SurfaceRotation::Rotated90Degrees;
            *blitFlipYOut = false;
            break;
        case SurfaceRotation::Rotated180Degrees:
            *blitAngleOut = SurfaceRotation::Rotated180Degrees;
            break;
        case SurfaceRotation::Rotated270Degrees:
            *blitAngleOut = SurfaceRotation::Rotated270Degrees;
            *blitFlipYOut = false;
            break;
        default:
            UNREACHABLE();
            break;
    }
}

void AdjustBlitAreaForPreRotation(SurfaceRotation framebufferAngle,
                                  const gl::Rectangle &blitAreaIn,
                                  gl::Rectangle framebufferDimensions,
                                  gl::Rectangle *blitAreaOut)
{
    switch (framebufferAngle)
    {
        case SurfaceRotation::Identity:
            // No adjustments needed
            break;
        case SurfaceRotation::Rotated90Degrees:
            blitAreaOut->x = blitAreaIn.y;
            blitAreaOut->y = blitAreaIn.x;
            std::swap(blitAreaOut->width, blitAreaOut->height);
            break;
        case SurfaceRotation::Rotated180Degrees:
            blitAreaOut->x = framebufferDimensions.width - blitAreaIn.x - blitAreaIn.width;
            blitAreaOut->y = framebufferDimensions.height - blitAreaIn.y - blitAreaIn.height;
            break;
        case SurfaceRotation::Rotated270Degrees:
            blitAreaOut->x = framebufferDimensions.height - blitAreaIn.y - blitAreaIn.height;
            blitAreaOut->y = framebufferDimensions.width - blitAreaIn.x - blitAreaIn.width;
            std::swap(blitAreaOut->width, blitAreaOut->height);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

void AdjustFramebufferDimensionsForPreRotation(SurfaceRotation framebufferAngle,
                                               gl::Rectangle *framebufferDimensions)
{
    switch (framebufferAngle)
    {
        case SurfaceRotation::Identity:
            // No adjustments needed
            break;
        case SurfaceRotation::Rotated90Degrees:
            std::swap(framebufferDimensions->width, framebufferDimensions->height);
            break;
        case SurfaceRotation::Rotated180Degrees:
            break;
        case SurfaceRotation::Rotated270Degrees:
            std::swap(framebufferDimensions->width, framebufferDimensions->height);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

// When blitting, the source and destination areas are viewed like UVs.  For example, a 64x64
// texture if flipped should have an offset of 64 in either X or Y which corresponds to U or V of 1.
// On the other hand, when resolving, the source and destination areas are used as fragment
// coordinates to fetch from.  In that case, when flipped, the texture in the above example must
// have an offset of 63.
void AdjustBlitResolveParametersForResolve(const gl::Rectangle &sourceArea,
                                           const gl::Rectangle &destArea,
                                           UtilsVk::BlitResolveParameters *params)
{
    params->srcOffset[0]  = sourceArea.x;
    params->srcOffset[1]  = sourceArea.y;
    params->destOffset[0] = destArea.x;
    params->destOffset[1] = destArea.y;

    if (sourceArea.isReversedX())
    {
        ASSERT(sourceArea.x > 0);
        --params->srcOffset[0];
    }
    if (sourceArea.isReversedY())
    {
        ASSERT(sourceArea.y > 0);
        --params->srcOffset[1];
    }
    if (destArea.isReversedX())
    {
        ASSERT(destArea.x > 0);
        --params->destOffset[0];
    }
    if (destArea.isReversedY())
    {
        ASSERT(destArea.y > 0);
        --params->destOffset[1];
    }
}

// Potentially make adjustments for pre-rotatation.  Depending on the angle some of the params need
// to be swapped and/or changes made to which axis are flipped.
void AdjustBlitResolveParametersForPreRotation(SurfaceRotation framebufferAngle,
                                               SurfaceRotation srcFramebufferAngle,
                                               UtilsVk::BlitResolveParameters *params)
{
    switch (framebufferAngle)
    {
        case SurfaceRotation::Identity:
            break;
        case SurfaceRotation::Rotated90Degrees:
            std::swap(params->stretch[0], params->stretch[1]);
            std::swap(params->srcOffset[0], params->srcOffset[1]);
            std::swap(params->rotatedOffsetFactor[0], params->rotatedOffsetFactor[1]);
            if (srcFramebufferAngle == framebufferAngle)
            {
                std::swap(params->destOffset[0], params->destOffset[1]);
                std::swap(params->stretch[0], params->stretch[1]);
                std::swap(params->flipX, params->flipY);
            }
            break;
        case SurfaceRotation::Rotated180Degrees:
            ASSERT(!params->flipX && params->flipY);
            params->flipX = true;
            params->flipY = false;
            break;
        case SurfaceRotation::Rotated270Degrees:
            std::swap(params->stretch[0], params->stretch[1]);
            std::swap(params->srcOffset[0], params->srcOffset[1]);
            std::swap(params->rotatedOffsetFactor[0], params->rotatedOffsetFactor[1]);
            if (srcFramebufferAngle == framebufferAngle)
            {
                std::swap(params->stretch[0], params->stretch[1]);
            }
            ASSERT(!params->flipX && !params->flipY);
            params->flipX = true;
            params->flipY = true;
            break;
        default:
            UNREACHABLE();
            break;
    }
}

bool HasResolveAttachment(const gl::AttachmentArray<RenderTargetVk *> &colorRenderTargets,
                          const gl::DrawBufferMask &getEnabledDrawBuffers)
{
    for (size_t colorIndexGL : getEnabledDrawBuffers)
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
        if (colorRenderTarget->hasResolveAttachment())
        {
            return true;
        }
    }
    return false;
}
}  // anonymous namespace

// static
FramebufferVk *FramebufferVk::CreateUserFBO(RendererVk *renderer, const gl::FramebufferState &state)
{
    return new FramebufferVk(renderer, state, nullptr);
}

// static
FramebufferVk *FramebufferVk::CreateDefaultFBO(RendererVk *renderer,
                                               const gl::FramebufferState &state,
                                               WindowSurfaceVk *backbuffer)
{
    return new FramebufferVk(renderer, state, backbuffer);
}

FramebufferVk::FramebufferVk(RendererVk *renderer,
                             const gl::FramebufferState &state,
                             WindowSurfaceVk *backbuffer)
    : FramebufferImpl(state),
      mBackbuffer(backbuffer),
      mFramebuffer(nullptr),
      mActiveColorComponents(0),
      mReadOnlyDepthStencilMode(false),
      mReadOnlyDepthFeedbackLoopMode(false)
{
    mReadPixelBuffer.init(renderer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, kReadPixelsBufferAlignment,
                          kMinReadPixelsBufferSize, true);
}

FramebufferVk::~FramebufferVk() = default;

void FramebufferVk::clearCache(ContextVk *contextVk)
{
    for (auto &entry : mFramebufferCache)
    {
        vk::FramebufferHelper &tmpFB = entry.second;
        tmpFB.release(contextVk);
    }
    mFramebufferCache.clear();
}

void FramebufferVk::destroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    mReadPixelBuffer.release(contextVk->getRenderer());
    clearCache(contextVk);
}

angle::Result FramebufferVk::discard(const gl::Context *context,
                                     size_t count,
                                     const GLenum *attachments)
{
    return invalidate(context, count, attachments);
}

angle::Result FramebufferVk::invalidate(const gl::Context *context,
                                        size_t count,
                                        const GLenum *attachments)
{
    ContextVk *contextVk = vk::GetImpl(context);

    ANGLE_TRY(invalidateImpl(contextVk, count, attachments, false));
    return angle::Result::Continue;
}

angle::Result FramebufferVk::invalidateSub(const gl::Context *context,
                                           size_t count,
                                           const GLenum *attachments,
                                           const gl::Rectangle &area)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // If there are deferred clears, flush them.  syncState may have accumulated deferred clears,
    // but if the framebuffer's attachments are used after this call not through the framebuffer,
    // those clears wouldn't get flushed otherwise (for example as the destination of
    // glCopyTex[Sub]Image, shader storage image, etc).
    ANGLE_TRY(flushDeferredClears(contextVk, getRotatedCompleteRenderArea(contextVk)));

    if (area.encloses(contextVk->getStartedRenderPassCommands().getRenderArea()))
    {
        // Because the render pass's render area is within the invalidated area, it is fine for
        // invalidateImpl() to use a storeOp of DONT_CARE (i.e. fine to not store the contents of
        // the invalidated area).
        ANGLE_TRY(invalidateImpl(contextVk, count, attachments, true));
    }
    else
    {
        ANGLE_PERF_WARNING(
            contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
            "InvalidateSubFramebuffer ignored due to area not covering the render area");
    }

    return angle::Result::Continue;
}

angle::Result FramebufferVk::clear(const gl::Context *context, GLbitfield mask)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "FramebufferVk::clear");
    ContextVk *contextVk = vk::GetImpl(context);

    bool clearColor   = IsMaskFlagSet(mask, static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT));
    bool clearDepth   = IsMaskFlagSet(mask, static_cast<GLbitfield>(GL_DEPTH_BUFFER_BIT));
    bool clearStencil = IsMaskFlagSet(mask, static_cast<GLbitfield>(GL_STENCIL_BUFFER_BIT));
    gl::DrawBufferMask clearColorBuffers;
    if (clearColor)
    {
        clearColorBuffers = mState.getEnabledDrawBuffers();
    }

    const VkClearColorValue &clearColorValue = contextVk->getClearColorValue().color;
    const VkClearDepthStencilValue &clearDepthStencilValue =
        contextVk->getClearDepthStencilValue().depthStencil;

    return clearImpl(context, clearColorBuffers, clearDepth, clearStencil, clearColorValue,
                     clearDepthStencilValue);
}

angle::Result FramebufferVk::clearImpl(const gl::Context *context,
                                       gl::DrawBufferMask clearColorBuffers,
                                       bool clearDepth,
                                       bool clearStencil,
                                       const VkClearColorValue &clearColorValue,
                                       const VkClearDepthStencilValue &clearDepthStencilValue)
{
    ContextVk *contextVk = vk::GetImpl(context);

    const gl::Rectangle scissoredRenderArea = getRotatedScissoredRenderArea(contextVk);

    // Discard clear altogether if scissor has 0 width or height.
    if (scissoredRenderArea.width == 0 || scissoredRenderArea.height == 0)
    {
        return angle::Result::Continue;
    }

    // We can sometimes get to a clear operation with other pending clears (e.g. for emulated
    // formats). Ensure the prior clears happen before the new clear. Note that we do not defer
    // clears for scissored operations. Note that some clears may be redundant with the current
    // clear. Due to complexity we haven't implemented de-duplication here.
    ANGLE_TRY(flushDeferredClears(contextVk, scissoredRenderArea));

    // This function assumes that only enabled attachments are asked to be cleared.
    ASSERT((clearColorBuffers & mState.getEnabledDrawBuffers()) == clearColorBuffers);

    // Adjust clear behavior based on whether the respective attachments are present; if asked to
    // clear a non-existent attachment, don't attempt to clear it.

    VkColorComponentFlags colorMaskFlags = contextVk->getClearColorMask();
    bool clearColor                      = clearColorBuffers.any();

    const gl::FramebufferAttachment *depthAttachment = mState.getDepthAttachment();
    clearDepth                                       = clearDepth && depthAttachment;
    ASSERT(!clearDepth || depthAttachment->isAttached());

    const gl::FramebufferAttachment *stencilAttachment = mState.getStencilAttachment();
    clearStencil                                       = clearStencil && stencilAttachment;
    ASSERT(!clearStencil || stencilAttachment->isAttached());

    uint8_t stencilMask =
        static_cast<uint8_t>(contextVk->getState().getDepthStencilState().stencilWritemask);

    // The front-end should ensure we don't attempt to clear color if all channels are masked.
    ASSERT(!clearColor || colorMaskFlags != 0);
    // The front-end should ensure we don't attempt to clear depth if depth write is disabled.
    ASSERT(!clearDepth || contextVk->getState().getDepthStencilState().depthMask);
    // The front-end should ensure we don't attempt to clear stencil if all bits are masked.
    ASSERT(!clearStencil || stencilMask != 0);

    bool scissoredClear = scissoredRenderArea != getRotatedCompleteRenderArea(contextVk);

    // If there is nothing to clear, return right away (for example, if asked to clear depth, but
    // there is no depth attachment).
    if (!clearColor && !clearDepth && !clearStencil)
    {
        return angle::Result::Continue;
    }

    // We can use render pass load ops if clearing depth, unmasked color or unmasked stencil.  If
    // there's a depth mask, depth clearing is already disabled.
    bool maskedClearColor =
        clearColor && (mActiveColorComponents & colorMaskFlags) != mActiveColorComponents;
    bool maskedClearStencil = clearStencil && stencilMask != 0xFF;

    bool clearColorWithRenderPassLoadOp   = clearColor && !maskedClearColor && !scissoredClear;
    bool clearDepthWithRenderPassLoadOp   = clearDepth && !scissoredClear;
    bool clearStencilWithRenderPassLoadOp = clearStencil && !maskedClearStencil && !scissoredClear;

    // At least one of color, depth or stencil should be clearable with render pass loadOp for us
    // to use this clear path.
    bool clearAnyWithRenderPassLoadOp = clearColorWithRenderPassLoadOp ||
                                        clearDepthWithRenderPassLoadOp ||
                                        clearStencilWithRenderPassLoadOp;

    if (clearAnyWithRenderPassLoadOp)
    {
        vk::Framebuffer *currentFramebuffer = nullptr;
        ANGLE_TRY(getFramebuffer(contextVk, &currentFramebuffer, nullptr));

        gl::DrawBufferMask clearColorDrawBuffersMask;
        if (clearColorWithRenderPassLoadOp)
        {
            clearColorDrawBuffersMask = clearColorBuffers;
        }

        // If we are in an active renderpass that has recorded commands and the framebuffer hasn't
        // changed, inline the clear
        if (contextVk->hasStartedRenderPassWithCommands() &&
            contextVk->hasStartedRenderPassWithFramebuffer(currentFramebuffer))
        {
            ANGLE_PERF_WARNING(
                contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
                "Clear effectively discarding previous draw call results. Suggest earlier Clear "
                "followed by masked color or depth/stencil draw calls instead");

            RendererVk *renderer     = contextVk->getRenderer();
            bool clearAnyWithCommand = clearAnyWithRenderPassLoadOp;

            // On buggy hardware, prefer to clear color with a draw call instead of
            // vkCmdClearAttachments.
            if (renderer->getFeatures().preferDrawClearOverVkCmdClearAttachments.enabled)
            {
                clearColorDrawBuffersMask.reset();
                clearAnyWithCommand =
                    clearDepthWithRenderPassLoadOp || clearStencilWithRenderPassLoadOp;
            }

            if (clearAnyWithCommand)
            {
                ANGLE_TRY(clearWithCommand(
                    contextVk, &contextVk->getStartedRenderPassCommands(), scissoredRenderArea,
                    clearColorDrawBuffersMask, clearDepthWithRenderPassLoadOp,
                    clearStencilWithRenderPassLoadOp, clearColorValue, clearDepthStencilValue));
            }
        }
        else
        {
            ANGLE_TRY(clearWithLoadOp(
                contextVk, clearColorDrawBuffersMask, clearDepthWithRenderPassLoadOp,
                clearStencilWithRenderPassLoadOp, clearColorValue, clearDepthStencilValue));
        }

        // Fallback to other methods for whatever isn't cleared here.
        if (clearColorDrawBuffersMask.any())
        {
            clearColorBuffers.reset();
            clearColor = false;
        }
        if (clearDepthWithRenderPassLoadOp)
        {
            clearDepth = false;
        }
        if (clearStencilWithRenderPassLoadOp)
        {
            clearStencil = false;
        }

        // If nothing left to clear, early out.
        if (!clearColor && !clearStencil)
        {
            return angle::Result::Continue;
        }
    }

    if (scissoredClear && !maskedClearColor && !maskedClearStencil)
    {
        return clearImmediatelyWithRenderPassOp(contextVk, scissoredRenderArea, clearColorBuffers,
                                                clearDepth, clearStencil, clearColorValue,
                                                clearDepthStencilValue);
    }

    // The most costly clear mode is when we need to mask out specific color channels or stencil
    // bits. This can only be done with a draw call.
    return clearWithDraw(contextVk, scissoredRenderArea, clearColorBuffers, clearDepth,
                         clearStencil, colorMaskFlags, stencilMask, clearColorValue,
                         clearDepthStencilValue);
}

angle::Result FramebufferVk::clearBufferfv(const gl::Context *context,
                                           GLenum buffer,
                                           GLint drawbuffer,
                                           const GLfloat *values)
{
    VkClearValue clearValue = {};

    bool clearDepth = false;
    gl::DrawBufferMask clearColorBuffers;

    if (buffer == GL_DEPTH)
    {
        clearDepth                    = true;
        clearValue.depthStencil.depth = values[0];
    }
    else
    {
        clearColorBuffers.set(drawbuffer);
        clearValue.color.float32[0] = values[0];
        clearValue.color.float32[1] = values[1];
        clearValue.color.float32[2] = values[2];
        clearValue.color.float32[3] = values[3];
    }

    return clearImpl(context, clearColorBuffers, clearDepth, false, clearValue.color,
                     clearValue.depthStencil);
}

angle::Result FramebufferVk::clearBufferuiv(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            const GLuint *values)
{
    VkClearValue clearValue = {};

    gl::DrawBufferMask clearColorBuffers;
    clearColorBuffers.set(drawbuffer);

    clearValue.color.uint32[0] = values[0];
    clearValue.color.uint32[1] = values[1];
    clearValue.color.uint32[2] = values[2];
    clearValue.color.uint32[3] = values[3];

    return clearImpl(context, clearColorBuffers, false, false, clearValue.color,
                     clearValue.depthStencil);
}

angle::Result FramebufferVk::clearBufferiv(const gl::Context *context,
                                           GLenum buffer,
                                           GLint drawbuffer,
                                           const GLint *values)
{
    VkClearValue clearValue = {};

    bool clearStencil = false;
    gl::DrawBufferMask clearColorBuffers;

    if (buffer == GL_STENCIL)
    {
        clearStencil = true;
        clearValue.depthStencil.stencil =
            gl::clamp(values[0], 0, std::numeric_limits<uint8_t>::max());
    }
    else
    {
        clearColorBuffers.set(drawbuffer);
        clearValue.color.int32[0] = values[0];
        clearValue.color.int32[1] = values[1];
        clearValue.color.int32[2] = values[2];
        clearValue.color.int32[3] = values[3];
    }

    return clearImpl(context, clearColorBuffers, false, clearStencil, clearValue.color,
                     clearValue.depthStencil);
}

angle::Result FramebufferVk::clearBufferfi(const gl::Context *context,
                                           GLenum buffer,
                                           GLint drawbuffer,
                                           GLfloat depth,
                                           GLint stencil)
{
    VkClearValue clearValue = {};

    clearValue.depthStencil.depth   = depth;
    clearValue.depthStencil.stencil = gl::clamp(stencil, 0, std::numeric_limits<uint8_t>::max());

    return clearImpl(context, gl::DrawBufferMask(), true, true, clearValue.color,
                     clearValue.depthStencil);
}

const gl::InternalFormat &FramebufferVk::getImplementationColorReadFormat(
    const gl::Context *context) const
{
    ContextVk *contextVk       = vk::GetImpl(context);
    GLenum sizedFormat         = mState.getReadAttachment()->getFormat().info->sizedInternalFormat;
    const vk::Format &vkFormat = contextVk->getRenderer()->getFormat(sizedFormat);
    GLenum implFormat          = vkFormat.actualImageFormat().fboImplementationInternalFormat;
    return gl::GetSizedInternalFormatInfo(implFormat);
}

angle::Result FramebufferVk::readPixels(const gl::Context *context,
                                        const gl::Rectangle &area,
                                        GLenum format,
                                        GLenum type,
                                        const gl::PixelPackState &pack,
                                        gl::Buffer *packBuffer,
                                        void *pixels)
{
    // Clip read area to framebuffer.
    const gl::Extents &fbSize = getState().getReadPixelsAttachment(format)->getSize();
    const gl::Rectangle fbRect(0, 0, fbSize.width, fbSize.height);
    ContextVk *contextVk = vk::GetImpl(context);

    gl::Rectangle clippedArea;
    if (!ClipRectangle(area, fbRect, &clippedArea))
    {
        // nothing to read
        return angle::Result::Continue;
    }

    // Flush any deferred clears.
    gl::Rectangle rotatedFbRect = fbRect;
    if (contextVk->isRotatedAspectRatioForReadFBO())
    {
        // The surface is rotated 90/270 degrees.  This changes the aspect ratio of the surface.
        // Since fbRect.{x|y} are both 0, there's no need to swap them.
        std::swap(rotatedFbRect.width, rotatedFbRect.height);
    }
    ANGLE_TRY(flushDeferredClears(contextVk, rotatedFbRect));

    GLuint outputSkipBytes = 0;
    PackPixelsParams params;
    ANGLE_TRY(vk::ImageHelper::GetReadPixelsParams(contextVk, pack, packBuffer, format, type, area,
                                                   clippedArea, &params, &outputSkipBytes));

    bool flipY = contextVk->isViewportFlipEnabledForReadFBO();
    switch (params.rotation = contextVk->getRotationReadFramebuffer())
    {
        case SurfaceRotation::Identity:
            // Do not rotate gl_Position (surface matches the device's orientation):
            if (flipY)
            {
                params.area.y = fbRect.height - clippedArea.y - clippedArea.height;
            }
            break;
        case SurfaceRotation::Rotated90Degrees:
            // Rotate gl_Position 90 degrees:
            params.area.x = clippedArea.y;
            params.area.y =
                flipY ? clippedArea.x : fbRect.width - clippedArea.x - clippedArea.width;
            std::swap(params.area.width, params.area.height);
            break;
        case SurfaceRotation::Rotated180Degrees:
            // Rotate gl_Position 180 degrees:
            params.area.x = fbRect.width - clippedArea.x - clippedArea.width;
            params.area.y =
                flipY ? clippedArea.y : fbRect.height - clippedArea.y - clippedArea.height;
            break;
        case SurfaceRotation::Rotated270Degrees:
            // Rotate gl_Position 270 degrees:
            params.area.x = fbRect.height - clippedArea.y - clippedArea.height;
            params.area.y =
                flipY ? fbRect.width - clippedArea.x - clippedArea.width : clippedArea.x;
            std::swap(params.area.width, params.area.height);
            break;
        default:
            UNREACHABLE();
            break;
    }
    if (flipY)
    {
        params.reverseRowOrder = !params.reverseRowOrder;
    }

    ANGLE_TRY(readPixelsImpl(contextVk, params.area, params, getReadPixelsAspectFlags(format),
                             getReadPixelsRenderTarget(format),
                             static_cast<uint8_t *>(pixels) + outputSkipBytes));
    mReadPixelBuffer.releaseInFlightBuffers(contextVk);
    return angle::Result::Continue;
}

RenderTargetVk *FramebufferVk::getDepthStencilRenderTarget() const
{
    return mRenderTargetCache.getDepthStencil();
}

RenderTargetVk *FramebufferVk::getColorDrawRenderTarget(size_t colorIndex) const
{
    RenderTargetVk *renderTarget = mRenderTargetCache.getColorDraw(mState, colorIndex);
    ASSERT(renderTarget && renderTarget->getImageForRenderPass().valid());
    return renderTarget;
}

RenderTargetVk *FramebufferVk::getColorReadRenderTarget() const
{
    RenderTargetVk *renderTarget = mRenderTargetCache.getColorRead(mState);
    ASSERT(renderTarget && renderTarget->getImageForRenderPass().valid());
    return renderTarget;
}

RenderTargetVk *FramebufferVk::getReadPixelsRenderTarget(GLenum format) const
{
    switch (format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_STENCIL_INDEX_OES:
            return getDepthStencilRenderTarget();
        default:
            return getColorReadRenderTarget();
    }
}

VkImageAspectFlagBits FramebufferVk::getReadPixelsAspectFlags(GLenum format) const
{
    switch (format)
    {
        case GL_DEPTH_COMPONENT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case GL_STENCIL_INDEX_OES:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

angle::Result FramebufferVk::blitWithCommand(ContextVk *contextVk,
                                             const gl::Rectangle &sourceArea,
                                             const gl::Rectangle &destArea,
                                             RenderTargetVk *readRenderTarget,
                                             RenderTargetVk *drawRenderTarget,
                                             GLenum filter,
                                             bool colorBlit,
                                             bool depthBlit,
                                             bool stencilBlit,
                                             bool flipX,
                                             bool flipY)
{
    // Since blitRenderbufferRect is called for each render buffer that needs to be blitted,
    // it should never be the case that both color and depth/stencil need to be blitted at
    // at the same time.
    ASSERT(colorBlit != (depthBlit || stencilBlit));

    vk::ImageHelper *srcImage = &readRenderTarget->getImageForCopy();
    vk::ImageHelper *dstImage = &drawRenderTarget->getImageForWrite();

    VkImageAspectFlags imageAspectMask = srcImage->getAspectFlags();
    VkImageAspectFlags blitAspectMask  = imageAspectMask;

    // Remove depth or stencil aspects if they are not requested to be blitted.
    if (!depthBlit)
    {
        blitAspectMask &= ~VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (!stencilBlit)
    {
        blitAspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    ANGLE_TRY(contextVk->onImageTransferRead(imageAspectMask, srcImage));
    ANGLE_TRY(contextVk->onImageTransferWrite(imageAspectMask, dstImage));
    vk::CommandBuffer &commandBuffer = contextVk->getOutsideRenderPassCommandBuffer();

    VkImageBlit blit               = {};
    blit.srcSubresource.aspectMask = blitAspectMask;
    blit.srcSubresource.mipLevel   = srcImage->toVkLevel(readRenderTarget->getLevelIndex()).get();
    blit.srcSubresource.baseArrayLayer = readRenderTarget->getLayerIndex();
    blit.srcSubresource.layerCount     = 1;
    blit.srcOffsets[0]                 = {sourceArea.x0(), sourceArea.y0(), 0};
    blit.srcOffsets[1]                 = {sourceArea.x1(), sourceArea.y1(), 1};
    blit.dstSubresource.aspectMask     = blitAspectMask;
    blit.dstSubresource.mipLevel = dstImage->toVkLevel(drawRenderTarget->getLevelIndex()).get();
    blit.dstSubresource.baseArrayLayer = drawRenderTarget->getLayerIndex();
    blit.dstSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = {destArea.x0(), destArea.y0(), 0};
    blit.dstOffsets[1]                 = {destArea.x1(), destArea.y1(), 1};

    commandBuffer.blitImage(srcImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            dstImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                            gl_vk::GetFilter(filter));

    return angle::Result::Continue;
}

angle::Result FramebufferVk::blit(const gl::Context *context,
                                  const gl::Rectangle &sourceAreaIn,
                                  const gl::Rectangle &destAreaIn,
                                  GLbitfield mask,
                                  GLenum filter)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();
    UtilsVk &utilsVk     = contextVk->getUtils();

    // We can sometimes end up in a blit with some clear commands saved. Ensure all clear commands
    // are issued before we issue the blit command.
    ANGLE_TRY(flushDeferredClears(contextVk, getRotatedCompleteRenderArea(contextVk)));

    const gl::State &glState              = contextVk->getState();
    const gl::Framebuffer *srcFramebuffer = glState.getReadFramebuffer();
    FramebufferVk *srcFramebufferVk       = vk::GetImpl(srcFramebuffer);

    const bool blitColorBuffer   = (mask & GL_COLOR_BUFFER_BIT) != 0;
    const bool blitDepthBuffer   = (mask & GL_DEPTH_BUFFER_BIT) != 0;
    const bool blitStencilBuffer = (mask & GL_STENCIL_BUFFER_BIT) != 0;

    // If a framebuffer contains a mixture of multisampled and multisampled-render-to-texture
    // attachments, this function could be simultaneously doing a blit on one attachment and resolve
    // on another.  For the most part, this means resolve semantics apply.  However, as the resolve
    // path cannot be taken for multisampled-render-to-texture attachments, the distinction of
    // whether resolve is done for each attachment or blit is made.
    const bool isColorResolve =
        blitColorBuffer &&
        srcFramebufferVk->getColorReadRenderTarget()->getImageForCopy().getSamples() > 1;
    const bool isDepthStencilResolve =
        (blitDepthBuffer || blitStencilBuffer) &&
        srcFramebufferVk->getDepthStencilRenderTarget()->getImageForCopy().getSamples() > 1;
    const bool isResolve = isColorResolve || isDepthStencilResolve;

    bool srcFramebufferFlippedY  = contextVk->isViewportFlipEnabledForReadFBO();
    bool destFramebufferFlippedY = contextVk->isViewportFlipEnabledForDrawFBO();

    gl::Rectangle sourceArea = sourceAreaIn;
    gl::Rectangle destArea   = destAreaIn;

    // Note: GLES (all 3.x versions) require source and dest area to be identical when
    // resolving.
    ASSERT(!isResolve ||
           (sourceArea.x == destArea.x && sourceArea.y == destArea.y &&
            sourceArea.width == destArea.width && sourceArea.height == destArea.height));

    gl::Rectangle srcFramebufferDimensions  = srcFramebufferVk->getNonRotatedCompleteRenderArea();
    gl::Rectangle destFramebufferDimensions = getNonRotatedCompleteRenderArea();

    // If the destination is flipped in either direction, we will flip the source instead so that
    // the destination area is always unflipped.
    sourceArea = sourceArea.flip(destArea.isReversedX(), destArea.isReversedY());
    destArea   = destArea.removeReversal();

    // Calculate the stretch factor prior to any clipping, as it needs to remain constant.
    const float stretch[2] = {
        std::abs(sourceArea.width / static_cast<float>(destArea.width)),
        std::abs(sourceArea.height / static_cast<float>(destArea.height)),
    };

    // Potentially make adjustments for pre-rotatation.  To handle various cases (e.g. clipping)
    // and to not interrupt the normal flow of the code, different adjustments are made in
    // different parts of the code.  These first adjustments are for whether or not to flip the
    // y-axis, and to note the overall rotation (regardless of whether it is the source or
    // destination that is rotated).
    SurfaceRotation srcFramebufferRotation  = contextVk->getRotationReadFramebuffer();
    SurfaceRotation destFramebufferRotation = contextVk->getRotationDrawFramebuffer();
    SurfaceRotation rotation                = SurfaceRotation::Identity;
    // Both the source and destination cannot be rotated (which would indicate both are the default
    // framebuffer (i.e. swapchain image).
    ASSERT((srcFramebufferRotation == SurfaceRotation::Identity) ||
           (destFramebufferRotation == SurfaceRotation::Identity));
    EarlyAdjustFlipYForPreRotation(srcFramebufferRotation, &rotation, &srcFramebufferFlippedY);
    EarlyAdjustFlipYForPreRotation(destFramebufferRotation, &rotation, &destFramebufferFlippedY);

    // First, clip the source area to framebuffer.  That requires transforming the dest area to
    // match the clipped source.
    gl::Rectangle absSourceArea = sourceArea.removeReversal();
    gl::Rectangle clippedSourceArea;
    if (!gl::ClipRectangle(srcFramebufferDimensions, absSourceArea, &clippedSourceArea))
    {
        return angle::Result::Continue;
    }

    // Resize the destination area based on the new size of source.  Note again that stretch is
    // calculated as SrcDimension/DestDimension.
    gl::Rectangle srcClippedDestArea;
    if (isResolve)
    {
        // Source and dest areas are identical in resolve (except rotate it, if appropriate).
        srcClippedDestArea = clippedSourceArea;
        AdjustBlitAreaForPreRotation(destFramebufferRotation, clippedSourceArea,
                                     destFramebufferDimensions, &srcClippedDestArea);
    }
    else if (clippedSourceArea == absSourceArea)
    {
        // If there was no clipping, keep dest area as is (except rotate it, if appropriate).
        srcClippedDestArea = destArea;
        AdjustBlitAreaForPreRotation(destFramebufferRotation, destArea, destFramebufferDimensions,
                                     &srcClippedDestArea);
    }
    else
    {
        // Shift dest area's x0,y0,x1,y1 by as much as the source area's got shifted (taking
        // stretching into account)
        float x0Shift = std::round((clippedSourceArea.x - absSourceArea.x) / stretch[0]);
        float y0Shift = std::round((clippedSourceArea.y - absSourceArea.y) / stretch[1]);
        float x1Shift = std::round((absSourceArea.x1() - clippedSourceArea.x1()) / stretch[0]);
        float y1Shift = std::round((absSourceArea.y1() - clippedSourceArea.y1()) / stretch[1]);

        // If the source area was reversed in any direction, the shift should be applied in the
        // opposite direction as well.
        if (sourceArea.isReversedX())
        {
            std::swap(x0Shift, x1Shift);
        }

        if (sourceArea.isReversedY())
        {
            std::swap(y0Shift, y1Shift);
        }

        srcClippedDestArea.x = destArea.x0() + static_cast<int>(x0Shift);
        srcClippedDestArea.y = destArea.y0() + static_cast<int>(y0Shift);
        int x1               = destArea.x1() - static_cast<int>(x1Shift);
        int y1               = destArea.y1() - static_cast<int>(y1Shift);

        srcClippedDestArea.width  = x1 - srcClippedDestArea.x;
        srcClippedDestArea.height = y1 - srcClippedDestArea.y;

        // Rotate srcClippedDestArea if the destination is rotated
        if (destFramebufferRotation != SurfaceRotation::Identity)
        {
            gl::Rectangle originalSrcClippedDestArea = srcClippedDestArea;
            AdjustBlitAreaForPreRotation(destFramebufferRotation, originalSrcClippedDestArea,
                                         destFramebufferDimensions, &srcClippedDestArea);
        }
    }

    // If framebuffers are flipped in Y, flip the source and dest area (which define the
    // transformation regardless of clipping), as well as the blit area (which is the clipped
    // dest area).
    if (srcFramebufferFlippedY)
    {
        sourceArea.y      = srcFramebufferDimensions.height - sourceArea.y;
        sourceArea.height = -sourceArea.height;
    }
    if (destFramebufferFlippedY)
    {
        destArea.y      = destFramebufferDimensions.height - destArea.y;
        destArea.height = -destArea.height;

        srcClippedDestArea.y =
            destFramebufferDimensions.height - srcClippedDestArea.y - srcClippedDestArea.height;
    }

    const bool flipX = sourceArea.isReversedX() != destArea.isReversedX();
    const bool flipY = sourceArea.isReversedY() != destArea.isReversedY();

    // GLES doesn't allow flipping the parameters of glBlitFramebuffer if performing a resolve.
    ASSERT(!isResolve ||
           (flipX == false && flipY == (srcFramebufferFlippedY != destFramebufferFlippedY)));

    // Again, transfer the destination flip to source, so dest is unflipped.  Note that destArea
    // was not reversed until the final possible Y-flip.
    ASSERT(!destArea.isReversedX());
    sourceArea = sourceArea.flip(false, destArea.isReversedY());
    destArea   = destArea.removeReversal();

    // Now that clipping and flipping is done, rotate certain values that will be used for
    // UtilsVk::BlitResolveParameters
    gl::Rectangle sourceAreaOld = sourceArea;
    gl::Rectangle destAreaOld   = destArea;
    if (srcFramebufferRotation == rotation)
    {
        AdjustBlitAreaForPreRotation(srcFramebufferRotation, sourceAreaOld,
                                     srcFramebufferDimensions, &sourceArea);
        AdjustFramebufferDimensionsForPreRotation(srcFramebufferRotation,
                                                  &srcFramebufferDimensions);
    }
    SurfaceRotation rememberDestFramebufferRotation = destFramebufferRotation;
    if (srcFramebufferRotation == SurfaceRotation::Rotated90Degrees)
    {
        destFramebufferRotation = rotation;
    }
    AdjustBlitAreaForPreRotation(destFramebufferRotation, destAreaOld, destFramebufferDimensions,
                                 &destArea);
    destFramebufferRotation = rememberDestFramebufferRotation;

    // Clip the destination area to the framebuffer size and scissor.  Note that we don't care
    // about the source area anymore.  The offset translation is done based on the original source
    // and destination rectangles.  The stretch factor is already calculated as well.
    gl::Rectangle blitArea;
    if (!gl::ClipRectangle(getRotatedScissoredRenderArea(contextVk), srcClippedDestArea, &blitArea))
    {
        return angle::Result::Continue;
    }

    bool noClip = blitArea == destArea && stretch[0] == 1.0f && stretch[1] == 1.0f;
    bool noFlip = !flipX && !flipY;
    bool disableFlippingBlitWithCommand =
        contextVk->getRenderer()->getFeatures().disableFlippingBlitWithCommand.enabled;

    UtilsVk::BlitResolveParameters commonParams;
    commonParams.srcOffset[0]           = sourceArea.x;
    commonParams.srcOffset[1]           = sourceArea.y;
    commonParams.destOffset[0]          = destArea.x;
    commonParams.destOffset[1]          = destArea.y;
    commonParams.rotatedOffsetFactor[0] = std::abs(sourceArea.width);
    commonParams.rotatedOffsetFactor[1] = std::abs(sourceArea.height);
    commonParams.stretch[0]             = stretch[0];
    commonParams.stretch[1]             = stretch[1];
    commonParams.srcExtents[0]          = srcFramebufferDimensions.width;
    commonParams.srcExtents[1]          = srcFramebufferDimensions.height;
    commonParams.blitArea               = blitArea;
    commonParams.linear                 = filter == GL_LINEAR;
    commonParams.flipX                  = flipX;
    commonParams.flipY                  = flipY;
    commonParams.rotation               = rotation;

    if (blitColorBuffer)
    {
        RenderTargetVk *readRenderTarget      = srcFramebufferVk->getColorReadRenderTarget();
        UtilsVk::BlitResolveParameters params = commonParams;
        params.srcLayer                       = readRenderTarget->getLayerIndex();

        // Multisampled images are not allowed to have mips.
        ASSERT(!isColorResolve || readRenderTarget->getLevelIndex() == gl::LevelIndex(0));

        // If there was no clipping and the format capabilities allow us, use Vulkan's builtin blit.
        // The reason clipping is prohibited in this path is that due to rounding errors, it would
        // be hard to guarantee the image stretching remains perfect.  That also allows us not to
        // have to transform back the dest clipping to source.
        //
        // Non-identity pre-rotation cases do not use Vulkan's builtin blit.
        //
        // For simplicity, we either blit all render targets with a Vulkan command, or none.
        bool canBlitWithCommand = !isColorResolve && noClip &&
                                  (noFlip || !disableFlippingBlitWithCommand) &&
                                  HasSrcBlitFeature(renderer, readRenderTarget) &&
                                  (rotation == SurfaceRotation::Identity);
        bool areChannelsBlitCompatible = true;
        bool areFormatsIdentical       = true;
        for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
        {
            RenderTargetVk *drawRenderTarget = mRenderTargetCache.getColors()[colorIndexGL];
            canBlitWithCommand =
                canBlitWithCommand && HasDstBlitFeature(renderer, drawRenderTarget);
            areChannelsBlitCompatible =
                areChannelsBlitCompatible &&
                AreSrcAndDstColorChannelsBlitCompatible(readRenderTarget, drawRenderTarget);
            areFormatsIdentical = AreSrcAndDstFormatsIdentical(readRenderTarget, drawRenderTarget);
        }

        // Now that all flipping is done, adjust the offsets for resolve and prerotation
        if (isColorResolve)
        {
            AdjustBlitResolveParametersForResolve(sourceArea, destArea, &params);
        }
        AdjustBlitResolveParametersForPreRotation(rotation, srcFramebufferRotation, &params);

        if (canBlitWithCommand && areChannelsBlitCompatible)
        {
            for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
            {
                RenderTargetVk *drawRenderTarget = mRenderTargetCache.getColors()[colorIndexGL];
                ANGLE_TRY(blitWithCommand(contextVk, sourceArea, destArea, readRenderTarget,
                                          drawRenderTarget, filter, true, false, false, flipX,
                                          flipY));
            }
        }
        // If we're not flipping or rotating, use Vulkan's builtin resolve.
        else if (isColorResolve && !flipX && !flipY && areChannelsBlitCompatible &&
                 areFormatsIdentical && rotation == SurfaceRotation::Identity)
        {
            // Resolving with a subpass resolve attachment has a few restrictions:
            // 1.) glBlitFramebuffer() needs to copy the read color attachment to all enabled
            // attachments in the draw framebuffer, but Vulkan requires a 1:1 relationship for
            // multisample attachments to resolve attachments in the render pass subpass.
            // Due to this, we currently only support using resolve attachments when there is a
            // single draw attachment enabled.
            // 2.) Using a subpass resolve attachment relies on using the render pass that performs
            // the draw to still be open, so it can be updated to use the resolve attachment to draw
            // into. If there's no render pass with commands, then the multisampled render pass is
            // already done and whose data is already flushed from the tile (in a tile-based
            // renderer), so there's no chance for the resolve attachment to take advantage of the
            // data already being present in the tile.
            vk::Framebuffer *srcVkFramebuffer = nullptr;
            ANGLE_TRY(srcFramebufferVk->getFramebuffer(contextVk, &srcVkFramebuffer, nullptr));

            // TODO(https://anglebug.com/4968): Support multiple open render passes so we can remove
            //  this hack to 'restore' the finished render pass.
            contextVk->restoreFinishedRenderPass(srcVkFramebuffer);

            if ((mState.getEnabledDrawBuffers().count() == 1) &&
                contextVk->hasStartedRenderPassWithFramebuffer(srcVkFramebuffer))
            {
                // glBlitFramebuffer() needs to copy the read color attachment to all enabled
                // attachments in the draw framebuffer, but Vulkan requires a 1:1 relationship for
                // multisample attachments to resolve attachments in the render pass subpass.
                // Due to this, we currently only support using resolve attachments when there is a
                // single draw attachment enabled.
                ANGLE_TRY(resolveColorWithSubpass(contextVk, params));
            }
            else
            {
                ANGLE_TRY(resolveColorWithCommand(contextVk, params,
                                                  &readRenderTarget->getImageForCopy()));
            }
        }
        // Otherwise use a shader to do blit or resolve.
        else
        {
            // Flush the render pass, which may incur a vkQueueSubmit, before taking any views.
            // Otherwise the view serials would not reflect the render pass they are really used in.
            // http://crbug.com/1272266#c22
            ANGLE_TRY(
                contextVk->flushCommandsAndEndRenderPass());

            const vk::ImageView *copyImageView = nullptr;
            ANGLE_TRY(readRenderTarget->getAndRetainCopyImageView(contextVk, &copyImageView));
            ANGLE_TRY(utilsVk.colorBlitResolve(
                contextVk, this, &readRenderTarget->getImageForCopy(), copyImageView, params));
        }
    }

    if (blitDepthBuffer || blitStencilBuffer)
    {
        RenderTargetVk *readRenderTarget      = srcFramebufferVk->getDepthStencilRenderTarget();
        RenderTargetVk *drawRenderTarget      = mRenderTargetCache.getDepthStencil();
        UtilsVk::BlitResolveParameters params = commonParams;
        params.srcLayer                       = readRenderTarget->getLayerIndex();

        // Multisampled images are not allowed to have mips.
        ASSERT(!isDepthStencilResolve || readRenderTarget->getLevelIndex() == gl::LevelIndex(0));

        // Similarly, only blit if there's been no clipping or rotating.
        bool canBlitWithCommand = !isDepthStencilResolve && noClip &&
                                  (noFlip || !disableFlippingBlitWithCommand) &&
                                  HasSrcBlitFeature(renderer, readRenderTarget) &&
                                  HasDstBlitFeature(renderer, drawRenderTarget) &&
                                  (rotation == SurfaceRotation::Identity);
        bool areChannelsBlitCompatible =
            AreSrcAndDstDepthStencilChannelsBlitCompatible(readRenderTarget, drawRenderTarget);

        // glBlitFramebuffer requires that depth/stencil blits have matching formats.
        ASSERT(AreSrcAndDstFormatsIdentical(readRenderTarget, drawRenderTarget));

        if (canBlitWithCommand && areChannelsBlitCompatible)
        {
            ANGLE_TRY(blitWithCommand(contextVk, sourceArea, destArea, readRenderTarget,
                                      drawRenderTarget, filter, false, blitDepthBuffer,
                                      blitStencilBuffer, flipX, flipY));
        }
        else
        {
            // Now that all flipping is done, adjust the offsets for resolve and prerotation
            if (isDepthStencilResolve)
            {
                AdjustBlitResolveParametersForResolve(sourceArea, destArea, &params);
            }
            AdjustBlitResolveParametersForPreRotation(rotation, srcFramebufferRotation, &params);

            // Create depth- and stencil-only views for reading.
            vk::DeviceScoped<vk::ImageView> depthView(contextVk->getDevice());
            vk::DeviceScoped<vk::ImageView> stencilView(contextVk->getDevice());

            vk::ImageHelper *depthStencilImage = &readRenderTarget->getImageForCopy();
            vk::LevelIndex levelIndex =
                depthStencilImage->toVkLevel(readRenderTarget->getLevelIndex());
            uint32_t layerIndex         = readRenderTarget->getLayerIndex();
            gl::TextureType textureType = vk::Get2DTextureType(depthStencilImage->getLayerCount(),
                                                               depthStencilImage->getSamples());

            if (blitDepthBuffer)
            {
                ANGLE_TRY(depthStencilImage->initLayerImageView(
                    contextVk, textureType, VK_IMAGE_ASPECT_DEPTH_BIT, gl::SwizzleState(),
                    &depthView.get(), levelIndex, 1, layerIndex, 1));
            }

            if (blitStencilBuffer)
            {
                ANGLE_TRY(depthStencilImage->initLayerImageView(
                    contextVk, textureType, VK_IMAGE_ASPECT_STENCIL_BIT, gl::SwizzleState(),
                    &stencilView.get(), levelIndex, 1, layerIndex, 1));
            }

            // If shader stencil export is not possible, defer stencil blit/stencil to another pass.
            bool hasShaderStencilExport =
                contextVk->getRenderer()->getFeatures().supportsShaderStencilExport.enabled;

            // Blit depth. If shader stencil export is present, blit stencil as well.
            if (blitDepthBuffer || (blitStencilBuffer && hasShaderStencilExport))
            {
                const vk::ImageView *depth = blitDepthBuffer ? &depthView.get() : nullptr;
                const vk::ImageView *stencil =
                    blitStencilBuffer && hasShaderStencilExport ? &stencilView.get() : nullptr;

                ANGLE_TRY(utilsVk.depthStencilBlitResolve(contextVk, this, depthStencilImage, depth,
                                                          stencil, params));
            }

            // If shader stencil export is not present, blit stencil through a different path.
            if (blitStencilBuffer && !hasShaderStencilExport)
            {
                ANGLE_PERF_WARNING(contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
                                   "Inefficient BlitFramebuffer operation on the stencil aspect "
                                   "due to lack of shader stencil export support");
                ANGLE_TRY(utilsVk.stencilBlitResolveNoShaderExport(
                    contextVk, this, depthStencilImage, &stencilView.get(), params));
            }

            vk::ImageView depthViewObject   = depthView.release();
            vk::ImageView stencilViewObject = stencilView.release();

            contextVk->addGarbage(&depthViewObject);
            contextVk->addGarbage(&stencilViewObject);
        }
    }

    return angle::Result::Continue;
}  // namespace rx

void FramebufferVk::updateColorResolveAttachment(
    uint32_t colorIndexGL,
    vk::ImageViewSubresourceSerial resolveImageViewSerial)
{
    mCurrentFramebufferDesc.updateColorResolve(colorIndexGL, resolveImageViewSerial);
    mFramebuffer = nullptr;
    mRenderPassDesc.packColorResolveAttachment(colorIndexGL);
}

void FramebufferVk::removeColorResolveAttachment(uint32_t colorIndexGL)
{
    mCurrentFramebufferDesc.updateColorResolve(colorIndexGL,
                                               vk::kInvalidImageViewSubresourceSerial);
    mFramebuffer = nullptr;
    mRenderPassDesc.removeColorResolveAttachment(colorIndexGL);
}

angle::Result FramebufferVk::resolveColorWithSubpass(ContextVk *contextVk,
                                                     const UtilsVk::BlitResolveParameters &params)
{
    // Vulkan requires a 1:1 relationship for multisample attachments to resolve attachments in the
    // render pass subpass. Due to this, we currently only support using resolve attachments when
    // there is a single draw attachment enabled.
    ASSERT(mState.getEnabledDrawBuffers().count() == 1);
    uint32_t drawColorIndexGL = static_cast<uint32_t>(*mState.getEnabledDrawBuffers().begin());

    const gl::State &glState              = contextVk->getState();
    const gl::Framebuffer *srcFramebuffer = glState.getReadFramebuffer();
    FramebufferVk *srcFramebufferVk       = vk::GetImpl(srcFramebuffer);
    uint32_t readColorIndexGL             = srcFramebuffer->getState().getReadIndex();

    // Use the draw FBO's color attachments as resolve attachments in the read FBO.
    // - Assign the draw FBO's color attachment Serial to the read FBO's resolve attachment
    // - Deactivate the source Framebuffer, since the description changed
    // - Update the renderpass description to indicate there's a resolve attachment
    vk::ImageViewSubresourceSerial resolveImageViewSerial =
        mCurrentFramebufferDesc.getColorImageViewSerial(drawColorIndexGL);
    ASSERT(resolveImageViewSerial.imageViewSerial.valid());
    srcFramebufferVk->updateColorResolveAttachment(readColorIndexGL, resolveImageViewSerial);

    // Since the source FBO was updated with a resolve attachment it didn't have when the render
    // pass was started, we need to:
    // 1. Get the new framebuffer
    //   - The draw framebuffer's ImageView will be used as the resolve attachment, so pass it along
    //   in case vkCreateFramebuffer() needs to be called to create a new vkFramebuffer with the new
    //   resolve attachment.
    RenderTargetVk *drawRenderTarget      = mRenderTargetCache.getColors()[drawColorIndexGL];
    const vk::ImageView *resolveImageView = nullptr;
    ANGLE_TRY(drawRenderTarget->getImageView(contextVk, &resolveImageView));
    vk::Framebuffer *newSrcFramebuffer = nullptr;
    ANGLE_TRY(srcFramebufferVk->getFramebuffer(contextVk, &newSrcFramebuffer, resolveImageView));
    // 2. Update the CommandBufferHelper with the new framebuffer and render pass
    vk::CommandBufferHelper &commandBufferHelper = contextVk->getStartedRenderPassCommands();
    commandBufferHelper.updateRenderPassForResolve(newSrcFramebuffer,
                                                   srcFramebufferVk->getRenderPassDesc());

    // End the render pass now since we don't (yet) support subpass dependencies.
    RenderTargetVk *readRenderTarget = getColorReadRenderTarget();
    contextVk->onImageRenderPassWrite(VK_IMAGE_ASPECT_COLOR_BIT, vk::ImageLayout::ColorAttachment,
                                      &readRenderTarget->getImageForRenderPass());
    ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());

    // Remove the resolve attachment from the source framebuffer.
    srcFramebufferVk->removeColorResolveAttachment(readColorIndexGL);

    return angle::Result::Continue;
}

angle::Result FramebufferVk::resolveColorWithCommand(ContextVk *contextVk,
                                                     const UtilsVk::BlitResolveParameters &params,
                                                     vk::ImageHelper *srcImage)
{
    ANGLE_TRY(contextVk->onImageTransferRead(VK_IMAGE_ASPECT_COLOR_BIT, srcImage));

    VkImageResolve resolveRegion                = {};
    resolveRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.srcSubresource.mipLevel       = 0;
    resolveRegion.srcSubresource.baseArrayLayer = params.srcLayer;
    resolveRegion.srcSubresource.layerCount     = 1;
    resolveRegion.srcOffset.x                   = params.blitArea.x;
    resolveRegion.srcOffset.y                   = params.blitArea.y;
    resolveRegion.srcOffset.z                   = 0;
    resolveRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    resolveRegion.dstSubresource.layerCount     = 1;
    resolveRegion.dstOffset.x                   = params.blitArea.x;
    resolveRegion.dstOffset.y                   = params.blitArea.y;
    resolveRegion.dstOffset.z                   = 0;
    resolveRegion.extent.width                  = params.blitArea.width;
    resolveRegion.extent.height                 = params.blitArea.height;
    resolveRegion.extent.depth                  = 1;

    vk::PerfCounters &perfCounters = contextVk->getPerfCounters();
    for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *drawRenderTarget = mRenderTargetCache.getColors()[colorIndexGL];
        ANGLE_TRY(contextVk->onImageTransferWrite(VK_IMAGE_ASPECT_COLOR_BIT,
                                                  &drawRenderTarget->getImageForWrite()));

        vk::CommandBuffer &commandBuffer = contextVk->getOutsideRenderPassCommandBuffer();

        vk::ImageHelper &dstImage = drawRenderTarget->getImageForWrite();
        vk::LevelIndex levelVk    = dstImage.toVkLevel(drawRenderTarget->getLevelIndex());
        resolveRegion.dstSubresource.mipLevel       = levelVk.get();
        resolveRegion.dstSubresource.baseArrayLayer = drawRenderTarget->getLayerIndex();

        srcImage->resolve(&dstImage, resolveRegion, &commandBuffer);

        perfCounters.resolveImageCommands++;
    }

    return angle::Result::Continue;
}

bool FramebufferVk::checkStatus(const gl::Context *context) const
{
    // if we have both a depth and stencil buffer, they must refer to the same object
    // since we only support packed_depth_stencil and not separate depth and stencil
    if (mState.hasSeparateDepthAndStencilAttachments())
    {
        return false;
    }

    return true;
}

angle::Result FramebufferVk::invalidateImpl(ContextVk *contextVk,
                                            size_t count,
                                            const GLenum *attachments,
                                            bool isSubInvalidate)
{
    gl::DrawBufferMask invalidateColorBuffers;
    bool invalidateDepthBuffer   = false;
    bool invalidateStencilBuffer = false;

    for (size_t i = 0; i < count; ++i)
    {
        const GLenum attachment = attachments[i];

        switch (attachment)
        {
            case GL_DEPTH:
            case GL_DEPTH_ATTACHMENT:
                invalidateDepthBuffer = true;
                break;
            case GL_STENCIL:
            case GL_STENCIL_ATTACHMENT:
                invalidateStencilBuffer = true;
                break;
            case GL_DEPTH_STENCIL_ATTACHMENT:
                invalidateDepthBuffer   = true;
                invalidateStencilBuffer = true;
                break;
            default:
                ASSERT(
                    (attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15) ||
                    (attachment == GL_COLOR));

                invalidateColorBuffers.set(
                    attachment == GL_COLOR ? 0u : (attachment - GL_COLOR_ATTACHMENT0));
        }
    }

    // Shouldn't try to issue deferred clears if invalidating sub framebuffer.
    ASSERT(mDeferredClears.empty() || !isSubInvalidate);

    // Remove deferred clears for the invalidated attachments.
    if (invalidateDepthBuffer)
    {
        mDeferredClears.reset(vk::kUnpackedDepthIndex);
    }
    if (invalidateStencilBuffer)
    {
        mDeferredClears.reset(vk::kUnpackedStencilIndex);
    }
    for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
    {
        if (invalidateColorBuffers.test(colorIndexGL))
        {
            mDeferredClears.reset(colorIndexGL);
        }
    }

    // If there are still deferred clears, flush them.  See relevant comment in invalidateSub.
    ANGLE_TRY(flushDeferredClears(contextVk, getRotatedCompleteRenderArea(contextVk)));

    const auto &colorRenderTargets           = mRenderTargetCache.getColors();
    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();

    // To ensure we invalidate the right renderpass we require that the current framebuffer be the
    // same as the current renderpass' framebuffer. E.g. prevent sequence like:
    //- Bind FBO 1, draw
    //- Bind FBO 2, draw
    //- Bind FBO 1, invalidate D/S
    // to invalidate the D/S of FBO 2 since it would be the currently active renderpass.
    vk::Framebuffer *currentFramebuffer = nullptr;
    ANGLE_TRY(getFramebuffer(contextVk, &currentFramebuffer, nullptr));

    if (contextVk->hasStartedRenderPassWithFramebuffer(currentFramebuffer))
    {
        // Set the appropriate storeOp for attachments.
        vk::PackedAttachmentIndex colorIndexVk(0);
        for (size_t colorIndexGL : mState.getColorAttachmentsMask())
        {
            if (mState.getEnabledDrawBuffers()[colorIndexGL] &&
                invalidateColorBuffers.test(colorIndexGL))
            {
                contextVk->getStartedRenderPassCommands().invalidateRenderPassColorAttachment(
                    colorIndexVk);
            }
            ++colorIndexVk;
        }

        if (depthStencilRenderTarget)
        {
            const gl::DepthStencilState &dsState = contextVk->getState().getDepthStencilState();
            if (invalidateDepthBuffer)
            {
                contextVk->getStartedRenderPassCommands().invalidateRenderPassDepthAttachment(
                    dsState);
            }

            if (invalidateStencilBuffer)
            {
                contextVk->getStartedRenderPassCommands().invalidateRenderPassStencilAttachment(
                    dsState);
            }
        }
        if (invalidateColorBuffers.any())
        {
            // Only end the render pass if invalidating at least one color buffer.  Do not end the
            // render pass if only the depth and/or stencil buffer is invalidated.  At least one
            // application invalidates those every frame, disables depth, and then continues to
            // draw only to the color buffer.
            //
            // Since we are not aware of any application that invalidates a color buffer and
            // continues to draw to it, we leave that unoptimized.
            ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());
        }
    }

    // If not a partial invalidate, mark the contents of the invalidated attachments as undefined,
    // so their loadOp can be set to DONT_CARE in the following render pass.
    if (!isSubInvalidate)
    {
        for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
        {
            if (invalidateColorBuffers.test(colorIndexGL))
            {
                RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
                ASSERT(colorRenderTarget);
                colorRenderTarget->invalidateEntireContent();
            }
        }

        // If we have a depth / stencil render target AND we invalidate both we'll mark it as
        // invalid. Maybe in the future add separate depth & stencil invalid flags.
        if (depthStencilRenderTarget && invalidateDepthBuffer && invalidateStencilBuffer)
        {
            depthStencilRenderTarget->invalidateEntireContent();
        }
    }

    return angle::Result::Continue;
}

angle::Result FramebufferVk::updateColorAttachment(const gl::Context *context,
                                                   bool deferClears,
                                                   uint32_t colorIndexGL)
{
    ContextVk *contextVk = vk::GetImpl(context);

    ANGLE_TRY(mRenderTargetCache.updateColorRenderTarget(context, mState, colorIndexGL));

    // Update cached masks for masked clears.
    RenderTargetVk *renderTarget = mRenderTargetCache.getColors()[colorIndexGL];
    if (renderTarget)
    {
        const angle::Format &actualFormat = renderTarget->getImageFormat().actualImageFormat();
        updateActiveColorMasks(colorIndexGL, actualFormat.redBits > 0, actualFormat.greenBits > 0,
                               actualFormat.blueBits > 0, actualFormat.alphaBits > 0);

        const angle::Format &sourceFormat = renderTarget->getImageFormat().intendedFormat();
        mEmulatedAlphaAttachmentMask.set(colorIndexGL,
                                         sourceFormat.alphaBits == 0 && actualFormat.alphaBits > 0);

        contextVk->updateColorMask(context->getState().getBlendState());

        if (deferClears && mState.getEnabledDrawBuffers().test(colorIndexGL))
        {
            ANGLE_TRY(renderTarget->flushStagedUpdates(contextVk, &mDeferredClears, colorIndexGL));
        }
        else
        {
            ANGLE_TRY(renderTarget->flushStagedUpdates(contextVk, nullptr, 0));
        }
    }
    else
    {
        updateActiveColorMasks(colorIndexGL, false, false, false, false);
    }

    const bool enabledColor =
        renderTarget && mState.getColorAttachments()[colorIndexGL].isAttached();
    const bool enabledResolve = enabledColor && renderTarget->hasResolveAttachment();

    if (enabledColor)
    {
        mCurrentFramebufferDesc.updateColor(colorIndexGL, renderTarget->getDrawSubresourceSerial());
    }
    else
    {
        mCurrentFramebufferDesc.updateColor(colorIndexGL, vk::kInvalidImageViewSubresourceSerial);
    }

    if (enabledResolve)
    {
        mCurrentFramebufferDesc.updateColorResolve(colorIndexGL,
                                                   renderTarget->getResolveSubresourceSerial());
    }
    else
    {
        mCurrentFramebufferDesc.updateColorResolve(colorIndexGL,
                                                   vk::kInvalidImageViewSubresourceSerial);
    }

    return angle::Result::Continue;
}

angle::Result FramebufferVk::updateDepthStencilAttachment(const gl::Context *context,
                                                          bool deferClears)
{
    ANGLE_TRY(mRenderTargetCache.updateDepthStencilRenderTarget(context, mState));

    ContextVk *contextVk = vk::GetImpl(context);
    updateDepthStencilAttachmentSerial(contextVk);

    RenderTargetVk *depthStencilRT = getDepthStencilRenderTarget();
    if (depthStencilRT)
    {
        if (deferClears)
        {
            ANGLE_TRY(depthStencilRT->flushStagedUpdates(contextVk, &mDeferredClears,
                                                         vk::kUnpackedDepthIndex));
        }
        else
        {
            ANGLE_TRY(depthStencilRT->flushStagedUpdates(contextVk, nullptr, 0));
        }
    }

    return angle::Result::Continue;
}

void FramebufferVk::updateDepthStencilAttachmentSerial(ContextVk *contextVk)
{
    RenderTargetVk *depthStencilRT = getDepthStencilRenderTarget();

    if (depthStencilRT != nullptr)
    {
        mCurrentFramebufferDesc.updateDepthStencil(depthStencilRT->getDrawSubresourceSerial());
    }
    else
    {
        mCurrentFramebufferDesc.updateDepthStencil(vk::kInvalidImageViewSubresourceSerial);
    }

    if (depthStencilRT != nullptr && depthStencilRT->hasResolveAttachment())
    {
        mCurrentFramebufferDesc.updateDepthStencilResolve(
            depthStencilRT->getResolveSubresourceSerial());
    }
    else
    {
        mCurrentFramebufferDesc.updateDepthStencilResolve(vk::kInvalidImageViewSubresourceSerial);
    }
}

angle::Result FramebufferVk::syncState(const gl::Context *context,
                                       GLenum binding,
                                       const gl::Framebuffer::DirtyBits &dirtyBits,
                                       gl::Command command)
{
    ContextVk *contextVk = vk::GetImpl(context);

    vk::FramebufferDesc priorFramebufferDesc = mCurrentFramebufferDesc;

    // Only defer clears for whole draw framebuffer ops. If the scissor test is on and the scissor
    // rect doesn't match the draw rect, forget it.
    //
    // NOTE: Neither renderArea nor scissoredRenderArea are rotated, since scissoredRenderArea did
    // not come from FramebufferVk::getRotatedScissoredRenderArea().
    gl::Rectangle renderArea          = getNonRotatedCompleteRenderArea();
    gl::Rectangle scissoredRenderArea = ClipRectToScissor(context->getState(), renderArea, false);
    bool deferClears = binding == GL_DRAW_FRAMEBUFFER && renderArea == scissoredRenderArea;

    // If we are notified that any attachment is dirty, but we have deferred clears for them, a
    // flushDeferredClears() call is missing somewhere.  ASSERT this to catch these bugs.
    vk::ClearValuesArray previousDeferredClears = mDeferredClears;
    bool shouldUpdateColorMask                  = false;

    // For any updated attachments we'll update their Serials below
    ASSERT(dirtyBits.any());
    for (size_t dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::Framebuffer::DIRTY_BIT_DEPTH_ATTACHMENT:
            case gl::Framebuffer::DIRTY_BIT_DEPTH_BUFFER_CONTENTS:
            case gl::Framebuffer::DIRTY_BIT_STENCIL_ATTACHMENT:
            case gl::Framebuffer::DIRTY_BIT_STENCIL_BUFFER_CONTENTS:
                ASSERT(!previousDeferredClears.testDepth());
                ASSERT(!previousDeferredClears.testStencil());
                ANGLE_TRY(updateDepthStencilAttachment(context, deferClears));
                break;
            case gl::Framebuffer::DIRTY_BIT_READ_BUFFER:
                ANGLE_TRY(mRenderTargetCache.update(context, mState, dirtyBits));
                break;
            case gl::Framebuffer::DIRTY_BIT_DRAW_BUFFERS:
                shouldUpdateColorMask = true;
                break;
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_WIDTH:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_HEIGHT:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_SAMPLES:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_FIXED_SAMPLE_LOCATIONS:
                // Invalidate the cache. If we have performance critical code hitting this path we
                // can add related data (such as width/height) to the cache
                clearCache(contextVk);
                break;
            default:
            {
                static_assert(gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0 == 0, "FB dirty bits");
                uint32_t colorIndexGL;
                if (dirtyBit < gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_MAX)
                {
                    colorIndexGL = static_cast<uint32_t>(
                        dirtyBit - gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0);
                }
                else
                {
                    ASSERT(dirtyBit >= gl::Framebuffer::DIRTY_BIT_COLOR_BUFFER_CONTENTS_0 &&
                           dirtyBit < gl::Framebuffer::DIRTY_BIT_COLOR_BUFFER_CONTENTS_MAX);
                    colorIndexGL = static_cast<uint32_t>(
                        dirtyBit - gl::Framebuffer::DIRTY_BIT_COLOR_BUFFER_CONTENTS_0);
                }

                ASSERT(!previousDeferredClears.test(colorIndexGL));

                ANGLE_TRY(updateColorAttachment(context, deferClears, colorIndexGL));
                shouldUpdateColorMask = true;
                break;
            }
        }
    }

    if (shouldUpdateColorMask)
    {
        contextVk->updateColorMask(context->getState().getBlendState());
    }

    // In some cases we'll need to force a flush of deferred clears. When we're syncing the read
    // framebuffer we might not get a RenderPass. Also when there are masked out cleared color
    // channels.
    if (binding == GL_READ_FRAMEBUFFER && !mDeferredClears.empty())
    {
        // Rotate scissoredRenderArea based on the read FBO's rotation (different than
        // FramebufferVk::getRotatedScissoredRenderArea(), which is based on the draw FBO's
        // rotation).  Since the rectangle is scissored, it must be fully rotated, and not just
        // have the width and height swapped.
        bool invertViewport = contextVk->isViewportFlipEnabledForReadFBO();
        gl::Rectangle rotatedScissoredRenderArea;
        RotateRectangle(contextVk->getRotationReadFramebuffer(), invertViewport, renderArea.width,
                        renderArea.height, scissoredRenderArea, &rotatedScissoredRenderArea);

        ANGLE_TRY(flushDeferredClears(contextVk, rotatedScissoredRenderArea));
    }

    // No-op redundant changes to prevent closing the RenderPass.
    if (mCurrentFramebufferDesc == priorFramebufferDesc)
    {
        return angle::Result::Continue;
    }

    // Default to writable depth on any Framebuffer change.
    mReadOnlyDepthStencilMode = false;

    // The FBO's new attachment may have changed the renderable area
    const gl::State &glState = context->getState();
    ANGLE_TRY(contextVk->updateScissor(glState));

    mActiveColorComponents = gl_vk::GetColorComponentFlags(
        mActiveColorComponentMasksForClear[0].any(), mActiveColorComponentMasksForClear[1].any(),
        mActiveColorComponentMasksForClear[2].any(), mActiveColorComponentMasksForClear[3].any());

    if (command != gl::Command::Blit)
    {
        // Don't end the render pass when handling a blit to resolve, since we may be able to
        // optimize that path which requires modifying the current render pass.
        // We're deferring the resolve check to FramebufferVk::blit(), since if the read buffer is
        // multisampled-render-to-texture, then srcFramebuffer->getSamples(context) gives > 1, but
        // there's no resolve happening as the read buffer's singlesampled image will be used as
        // blit src. FramebufferVk::blit() will handle those details for us.
        ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());
    }

    updateRenderPassDesc();

    // Notify the ContextVk to update the pipeline desc.
    FramebufferVk *currentDrawFramebuffer = vk::GetImpl(context->getState().getDrawFramebuffer());
    if (currentDrawFramebuffer == this)
    {
        contextVk->onDrawFramebufferChange(this);
    }
    // Deactivate Framebuffer
    mFramebuffer = nullptr;

    return angle::Result::Continue;
}

void FramebufferVk::updateRenderPassDesc()
{
    mRenderPassDesc = {};
    mRenderPassDesc.setSamples(getSamples());

    // Color attachments.
    const auto &colorRenderTargets               = mRenderTargetCache.getColors();
    const gl::DrawBufferMask colorAttachmentMask = mState.getColorAttachmentsMask();
    for (size_t colorIndexGL = 0; colorIndexGL < colorAttachmentMask.size(); ++colorIndexGL)
    {
        if (colorAttachmentMask[colorIndexGL])
        {
            RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
            ASSERT(colorRenderTarget);
            mRenderPassDesc.packColorAttachment(
                colorIndexGL,
                colorRenderTarget->getImageForRenderPass().getFormat().intendedFormatID);

            // Add the resolve attachment, if any.
            if (colorRenderTarget->hasResolveAttachment())
            {
                mRenderPassDesc.packColorResolveAttachment(colorIndexGL);
            }
        }
        else
        {
            mRenderPassDesc.packColorAttachmentGap(colorIndexGL);
        }
    }

    // Depth/stencil attachment.
    RenderTargetVk *depthStencilRenderTarget = getDepthStencilRenderTarget();
    if (depthStencilRenderTarget)
    {
        vk::ResourceAccess dsAccess =
            mReadOnlyDepthStencilMode ? vk::ResourceAccess::ReadOnly : vk::ResourceAccess::Write;

        mRenderPassDesc.packDepthStencilAttachment(
            depthStencilRenderTarget->getImageForRenderPass().getFormat().intendedFormatID,
            dsAccess);

        // Add the resolve attachment, if any.
        if (depthStencilRenderTarget->hasResolveAttachment())
        {
            const vk::Format &format = depthStencilRenderTarget->getImageFormat();
            bool hasDepth            = format.intendedFormat().depthBits > 0;
            bool hasStencil          = format.intendedFormat().stencilBits > 0;

            mRenderPassDesc.packDepthStencilResolveAttachment(hasDepth, hasStencil);
        }
    }
}

angle::Result FramebufferVk::getFramebuffer(ContextVk *contextVk,
                                            vk::Framebuffer **framebufferOut,
                                            const vk::ImageView *resolveImageViewIn)
{
    // First return a presently valid Framebuffer
    if (mFramebuffer != nullptr)
    {
        *framebufferOut = &mFramebuffer->getFramebuffer();
        return angle::Result::Continue;
    }
    // No current FB, so now check for previously cached Framebuffer
    auto iter = mFramebufferCache.find(mCurrentFramebufferDesc);
    if (iter != mFramebufferCache.end())
    {
        if (contextVk->getRenderer()->getFeatures().enableFramebufferVkCache.enabled)
        {
            *framebufferOut = &iter->second.getFramebuffer();
            return angle::Result::Continue;
        }
        else
        {
            // When cache is off just release previous entry, it will be recreated below
            iter->second.release(contextVk);
        }
    }

    vk::RenderPass *compatibleRenderPass = nullptr;
    ANGLE_TRY(contextVk->getCompatibleRenderPass(mRenderPassDesc, &compatibleRenderPass));

    // If we've a Framebuffer provided by a Surface (default FBO/backbuffer), query it.
    if (mBackbuffer)
    {
        return mBackbuffer->getCurrentFramebuffer(contextVk, *compatibleRenderPass, framebufferOut);
    }

    // Gather VkImageViews over all FBO attachments, also size of attached region.
    std::vector<VkImageView> attachments;
    gl::Extents attachmentsSize;

    // Color attachments.
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndexGL : mState.getColorAttachmentsMask())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
        ASSERT(colorRenderTarget);

        const vk::ImageView *imageView = nullptr;
        ANGLE_TRY(colorRenderTarget->getImageView(contextVk, &imageView));

        attachments.push_back(imageView->getHandle());

        ASSERT(attachmentsSize.empty() || attachmentsSize == colorRenderTarget->getExtents());
        attachmentsSize = colorRenderTarget->getExtents();
    }

    // Depth/stencil attachment.
    RenderTargetVk *depthStencilRenderTarget = getDepthStencilRenderTarget();
    if (depthStencilRenderTarget)
    {
        const vk::ImageView *imageView = nullptr;
        ANGLE_TRY(depthStencilRenderTarget->getImageView(contextVk, &imageView));

        attachments.push_back(imageView->getHandle());

        ASSERT(attachmentsSize.empty() ||
               attachmentsSize == depthStencilRenderTarget->getExtents());
        attachmentsSize = depthStencilRenderTarget->getExtents();
    }

    // Color resolve attachments.
    if (resolveImageViewIn)
    {
        ASSERT(!HasResolveAttachment(colorRenderTargets, mState.getEnabledDrawBuffers()));

        // Need to use the passed in ImageView for the resolve attachment, since it came from
        // another Framebuffer.
        attachments.push_back(resolveImageViewIn->getHandle());
    }
    else
    {
        // This Framebuffer owns all of the ImageViews, including its own resolve ImageViews.
        for (size_t colorIndexGL : mState.getColorAttachmentsMask())
        {
            RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
            ASSERT(colorRenderTarget);

            if (colorRenderTarget->hasResolveAttachment())
            {
                const vk::ImageView *resolveImageView = nullptr;
                ANGLE_TRY(colorRenderTarget->getResolveImageView(contextVk, &resolveImageView));

                attachments.push_back(resolveImageView->getHandle());

                ASSERT(!attachmentsSize.empty());
            }
        }
    }

    // Depth/stencil resolve attachment.
    if (depthStencilRenderTarget && depthStencilRenderTarget->hasResolveAttachment())
    {
        const vk::ImageView *imageView = nullptr;
        ANGLE_TRY(depthStencilRenderTarget->getResolveImageView(contextVk, &imageView));

        attachments.push_back(imageView->getHandle());

        ASSERT(!attachmentsSize.empty());
    }

    if (attachmentsSize.empty())
    {
        // No attachments, so use the default values.
        attachmentsSize.height = mState.getDefaultHeight();
        attachmentsSize.width  = mState.getDefaultWidth();
        attachmentsSize.depth  = 0;
    }
    VkFramebufferCreateInfo framebufferInfo = {};

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags           = 0;
    framebufferInfo.renderPass      = compatibleRenderPass->getHandle();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = static_cast<uint32_t>(attachmentsSize.width);
    framebufferInfo.height          = static_cast<uint32_t>(attachmentsSize.height);
    framebufferInfo.layers          = 1;

    vk::FramebufferHelper newFramebuffer;
    ANGLE_TRY(newFramebuffer.init(contextVk, framebufferInfo));

    // Sanity check that our description matches our attachments. Can catch implementation bugs.
    ASSERT(static_cast<uint32_t>(attachments.size()) == mCurrentFramebufferDesc.attachmentCount());

    mFramebufferCache[mCurrentFramebufferDesc] = std::move(newFramebuffer);
    mFramebuffer                               = &mFramebufferCache[mCurrentFramebufferDesc];
    *framebufferOut                            = &mFramebuffer->getFramebuffer();
    return angle::Result::Continue;
}

angle::Result FramebufferVk::clearImmediatelyWithRenderPassOp(
    ContextVk *contextVk,
    const gl::Rectangle &clearArea,
    gl::DrawBufferMask clearColorBuffers,
    bool clearDepth,
    bool clearStencil,
    const VkClearColorValue &clearColorValue,
    const VkClearDepthStencilValue &clearDepthStencilValue)
{
    for (size_t colorIndexGL : clearColorBuffers)
    {
        VkClearValue clearValue = getCorrectedColorClearValue(colorIndexGL, clearColorValue);
        mDeferredClears.store(static_cast<uint32_t>(colorIndexGL), VK_IMAGE_ASPECT_COLOR_BIT,
                              clearValue);
    }

    if (clearDepth)
    {
        VkClearValue clearValue;
        clearValue.depthStencil = clearDepthStencilValue;
        mDeferredClears.store(vk::kUnpackedDepthIndex, VK_IMAGE_ASPECT_DEPTH_BIT, clearValue);
    }

    if (clearStencil)
    {
        VkClearValue clearValue;
        clearValue.depthStencil = clearDepthStencilValue;
        mDeferredClears.store(vk::kUnpackedStencilIndex, VK_IMAGE_ASPECT_STENCIL_BIT, clearValue);
    }

    // Ensure the clear happens immediately.
    return flushDeferredClears(contextVk, clearArea);
}

angle::Result FramebufferVk::clearWithDraw(ContextVk *contextVk,
                                           const gl::Rectangle &clearArea,
                                           gl::DrawBufferMask clearColorBuffers,
                                           bool clearDepth,
                                           bool clearStencil,
                                           VkColorComponentFlags colorMaskFlags,
                                           uint8_t stencilMask,
                                           const VkClearColorValue &clearColorValue,
                                           const VkClearDepthStencilValue &clearDepthStencilValue)
{
    if (clearDepth)
    {
        VkClearValue clearValue;
        clearValue.depthStencil = clearDepthStencilValue;
        mDeferredClears.store(vk::kUnpackedDepthIndex, VK_IMAGE_ASPECT_DEPTH_BIT, clearValue);

        // Scissored-only clears are handled in clearImmediatelyWithRenderPassOp.
        ASSERT(clearColorBuffers.any() || clearStencil);

        // Force start a new render pass for the depth clear to take effect.
        // UtilsVk::clearFramebuffer may not start a new render pass if there's one already started.
        ANGLE_TRY(flushDeferredClears(contextVk, clearArea));
    }

    UtilsVk::ClearFramebufferParameters params = {};
    params.clearArea                           = clearArea;
    params.colorClearValue                     = clearColorValue;
    params.stencilClearValue = static_cast<uint8_t>(clearDepthStencilValue.stencil);
    params.stencilMask       = stencilMask;

    params.clearColor   = true;
    params.clearStencil = clearStencil;

    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndexGL : clearColorBuffers)
    {
        const RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
        ASSERT(colorRenderTarget);

        params.colorFormat =
            &colorRenderTarget->getImageForRenderPass().getFormat().actualImageFormat();
        params.colorAttachmentIndexGL = static_cast<uint32_t>(colorIndexGL);
        params.colorMaskFlags         = colorMaskFlags;
        if (mEmulatedAlphaAttachmentMask[colorIndexGL])
        {
            params.colorMaskFlags &= ~VK_COLOR_COMPONENT_A_BIT;
        }

        ANGLE_TRY(contextVk->getUtils().clearFramebuffer(contextVk, this, params));

        // Clear stencil only once!
        params.clearStencil = false;
    }

    // If there was no color clear, clear stencil alone.
    if (params.clearStencil)
    {
        params.clearColor = false;
        ANGLE_TRY(contextVk->getUtils().clearFramebuffer(contextVk, this, params));
    }

    return angle::Result::Continue;
}

VkClearValue FramebufferVk::getCorrectedColorClearValue(size_t colorIndexGL,
                                                        const VkClearColorValue &clearColor) const
{
    VkClearValue clearValue;
    clearValue.color = clearColor;

    if (!mEmulatedAlphaAttachmentMask[colorIndexGL])
    {
        return clearValue;
    }

    // If the render target doesn't have alpha, but its emulated format has it, clear the alpha
    // to 1.
    RenderTargetVk *renderTarget = getColorDrawRenderTarget(colorIndexGL);
    const vk::Format &format     = renderTarget->getImageFormat();
    if (format.vkFormatIsInt)
    {
        if (format.vkFormatIsUnsigned)
        {
            clearValue.color.uint32[3] = kEmulatedAlphaValue;
        }
        else
        {
            clearValue.color.int32[3] = kEmulatedAlphaValue;
        }
    }
    else
    {
        clearValue.color.float32[3] = kEmulatedAlphaValue;
    }

    return clearValue;
}

angle::Result FramebufferVk::clearWithLoadOp(ContextVk *contextVk,
                                             gl::DrawBufferMask clearColorBuffers,
                                             bool clearDepth,
                                             bool clearStencil,
                                             const VkClearColorValue &clearColorValue,
                                             const VkClearDepthStencilValue &clearDepthStencilValue)
{
    // Set the appropriate loadOp and clear values for depth and stencil.
    VkImageAspectFlags dsAspectFlags = 0;
    if (clearDepth)
    {
        dsAspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    if (clearStencil)
    {
        dsAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (contextVk->hasStartedRenderPass())
    {
        vk::CommandBufferHelper &commands = contextVk->getStartedRenderPassCommands();

        ASSERT(commands.getCommandBuffer().empty());

        vk::PackedAttachmentIndex colorIndexVk(0);
        for (size_t colorIndexGL : mState.getColorAttachmentsMask())
        {
            if (mState.getEnabledDrawBuffers()[colorIndexGL] && clearColorBuffers[colorIndexGL])
            {
                VkClearValue clearValue =
                    getCorrectedColorClearValue(colorIndexGL, clearColorValue);
                commands.updateRenderPassColorClear(colorIndexVk, clearValue);
            }
            ++colorIndexVk;
        }

        if (dsAspectFlags)
        {
            VkClearValue clearValue;
            clearValue.depthStencil = clearDepthStencilValue;
            commands.updateRenderPassDepthStencilClear(dsAspectFlags, clearValue);
            // If we were in depth read only mode, we must change to write mode
            ANGLE_TRY(updateRenderPassReadOnlyDepthMode(contextVk, &commands));
        }
    }
    else
    {
        for (size_t colorIndexGL : clearColorBuffers)
        {
            ASSERT(mState.getEnabledDrawBuffers().test(colorIndexGL));
            RenderTargetVk *renderTarget = getColorDrawRenderTarget(colorIndexGL);
            VkClearValue clearValue   = getCorrectedColorClearValue(colorIndexGL, clearColorValue);
            gl::ImageIndex imageIndex = renderTarget->getImageIndex();
            renderTarget->getImageForWrite().stageClear(imageIndex, VK_IMAGE_ASPECT_COLOR_BIT,
                                                        clearValue);
        }

        if (dsAspectFlags)
        {
            RenderTargetVk *renderTarget = getDepthStencilRenderTarget();
            ASSERT(renderTarget);

            VkClearValue clearValue;
            clearValue.depthStencil = clearDepthStencilValue;

            gl::ImageIndex imageIndex = renderTarget->getImageIndex();
            renderTarget->getImageForWrite().stageClear(imageIndex, dsAspectFlags, clearValue);
        }
    }
    return angle::Result::Continue;
}

angle::Result FramebufferVk::clearWithCommand(
    ContextVk *contextVk,
    vk::CommandBufferHelper *renderpassCommands,
    const gl::Rectangle &scissoredRenderArea,
    gl::DrawBufferMask clearColorBuffers,
    bool clearDepth,
    bool clearStencil,
    const VkClearColorValue &clearColorValue,
    const VkClearDepthStencilValue &clearDepthStencilValue)
{
    gl::DrawBuffersVector<VkClearAttachment> attachments;
    // Go through clearColorBuffers and add them to the list of attachments to clear.
    for (size_t colorIndexGL : clearColorBuffers)
    {
        ASSERT(mState.getEnabledDrawBuffers().test(colorIndexGL));
        VkClearValue clearValue = getCorrectedColorClearValue(colorIndexGL, clearColorValue);
        attachments.emplace_back(VkClearAttachment{
            VK_IMAGE_ASPECT_COLOR_BIT, static_cast<uint32_t>(colorIndexGL), clearValue});
    }

    // Add depth and stencil to list of attachments as needed.
    VkImageAspectFlags dsAspectFlags = 0;
    VkClearValue dsClearValue        = {};
    if (clearDepth)
    {
        dsAspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        dsClearValue.depthStencil = clearDepthStencilValue;
        // Explicitly mark a depth write because we are clearing the depth buffer.
        renderpassCommands->onDepthAccess(vk::ResourceAccess::Write);
    }

    if (clearStencil)
    {
        dsAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        dsClearValue.depthStencil = clearDepthStencilValue;
        // Explicitly mark a stencil write because we are clearing the stencil buffer.
        renderpassCommands->onStencilAccess(vk::ResourceAccess::Write);
    }

    if (dsAspectFlags != 0)
    {
        attachments.emplace_back(VkClearAttachment{dsAspectFlags, 0, dsClearValue});
        // Because we may have changed the depth stencil access mode, update read only depth mode
        // now.
        ANGLE_TRY(updateRenderPassReadOnlyDepthMode(contextVk, renderpassCommands));
    }

    VkClearRect rect                           = {};
    rect.rect.extent.width                     = scissoredRenderArea.width;
    rect.rect.extent.height                    = scissoredRenderArea.height;
    rect.layerCount                            = 1;
    vk::CommandBuffer *renderPassCommandBuffer = &renderpassCommands->getCommandBuffer();
    renderPassCommandBuffer->clearAttachments(static_cast<uint32_t>(attachments.size()),
                                              attachments.data(), 1, &rect);
    return angle::Result::Continue;
}

angle::Result FramebufferVk::getSamplePosition(const gl::Context *context,
                                               size_t index,
                                               GLfloat *xy) const
{
    int sampleCount = getSamples();
    rx::GetSamplePosition(sampleCount, index, xy);
    return angle::Result::Continue;
}

angle::Result FramebufferVk::startNewRenderPass(ContextVk *contextVk,
                                                bool readOnlyDepthMode,
                                                const gl::Rectangle &renderArea,
                                                vk::CommandBuffer **commandBufferOut)
{
    ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());

    // Initialize RenderPass info.
    vk::AttachmentOpsArray renderPassAttachmentOps;
    vk::PackedClearValuesArray packedClearValues;
    gl::DrawBufferMask previousUnresolveColorMask =
        mRenderPassDesc.getColorUnresolveAttachmentMask();
    bool previousUnresolveDepth   = mRenderPassDesc.hasDepthUnresolveAttachment();
    bool previousUnresolveStencil = mRenderPassDesc.hasStencilUnresolveAttachment();

    // Color attachments.
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    vk::PackedAttachmentIndex colorIndexVk(0);
    for (size_t colorIndexGL : mState.getColorAttachmentsMask())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
        ASSERT(colorRenderTarget);

        // Color render targets are never entirely transient.  Only depth/stencil
        // multisampled-render-to-texture textures can be so.
        ASSERT(!colorRenderTarget->isEntirelyTransient());

        renderPassAttachmentOps.setLayouts(colorIndexVk, vk::ImageLayout::ColorAttachment,
                                           vk::ImageLayout::ColorAttachment);

        const VkAttachmentStoreOp storeOp = colorRenderTarget->isImageTransient()
                                                ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                                                : VK_ATTACHMENT_STORE_OP_STORE;

        if (mDeferredClears.test(colorIndexGL))
        {
            renderPassAttachmentOps.setOps(colorIndexVk, VK_ATTACHMENT_LOAD_OP_CLEAR, storeOp);
            packedClearValues.store(colorIndexVk, VK_IMAGE_ASPECT_COLOR_BIT,
                                    mDeferredClears[colorIndexGL]);
            mDeferredClears.reset(colorIndexGL);
        }
        else
        {
            const VkAttachmentLoadOp loadOp = colorRenderTarget->hasDefinedContent()
                                                  ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                  : VK_ATTACHMENT_LOAD_OP_DONT_CARE;

            renderPassAttachmentOps.setOps(colorIndexVk, loadOp, storeOp);
            packedClearValues.store(colorIndexVk, VK_IMAGE_ASPECT_COLOR_BIT,
                                    kUninitializedClearValue);
        }
        renderPassAttachmentOps.setStencilOps(colorIndexVk, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                              VK_ATTACHMENT_STORE_OP_DONT_CARE);

        // If there's a resolve attachment, and loadOp needs to be LOAD, the multisampled attachment
        // needs to take its value from the resolve attachment.  In this case, an initial subpass is
        // added for this very purpose which uses the resolve attachment as input attachment.  As a
        // result, loadOp of the multisampled attachment can remain DONT_CARE.
        //
        // Note that this only needs to be done if the multisampled image and the resolve attachment
        // come from the same source.  isImageTransient() indicates whether this should happen.
        if (colorRenderTarget->hasResolveAttachment() && colorRenderTarget->isImageTransient())
        {
            if (renderPassAttachmentOps[colorIndexVk].loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            {
                renderPassAttachmentOps[colorIndexVk].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

                // Update the render pass desc to specify that this attachment should be unresolved.
                mRenderPassDesc.packColorUnresolveAttachment(colorIndexGL);
            }
            else
            {
                mRenderPassDesc.removeColorUnresolveAttachment(colorIndexGL);
            }
        }
        else
        {
            ASSERT(!mRenderPassDesc.getColorUnresolveAttachmentMask().test(colorIndexGL));
        }

        ++colorIndexVk;
    }

    // Depth/stencil attachment.
    vk::PackedAttachmentIndex depthStencilAttachmentIndex = vk::kAttachmentIndexInvalid;
    RenderTargetVk *depthStencilRenderTarget              = getDepthStencilRenderTarget();
    if (depthStencilRenderTarget)
    {
        const bool canExportStencil =
            contextVk->getRenderer()->getFeatures().supportsShaderStencilExport.enabled;

        // depth stencil attachment always immediately follows color attachment
        depthStencilAttachmentIndex = colorIndexVk;

        VkAttachmentLoadOp depthLoadOp     = VK_ATTACHMENT_LOAD_OP_LOAD;
        VkAttachmentLoadOp stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_LOAD;
        VkAttachmentStoreOp depthStoreOp   = VK_ATTACHMENT_STORE_OP_STORE;
        VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

        // If the image data was previously discarded (with no update in between), don't attempt to
        // load the image.  Additionally, if the multisampled image data is transient and there is
        // no resolve attachment, there's no data to load.  The latter is the case with
        // depth/stencil texture attachments per GL_EXT_multisampled_render_to_texture2.
        if (!depthStencilRenderTarget->hasDefinedContent() ||
            depthStencilRenderTarget->isEntirelyTransient())
        {
            depthLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        // If depth/stencil image is transient, no need to store its data at the end of the render
        // pass.  If shader stencil export is not supported, stencil data cannot be unresolved on
        // the next render pass, so it must be stored/loaded.  If the image is entirely transient,
        // there is no resolve/unresolve and the image data is never stored/loaded.
        if (depthStencilRenderTarget->isImageTransient())
        {
            depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            if (canExportStencil || depthStencilRenderTarget->isEntirelyTransient())
            {
                stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }
        }

        if (mDeferredClears.testDepth() || mDeferredClears.testStencil())
        {
            VkClearValue clearValue;

            if (mDeferredClears.testDepth())
            {
                depthLoadOp                   = VK_ATTACHMENT_LOAD_OP_CLEAR;
                clearValue.depthStencil.depth = mDeferredClears.getDepthValue();
                mDeferredClears.reset(vk::kUnpackedDepthIndex);
            }

            if (mDeferredClears.testStencil())
            {
                stencilLoadOp                   = VK_ATTACHMENT_LOAD_OP_CLEAR;
                clearValue.depthStencil.stencil = mDeferredClears.getStencilValue();
                mDeferredClears.reset(vk::kUnpackedStencilIndex);
            }

            // Note the aspect is only depth here. That's intentional.
            packedClearValues.store(depthStencilAttachmentIndex, VK_IMAGE_ASPECT_DEPTH_BIT,
                                    clearValue);
        }
        else
        {
            // Note the aspect is only depth here. That's intentional.
            packedClearValues.store(depthStencilAttachmentIndex, VK_IMAGE_ASPECT_DEPTH_BIT,
                                    kUninitializedClearValue);
        }

        const vk::Format &format = depthStencilRenderTarget->getImageFormat();
        // If the format we picked has stencil but user did not ask for it due to hardware
        // limitations, use DONT_CARE for load/store. The same logic for depth follows.
        if (format.intendedFormat().stencilBits == 0)
        {
            stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        if (format.intendedFormat().depthBits == 0)
        {
            depthLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        // Similar to color attachments, if there's a resolve attachment and the multisampled image
        // is transient, depth/stencil data need to be unresolved in an initial subpass.
        //
        // Note that stencil unresolve is currently only possible if shader stencil export is
        // supported.
        if (depthStencilRenderTarget->hasResolveAttachment() &&
            depthStencilRenderTarget->isImageTransient())
        {
            const bool unresolveDepth = depthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD;
            const bool unresolveStencil =
                stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD && canExportStencil;

            if (unresolveDepth)
            {
                depthLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }

            if (unresolveStencil)
            {
                stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }

            if (unresolveDepth || unresolveStencil)
            {
                mRenderPassDesc.packDepthStencilUnresolveAttachment(unresolveDepth,
                                                                    unresolveStencil);
            }
            else
            {
                mRenderPassDesc.removeDepthStencilUnresolveAttachment();
            }
        }

        renderPassAttachmentOps.setOps(depthStencilAttachmentIndex, depthLoadOp, depthStoreOp);
        renderPassAttachmentOps.setStencilOps(depthStencilAttachmentIndex, stencilLoadOp,
                                              stencilStoreOp);

        // We can only start the renderpass in read only mode if it is requested to be read only and
        // we are not doing clear.
        bool depthStencilReadOnly =
            readOnlyDepthMode && !depthStencilRenderTarget->hasResolveAttachment() &&
            renderPassAttachmentOps[depthStencilAttachmentIndex].loadOp !=
                VK_ATTACHMENT_LOAD_OP_CLEAR &&
            renderPassAttachmentOps[depthStencilAttachmentIndex].stencilLoadOp !=
                VK_ATTACHMENT_LOAD_OP_CLEAR;
        setReadOnlyDepthMode(depthStencilReadOnly);

        vk::ImageLayout dsLayout = mReadOnlyDepthStencilMode
                                       ? vk::ImageLayout::DepthStencilReadOnly
                                       : vk::ImageLayout::DepthStencilAttachment;

        renderPassAttachmentOps.setLayouts(depthStencilAttachmentIndex, dsLayout, dsLayout);
    }

    // If render pass description is changed, the previous render pass desc is no longer compatible.
    // Tell the context so that the graphics pipelines can be recreated.
    //
    // Note that render passes are compatible only if the differences are in loadOp/storeOp values,
    // or the existence of resolve attachments in single subpass render passes.  The modification
    // here can add/remove a subpass, or modify its input attachments.
    gl::DrawBufferMask unresolveColorMask = mRenderPassDesc.getColorUnresolveAttachmentMask();
    bool unresolveDepth                   = mRenderPassDesc.hasDepthUnresolveAttachment();
    bool unresolveStencil                 = mRenderPassDesc.hasStencilUnresolveAttachment();
    if (previousUnresolveColorMask != unresolveColorMask ||
        previousUnresolveDepth != unresolveDepth || previousUnresolveStencil != unresolveStencil)
    {
        contextVk->onDrawFramebufferRenderPassDescChange(this);

        // Make sure framebuffer is recreated.
        mFramebuffer = nullptr;

        vk::FramebufferNonResolveAttachmentMask unresolveMask(unresolveColorMask.bits());
        if (unresolveDepth)
        {
            unresolveMask.set(vk::kUnpackedDepthIndex);
        }
        if (unresolveStencil)
        {
            unresolveMask.set(vk::kUnpackedStencilIndex);
        }

        mCurrentFramebufferDesc.updateUnresolveMask(unresolveMask);
    }

    vk::Framebuffer *framebuffer = nullptr;
    ANGLE_TRY(getFramebuffer(contextVk, &framebuffer, nullptr));

    ANGLE_TRY(contextVk->beginNewRenderPass(*framebuffer, renderArea, mRenderPassDesc,
                                            renderPassAttachmentOps, depthStencilAttachmentIndex,
                                            packedClearValues, commandBufferOut));

    // Transition the images to the correct layout (through onColorDraw).
    for (size_t colorIndexGL : mState.getColorAttachmentsMask())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndexGL];
        colorRenderTarget->onColorDraw(contextVk);
    }

    if (depthStencilRenderTarget)
    {
        // This must be called after hasDefinedContent() since it will set content to valid. We are
        // tracking content valid very loosely here that as long as it is attached, it assumes will
        // have valid content. The only time it has undefined content is between swap and
        // startNewRenderPass
        depthStencilRenderTarget->onDepthStencilDraw(contextVk, mReadOnlyDepthStencilMode);
    }

    if (unresolveColorMask.any() || unresolveDepth || unresolveStencil)
    {
        // Unresolve attachments if any.
        UtilsVk::UnresolveParameters params;
        params.unresolveColorMask = unresolveColorMask;
        params.unresolveDepth     = unresolveDepth;
        params.unresolveStencil   = unresolveStencil;

        ANGLE_TRY(contextVk->getUtils().unresolve(contextVk, this, params));

        // The unresolve subpass has only one draw call.
        contextVk->startNextSubpass();
    }

    return angle::Result::Continue;
}

void FramebufferVk::restoreDepthStencilDefinedContents()
{
    // If the depthStencilRenderTarget does not have "defined content" (i.e. meaning that a future
    // render pass should use a loadOp of DONT_CARE), we should restore it (i.e. so that a future
    // render pass uses a loadOp of LOAD).
    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        depthStencilRenderTarget->restoreEntireContent();
    }
}

void FramebufferVk::updateActiveColorMasks(size_t colorIndexGL, bool r, bool g, bool b, bool a)
{
    mActiveColorComponentMasksForClear[0].set(colorIndexGL, r);
    mActiveColorComponentMasksForClear[1].set(colorIndexGL, g);
    mActiveColorComponentMasksForClear[2].set(colorIndexGL, b);
    mActiveColorComponentMasksForClear[3].set(colorIndexGL, a);
}

const gl::DrawBufferMask &FramebufferVk::getEmulatedAlphaAttachmentMask() const
{
    return mEmulatedAlphaAttachmentMask;
}

angle::Result FramebufferVk::readPixelsImpl(ContextVk *contextVk,
                                            const gl::Rectangle &area,
                                            const PackPixelsParams &packPixelsParams,
                                            VkImageAspectFlagBits copyAspectFlags,
                                            RenderTargetVk *renderTarget,
                                            void *pixels)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "FramebufferVk::readPixelsImpl");
    gl::LevelIndex levelGL = renderTarget->getLevelIndex();
    uint32_t layer         = renderTarget->getLayerIndex();
    return renderTarget->getImageForCopy().readPixels(contextVk, area, packPixelsParams,
                                                      copyAspectFlags, levelGL, layer, pixels,
                                                      &mReadPixelBuffer);
}

gl::Extents FramebufferVk::getReadImageExtents() const
{
    RenderTargetVk *readRenderTarget = mRenderTargetCache.getColorRead(mState);

    ASSERT(readRenderTarget->getExtents().width == mState.getDimensions().width);
    ASSERT(readRenderTarget->getExtents().height == mState.getDimensions().height);

    return readRenderTarget->getExtents();
}

// Return the framebuffer's non-rotated render area.  This is a gl::Rectangle that is based on the
// dimensions of the framebuffer, IS NOT rotated, and IS NOT y-flipped
gl::Rectangle FramebufferVk::getNonRotatedCompleteRenderArea() const
{
    const gl::Box &dimensions = mState.getDimensions();
    return gl::Rectangle(0, 0, dimensions.width, dimensions.height);
}

// Return the framebuffer's rotated render area.  This is a gl::Rectangle that is based on the
// dimensions of the framebuffer, IS ROTATED for the draw FBO, and IS NOT y-flipped
//
// Note: Since the rectangle is not scissored (i.e. x and y are guaranteed to be zero), only the
// width and height must be swapped if the rotation is 90 or 270 degrees.
gl::Rectangle FramebufferVk::getRotatedCompleteRenderArea(ContextVk *contextVk) const
{
    gl::Rectangle renderArea = getNonRotatedCompleteRenderArea();
    if (contextVk->isRotatedAspectRatioForDrawFBO())
    {
        // The surface is rotated 90/270 degrees.  This changes the aspect ratio of the surface.
        std::swap(renderArea.width, renderArea.height);
    }
    return renderArea;
}

// Return the framebuffer's scissored and rotated render area.  This is a gl::Rectangle that is
// based on the dimensions of the framebuffer, is clipped to the scissor, IS ROTATED and IS
// Y-FLIPPED for the draw FBO.
//
// Note: Since the rectangle is scissored, it must be fully rotated, and not just have the width
// and height swapped.
gl::Rectangle FramebufferVk::getRotatedScissoredRenderArea(ContextVk *contextVk) const
{
    const gl::Rectangle renderArea = getNonRotatedCompleteRenderArea();
    bool invertViewport            = contextVk->isViewportFlipEnabledForDrawFBO();
    gl::Rectangle scissoredArea    = ClipRectToScissor(contextVk->getState(), renderArea, false);
    gl::Rectangle rotatedScissoredArea;
    RotateRectangle(contextVk->getRotationDrawFramebuffer(), invertViewport, renderArea.width,
                    renderArea.height, scissoredArea, &rotatedScissoredArea);
    return rotatedScissoredArea;
}

RenderTargetVk *FramebufferVk::getFirstRenderTarget() const
{
    for (auto *renderTarget : mRenderTargetCache.getColors())
    {
        if (renderTarget)
        {
            return renderTarget;
        }
    }

    return getDepthStencilRenderTarget();
}

GLint FramebufferVk::getSamples() const
{
    RenderTargetVk *firstRT = getFirstRenderTarget();
    return firstRT ? firstRT->getImageForRenderPass().getSamples() : 1;
}

angle::Result FramebufferVk::flushDeferredClears(ContextVk *contextVk,
                                                 const gl::Rectangle &renderArea)
{
    if (mDeferredClears.empty())
        return angle::Result::Continue;

    return contextVk->startRenderPass(renderArea, nullptr);
}

void FramebufferVk::setReadOnlyDepthMode(bool readOnlyDepthEnabled)
{
    if (mReadOnlyDepthStencilMode != readOnlyDepthEnabled)
    {
        mReadOnlyDepthStencilMode = readOnlyDepthEnabled;

        ASSERT(getDepthStencilRenderTarget());
        vk::ResourceAccess dsAccess =
            isReadOnlyDepthMode() ? vk::ResourceAccess::ReadOnly : vk::ResourceAccess::Write;
        ASSERT(mRenderPassDesc.hasDepthStencilAttachment());
        mRenderPassDesc.updateDepthStencilAccess(dsAccess);
    }
}

angle::Result FramebufferVk::updateRenderPassReadOnlyDepthMode(ContextVk *contextVk,
                                                               vk::CommandBufferHelper *renderPass)
{
    ASSERT(getDepthStencilRenderTarget());

    bool readOnlyDepthStencil =
        !getDepthStencilRenderTarget()->hasResolveAttachment() &&
        (mReadOnlyDepthFeedbackLoopMode || !renderPass->hasDepthStencilWriteOrClear());
    if (readOnlyDepthStencil == mReadOnlyDepthStencilMode)
    {
        return angle::Result::Continue;
    }

    // If readOnlyDepthStencil is false, we are switching out of read only mode due to depth write.
    // We must not be in the read only feedback loop mode because the logic in
    // ContextVk::updateRenderPassDepthStencilAccess() should ensure we end the previous renderpass
    // and a new renderpass will start with feedback loop disabled.
    ASSERT(readOnlyDepthStencil || !mReadOnlyDepthFeedbackLoopMode);

    setReadOnlyDepthMode(readOnlyDepthStencil);

    // When we toggle read/write mode, we must insert a layout transition.
    getDepthStencilRenderTarget()->onDepthStencilDraw(contextVk, readOnlyDepthStencil);

    vk::Framebuffer *currentFramebuffer = nullptr;
    ANGLE_TRY(getFramebuffer(contextVk, &currentFramebuffer, nullptr));

    renderPass->updateStartedRenderPassWithDepthMode(*currentFramebuffer, mRenderPassDesc,
                                                     readOnlyDepthStencil);

    return angle::Result::Continue;
}
}  // namespace rx
