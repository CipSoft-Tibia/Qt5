// Copyright 2023 The Dawn Authors
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

#include "dawn/native/d3d11/TextureD3D11.h"

#include <algorithm>
#include <string>
#include <utility>

#include "dawn/common/Constants.h"
#include "dawn/common/Math.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/CommandValidation.h"
#include "dawn/native/DynamicUploader.h"
#include "dawn/native/EnumMaskIterator.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/ToBackend.h"
#include "dawn/native/d3d/D3DError.h"
#include "dawn/native/d3d/UtilsD3D.h"
#include "dawn/native/d3d11/DeviceD3D11.h"
#include "dawn/native/d3d11/FenceD3D11.h"
#include "dawn/native/d3d11/Forward.h"
#include "dawn/native/d3d11/SharedTextureMemoryD3D11.h"
#include "dawn/native/d3d11/UtilsD3D11.h"

namespace dawn::native::d3d11 {
namespace {

UINT D3D11TextureBindFlags(wgpu::TextureUsage usage, const Format& format) {
    bool isDepthOrStencilFormat = format.HasDepthOrStencil();
    UINT bindFlags = 0;
    if (usage & wgpu::TextureUsage::TextureBinding) {
        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (usage & wgpu::TextureUsage::StorageBinding) {
        bindFlags |= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    }
    if (usage & wgpu::TextureUsage::RenderAttachment) {
        bindFlags |= isDepthOrStencilFormat ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;
    }
    return bindFlags;
}

Aspect D3D11Aspect(Aspect aspect) {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources
    // Planar formats existed in Direct3D 11, but individual planes could not be addressed
    // individually.
    if (IsSubset(aspect, Aspect::Depth | Aspect::Stencil)) {
        return Aspect::CombinedDepthStencil;
    }

    ASSERT(HasOneBit(aspect));
    return aspect;
}

// Gets the uncompressed texture format for reinterpretation conversion.
wgpu::TextureFormat UncompressedTextureFormat(wgpu::TextureFormat compressedFormat) {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression
    switch (compressedFormat) {
        case wgpu::TextureFormat::BC1RGBAUnorm:
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
        case wgpu::TextureFormat::BC4RSnorm:
        case wgpu::TextureFormat::BC4RUnorm:
            return wgpu::TextureFormat::RGBA16Uint;

        case wgpu::TextureFormat::BC2RGBAUnorm:
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
        case wgpu::TextureFormat::BC3RGBAUnorm:
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
        case wgpu::TextureFormat::BC5RGSnorm:
        case wgpu::TextureFormat::BC5RGUnorm:
        case wgpu::TextureFormat::BC6HRGBFloat:
        case wgpu::TextureFormat::BC6HRGBUfloat:
        case wgpu::TextureFormat::BC7RGBAUnorm:
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
            return wgpu::TextureFormat::RGBA32Uint;
        default:
            UNREACHABLE();
    }
}

// The memory layout of depth or stencil component inside a texel of depth-stencil format.
struct DepthStencilAspectLayout {
    // Texel size of a depth/stencil DXGI format in bytes.
    uint32_t texelSize = 0u;
    // Depth/Stencil component offset inside the texel in bytes.
    uint32_t componentOffset = 0u;
    // Depth/Stencil component size in bytes.
    uint32_t componentSize = 0u;
};

DepthStencilAspectLayout DepthStencilAspectLayout(DXGI_FORMAT format, Aspect aspect) {
    ASSERT(aspect == Aspect::Depth || aspect == Aspect::Stencil);
    uint32_t texelSize = 0u;
    uint32_t componentOffset = 0u;
    uint32_t componentSize = 0u;

    switch (format) {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            componentOffset = aspect == Aspect::Stencil ? 3u : 0u;
            componentSize = aspect == Aspect::Stencil ? 1u : 3u;
            texelSize = 4u;
            break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            componentOffset = aspect == Aspect::Stencil ? 4u : 0u;
            componentSize = aspect == Aspect::Stencil ? 1u : 4u;
            texelSize = 8u;
            break;
        default:
            UNREACHABLE();
    }
    return {texelSize, componentOffset, componentSize};
}

}  // namespace

MaybeError ValidateTextureCanBeWrapped(ID3D11Resource* d3d11Resource,
                                       const TextureDescriptor* dawnDescriptor) {
    ComPtr<ID3D11Texture2D> d3d11Texture;
    DAWN_TRY(
        CheckHRESULT(d3d11Resource->QueryInterface(IID_PPV_ARGS(&d3d11Texture)), "QueryInterface"));

    D3D11_TEXTURE2D_DESC d3dDescriptor;
    d3d11Texture->GetDesc(&d3dDescriptor);

    DAWN_INVALID_IF(
        (dawnDescriptor->size.width != d3dDescriptor.Width) ||
            (dawnDescriptor->size.height != d3dDescriptor.Height) ||
            (dawnDescriptor->size.depthOrArrayLayers != 1),
        "D3D11 texture size (Width: %u, Height: %u, DepthOrArraySize: 1) doesn't match Dawn "
        "descriptor size (width: %u, height: %u, depthOrArrayLayers: %u).",
        d3dDescriptor.Width, d3dDescriptor.Height, dawnDescriptor->size.width,
        dawnDescriptor->size.height, dawnDescriptor->size.depthOrArrayLayers);

    const DXGI_FORMAT dxgiFormatFromDescriptor = d3d::DXGITextureFormat(dawnDescriptor->format);
    DAWN_INVALID_IF(dxgiFormatFromDescriptor != d3dDescriptor.Format,
                    "D3D11 texture format (%x) is not compatible with Dawn descriptor format (%s).",
                    d3dDescriptor.Format, dawnDescriptor->format);

    DAWN_INVALID_IF(d3dDescriptor.ArraySize != 1, "D3D12 texture array size (%u) is not 1.",
                    d3dDescriptor.ArraySize);

    DAWN_INVALID_IF(d3dDescriptor.MipLevels != 1,
                    "D3D11 texture number of miplevels (%u) is not 1.", d3dDescriptor.MipLevels);

    // Shared textures cannot be multi-sample so no need to check those.
    ASSERT(d3dDescriptor.SampleDesc.Count == 1);
    ASSERT(d3dDescriptor.SampleDesc.Quality == 0);

    return {};
}

MaybeError ValidateVideoTextureCanBeShared(Device* device, DXGI_FORMAT textureFormat) {
    const bool supportsSharedResourceCapabilityTier2 =
        device->GetDeviceInfo().supportsSharedResourceCapabilityTier2;
    switch (textureFormat) {
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
            if (supportsSharedResourceCapabilityTier2) {
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
    Ref<Texture> texture = AcquireRef(new Texture(device, descriptor, Kind::Normal));
    DAWN_TRY(texture->InitializeAsInternalTexture());
    return std::move(texture);
}

// static
ResultOrError<Ref<Texture>> Texture::Create(Device* device,
                                            const TextureDescriptor* descriptor,
                                            ComPtr<ID3D11Resource> d3d11Texture) {
    Ref<Texture> dawnTexture = AcquireRef(new Texture(device, descriptor, Kind::Normal));
    DAWN_TRY(dawnTexture->InitializeAsSwapChainTexture(std::move(d3d11Texture)));
    return std::move(dawnTexture);
}

ResultOrError<Ref<Texture>> Texture::CreateInternal(Device* device,
                                                    const TextureDescriptor* descriptor,
                                                    Kind kind) {
    Ref<Texture> texture = AcquireRef(new Texture(device, descriptor, kind));
    DAWN_TRY(texture->InitializeAsInternalTexture());
    return std::move(texture);
}

// static
ResultOrError<Ref<Texture>> Texture::CreateExternalImage(Device* device,
                                                         const TextureDescriptor* descriptor,
                                                         ComPtr<IUnknown> d3dTexture,
                                                         std::vector<Ref<d3d::Fence>> waitFences,
                                                         bool isSwapChainTexture,
                                                         bool isInitialized) {
    Ref<Texture> dawnTexture = AcquireRef(new Texture(device, descriptor, Kind::Normal));
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
ResultOrError<Ref<Texture>> Texture::CreateFromSharedTextureMemory(
    SharedTextureMemory* memory,
    const TextureDescriptor* descriptor) {
    Device* device = ToBackend(memory->GetDevice());
    Ref<Texture> texture = AcquireRef(new Texture(device, descriptor, Kind::Normal));
    DAWN_TRY(texture->InitializeAsExternalTexture(memory->GetD3DResource(), {}, false));
    texture->mSharedTextureMemoryState = memory->GetState();
    return texture;
}

template <typename T>
T Texture::GetD3D11TextureDesc() const {
    T desc;

    if constexpr (std::is_same<T, D3D11_TEXTURE1D_DESC>::value) {
        desc.Width = GetSize().width;
        desc.ArraySize = GetArrayLayers();
        desc.MiscFlags = 0;
    } else if constexpr (std::is_same<T, D3D11_TEXTURE2D_DESC>::value) {
        desc.Width = GetSize().width;
        desc.Height = GetSize().height;
        desc.ArraySize = GetArrayLayers();
        desc.SampleDesc.Count = GetSampleCount();
        desc.SampleDesc.Quality = 0;
        desc.MiscFlags = 0;
        if (GetArrayLayers() >= 6 && desc.Width == desc.Height) {
            // Texture layers are more than 6. It can be used as a cube map.
            desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }
    } else if constexpr (std::is_same<T, D3D11_TEXTURE3D_DESC>::value) {
        desc.Width = GetSize().width;
        desc.Height = GetSize().height;
        desc.Depth = GetSize().depthOrArrayLayers;
        desc.MiscFlags = 0;
    }

    desc.MipLevels = static_cast<UINT16>(GetNumMipLevels());
    // To sample from a depth or stencil texture, we need to create a typeless texture.
    bool needsTypelessFormat =
        GetFormat().HasDepthOrStencil() && (GetUsage() & wgpu::TextureUsage::TextureBinding);
    // We need to use the typeless format if view format reinterpretation is required.
    needsTypelessFormat |= GetViewFormats().any();
    // We need to use the typeless format if it's a staging texture for writting to depth-stencil
    // textures.
    needsTypelessFormat |=
        d3d::IsDepthStencil(d3d::DXGITextureFormat(GetFormat().format)) && mKind == Kind::Staging;
    desc.Format = needsTypelessFormat ? d3d::DXGITypelessTextureFormat(GetFormat().format)
                                      : d3d::DXGITextureFormat(GetFormat().format);
    desc.Usage = mKind == Kind::Staging ? D3D11_USAGE_STAGING : D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11TextureBindFlags(GetInternalUsage(), GetFormat());
    constexpr UINT kCPUReadWriteFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.CPUAccessFlags = mKind == Kind::Staging ? kCPUReadWriteFlags : 0;

    return desc;
}

MaybeError Texture::InitializeAsInternalTexture() {
    Device* device = ToBackend(GetDevice());

    if (GetFormat().isRenderable && mKind != Kind::Staging) {
        // If the texture format is renderable, we need to add the render attachment usage
        // internally, so the texture can be cleared with GPU.
        AddInternalUsage(wgpu::TextureUsage::RenderAttachment);
    }

    switch (GetDimension()) {
        case wgpu::TextureDimension::e1D: {
            D3D11_TEXTURE1D_DESC desc = GetD3D11TextureDesc<D3D11_TEXTURE1D_DESC>();
            ComPtr<ID3D11Texture1D> d3d11Texture1D;
            DAWN_TRY(CheckOutOfMemoryHRESULT(
                device->GetD3D11Device()->CreateTexture1D(&desc, nullptr, &d3d11Texture1D),
                "D3D11 create texture1d"));
            mD3d11Resource = std::move(d3d11Texture1D);
            break;
        }
        case wgpu::TextureDimension::e2D: {
            D3D11_TEXTURE2D_DESC desc = GetD3D11TextureDesc<D3D11_TEXTURE2D_DESC>();
            ComPtr<ID3D11Texture2D> d3d11Texture2D;
            DAWN_TRY(CheckOutOfMemoryHRESULT(
                device->GetD3D11Device()->CreateTexture2D(&desc, nullptr, &d3d11Texture2D),
                "D3D11 create texture2d"));
            mD3d11Resource = std::move(d3d11Texture2D);
            break;
        }
        case wgpu::TextureDimension::e3D: {
            D3D11_TEXTURE3D_DESC desc = GetD3D11TextureDesc<D3D11_TEXTURE3D_DESC>();
            ComPtr<ID3D11Texture3D> d3d11Texture3D;
            DAWN_TRY(CheckOutOfMemoryHRESULT(
                device->GetD3D11Device()->CreateTexture3D(&desc, nullptr, &d3d11Texture3D),
                "D3D11 create texture3d"));
            mD3d11Resource = std::move(d3d11Texture3D);
            break;
        }
    }

    // Staging texture is used internally, so we don't need to clear it.
    if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
        mKind == Kind::Normal) {
        CommandRecordingContext* commandContext = device->GetPendingCommandContext();
        DAWN_TRY(Clear(commandContext, GetAllSubresources(), TextureBase::ClearValue::NonZero));
    }

    SetLabelImpl();

    return {};
}

MaybeError Texture::InitializeAsSwapChainTexture(ComPtr<ID3D11Resource> d3d11Texture) {
    mD3d11Resource = std::move(d3d11Texture);
    SetLabelHelper("Dawn_SwapChainTexture");

    return {};
}

MaybeError Texture::InitializeAsExternalTexture(ComPtr<IUnknown> d3dTexture,
                                                std::vector<Ref<d3d::Fence>> waitFences,
                                                bool isSwapChainTexture) {
    ComPtr<ID3D11Resource> d3d11Texture;
    DAWN_TRY(CheckHRESULT(d3dTexture.As(&d3d11Texture), "Query ID3D11Resource from IUnknown"));

    CommandRecordingContext* commandContext = ToBackend(GetDevice())->GetPendingCommandContext();
    ID3D11DeviceContext4* d3d11DeviceContext4 = commandContext->GetD3D11DeviceContext4();
    for (Ref<d3d::Fence>& fence : waitFences) {
        DAWN_TRY(CheckHRESULT(
            d3d11DeviceContext4->Wait(static_cast<Fence*>(fence.Get())->GetD3D11Fence(),
                                      fence->GetFenceValue()),
            "ID3D11DeviceContext4::Wait"));
    }
    mD3d11Resource = std::move(d3d11Texture);
    SetLabelHelper("Dawn_ExternalTexture");
    return {};
}

Texture::Texture(Device* device, const TextureDescriptor* descriptor, Kind kind)
    : Base(device, descriptor), mKind(kind) {}

Texture::~Texture() = default;

void Texture::DestroyImpl() {
    TextureBase::DestroyImpl();
    mD3d11Resource = nullptr;
}

ID3D11Resource* Texture::GetD3D11Resource() const {
    return mD3d11Resource.Get();
}

D3D11_RENDER_TARGET_VIEW_DESC Texture::GetRTVDescriptor(
    const Format& format,
    const SubresourceRange& singleLevelRange) const {
    ASSERT(singleLevelRange.levelCount == 1);
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = d3d::DXGITextureFormat(format.format);
    if (IsMultisampledTexture()) {
        ASSERT(GetDimension() == wgpu::TextureDimension::e2D);
        ASSERT(GetNumMipLevels() == 1);
        ASSERT(singleLevelRange.baseMipLevel == 0);
        ASSERT(singleLevelRange.baseArrayLayer == 0);
        ASSERT(singleLevelRange.layerCount == 1);
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        return rtvDesc;
    }
    switch (GetDimension()) {
        case wgpu::TextureDimension::e2D:
            // Currently we always use D3D11_TEX2D_ARRAY_RTV because we cannot specify base
            // array layer and layer count in D3D11_TEX2D_RTV. For 2D texture views, we treat
            // them as 1-layer 2D array textures. (Just like how we treat SRVs)
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ns-d3d11-d3d11_tex2d_rtv
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ns-d3d11-d3d11_tex2d_array
            // _rtv
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = singleLevelRange.baseMipLevel;
            rtvDesc.Texture2DArray.FirstArraySlice = singleLevelRange.baseArrayLayer;
            rtvDesc.Texture2DArray.ArraySize = singleLevelRange.layerCount;
            break;
        case wgpu::TextureDimension::e3D:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = singleLevelRange.baseMipLevel;
            rtvDesc.Texture3D.FirstWSlice = singleLevelRange.baseArrayLayer;
            rtvDesc.Texture3D.WSize = singleLevelRange.layerCount;
            break;
        case wgpu::TextureDimension::e1D:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
            rtvDesc.Texture1D.MipSlice = singleLevelRange.baseMipLevel;
            break;
    }
    return rtvDesc;
}

D3D11_DEPTH_STENCIL_VIEW_DESC Texture::GetDSVDescriptor(const SubresourceRange& singleLevelRange,
                                                        bool depthReadOnly,
                                                        bool stencilReadOnly) const {
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    ASSERT(singleLevelRange.levelCount == 1);
    dsvDesc.Format = d3d::DXGITextureFormat(GetFormat().format);
    dsvDesc.Flags = 0;
    if (depthReadOnly && singleLevelRange.aspects & Aspect::Depth) {
        dsvDesc.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
    }
    if (stencilReadOnly && singleLevelRange.aspects & Aspect::Stencil) {
        dsvDesc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
    }

    if (IsMultisampledTexture()) {
        ASSERT(GetNumMipLevels() == 1);
        ASSERT(singleLevelRange.baseMipLevel == 0);
        ASSERT(singleLevelRange.baseArrayLayer == 0);
        ASSERT(singleLevelRange.layerCount == 1);
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    } else {
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = singleLevelRange.baseMipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = singleLevelRange.baseArrayLayer;
        dsvDesc.Texture2DArray.ArraySize = singleLevelRange.layerCount;
    }

    return dsvDesc;
}

MaybeError Texture::Clear(CommandRecordingContext* commandContext,
                          const SubresourceRange& range,
                          TextureBase::ClearValue clearValue) {
    bool isRenderable = GetInternalUsage() & wgpu::TextureUsage::RenderAttachment;

    if (isRenderable) {
        float color = clearValue == ClearValue::Zero ? 0.0f : 1.0f;
        float depth = clearValue == ClearValue::Zero ? 0.0f : 1.0f;
        uint8_t stencil = clearValue == ClearValue::Zero ? 0u : 1u;
        D3D11ClearValue d3d11ClearValue = {{color, color, color, color}, depth, stencil};
        DAWN_TRY(ClearRenderable(commandContext, range, clearValue, d3d11ClearValue));
    } else if (GetFormat().isCompressed) {
        DAWN_TRY(ClearCompressed(commandContext, range, clearValue));
    } else {
        DAWN_TRY(ClearNonRenderable(commandContext, range, clearValue));
    }

    if (clearValue == TextureBase::ClearValue::Zero && mKind == Kind::Normal) {
        SetIsSubresourceContentInitialized(true, range);
        GetDevice()->IncrementLazyClearCountForTesting();
    }

    return {};
}

MaybeError Texture::ClearRenderable(CommandRecordingContext* commandContext,
                                    const SubresourceRange& range,
                                    TextureBase::ClearValue clearValue,
                                    const D3D11ClearValue& d3d11ClearValue) {
    ID3D11DeviceContext* d3d11DeviceContext = commandContext->GetD3D11DeviceContext();

    TextureViewDescriptor desc = {};
    desc.label = "ClearTextureView";
    desc.format = GetFormat().format;
    switch (GetDimension()) {
        case wgpu::TextureDimension::e1D:
            desc.dimension = wgpu::TextureViewDimension::e1D;
            break;
        case wgpu::TextureDimension::e2D:
            desc.dimension = wgpu::TextureViewDimension::e2D;
            break;
        case wgpu::TextureDimension::e3D:
            desc.dimension = wgpu::TextureViewDimension::e3D;
            break;
    }
    // Whether content is initialized is tracked by frontend in unit of a single layer and
    // level, so we need to check to clear layer by layer, and level by level to make sure that
    // lazy clears won't overwrite any initialized content.
    desc.baseMipLevel = range.baseMipLevel;
    desc.mipLevelCount = range.levelCount;
    desc.arrayLayerCount = 1u;
    desc.aspect = wgpu::TextureAspect::All;

    UINT clearFlags = 0;
    if (GetFormat().HasDepth() && range.aspects & Aspect::Depth) {
        clearFlags |= D3D11_CLEAR_DEPTH;
    }
    if (GetFormat().HasStencil() && range.aspects & Aspect::Stencil) {
        clearFlags |= D3D11_CLEAR_STENCIL;
    }

    for (uint32_t arrayLayer = range.baseArrayLayer;
         arrayLayer < range.baseArrayLayer + range.layerCount; ++arrayLayer) {
        desc.baseArrayLayer = arrayLayer;
        Ref<TextureView> view = TextureView::Create(this, &desc);
        for (uint32_t mipLevel = range.baseMipLevel;
             mipLevel < range.baseMipLevel + range.levelCount; ++mipLevel) {
            if (clearValue == TextureBase::ClearValue::Zero &&
                IsSubresourceContentInitialized(
                    SubresourceRange::SingleMipAndLayer(mipLevel, arrayLayer, range.aspects))) {
                // Skip lazy clears if already initialized.
                continue;
            }
            if (GetFormat().HasDepthOrStencil()) {
                ComPtr<ID3D11DepthStencilView> d3d11DSV;
                DAWN_TRY_ASSIGN(d3d11DSV, view->CreateD3D11DepthStencilView(
                                              /*depthReadOnly=*/false,
                                              /*stencilReadOnly=*/false, mipLevel));
                d3d11DeviceContext->ClearDepthStencilView(
                    d3d11DSV.Get(), clearFlags, d3d11ClearValue.depth, d3d11ClearValue.stencil);
            } else {
                ComPtr<ID3D11RenderTargetView> d3d11RTV;
                DAWN_TRY_ASSIGN(d3d11RTV, view->CreateD3D11RenderTargetView(mipLevel));
                d3d11DeviceContext->ClearRenderTargetView(d3d11RTV.Get(), d3d11ClearValue.color);
            }
        }
    }

    return {};
}

MaybeError Texture::ClearNonRenderable(CommandRecordingContext* commandContext,
                                       const SubresourceRange& range,
                                       TextureBase::ClearValue clearValue) {
    const TexelBlockInfo& blockInfo = GetFormat().GetAspectInfo(range.aspects).block;
    // TODO(dawn:1705): Use interim texture clear-and-copy as compressed textures do to
    // avoid CPU-to-GPU write.
    Extent3D writeSize = GetMipLevelSubresourceVirtualSize(range.baseMipLevel);
    uint32_t bytesPerRow = blockInfo.byteSize * writeSize.width;

    uint32_t rowsPerImage = writeSize.height;
    uint64_t byteLength;
    DAWN_TRY_ASSIGN(byteLength,
                    ComputeRequiredBytesInCopy(blockInfo, writeSize, bytesPerRow, rowsPerImage));

    std::vector<uint8_t> clearData(byteLength, clearValue == ClearValue::Zero ? 0 : 1);
    SubresourceRange writeRange = range;
    writeRange.layerCount = 1;
    writeRange.levelCount = 1;
    for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount;
         ++layer) {
        for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
             ++level) {
            if (clearValue == TextureBase::ClearValue::Zero &&
                IsSubresourceContentInitialized(
                    SubresourceRange::SingleMipAndLayer(level, layer, range.aspects))) {
                // Skip lazy clears if already initialized.
                continue;
            }
            writeRange.baseArrayLayer = layer;
            writeRange.baseMipLevel = level;
            writeSize = GetMipLevelSubresourceVirtualSize(level);
            bytesPerRow = blockInfo.byteSize * writeSize.width;
            rowsPerImage = writeSize.height;
            DAWN_TRY(WriteInternal(commandContext, writeRange, {0, 0, 0}, writeSize,
                                   clearData.data(), bytesPerRow, rowsPerImage));
        }
    }

    return {};
}

MaybeError Texture::ClearCompressed(CommandRecordingContext* commandContext,
                                    const SubresourceRange& range,
                                    TextureBase::ClearValue clearValue) {
    const TexelBlockInfo& blockInfo = GetFormat().GetAspectInfo(range.aspects).block;
    // Create an interim texture of renderable format for reinterpretation conversion.
    TextureDescriptor desc = {};
    desc.label = "CopyUncompressedTextureToCompressedTexureInterim";
    desc.dimension = GetDimension();
    ASSERT(desc.dimension == wgpu::TextureDimension::e2D);
    desc.size = {GetSize().width, GetSize().height, 1};
    desc.format = UncompressedTextureFormat(GetFormat().format);
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;

    Ref<Texture> interimTexture;
    DAWN_TRY_ASSIGN(interimTexture, CreateInternal(ToBackend(GetDevice()), &desc, Kind::Interim));

    float color = 0.0f;
    if (clearValue == ClearValue::NonZero) {
        // Ensure value 1 per byte rather than per component. This is required in this test case:
        // https://source.chromium.org/chromium/chromium/src/+/refs/heads/main:third_party/dawn/src/dawn/tests/end2end/NonzeroTextureCreationTests.cpp;drc=7a6604d0564b56cce77b72ae759b3773a756423c;l=244
        double valueOnePerByte;
        switch (desc.format) {
            case wgpu::TextureFormat::RGBA16Uint:
                valueOnePerByte = 0x0101;
                break;
            case wgpu::TextureFormat::RGBA32Uint:
                valueOnePerByte = 0x01010101;
                break;
            default:
                UNREACHABLE();
        }
        color = valueOnePerByte;
    }
    float depth = clearValue == ClearValue::Zero ? 0.0f : 1.0f;
    uint8_t stencil = clearValue == ClearValue::Zero ? 0u : 1u;
    D3D11ClearValue d3d11ClearValue = {{color, color, color, color}, depth, stencil};
    DAWN_TRY(interimTexture->ClearRenderable(commandContext,
                                             SubresourceRange::MakeFull(Aspect::Color, 1, 1),
                                             clearValue, d3d11ClearValue));

    D3D11_BOX srcBox;
    srcBox.left = 0;
    srcBox.top = 0;
    srcBox.front = 0;
    srcBox.back = 1;
    uint32_t srcSubresource = interimTexture->GetSubresourceIndex(0, 0, Aspect::Color);
    // Copy from the interim texture to the dest texture.
    for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount;
         ++layer) {
        for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
             ++level) {
            if (clearValue == TextureBase::ClearValue::Zero &&
                IsSubresourceContentInitialized(
                    SubresourceRange::SingleMipAndLayer(level, layer, range.aspects))) {
                // Skip lazy clears if already initialized.
                continue;
            }
            uint32_t dstSubresource = GetSubresourceIndex(level, layer, D3D11Aspect(range.aspects));
            auto physicalSize = GetMipLevelSingleSubresourcePhysicalSize(level);
            // The documentation says D3D11_BOX's coordinates should be in texels for
            // textures. However the validation layer seemingly assumes them to be in
            // blocks. Otherwise it would complain like this:
            //     ID3D11DeviceContext::CopySubresourceRegion: When offset by the
            //     destination coordinates and converted to block dimensions, pSrcBox does
            //     not fit on the destination subresource. OffsetSrcBox = { left:0, top:0,
            //     front:0, right:128, bottom:128, back:1 } (in blocks). DstSubresource = {
            //     left:0, top:0, front:0, right:32, bottom:32, back:1 } (in blocks). [
            //     RESOURCE_MANIPULATION ERROR #280:COPYSUBRESOURCEREGION_INVALIDSOURCEBOX]
            srcBox.right = physicalSize.width / blockInfo.width;
            srcBox.bottom = physicalSize.height / blockInfo.height;
            commandContext->GetD3D11DeviceContext1()->CopySubresourceRegion(
                GetD3D11Resource(), dstSubresource, 0, 0, 0, interimTexture->GetD3D11Resource(),
                srcSubresource, &srcBox);
        }
    }

