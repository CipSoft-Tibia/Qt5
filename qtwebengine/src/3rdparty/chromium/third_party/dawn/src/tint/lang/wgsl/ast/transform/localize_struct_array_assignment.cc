// Copyright 2021 The Tint Authors.
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

#include "src/tint/lang/wgsl/ast/transform/localize_struct_array_assignment.h"

#include <unordered_map>
#include <utility>

#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/wgsl/ast/assignment_statement.h"
#include "src/tint/lang/wgsl/ast/transform/simplify_pointers.h"
#include "src/tint/lang/wgsl/ast/traverse_expressions.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/member_accessor_expression.h"
#include "src/tint/lang/wgsl/sem/statement.h"
#include "src/tint/lang/wgsl/sem/value_expression.h"
#include "src/tint/lang/wgsl/sem/variable.h"
#include "src/tint/utils/macros/scoped_assignment.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::LocalizeStructArrayAssignment);

namespace tint::ast::transform {

/// PIMPL state for the transform
struct LocalizeStructArrayAssignment::State {
    /// Constructor
    /// @param program the source program
    explicit State(const Program* program) : src(program) {}

    /// Runs the transform
    /// @returns the new program or SkipTransform if the transform is not required
    ApplyResult Run() {
        struct Shared {
            bool process_nested_nodes = false;
            tint::Vector<const Statement*, 4> insert_before_stmts;
            tint::Vector<const Statement*, 4> insert_after_stmts;
        } s;

        bool made_changes = false;

        for (auto* node : ctx.src->ASTNodes().Objects()) {
            if (auto* assign_stmt = node->As<AssignmentStatement>()) {
                // Process if it's an assignment statement to a dynamically indexed array
                // within a struct on a function or private storage variable. This
                // specific use-case is what FXC fails to compile with:
                // error X3500: array reference cannot be used as an l-value; not natively
                // addressable
                if (!ContainsStructArrayIndex(assign_stmt->lhs)) {
                    continue;
                }
                auto og = GetOriginatingTypeAndAddressSpace(assign_stmt);
                if (!(og.first->Is<core::type::Struct>() &&
                      (og.second == core::AddressSpace::kFunction ||
                       og.second == core::AddressSpace::kPrivate))) {
                    continue;
                }

                ctx.Replace(assign_stmt, [&, assign_stmt] {
                    // Reset shared state for this assignment statement
                    s = Shared{};

                    const Expression* new_lhs = nullptr;
                    {
                        TINT_SCOPED_ASSIGNMENT(s.process_nested_nodes, true);
                        new_lhs = ctx.Clone(assign_stmt->lhs);
                    }

                    auto* new_assign_stmt = b.Assign(new_lhs, ctx.Clone(assign_stmt->rhs));

                    // Combine insert_before_stmts + new_assign_stmt + insert_after_stmts into
                    // a block and return it
                    auto stmts = std::move(s.insert_before_stmts);
                    stmts.Reserve(1 + s.insert_after_stmts.Length());
                    stmts.Push(new_assign_stmt);
                    for (auto* stmt : s.insert_after_stmts) {
                        stmts.Push(stmt);
                    }

                    return b.Block(std::move(stmts));
                });

                made_changes = true;
            }
        }

        if (!made_changes) {
            return SkipTransform;
        }

        ctx.ReplaceAll([&](const IndexAccessorExpression* index_access) -> const Expression* {
            if (!s.process_nested_nodes) {
                return nullptr;
            }

            // Indexing a member access expr?
            auto* mem_access = index_access->object->As<MemberAccessorExpression>();
            if (!mem_access) {
                return nullptr;
            }

            // Process any nested IndexAccessorExpressions
            mem_access = ctx.Clone(mem_access);

            // Store the address of the member access into a let as we need to read
            // the value twice e.g. let tint_symbol = &(s.a1);
            auto mem_access_ptr = b.Sym();
            s.insert_before_stmts.Push(b.Decl(b.Let(mem_access_ptr, b.AddressOf(mem_access))));

            // Disable further transforms when cloning
            TINT_SCOPED_ASSIGNMENT(s.process_nested_nodes, false);

            // Copy entire array out of struct into local temp var
            // e.g. var tint_symbol_1 = *(tint_symbol);
            auto tmp_var = b.Sym();
            s.insert_before_stmts.Push(b.Decl(b.Var(tmp_var, b.Deref(mem_access_ptr))));

            // Replace input index_access with a clone of itself, but with its
            // .object replaced by the new temp var. This is returned from this
            // function to modify the original assignment statement. e.g.
            // tint_symbol_1[uniforms.i]
            auto* new_index_access = b.IndexAccessor(tmp_var, ctx.Clone(index_access->index));

            // Assign temp var back to array
            // e.g. *(tint_symbol) = tint_symbol_1;
            auto* assign_rhs_to_temp = b.Assign(b.Deref(mem_access_ptr), tmp_var);
            {
                tint::Vector<const Statement*, 8> stmts{assign_rhs_to_temp};
                for (auto* stmt : s.insert_after_stmts) {
                    stmts.Push(stmt);
                }
                s.insert_after_stmts = std::move(stmts);
            }

            return new_index_access;
        });

        ctx.Clone();
        return resolver::Resolve(b);
    }

