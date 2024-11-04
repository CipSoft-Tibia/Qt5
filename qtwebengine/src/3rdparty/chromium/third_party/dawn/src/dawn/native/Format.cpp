// Copyright 2019 The Dawn Authors
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

#include "dawn/native/Format.h"

#include <bitset>

#include "dawn/common/TypedInteger.h"
#include "dawn/native/Device.h"
#include "dawn/native/EnumMaskIterator.h"
#include "dawn/native/Features.h"
#include "dawn/native/Texture.h"

namespace dawn {
enum class Cap : uint16_t {
    None = 0x0,
    Multisample = 0x1,
    Renderable = 0x2,
    Resolve = 0x4,
    StorageW = 0x8,
    StorageRW = 0x10,  // Implies StorageW
};

template <>
struct IsDawnBitmask<Cap> {
    static constexpr bool enable = true;
};
}  // namespace dawn

namespace dawn::native {

// Format

SampleTypeBit SampleTypeToSampleTypeBit(wgpu::TextureSampleType sampleType) {
    switch (sampleType) {
        case wgpu::TextureSampleType::Float:
        case wgpu::TextureSampleType::UnfilterableFloat:
        case wgpu::TextureSampleType::Sint:
        case wgpu::TextureSampleType::Uint:
        case wgpu::TextureSampleType::Depth:
        case wgpu::TextureSampleType::Undefined:
            // When the compiler complains that you need to add a case statement here, please
            // also add a corresponding static assert below!
            break;
    }

    static_assert(static_cast<uint32_t>(wgpu::TextureSampleType::Undefined) == 0);
    if (sampleType == wgpu::TextureSampleType::Undefined) {
        return SampleTypeBit::None;
    }

    // Check that SampleTypeBit bits are in the same position / order as the respective
    // wgpu::TextureSampleType value.
    static_assert(SampleTypeBit::Float ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Float) - 1)));
    static_assert(
        SampleTypeBit::UnfilterableFloat ==
        static_cast<SampleTypeBit>(
            1 << (static_cast<uint32_t>(wgpu::TextureSampleType::UnfilterableFloat) - 1)));
    static_assert(SampleTypeBit::Uint ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Uint) - 1)));
    static_assert(SampleTypeBit::Sint ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Sint) - 1)));
    static_assert(SampleTypeBit::Depth ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Depth) - 1)));
    return static_cast<SampleTypeBit>(1 << (static_cast<uint32_t>(sampleType) - 1));
}

const UnsupportedReason Format::supported;

bool Format::IsSupported() const {
    return std::holds_alternative<std::monostate>(unsupportedReason);
}

bool Format::IsColor() const {
    return aspects == Aspect::Color;
}

bool Format::HasDepth() const {
    return aspects & Aspect::Depth;
}

bool Format::HasStencil() const {
    return aspects & Aspect::Stencil;
}

bool Format::HasDepthOrStencil() const {
    return aspects & (Aspect::Depth | Aspect::Stencil);
}

bool Format::HasAlphaChannel() const {
    // This is true for current formats. May need revisit if new formats and extensions are added.
    return componentCount == 4 && IsColor();
}

bool Format::IsSnorm() const {
    return format == wgpu::TextureFormat::RGBA8Snorm || format == wgpu::TextureFormat::RG8Snorm ||
           format == wgpu::TextureFormat::R8Snorm;
}

bool Format::IsMultiPlanar() const {
    return aspects & (Aspect::Plane0 | Aspect::Plane1);
}

bool Format::CopyCompatibleWith(const Format& otherFormat) const {
    // TODO(crbug.com/dawn/1332): Add a Format compatibility matrix.
    return baseFormat == otherFormat.baseFormat;
}

bool Format::ViewCompatibleWith(const Format& otherFormat) const {
    // TODO(crbug.com/dawn/1332): Add a Format compatibility matrix.
    return baseFormat == otherFormat.baseFormat;
}

const AspectInfo& Format::GetAspectInfo(wgpu::TextureAspect aspect) const {
    return GetAspectInfo(SelectFormatAspects(*this, aspect));
}

const AspectInfo& Format::GetAspectInfo(Aspect aspect) const {
    ASSERT(HasOneBit(aspect));
    ASSERT(aspects & aspect);
    const size_t aspectIndex = GetAspectIndex(aspect);
    ASSERT(aspectIndex < GetAspectCount(aspects));
    return aspectInfo[aspectIndex];
}

