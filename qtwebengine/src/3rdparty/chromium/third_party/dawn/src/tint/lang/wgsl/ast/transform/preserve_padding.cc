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

#include "src/tint/lang/wgsl/ast/transform/preserve_padding.h"

#include <unordered_set>
#include <utility>

#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/struct.h"
#include "src/tint/utils/containers/map.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/rtti/switch.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::PreservePadding);

namespace tint::ast::transform {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

PreservePadding::PreservePadding() = default;

PreservePadding::~PreservePadding() = default;

/// The PIMPL state for the PreservePadding transform
struct PreservePadding::State {
    /// Constructor
    /// @param src the source Program
    explicit State(const Program* src) : ctx{&b, src, /* auto_clone_symbols */ true} {}

    /// The main function for the transform.
    /// @returns the ApplyResult
    ApplyResult Run() {
        // Gather a list of assignments that need to be transformed.
        std::unordered_set<const AssignmentStatement*> assignments_to_transform;
        for (auto* node : ctx.src->ASTNodes().Objects()) {
            Switch(
                node,  //
                [&](const AssignmentStatement* assign) {
                    auto* ty = sem.GetVal(assign->lhs)->Type();
                    if (assign->lhs->Is<PhonyExpression>()) {
                        // Ignore phony assignment.
                        return;
                    }
                    if (ty->As<core::type::Reference>()->AddressSpace() !=
                        core::AddressSpace::kStorage) {
                        // We only care about assignments that write to variables in the storage
                        // address space, as nothing else is host-visible.
                        return;
                    }
                    if (HasPadding(ty->UnwrapRef())) {
                        // The assigned type has padding bytes, so we need to decompose the writes.
                        assignments_to_transform.insert(assign);
                    }
                },
                [&](const Enable* enable) {
                    // Check if the full pointer parameters extension is already enabled.
                    if (enable->HasExtension(
                            core::Extension::kChromiumExperimentalFullPtrParameters)) {
                        ext_enabled = true;
                    }
                });
        }
        if (assignments_to_transform.empty()) {
            return SkipTransform;
        }

        // Replace all assignments that include padding with decomposed versions.
        ctx.ReplaceAll([&](const AssignmentStatement* assign) -> const Statement* {
            if (!assignments_to_transform.count(assign)) {
                return nullptr;
            }
            auto* ty = sem.GetVal(assign->lhs)->Type()->UnwrapRef();
            return MakeAssignment(ty, ctx.Clone(assign->lhs), ctx.Clone(assign->rhs));
        });

        ctx.Clone();
        return resolver::Resolve(b);
    }

