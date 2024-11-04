// Copyright 2022 The Tint Authors.
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

#include "src/tint/lang/core/type/depth_texture.h"

#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/texture_dimension.h"
#include "src/tint/utils/diagnostic/diagnostic.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/math/hash.h"
#include "src/tint/utils/text/string_stream.h"

TINT_INSTANTIATE_TYPEINFO(tint::core::type::DepthTexture);

namespace tint::core::type {
namespace {

bool IsValidDepthDimension(TextureDimension dim) {
    return dim == TextureDimension::k2d || dim == TextureDimension::k2dArray ||
           dim == TextureDimension::kCube || dim == TextureDimension::kCubeArray;
}

}  // namespace

DepthTexture::DepthTexture(TextureDimension dim)
    : Base(Hash(TypeInfo::Of<DepthTexture>().full_hashcode, dim), dim) {
    TINT_ASSERT(IsValidDepthDimension(dim));
}

DepthTexture::~DepthTexture() = default;

bool DepthTexture::Equals(const UniqueNode& other) const {
    if (auto* o = other.As<DepthTexture>()) {
        return o->dim() == dim();
    }
    return false;
}

std::string DepthTexture::FriendlyName() const {
    StringStream out;
    out << "texture_depth_" << dim();
    return out.str();
}

DepthTexture* DepthTexture::Clone(CloneContext& ctx) const {
    return ctx.dst.mgr->Get<DepthTexture>(dim());
}

}  // namespace tint::core::type
