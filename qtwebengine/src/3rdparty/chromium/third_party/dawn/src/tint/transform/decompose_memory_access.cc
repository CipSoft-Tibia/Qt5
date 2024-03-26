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

#include "src/tint/transform/decompose_memory_access.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "src/tint/ast/assignment_statement.h"
#include "src/tint/ast/call_statement.h"
#include "src/tint/ast/disable_validation_attribute.h"
#include "src/tint/ast/unary_op.h"
#include "src/tint/program_builder.h"
#include "src/tint/sem/call.h"
#include "src/tint/sem/member_accessor_expression.h"
#include "src/tint/sem/statement.h"
#include "src/tint/sem/struct.h"
#include "src/tint/sem/variable.h"
#include "src/tint/type/array.h"
#include "src/tint/type/atomic.h"
#include "src/tint/type/reference.h"
#include "src/tint/utils/block_allocator.h"
#include "src/tint/utils/hash.h"
#include "src/tint/utils/map.h"

using namespace tint::number_suffixes;  // NOLINT

TINT_INSTANTIATE_TYPEINFO(tint::transform::DecomposeMemoryAccess);
TINT_INSTANTIATE_TYPEINFO(tint::transform::DecomposeMemoryAccess::Intrinsic);

namespace tint::transform {

namespace {

bool ShouldRun(const Program* program) {
    for (auto* decl : program->AST().GlobalDeclarations()) {
        if (auto* var = program->Sem().Get<sem::Variable>(decl)) {
            if (var->AddressSpace() == builtin::AddressSpace::kStorage ||
                var->AddressSpace() == builtin::AddressSpace::kUniform) {
                return true;
            }
        }
    }
    return false;
}

/// Offset is a simple ast::Expression builder interface, used to build byte
/// offsets for storage and uniform buffer accesses.
struct Offset : Castable<Offset> {
    /// @returns builds and returns the ast::Expression in `ctx.dst`
    virtual const ast::Expression* Build(CloneContext& ctx) const = 0;
};

/// OffsetExpr is an implementation of Offset that clones and casts the given
/// expression to `u32`.
struct OffsetExpr : Offset {
    const ast::Expression* const expr = nullptr;

    explicit OffsetExpr(const ast::Expression* e) : expr(e) {}

    const ast::Expression* Build(CloneContext& ctx) const override {
        auto* type = ctx.src->Sem().GetVal(expr)->Type()->UnwrapRef();
        auto* res = ctx.Clone(expr);
        if (!type->Is<type::U32>()) {
            res = ctx.dst->Call<u32>(res);
        }
        return res;
    }
};

/// OffsetLiteral is an implementation of Offset that constructs a u32 literal
/// value.
struct OffsetLiteral final : Castable<OffsetLiteral, Offset> {
    uint32_t const literal = 0;

    explicit OffsetLiteral(uint32_t lit) : literal(lit) {}

    const ast::Expression* Build(CloneContext& ctx) const override {
        return ctx.dst->Expr(u32(literal));
    }
};

/// OffsetBinOp is an implementation of Offset that constructs a binary-op of
/// two Offsets.
struct OffsetBinOp : Offset {
    ast::BinaryOp op;
    Offset const* lhs = nullptr;
    Offset const* rhs = nullptr;

