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

#include "src/tint/lang/wgsl/ast/transform/msl_subgroup_ballot.h"

#include <utility>

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/call.h"
#include "src/tint/lang/wgsl/sem/function.h"
#include "src/tint/lang/wgsl/sem/statement.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::MslSubgroupBallot);
TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::MslSubgroupBallot::SimdActiveThreadsMask);

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::ast::transform {

/// PIMPL state for the transform
struct MslSubgroupBallot::State {
    /// The source program
    const Program* const src;
    /// The target program builder
    ProgramBuilder b;
    /// The clone context
    program::CloneContext ctx = {&b, src, /* auto_clone_symbols */ true};

    /// The name of the `tint_subgroup_ballot` helper function.
    Symbol ballot_helper{};

    /// The name of the `tint_subgroup_size_mask` global variable.
    Symbol subgroup_size_mask{};

    /// The set of a functions that directly call `subgroupBallot()`.
    Hashset<const sem::Function*, 4> ballot_callers;

    /// Constructor
    /// @param program the source program
    explicit State(const Program* program) : src(program) {}

    /// Runs the transform
    /// @returns the new program or SkipTransform if the transform is not required
    ApplyResult Run() {
        auto& sem = src->Sem();

        bool made_changes = false;
        for (auto* node : ctx.src->ASTNodes().Objects()) {
            auto* call = sem.Get<sem::Call>(node);
            if (call) {
                // If this is a call to a `subgroupBallot()` builtin, replace it with a call to the
                // helper function and make a note of the function that we are in.
                auto* builtin = call->Target()->As<sem::Builtin>();
                if (builtin && builtin->Type() == core::Function::kSubgroupBallot) {
                    ctx.Replace(call->Declaration(), b.Call(GetHelper()));
                    ballot_callers.Add(call->Stmt()->Function());
                    made_changes = true;
                }
            }
        }
        if (!made_changes) {
            return SkipTransform;
        }

        // Set the subgroup size mask at the start of each entry point that transitively calls
        // `subgroupBallot()`.
        for (auto* global : src->AST().GlobalDeclarations()) {
            auto* func = global->As<Function>();
            if (func && func->IsEntryPoint() && TransitvelyCallsSubgroupBallot(sem.Get(func))) {
                SetSubgroupSizeMask(func);
            }
        }

        ctx.Clone();
        return resolver::Resolve(b);
    }

    /// Get (or create) the `tint_msl_subgroup` helper function.
    /// @returns the name of the helper function
    Symbol GetHelper() {
        if (!ballot_helper) {
            auto intrinsic = b.Symbols().New("tint_msl_simd_active_threads_mask");
            subgroup_size_mask = b.Symbols().New("tint_subgroup_size_mask");
            ballot_helper = b.Symbols().New("tint_msl_subgroup_ballot");

            // Declare the `tint_msl_subgroup_ballot` intrinsic function, which will use the
            // `simd_active_threads_mask` function to return 64-bit vote.
            b.Func(intrinsic, Empty, b.ty.vec2<u32>(), nullptr,
                   Vector{b.ASTNodes().Create<SimdActiveThreadsMask>(b.ID(), b.AllocateNodeID()),
                          b.Disable(DisabledValidation::kFunctionHasNoBody)});

            // Declare the `tint_subgroup_size_mask` variable.
            b.GlobalVar(subgroup_size_mask, core::AddressSpace::kPrivate, b.ty.vec4<u32>());

            // Declare the `tint_msl_subgroup_ballot` helper function as follows:
            //   fn tint_msl_subgroup_ballot() -> vec4u {
            //     let vote : vec2u = vec4f(tint_simd_active_threads_mask(), 0, 0);
            //     return (vote & tint_subgroup_size_mask);
            //   }
            auto* vote = b.Let(b.Sym(), b.Call(b.ty.vec4<u32>(), b.Call(intrinsic), 0_u, 0_u));
            b.Func(ballot_helper, Empty, b.ty.vec4<u32>(),
                   Vector{
                       b.Decl(vote),
                       b.Return(b.And(vote, subgroup_size_mask)),
                   });
        }
        return ballot_helper;
    }

    /// Check if a function directly or transitively calls the `subgroupBallot()` builtin.
    /// @param func the function to check
    /// @returns true if the function transitively calls `subgroupBallot()`
    bool TransitvelyCallsSubgroupBallot(const sem::Function* func) {
        if (ballot_callers.Contains(func)) {
            return true;
        }
        for (auto* called : func->TransitivelyCalledFunctions()) {
            if (ballot_callers.Contains(called)) {
                return true;
            }
        }
        return false;
    }

    /// Add code to set the `subgroup_size_mask` variable at the start of an entry point.
    /// @param ep the entry point
    void SetSubgroupSizeMask(const ast::Function* ep) {
        // Check the entry point parameters for an existing `subgroup_size` builtin.
        Symbol subgroup_size;
        for (auto* param : ep->params) {
            auto* builtin = ast::GetAttribute<ast::BuiltinAttribute>(param->attributes);
            if (builtin &&
                src->Sem()
                        .Get<sem::BuiltinEnumExpression<core::BuiltinValue>>(builtin->builtin)
                        ->Value() == core::BuiltinValue::kSubgroupSize) {
                subgroup_size = ctx.Clone(param->name->symbol);
            }
        }
        if (!subgroup_size.IsValid()) {
            // No `subgroup_size` builtin parameter was found, so add one.
            subgroup_size = b.Symbols().New("tint_subgroup_size");
            ctx.InsertBack(ep->params, b.Param(subgroup_size, b.ty.u32(),
                                               Vector{
                                                   b.Builtin(core::BuiltinValue::kSubgroupSize),
                                               }));
        }

        // Add the following to the top of the entry point:
        // {
        //   let gt = subgroup_size > 32;
        //   subgroup_size_mask[0] = select(0xffffffff >> (32 - subgroup_size), 0xffffffff, gt);
        //   subgroup_size_mask[1] = select(0, 0xffffffff >> (64 - subgroup_size), gt);
        // }
        auto* gt = b.Let(b.Sym("gt"), b.GreaterThan(subgroup_size, 32_u));
        auto* lo =
            b.Call("select", b.Shr(0xffffffff_u, b.Sub(32_u, subgroup_size)), 0xffffffff_u, gt);
        auto* hi = b.Call("select", 0_u, b.Shr(0xffffffff_u, b.Sub(64_u, subgroup_size)), gt);
        auto* block = b.Block(Vector{
            b.Decl(gt),
            b.Assign(b.IndexAccessor(subgroup_size_mask, 0_u), lo),
            b.Assign(b.IndexAccessor(subgroup_size_mask, 1_u), hi),
        });
        ctx.InsertFront(ep->body->statements, block);
    }
};

MslSubgroupBallot::MslSubgroupBallot() = default;

MslSubgroupBallot::~MslSubgroupBallot() = default;

Transform::ApplyResult MslSubgroupBallot::Apply(const Program* src,
                                                const DataMap&,
                                                DataMap&) const {
    return State(src).Run();
}

MslSubgroupBallot::SimdActiveThreadsMask::~SimdActiveThreadsMask() = default;

const MslSubgroupBallot::SimdActiveThreadsMask* MslSubgroupBallot::SimdActiveThreadsMask::Clone(
    ast::CloneContext& ctx) const {
    return ctx.dst->ASTNodes().Create<MslSubgroupBallot::SimdActiveThreadsMask>(
        ctx.dst->ID(), ctx.dst->AllocateNodeID());
}

}  // namespace tint::ast::transform