    return {};
}

void Texture::SetLabelHelper(const char* prefix) {
    SetDebugName(ToBackend(GetDevice()), mD3d11Resource.Get(), prefix, GetLabel());
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
        DAWN_TRY(Clear(commandContext, range, TextureBase::ClearValue::Zero));
    }
    return {};
}

MaybeError Texture::Write(CommandRecordingContext* commandContext,
                          const SubresourceRange& subresources,
                          const Origin3D& origin,
                          const Extent3D& size,
                          const uint8_t* data,
                          uint32_t bytesPerRow,
                          uint32_t rowsPerImage) {
    if (IsCompleteSubresourceCopiedTo(this, size, subresources.baseMipLevel)) {
        SetIsSubresourceContentInitialized(true, subresources);
    } else {
        // Dawn validation should have ensured that full subresources write for depth/stencil
        // textures.
        ASSERT(!GetFormat().HasDepthOrStencil());
        DAWN_TRY(EnsureSubresourceContentInitialized(commandContext, subresources));
    }
    DAWN_TRY(
        WriteInternal(commandContext, subresources, origin, size, data, bytesPerRow, rowsPerImage));

    return {};
}

MaybeError Texture::WriteInternal(CommandRecordingContext* commandContext,
                                  const SubresourceRange& subresources,
                                  const Origin3D& origin,
                                  const Extent3D& size,
                                  const uint8_t* data,
                                  uint32_t bytesPerRow,
                                  uint32_t rowsPerImage) {
    ASSERT(size.width != 0 && size.height != 0 && size.depthOrArrayLayers != 0);
    ASSERT(subresources.levelCount == 1);

    if (d3d::IsDepthStencil(d3d::DXGITextureFormat(GetFormat().format))) {
        DAWN_TRY(WriteDepthStencilInternal(commandContext, subresources, origin, size, data,
                                           bytesPerRow, rowsPerImage));
        return {};
    }

    D3D11_BOX dstBox;
    dstBox.left = origin.x;
    dstBox.right = origin.x + size.width;
    dstBox.top = origin.y;
    dstBox.bottom = origin.y + size.height;

    if (GetDimension() == wgpu::TextureDimension::e3D) {
        dstBox.front = origin.z;
        dstBox.back = origin.z + size.depthOrArrayLayers;
        uint32_t subresource =
            GetSubresourceIndex(subresources.baseMipLevel, 0, D3D11Aspect(subresources.aspects));
        commandContext->GetD3D11DeviceContext1()->UpdateSubresource(GetD3D11Resource(), subresource,
                                                                    &dstBox, data, bytesPerRow,
                                                                    bytesPerRow * rowsPerImage);
    } else {
        dstBox.front = 0;
        dstBox.back = 1;
        for (uint32_t layer = 0; layer < subresources.layerCount; ++layer) {
            uint32_t subresource =
                GetSubresourceIndex(subresources.baseMipLevel, subresources.baseArrayLayer + layer,
                                    D3D11Aspect(subresources.aspects));
            D3D11_BOX* pDstBox = GetFormat().HasDepthOrStencil() ? nullptr : &dstBox;
            commandContext->GetD3D11DeviceContext1()->UpdateSubresource(
                GetD3D11Resource(), subresource, pDstBox, data, bytesPerRow, 0);
            data += rowsPerImage * bytesPerRow;
        }
    }

    return {};
}

