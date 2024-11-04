// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn/native/d3d12/TextureD3D12.h"

#include <algorithm>
#include <utility>

#include "absl/numeric/bits.h"
#include "dawn/common/Constants.h"
#include "dawn/common/Math.h"
#include "dawn/native/DynamicUploader.h"
#include "dawn/native/EnumMaskIterator.h"
#include "dawn/native/Error.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/ResourceMemoryAllocation.h"
#include "dawn/native/ToBackend.h"
#include "dawn/native/d3d/D3DError.h"
#include "dawn/native/d3d12/BufferD3D12.h"
#include "dawn/native/d3d12/CommandRecordingContext.h"
#include "dawn/native/d3d12/DeviceD3D12.h"
#include "dawn/native/d3d12/Forward.h"
#include "dawn/native/d3d12/HeapD3D12.h"
#include "dawn/native/d3d12/ResourceAllocatorManagerD3D12.h"
#include "dawn/native/d3d12/SharedFenceD3D12.h"
#include "dawn/native/d3d12/SharedTextureMemoryD3D12.h"
#include "dawn/native/d3d12/StagingDescriptorAllocatorD3D12.h"
#include "dawn/native/d3d12/TextureCopySplitter.h"
#include "dawn/native/d3d12/UtilsD3D12.h"

