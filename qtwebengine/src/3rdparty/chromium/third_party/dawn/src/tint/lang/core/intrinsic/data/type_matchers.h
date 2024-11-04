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

#ifndef SRC_TINT_LANG_CORE_INTRINSIC_DATA_TYPE_MATCHERS_H_
#define SRC_TINT_LANG_CORE_INTRINSIC_DATA_TYPE_MATCHERS_H_

#include "src/tint/lang/core/evaluation_stage.h"
#include "src/tint/lang/core/intrinsic/table_data.h"
#include "src/tint/lang/core/type/abstract_float.h"
#include "src/tint/lang/core/type/abstract_int.h"
#include "src/tint/lang/core/type/abstract_numeric.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/bool.h"
#include "src/tint/lang/core/type/builtin_structs.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/external_texture.h"
#include "src/tint/lang/core/type/f16.h"
#include "src/tint/lang/core/type/f32.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/texture_dimension.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/lang/core/type/vector.h"

namespace tint::core::intrinsic::data {

inline bool MatchBool(intrinsic::MatchState&, const type::Type* ty) {
    return ty->IsAnyOf<intrinsic::Any, type::Bool>();
}

inline const type::AbstractFloat* BuildFa(intrinsic::MatchState& state) {
    return state.types.AFloat();
}

inline bool MatchFa(intrinsic::MatchState& state, const type::Type* ty) {
    return (state.earliest_eval_stage <= EvaluationStage::kConstant) &&
           ty->IsAnyOf<intrinsic::Any, core::type::AbstractNumeric>();
}

inline const type::AbstractInt* BuildIa(intrinsic::MatchState& state) {
    return state.types.AInt();
}

inline bool MatchIa(intrinsic::MatchState& state, const type::Type* ty) {
    return (state.earliest_eval_stage <= EvaluationStage::kConstant) &&
           ty->IsAnyOf<intrinsic::Any, core::type::AbstractInt>();
}

inline const type::Bool* BuildBool(intrinsic::MatchState& state) {
    return state.types.bool_();
}

inline const type::F16* BuildF16(intrinsic::MatchState& state) {
    return state.types.f16();
}

inline bool MatchF16(intrinsic::MatchState&, const type::Type* ty) {
    return ty->IsAnyOf<intrinsic::Any, type::F16, type::AbstractNumeric>();
}

inline const type::F32* BuildF32(intrinsic::MatchState& state) {
    return state.types.f32();
}

inline bool MatchF32(intrinsic::MatchState&, const type::Type* ty) {
    return ty->IsAnyOf<intrinsic::Any, type::F32, type::AbstractNumeric>();
}

inline const type::I32* BuildI32(intrinsic::MatchState& state) {
    return state.types.i32();
}

inline bool MatchI32(intrinsic::MatchState&, const type::Type* ty) {
    return ty->IsAnyOf<intrinsic::Any, type::I32, type::AbstractInt>();
}

inline const type::U32* BuildU32(intrinsic::MatchState& state) {
    return state.types.u32();
}

inline bool MatchU32(intrinsic::MatchState&, const type::Type* ty) {
    return ty->IsAnyOf<intrinsic::Any, type::U32, type::AbstractInt>();
}

inline bool MatchVec(intrinsic::MatchState&,
                     const type::Type* ty,
                     intrinsic::Number& N,
                     const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        N = intrinsic::Number::any;
        T = ty;
        return true;
    }

    if (auto* v = ty->As<core::type::Vector>()) {
        N = v->Width();
        T = v->type();
        return true;
    }
    return false;
}

template <uint32_t N>
inline bool MatchVec(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }

    if (auto* v = ty->As<core::type::Vector>()) {
        if (v->Width() == N) {
            T = v->type();
            return true;
        }
    }
    return false;
}

inline const type::Vector* BuildVec(intrinsic::MatchState& state,
                                    intrinsic::Number N,
                                    const type::Type* el) {
    return state.types.vec(el, N.Value());
}

