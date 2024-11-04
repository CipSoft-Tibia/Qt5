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

#include "src/tint/lang/wgsl/ast/transform/clamp_frag_depth.h"

#include <utility>

#include "src/tint/lang/core/builtin_value.h"
#include "src/tint/lang/wgsl/ast/attribute.h"
#include "src/tint/lang/wgsl/ast/builtin_attribute.h"
#include "src/tint/lang/wgsl/ast/function.h"
#include "src/tint/lang/wgsl/ast/module.h"
#include "src/tint/lang/wgsl/ast/struct.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/function.h"
#include "src/tint/lang/wgsl/sem/statement.h"
#include "src/tint/lang/wgsl/sem/struct.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/macros/scoped_assignment.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::ClampFragDepth);

namespace tint::ast::transform {

/// PIMPL state for the transform
struct ClampFragDepth::State {
    /// The source program
    const Program* const src;
    /// The target program builder
    ProgramBuilder b{};
    /// The clone context
    program::CloneContext ctx = {&b, src, /* auto_clone_symbols */ true};
    /// The sem::Info of the program
    const sem::Info& sem = src->Sem();
    /// The symbols of the program
    const SymbolTable& sym = src->Symbols();

    /// Runs the transform
    /// @returns the new program or SkipTransform if the transform is not required
    Transform::ApplyResult Run() {
        // Abort on any use of push constants in the module.
        for (auto* global : src->AST().GlobalVariables()) {
            if (auto* var = global->As<Var>()) {
                auto* v = src->Sem().Get(var);
                if (TINT_UNLIKELY(v->AddressSpace() == core::AddressSpace::kPushConstant)) {
                    TINT_ICE()
                        << "ClampFragDepth doesn't know how to handle module that already use push "
                           "constants";
                    return resolver::Resolve(b);
                }
            }
        }

        if (!ShouldRun()) {
            return SkipTransform;
        }

        // At least one entry-point needs clamping. Add the following to the module:
        //
        //   enable chromium_experimental_push_constant;
        //
        //   struct FragDepthClampArgs {
        //       min : f32,
        //       max : f32,
        //   }
        //   var<push_constant> frag_depth_clamp_args : FragDepthClampArgs;
        //
        //   fn clamp_frag_depth(v : f32) -> f32 {
        //       return clamp(v, frag_depth_clamp_args.min, frag_depth_clamp_args.max);
        //   }
        b.Enable(core::Extension::kChromiumExperimentalPushConstant);

        b.Structure(b.Symbols().New("FragDepthClampArgs"),
                    tint::Vector{b.Member("min", b.ty.f32()), b.Member("max", b.ty.f32())});

        auto args_sym = b.Symbols().New("frag_depth_clamp_args");
        b.GlobalVar(args_sym, b.ty("FragDepthClampArgs"), core::AddressSpace::kPushConstant);

        auto base_fn_sym = b.Symbols().New("clamp_frag_depth");
        b.Func(base_fn_sym, tint::Vector{b.Param("v", b.ty.f32())}, b.ty.f32(),
               tint::Vector{b.Return(b.Call("clamp", "v", b.MemberAccessor(args_sym, "min"),
                                            b.MemberAccessor(args_sym, "max")))});

        // If true, the currently cloned function returns frag depth directly as a scalar
        bool returns_frag_depth_as_value = false;

        // If valid, the currently cloned function returns frag depth in a struct
        // The symbol is the name of the helper function to apply the depth clamping.
        Symbol returns_frag_depth_as_struct_helper;

        // Map of io struct to helper function to return the structure with the depth clamping
        // applied.
        Hashmap<const Struct*, Symbol, 4u> io_structs_clamp_helpers;

        // Register a callback that will be called for each visted AST function.
        // This call wraps the cloning of the function's statements, and will assign to
        // `returns_frag_depth_as_value` or `returns_frag_depth_as_struct_helper` if the function's
        // return value requires depth clamping.
        ctx.ReplaceAll([&](const Function* fn) {
            if (fn->PipelineStage() != PipelineStage::kFragment) {
                return ctx.CloneWithoutTransform(fn);
            }

            if (ReturnsFragDepthAsValue(fn)) {
                TINT_SCOPED_ASSIGNMENT(returns_frag_depth_as_value, true);
                return ctx.CloneWithoutTransform(fn);
            }

            if (ReturnsFragDepthInStruct(fn)) {
                // At most once per I/O struct, add the conversion function:
                //
                //   fn clamp_frag_depth_S(s : S) -> S {
                //       return S(s.first, s.second, clamp_frag_depth(s.frag_depth), s.last);
                //   }
                auto* struct_ty = sem.Get(fn)->ReturnType()->As<sem::Struct>()->Declaration();
                auto helper = io_structs_clamp_helpers.GetOrCreate(struct_ty, [&] {
                    auto return_ty = fn->return_type;
                    auto fn_sym =
                        b.Symbols().New("clamp_frag_depth_" + struct_ty->name->symbol.Name());

                    tint::Vector<const Expression*, 8u> initializer_args;
                    for (auto* member : struct_ty->members) {
                        const Expression* arg =
                            b.MemberAccessor("s", ctx.Clone(member->name->symbol));
                        if (ContainsFragDepth(member->attributes)) {
                            arg = b.Call(base_fn_sym, arg);
                        }
                        initializer_args.Push(arg);
                    }
                    tint::Vector params{b.Param("s", ctx.Clone(return_ty))};
                    tint::Vector body{
                        b.Return(b.Call(ctx.Clone(return_ty), std::move(initializer_args))),
                    };
                    b.Func(fn_sym, params, ctx.Clone(return_ty), body);
                    return fn_sym;
                });

                TINT_SCOPED_ASSIGNMENT(returns_frag_depth_as_struct_helper, helper);
                return ctx.CloneWithoutTransform(fn);
            }

            return ctx.CloneWithoutTransform(fn);
        });

        // Replace the return statements `return expr` with `return clamp_frag_depth(expr)`.
        ctx.ReplaceAll([&](const ReturnStatement* stmt) -> const ReturnStatement* {
            if (returns_frag_depth_as_value) {
                return b.Return(stmt->source, b.Call(base_fn_sym, ctx.Clone(stmt->value)));
            }
            if (returns_frag_depth_as_struct_helper.IsValid()) {
                return b.Return(stmt->source, b.Call(returns_frag_depth_as_struct_helper,
                                                     ctx.Clone(stmt->value)));
            }
            return nullptr;
        });

        ctx.Clone();
        return resolver::Resolve(b);
    }

