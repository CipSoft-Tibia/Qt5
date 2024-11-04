// Copyright 2023 The Tint Authors.
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

#ifndef SRC_TINT_LANG_CORE_TYPE_TEXTURE_DIMENSION_H_
#define SRC_TINT_LANG_CORE_TYPE_TEXTURE_DIMENSION_H_

#include "src/tint/utils/text/string_stream.h"
#include "src/tint/utils/traits/traits.h"

namespace tint::core::type {

/// The dimensionality of the texture
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

/// @param dim the enum value
/// @returns the string for the given enum value
std::string_view ToString(enum type::TextureDimension dim);

/// @param out the stream to write to
/// @param dim the type::TextureDimension
/// @return the stream so calls can be chained
template <typename STREAM, typename = traits::EnableIfIsOStream<STREAM>>
auto& operator<<(STREAM& out, core::type::TextureDimension dim) {
    return out << ToString(dim);
}

}  // namespace tint::core::type

#endif  // SRC_TINT_LANG_CORE_TYPE_TEXTURE_DIMENSION_H_
