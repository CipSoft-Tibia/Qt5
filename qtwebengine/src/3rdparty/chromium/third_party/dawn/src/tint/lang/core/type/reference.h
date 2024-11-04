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

#ifndef SRC_TINT_LANG_CORE_TYPE_REFERENCE_H_
#define SRC_TINT_LANG_CORE_TYPE_REFERENCE_H_

#include <string>

#include "src/tint/lang/core/access.h"
#include "src/tint/lang/core/address_space.h"
#include "src/tint/lang/core/type/type.h"

namespace tint::core::type {

/// A reference type.
class Reference final : public Castable<Reference, Type> {
  public:
    /// Constructor
    /// @param address_space the address space of the reference
    /// @param subtype the pointee type
    /// @param access the resolved access control of the reference
    Reference(core::AddressSpace address_space, const Type* subtype, core::Access access);

    /// Destructor
    ~Reference() override;

    /// @param other the other node to compare against
    /// @returns true if the this type is equal to @p other
    bool Equals(const UniqueNode& other) const override;

    /// @returns the pointee type
    const Type* StoreType() const { return subtype_; }

    /// @returns the address space of the reference
    core::AddressSpace AddressSpace() const { return address_space_; }

    /// @returns the resolved access control of the reference.
    core::Access Access() const { return access_; }

    /// @returns the name for this type that closely resembles how it would be
    /// declared in WGSL.
    std::string FriendlyName() const override;

    /// @param ctx the clone context
    /// @returns a clone of this type
    Reference* Clone(CloneContext& ctx) const override;

  private:
    Type const* const subtype_;
    core::AddressSpace const address_space_;
    core::Access const access_;
};

}  // namespace tint::core::type

#endif  // SRC_TINT_LANG_CORE_TYPE_REFERENCE_H_
