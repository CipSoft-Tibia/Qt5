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

#include "src/tint/writer/spirv/generator_impl.h"

#include <utility>
#include <vector>

#include "src/tint/transform/add_block_attribute.h"
#include "src/tint/transform/add_empty_entry_point.h"
#include "src/tint/transform/builtin_polyfill.h"
#include "src/tint/transform/canonicalize_entry_point_io.h"
#include "src/tint/transform/demote_to_helper.h"
#include "src/tint/transform/direct_variable_access.h"
#include "src/tint/transform/disable_uniformity_analysis.h"
#include "src/tint/transform/expand_compound_assignment.h"
#include "src/tint/transform/for_loop_to_loop.h"
#include "src/tint/transform/manager.h"
#include "src/tint/transform/merge_return.h"
#include "src/tint/transform/preserve_padding.h"
#include "src/tint/transform/promote_side_effects_to_decl.h"
#include "src/tint/transform/remove_phonies.h"
#include "src/tint/transform/remove_unreachable_statements.h"
#include "src/tint/transform/simplify_pointers.h"
#include "src/tint/transform/std140.h"
#include "src/tint/transform/unshadow.h"
#include "src/tint/transform/var_for_dynamic_index.h"
#include "src/tint/transform/vectorize_matrix_conversions.h"
#include "src/tint/transform/vectorize_scalar_matrix_initializers.h"
#include "src/tint/transform/while_to_loop.h"
#include "src/tint/transform/zero_init_workgroup_memory.h"
#include "src/tint/writer/generate_external_texture_bindings.h"

namespace tint::writer::spirv {

SanitizedResult Sanitize(const Program* in, const Options& options) {
    transform::Manager manager;
    transform::DataMap data;

    manager.Add<transform::DisableUniformityAnalysis>();

    // ExpandCompoundAssignment must come before BuiltinPolyfill
    manager.Add<transform::ExpandCompoundAssignment>();

    {  // Builtin polyfills
        transform::BuiltinPolyfill::Builtins polyfills;
        polyfills.acosh = transform::BuiltinPolyfill::Level::kRangeCheck;
        polyfills.atanh = transform::BuiltinPolyfill::Level::kRangeCheck;
        polyfills.bgra8unorm = true;
        polyfills.bitshift_modulo = true;
        polyfills.clamp_int = true;
        polyfills.count_leading_zeros = true;
        polyfills.count_trailing_zeros = true;
        polyfills.extract_bits = transform::BuiltinPolyfill::Level::kClampParameters;
        polyfills.first_leading_bit = true;
        polyfills.first_trailing_bit = true;
        polyfills.insert_bits = transform::BuiltinPolyfill::Level::kClampParameters;
        polyfills.int_div_mod = true;
        polyfills.saturate = true;
        polyfills.texture_sample_base_clamp_to_edge_2d_f32 = true;
        polyfills.quantize_to_vec_f16 = true;  // crbug.com/tint/1741
        polyfills.workgroup_uniform_load = true;
        data.Add<transform::BuiltinPolyfill::Config>(polyfills);
        manager.Add<transform::BuiltinPolyfill>();
    }

    if (options.generate_external_texture_bindings) {
        auto new_bindings_map = GenerateExternalTextureBindings(in);
        data.Add<transform::MultiplanarExternalTexture::NewBindingPoints>(new_bindings_map);
    }
    manager.Add<transform::MultiplanarExternalTexture>();

    manager.Add<transform::PreservePadding>();  // Must come before DirectVariableAccess

    manager.Add<transform::Unshadow>();  // Must come before DirectVariableAccess
    bool disable_workgroup_init_in_sanitizer =
        options.disable_workgroup_init || options.use_zero_initialize_workgroup_memory_extension;
    if (!disable_workgroup_init_in_sanitizer) {
        manager.Add<transform::ZeroInitWorkgroupMemory>();
    }

    {
        transform::DirectVariableAccess::Options opts;
        opts.transform_private = true;
        opts.transform_function = true;
        data.Add<transform::DirectVariableAccess::Config>(opts);
        manager.Add<transform::DirectVariableAccess>();
    }

    manager.Add<transform::RemoveUnreachableStatements>();
    manager.Add<transform::PromoteSideEffectsToDecl>();
    manager.Add<transform::SimplifyPointers>();  // Required for arrayLength()
    manager.Add<transform::RemovePhonies>();
    manager.Add<transform::VectorizeScalarMatrixInitializers>();
    manager.Add<transform::VectorizeMatrixConversions>();
    manager.Add<transform::WhileToLoop>();  // ZeroInitWorkgroupMemory
    manager.Add<transform::MergeReturn>();
    manager.Add<transform::CanonicalizeEntryPointIO>();
    manager.Add<transform::AddEmptyEntryPoint>();
    manager.Add<transform::AddBlockAttribute>();

    // DemoteToHelper must come after CanonicalizeEntryPointIO, PromoteSideEffectsToDecl, and
    // ExpandCompoundAssignment.
    // TODO(crbug.com/tint/1752): Use SPV_EXT_demote_to_helper_invocation if available.
    manager.Add<transform::DemoteToHelper>();

    // Std140 must come after PromoteSideEffectsToDecl.
    // Std140 must come before VarForDynamicIndex and ForLoopToLoop.
    manager.Add<transform::Std140>();

    // VarForDynamicIndex must come after Std140
    manager.Add<transform::VarForDynamicIndex>();

    // ForLoopToLoop must come after Std140, ZeroInitWorkgroupMemory
    manager.Add<transform::ForLoopToLoop>();

    data.Add<transform::CanonicalizeEntryPointIO::Config>(
        transform::CanonicalizeEntryPointIO::Config(
            transform::CanonicalizeEntryPointIO::ShaderStyle::kSpirv, 0xFFFFFFFF,
            options.emit_vertex_point_size));

    SanitizedResult result;
    result.program = std::move(manager.Run(in, data).program);
    return result;
}

GeneratorImpl::GeneratorImpl(const Program* program, bool zero_initialize_workgroup_memory)
    : builder_(program, zero_initialize_workgroup_memory) {}

bool GeneratorImpl::Generate() {
    if (builder_.Build()) {
        writer_.WriteHeader(builder_.id_bound());
        writer_.WriteBuilder(&builder_);
        return true;
    }
    return false;
}

const std::vector<uint32_t>& GeneratorImpl::result() const {
    return writer_.result();
}

std::vector<uint32_t>& GeneratorImpl::result() {
    return writer_.result();
}

std::string GeneratorImpl::error() const {
    return builder_.error();
}

}  // namespace tint::writer::spirv