namespace dawn::native::d3d12 {

namespace {

D3D12_RESOURCE_STATES D3D12TextureUsage(wgpu::TextureUsage usage, const Format& format) {
    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

    if (usage & kPresentTextureUsage) {
        // The present usage is only used internally by the swapchain and is never used in
        // combination with other usages.
        ASSERT(usage == kPresentTextureUsage);
        return D3D12_RESOURCE_STATE_PRESENT;
    }

    if (usage & wgpu::TextureUsage::CopySrc) {
        resourceState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (usage & wgpu::TextureUsage::CopyDst) {
        resourceState |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (usage & (wgpu::TextureUsage::TextureBinding | kReadOnlyStorageTexture)) {
        resourceState |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    }
    if (usage & wgpu::TextureUsage::StorageBinding) {
        resourceState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if (usage & wgpu::TextureUsage::RenderAttachment) {
        if (format.HasDepthOrStencil()) {
            resourceState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        } else {
            resourceState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        }
    }

    if (usage & kReadOnlyRenderAttachment) {
        // There is no STENCIL_READ state. Readonly for stencil is bundled with DEPTH_READ.
        resourceState |= D3D12_RESOURCE_STATE_DEPTH_READ;
    }

    return resourceState;
}

D3D12_RESOURCE_FLAGS D3D12ResourceFlags(wgpu::TextureUsage usage, const Format& format) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (usage & wgpu::TextureUsage::StorageBinding) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    if (usage & wgpu::TextureUsage::RenderAttachment) {
        if (format.HasDepthOrStencil()) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        } else {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
    }

    ASSERT(!(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ||
           flags == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    return flags;
}

D3D12_RESOURCE_DIMENSION D3D12TextureDimension(wgpu::TextureDimension dimension) {
    switch (dimension) {
        case wgpu::TextureDimension::e1D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        case wgpu::TextureDimension::e2D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        case wgpu::TextureDimension::e3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }
}

}  // namespace

MaybeError ValidateTextureCanBeWrapped(ID3D12Resource* d3d12Resource,
                                       const TextureDescriptor* dawnDescriptor) {
    const D3D12_RESOURCE_DESC d3dDescriptor = d3d12Resource->GetDesc();
    DAWN_INVALID_IF(
        (dawnDescriptor->size.width != d3dDescriptor.Width) ||
            (dawnDescriptor->size.height != d3dDescriptor.Height) ||
            (dawnDescriptor->size.depthOrArrayLayers != 1),
        "D3D12 texture size (Width: %u, Height: %u, DepthOrArraySize: 1) doesn't match Dawn "
        "descriptor size (width: %u, height: %u, depthOrArrayLayers: %u).",
        d3dDescriptor.Width, d3dDescriptor.Height, dawnDescriptor->size.width,
        dawnDescriptor->size.height, dawnDescriptor->size.depthOrArrayLayers);

    const DXGI_FORMAT dxgiFormatFromDescriptor = d3d::DXGITextureFormat(dawnDescriptor->format);
    DAWN_INVALID_IF(dxgiFormatFromDescriptor != d3dDescriptor.Format,
                    "D3D12 texture format (%x) is not compatible with Dawn descriptor format (%s).",
                    d3dDescriptor.Format, dawnDescriptor->format);

    DAWN_INVALID_IF(d3dDescriptor.MipLevels != 1,
                    "D3D12 texture number of miplevels (%u) is not 1.", d3dDescriptor.MipLevels);

    DAWN_INVALID_IF(d3dDescriptor.DepthOrArraySize != 1, "D3D12 texture array size (%u) is not 1.",
                    d3dDescriptor.DepthOrArraySize);

    // Shared textures cannot be multi-sample so no need to check those.
    ASSERT(d3dDescriptor.SampleDesc.Count == 1);
    ASSERT(d3dDescriptor.SampleDesc.Quality == 0);

    return {};
}

// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shared_resource_compatibility_tier
MaybeError ValidateVideoTextureCanBeShared(Device* device, DXGI_FORMAT textureFormat) {
    const bool supportsSharedResourceCapabilityTier1 =
        device->GetDeviceInfo().supportsSharedResourceCapabilityTier1;
    switch (textureFormat) {
        // MSDN docs are not correct, NV12 requires at-least tier 1.
        case DXGI_FORMAT_NV12:
            if (supportsSharedResourceCapabilityTier1) {
                return {};
            }
            break;
        default:
            break;
    }

    return DAWN_VALIDATION_ERROR("DXGI format does not support cross-API sharing.");
}

// static
ResultOrError<Ref<Texture>> Texture::Create(Device* device, const TextureDescriptor* descriptor) {
    Ref<Texture> dawnTexture = AcquireRef(new Texture(device, descriptor));

    DAWN_INVALID_IF(dawnTexture->GetFormat().IsMultiPlanar(),
                    "Cannot create a multi-planar formatted texture directly");

    DAWN_TRY(dawnTexture->InitializeAsInternalTexture());
    return std::move(dawnTexture);
}

// static
ResultOrError<Ref<Texture>> Texture::CreateExternalImage(Device* device,
                                                         const TextureDescriptor* descriptor,
                                                         ComPtr<IUnknown> d3dTexture,
                                                         std::vector<Ref<d3d::Fence>> waitFences,
                                                         bool isSwapChainTexture,
                                                         bool isInitialized) {
    Ref<Texture> dawnTexture = AcquireRef(new Texture(device, descriptor));
    DAWN_TRY(dawnTexture->InitializeAsExternalTexture(std::move(d3dTexture), std::move(waitFences),
                                                      isSwapChainTexture));

    // Importing a multi-planar format must be initialized. This is required because
    // a shared multi-planar format cannot be initialized by Dawn.
    DAWN_INVALID_IF(
        !isInitialized && dawnTexture->GetFormat().IsMultiPlanar(),
        "Cannot create a texture with a multi-planar format (%s) with uninitialized data.",
        dawnTexture->GetFormat().format);

    dawnTexture->SetIsSubresourceContentInitialized(isInitialized,
                                                    dawnTexture->GetAllSubresources());
    return std::move(dawnTexture);
}

// static
ResultOrError<Ref<Texture>> Texture::Create(Device* device,
                                            const TextureDescriptor* descriptor,
                                            ComPtr<ID3D12Resource> d3d12Texture) {
    Ref<Texture> dawnTexture = AcquireRef(new Texture(device, descriptor));
    DAWN_TRY(dawnTexture->InitializeAsSwapChainTexture(std::move(d3d12Texture)));
    return std::move(dawnTexture);
}

// static
ResultOrError<Ref<Texture>> Texture::CreateFromSharedTextureMemory(
    SharedTextureMemory* memory,
    const TextureDescriptor* descriptor) {
    Device* device = ToBackend(memory->GetDevice());
    Ref<Texture> texture = AcquireRef(new Texture(device, descriptor));
    DAWN_TRY(texture->InitializeAsExternalTexture(memory->GetD3DResource(), {}, false));
    texture->mSharedTextureMemoryState = memory->GetState();
    return texture;
}

MaybeError Texture::InitializeAsExternalTexture(ComPtr<IUnknown> d3dTexture,
                                                std::vector<Ref<d3d::Fence>> waitFences,
                                                bool isSwapChainTexture) {
    ComPtr<ID3D12Resource> d3d12Texture;
    DAWN_TRY(CheckHRESULT(d3dTexture.As(&d3d12Texture), "texture is not a valid ID3D12Resource"));

    D3D12_RESOURCE_DESC desc = d3d12Texture->GetDesc();
    mD3D12ResourceFlags = desc.Flags;
    ASSERT(mD3D12ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);

    AllocationInfo info;
    info.mMethod = AllocationMethod::kExternal;
    // When creating the ResourceHeapAllocation, the resource heap is set to nullptr because the
    // texture is owned externally. The texture's owning entity must remain responsible for
    // memory management.
    mResourceAllocation = {info, 0, std::move(d3d12Texture), nullptr};

    mWaitFences = std::move(waitFences);
    mSwapChainTexture = isSwapChainTexture;

    SetLabelHelper("Dawn_ExternalTexture");

    return {};
}

MaybeError Texture::InitializeAsInternalTexture() {
    D3D12_RESOURCE_DESC resourceDescriptor;
    resourceDescriptor.Dimension = D3D12TextureDimension(GetDimension());
    resourceDescriptor.Alignment = 0;

    const Extent3D& size = GetSize();
    resourceDescriptor.Width = size.width;
    resourceDescriptor.Height = size.height;
    resourceDescriptor.DepthOrArraySize = size.depthOrArrayLayers;

    Device* device = ToBackend(GetDevice());
    // When the depth stencil texture is created on a not-zeroed heap, its first usage will also be
    // copy destination when it is initialized with a non-zero value, which also triggered the issue
    // about copying data into a placed depth stencil texture with a dirty memory, so in this
    // situation the workaround should also be enabled.
    bool applyForceClearCopyableDepthStencilTextureOnCreationToggle =
        device->IsToggleEnabled(Toggle::D3D12ForceClearCopyableDepthStencilTextureOnCreation) &&
        GetFormat().HasDepthOrStencil() &&
        ((GetInternalUsage() & wgpu::TextureUsage::CopyDst) ||
         (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
          device->IsToggleEnabled(Toggle::D3D12CreateNotZeroedHeap)));
    if (applyForceClearCopyableDepthStencilTextureOnCreationToggle) {
        AddInternalUsage(wgpu::TextureUsage::RenderAttachment);
    }

    // This will need to be much more nuanced when WebGPU has
    // texture view compatibility rules.
    const bool needsTypelessFormat =
        (GetDevice()->IsToggleEnabled(Toggle::D3D12AlwaysUseTypelessFormatsForCastableTexture) &&
         GetViewFormats().any()) ||
        (GetFormat().HasDepthOrStencil() &&
         (GetInternalUsage() & wgpu::TextureUsage::TextureBinding) != 0);

    DXGI_FORMAT dxgiFormat = needsTypelessFormat
                                 ? d3d::DXGITypelessTextureFormat(GetFormat().format)
                                 : d3d::DXGITextureFormat(GetFormat().format);

    resourceDescriptor.MipLevels = static_cast<UINT16>(GetNumMipLevels());
    resourceDescriptor.Format = dxgiFormat;
    resourceDescriptor.SampleDesc.Count = GetSampleCount();
    resourceDescriptor.SampleDesc.Quality = 0;
    resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescriptor.Flags = D3D12ResourceFlags(GetInternalUsage(), GetFormat());
    mD3D12ResourceFlags = resourceDescriptor.Flags;

    uint32_t bytesPerBlock = 0;
    if (GetFormat().IsColor()) {
        bytesPerBlock = GetFormat().GetAspectInfo(wgpu::TextureAspect::All).block.byteSize;
    }
    bool forceAllocateAsCommittedResource =
        (device->IsToggleEnabled(
            Toggle::DisableSubAllocationFor2DTextureWithCopyDstOrRenderAttachment)) &&
        GetDimension() == wgpu::TextureDimension::e2D &&
        (GetInternalUsage() & (wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::RenderAttachment));
    DAWN_TRY_ASSIGN(mResourceAllocation,
                    device->AllocateMemory(D3D12_HEAP_TYPE_DEFAULT, resourceDescriptor,
                                           D3D12_RESOURCE_STATE_COMMON, bytesPerBlock,
                                           forceAllocateAsCommittedResource));

    SetLabelImpl();

    if (applyForceClearCopyableDepthStencilTextureOnCreationToggle ||
        device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)) {
        CommandRecordingContext* commandContext;
        DAWN_TRY_ASSIGN(commandContext, device->GetPendingCommandContext());
        ClearValue clearValue =
            device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)
                ? ClearValue::NonZero
                : ClearValue::Zero;
        DAWN_TRY(ClearTexture(commandContext, GetAllSubresources(), clearValue));
    }

    return {};
}

MaybeError Texture::InitializeAsSwapChainTexture(ComPtr<ID3D12Resource> d3d12Texture) {
    AllocationInfo info;
    info.mMethod = AllocationMethod::kExternal;

    D3D12_RESOURCE_DESC desc = d3d12Texture->GetDesc();
    mD3D12ResourceFlags = desc.Flags;

    // When creating the ResourceHeapAllocation, the resource heap is set to nullptr because the
    // texture is owned externally. The texture's owning entity must remain responsible for
    // memory management.
    mResourceAllocation = {info, 0, std::move(d3d12Texture), nullptr};

    SetLabelHelper("Dawn_SwapChainTexture");

    return {};
}

Texture::Texture(Device* device, const TextureDescriptor* descriptor)
    : Base(device, descriptor), mSubresourceStateAndDecay(InitialSubresourceStateAndDecay()) {}

Texture::~Texture() = default;

void Texture::DestroyImpl() {
    TextureBase::DestroyImpl();
    ToBackend(GetDevice())->DeallocateMemory(mResourceAllocation);
    // Set mSwapChainTexture to false to prevent ever calling ID3D12SharingContract::Present again.
    mSwapChainTexture = false;
}

ResultOrError<ExecutionSerial> Texture::EndAccess() {
    ASSERT(mD3D12ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);

    Device* device = ToBackend(GetDevice());
    // Synchronize if texture access wasn't synchronized already due to ExecuteCommandLists.
    if (!mSignalFenceValue.has_value()) {
        // Needed to ensure that command allocator doesn't get destroyed before pending commands
        // are submitted due to calling NextSerial(). No-op if there are no pending commands.
        DAWN_TRY(device->ExecutePendingCommandContext());
        // If there were pending commands that used this texture mSignalFenceValue will be set,
        // but if it's still not set, generate a signal fence after waiting on wait fences.
        if (!mSignalFenceValue.has_value()) {
            DAWN_TRY(SynchronizeImportedTextureBeforeUse());
            DAWN_TRY(SynchronizeImportedTextureAfterUse());
        }
        DAWN_TRY(device->NextSerial());
        ASSERT(mSignalFenceValue.has_value());
    }
    ExecutionSerial ret = mSignalFenceValue.value();
    ASSERT(ret <= device->GetLastSubmittedCommandSerial());
    // Explicitly call reset() since std::move() on optional doesn't make it std::nullopt.
    mSignalFenceValue.reset();
    return ret;
}

DXGI_FORMAT Texture::GetD3D12Format() const {
    return d3d::DXGITextureFormat(GetFormat().format);
}

ID3D12Resource* Texture::GetD3D12Resource() const {
    return mResourceAllocation.GetD3D12Resource();
}

D3D12_RESOURCE_FLAGS Texture::GetD3D12ResourceFlags() const {
    return mD3D12ResourceFlags;
}

DXGI_FORMAT Texture::GetD3D12CopyableSubresourceFormat(Aspect aspect) const {
    ASSERT(GetFormat().aspects & aspect);

    switch (GetFormat().format) {
        case wgpu::TextureFormat::Depth24PlusStencil8:
        case wgpu::TextureFormat::Depth32FloatStencil8:
        case wgpu::TextureFormat::Stencil8:
            switch (aspect) {
                case Aspect::Depth:
                    return DXGI_FORMAT_R32_FLOAT;
                case Aspect::Stencil:
                    return DXGI_FORMAT_R8_UINT;
                default:
                    UNREACHABLE();
            }
        default:
            ASSERT(HasOneBit(GetFormat().aspects));
            return GetD3D12Format();
    }
}

MaybeError Texture::SynchronizeImportedTextureBeforeUse() {
    // Perform the wait only on the first call.
    Device* device = ToBackend(GetDevice());
    for (Ref<d3d::Fence>& fence : mWaitFences) {
        DAWN_TRY(CheckHRESULT(
                     device->GetCommandQueue()->Wait(
                         static_cast<Fence*>(fence.Get())->GetD3D12Fence(), fence->GetFenceValue()),
                     "D3D12 fence wait"););
        // Keep D3D12 fence alive since we'll clear the waitFences list below.
        device->ReferenceUntilUnused(static_cast<Fence*>(fence.Get())->GetD3D12Fence());
    }
    mWaitFences.clear();

    SharedTextureMemoryBase::PendingFenceList fences;
    SharedTextureMemoryState* memoryState = GetSharedTextureMemoryState();
    if (memoryState != nullptr) {
        memoryState->AcquirePendingFences(&fences);
        memoryState->SetLastUsageSerial(GetDevice()->GetPendingCommandSerial());
    }

    for (const auto& fence : fences) {
        DAWN_TRY(CheckHRESULT(device->GetCommandQueue()->Wait(
                                  ToBackend(fence.object)->GetD3DFence(), fence.signaledValue),
                              "D3D12 fence wait"));
        // Keep D3D12 fence alive until commands complete.
        device->ReferenceUntilUnused(ToBackend(fence.object)->GetD3DFence());
    }
    return {};
}

MaybeError Texture::SynchronizeImportedTextureAfterUse() {
    // In PIX's D3D12-only mode, there is no way to determine frame boundaries
    // for WebGPU since Dawn does not manage DXGI swap chains. Without assistance,
    // PIX will wait forever for a present that never happens.
    // If we know we're dealing with a swapbuffer texture, inform PIX we've
    // "presented" the texture so it can determine frame boundaries and use its
    // contents for the UI.
    Device* device = ToBackend(GetDevice());
    if (mSwapChainTexture) {
        ID3D12SharingContract* d3dSharingContract = device->GetSharingContract();
        if (d3dSharingContract != nullptr) {
            d3dSharingContract->Present(mResourceAllocation.GetD3D12Resource(), 0, 0);
        }
    }
    // NextSerial() will be called after this - this is also checked in EndAccess().
    mSignalFenceValue = device->GetPendingCommandSerial();
    return {};
}

void Texture::TrackUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                         wgpu::TextureUsage usage,
                                         const SubresourceRange& range) {
    TrackUsageAndTransitionNow(commandContext, D3D12TextureUsage(usage, GetFormat()), range);
}

void Texture::TrackUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                         D3D12_RESOURCE_STATES newState,
                                         const SubresourceRange& range) {
    if (mResourceAllocation.GetInfo().mMethod != AllocationMethod::kExternal) {
        // Track the underlying heap to ensure residency.
        Heap* heap = ToBackend(mResourceAllocation.GetResourceHeap());
        commandContext->TrackHeapUsage(heap, GetDevice()->GetPendingCommandSerial());
    }

    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    uint32_t aspectCount = absl::popcount(static_cast<uint8_t>(range.aspects));
    barriers.reserve(range.levelCount * range.layerCount * aspectCount);

    TransitionUsageAndGetResourceBarrier(commandContext, &barriers, newState, range);
    if (barriers.size()) {
        commandContext->GetCommandList()->ResourceBarrier(barriers.size(), barriers.data());
    }
}

void Texture::TransitionSubresourceRange(std::vector<D3D12_RESOURCE_BARRIER>* barriers,
                                         const SubresourceRange& range,
                                         StateAndDecay* state,
                                         D3D12_RESOURCE_STATES newState,
                                         ExecutionSerial pendingCommandSerial) const {
    D3D12_RESOURCE_STATES lastState = state->lastState;

    // If the transition is from-UAV-to-UAV, then a UAV barrier is needed.
    // If one of the usages isn't UAV, then other barriers are used.
    bool needsUAVBarrier = lastState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
                           newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (needsUAVBarrier) {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = GetD3D12Resource();
        barriers->push_back(barrier);
        return;
    }

    // Reuse the subresource(s) directly and avoid transition when it isn't needed, and
    // return false.
    if (lastState == newState) {
        return;
    }

    // The COMMON state represents a state where no write operations can be pending, and
    // where all pixels are uncompressed. This makes it possible to transition to and
    // from some states without synchronization (i.e. without an explicit
    // ResourceBarrier call). Textures can be implicitly promoted to 1) a single write
    // state, or 2) multiple read states. Textures will implicitly decay to the COMMON
    // state when all of the following are true: 1) the texture is accessed on a command
    // list, 2) the ExecuteCommandLists call that uses that command list has ended, and
    // 3) the texture was promoted implicitly to a read-only state and is still in that
    // state.
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions

    // To track implicit decays, we must record the pending serial on which that
    // transition will occur. When that texture is used again, the previously recorded
    // serial must be compared to the last completed serial to determine if the texture
    // has implicity decayed to the common state.
    if (state->isValidToDecay && pendingCommandSerial > state->lastDecaySerial) {
        lastState = D3D12_RESOURCE_STATE_COMMON;
    }

    // Update the tracked state.
    state->lastState = newState;

    // All simultaneous-access textures are qualified for an implicit promotion.
    // Destination states that qualify for an implicit promotion for a
    // non-simultaneous-access texture: NON_PIXEL_SHADER_RESOURCE,
    // PIXEL_SHADER_RESOURCE, COPY_SRC, COPY_DEST.
    {
        const D3D12_RESOURCE_STATES kD3D12PromotableReadOnlyStates =
            D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

        if (lastState == D3D12_RESOURCE_STATE_COMMON) {
            if (IsSubset(newState, kD3D12PromotableReadOnlyStates) ||
                mD3D12ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) {
                // Implicit texture state decays can only occur when the texture was implicitly
                // transitioned to a read-only state. isValidToDecay is needed to differentiate
                // between resources that were implictly or explicitly transitioned to a
                // read-only state.
                state->isValidToDecay = true;
                state->lastDecaySerial = pendingCommandSerial;
                return;
            } else if (newState == D3D12_RESOURCE_STATE_COPY_DEST) {
                state->isValidToDecay = false;
                return;
            }
        }
    }

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = GetD3D12Resource();
    barrier.Transition.StateBefore = lastState;
    barrier.Transition.StateAfter = newState;

    // Currently Stencil8 is implemented with DXGI_FORMAT_D24_UNORM_S8_UINT, while we can only
    // choose the stencil aspect for Stencil8 textures, so actually we cannot select the full
    // subresource range on the Stencil8 textures.
    bool isFullRange =
        range.baseArrayLayer == 0 && range.baseMipLevel == 0 &&
        range.layerCount == GetArrayLayers() && range.levelCount == GetNumMipLevels() &&
        range.aspects == GetFormat().aspects && GetFormat().format != wgpu::TextureFormat::Stencil8;

    // Use a single transition for all subresources if possible.
    if (isFullRange) {
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers->push_back(barrier);
    } else {
        for (Aspect aspect : IterateEnumMask(range.aspects)) {
            for (uint32_t arrayLayer = 0; arrayLayer < range.layerCount; ++arrayLayer) {
                for (uint32_t mipLevel = 0; mipLevel < range.levelCount; ++mipLevel) {
                    barrier.Transition.Subresource = GetSubresourceIndex(
                        range.baseMipLevel + mipLevel, range.baseArrayLayer + arrayLayer, aspect);
                    barriers->push_back(barrier);
                }
            }
        }
    }

    state->isValidToDecay = false;
}

void Texture::HandleTransitionSpecialCases(CommandRecordingContext* commandContext) {
    // Externally allocated textures can be written from other graphics queues. Hence, they must be
    // acquired before command list submission to ensure work from the other queues has finished.
    // See CommandRecordingContext::ExecuteCommandList.
    if (mResourceAllocation.GetInfo().mMethod == AllocationMethod::kExternal) {
        commandContext->AddToSharedTextureList(this);
    }
}

void Texture::TransitionUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                                   std::vector<D3D12_RESOURCE_BARRIER>* barrier,
                                                   wgpu::TextureUsage usage,
                                                   const SubresourceRange& range) {
    TransitionUsageAndGetResourceBarrier(commandContext, barrier,
                                         D3D12TextureUsage(usage, GetFormat()), range);
}