    const ast::Expression* Build(CloneContext& ctx) const override {
        return ctx.dst->create<ast::BinaryExpression>(op, lhs->Build(ctx), rhs->Build(ctx));
    }
};

/// LoadStoreKey is the unordered map key to a load or store intrinsic.
struct LoadStoreKey {
    builtin::AddressSpace const address_space;  // buffer address space
    builtin::Access const access;               // buffer access
    type::Type const* buf_ty = nullptr;         // buffer type
    type::Type const* el_ty = nullptr;          // element type
    bool operator==(const LoadStoreKey& rhs) const {
        return address_space == rhs.address_space && access == rhs.access && buf_ty == rhs.buf_ty &&
               el_ty == rhs.el_ty;
    }
    struct Hasher {
        inline std::size_t operator()(const LoadStoreKey& u) const {
            return utils::Hash(u.address_space, u.access, u.buf_ty, u.el_ty);
        }
    };
};

/// AtomicKey is the unordered map key to an atomic intrinsic.
struct AtomicKey {
    builtin::Access const access;        // buffer access
    type::Type const* buf_ty = nullptr;  // buffer type
    type::Type const* el_ty = nullptr;   // element type
    sem::BuiltinType const op;           // atomic op
    bool operator==(const AtomicKey& rhs) const {
        return access == rhs.access && buf_ty == rhs.buf_ty && el_ty == rhs.el_ty && op == rhs.op;
    }
    struct Hasher {
        inline std::size_t operator()(const AtomicKey& u) const {
            return utils::Hash(u.access, u.buf_ty, u.el_ty, u.op);
        }
    };
};

bool IntrinsicDataTypeFor(const type::Type* ty, DecomposeMemoryAccess::Intrinsic::DataType& out) {
    if (ty->Is<type::I32>()) {
        out = DecomposeMemoryAccess::Intrinsic::DataType::kI32;
        return true;
    }
    if (ty->Is<type::U32>()) {
        out = DecomposeMemoryAccess::Intrinsic::DataType::kU32;
        return true;
    }
    if (ty->Is<type::F32>()) {
        out = DecomposeMemoryAccess::Intrinsic::DataType::kF32;
        return true;
    }
    if (ty->Is<type::F16>()) {
        out = DecomposeMemoryAccess::Intrinsic::DataType::kF16;
        return true;
    }
    if (auto* vec = ty->As<type::Vector>()) {
        switch (vec->Width()) {
            case 2:
                if (vec->type()->Is<type::I32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec2I32;
                    return true;
                }
                if (vec->type()->Is<type::U32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec2U32;
                    return true;
                }
                if (vec->type()->Is<type::F32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec2F32;
                    return true;
                }
                if (vec->type()->Is<type::F16>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec2F16;
                    return true;
                }
                break;
            case 3:
                if (vec->type()->Is<type::I32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec3I32;
                    return true;
                }
                if (vec->type()->Is<type::U32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec3U32;
                    return true;
                }
                if (vec->type()->Is<type::F32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec3F32;
                    return true;
                }
                if (vec->type()->Is<type::F16>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec3F16;
                    return true;
                }
                break;
            case 4:
                if (vec->type()->Is<type::I32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec4I32;
                    return true;
                }
                if (vec->type()->Is<type::U32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec4U32;
                    return true;
                }
                if (vec->type()->Is<type::F32>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec4F32;
                    return true;
                }
                if (vec->type()->Is<type::F16>()) {
                    out = DecomposeMemoryAccess::Intrinsic::DataType::kVec4F16;
                    return true;
                }
                break;
        }
        return false;
    }

    return false;
}

/// @returns a DecomposeMemoryAccess::Intrinsic attribute that can be applied
/// to a stub function to load the type `ty`.
DecomposeMemoryAccess::Intrinsic* IntrinsicLoadFor(ProgramBuilder* builder,
                                                   builtin::AddressSpace address_space,
                                                   const type::Type* ty) {
    DecomposeMemoryAccess::Intrinsic::DataType type;
    if (!IntrinsicDataTypeFor(ty, type)) {
        return nullptr;
    }
    return builder->ASTNodes().Create<DecomposeMemoryAccess::Intrinsic>(
        builder->ID(), builder->AllocateNodeID(), DecomposeMemoryAccess::Intrinsic::Op::kLoad,
        address_space, type);
}

/// @returns a DecomposeMemoryAccess::Intrinsic attribute that can be applied
/// to a stub function to store the type `ty`.
DecomposeMemoryAccess::Intrinsic* IntrinsicStoreFor(ProgramBuilder* builder,
                                                    builtin::AddressSpace address_space,
                                                    const type::Type* ty) {
    DecomposeMemoryAccess::Intrinsic::DataType type;
    if (!IntrinsicDataTypeFor(ty, type)) {
        return nullptr;
    }
    return builder->ASTNodes().Create<DecomposeMemoryAccess::Intrinsic>(
        builder->ID(), builder->AllocateNodeID(), DecomposeMemoryAccess::Intrinsic::Op::kStore,
        address_space, type);
}

/// @returns a DecomposeMemoryAccess::Intrinsic attribute that can be applied
/// to a stub function for the atomic op and the type `ty`.
DecomposeMemoryAccess::Intrinsic* IntrinsicAtomicFor(ProgramBuilder* builder,
                                                     sem::BuiltinType ity,
                                                     const type::Type* ty) {
    auto op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicLoad;
    switch (ity) {
        case sem::BuiltinType::kAtomicLoad:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicLoad;
            break;
        case sem::BuiltinType::kAtomicStore:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicStore;
            break;
        case sem::BuiltinType::kAtomicAdd:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicAdd;
            break;
        case sem::BuiltinType::kAtomicSub:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicSub;
            break;
        case sem::BuiltinType::kAtomicMax:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicMax;
            break;
        case sem::BuiltinType::kAtomicMin:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicMin;
            break;
        case sem::BuiltinType::kAtomicAnd:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicAnd;
            break;
        case sem::BuiltinType::kAtomicOr:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicOr;
            break;
        case sem::BuiltinType::kAtomicXor:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicXor;
            break;
        case sem::BuiltinType::kAtomicExchange:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicExchange;
            break;
        case sem::BuiltinType::kAtomicCompareExchangeWeak:
            op = DecomposeMemoryAccess::Intrinsic::Op::kAtomicCompareExchangeWeak;
            break;
        default:
            TINT_ICE(Transform, builder->Diagnostics())
                << "invalid IntrinsicType for DecomposeMemoryAccess::Intrinsic: "
                << ty->TypeInfo().name;
            break;
    }

    DecomposeMemoryAccess::Intrinsic::DataType type;
    if (!IntrinsicDataTypeFor(ty, type)) {
        return nullptr;
    }
    return builder->ASTNodes().Create<DecomposeMemoryAccess::Intrinsic>(
        builder->ID(), builder->AllocateNodeID(), op, builtin::AddressSpace::kStorage, type);
}

/// BufferAccess describes a single storage or uniform buffer access
struct BufferAccess {
    sem::ValueExpression const* var = nullptr;  // Storage buffer variable
    Offset const* offset = nullptr;             // The byte offset on var
    type::Type const* type = nullptr;           // The type of the access
    operator bool() const { return var; }       // Returns true if valid
};

/// Store describes a single storage or uniform buffer write
struct Store {
    const ast::AssignmentStatement* assignment;  // The AST assignment statement
    BufferAccess target;                         // The target for the write
};

}  // namespace

/// PIMPL state for the transform
struct DecomposeMemoryAccess::State {
    /// The clone context
    CloneContext& ctx;
    /// Alias to `*ctx.dst`
    ProgramBuilder& b;
    /// Map of AST expression to storage or uniform buffer access
    /// This map has entries added when encountered, and removed when outer
    /// expressions chain the access.
    /// Subset of #expression_order, as expressions are not removed from
    /// #expression_order.
    std::unordered_map<const ast::Expression*, BufferAccess> accesses;
    /// The visited order of AST expressions (superset of #accesses)
    std::vector<const ast::Expression*> expression_order;
    /// [buffer-type, element-type] -> load function name
    std::unordered_map<LoadStoreKey, Symbol, LoadStoreKey::Hasher> load_funcs;
    /// [buffer-type, element-type] -> store function name
    std::unordered_map<LoadStoreKey, Symbol, LoadStoreKey::Hasher> store_funcs;
    /// [buffer-type, element-type, atomic-op] -> load function name
    std::unordered_map<AtomicKey, Symbol, AtomicKey::Hasher> atomic_funcs;
    /// List of storage or uniform buffer writes
    std::vector<Store> stores;
    /// Allocations for offsets
    utils::BlockAllocator<Offset> offsets_;