MaybeError Texture::WriteDepthStencilInternal(CommandRecordingContext* commandContext,
                                              const SubresourceRange& subresources,
                                              const Origin3D& origin,
                                              const Extent3D& size,
                                              const uint8_t* data,
                                              uint32_t bytesPerRow,
                                              uint32_t rowsPerImage) {
    TextureDescriptor desc = {};
    desc.label = "WriteStencilTextureStaging";
    desc.dimension = GetDimension();
    desc.size = size;
    desc.format = GetFormat().format;
    desc.mipLevelCount = 1;
    desc.sampleCount = GetSampleCount();

    Ref<Texture> stagingTexture;
    DAWN_TRY_ASSIGN(stagingTexture, CreateInternal(ToBackend(GetDevice()), &desc, Kind::Staging));

    // Depth-stencil subresources can only be written to completely and not partially.
    ASSERT(IsCompleteSubresourceCopiedTo(this, size, subresources.baseMipLevel));

    SubresourceRange otherRange = subresources;
    Aspect otherAspects = GetFormat().aspects & ~subresources.aspects;
    ASSERT(HasZeroOrOneBits(otherAspects));
    otherRange.aspects = otherAspects;
    // We need to copy the texture over if the other aspect is present and initialized so that it is
    // preserved during the write.
    bool shouldCopyExistingDataFirst =
        HasOneBit(otherAspects) && IsSubresourceContentInitialized(otherRange);

    if (shouldCopyExistingDataFirst) {
        // Copy the dest texture to a staging texture.
        CopyTextureToTextureCmd copyCmd;
        copyCmd.source.texture = this;
        copyCmd.source.origin = origin;
        copyCmd.source.mipLevel = subresources.baseMipLevel;
        copyCmd.source.aspect = Aspect::CombinedDepthStencil;
        copyCmd.destination.texture = stagingTexture.Get();
        copyCmd.destination.origin = {0, 0, 0};
        copyCmd.destination.mipLevel = 0;
        copyCmd.destination.aspect = Aspect::CombinedDepthStencil;
        copyCmd.copySize = size;
        DAWN_TRY(Texture::CopyInternal(commandContext, &copyCmd));
    }

    const auto aspectLayout =
        DepthStencilAspectLayout(d3d::DXGITextureFormat(GetFormat().format), subresources.aspects);

    // Map and write to the staging texture.
    ID3D11DeviceContext1* d3d11DeviceContext1 = commandContext->GetD3D11DeviceContext1();
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    const uint8_t* pSrcData = data;
    for (uint32_t layer = 0; layer < size.depthOrArrayLayers; ++layer) {
        DAWN_TRY(CheckHRESULT(d3d11DeviceContext1->Map(stagingTexture->GetD3D11Resource(), layer,
                                                       D3D11_MAP_READ, 0, &mappedResource),
                              "D3D11 map staging texture"));
        uint8_t* pDstData = static_cast<uint8_t*>(mappedResource.pData);
        for (uint32_t y = 0; y < size.height; ++y) {
            const uint8_t* pSrcRow = pSrcData;
            uint8_t* pDstRow = pDstData;
            pDstRow += aspectLayout.componentOffset;
            for (uint32_t x = 0; x < size.width; ++x) {
                std::memcpy(pDstRow, pSrcRow, aspectLayout.componentSize);
                pDstRow += aspectLayout.texelSize;
                pSrcRow += aspectLayout.componentSize;
            }
            pDstData += mappedResource.RowPitch;
            pSrcData += bytesPerRow;
        }
        d3d11DeviceContext1->Unmap(stagingTexture->GetD3D11Resource(), layer);
        ASSERT(size.height <= rowsPerImage);
        // Skip the padding rows.
        pSrcData += (rowsPerImage - size.height) * bytesPerRow;
    }

    // Copy to the dest texture from the staging texture.
    CopyTextureToTextureCmd copyCmd;
    copyCmd.source.texture = stagingTexture.Get();
    copyCmd.source.origin = {0, 0, 0};
    copyCmd.source.mipLevel = 0;
    copyCmd.source.aspect = Aspect::CombinedDepthStencil;
    copyCmd.destination.texture = this;
    copyCmd.destination.origin = origin;
    copyCmd.destination.mipLevel = subresources.baseMipLevel;
    copyCmd.destination.aspect = Aspect::CombinedDepthStencil;
    copyCmd.copySize = size;
    DAWN_TRY(Texture::CopyInternal(commandContext, &copyCmd));

    return {};
}