Extent3D Format::GetAspectSize(Aspect aspect, const Extent3D& textureSize) const {
    switch (aspect) {
        case Aspect::Color:
        case Aspect::Depth:
        case Aspect::Stencil:
        case Aspect::CombinedDepthStencil:
            return textureSize;
        case Aspect::Plane0:
            ASSERT(IsMultiPlanar());
            return textureSize;
        case Aspect::Plane1: {
            ASSERT(IsMultiPlanar());
            auto planeSize = textureSize;
            switch (format) {
                case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
                    if (planeSize.width > 1) {
                        planeSize.width >>= 1;
                    }
                    if (planeSize.height > 1) {
                        planeSize.height >>= 1;
                    }
                    break;
                default:
                    UNREACHABLE();
            }
            return planeSize;
        }
        case Aspect::None:
            break;
    }

    UNREACHABLE();
}

FormatIndex Format::GetIndex() const {
    return ComputeFormatIndex(format);
}

// FormatSet implementation

bool FormatSet::operator[](const Format& format) const {
    return Base::operator[](format.GetIndex());
}

typename std::bitset<kKnownFormatCount>::reference FormatSet::operator[](const Format& format) {
    return Base::operator[](format.GetIndex());
}

// Implementation details of the format table of the DeviceBase

// For the enum for formats are packed but this might change when we have a broader feature
// mechanism for webgpu.h. Formats start at 1 because 0 is the undefined format.
FormatIndex ComputeFormatIndex(wgpu::TextureFormat format) {
    // This takes advantage of overflows to make the index of TextureFormat::Undefined outside
    // of the range of the FormatTable.
    static_assert(static_cast<uint32_t>(wgpu::TextureFormat::Undefined) - 1 > kKnownFormatCount);
    return static_cast<FormatIndex>(static_cast<uint32_t>(format) - 1);
}