    /// Constructor
    /// @param context the CloneContext
    explicit State(CloneContext& context) : ctx(context), b(*ctx.dst) {}

    /// @param offset the offset value to wrap in an Offset
    /// @returns an Offset for the given literal value
    const Offset* ToOffset(uint32_t offset) { return offsets_.Create<OffsetLiteral>(offset); }

    /// @param expr the expression to convert to an Offset
    /// @returns an Offset for the given ast::Expression
    const Offset* ToOffset(const ast::Expression* expr) {
        if (auto* lit = expr->As<ast::IntLiteralExpression>()) {
            if (lit->value >= 0) {
                return offsets_.Create<OffsetLiteral>(static_cast<uint32_t>(lit->value));
            }
        }
        return offsets_.Create<OffsetExpr>(expr);
    }

    /// @param offset the Offset that is returned
    /// @returns the given offset (pass-through)
    const Offset* ToOffset(const Offset* offset) { return offset; }

    /// @param lhs_ the left-hand side of the add expression
    /// @param rhs_ the right-hand side of the add expression
    /// @return an Offset that is a sum of lhs and rhs, performing basic constant
    /// folding if possible
    template <typename LHS, typename RHS>
    const Offset* Add(LHS&& lhs_, RHS&& rhs_) {
        auto* lhs = ToOffset(std::forward<LHS>(lhs_));
        auto* rhs = ToOffset(std::forward<RHS>(rhs_));
        auto* lhs_lit = tint::As<OffsetLiteral>(lhs);
        auto* rhs_lit = tint::As<OffsetLiteral>(rhs);
        if (lhs_lit && lhs_lit->literal == 0) {
            return rhs;
        }
        if (rhs_lit && rhs_lit->literal == 0) {
            return lhs;
        }
        if (lhs_lit && rhs_lit) {
            if (static_cast<uint64_t>(lhs_lit->literal) + static_cast<uint64_t>(rhs_lit->literal) <=
                0xffffffff) {
                return offsets_.Create<OffsetLiteral>(lhs_lit->literal + rhs_lit->literal);
            }
        }
        auto* out = offsets_.Create<OffsetBinOp>();
        out->op = ast::BinaryOp::kAdd;
        out->lhs = lhs;
        out->rhs = rhs;
        return out;
    }

    /// @param lhs_ the left-hand side of the multiply expression
    /// @param rhs_ the right-hand side of the multiply expression
    /// @return an Offset that is the multiplication of lhs and rhs, performing
    /// basic constant folding if possible
    template <typename LHS, typename RHS>
    const Offset* Mul(LHS&& lhs_, RHS&& rhs_) {
        auto* lhs = ToOffset(std::forward<LHS>(lhs_));
        auto* rhs = ToOffset(std::forward<RHS>(rhs_));
        auto* lhs_lit = tint::As<OffsetLiteral>(lhs);
        auto* rhs_lit = tint::As<OffsetLiteral>(rhs);
        if (lhs_lit && lhs_lit->literal == 0) {
            return offsets_.Create<OffsetLiteral>(0u);
        }
        if (rhs_lit && rhs_lit->literal == 0) {
            return offsets_.Create<OffsetLiteral>(0u);
        }
        if (lhs_lit && lhs_lit->literal == 1) {
            return rhs;
        }
        if (rhs_lit && rhs_lit->literal == 1) {
            return lhs;
        }
        if (lhs_lit && rhs_lit) {
            return offsets_.Create<OffsetLiteral>(lhs_lit->literal * rhs_lit->literal);
        }
        auto* out = offsets_.Create<OffsetBinOp>();
        out->op = ast::BinaryOp::kMultiply;
        out->lhs = lhs;
        out->rhs = rhs;
        return out;
    }