void Texture::TransitionUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                                   std::vector<D3D12_RESOURCE_BARRIER>* barriers,
                                                   D3D12_RESOURCE_STATES newState,
                                                   const SubresourceRange& range) {
    HandleTransitionSpecialCases(commandContext);

    const ExecutionSerial pendingCommandSerial = ToBackend(GetDevice())->GetPendingCommandSerial();

    mSubresourceStateAndDecay.Update(range, [&](const SubresourceRange& updateRange,
                                                StateAndDecay* state) {
        TransitionSubresourceRange(barriers, updateRange, state, newState, pendingCommandSerial);
    });
}

void Texture::TrackUsageAndGetResourceBarrierForPass(CommandRecordingContext* commandContext,
                                                     std::vector<D3D12_RESOURCE_BARRIER>* barriers,
                                                     const TextureSubresourceUsage& textureUsages) {
    if (mResourceAllocation.GetInfo().mMethod != AllocationMethod::kExternal) {
        // Track the underlying heap to ensure residency.
        Heap* heap = ToBackend(mResourceAllocation.GetResourceHeap());
        commandContext->TrackHeapUsage(heap, GetDevice()->GetPendingCommandSerial());
    }

    HandleTransitionSpecialCases(commandContext);

    const ExecutionSerial pendingCommandSerial = ToBackend(GetDevice())->GetPendingCommandSerial();

    mSubresourceStateAndDecay.Merge(
        textureUsages,
        [&](const SubresourceRange& mergeRange, StateAndDecay* state, wgpu::TextureUsage usage) {
            // Skip if this subresource is not used during the current pass
            if (usage == wgpu::TextureUsage::None) {
                return;
            }

            D3D12_RESOURCE_STATES newState = D3D12TextureUsage(usage, GetFormat());
            TransitionSubresourceRange(barriers, mergeRange, state, newState, pendingCommandSerial);
        });
}