MaybeError Texture::ReadStaging(CommandRecordingContext* commandContext,
                                const SubresourceRange& subresources,
                                const Origin3D& origin,
                                Extent3D size,
                                uint32_t dstBytesPerRow,
                                uint32_t dstRowsPerImage,
                                Texture::ReadCallback callback) {
    ASSERT(size.width != 0 && size.height != 0 && size.depthOrArrayLayers != 0);
    ASSERT(mKind == Kind::Staging);
    ASSERT(subresources.baseArrayLayer == 0);
    ASSERT(origin.z == 0);

    ID3D11DeviceContext1* d3d11DeviceContext1 = commandContext->GetD3D11DeviceContext1();
    const TexelBlockInfo& blockInfo = GetFormat().GetAspectInfo(subresources.aspects).block;
    const bool hasStencil = GetFormat().HasStencil();
    ASSERT(size.width % blockInfo.width == 0);
    ASSERT(size.height % blockInfo.height == 0);
    const uint32_t bytesPerRow = blockInfo.byteSize * (size.width / blockInfo.width);
    const uint32_t rowsPerImage = size.height / blockInfo.height;

    if (GetDimension() == wgpu::TextureDimension::e2D) {
        for (uint32_t layer = 0; layer < subresources.layerCount; ++layer) {
            // Copy the staging texture to the buffer.
            // The Map() will block until the GPU is done with the texture.
            // TODO(dawn:1705): avoid blocking the CPU.
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            DAWN_TRY(CheckHRESULT(d3d11DeviceContext1->Map(GetD3D11Resource(), layer,
                                                           D3D11_MAP_READ, 0, &mappedResource),
                                  "D3D11 map staging texture"));

            uint8_t* pSrcData = static_cast<uint8_t*>(mappedResource.pData);
            uint64_t dstOffset = dstBytesPerRow * dstRowsPerImage * layer;
            if (dstBytesPerRow == bytesPerRow && mappedResource.RowPitch == bytesPerRow) {
                // If there is no padding in the rows, we can upload the whole image
                // in one read.
                DAWN_TRY(callback(pSrcData, dstOffset, dstBytesPerRow * rowsPerImage));
            } else if (hasStencil) {
                // We need to read texel by texel for depth-stencil formats.
                std::vector<uint8_t> depthOrStencilData(size.width * blockInfo.byteSize);
                const auto aspectLayout = DepthStencilAspectLayout(
                    d3d::DXGITextureFormat(GetFormat().format), subresources.aspects);
                ASSERT(blockInfo.byteSize == aspectLayout.componentSize);
                for (uint32_t y = 0; y < rowsPerImage; ++y) {
                    // Filter the depth/stencil data out.
                    uint8_t* src = pSrcData;
                    uint8_t* dst = depthOrStencilData.data();
                    src += aspectLayout.componentOffset;
                    for (uint32_t x = 0; x < size.width; ++x) {
                        std::memcpy(dst, src, aspectLayout.componentSize);
                        src += aspectLayout.texelSize;
                        dst += aspectLayout.componentSize;
                    }
                    DAWN_TRY(callback(depthOrStencilData.data(), dstOffset, bytesPerRow));
                    dstOffset += dstBytesPerRow;
                    pSrcData += mappedResource.RowPitch;
                }
            } else {
                // Otherwise, we need to read each row separately.
                for (uint32_t y = 0; y < rowsPerImage; ++y) {
                    DAWN_TRY(callback(pSrcData, dstOffset, bytesPerRow));
                    dstOffset += dstBytesPerRow;
                    pSrcData += mappedResource.RowPitch;
                }
            }
            d3d11DeviceContext1->Unmap(GetD3D11Resource(), layer);
        }
        return {};
    }

    // 3D textures are copied one slice at a time.
    // Copy the staging texture to the buffer.
    // The Map() will block until the GPU is done with the texture.
    // TODO(dawn:1705): avoid blocking the CPU.
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DAWN_TRY(CheckHRESULT(
        d3d11DeviceContext1->Map(GetD3D11Resource(), 0, D3D11_MAP_READ, 0, &mappedResource),
        "D3D11 map staging texture"));

    for (uint32_t z = 0; z < size.depthOrArrayLayers; ++z) {
        uint64_t dstOffset = dstBytesPerRow * dstRowsPerImage * z;
        uint8_t* pSrcData =
            static_cast<uint8_t*>(mappedResource.pData) + z * mappedResource.DepthPitch;
        if (dstBytesPerRow == bytesPerRow && mappedResource.RowPitch == bytesPerRow) {
            // If there is no padding in the rows, we can upload the whole image
            // in one read.
            DAWN_TRY(callback(pSrcData, dstOffset, bytesPerRow * size.height));
        } else {
            // Otherwise, we need to read each row separately.
            for (uint32_t y = 0; y < size.height; ++y) {
                DAWN_TRY(callback(pSrcData, dstOffset, bytesPerRow));
                dstOffset += dstBytesPerRow;
                pSrcData += mappedResource.RowPitch;
            }
        }
    }
    d3d11DeviceContext1->Unmap(GetD3D11Resource(), 0);

    return {};
}