template <uint32_t N>
inline const type::Vector* BuildVec(intrinsic::MatchState& state, const type::Type* el) {
    return state.types.vec(el, N);
}

constexpr auto MatchVec2 = MatchVec<2>;
constexpr auto MatchVec3 = MatchVec<3>;
constexpr auto MatchVec4 = MatchVec<4>;

constexpr auto BuildVec2 = BuildVec<2>;
constexpr auto BuildVec3 = BuildVec<3>;
constexpr auto BuildVec4 = BuildVec<4>;

inline bool MatchPackedVec3(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }

    if (auto* v = ty->As<core::type::Vector>()) {
        if (v->Packed()) {
            T = v->type();
            return true;
        }
    }
    return false;
}

inline const type::Vector* BuildPackedVec3(intrinsic::MatchState& state, const type::Type* el) {
    return state.types.Get<type::Vector>(el, 3u, /* packed */ true);
}

inline bool MatchMat(intrinsic::MatchState&,
                     const type::Type* ty,
                     intrinsic::Number& M,
                     intrinsic::Number& N,
                     const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        M = intrinsic::Number::any;
        N = intrinsic::Number::any;
        T = ty;
        return true;
    }
    if (auto* m = ty->As<core::type::Matrix>()) {
        M = m->columns();
        N = m->ColumnType()->Width();
        T = m->type();
        return true;
    }
    return false;
}

template <uint32_t C, uint32_t R>
inline bool MatchMat(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }
    if (auto* m = ty->As<core::type::Matrix>()) {
        if (m->columns() == C && m->rows() == R) {
            T = m->type();
            return true;
        }
    }
    return false;
}

inline const type::Matrix* BuildMat(intrinsic::MatchState& state,
                                    intrinsic::Number C,
                                    intrinsic::Number R,
                                    const type::Type* T) {
    auto* column_type = state.types.vec(T, R.Value());
    return state.types.mat(column_type, C.Value());
}

template <uint32_t C, uint32_t R>
inline const type::Matrix* BuildMat(intrinsic::MatchState& state, const type::Type* T) {
    auto* column_type = state.types.vec(T, R);
    return state.types.mat(column_type, C);
}

constexpr auto BuildMat2X2 = BuildMat<2, 2>;
constexpr auto BuildMat2X3 = BuildMat<2, 3>;
constexpr auto BuildMat2X4 = BuildMat<2, 4>;
constexpr auto BuildMat3X2 = BuildMat<3, 2>;
constexpr auto BuildMat3X3 = BuildMat<3, 3>;
constexpr auto BuildMat3X4 = BuildMat<3, 4>;
constexpr auto BuildMat4X2 = BuildMat<4, 2>;
constexpr auto BuildMat4X3 = BuildMat<4, 3>;
constexpr auto BuildMat4X4 = BuildMat<4, 4>;

constexpr auto MatchMat2X2 = MatchMat<2, 2>;
constexpr auto MatchMat2X3 = MatchMat<2, 3>;
constexpr auto MatchMat2X4 = MatchMat<2, 4>;
constexpr auto MatchMat3X2 = MatchMat<3, 2>;
constexpr auto MatchMat3X3 = MatchMat<3, 3>;
constexpr auto MatchMat3X4 = MatchMat<3, 4>;
constexpr auto MatchMat4X2 = MatchMat<4, 2>;
constexpr auto MatchMat4X3 = MatchMat<4, 3>;
constexpr auto MatchMat4X4 = MatchMat<4, 4>;

inline bool MatchArray(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }

    if (auto* a = ty->As<core::type::Array>()) {
        if (a->Count()->Is<core::type::RuntimeArrayCount>()) {
            T = a->ElemType();
            return true;
        }
    }
    return false;
}

inline const type::Array* BuildArray(intrinsic::MatchState& state, const type::Type* el) {
    return state.types.Get<type::Array>(el,
                                        /* count */ state.types.Get<type::RuntimeArrayCount>(),
                                        /* align */ 0u,
                                        /* size */ 0u,
                                        /* stride */ 0u,
                                        /* stride_implicit */ 0u);
}

