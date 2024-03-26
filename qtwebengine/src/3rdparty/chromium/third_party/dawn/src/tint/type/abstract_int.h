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

#ifndef SRC_TINT_TYPE_ABSTRACT_INT_H_
#define SRC_TINT_TYPE_ABSTRACT_INT_H_

#include <string>

#include "src/tint/type/abstract_numeric.h"

namespace tint::type {

/// An abstract-int type.
/// @see https://www.w3.org/TR/WGSL/#abstractint
class AbstractInt final : public Castable<AbstractInt, AbstractNumeric> {
  public:
    /// Constructor
    AbstractInt();

    /// Destructor
    ~AbstractInt() override;

    /// @param other the other node to compare against
    /// @returns true if the this type is equal to @p other
    bool Equals(const UniqueNode& other) const override;

    /// @param symbols the program's symbol table
    /// @returns the name for this type when printed in diagnostics.
    std::string FriendlyName(const SymbolTable& symbols) const override;

    /// @param ctx the clone context
    /// @returns a clone of this type
    AbstractInt* Clone(CloneContext& ctx) const override;
};

}  // namespace tint::type

#endif  // SRC_TINT_TYPE_ABSTRACT_INT_H_