FormatTable BuildFormatTable(const DeviceBase* device) {
    FormatTable table;
    FormatSet formatsSet;

    static constexpr SampleTypeBit kAnyFloat =
        SampleTypeBit::Float | SampleTypeBit::UnfilterableFloat;

    auto AddFormat = [&table, &formatsSet](Format format) {
        FormatIndex index = ComputeFormatIndex(format.format);
        ASSERT(index < table.size());

        // This checks that each format is set at most once, the first part of checking that all
        // formats are set exactly once.
        ASSERT(!formatsSet[index]);

        // Vulkan describes bytesPerRow in units of texels. If there's any format for which this
        // ASSERT isn't true, then additional validation on bytesPerRow must be added.
        const bool hasMultipleAspects = !HasOneBit(format.aspects);
        ASSERT(hasMultipleAspects ||
               (kTextureBytesPerRowAlignment % format.aspectInfo[0].block.byteSize) == 0);

        table[index] = format;
        formatsSet.set(index);
    };

    using Width = TypedInteger<struct WidthT, uint32_t>;
    using Height = TypedInteger<struct HeightT, uint32_t>;
    using ByteSize = TypedInteger<struct ByteSizeT, uint32_t>;
    using RenderTargetPixelByteCost = TypedInteger<struct RenderTargetPixelByteCostT, uint32_t>;
    using ComponentCount = TypedInteger<struct ComponentCountT, uint32_t>;
    using RenderTargetComponentAlignment =
        TypedInteger<struct RenderTargetComponentAlignmentT, uint32_t>;

    auto AddConditionalColorFormat =
        [&AddFormat](
            wgpu::TextureFormat format, UnsupportedReason unsupportedReason, Cap capabilities,
            ByteSize byteSize, SampleTypeBit sampleTypes, ComponentCount componentCount,
            RenderTargetPixelByteCost renderTargetPixelByteCost = RenderTargetPixelByteCost(0),
            RenderTargetComponentAlignment renderTargetComponentAlignment =
                RenderTargetComponentAlignment(0),
            wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
            Format internalFormat;
            internalFormat.format = format;
            bool renderable = capabilities & Cap::Renderable;
            internalFormat.isRenderable = renderable;
            internalFormat.isCompressed = false;
            internalFormat.unsupportedReason = unsupportedReason;
            internalFormat.supportsStorageUsage = capabilities & (Cap::StorageW | Cap::StorageRW);
            internalFormat.supportsReadWriteStorageUsage = capabilities & Cap::StorageRW;

            bool supportsMultisample = capabilities & Cap::Multisample;
            if (supportsMultisample) {
                ASSERT(renderable);
            }
            internalFormat.supportsMultisample = supportsMultisample;
            internalFormat.supportsResolveTarget = capabilities & Cap::Resolve;
            internalFormat.aspects = Aspect::Color;
            internalFormat.componentCount = static_cast<uint32_t>(componentCount);
            if (renderable) {
                // If the color format is renderable, it must have a pixel byte size and component
                // alignment specified.
                ASSERT(renderTargetPixelByteCost != RenderTargetPixelByteCost(0) &&
                       renderTargetComponentAlignment != RenderTargetComponentAlignment(0));
                internalFormat.renderTargetPixelByteCost =
                    static_cast<uint32_t>(renderTargetPixelByteCost);
                internalFormat.renderTargetComponentAlignment =
                    static_cast<uint32_t>(renderTargetComponentAlignment);
            }

            // Default baseFormat of each color formats should be themselves.
            if (baseFormat == wgpu::TextureFormat::Undefined) {
                internalFormat.baseFormat = format;
            } else {
                internalFormat.baseFormat = baseFormat;
            }

            AspectInfo* firstAspect = internalFormat.aspectInfo.data();
            firstAspect->block.byteSize = static_cast<uint32_t>(byteSize);
            firstAspect->block.width = 1;
            firstAspect->block.height = 1;
            if (HasOneBit(sampleTypes)) {
                switch (sampleTypes) {
                    case SampleTypeBit::Float:
                    case SampleTypeBit::UnfilterableFloat:
                        firstAspect->baseType = TextureComponentType::Float;
                        break;
                    case SampleTypeBit::Sint:
                        firstAspect->baseType = TextureComponentType::Sint;
                        break;
                    case SampleTypeBit::Uint:
                        firstAspect->baseType = TextureComponentType::Uint;
                        break;
                    default:
                        UNREACHABLE();
                }
            } else {
                ASSERT(sampleTypes & SampleTypeBit::Float);
                firstAspect->baseType = TextureComponentType::Float;
            }
            firstAspect->supportedSampleTypes = sampleTypes;
            firstAspect->format = format;
            AddFormat(internalFormat);
        };

    auto AddColorFormat = [&AddConditionalColorFormat](
                              wgpu::TextureFormat format, Cap capabilites, ByteSize byteSize,
                              SampleTypeBit sampleTypes, ComponentCount componentCount,
                              RenderTargetPixelByteCost renderTargetPixelByteCost =
                                  RenderTargetPixelByteCost(0),
                              RenderTargetComponentAlignment renderTargetComponentAlignment =
                                  RenderTargetComponentAlignment(0),
                              wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
        AddConditionalColorFormat(format, std::monostate{}, capabilites, byteSize, sampleTypes,
                                  componentCount, renderTargetPixelByteCost,
                                  renderTargetComponentAlignment, baseFormat);
    };

    auto AddDepthFormat = [&AddFormat](wgpu::TextureFormat format, uint32_t byteSize,
                                       UnsupportedReason unsupportedReason) {
        Format internalFormat;
        internalFormat.format = format;
        internalFormat.baseFormat = format;
        internalFormat.isRenderable = true;
        internalFormat.isCompressed = false;
        internalFormat.unsupportedReason = unsupportedReason;
        internalFormat.supportsStorageUsage = false;
        internalFormat.supportsMultisample = true;
        internalFormat.supportsResolveTarget = false;
        internalFormat.aspects = Aspect::Depth;
        internalFormat.componentCount = 1;

        AspectInfo* firstAspect = internalFormat.aspectInfo.data();
        firstAspect->block.byteSize = byteSize;
        firstAspect->block.width = 1;
        firstAspect->block.height = 1;
        firstAspect->baseType = TextureComponentType::Float;
        firstAspect->supportedSampleTypes = SampleTypeBit::Depth | SampleTypeBit::UnfilterableFloat;
        firstAspect->format = format;
        AddFormat(internalFormat);
    };

    auto AddStencilFormat = [&AddFormat](wgpu::TextureFormat format,
                                         UnsupportedReason unsupportedReason) {
        Format internalFormat;
        internalFormat.format = format;
        internalFormat.baseFormat = format;
        internalFormat.isRenderable = true;
        internalFormat.isCompressed = false;
        internalFormat.unsupportedReason = unsupportedReason;
        internalFormat.supportsStorageUsage = false;
        internalFormat.supportsMultisample = true;
        internalFormat.supportsResolveTarget = false;
        internalFormat.aspects = Aspect::Stencil;
        internalFormat.componentCount = 1;

        // Duplicate the data for the stencil aspect in both the first and second aspect info.
        //  - aspectInfo[0] is used by AddMultiAspectFormat to copy the info for the whole
        //    stencil8 aspect of depth-stencil8 formats.
        //  - aspectInfo[1] is the actual info used in the rest of Dawn since
        //    GetAspectIndex(Aspect::Stencil) is 1.
        ASSERT(GetAspectIndex(Aspect::Stencil) == 1);

        internalFormat.aspectInfo[0].block.byteSize = 1;
        internalFormat.aspectInfo[0].block.width = 1;
        internalFormat.aspectInfo[0].block.height = 1;
        internalFormat.aspectInfo[0].baseType = TextureComponentType::Uint;
        internalFormat.aspectInfo[0].supportedSampleTypes = SampleTypeBit::Uint;
        internalFormat.aspectInfo[0].format = format;

        internalFormat.aspectInfo[1] = internalFormat.aspectInfo[0];

        AddFormat(internalFormat);
    };

    auto AddCompressedFormat =
        [&AddFormat](wgpu::TextureFormat format, ByteSize byteSize, Width width, Height height,
                     UnsupportedReason unsupportedReason, ComponentCount componentCount,
                     wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
            Format internalFormat;
            internalFormat.format = format;
            internalFormat.isRenderable = false;
            internalFormat.isCompressed = true;
            internalFormat.unsupportedReason = unsupportedReason;
            internalFormat.supportsStorageUsage = false;
            internalFormat.supportsMultisample = false;
            internalFormat.supportsResolveTarget = false;
            internalFormat.aspects = Aspect::Color;
            internalFormat.componentCount = static_cast<uint32_t>(componentCount);

            // Default baseFormat of each compressed formats should be themselves.
            if (baseFormat == wgpu::TextureFormat::Undefined) {
                internalFormat.baseFormat = format;
            } else {
                internalFormat.baseFormat = baseFormat;
            }

            AspectInfo* firstAspect = internalFormat.aspectInfo.data();
            firstAspect->block.byteSize = static_cast<uint32_t>(byteSize);
            firstAspect->block.width = static_cast<uint32_t>(width);
            firstAspect->block.height = static_cast<uint32_t>(height);
            firstAspect->baseType = TextureComponentType::Float;
            firstAspect->supportedSampleTypes = kAnyFloat;
            firstAspect->format = format;
            AddFormat(internalFormat);
        };

    auto AddMultiAspectFormat =
        [&AddFormat, &table](wgpu::TextureFormat format, Aspect aspects,
                             wgpu::TextureFormat firstFormat, wgpu::TextureFormat secondFormat,
                             Cap capabilites, UnsupportedReason unsupportedReason,
                             ComponentCount componentCount) {
            Format internalFormat;
            internalFormat.format = format;
            internalFormat.baseFormat = format;
            internalFormat.isRenderable = capabilites & Cap::Renderable;
            internalFormat.isCompressed = false;
            internalFormat.unsupportedReason = unsupportedReason;
            internalFormat.supportsStorageUsage = false;
            internalFormat.supportsMultisample = capabilites & Cap::Multisample;
            internalFormat.supportsResolveTarget = false;
            internalFormat.aspects = aspects;
            internalFormat.componentCount = static_cast<uint32_t>(componentCount);

            // Multi aspect formats just copy information about single-aspect formats. This
            // means that the single-plane formats must have been added before multi-aspect
            // ones. (it is ASSERTed below).
            const FormatIndex firstFormatIndex = ComputeFormatIndex(firstFormat);
            const FormatIndex secondFormatIndex = ComputeFormatIndex(secondFormat);

            ASSERT(table[firstFormatIndex].aspectInfo[0].format != wgpu::TextureFormat::Undefined);
            ASSERT(table[secondFormatIndex].aspectInfo[0].format != wgpu::TextureFormat::Undefined);

            internalFormat.aspectInfo[0] = table[firstFormatIndex].aspectInfo[0];
            internalFormat.aspectInfo[1] = table[secondFormatIndex].aspectInfo[0];

            AddFormat(internalFormat);
        };

    // clang-format off
    // 1 byte color formats
    AddColorFormat(wgpu::TextureFormat::R8Unorm, Cap::Renderable | Cap::Multisample | Cap::Resolve, ByteSize(1), kAnyFloat, ComponentCount(1), RenderTargetPixelByteCost(1), RenderTargetComponentAlignment(1));
    AddColorFormat(wgpu::TextureFormat::R8Snorm, Cap::None, ByteSize(1), kAnyFloat, ComponentCount(1));
    AddColorFormat(wgpu::TextureFormat::R8Uint, Cap::Renderable | Cap::Multisample, ByteSize(1), SampleTypeBit::Uint, ComponentCount(1), RenderTargetPixelByteCost(1), RenderTargetComponentAlignment(1));
    AddColorFormat(wgpu::TextureFormat::R8Sint, Cap::Renderable | Cap::Multisample, ByteSize(1), SampleTypeBit::Sint, ComponentCount(1), RenderTargetPixelByteCost(1), RenderTargetComponentAlignment(1));

    // 2 bytes color formats
    AddColorFormat(wgpu::TextureFormat::R16Uint, Cap::Renderable | Cap::Multisample, ByteSize(2), SampleTypeBit::Uint, ComponentCount(1), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::R16Sint, Cap::Renderable | Cap::Multisample, ByteSize(2), SampleTypeBit::Sint, ComponentCount(1), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::R16Float, Cap::Renderable | Cap::Multisample | Cap::Resolve, ByteSize(2), kAnyFloat, ComponentCount(1), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RG8Unorm, Cap::Renderable | Cap::Multisample | Cap::Resolve, ByteSize(2), kAnyFloat, ComponentCount(2), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(1));
    AddColorFormat(wgpu::TextureFormat::RG8Snorm, Cap::None, ByteSize(2), kAnyFloat, ComponentCount(2));
    AddColorFormat(wgpu::TextureFormat::RG8Uint, Cap::Renderable | Cap::Multisample, ByteSize(2), SampleTypeBit::Uint, ComponentCount(2), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(1));
    AddColorFormat(wgpu::TextureFormat::RG8Sint, Cap::Renderable | Cap::Multisample, ByteSize(2), SampleTypeBit::Sint, ComponentCount(2), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(1));

    // 4 bytes color formats
    SampleTypeBit sampleTypeFor32BitFloatFormats = device->HasFeature(Feature::Float32Filterable) ? kAnyFloat : SampleTypeBit::UnfilterableFloat;
    auto supportsReadWriteStorageUsage = device->HasFeature(Feature::ChromiumExperimentalReadWriteStorageTexture) ? Cap::StorageRW : Cap::None;

    AddColorFormat(wgpu::TextureFormat::R32Uint, Cap::Renderable | Cap::StorageW | supportsReadWriteStorageUsage, ByteSize(4), SampleTypeBit::Uint, ComponentCount(1), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::R32Sint, Cap::Renderable | Cap::StorageW | supportsReadWriteStorageUsage, ByteSize(4), SampleTypeBit::Sint, ComponentCount(1), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::R32Float, Cap::Renderable | Cap::Multisample | Cap::StorageW | supportsReadWriteStorageUsage, ByteSize(4), sampleTypeFor32BitFloatFormats, ComponentCount(1), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RG16Uint, Cap::Renderable | Cap::Multisample, ByteSize(4), SampleTypeBit::Uint, ComponentCount(2), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RG16Sint, Cap::Renderable | Cap::Multisample, ByteSize(4), SampleTypeBit::Sint, ComponentCount(2), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RG16Float, Cap::Renderable | Cap::Multisample | Cap::Resolve, ByteSize(4), kAnyFloat, ComponentCount(2), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RGBA8Unorm, Cap::Renderable | Cap::StorageW | Cap::Multisample | Cap::Resolve, ByteSize(4), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(1));
    AddColorFormat(wgpu::TextureFormat::RGBA8UnormSrgb, Cap::Renderable | Cap::Multisample | Cap::Resolve, ByteSize(4), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(1), wgpu::TextureFormat::RGBA8Unorm);
    AddColorFormat(wgpu::TextureFormat::RGBA8Snorm, Cap::StorageW, ByteSize(4), kAnyFloat, ComponentCount(4));
    AddColorFormat(wgpu::TextureFormat::RGBA8Uint, Cap::Renderable | Cap::StorageW | Cap::Multisample, ByteSize(4), SampleTypeBit::Uint, ComponentCount(4), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(1));
    AddColorFormat(wgpu::TextureFormat::RGBA8Sint, Cap::Renderable | Cap::StorageW | Cap::Multisample, ByteSize(4), SampleTypeBit::Sint, ComponentCount(4), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(1));

    auto BGRA8UnormSupportsStorageUsage = device->HasFeature(Feature::BGRA8UnormStorage) ? Cap::StorageW : Cap::None;
    AddColorFormat(wgpu::TextureFormat::BGRA8Unorm, Cap::Renderable | BGRA8UnormSupportsStorageUsage | Cap::Multisample | Cap::Resolve, ByteSize(4), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(1));
    AddConditionalColorFormat(wgpu::TextureFormat::BGRA8UnormSrgb, device->IsCompatibilityMode() ? UnsupportedReason(CompatibilityMode{}) : Format::supported, Cap::Renderable |  Cap::Multisample | Cap::Resolve, ByteSize(4), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(1), wgpu::TextureFormat::BGRA8Unorm);
    AddColorFormat(wgpu::TextureFormat::RGB10A2Unorm, Cap::Renderable |  Cap::Multisample | Cap::Resolve, ByteSize(4), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(4));

    auto isRG11B10UfloatCapabilities = device->HasFeature(Feature::RG11B10UfloatRenderable) ? Cap::Renderable | Cap::Multisample | Cap::Resolve : Cap::None;
    AddColorFormat(wgpu::TextureFormat::RG11B10Ufloat, isRG11B10UfloatCapabilities, ByteSize(4), kAnyFloat, ComponentCount(3), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RGB9E5Ufloat, Cap::None, ByteSize(4), kAnyFloat, ComponentCount(3));

    // 8 bytes color formats
    AddColorFormat(wgpu::TextureFormat::RG32Uint, Cap::Renderable | Cap::StorageW, ByteSize(8), SampleTypeBit::Uint, ComponentCount(2), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RG32Sint, Cap::Renderable | Cap::StorageW, ByteSize(8), SampleTypeBit::Sint, ComponentCount(2), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RG32Float, Cap::Renderable | Cap::StorageW, ByteSize(8), sampleTypeFor32BitFloatFormats, ComponentCount(2), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RGBA16Uint, Cap::Renderable | Cap::StorageW | Cap::Multisample, ByteSize(8), SampleTypeBit::Uint, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RGBA16Sint, Cap::Renderable | Cap::StorageW | Cap::Multisample, ByteSize(8), SampleTypeBit::Sint, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RGBA16Float, Cap::Renderable | Cap::StorageW | Cap::Multisample | Cap::Resolve, ByteSize(8), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(2));

    // 16 bytes color formats
    AddColorFormat(wgpu::TextureFormat::RGBA32Uint, Cap::Renderable | Cap::StorageW, ByteSize(16), SampleTypeBit::Uint, ComponentCount(4), RenderTargetPixelByteCost(16), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RGBA32Sint, Cap::Renderable | Cap::StorageW, ByteSize(16), SampleTypeBit::Sint, ComponentCount(4), RenderTargetPixelByteCost(16), RenderTargetComponentAlignment(4));
    AddColorFormat(wgpu::TextureFormat::RGBA32Float, Cap::Renderable | Cap::StorageW, ByteSize(16), sampleTypeFor32BitFloatFormats, ComponentCount(4), RenderTargetPixelByteCost(16), RenderTargetComponentAlignment(4));

    // Norm16 color formats
    auto norm16Capabilities = device->HasFeature(Feature::Norm16TextureFormats) ? Cap::Renderable | Cap::Multisample | Cap::Resolve : Cap::None;
    AddColorFormat(wgpu::TextureFormat::R16Unorm, norm16Capabilities, ByteSize(2), kAnyFloat, ComponentCount(1), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RG16Unorm, norm16Capabilities, ByteSize(4), kAnyFloat, ComponentCount(2), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RGBA16Unorm, norm16Capabilities, ByteSize(8), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::R16Snorm, norm16Capabilities, ByteSize(2), kAnyFloat, ComponentCount(1), RenderTargetPixelByteCost(2), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RG16Snorm, norm16Capabilities, ByteSize(4), kAnyFloat, ComponentCount(2), RenderTargetPixelByteCost(4), RenderTargetComponentAlignment(2));
    AddColorFormat(wgpu::TextureFormat::RGBA16Snorm, norm16Capabilities, ByteSize(8), kAnyFloat, ComponentCount(4), RenderTargetPixelByteCost(8), RenderTargetComponentAlignment(2));

    // Depth-stencil formats
    AddStencilFormat(wgpu::TextureFormat::Stencil8, Format::supported);
    AddDepthFormat(wgpu::TextureFormat::Depth16Unorm, 2, Format::supported);
    // TODO(crbug.com/dawn/843): This is 4 because we read this to perform zero initialization,
    // and textures are always use depth32float. We should improve this to be more robust. Perhaps,
    // using 0 here to mean "unsized" and adding a backend-specific query for the block size.
    AddDepthFormat(wgpu::TextureFormat::Depth24Plus, 4, Format::supported);
    AddMultiAspectFormat(wgpu::TextureFormat::Depth24PlusStencil8,
                          Aspect::Depth | Aspect::Stencil, wgpu::TextureFormat::Depth24Plus, wgpu::TextureFormat::Stencil8, Cap::Renderable | Cap::Multisample, Format::supported, ComponentCount(2));
    AddDepthFormat(wgpu::TextureFormat::Depth32Float, 4, Format::supported);
    UnsupportedReason d32s8UnsupportedReason = device->HasFeature(Feature::Depth32FloatStencil8) ? Format::supported : RequiresFeature{wgpu::FeatureName::Depth32FloatStencil8};
    AddMultiAspectFormat(wgpu::TextureFormat::Depth32FloatStencil8,
                          Aspect::Depth | Aspect::Stencil, wgpu::TextureFormat::Depth32Float, wgpu::TextureFormat::Stencil8, Cap::Renderable | Cap::Multisample, d32s8UnsupportedReason, ComponentCount(2));

    // BC compressed formats
    UnsupportedReason bcFormatUnsupportedReason = device->HasFeature(Feature::TextureCompressionBC) ? Format::supported : RequiresFeature{wgpu::FeatureName::TextureCompressionBC};
    AddCompressedFormat(wgpu::TextureFormat::BC1RGBAUnorm, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::BC1RGBAUnormSrgb, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC1RGBAUnorm);
    AddCompressedFormat(wgpu::TextureFormat::BC4RSnorm, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(1));
    AddCompressedFormat(wgpu::TextureFormat::BC4RUnorm, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(1));
    AddCompressedFormat(wgpu::TextureFormat::BC2RGBAUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::BC2RGBAUnormSrgb, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC2RGBAUnorm);
    AddCompressedFormat(wgpu::TextureFormat::BC3RGBAUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::BC3RGBAUnormSrgb, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC3RGBAUnorm);
    AddCompressedFormat(wgpu::TextureFormat::BC5RGSnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(2));
    AddCompressedFormat(wgpu::TextureFormat::BC5RGUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(2));
    AddCompressedFormat(wgpu::TextureFormat::BC6HRGBFloat, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(3));
    AddCompressedFormat(wgpu::TextureFormat::BC6HRGBUfloat, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(3));
    AddCompressedFormat(wgpu::TextureFormat::BC7RGBAUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::BC7RGBAUnormSrgb, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC7RGBAUnorm);

    // ETC2/EAC compressed formats
    UnsupportedReason etc2FormatUnsupportedReason = device->HasFeature(Feature::TextureCompressionETC2) ?  Format::supported : RequiresFeature{wgpu::FeatureName::TextureCompressionETC2};
    AddCompressedFormat(wgpu::TextureFormat::ETC2RGB8Unorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(3));
    AddCompressedFormat(wgpu::TextureFormat::ETC2RGB8UnormSrgb, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(3), wgpu::TextureFormat::ETC2RGB8Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ETC2RGB8A1Unorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ETC2RGB8A1UnormSrgb, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ETC2RGB8A1Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ETC2RGBA8Unorm, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ETC2RGBA8UnormSrgb, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ETC2RGBA8Unorm);
    AddCompressedFormat(wgpu::TextureFormat::EACR11Unorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(1));
    AddCompressedFormat(wgpu::TextureFormat::EACR11Snorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(1));
    AddCompressedFormat(wgpu::TextureFormat::EACRG11Unorm, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(2));
    AddCompressedFormat(wgpu::TextureFormat::EACRG11Snorm, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(2));

    // ASTC compressed formats
    UnsupportedReason astcFormatUnsupportedReason = device->HasFeature(Feature::TextureCompressionASTC) ?  Format::supported : RequiresFeature{wgpu::FeatureName::TextureCompressionASTC};
    AddCompressedFormat(wgpu::TextureFormat::ASTC4x4Unorm, ByteSize(16), Width(4), Height(4), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC4x4UnormSrgb, ByteSize(16), Width(4), Height(4), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC4x4Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC5x4Unorm, ByteSize(16), Width(5), Height(4), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC5x4UnormSrgb, ByteSize(16), Width(5), Height(4), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC5x4Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC5x5Unorm, ByteSize(16), Width(5), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC5x5UnormSrgb, ByteSize(16), Width(5), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC5x5Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC6x5Unorm, ByteSize(16), Width(6), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC6x5UnormSrgb, ByteSize(16), Width(6), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC6x5Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC6x6Unorm, ByteSize(16), Width(6), Height(6), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC6x6UnormSrgb, ByteSize(16), Width(6), Height(6), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC6x6Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC8x5Unorm, ByteSize(16), Width(8), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC8x5UnormSrgb, ByteSize(16), Width(8), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC8x5Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC8x6Unorm, ByteSize(16), Width(8), Height(6), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC8x6UnormSrgb, ByteSize(16), Width(8), Height(6), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC8x6Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC8x8Unorm, ByteSize(16), Width(8), Height(8), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC8x8UnormSrgb, ByteSize(16), Width(8), Height(8), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC8x8Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x5Unorm, ByteSize(16), Width(10), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x5UnormSrgb, ByteSize(16), Width(10), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x5Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x6Unorm, ByteSize(16), Width(10), Height(6), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x6UnormSrgb, ByteSize(16), Width(10), Height(6), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x6Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x8Unorm, ByteSize(16), Width(10), Height(8), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x8UnormSrgb, ByteSize(16), Width(10), Height(8), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x8Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x10Unorm, ByteSize(16), Width(10), Height(10), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC10x10UnormSrgb, ByteSize(16), Width(10), Height(10), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x10Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC12x10Unorm, ByteSize(16), Width(12), Height(10), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC12x10UnormSrgb, ByteSize(16), Width(12), Height(10), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC12x10Unorm);
    AddCompressedFormat(wgpu::TextureFormat::ASTC12x12Unorm, ByteSize(16), Width(12), Height(12), astcFormatUnsupportedReason, ComponentCount(4));
    AddCompressedFormat(wgpu::TextureFormat::ASTC12x12UnormSrgb, ByteSize(16), Width(12), Height(12), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC12x12Unorm);

    // multi-planar formats
    const UnsupportedReason multiPlanarFormatUnsupportedReason = device->HasFeature(Feature::DawnMultiPlanarFormats) ?  Format::supported : RequiresFeature{wgpu::FeatureName::DawnMultiPlanarFormats};
    AddMultiAspectFormat(wgpu::TextureFormat::R8BG8Biplanar420Unorm, Aspect::Plane0 | Aspect::Plane1,
        wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::RG8Unorm, Cap::None, multiPlanarFormatUnsupportedReason, ComponentCount(3));
    const UnsupportedReason multiPlanarFormatP010UnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatP010) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatP010};
    AddMultiAspectFormat(wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm, Aspect::Plane0 | Aspect::Plane1,
        wgpu::TextureFormat::R16Unorm, wgpu::TextureFormat::RG16Unorm, Cap::None, multiPlanarFormatP010UnsupportedReason, ComponentCount(3));

    // clang-format on

    // This checks that each format is set at least once, the second part of checking that all
    // formats are checked exactly once. If this assertion is failing and texture formats have
    // been added or removed recently, check that kKnownFormatCount has been updated.
    ASSERT(formatsSet.all());

    return table;
}

namespace {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // anonymous namespace

absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const UnsupportedReason& value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s) {
    std::visit(
        overloaded{
            [](const std::monostate&) { UNREACHABLE(); },
            [s](const RequiresFeature& requiresFeature) {
                s->Append(absl::StrFormat("requires feature %s", requiresFeature.feature));
            },
            [s](const CompatibilityMode&) { s->Append("not supported in compatibility mode"); }},
        value);
    return {true};
}

}  // namespace dawn::native