    /// AddAccess() adds the `expr -> access` map item to #accesses, and `expr`
    /// to #expression_order.
    /// @param expr the expression that performs the access
    /// @param access the access
    void AddAccess(const ast::Expression* expr, const BufferAccess& access) {
        TINT_ASSERT(Transform, access.type);
        accesses.emplace(expr, access);
        expression_order.emplace_back(expr);
    }

    /// TakeAccess() removes the `node` item from #accesses (if it exists),
    /// returning the BufferAccess. If #accesses does not hold an item for
    /// `node`, an invalid BufferAccess is returned.
    /// @param node the expression that performed an access
    /// @return the BufferAccess for the given expression
    BufferAccess TakeAccess(const ast::Expression* node) {
        auto lhs_it = accesses.find(node);
        if (lhs_it == accesses.end()) {
            return {};
        }
        auto access = lhs_it->second;
        accesses.erase(node);
        return access;
    }

    /// LoadFunc() returns a symbol to an intrinsic function that loads an element of type `el_ty`
    /// from a storage or uniform buffer of type `buf_ty`.
    /// The emitted function has the signature:
    ///   `fn load(buf : ptr<SC, buf_ty, A>, offset : u32) -> el_ty`
    /// @param buf_ty the storage or uniform buffer type
    /// @param el_ty the storage or uniform buffer element type
    /// @param var_user the variable user
    /// @return the name of the function that performs the load
    Symbol LoadFunc(const type::Type* buf_ty,
                    const type::Type* el_ty,
                    const sem::VariableUser* var_user) {
        auto address_space = var_user->Variable()->AddressSpace();
        auto access = var_user->Variable()->Access();
        if (address_space != builtin::AddressSpace::kStorage) {
            access = builtin::Access::kUndefined;
        }
        return utils::GetOrCreate(
            load_funcs, LoadStoreKey{address_space, access, buf_ty, el_ty}, [&] {
                utils::Vector params{
                    b.Param("buffer",
                            b.ty.pointer(CreateASTTypeFor(ctx, buf_ty), address_space, access),
                            utils::Vector{b.Disable(ast::DisabledValidation::kFunctionParameter)}),
                    b.Param("offset", b.ty.u32()),
                };

                auto name = b.Sym();

                if (auto* intrinsic = IntrinsicLoadFor(ctx.dst, address_space, el_ty)) {
                    auto el_ast_ty = CreateASTTypeFor(ctx, el_ty);
                    b.Func(name, params, el_ast_ty, nullptr,
                           utils::Vector{
                               intrinsic,
                               b.Disable(ast::DisabledValidation::kFunctionHasNoBody),
                           });
                } else if (auto* arr_ty = el_ty->As<type::Array>()) {
                    // fn load_func(buffer : buf_ty, offset : u32) -> array<T, N> {
                    //   var arr : array<T, N>;
                    //   for (var i = 0u; i < array_count; i = i + 1) {
                    //     arr[i] = el_load_func(buffer, offset + i * array_stride)
                    //   }
                    //   return arr;
                    // }
                    auto load = LoadFunc(buf_ty, arr_ty->ElemType()->UnwrapRef(), var_user);
                    auto* arr = b.Var(b.Symbols().New("arr"), CreateASTTypeFor(ctx, arr_ty));
                    auto* i = b.Var(b.Symbols().New("i"), b.Expr(0_u));
                    auto* for_init = b.Decl(i);
                    auto arr_cnt = arr_ty->ConstantCount();
                    if (TINT_UNLIKELY(!arr_cnt)) {
                        // Non-constant counts should not be possible:
                        // * Override-expression counts can only be applied to workgroup arrays, and
                        //   this method only handles storage and uniform.
                        // * Runtime-sized arrays are not loadable.
                        TINT_ICE(Transform, b.Diagnostics())
                            << "unexpected non-constant array count";
                        arr_cnt = 1;
                    }
                    auto* for_cond = b.create<ast::BinaryExpression>(
                        ast::BinaryOp::kLessThan, b.Expr(i), b.Expr(u32(arr_cnt.value())));
                    auto* for_cont = b.Assign(i, b.Add(i, 1_u));
                    auto* arr_el = b.IndexAccessor(arr, i);
                    auto* el_offset = b.Add(b.Expr("offset"), b.Mul(i, u32(arr_ty->Stride())));
                    auto* el_val = b.Call(load, "buffer", el_offset);
                    auto* for_loop =
                        b.For(for_init, for_cond, for_cont, b.Block(b.Assign(arr_el, el_val)));

                    b.Func(name, params, CreateASTTypeFor(ctx, arr_ty),
                           utils::Vector{
                               b.Decl(arr),
                               for_loop,
                               b.Return(arr),
                           });
                } else {
                    utils::Vector<const ast::Expression*, 8> values;
                    if (auto* mat_ty = el_ty->As<type::Matrix>()) {
                        auto* vec_ty = mat_ty->ColumnType();
                        Symbol load = LoadFunc(buf_ty, vec_ty, var_user);
                        for (uint32_t i = 0; i < mat_ty->columns(); i++) {
                            auto* offset = b.Add("offset", u32(i * mat_ty->ColumnStride()));
                            values.Push(b.Call(load, "buffer", offset));
                        }
                    } else if (auto* str = el_ty->As<sem::Struct>()) {
                        for (auto* member : str->Members()) {
                            auto* offset = b.Add("offset", u32(member->Offset()));
                            Symbol load = LoadFunc(buf_ty, member->Type()->UnwrapRef(), var_user);
                            values.Push(b.Call(load, "buffer", offset));
                        }
                    }
                    b.Func(name, params, CreateASTTypeFor(ctx, el_ty),
                           utils::Vector{
                               b.Return(b.Call(CreateASTTypeFor(ctx, el_ty), values)),
                           });
                }
                return name;
            });
    }