MaybeError Texture::Read(CommandRecordingContext* commandContext,
                         const SubresourceRange& subresources,
                         const Origin3D& origin,
                         Extent3D size,
                         uint32_t dstBytesPerRow,
                         uint32_t dstRowsPerImage,
                         Texture::ReadCallback callback) {
    ASSERT(size.width != 0 && size.height != 0 && size.depthOrArrayLayers != 0);
    ASSERT(mKind != Kind::Staging);

    DAWN_TRY(EnsureSubresourceContentInitialized(commandContext, subresources));
    TextureDescriptor desc = {};
    desc.label = "CopyTextureToBufferStaging";
    desc.dimension = GetDimension();
    desc.size = size;
    desc.format = GetFormat().format;
    desc.mipLevelCount = subresources.levelCount;
    desc.sampleCount = GetSampleCount();

    Ref<Texture> stagingTexture;
    DAWN_TRY_ASSIGN(stagingTexture, CreateInternal(ToBackend(GetDevice()), &desc, Kind::Staging));

    CopyTextureToTextureCmd copyCmd;
    copyCmd.source.texture = this;
    copyCmd.source.origin = origin;
    copyCmd.source.mipLevel = subresources.baseMipLevel;
    copyCmd.source.aspect = subresources.aspects;
    copyCmd.destination.texture = stagingTexture.Get();
    copyCmd.destination.origin = {0, 0, 0};
    copyCmd.destination.mipLevel = 0;
    copyCmd.destination.aspect = subresources.aspects;
    copyCmd.copySize = size;

    DAWN_TRY(Texture::Copy(commandContext, &copyCmd));
    SubresourceRange stagingSubresources = SubresourceRange::MakeFull(
        subresources.aspects, subresources.layerCount, subresources.levelCount);

    return stagingTexture->ReadStaging(commandContext, stagingSubresources, {0, 0, 0}, size,
                                       dstBytesPerRow, dstRowsPerImage, callback);
}

