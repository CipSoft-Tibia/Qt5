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

#ifndef SRC_TINT_TYPE_SAMPLED_TEXTURE_H_
#define SRC_TINT_TYPE_SAMPLED_TEXTURE_H_

#include <string>

#include "src/tint/type/texture.h"
#include "src/tint/type/texture_dimension.h"

namespace tint::type {

/// A sampled texture type.
class SampledTexture final : public Castable<SampledTexture, Texture> {
  public:
    /// Constructor
    /// @param dim the dimensionality of the texture
    /// @param type the data type of the sampled texture
    SampledTexture(TextureDimension dim, const Type* type);

    /// Destructor
    ~SampledTexture() override;

    /// @param other the other node to compare against
    /// @returns true if the this type is equal to @p other
    bool Equals(const UniqueNode& other) const override;

    /// @returns the subtype of the sampled texture
    Type* type() const { return const_cast<Type*>(type_); }

    /// @param symbols the program's symbol table
    /// @returns the name for this type that closely resembles how it would be
    /// declared in WGSL.
    std::string FriendlyName(const SymbolTable& symbols) const override;

    /// @param ctx the clone context
    /// @returns a clone of this type
    SampledTexture* Clone(CloneContext& ctx) const override;

  private:
    const Type* const type_;
};

}  // namespace tint::type

#endif  // SRC_TINT_TYPE_SAMPLED_TEXTURE_H_
