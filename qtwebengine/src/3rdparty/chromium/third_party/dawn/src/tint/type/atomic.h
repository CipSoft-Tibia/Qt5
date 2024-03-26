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

#ifndef SRC_TINT_TYPE_ATOMIC_H_
#define SRC_TINT_TYPE_ATOMIC_H_

#include <string>

#include "src/tint/type/type.h"

namespace tint::type {

/// A atomic type.
class Atomic final : public Castable<Atomic, Type> {
  public:
    /// Constructor
    /// @param subtype the atomic type
    explicit Atomic(const type::Type* subtype);

    /// Destructor
    ~Atomic() override;

    /// @param other the other node to compare against
    /// @returns true if the this type is equal to @p other
    bool Equals(const type::UniqueNode& other) const override;

    /// @returns the atomic type
    const type::Type* Type() const { return subtype_; }

    /// @param symbols the program's symbol table
    /// @returns the name for this type that closely resembles how it would be
    /// declared in WGSL.
    std::string FriendlyName(const SymbolTable& symbols) const override;

    /// @returns the size in bytes of the type.
    uint32_t Size() const override;

    /// @returns the alignment in bytes of the type.
    uint32_t Align() const override;

    /// @param ctx the clone context
    /// @returns a clone of this type
    Atomic* Clone(CloneContext& ctx) const override;

  private:
    type::Type const* const subtype_;
};

}  // namespace tint::type

#endif  // SRC_TINT_TYPE_ATOMIC_H_