// static
MaybeError Texture::Copy(CommandRecordingContext* commandContext, CopyTextureToTextureCmd* copy) {
    ASSERT(copy->copySize.width != 0 && copy->copySize.height != 0 &&
           copy->copySize.depthOrArrayLayers != 0);

    auto& src = copy->source;
    auto& dst = copy->destination;

    ASSERT(src.aspect == dst.aspect);

    // TODO(dawn:1705): support copy between textures with different dimensions.
    if (src.texture->GetDimension() != dst.texture->GetDimension()) {
        return DAWN_UNIMPLEMENTED_ERROR("Copy between textures with different dimensions");
    }

    SubresourceRange srcSubresources = GetSubresourcesAffectedByCopy(src, copy->copySize);
    DAWN_TRY(ToBackend(src.texture)
                 ->EnsureSubresourceContentInitialized(commandContext, srcSubresources));

    SubresourceRange dstSubresources = GetSubresourcesAffectedByCopy(dst, copy->copySize);
    if (IsCompleteSubresourceCopiedTo(dst.texture.Get(), copy->copySize, dst.mipLevel)) {
        dst.texture->SetIsSubresourceContentInitialized(true, dstSubresources);
    } else {
        // Partial update subresource of a depth/stencil texture is not allowed.
        ASSERT(!dst.texture->GetFormat().HasDepthOrStencil());
        DAWN_TRY(ToBackend(dst.texture)
                     ->EnsureSubresourceContentInitialized(commandContext, dstSubresources));
    }

    DAWN_TRY(CopyInternal(commandContext, copy));

    return {};
}