inline bool MatchPtr(intrinsic::MatchState&,
                     const type::Type* ty,
                     intrinsic::Number& S,
                     const type::Type*& T,
                     intrinsic::Number& A) {
    if (ty->Is<intrinsic::Any>()) {
        S = intrinsic::Number::any;
        T = ty;
        A = intrinsic::Number::any;
        return true;
    }

    if (auto* p = ty->As<core::type::Pointer>()) {
        S = intrinsic::Number(static_cast<uint32_t>(p->AddressSpace()));
        T = p->StoreType();
        A = intrinsic::Number(static_cast<uint32_t>(p->Access()));
        return true;
    }
    return false;
}

inline const type::Pointer* BuildPtr(intrinsic::MatchState& state,
                                     intrinsic::Number S,
                                     const type::Type* T,
                                     intrinsic::Number& A) {
    return state.types.ptr(static_cast<core::AddressSpace>(S.Value()), T,
                           static_cast<core::Access>(A.Value()));
}

inline bool MatchAtomic(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }

    if (auto* a = ty->As<core::type::Atomic>()) {
        T = a->Type();
        return true;
    }
    return false;
}

inline const type::Atomic* BuildAtomic(intrinsic::MatchState& state, const type::Type* T) {
    return state.types.atomic(T);
}

inline bool MatchSampler(intrinsic::MatchState&, const type::Type* ty) {
    if (ty->Is<intrinsic::Any>()) {
        return true;
    }
    return ty->Is([](const core::type::Sampler* s) {
        return s->kind() == core::type::SamplerKind::kSampler;
    });
}

inline const type::Sampler* BuildSampler(intrinsic::MatchState& state) {
    return state.types.sampler();
}

inline bool MatchSamplerComparison(intrinsic::MatchState&, const type::Type* ty) {
    if (ty->Is<intrinsic::Any>()) {
        return true;
    }
    return ty->Is([](const core::type::Sampler* s) {
        return s->kind() == core::type::SamplerKind::kComparisonSampler;
    });
}

inline const type::Sampler* BuildSamplerComparison(intrinsic::MatchState& state) {
    return state.types.comparison_sampler();
}

inline bool MatchTexture(intrinsic::MatchState&,
                         const type::Type* ty,
                         type::TextureDimension dim,
                         const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }
    if (auto* v = ty->As<core::type::SampledTexture>()) {
        if (v->dim() == dim) {
            T = v->type();
            return true;
        }
    }
    return false;
}

#define JOIN(a, b) a##b

#define DECLARE_SAMPLED_TEXTURE(suffix, dim)                                                     \
    inline bool JOIN(MatchTexture, suffix)(intrinsic::MatchState & state, const type::Type* ty,  \
                                           const type::Type*& T) {                               \
        return MatchTexture(state, ty, dim, T);                                                  \
    }                                                                                            \
    inline const type::SampledTexture* JOIN(BuildTexture, suffix)(intrinsic::MatchState & state, \
                                                                  const type::Type* T) {         \
        return state.types.Get<type::SampledTexture>(dim, T);                                    \
    }

DECLARE_SAMPLED_TEXTURE(1D, type::TextureDimension::k1d)
DECLARE_SAMPLED_TEXTURE(2D, type::TextureDimension::k2d)
DECLARE_SAMPLED_TEXTURE(2DArray, type::TextureDimension::k2dArray)
DECLARE_SAMPLED_TEXTURE(3D, type::TextureDimension::k3d)
DECLARE_SAMPLED_TEXTURE(Cube, type::TextureDimension::kCube)
DECLARE_SAMPLED_TEXTURE(CubeArray, type::TextureDimension::kCubeArray)
#undef DECLARE_SAMPLED_TEXTURE