SubresourceStorage<Texture::StateAndDecay> Texture::InitialSubresourceStateAndDecay() const {
    return {GetFormat().aspects,
            GetArrayLayers(),
            GetNumMipLevels(),
            {D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, kMaxExecutionSerial, false}};
}

void Texture::ResetSubresourceStateAndDecayToCommon() {
    mSubresourceStateAndDecay = InitialSubresourceStateAndDecay();
}

D3D12_RENDER_TARGET_VIEW_DESC Texture::GetRTVDescriptor(const Format& format,
                                                        uint32_t mipLevel,
                                                        uint32_t baseSlice,
                                                        uint32_t sliceCount) const {
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = d3d::DXGITextureFormat(format.format);
    if (IsMultisampledTexture()) {
        ASSERT(GetDimension() == wgpu::TextureDimension::e2D);
        ASSERT(GetNumMipLevels() == 1);
        ASSERT(sliceCount == 1);
        ASSERT(baseSlice == 0);
        ASSERT(mipLevel == 0);
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        return rtvDesc;
    }
    switch (GetDimension()) {
        case wgpu::TextureDimension::e2D:
            // Currently we always use D3D12_TEX2D_ARRAY_RTV because we cannot specify base
            // array layer and layer count in D3D12_TEX2D_RTV. For 2D texture views, we treat
            // them as 1-layer 2D array textures. (Just like how we treat SRVs)
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_rtv
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_array
            // _rtv
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.FirstArraySlice = baseSlice;
            rtvDesc.Texture2DArray.ArraySize = sliceCount;
            rtvDesc.Texture2DArray.MipSlice = mipLevel;
            rtvDesc.Texture2DArray.PlaneSlice = 0;
            break;
        case wgpu::TextureDimension::e3D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = mipLevel;
            rtvDesc.Texture3D.FirstWSlice = baseSlice;
            rtvDesc.Texture3D.WSize = sliceCount;
            break;
        case wgpu::TextureDimension::e1D:
            UNREACHABLE();
            break;
    }
    return rtvDesc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC Texture::GetDSVDescriptor(uint32_t mipLevel,
                                                        uint32_t baseArrayLayer,
                                                        uint32_t layerCount,
                                                        Aspect aspects,
                                                        bool depthReadOnly,
                                                        bool stencilReadOnly) const {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = GetD3D12Format();
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    if (depthReadOnly && aspects & Aspect::Depth) {
        dsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    }
    if (stencilReadOnly && aspects & Aspect::Stencil) {
        dsvDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    }

    if (IsMultisampledTexture()) {
        ASSERT(GetNumMipLevels() == 1);
        ASSERT(layerCount == 1);
        ASSERT(baseArrayLayer == 0);
        ASSERT(mipLevel == 0);
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    } else {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.FirstArraySlice = baseArrayLayer;
        dsvDesc.Texture2DArray.ArraySize = layerCount;
        dsvDesc.Texture2DArray.MipSlice = mipLevel;
    }

    return dsvDesc;
}

MaybeError Texture::ClearTexture(CommandRecordingContext* commandContext,
                                 const SubresourceRange& range,
                                 TextureBase::ClearValue clearValue) {
    ID3D12GraphicsCommandList* commandList = commandContext->GetCommandList();

    Device* device = ToBackend(GetDevice());

    uint8_t clearColor = (clearValue == TextureBase::ClearValue::Zero) ? 0 : 1;
    float fClearColor = (clearValue == TextureBase::ClearValue::Zero) ? 0.f : 1.f;

    if ((mD3D12ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0) {
        TrackUsageAndTransitionNow(commandContext, D3D12_RESOURCE_STATE_DEPTH_WRITE, range);

        for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
             ++level) {
            for (uint32_t layer = range.baseArrayLayer;
                 layer < range.baseArrayLayer + range.layerCount; ++layer) {
                // Iterate the aspects individually to determine which clear flags to use.
                D3D12_CLEAR_FLAGS clearFlags = {};
                for (Aspect aspect : IterateEnumMask(range.aspects)) {
                    if (clearValue == TextureBase::ClearValue::Zero &&
                        IsSubresourceContentInitialized(
                            SubresourceRange::SingleMipAndLayer(level, layer, aspect))) {
                        // Skip lazy clears if already initialized.
                        continue;
                    }

                    switch (aspect) {
                        case Aspect::Depth:
                            clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
                            break;
                        case Aspect::Stencil:
                            clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
                            break;
                        default:
                            UNREACHABLE();
                    }
                }

                if (clearFlags == 0) {
                    continue;
                }

                CPUDescriptorHeapAllocation dsvHandle;
                DAWN_TRY_ASSIGN(
                    dsvHandle,
                    device->GetDepthStencilViewAllocator()->AllocateTransientCPUDescriptors());
                const D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor = dsvHandle.GetBaseDescriptor();
                D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc =
                    GetDSVDescriptor(level, layer, 1, range.aspects, false, false);
                device->GetD3D12Device()->CreateDepthStencilView(GetD3D12Resource(), &dsvDesc,
                                                                 baseDescriptor);

                commandList->ClearDepthStencilView(baseDescriptor, clearFlags, fClearColor,
                                                   clearColor, 0, nullptr);
            }
        }
    } else if ((mD3D12ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0) {
        TrackUsageAndTransitionNow(commandContext, D3D12_RESOURCE_STATE_RENDER_TARGET, range);

        const float clearColorRGBA[4] = {fClearColor, fClearColor, fClearColor, fClearColor};

        ASSERT(range.aspects == Aspect::Color);
        for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
             ++level) {
            for (uint32_t layer = range.baseArrayLayer;
                 layer < range.baseArrayLayer + range.layerCount; ++layer) {
                if (clearValue == TextureBase::ClearValue::Zero &&
                    IsSubresourceContentInitialized(
                        SubresourceRange::SingleMipAndLayer(level, layer, Aspect::Color))) {
                    // Skip lazy clears if already initialized.
                    continue;
                }

                CPUDescriptorHeapAllocation rtvHeap;
                DAWN_TRY_ASSIGN(
                    rtvHeap,
                    device->GetRenderTargetViewAllocator()->AllocateTransientCPUDescriptors());
                const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap.GetBaseDescriptor();

                uint32_t baseSlice = layer;
                uint32_t sliceCount = 1;
                if (GetDimension() == wgpu::TextureDimension::e3D) {
                    baseSlice = 0;
                    sliceCount = std::max(GetDepth() >> level, 1u);
                }
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
                    GetRTVDescriptor(GetFormat(), level, baseSlice, sliceCount);
                device->GetD3D12Device()->CreateRenderTargetView(GetD3D12Resource(), &rtvDesc,
                                                                 rtvHandle);
                commandList->ClearRenderTargetView(rtvHandle, clearColorRGBA, 0, nullptr);
            }
        }
    } else {
        ASSERT(!IsMultisampledTexture());

        // create temp buffer with clear color to copy to the texture image
        TrackUsageAndTransitionNow(commandContext, D3D12_RESOURCE_STATE_COPY_DEST, range);

        for (Aspect aspect : IterateEnumMask(range.aspects)) {
            const TexelBlockInfo& blockInfo = GetFormat().GetAspectInfo(aspect).block;

            Extent3D largestMipSize = GetMipLevelSingleSubresourcePhysicalSize(range.baseMipLevel);

            uint32_t bytesPerRow =
                Align((largestMipSize.width / blockInfo.width) * blockInfo.byteSize,
                      kTextureBytesPerRowAlignment);
            uint64_t bufferSize = bytesPerRow * (largestMipSize.height / blockInfo.height) *
                                  largestMipSize.depthOrArrayLayers;
            DynamicUploader* uploader = device->GetDynamicUploader();
            UploadHandle uploadHandle;
            DAWN_TRY_ASSIGN(uploadHandle,
                            uploader->Allocate(bufferSize, device->GetPendingCommandSerial(),
                                               blockInfo.byteSize));
            memset(uploadHandle.mappedBuffer, clearColor, bufferSize);

            for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
                 ++level) {
                // compute d3d12 texture copy locations for texture and buffer
                Extent3D copySize = GetMipLevelSingleSubresourcePhysicalSize(level);

                for (uint32_t layer = range.baseArrayLayer;
                     layer < range.baseArrayLayer + range.layerCount; ++layer) {
                    if (clearValue == TextureBase::ClearValue::Zero &&
                        IsSubresourceContentInitialized(
                            SubresourceRange::SingleMipAndLayer(level, layer, aspect))) {
                        // Skip lazy clears if already initialized.
                        continue;
                    }

                    TextureCopy textureCopy;
                    textureCopy.texture = this;
                    textureCopy.origin = {0, 0, layer};
                    textureCopy.mipLevel = level;
                    textureCopy.aspect = aspect;
                    RecordBufferTextureCopyWithBufferHandle(
                        BufferTextureCopyDirection::B2T, commandList,
                        ToBackend(uploadHandle.stagingBuffer)->GetD3D12Resource(),
                        uploadHandle.startOffset, bytesPerRow, largestMipSize.height, textureCopy,
                        copySize);
                }
            }
        }
    }
    if (clearValue == TextureBase::ClearValue::Zero) {
        SetIsSubresourceContentInitialized(true, range);
        GetDevice()->IncrementLazyClearCountForTesting();
    }
    return {};
}

