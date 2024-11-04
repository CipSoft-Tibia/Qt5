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

#ifndef SRC_TINT_LANG_WGSL_AST_TRANSFORM_MSL_SUBGROUP_BALLOT_H_
#define SRC_TINT_LANG_WGSL_AST_TRANSFORM_MSL_SUBGROUP_BALLOT_H_

#include <string>

#include "src/tint/lang/wgsl/ast/internal_attribute.h"
#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace tint::ast::transform {

/// MslSubgroupBallot is a transform that replaces calls to `subgroupBallot()` with an
/// implementation that uses MSL's `simd_active_threads_mask()`.
///
/// @note Depends on the following transforms to have been run first:
/// * CanonicalizeEntryPointIO
class MslSubgroupBallot final : public Castable<MslSubgroupBallot, Transform> {
  public:
    /// Constructor
    MslSubgroupBallot();

    /// Destructor
    ~MslSubgroupBallot() override;

    /// @copydoc Transform::Apply
    ApplyResult Apply(const Program* program,
                      const DataMap& inputs,
                      DataMap& outputs) const override;

    /// SimdActiveThreadsMask is an InternalAttribute that is used to decorate a stub function so
    /// that the MSL backend transforms this into calls to the `simd_active_threads_mask` function.
    class SimdActiveThreadsMask final : public Castable<SimdActiveThreadsMask, InternalAttribute> {
      public:
        /// Constructor
        /// @param pid the identifier of the program that owns this node
        /// @param nid the unique node identifier
        SimdActiveThreadsMask(GenerationID pid, NodeID nid) : Base(pid, nid, Empty) {}

        /// Destructor
        ~SimdActiveThreadsMask() override;

        /// @copydoc InternalAttribute::InternalName
        std::string InternalName() const override { return "simd_active_threads_mask"; }

        /// Performs a deep clone of this object using the program::CloneContext `ctx`.
        /// @param ctx the clone context
        /// @return the newly cloned object
        const SimdActiveThreadsMask* Clone(CloneContext& ctx) const override;
    };

  private:
    struct State;
};

}  // namespace tint::ast::transform

#endif  // SRC_TINT_LANG_WGSL_AST_TRANSFORM_MSL_SUBGROUP_BALLOT_H_
