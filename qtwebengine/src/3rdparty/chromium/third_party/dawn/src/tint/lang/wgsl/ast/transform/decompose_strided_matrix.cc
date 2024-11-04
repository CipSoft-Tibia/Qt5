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

#include "src/tint/lang/wgsl/ast/transform/decompose_strided_matrix.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/wgsl/ast/transform/simplify_pointers.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/member_accessor_expression.h"
#include "src/tint/lang/wgsl/sem/value_expression.h"
#include "src/tint/utils/containers/map.h"
#include "src/tint/utils/math/hash.h"

using namespace tint::core::fluent_types;  // NOLINT

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::DecomposeStridedMatrix);

namespace tint::ast::transform {
namespace {

/// MatrixInfo describes a matrix member with a custom stride
struct MatrixInfo {
    /// The stride in bytes between columns of the matrix
    uint32_t stride = 0;
    /// The type of the matrix
    const core::type::Matrix* matrix = nullptr;

    /// @returns the identifier of an array that holds an vector column for each row of the matrix.
    Type array(ast::Builder* b) const {
        return b->ty.array(b->ty.vec<f32>(matrix->rows()), u32(matrix->columns()),
                           tint::Vector{
                               b->Stride(stride),
                           });
    }

    /// Equality operator
    bool operator==(const MatrixInfo& info) const {
        return stride == info.stride && matrix == info.matrix;
    }
    /// Hash function
    struct Hasher {
        size_t operator()(const MatrixInfo& t) const { return Hash(t.stride, t.matrix); }
    };
};

}  // namespace

DecomposeStridedMatrix::DecomposeStridedMatrix() = default;

DecomposeStridedMatrix::~DecomposeStridedMatrix() = default;

Transform::ApplyResult DecomposeStridedMatrix::Apply(const Program* src,
                                                     const DataMap&,
                                                     DataMap&) const {
    ProgramBuilder b;
    program::CloneContext ctx{&b, src, /* auto_clone_symbols */ true};

    // Scan the program for all storage and uniform structure matrix members with
    // a custom stride attribute. Replace these matrices with an equivalent array,
    // and populate the `decomposed` map with the members that have been replaced.
    Hashmap<const core::type::StructMember*, MatrixInfo, 8> decomposed;
    for (auto* node : src->ASTNodes().Objects()) {
        if (auto* str = node->As<Struct>()) {
            auto* str_ty = src->Sem().Get(str);
            if (!str_ty->UsedAs(core::AddressSpace::kUniform) &&
                !str_ty->UsedAs(core::AddressSpace::kStorage)) {
                continue;
            }
            for (auto* member : str_ty->Members()) {
                auto* matrix = member->Type()->As<core::type::Matrix>();
                if (!matrix) {
                    continue;
                }
                auto* attr = GetAttribute<StrideAttribute>(member->Declaration()->attributes);
                if (!attr) {
                    continue;
                }
                uint32_t stride = attr->stride;
                if (matrix->ColumnStride() == stride) {
                    continue;
                }
                // We've got ourselves a struct member of a matrix type with a custom
                // stride. Replace this with an array of column vectors.
                MatrixInfo info{stride, matrix};
                auto* replacement =
                    b.Member(member->Offset(), ctx.Clone(member->Name()), info.array(ctx.dst));
                ctx.Replace(member->Declaration(), replacement);
                decomposed.Add(member, info);
            }
        }
    }

    if (decomposed.IsEmpty()) {
        return SkipTransform;
    }

    // For all expressions where a single matrix column vector was indexed, we can
    // preserve these without calling conversion functions.
    // Example:
    //   ssbo.mat[2] -> ssbo.mat[2]
    ctx.ReplaceAll([&](const IndexAccessorExpression* expr) -> const IndexAccessorExpression* {
        if (auto* access = src->Sem().Get<sem::StructMemberAccess>(expr->object)) {
            if (decomposed.Contains(access->Member())) {
                auto* obj = ctx.CloneWithoutTransform(expr->object);
                auto* idx = ctx.Clone(expr->index);
                return b.IndexAccessor(obj, idx);
            }
        }
        return nullptr;
    });

    // For all struct member accesses to the matrix on the LHS of an assignment,
    // we need to convert the matrix to the array before assigning to the
    // structure.
    // Example:
    //   ssbo.mat = mat_to_arr(m)
    std::unordered_map<MatrixInfo, Symbol, MatrixInfo::Hasher> mat_to_arr;
    ctx.ReplaceAll([&](const AssignmentStatement* stmt) -> const Statement* {
        if (auto* access = src->Sem().Get<sem::StructMemberAccess>(stmt->lhs)) {
            if (auto info = decomposed.Find(access->Member())) {
                auto fn = tint::GetOrCreate(mat_to_arr, *info, [&] {
                    auto name =
                        b.Symbols().New("mat" + std::to_string(info->matrix->columns()) + "x" +
                                        std::to_string(info->matrix->rows()) + "_stride_" +
                                        std::to_string(info->stride) + "_to_arr");

                    auto matrix = [&] { return CreateASTTypeFor(ctx, info->matrix); };
                    auto array = [&] { return info->array(ctx.dst); };

                    auto mat = b.Sym("m");
                    tint::Vector<const Expression*, 4> columns;
                    for (uint32_t i = 0; i < static_cast<uint32_t>(info->matrix->columns()); i++) {
                        columns.Push(b.IndexAccessor(mat, u32(i)));
                    }
                    b.Func(name,
                           tint::Vector{
                               b.Param(mat, matrix()),
                           },
                           array(),
                           tint::Vector{
                               b.Return(b.Call(array(), columns)),
                           });
                    return name;
                });
                auto* lhs = ctx.CloneWithoutTransform(stmt->lhs);
                auto* rhs = b.Call(fn, ctx.Clone(stmt->rhs));
                return b.Assign(lhs, rhs);
            }
        }
        return nullptr;
    });

    // For all other struct member accesses, we need to convert the array to the
    // matrix type. Example:
    //   m = arr_to_mat(ssbo.mat)
    std::unordered_map<MatrixInfo, Symbol, MatrixInfo::Hasher> arr_to_mat;
    ctx.ReplaceAll([&](const MemberAccessorExpression* expr) -> const Expression* {
        if (auto* access = src->Sem().Get(expr)->UnwrapLoad()->As<sem::StructMemberAccess>()) {
            if (auto info = decomposed.Find(access->Member())) {
                auto fn = tint::GetOrCreate(arr_to_mat, *info, [&] {
                    auto name =
                        b.Symbols().New("arr_to_mat" + std::to_string(info->matrix->columns()) +
                                        "x" + std::to_string(info->matrix->rows()) + "_stride_" +
                                        std::to_string(info->stride));

                    auto matrix = [&] { return CreateASTTypeFor(ctx, info->matrix); };
                    auto array = [&] { return info->array(ctx.dst); };

                    auto arr = b.Sym("arr");
                    tint::Vector<const Expression*, 4> columns;
                    for (uint32_t i = 0; i < static_cast<uint32_t>(info->matrix->columns()); i++) {
                        columns.Push(b.IndexAccessor(arr, u32(i)));
                    }
                    b.Func(name,
                           tint::Vector{
                               b.Param(arr, array()),
                           },
                           matrix(),
                           tint::Vector{
                               b.Return(b.Call(matrix(), columns)),
                           });
                    return name;
                });
                return b.Call(fn, ctx.CloneWithoutTransform(expr));
            }
        }
        return nullptr;
    });

    ctx.Clone();
    return resolver::Resolve(b);
}

}  // namespace tint::ast::transform