    /// StoreFunc() returns a symbol to an intrinsic function that stores an
    /// element of type `el_ty` to a storage buffer of type `buf_ty`.
    /// The function has the signature:
    ///   `fn store(buf : ptr<SC, buf_ty, A>, offset : u32, value : el_ty)`
    /// @param buf_ty the storage buffer type
    /// @param el_ty the storage buffer element type
    /// @param var_user the variable user
    /// @return the name of the function that performs the store
    Symbol StoreFunc(const type::Type* buf_ty,
                     const type::Type* el_ty,
                     const sem::VariableUser* var_user) {
        auto address_space = var_user->Variable()->AddressSpace();
        auto access = var_user->Variable()->Access();
        if (address_space != builtin::AddressSpace::kStorage) {
            access = builtin::Access::kUndefined;
        }
        return utils::GetOrCreate(
            store_funcs, LoadStoreKey{address_space, access, buf_ty, el_ty}, [&] {
                utils::Vector params{
                    b.Param("buffer",
                            b.ty.pointer(CreateASTTypeFor(ctx, buf_ty), address_space, access),
                            utils::Vector{b.Disable(ast::DisabledValidation::kFunctionParameter)}),
                    b.Param("offset", b.ty.u32()),
                    b.Param("value", CreateASTTypeFor(ctx, el_ty)),
                };

                auto name = b.Sym();

                if (auto* intrinsic = IntrinsicStoreFor(ctx.dst, address_space, el_ty)) {
                    b.Func(name, params, b.ty.void_(), nullptr,
                           utils::Vector{
                               intrinsic,
                               b.Disable(ast::DisabledValidation::kFunctionHasNoBody),
                           });
                } else {
                    auto body = Switch<utils::Vector<const ast::Statement*, 8>>(
                        el_ty,  //
                        [&](const type::Array* arr_ty) {
                            // fn store_func(buffer : buf_ty, offset : u32, value : el_ty) {
                            //   var array = value; // No dynamic indexing on constant arrays
                            //   for (var i = 0u; i < array_count; i = i + 1) {
                            //     arr[i] = el_store_func(buffer, offset + i * array_stride,
                            //     value[i])
                            //   }
                            //   return arr;
                            // }
                            auto* array = b.Var(b.Symbols().New("array"), b.Expr("value"));
                            auto store =
                                StoreFunc(buf_ty, arr_ty->ElemType()->UnwrapRef(), var_user);
                            auto* i = b.Var(b.Symbols().New("i"), b.Expr(0_u));
                            auto* for_init = b.Decl(i);
                            auto arr_cnt = arr_ty->ConstantCount();
                            if (TINT_UNLIKELY(!arr_cnt)) {
                                // Non-constant counts should not be possible:
                                // * Override-expression counts can only be applied to workgroup
                                //   arrays, and this method only handles storage and uniform.
                                // * Runtime-sized arrays are not storable.
                                TINT_ICE(Transform, b.Diagnostics())
                                    << "unexpected non-constant array count";
                                arr_cnt = 1;
                            }
                            auto* for_cond = b.create<ast::BinaryExpression>(
                                ast::BinaryOp::kLessThan, b.Expr(i), b.Expr(u32(arr_cnt.value())));
                            auto* for_cont = b.Assign(i, b.Add(i, 1_u));
                            auto* arr_el = b.IndexAccessor(array, i);
                            auto* el_offset =
                                b.Add(b.Expr("offset"), b.Mul(i, u32(arr_ty->Stride())));
                            auto* store_stmt =
                                b.CallStmt(b.Call(store, "buffer", el_offset, arr_el));
                            auto* for_loop =
                                b.For(for_init, for_cond, for_cont, b.Block(store_stmt));

                            return utils::Vector{b.Decl(array), for_loop};
                        },
                        [&](const type::Matrix* mat_ty) {
                            auto* vec_ty = mat_ty->ColumnType();
                            Symbol store = StoreFunc(buf_ty, vec_ty, var_user);
                            utils::Vector<const ast::Statement*, 4> stmts;
                            for (uint32_t i = 0; i < mat_ty->columns(); i++) {
                                auto* offset = b.Add("offset", u32(i * mat_ty->ColumnStride()));
                                auto* element = b.IndexAccessor("value", u32(i));
                                auto* call = b.Call(store, "buffer", offset, element);
                                stmts.Push(b.CallStmt(call));
                            }
                            return stmts;
                        },
                        [&](const sem::Struct* str) {
                            utils::Vector<const ast::Statement*, 8> stmts;
                            for (auto* member : str->Members()) {
                                auto* offset = b.Add("offset", u32(member->Offset()));
                                auto* element =
                                    b.MemberAccessor("value", ctx.Clone(member->Name()));
                                Symbol store =
                                    StoreFunc(buf_ty, member->Type()->UnwrapRef(), var_user);
                                auto* call = b.Call(store, "buffer", offset, element);
                                stmts.Push(b.CallStmt(call));
                            }
                            return stmts;
                        });

                    b.Func(name, params, b.ty.void_(), body);
                }

                return name;
            });
    }