void Texture::SetLabelHelper(const char* prefix) {
    SetDebugName(ToBackend(GetDevice()), mResourceAllocation.GetD3D12Resource(), prefix,
                 GetLabel());
}

void Texture::SetLabelImpl() {
    SetLabelHelper("Dawn_InternalTexture");
}

MaybeError Texture::EnsureSubresourceContentInitialized(CommandRecordingContext* commandContext,
                                                        const SubresourceRange& range) {
    if (!ToBackend(GetDevice())->IsToggleEnabled(Toggle::LazyClearResourceOnFirstUse)) {
        return {};
    }
    if (!IsSubresourceContentInitialized(range)) {
        // If subresource has not been initialized, clear it to black as it could contain
        // dirty bits from recycled memory
        DAWN_TRY(ClearTexture(commandContext, range, TextureBase::ClearValue::Zero));
    }
    return {};
}

bool Texture::StateAndDecay::operator==(const Texture::StateAndDecay& other) const {
    return lastState == other.lastState && lastDecaySerial == other.lastDecaySerial &&
           isValidToDecay == other.isValidToDecay;
}

// static
Ref<TextureView> TextureView::Create(TextureBase* texture,
                                     const TextureViewDescriptor* descriptor) {
    return AcquireRef(new TextureView(texture, descriptor));
}

