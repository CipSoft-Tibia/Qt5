// Copyright 2020 The Tint Authors.
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

#include "src/tint/lang/wgsl/ast/transform/transform.h"

#include <algorithm>
#include <string>

#include "src/tint/lang/core/builtin.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/core/type/sampler.h"
#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/block_statement.h"
#include "src/tint/lang/wgsl/sem/for_loop_statement.h"
#include "src/tint/lang/wgsl/sem/variable.h"

using namespace tint::core::fluent_types;  // NOLINT

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::Transform);

namespace tint::ast::transform {

Output::Output() = default;
Output::Output(Program&& p) : program(std::move(p)) {}
Transform::Transform() = default;
Transform::~Transform() = default;

Output Transform::Run(const Program* src, const DataMap& data /* = {} */) const {
    Output output;
    if (auto program = Apply(src, data, output.data)) {
        output.program = std::move(program.value());
    } else {
        ProgramBuilder b;
        program::CloneContext ctx{&b, src, /* auto_clone_symbols */ true};
        ctx.Clone();
        output.program = resolver::Resolve(b);
    }
    return output;
}

void Transform::RemoveStatement(program::CloneContext& ctx, const Statement* stmt) {
    auto* sem = ctx.src->Sem().Get(stmt);
    if (auto* block = tint::As<sem::BlockStatement>(sem->Parent())) {
        ctx.Remove(block->Declaration()->statements, stmt);
        return;
    }
    if (TINT_LIKELY(tint::Is<sem::ForLoopStatement>(sem->Parent()))) {
        ctx.Replace(stmt, static_cast<Expression*>(nullptr));
        return;
    }
    TINT_ICE() << "unable to remove statement from parent of type " << sem->TypeInfo().name;
}

Type Transform::CreateASTTypeFor(program::CloneContext& ctx, const core::type::Type* ty) {
    if (ty->Is<core::type::Void>()) {
        return Type{};
    }
    if (ty->Is<core::type::I32>()) {
        return ctx.dst->ty.i32();
    }
    if (ty->Is<core::type::U32>()) {
        return ctx.dst->ty.u32();
    }
    if (ty->Is<core::type::F16>()) {
        return ctx.dst->ty.f16();
    }
    if (ty->Is<core::type::F32>()) {
        return ctx.dst->ty.f32();
    }
    if (ty->Is<core::type::Bool>()) {
        return ctx.dst->ty.bool_();
    }
    if (auto* m = ty->As<core::type::Matrix>()) {
        auto el = CreateASTTypeFor(ctx, m->type());
        return ctx.dst->ty.mat(el, m->columns(), m->rows());
    }
    if (auto* v = ty->As<core::type::Vector>()) {
        auto el = CreateASTTypeFor(ctx, v->type());
        if (v->Packed()) {
            TINT_ASSERT(v->Width() == 3u);
            return ctx.dst->ty(core::Builtin::kPackedVec3, el);
        } else {
            return ctx.dst->ty.vec(el, v->Width());
        }
    }
    if (auto* a = ty->As<core::type::Array>()) {
        auto el = CreateASTTypeFor(ctx, a->ElemType());
        tint::Vector<const Attribute*, 1> attrs;
        if (!a->IsStrideImplicit()) {
            attrs.Push(ctx.dst->create<StrideAttribute>(a->Stride()));
        }
        if (a->Count()->Is<core::type::RuntimeArrayCount>()) {
            return ctx.dst->ty.array(el, std::move(attrs));
        }
        if (auto* override = a->Count()->As<sem::NamedOverrideArrayCount>()) {
            auto* count = ctx.Clone(override->variable->Declaration());
            return ctx.dst->ty.array(el, count, std::move(attrs));
        }
        if (auto* override = a->Count()->As<sem::UnnamedOverrideArrayCount>()) {
            // If the array count is an unnamed (complex) override expression, then its not safe to
            // redeclare this type as we'd end up with two types that would not compare equal.
            // See crbug.com/tint/1764.
            // Look for a type alias for this array.
            for (auto* type_decl : ctx.src->AST().TypeDecls()) {
                if (auto* alias = type_decl->As<Alias>()) {
                    if (ty == ctx.src->Sem().Get(alias)) {
                        // Alias found. Use the alias name to ensure types compare equal.
                        return ctx.dst->ty(ctx.Clone(alias->name->symbol));
                    }
                }
            }
            // Array is not aliased. Rebuild the array.
            auto* count = ctx.Clone(override->expr->Declaration());
            return ctx.dst->ty.array(el, count, std::move(attrs));
        }
        auto count = a->ConstantCount();
        if (TINT_UNLIKELY(!count)) {
            TINT_ICE() << core::type::Array::kErrExpectedConstantCount;
            return ctx.dst->ty.array(el, u32(1), std::move(attrs));
        }
        return ctx.dst->ty.array(el, u32(count.value()), std::move(attrs));
    }
    if (auto* s = ty->As<core::type::Struct>()) {
        return ctx.dst->ty(ctx.Clone(s->Name()));
    }
    if (auto* s = ty->As<core::type::Reference>()) {
        return CreateASTTypeFor(ctx, s->StoreType());
    }
    if (auto* a = ty->As<core::type::Atomic>()) {
        return ctx.dst->ty.atomic(CreateASTTypeFor(ctx, a->Type()));
    }
    if (auto* t = ty->As<core::type::DepthTexture>()) {
        return ctx.dst->ty.depth_texture(t->dim());
    }
    if (auto* t = ty->As<core::type::DepthMultisampledTexture>()) {
        return ctx.dst->ty.depth_multisampled_texture(t->dim());
    }
    if (ty->Is<core::type::ExternalTexture>()) {
        return ctx.dst->ty.external_texture();
    }
    if (auto* t = ty->As<core::type::MultisampledTexture>()) {
        return ctx.dst->ty.multisampled_texture(t->dim(), CreateASTTypeFor(ctx, t->type()));
    }
    if (auto* t = ty->As<core::type::SampledTexture>()) {
        return ctx.dst->ty.sampled_texture(t->dim(), CreateASTTypeFor(ctx, t->type()));
    }
    if (auto* t = ty->As<core::type::StorageTexture>()) {
        return ctx.dst->ty.storage_texture(t->dim(), t->texel_format(), t->access());
    }
    if (auto* s = ty->As<core::type::Sampler>()) {
        return ctx.dst->ty.sampler(s->kind());
    }
    if (auto* p = ty->As<core::type::Pointer>()) {
        // Note: core::type::Pointer always has an inferred access, but WGSL only allows an explicit
        // access in the 'storage' address space.
        auto address_space = p->AddressSpace();
        auto access =
            address_space == core::AddressSpace::kStorage ? p->Access() : core::Access::kUndefined;
        return ctx.dst->ty.ptr(address_space, CreateASTTypeFor(ctx, p->StoreType()), access);
    }
    TINT_UNREACHABLE() << "Unhandled type: " << ty->TypeInfo().name;
    return Type{};
}

}  // namespace tint::ast::transform