// static
MaybeError Texture::CopyInternal(CommandRecordingContext* commandContext,
                                 CopyTextureToTextureCmd* copy) {
    auto& src = copy->source;
    auto& dst = copy->destination;

    SubresourceRange srcSubresources = GetSubresourcesAffectedByCopy(src, copy->copySize);
    SubresourceRange dstSubresources = GetSubresourcesAffectedByCopy(dst, copy->copySize);

    D3D11_BOX srcBox;
    srcBox.left = src.origin.x;
    srcBox.right = src.origin.x + copy->copySize.width;
    srcBox.top = src.origin.y;
    srcBox.bottom = src.origin.y + copy->copySize.height;
    switch (src.texture->GetDimension()) {
        case wgpu::TextureDimension::e1D:
        case wgpu::TextureDimension::e2D:
            srcBox.front = 0;
            srcBox.back = 1;
            break;
        case wgpu::TextureDimension::e3D:
            srcBox.front = src.origin.z;
            srcBox.back = src.origin.z + copy->copySize.depthOrArrayLayers;
            break;
        default:
            UNREACHABLE();
    }

    bool isWholeSubresource = src.texture->CoverFullSubresource(src.mipLevel, copy->copySize) &&
                              dst.texture->CoverFullSubresource(dst.mipLevel, copy->copySize);
    // Partial update subresource of a depth/stencil texture is not allowed.
    ASSERT(isWholeSubresource || !src.texture->GetFormat().HasDepthOrStencil());

    for (uint32_t layer = 0; layer < srcSubresources.layerCount; ++layer) {
        uint32_t srcSubresource =
            src.texture->GetSubresourceIndex(src.mipLevel, srcSubresources.baseArrayLayer + layer,
                                             D3D11Aspect(srcSubresources.aspects));
        uint32_t dstSubresource =
            dst.texture->GetSubresourceIndex(dst.mipLevel, dstSubresources.baseArrayLayer + layer,
                                             D3D11Aspect(dstSubresources.aspects));
        commandContext->GetD3D11DeviceContext1()->CopySubresourceRegion(
            ToBackend(dst.texture)->GetD3D11Resource(), dstSubresource, dst.origin.x, dst.origin.y,
            dst.texture->GetDimension() == wgpu::TextureDimension::e3D ? dst.origin.z : 0,
            ToBackend(src.texture)->GetD3D11Resource(), srcSubresource,
            isWholeSubresource ? nullptr : &srcBox);
    }

    return {};
}

ResultOrError<ExecutionSerial> Texture::EndAccess() {
    // TODO(dawn:1705): submit pending commands if deferred context is used.
    return GetDevice()->GetLastSubmittedCommandSerial();
}

ResultOrError<ComPtr<ID3D11ShaderResourceView>> Texture::GetStencilSRV(
    CommandRecordingContext* commandContext,
    const TextureView* view) {
    ASSERT(GetFormat().HasStencil());

    if (!mTextureForStencilSampling.Get()) {
        // Create an interim texture of R8Uint format.
        TextureDescriptor desc = {};
        desc.label = "InterimStencilTexture";
        desc.dimension = GetDimension();
        desc.size = GetSize();
        desc.format = wgpu::TextureFormat::R8Uint;
        desc.mipLevelCount = GetNumMipLevels();
        desc.sampleCount = GetSampleCount();
        desc.usage = wgpu::TextureUsage::TextureBinding;

        DAWN_TRY_ASSIGN(mTextureForStencilSampling,
                        CreateInternal(ToBackend(GetDevice()), &desc, Kind::Interim));
    }

    // Sync the stencil data of this texture to the interim stencil-view texture.
    // TODO(dawn:1705): Improve to only sync as few as possible.
    const auto range = view->GetSubresourceRange();
    const TexelBlockInfo& blockInfo = GetFormat().GetAspectInfo(range.aspects).block;
    Extent3D size = GetMipLevelSubresourceVirtualSize(range.baseMipLevel);
    uint32_t bytesPerRow = blockInfo.byteSize * size.width;
    uint32_t rowsPerImage = size.height;
    uint64_t byteLength;
    DAWN_TRY_ASSIGN(byteLength,
                    ComputeRequiredBytesInCopy(blockInfo, size, bytesPerRow, rowsPerImage));

    std::vector<uint8_t> stagingData(byteLength);
    for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount;
         ++layer) {
        for (uint32_t level = range.baseMipLevel; level < range.baseMipLevel + range.levelCount;
             ++level) {
            size = GetMipLevelSubresourceVirtualSize(level);
            bytesPerRow = blockInfo.byteSize * size.width;
            rowsPerImage = size.height;
            auto singleRange = SubresourceRange::MakeSingle(range.aspects, layer, level);

            Texture::ReadCallback callback = [&](const uint8_t* data, uint64_t offset,
                                                 uint64_t length) -> MaybeError {
                std::memcpy(static_cast<uint8_t*>(stagingData.data()) + offset, data, length);
                return {};
            };

            // TODO(dawn:1705): Work out a way of GPU-GPU copy, rather than the CPU-GPU round trip.
            commandContext->GetDevice()->EmitWarningOnce(
                "Sampling the stencil component is rather slow now.");
            DAWN_TRY(Read(commandContext, singleRange, {0, 0, 0}, size, bytesPerRow, rowsPerImage,
                          callback));

            DAWN_TRY(mTextureForStencilSampling->WriteInternal(commandContext, singleRange,
                                                               {0, 0, 0}, size, stagingData.data(),
                                                               bytesPerRow, rowsPerImage));
        }
    }

    Ref<TextureViewBase> textureView;
    TextureViewDescriptor viewDesc = {};
    viewDesc.label = "InterimStencilTextureView";
    viewDesc.format = wgpu::TextureFormat::R8Uint;
    viewDesc.dimension = view->GetDimension();
    viewDesc.baseArrayLayer = view->GetBaseArrayLayer();
    viewDesc.arrayLayerCount = view->GetLayerCount();
    viewDesc.baseMipLevel = view->GetBaseMipLevel();
    viewDesc.mipLevelCount = view->GetLevelCount();
    DAWN_TRY_ASSIGN(textureView, mTextureForStencilSampling->CreateView(&viewDesc));

    ComPtr<ID3D11ShaderResourceView> srv;
    DAWN_TRY_ASSIGN(srv, ToBackend(textureView)->CreateD3D11ShaderResourceView());
    return srv;
}

// static
Ref<TextureView> TextureView::Create(TextureBase* texture,
                                     const TextureViewDescriptor* descriptor) {
    return AcquireRef(new TextureView(texture, descriptor));
}

TextureView::~TextureView() = default;