inline bool MatchTextureMultisampled(intrinsic::MatchState&,
                                     const type::Type* ty,
                                     type::TextureDimension dim,
                                     const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }
    if (auto* v = ty->As<core::type::MultisampledTexture>()) {
        if (v->dim() == dim) {
            T = v->type();
            return true;
        }
    }
    return false;
}

#define DECLARE_MULTISAMPLED_TEXTURE(suffix, dim)                                    \
    inline bool JOIN(MatchTextureMultisampled, suffix)(                              \
        intrinsic::MatchState & state, const type::Type* ty, const type::Type*& T) { \
        return MatchTextureMultisampled(state, ty, dim, T);                          \
    }                                                                                \
    inline const type::MultisampledTexture* JOIN(BuildTextureMultisampled, suffix)(  \
        intrinsic::MatchState & state, const type::Type* T) {                        \
        return state.types.Get<type::MultisampledTexture>(dim, T);                   \
    }

DECLARE_MULTISAMPLED_TEXTURE(2D, type::TextureDimension::k2d)
#undef DECLARE_MULTISAMPLED_TEXTURE

inline bool MatchTextureDepth(intrinsic::MatchState&,
                              const type::Type* ty,
                              type::TextureDimension dim) {
    if (ty->Is<intrinsic::Any>()) {
        return true;
    }
    return ty->Is([&](const core::type::DepthTexture* t) { return t->dim() == dim; });
}

#define DECLARE_DEPTH_TEXTURE(suffix, dim)                                         \
    inline bool JOIN(MatchTextureDepth, suffix)(intrinsic::MatchState & state,     \
                                                const type::Type* ty) {            \
        return MatchTextureDepth(state, ty, dim);                                  \
    }                                                                              \
    inline const type::DepthTexture* JOIN(BuildTextureDepth,                       \
                                          suffix)(intrinsic::MatchState & state) { \
        return state.types.Get<type::DepthTexture>(dim);                           \
    }

DECLARE_DEPTH_TEXTURE(2D, type::TextureDimension::k2d)
DECLARE_DEPTH_TEXTURE(2DArray, type::TextureDimension::k2dArray)
DECLARE_DEPTH_TEXTURE(Cube, type::TextureDimension::kCube)
DECLARE_DEPTH_TEXTURE(CubeArray, type::TextureDimension::kCubeArray)
#undef DECLARE_DEPTH_TEXTURE

inline bool MatchTextureDepthMultisampled2D(intrinsic::MatchState&, const type::Type* ty) {
    if (ty->Is<intrinsic::Any>()) {
        return true;
    }
    return ty->Is([&](const core::type::DepthMultisampledTexture* t) {
        return t->dim() == core::type::TextureDimension::k2d;
    });
}

inline type::DepthMultisampledTexture* BuildTextureDepthMultisampled2D(
    intrinsic::MatchState& state) {
    return state.types.Get<core::type::DepthMultisampledTexture>(core::type::TextureDimension::k2d);
}

inline bool MatchTextureStorage(intrinsic::MatchState&,
                                const type::Type* ty,
                                type::TextureDimension dim,
                                intrinsic::Number& F,
                                intrinsic::Number& A) {
    if (ty->Is<intrinsic::Any>()) {
        F = intrinsic::Number::any;
        A = intrinsic::Number::any;
        return true;
    }
    if (auto* v = ty->As<core::type::StorageTexture>()) {
        if (v->dim() == dim) {
            F = intrinsic::Number(static_cast<uint32_t>(v->texel_format()));
            A = intrinsic::Number(static_cast<uint32_t>(v->access()));
            return true;
        }
    }
    return false;
}

#define DECLARE_STORAGE_TEXTURE(suffix, dim)                                                  \
    inline bool JOIN(MatchTextureStorage, suffix)(intrinsic::MatchState & state,              \
                                                  const type::Type* ty, intrinsic::Number& F, \
                                                  intrinsic::Number& A) {                     \
        return MatchTextureStorage(state, ty, dim, F, A);                                     \
    }                                                                                         \
    inline const type::StorageTexture* JOIN(BuildTextureStorage, suffix)(                     \
        intrinsic::MatchState & state, intrinsic::Number F, intrinsic::Number A) {            \
        auto format = static_cast<TexelFormat>(F.Value());                                    \
        auto access = static_cast<Access>(A.Value());                                         \
        auto* T = type::StorageTexture::SubtypeFor(format, state.types);                      \
        return state.types.Get<type::StorageTexture>(dim, format, access, T);                 \
    }

