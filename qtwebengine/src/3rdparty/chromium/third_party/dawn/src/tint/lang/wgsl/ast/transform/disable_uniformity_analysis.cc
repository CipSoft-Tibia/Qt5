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

#include "src/tint/lang/wgsl/ast/transform/disable_uniformity_analysis.h"

#include <utility>

#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/module.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::DisableUniformityAnalysis);

namespace tint::ast::transform {

DisableUniformityAnalysis::DisableUniformityAnalysis() = default;

DisableUniformityAnalysis::~DisableUniformityAnalysis() = default;

Transform::ApplyResult DisableUniformityAnalysis::Apply(const Program* src,
                                                        const DataMap&,
                                                        DataMap&) const {
    if (src->Sem().Module()->Extensions().Contains(
            core::Extension::kChromiumDisableUniformityAnalysis)) {
        return SkipTransform;
    }

    ProgramBuilder b;
    program::CloneContext ctx{&b, src, /* auto_clone_symbols */ true};
    b.Enable(core::Extension::kChromiumDisableUniformityAnalysis);

    ctx.Clone();
    return resolver::Resolve(b);
}

}  // namespace tint::ast::transform