    /// Create a statement that will perform the assignment `lhs = rhs`, creating and using helper
    /// functions to decompose the assignment into element-wise copies if needed.
    /// @param ty the type of the assignment
    /// @param lhs the lhs expression (in the destination program)
    /// @param rhs the rhs expression (in the destination program)
    /// @returns the statement that performs the assignment
    const Statement* MakeAssignment(const core::type::Type* ty,
                                    const Expression* lhs,
                                    const Expression* rhs) {
        if (!HasPadding(ty)) {
            // No padding - use a regular assignment.
            return b.Assign(lhs, rhs);
        }

        // Call (and create if necessary) a helper function that assigns a composite using the
        // statements in `body`. The helper will have the form:
        //   fn assign_helper_T(dest : ptr<storage, T, read_write>, value : T) {
        //     <body>
        //   }
        // It will be called by passing a pointer to the original LHS:
        //   assign_helper_T(&lhs, rhs);
        //
        // Since this requires passing pointers to the storage address space, this will also enable
        // the chromium_experimental_full_ptr_parameters extension.
        const char* kDestParamName = "dest";
        const char* kValueParamName = "value";
        auto call_helper = [&](auto&& body) {
            EnableExtension();
            auto helper = helpers.GetOrCreate(ty, [&] {
                auto helper_name = b.Symbols().New("assign_and_preserve_padding");
                tint::Vector<const Parameter*, 2> params = {
                    b.Param(kDestParamName,
                            b.ty.ptr<storage, read_write>(CreateASTTypeFor(ctx, ty))),
                    b.Param(kValueParamName, CreateASTTypeFor(ctx, ty)),
                };
                b.Func(helper_name, params, b.ty.void_(), body());
                return helper_name;
            });
            return b.CallStmt(b.Call(helper, b.AddressOf(lhs), rhs));
        };

        return Switch(
            ty,  //
            [&](const core::type::Array* arr) {
                // Call a helper function that uses a loop to assigns each element separately.
                return call_helper([&] {
                    tint::Vector<const Statement*, 8> body;
                    auto* idx = b.Var("i", b.Expr(0_u));
                    body.Push(
                        b.For(b.Decl(idx), b.LessThan(idx, u32(arr->ConstantCount().value())),
                              b.Assign(idx, b.Add(idx, 1_u)),
                              b.Block(MakeAssignment(arr->ElemType(),
                                                     b.IndexAccessor(b.Deref(kDestParamName), idx),
                                                     b.IndexAccessor(kValueParamName, idx)))));
                    return body;
                });
            },
            [&](const core::type::Matrix* mat) {
                // Call a helper function that assigns each column separately.
                return call_helper([&] {
                    tint::Vector<const Statement*, 4> body;
                    for (uint32_t i = 0; i < mat->columns(); i++) {
                        body.Push(MakeAssignment(mat->ColumnType(),
                                                 b.IndexAccessor(b.Deref(kDestParamName), u32(i)),
                                                 b.IndexAccessor(kValueParamName, u32(i))));
                    }
                    return body;
                });
            },
            [&](const core::type::Struct* str) {
                // Call a helper function that assigns each member separately.
                return call_helper([&] {
                    tint::Vector<const Statement*, 8> body;
                    for (auto member : str->Members()) {
                        auto name = member->Name().Name();
                        body.Push(MakeAssignment(member->Type(),
                                                 b.MemberAccessor(b.Deref(kDestParamName), name),
                                                 b.MemberAccessor(kValueParamName, name)));
                    }
                    return body;
                });
            },
            [&](Default) {
                TINT_ICE() << "unhandled type with padding";
                return nullptr;
            });
    }

    /// Checks if a type contains padding bytes.
    /// @param ty the type to check
    /// @returns true if `ty` (or any of its contained types) have padding bytes
    bool HasPadding(const core::type::Type* ty) {
        return Switch(
            ty,  //
            [&](const core::type::Array* arr) {
                auto* elem_ty = arr->ElemType();
                if (elem_ty->Size() % elem_ty->Align() > 0) {
                    return true;
                }
                return HasPadding(elem_ty);
            },
            [&](const core::type::Matrix* mat) {
                auto* col_ty = mat->ColumnType();
                if (mat->ColumnStride() > col_ty->Size()) {
                    return true;
                }
                return HasPadding(col_ty);
            },
            [&](const core::type::Struct* str) {
                uint32_t current_offset = 0;
                for (auto* member : str->Members()) {
                    if (member->Offset() > current_offset) {
                        return true;
                    }
                    if (HasPadding(member->Type())) {
                        return true;
                    }
                    current_offset += member->Type()->Size();
                }
                return (current_offset < str->Size());
            },
            [&](Default) { return false; });
    }

    /// Enable the full pointer parameters extension, if we have not already done so.
    void EnableExtension() {
        if (!ext_enabled) {
            b.Enable(core::Extension::kChromiumExperimentalFullPtrParameters);
            ext_enabled = true;
        }
    }

  private:
    /// The program builder
    ProgramBuilder b;
    /// The clone context
    program::CloneContext ctx;
    /// Alias to the semantic info in ctx.src
    const sem::Info& sem = ctx.src->Sem();
    /// Alias to the symbols in ctx.src
    const SymbolTable& sym = ctx.src->Symbols();
    /// Flag to track whether we have already enabled the full pointer parameters extension.
    bool ext_enabled = false;
    /// Map of semantic types to their assignment helper functions.
    Hashmap<const core::type::Type*, Symbol, 8> helpers;
};

Transform::ApplyResult PreservePadding::Apply(const Program* program,
                                              const DataMap&,
                                              DataMap&) const {
    return State(program).Run();
}

}  // namespace tint::ast::transform