TextureView::TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor)
    : TextureViewBase(texture, descriptor) {
    mSrvDesc.Format = d3d::DXGITextureFormat(descriptor->format);
    mSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    UINT planeSlice = 0;
    const Format& textureFormat = texture->GetFormat();
    if (textureFormat.HasDepthOrStencil()) {
        // Configure the SRV descriptor to reinterpret the texture allocated as
        // TYPELESS as a single-plane shader-accessible view.
        switch (textureFormat.format) {
            case wgpu::TextureFormat::Depth32Float:
            case wgpu::TextureFormat::Depth24Plus:
                mSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case wgpu::TextureFormat::Depth16Unorm:
                mSrvDesc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            case wgpu::TextureFormat::Stencil8: {
                Aspect aspects = SelectFormatAspects(textureFormat, descriptor->aspect);
                ASSERT(aspects != Aspect::None);
                if (!HasZeroOrOneBits(aspects)) {
                    // A single aspect is not selected. The texture view must not be
                    // sampled.
                    mSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
                    break;
                }
                switch (aspects) {
                    case Aspect::Depth:
                        planeSlice = 0;
                        mSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                        break;
                    case Aspect::Stencil:
                        planeSlice = 1;
                        mSrvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
                        // Stencil is accessed using the .g component in the shader.
                        // Map it to the zeroth component to match other APIs.
                        mSrvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
                            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
                            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
                            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
                        break;
                    default:
                        UNREACHABLE();
                        break;
                }
                break;
            }
            case wgpu::TextureFormat::Depth24PlusStencil8:
            case wgpu::TextureFormat::Depth32FloatStencil8: {
                Aspect aspects = SelectFormatAspects(textureFormat, descriptor->aspect);
                ASSERT(aspects != Aspect::None);
                if (!HasZeroOrOneBits(aspects)) {
                    // A single aspect is not selected. The texture view must not be
                    // sampled.
                    mSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
                    break;
                }
                switch (aspects) {
                    case Aspect::Depth:
                        planeSlice = 0;
                        mSrvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                        break;
                    case Aspect::Stencil:
                        planeSlice = 1;
                        mSrvDesc.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
                        // Stencil is accessed using the .g component in the shader.
                        // Map it to the zeroth component to match other APIs.
                        mSrvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                            D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
                            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
                            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
                            D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
                        break;
                    default:
                        UNREACHABLE();
                        break;
                }
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    }

    // Per plane view formats must have the plane slice number be the index of the plane in the
    // array of textures.
    if (texture->GetFormat().IsMultiPlanar()) {
        const Aspect planeAspect = ConvertViewAspect(GetFormat(), descriptor->aspect);
        planeSlice = GetAspectIndex(planeAspect);
        mSrvDesc.Format =
            d3d::DXGITextureFormat(texture->GetFormat().GetAspectInfo(planeAspect).format);
    }

    // Currently we always use D3D12_TEX2D_ARRAY_SRV because we cannot specify base array layer
    // and layer count in D3D12_TEX2D_SRV. For 2D texture views, we treat them as 1-layer 2D
    // array textures.
    // Multisampled textures may only be one array layer, so we use
    // D3D12_SRV_DIMENSION_TEXTURE2DMS.
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_srv
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_array_srv
    if (GetTexture()->IsMultisampledTexture()) {
        switch (descriptor->dimension) {
            case wgpu::TextureViewDimension::e2DArray:
                ASSERT(texture->GetArrayLayers() == 1);
                [[fallthrough]];
            case wgpu::TextureViewDimension::e2D:
                ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                break;

            default:
                UNREACHABLE();
        }
    } else {
        switch (descriptor->dimension) {
            case wgpu::TextureViewDimension::e1D:
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                mSrvDesc.Texture1D.MipLevels = descriptor->mipLevelCount;
                mSrvDesc.Texture1D.MostDetailedMip = descriptor->baseMipLevel;
                mSrvDesc.Texture1D.ResourceMinLODClamp = 0;
                break;

            case wgpu::TextureViewDimension::e2D:
            case wgpu::TextureViewDimension::e2DArray:
                ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                mSrvDesc.Texture2DArray.ArraySize = descriptor->arrayLayerCount;
                mSrvDesc.Texture2DArray.FirstArraySlice = descriptor->baseArrayLayer;
                mSrvDesc.Texture2DArray.MipLevels = descriptor->mipLevelCount;
                mSrvDesc.Texture2DArray.MostDetailedMip = descriptor->baseMipLevel;
                mSrvDesc.Texture2DArray.PlaneSlice = planeSlice;
                mSrvDesc.Texture2DArray.ResourceMinLODClamp = 0;
                break;
            case wgpu::TextureViewDimension::Cube:
            case wgpu::TextureViewDimension::CubeArray:
                ASSERT(texture->GetDimension() == wgpu::TextureDimension::e2D);
                ASSERT(descriptor->arrayLayerCount % 6 == 0);
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                mSrvDesc.TextureCubeArray.First2DArrayFace = descriptor->baseArrayLayer;
                mSrvDesc.TextureCubeArray.NumCubes = descriptor->arrayLayerCount / 6;
                mSrvDesc.TextureCubeArray.MostDetailedMip = descriptor->baseMipLevel;
                mSrvDesc.TextureCubeArray.MipLevels = descriptor->mipLevelCount;
                mSrvDesc.TextureCubeArray.ResourceMinLODClamp = 0;
                break;
            case wgpu::TextureViewDimension::e3D:
                ASSERT(texture->GetDimension() == wgpu::TextureDimension::e3D);
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                mSrvDesc.Texture3D.MostDetailedMip = descriptor->baseMipLevel;
                mSrvDesc.Texture3D.MipLevels = descriptor->mipLevelCount;
                mSrvDesc.Texture3D.ResourceMinLODClamp = 0;
                break;

            case wgpu::TextureViewDimension::Undefined:
                UNREACHABLE();
        }
    }
}

DXGI_FORMAT TextureView::GetD3D12Format() const {
    return d3d::DXGITextureFormat(GetFormat().format);
}

const D3D12_SHADER_RESOURCE_VIEW_DESC& TextureView::GetSRVDescriptor() const {
    ASSERT(mSrvDesc.Format != DXGI_FORMAT_UNKNOWN);
    return mSrvDesc;
}

D3D12_RENDER_TARGET_VIEW_DESC TextureView::GetRTVDescriptor() const {
    return ToBackend(GetTexture())
        ->GetRTVDescriptor(GetFormat(), GetBaseMipLevel(), GetBaseArrayLayer(), GetLayerCount());
}

D3D12_DEPTH_STENCIL_VIEW_DESC TextureView::GetDSVDescriptor(bool depthReadOnly,
                                                            bool stencilReadOnly) const {
    ASSERT(GetLevelCount() == 1);
    return ToBackend(GetTexture())
        ->GetDSVDescriptor(GetBaseMipLevel(), GetBaseArrayLayer(), GetLayerCount(), GetAspects(),
                           depthReadOnly, stencilReadOnly);
}

D3D12_UNORDERED_ACCESS_VIEW_DESC TextureView::GetUAVDescriptor() const {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = GetD3D12Format();

    ASSERT(!GetTexture()->IsMultisampledTexture());
    switch (GetDimension()) {
        case wgpu::TextureViewDimension::e1D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = GetBaseMipLevel();
            break;
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e2DArray:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
            uavDesc.Texture2DArray.ArraySize = GetLayerCount();
            uavDesc.Texture2DArray.MipSlice = GetBaseMipLevel();
            uavDesc.Texture2DArray.PlaneSlice = 0;
            break;
        case wgpu::TextureViewDimension::e3D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.FirstWSlice = 0;
            uavDesc.Texture3D.WSize = GetTexture()->GetDepth() >> GetBaseMipLevel();
            uavDesc.Texture3D.MipSlice = GetBaseMipLevel();
            break;
        // Cube and Cubemap can't be used as storage texture. So there is no need to create UAV
        // descriptor for them.
        case wgpu::TextureViewDimension::Cube:
        case wgpu::TextureViewDimension::CubeArray:
        case wgpu::TextureViewDimension::Undefined:
            UNREACHABLE();
    }
    return uavDesc;
}

}  // namespace dawn::native::d3d12
