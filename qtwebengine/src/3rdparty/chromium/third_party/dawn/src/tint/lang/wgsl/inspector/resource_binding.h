// Copyright 2021 The Tint Authors.
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

#ifndef SRC_TINT_LANG_WGSL_INSPECTOR_RESOURCE_BINDING_H_
#define SRC_TINT_LANG_WGSL_INSPECTOR_RESOURCE_BINDING_H_

#include <cstdint>

#include "src/tint/lang/core/texel_format.h"
#include "src/tint/lang/core/type/texture_dimension.h"
#include "src/tint/lang/core/type/type.h"

namespace tint::inspector {

/// Container for information about how a resource is bound
struct ResourceBinding {
    /// The dimensionality of a texture
    enum class TextureDimension : uint8_t {
        /// 1 dimensional texture
        k1d,
        /// 2 dimensional texture
        k2d,
        /// 2 dimensional array texture
        k2dArray,
        /// 3 dimensional texture
        k3d,
        /// cube texture
        kCube,
        /// cube array texture
        kCubeArray,
        /// Invalid texture
        kNone,
    };

    /// Component type of the texture's data. Same as the Sampled Type parameter
    /// in SPIR-V OpTypeImage.
    enum class SampledKind : uint8_t { kFloat, kUInt, kSInt, kUnknown };

    /// Enumerator of texel image formats
    enum class TexelFormat : uint8_t {
        kBgra8Unorm,
        kRgba8Unorm,
        kRgba8Snorm,
        kRgba8Uint,
        kRgba8Sint,
        kRgba16Uint,
        kRgba16Sint,
        kRgba16Float,
        kR32Uint,
        kR32Sint,
        kR32Float,
        kRg32Uint,
        kRg32Sint,
        kRg32Float,
        kRgba32Uint,
        kRgba32Sint,
        kRgba32Float,
        kNone,
    };

    /// kXXX maps to entries returned by GetXXXResourceBindings call.
    enum class ResourceType {
        kUniformBuffer,
        kStorageBuffer,
        kReadOnlyStorageBuffer,
        kSampler,
        kComparisonSampler,
        kSampledTexture,
        kMultisampledTexture,
        kWriteOnlyStorageTexture,
        kReadOnlyStorageTexture,
        kReadWriteStorageTexture,
        kDepthTexture,
        kDepthMultisampledTexture,
        kExternalTexture
    };

    /// Type of resource that is bound.
    ResourceType resource_type;
    /// Bind group the binding belongs
    uint32_t bind_group;
    /// Identifier to identify this binding within the bind group
    uint32_t binding;
    /// Size for this binding, in bytes, if defined.
    uint64_t size;
    /// Size for this binding without trailing structure padding, in bytes, if
    /// defined.
    uint64_t size_no_padding;
    /// Dimensionality of this binding, if defined.
    TextureDimension dim;
    /// Kind of data being sampled, if defined.
    SampledKind sampled_kind;
    /// Format of data, if defined.
    TexelFormat image_format;
};

/// Convert from internal core::type::TextureDimension to public
/// ResourceBinding::TextureDimension
/// @param type_dim internal value to convert from
/// @returns the publicly visible equivalent
ResourceBinding::TextureDimension TypeTextureDimensionToResourceBindingTextureDimension(
    const core::type::TextureDimension& type_dim);

/// Infer ResourceBinding::SampledKind for a given core::type::Type
/// @param base_type internal type to infer from
/// @returns the publicly visible equivalent
ResourceBinding::SampledKind BaseTypeToSampledKind(const core::type::Type* base_type);

/// Convert from internal core::TexelFormat to public
/// ResourceBinding::TexelFormat
/// @param image_format internal value to convert from
/// @returns the publicly visible equivalent
ResourceBinding::TexelFormat TypeTexelFormatToResourceBindingTexelFormat(
    const core::TexelFormat& image_format);

}  // namespace tint::inspector

#endif  // SRC_TINT_LANG_WGSL_INSPECTOR_RESOURCE_BINDING_H_