  private:
    /// @returns true if the transform should run
    bool ShouldRun() {
        for (auto* fn : src->AST().Functions()) {
            if (fn->PipelineStage() == PipelineStage::kFragment &&
                (ReturnsFragDepthAsValue(fn) || ReturnsFragDepthInStruct(fn))) {
                return true;
            }
        }

        return false;
    }
    /// @param attrs the attributes to examine
    /// @returns true if @p attrs contains a `@builtin(frag_depth)` attribute
    bool ContainsFragDepth(VectorRef<const Attribute*> attrs) {
        for (auto* attribute : attrs) {
            if (auto* builtin_attr = attribute->As<BuiltinAttribute>()) {
                auto builtin = sem.Get(builtin_attr)->Value();
                if (builtin == core::BuiltinValue::kFragDepth) {
                    return true;
                }
            }
        }

        return false;
    }

    /// @param fn the function to examine
    /// @returns true if @p fn has a return type with a `@builtin(frag_depth)` attribute
    bool ReturnsFragDepthAsValue(const Function* fn) {
        return ContainsFragDepth(fn->return_type_attributes);
    }

    /// @param fn the function to examine
    /// @returns true if @p fn has a return structure with a `@builtin(frag_depth)` attribute on one
    /// of the members
    bool ReturnsFragDepthInStruct(const Function* fn) {
        if (auto* struct_ty = sem.Get(fn)->ReturnType()->As<sem::Struct>()) {
            for (auto* member : struct_ty->Members()) {
                if (ContainsFragDepth(member->Declaration()->attributes)) {
                    return true;
                }
            }
        }

        return false;
    }
};

ClampFragDepth::ClampFragDepth() = default;
ClampFragDepth::~ClampFragDepth() = default;

Transform::ApplyResult ClampFragDepth::Apply(const Program* src, const DataMap&, DataMap&) const {
    return State{src}.Run();
}

}  // namespace tint::ast::transform