ResultOrError<ComPtr<ID3D11ShaderResourceView>> TextureView::CreateD3D11ShaderResourceView() const {
    Device* device = ToBackend(GetDevice());
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = d3d::DXGITextureFormat(GetFormat().format);

    const Format& textureFormat = GetTexture()->GetFormat();
    // TODO(dawn:1705): share below code with D3D12?
    if (textureFormat.HasDepthOrStencil()) {
        // Configure the SRV descriptor to reinterpret the texture allocated as
        // TYPELESS as a single-plane shader-accessible view.
        switch (textureFormat.format) {
            case wgpu::TextureFormat::Depth32Float:
            case wgpu::TextureFormat::Depth24Plus:
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case wgpu::TextureFormat::Depth16Unorm:
                srvDesc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            case wgpu::TextureFormat::Stencil8: {
                Aspect aspects = GetAspects();
                ASSERT(aspects != Aspect::None);
                if (!HasZeroOrOneBits(aspects)) {
                    // A single aspect is not selected. The texture view must not be
                    // sampled.
                    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                    break;
                }
                switch (aspects) {
                    case Aspect::Depth:
                        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                        break;
                    case Aspect::Stencil:
                        srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
                        break;
                    default:
                        UNREACHABLE();
                        break;
                }
                break;
            }
            case wgpu::TextureFormat::Depth24PlusStencil8:
            case wgpu::TextureFormat::Depth32FloatStencil8: {
                Aspect aspects = GetAspects();
                ASSERT(aspects != Aspect::None);
                if (!HasZeroOrOneBits(aspects)) {
                    // A single aspect is not selected. The texture view must not be
                    // sampled.
                    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                    break;
                }
                switch (aspects) {
                    case Aspect::Depth:
                        srvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                        break;
                    case Aspect::Stencil:
                        srvDesc.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
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

    // Currently we always use D3D11_TEX2D_ARRAY_SRV because we cannot specify base array
    // layer and layer count in D3D11_TEX2D_SRV. For 2D texture views, we treat them as
    // 1-layer 2D array textures. Multisampled textures may only be one array layer, so we
    // use D3D11_SRV_DIMENSION_TEXTURE2DMS.
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ns-d3d11-d3d11_tex2d_srv
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/ns-d3d11-d3d11_tex2d_array_srv
    if (GetTexture()->IsMultisampledTexture()) {
        switch (GetDimension()) {
            case wgpu::TextureViewDimension::e2DArray:
                ASSERT(GetTexture()->GetArrayLayers() == 1);
                [[fallthrough]];
            case wgpu::TextureViewDimension::e2D:
                ASSERT(GetTexture()->GetDimension() == wgpu::TextureDimension::e2D);
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                break;

            default:
                UNREACHABLE();
        }
    } else {
        switch (GetDimension()) {
            case wgpu::TextureViewDimension::e1D:
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D.MipLevels = GetLevelCount();
                srvDesc.Texture1D.MostDetailedMip = GetBaseMipLevel();
                break;

            case wgpu::TextureViewDimension::e2D:
            case wgpu::TextureViewDimension::e2DArray:
                ASSERT(GetTexture()->GetDimension() == wgpu::TextureDimension::e2D);
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.ArraySize = GetLayerCount();
                srvDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
                srvDesc.Texture2DArray.MipLevels = GetLevelCount();
                srvDesc.Texture2DArray.MostDetailedMip = GetBaseMipLevel();
                break;
            case wgpu::TextureViewDimension::Cube:
            case wgpu::TextureViewDimension::CubeArray:
                ASSERT(GetTexture()->GetDimension() == wgpu::TextureDimension::e2D);
                ASSERT(GetLayerCount() % 6 == 0);
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.First2DArrayFace = GetBaseArrayLayer();
                srvDesc.TextureCubeArray.NumCubes = GetLayerCount() / 6;
                srvDesc.TextureCubeArray.MipLevels = GetLevelCount();
                srvDesc.TextureCubeArray.MostDetailedMip = GetBaseMipLevel();
                break;
            case wgpu::TextureViewDimension::e3D:
                ASSERT(GetTexture()->GetDimension() == wgpu::TextureDimension::e3D);
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MostDetailedMip = GetBaseMipLevel();
                srvDesc.Texture3D.MipLevels = GetLevelCount();
                break;

            case wgpu::TextureViewDimension::Undefined:
                UNREACHABLE();
        }
    }

    ComPtr<ID3D11ShaderResourceView> srv;
    DAWN_TRY(CheckHRESULT(device->GetD3D11Device()->CreateShaderResourceView(
                              ToBackend(GetTexture())->GetD3D11Resource(), &srvDesc, &srv),
                          "CreateShaderResourceView"));

    return srv;
}

ResultOrError<ComPtr<ID3D11RenderTargetView>> TextureView::CreateD3D11RenderTargetView(
    uint32_t mipLevel) const {
    auto range = GetSubresourceRange();
    ASSERT(mipLevel >= range.baseMipLevel && mipLevel < range.baseMipLevel + range.levelCount);
    range.baseMipLevel = mipLevel;
    range.levelCount = 1u;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc =
        ToBackend(GetTexture())->GetRTVDescriptor(GetFormat(), range);
    ComPtr<ID3D11RenderTargetView> rtv;
    DAWN_TRY(CheckHRESULT(
        ToBackend(GetDevice())
            ->GetD3D11Device()
            ->CreateRenderTargetView(ToBackend(GetTexture())->GetD3D11Resource(), &rtvDesc, &rtv),
        "CreateRenderTargetView"));
    return rtv;
}

ResultOrError<ComPtr<ID3D11DepthStencilView>> TextureView::CreateD3D11DepthStencilView(
    bool depthReadOnly,
    bool stencilReadOnly,
    uint32_t mipLevel) const {
    auto range = GetSubresourceRange();
    ASSERT(mipLevel >= range.baseMipLevel && mipLevel < range.baseMipLevel + range.levelCount);
    range.baseMipLevel = mipLevel;
    range.levelCount = 1u;
    ComPtr<ID3D11DepthStencilView> dsv;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc =
        ToBackend(GetTexture())->GetDSVDescriptor(range, depthReadOnly, stencilReadOnly);
    DAWN_TRY(CheckHRESULT(
        ToBackend(GetDevice())
            ->GetD3D11Device()
            ->CreateDepthStencilView(ToBackend(GetTexture())->GetD3D11Resource(), &dsvDesc, &dsv),
        "CreateDepthStencilView"));
    return dsv;
}

ResultOrError<ComPtr<ID3D11UnorderedAccessView>> TextureView::CreateD3D11UnorderedAccessView()
    const {
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = d3d::DXGITextureFormat(GetFormat().format);

    ASSERT(!GetTexture()->IsMultisampledTexture());
    switch (GetDimension()) {
        case wgpu::TextureViewDimension::e1D:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = GetBaseMipLevel();
            break;
        case wgpu::TextureViewDimension::e2D:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = GetBaseMipLevel();
            break;
        case wgpu::TextureViewDimension::e2DArray:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.FirstArraySlice = GetBaseArrayLayer();
            uavDesc.Texture2DArray.ArraySize = GetLayerCount();
            uavDesc.Texture2DArray.MipSlice = GetBaseMipLevel();
            break;
        case wgpu::TextureViewDimension::e3D:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
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

    ComPtr<ID3D11UnorderedAccessView> uav;
    DAWN_TRY(CheckHRESULT(ToBackend(GetDevice())
                              ->GetD3D11Device()
                              ->CreateUnorderedAccessView(
                                  ToBackend(GetTexture())->GetD3D11Resource(), &uavDesc, &uav),
                          "CreateUnorderedAccessView"));

    SetDebugName(ToBackend(GetDevice()), uav.Get(), "Dawn_TextureView", GetLabel());

    return uav;
}

}  // namespace dawn::native::d3d11