  private:
    /// The source program
    const Program* const src;
    /// The target program builder
    ProgramBuilder b;
    /// The clone context
    program::CloneContext ctx = {&b, src, /* auto_clone_symbols */ true};

    /// Returns true if `expr` contains an index accessor expression to a
    /// structure member of array type.
    bool ContainsStructArrayIndex(const Expression* expr) {
        bool result = false;
        TraverseExpressions(expr, [&](const IndexAccessorExpression* ia) {
            // Indexing using a runtime value?
            auto* idx_sem = src->Sem().GetVal(ia->index);
            if (!idx_sem->ConstantValue()) {
                // Indexing a member access expr?
                if (auto* ma = ia->object->As<MemberAccessorExpression>()) {
                    // That accesses an array?
                    if (src->TypeOf(ma)->UnwrapRef()->Is<core::type::Array>()) {
                        result = true;
                        return TraverseAction::Stop;
                    }
                }
            }
            return TraverseAction::Descend;
        });

        return result;
    }

    // Returns the type and address space of the originating variable of the lhs
    // of the assignment statement.
    // See https://www.w3.org/TR/WGSL/#originating-variable-section
    std::pair<const core::type::Type*, core::AddressSpace> GetOriginatingTypeAndAddressSpace(
        const AssignmentStatement* assign_stmt) {
        auto* root_ident = src->Sem().GetVal(assign_stmt->lhs)->RootIdentifier();
        if (TINT_UNLIKELY(!root_ident)) {
            TINT_ICE() << "Unable to determine originating variable for lhs of assignment "
                          "statement";
            return {};
        }

        return Switch(
            root_ident->Type(),  //
            [&](const core::type::Reference* ref) {
                return std::make_pair(ref->StoreType(), ref->AddressSpace());
            },
            [&](const core::type::Pointer* ptr) {
                return std::make_pair(ptr->StoreType(), ptr->AddressSpace());
            },
            [&](Default) {
                TINT_ICE() << "Expecting to find variable of type pointer or reference on lhs "
                              "of assignment statement";
                return std::pair<const core::type::Type*, core::AddressSpace>{};
            });
    }
};

LocalizeStructArrayAssignment::LocalizeStructArrayAssignment() = default;

LocalizeStructArrayAssignment::~LocalizeStructArrayAssignment() = default;

Transform::ApplyResult LocalizeStructArrayAssignment::Apply(const Program* src,
                                                            const DataMap&,
                                                            DataMap&) const {
    return State{src}.Run();
}

}  // namespace tint::ast::transform