    /// AtomicFunc() returns a symbol to an intrinsic function that performs an
    /// atomic operation from a storage buffer of type `buf_ty`. The function has
    /// the signature:
    // `fn atomic_op(buf : ptr<storage, buf_ty, A>, offset : u32, ...) -> T`
    /// @param buf_ty the storage buffer type
    /// @param el_ty the storage buffer element type
    /// @param intrinsic the atomic intrinsic
    /// @param var_user the variable user
    /// @return the name of the function that performs the load
    Symbol AtomicFunc(const type::Type* buf_ty,
                      const type::Type* el_ty,
                      const sem::Builtin* intrinsic,
                      const sem::VariableUser* var_user) {
        auto op = intrinsic->Type();
        auto address_space = var_user->Variable()->AddressSpace();
        auto access = var_user->Variable()->Access();
        if (address_space != builtin::AddressSpace::kStorage) {
            access = builtin::Access::kUndefined;
        }
        return utils::GetOrCreate(atomic_funcs, AtomicKey{access, buf_ty, el_ty, op}, [&] {
            // The first parameter to all WGSL atomics is the expression to the
            // atomic. This is replaced with two parameters: the buffer and offset.
            utils::Vector params{
                b.Param("buffer",
                        b.ty.pointer(CreateASTTypeFor(ctx, buf_ty), builtin::AddressSpace::kStorage,
                                     access),
                        utils::Vector{b.Disable(ast::DisabledValidation::kFunctionParameter)}),
                b.Param("offset", b.ty.u32()),
            };

            // Other parameters are copied as-is:
            for (size_t i = 1; i < intrinsic->Parameters().Length(); i++) {
                auto* param = intrinsic->Parameters()[i];
                auto ty = CreateASTTypeFor(ctx, param->Type());
                params.Push(b.Param("param_" + std::to_string(i), ty));
            }

            auto* atomic = IntrinsicAtomicFor(ctx.dst, op, el_ty);
            if (TINT_UNLIKELY(!atomic)) {
                TINT_ICE(Transform, b.Diagnostics())
                    << "IntrinsicAtomicFor() returned nullptr for op " << op << " and type "
                    << el_ty->TypeInfo().name;
            }

            ast::Type ret_ty;

            // For intrinsics that return a struct, there is no AST node for it, so create one now.
            if (intrinsic->Type() == sem::BuiltinType::kAtomicCompareExchangeWeak) {
                auto* str = intrinsic->ReturnType()->As<sem::Struct>();
                TINT_ASSERT(Transform, str && str->Declaration() == nullptr);

                utils::Vector<const ast::StructMember*, 8> ast_members;
                ast_members.Reserve(str->Members().Length());
                for (auto& m : str->Members()) {
                    ast_members.Push(
                        b.Member(ctx.Clone(m->Name()), CreateASTTypeFor(ctx, m->Type())));
                }

                auto name = b.Symbols().New("atomic_compare_exchange_weak_ret_type");
                auto* new_str = b.Structure(name, std::move(ast_members));
                ret_ty = b.ty.Of(new_str);
            } else {
                ret_ty = CreateASTTypeFor(ctx, intrinsic->ReturnType());
            }

            auto name = b.Symbols().New(std::string{"tint_"} + intrinsic->str());
            b.Func(name, std::move(params), ret_ty, nullptr,
                   utils::Vector{
                       atomic,
                       b.Disable(ast::DisabledValidation::kFunctionHasNoBody),
                   });
            return name;
        });
    }
};

DecomposeMemoryAccess::Intrinsic::Intrinsic(ProgramID pid,
                                            ast::NodeID nid,
                                            Op o,
                                            builtin::AddressSpace sc,
                                            DataType ty)
    : Base(pid, nid), op(o), address_space(sc), type(ty) {}
DecomposeMemoryAccess::Intrinsic::~Intrinsic() = default;
std::string DecomposeMemoryAccess::Intrinsic::InternalName() const {
    std::stringstream ss;
    switch (op) {
        case Op::kLoad:
            ss << "intrinsic_load_";
            break;
        case Op::kStore:
            ss << "intrinsic_store_";
            break;
        case Op::kAtomicLoad:
            ss << "intrinsic_atomic_load_";
            break;
        case Op::kAtomicStore:
            ss << "intrinsic_atomic_store_";
            break;
        case Op::kAtomicAdd:
            ss << "intrinsic_atomic_add_";
            break;
        case Op::kAtomicSub:
            ss << "intrinsic_atomic_sub_";
            break;
        case Op::kAtomicMax:
            ss << "intrinsic_atomic_max_";
            break;
        case Op::kAtomicMin:
            ss << "intrinsic_atomic_min_";
            break;
        case Op::kAtomicAnd:
            ss << "intrinsic_atomic_and_";
            break;
        case Op::kAtomicOr:
            ss << "intrinsic_atomic_or_";
            break;
        case Op::kAtomicXor:
            ss << "intrinsic_atomic_xor_";
            break;
        case Op::kAtomicExchange:
            ss << "intrinsic_atomic_exchange_";
            break;
        case Op::kAtomicCompareExchangeWeak:
            ss << "intrinsic_atomic_compare_exchange_weak_";
            break;
    }
    ss << address_space << "_";
    switch (type) {
        case DataType::kU32:
            ss << "u32";
            break;
        case DataType::kF32:
            ss << "f32";
            break;
        case DataType::kI32:
            ss << "i32";
            break;
        case DataType::kF16:
            ss << "f16";
            break;
        case DataType::kVec2U32:
            ss << "vec2_u32";
            break;
        case DataType::kVec2F32:
            ss << "vec2_f32";
            break;
        case DataType::kVec2I32:
            ss << "vec2_i32";
            break;
        case DataType::kVec2F16:
            ss << "vec2_f16";
            break;
        case DataType::kVec3U32:
            ss << "vec3_u32";
            break;
        case DataType::kVec3F32:
            ss << "vec3_f32";
            break;
        case DataType::kVec3I32:
            ss << "vec3_i32";
            break;
        case DataType::kVec3F16:
            ss << "vec3_f16";
            break;
        case DataType::kVec4U32:
            ss << "vec4_u32";
            break;
        case DataType::kVec4F32:
            ss << "vec4_f32";
            break;
        case DataType::kVec4I32:
            ss << "vec4_i32";
            break;
        case DataType::kVec4F16:
            ss << "vec4_f16";
            break;
    }
    return ss.str();
}

const DecomposeMemoryAccess::Intrinsic* DecomposeMemoryAccess::Intrinsic::Clone(
    CloneContext* ctx) const {
    return ctx->dst->ASTNodes().Create<DecomposeMemoryAccess::Intrinsic>(
        ctx->dst->ID(), ctx->dst->AllocateNodeID(), op, address_space, type);
}

bool DecomposeMemoryAccess::Intrinsic::IsAtomic() const {
    return op != Op::kLoad && op != Op::kStore;
}

DecomposeMemoryAccess::DecomposeMemoryAccess() = default;
DecomposeMemoryAccess::~DecomposeMemoryAccess() = default;

Transform::ApplyResult DecomposeMemoryAccess::Apply(const Program* src,
                                                    const DataMap&,
                                                    DataMap&) const {
    if (!ShouldRun(src)) {
        return SkipTransform;
    }

    auto& sem = src->Sem();
    ProgramBuilder b;
    CloneContext ctx{&b, src, /* auto_clone_symbols */ true};
    State state(ctx);

    // Scan the AST nodes for storage and uniform buffer accesses. Complex
    // expression chains (e.g. `storage_buffer.foo.bar[20].x`) are handled by
    // maintaining an offset chain via the `state.TakeAccess()`,
    // `state.AddAccess()` methods.
    //
    // Inner-most expression nodes are guaranteed to be visited first because AST
    // nodes are fully immutable and require their children to be constructed
    // first so their pointer can be passed to the parent's initializer.
    for (auto* node : src->ASTNodes().Objects()) {
        if (auto* ident = node->As<ast::IdentifierExpression>()) {
            // X
            if (auto* sem_ident = sem.GetVal(ident)) {
                if (auto* var = sem_ident->UnwrapLoad()->As<sem::VariableUser>()) {
                    if (var->Variable()->AddressSpace() == builtin::AddressSpace::kStorage ||
                        var->Variable()->AddressSpace() == builtin::AddressSpace::kUniform) {
                        // Variable to a storage or uniform buffer
                        state.AddAccess(ident, {
                                                   var,
                                                   state.ToOffset(0u),
                                                   var->Type()->UnwrapRef(),
                                               });
                    }
                }
            }
            continue;
        }

        if (auto* accessor = node->As<ast::MemberAccessorExpression>()) {
            // X.Y
            auto* accessor_sem = sem.Get(accessor)->UnwrapLoad();
            if (auto* swizzle = accessor_sem->As<sem::Swizzle>()) {
                if (swizzle->Indices().Length() == 1) {
                    if (auto access = state.TakeAccess(accessor->object)) {
                        auto* vec_ty = access.type->As<type::Vector>();
                        auto* offset = state.Mul(vec_ty->type()->Size(), swizzle->Indices()[0u]);
                        state.AddAccess(accessor, {
                                                      access.var,
                                                      state.Add(access.offset, offset),
                                                      vec_ty->type(),
                                                  });
                    }
                }
            } else {
                if (auto access = state.TakeAccess(accessor->object)) {
                    auto* str_ty = access.type->As<sem::Struct>();
                    auto* member = str_ty->FindMember(accessor->member->symbol);
                    auto offset = member->Offset();
                    state.AddAccess(accessor, {
                                                  access.var,
                                                  state.Add(access.offset, offset),
                                                  member->Type(),
                                              });
                }
            }
            continue;
        }

        if (auto* accessor = node->As<ast::IndexAccessorExpression>()) {
            if (auto access = state.TakeAccess(accessor->object)) {
                // X[Y]
                if (auto* arr = access.type->As<type::Array>()) {
                    auto* offset = state.Mul(arr->Stride(), accessor->index);
                    state.AddAccess(accessor, {
                                                  access.var,
                                                  state.Add(access.offset, offset),
                                                  arr->ElemType(),
                                              });
                    continue;
                }
                if (auto* vec_ty = access.type->As<type::Vector>()) {
                    auto* offset = state.Mul(vec_ty->type()->Size(), accessor->index);
                    state.AddAccess(accessor, {
                                                  access.var,
                                                  state.Add(access.offset, offset),
                                                  vec_ty->type(),
                                              });
                    continue;
                }
                if (auto* mat_ty = access.type->As<type::Matrix>()) {
                    auto* offset = state.Mul(mat_ty->ColumnStride(), accessor->index);
                    state.AddAccess(accessor, {
                                                  access.var,
                                                  state.Add(access.offset, offset),
                                                  mat_ty->ColumnType(),
                                              });
                    continue;
                }
            }
        }

        if (auto* op = node->As<ast::UnaryOpExpression>()) {
            if (op->op == ast::UnaryOp::kAddressOf) {
                // &X
                if (auto access = state.TakeAccess(op->expr)) {
                    // HLSL does not support pointers, so just take the access from the
                    // reference and place it on the pointer.
                    state.AddAccess(op, access);
                    continue;
                }
            }
        }

        if (auto* assign = node->As<ast::AssignmentStatement>()) {
            // X = Y
            // Move the LHS access to a store.
            if (auto lhs = state.TakeAccess(assign->lhs)) {
                state.stores.emplace_back(Store{assign, lhs});
            }
        }

        if (auto* call_expr = node->As<ast::CallExpression>()) {
            auto* call = sem.Get(call_expr)->UnwrapMaterialize()->As<sem::Call>();
            if (auto* builtin = call->Target()->As<sem::Builtin>()) {
                if (builtin->Type() == sem::BuiltinType::kArrayLength) {
                    // arrayLength(X)
                    // Don't convert X into a load, this builtin actually requires the real pointer.
                    state.TakeAccess(call_expr->args[0]);
                    continue;
                }
                if (builtin->IsAtomic()) {
                    if (auto access = state.TakeAccess(call_expr->args[0])) {
                        // atomic___(X)
                        ctx.Replace(call_expr, [=, &ctx, &state] {
                            auto* buf = access.var->Declaration();
                            auto* offset = access.offset->Build(ctx);
                            auto* buf_ty = access.var->Type()->UnwrapRef();
                            auto* el_ty = access.type->UnwrapRef()->As<type::Atomic>()->Type();
                            Symbol func = state.AtomicFunc(buf_ty, el_ty, builtin,
                                                           access.var->As<sem::VariableUser>());

                            utils::Vector<const ast::Expression*, 8> args{
                                ctx.dst->AddressOf(ctx.Clone(buf)), offset};
                            for (size_t i = 1; i < call_expr->args.Length(); i++) {
                                auto* arg = call_expr->args[i];
                                args.Push(ctx.Clone(arg));
                            }
                            return ctx.dst->Call(func, args);
                        });
                    }
                }
            }
        }
    }

    // All remaining accesses are loads, transform these into calls to the
    // corresponding load function
    // TODO(crbug.com/tint/1784): Use `sem::Load`s instead of maintaining `state.expression_order`.
    for (auto* expr : state.expression_order) {
        auto access_it = state.accesses.find(expr);
        if (access_it == state.accesses.end()) {
            continue;
        }
        BufferAccess access = access_it->second;
        ctx.Replace(expr, [=, &ctx, &state] {
            auto* buf = ctx.dst->AddressOf(ctx.CloneWithoutTransform(access.var->Declaration()));
            auto* offset = access.offset->Build(ctx);
            auto* buf_ty = access.var->Type()->UnwrapRef();
            auto* el_ty = access.type->UnwrapRef();
            Symbol func = state.LoadFunc(buf_ty, el_ty, access.var->As<sem::VariableUser>());
            return ctx.dst->Call(func, buf, offset);
        });
    }

    // And replace all storage and uniform buffer assignments with stores
    for (auto store : state.stores) {
        ctx.Replace(store.assignment, [=, &ctx, &state] {
            auto* buf =
                ctx.dst->AddressOf(ctx.CloneWithoutTransform((store.target.var->Declaration())));
            auto* offset = store.target.offset->Build(ctx);
            auto* buf_ty = store.target.var->Type()->UnwrapRef();
            auto* el_ty = store.target.type->UnwrapRef();
            auto* value = store.assignment->rhs;
            Symbol func = state.StoreFunc(buf_ty, el_ty, store.target.var->As<sem::VariableUser>());
            auto* call = ctx.dst->Call(func, buf, offset, ctx.Clone(value));
            return ctx.dst->CallStmt(call);
        });
    }

    ctx.Clone();
    return Program(std::move(b));
}

}  // namespace tint::transform

TINT_INSTANTIATE_TYPEINFO(tint::transform::Offset);
TINT_INSTANTIATE_TYPEINFO(tint::transform::OffsetLiteral);