DECLARE_STORAGE_TEXTURE(1D, type::TextureDimension::k1d)
DECLARE_STORAGE_TEXTURE(2D, type::TextureDimension::k2d)
DECLARE_STORAGE_TEXTURE(2DArray, type::TextureDimension::k2dArray)
DECLARE_STORAGE_TEXTURE(3D, type::TextureDimension::k3d)
#undef DECLARE_STORAGE_TEXTURE

inline bool MatchTextureExternal(intrinsic::MatchState&, const type::Type* ty) {
    return ty->IsAnyOf<intrinsic::Any, type::ExternalTexture>();
}

inline const type::ExternalTexture* BuildTextureExternal(intrinsic::MatchState& state) {
    return state.types.Get<type::ExternalTexture>();
}

// Builtin types starting with a _ prefix cannot be declared in WGSL, so they
// can only be used as return types. Because of this, they must only match Any,
// which is used as the return type matcher.
inline bool MatchModfResult(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (!ty->Is<intrinsic::Any>()) {
        return false;
    }
    T = ty;
    return true;
}
inline bool MatchModfResultVec(intrinsic::MatchState&,
                               const type::Type* ty,
                               intrinsic::Number& N,
                               const type::Type*& T) {
    if (!ty->Is<intrinsic::Any>()) {
        return false;
    }
    N = intrinsic::Number::any;
    T = ty;
    return true;
}
inline bool MatchFrexpResult(intrinsic::MatchState&, const type::Type* ty, const type::Type*& T) {
    if (!ty->Is<intrinsic::Any>()) {
        return false;
    }
    T = ty;
    return true;
}
inline bool MatchFrexpResultVec(intrinsic::MatchState&,
                                const type::Type* ty,
                                intrinsic::Number& N,
                                const type::Type*& T) {
    if (!ty->Is<intrinsic::Any>()) {
        return false;
    }
    N = intrinsic::Number::any;
    T = ty;
    return true;
}

inline bool MatchAtomicCompareExchangeResult(intrinsic::MatchState&,
                                             const type::Type* ty,
                                             const type::Type*& T) {
    if (ty->Is<intrinsic::Any>()) {
        T = ty;
        return true;
    }
    return false;
}

inline const type::Struct* BuildModfResult(intrinsic::MatchState& state, const type::Type* el) {
    return type::CreateModfResult(state.types, state.symbols, el);
}

inline const type::Struct* BuildModfResultVec(intrinsic::MatchState& state,
                                              intrinsic::Number& n,
                                              const type::Type* el) {
    auto* vec = state.types.vec(el, n.Value());
    return core::type::CreateModfResult(state.types, state.symbols, vec);
}

inline const type::Struct* BuildFrexpResult(intrinsic::MatchState& state, const type::Type* el) {
    return type::CreateFrexpResult(state.types, state.symbols, el);
}

inline const type::Struct* BuildFrexpResultVec(intrinsic::MatchState& state,
                                               intrinsic::Number& n,
                                               const type::Type* el) {
    auto* vec = state.types.vec(el, n.Value());
    return core::type::CreateFrexpResult(state.types, state.symbols, vec);
}

inline const type::Struct* BuildAtomicCompareExchangeResult(intrinsic::MatchState& state,
                                                            const type::Type* ty) {
    return type::CreateAtomicCompareExchangeResult(state.types, state.symbols, ty);
}

}  // namespace tint::core::intrinsic::data

#endif  // SRC_TINT_LANG_CORE_INTRINSIC_DATA_TYPE_MATCHERS_H_
