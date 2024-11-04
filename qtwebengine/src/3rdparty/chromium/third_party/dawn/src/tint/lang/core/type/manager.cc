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

#include "src/tint/lang/core/type/manager.h"

#include <algorithm>

#include "src/tint/lang/core/type/abstract_float.h"
#include "src/tint/lang/core/type/abstract_int.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/bool.h"
#include "src/tint/lang/core/type/f16.h"
#include "src/tint/lang/core/type/f32.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/core/type/void.h"

namespace tint::core::type {

Manager::Manager() = default;

Manager::Manager(Manager&&) = default;

Manager& Manager::operator=(Manager&& rhs) = default;

Manager::~Manager() = default;

const core::type::Void* Manager::void_() {
    return Get<core::type::Void>();
}

const core::type::Bool* Manager::bool_() {
    return Get<core::type::Bool>();
}

const core::type::I32* Manager::i32() {
    return Get<core::type::I32>();
}

const core::type::U32* Manager::u32() {
    return Get<core::type::U32>();
}

const core::type::F32* Manager::f32() {
    return Get<core::type::F32>();
}

const core::type::F16* Manager::f16() {
    return Get<core::type::F16>();
}

const core::type::AbstractFloat* Manager::AFloat() {
    return Get<core::type::AbstractFloat>();
}

const core::type::AbstractInt* Manager::AInt() {
    return Get<core::type::AbstractInt>();
}

const core::type::Atomic* Manager::atomic(const core::type::Type* inner) {
    return Get<core::type::Atomic>(inner);
}

const core::type::Vector* Manager::packed_vec(const core::type::Type* inner, uint32_t size) {
    return Get<core::type::Vector>(inner, size, true);
}

const core::type::Vector* Manager::vec(const core::type::Type* inner, uint32_t size) {
    return Get<core::type::Vector>(inner, size);
}

const core::type::Vector* Manager::vec2(const core::type::Type* inner) {
    return vec(inner, 2);
}

const core::type::Vector* Manager::vec3(const core::type::Type* inner) {
    return vec(inner, 3);
}

const core::type::Vector* Manager::vec4(const core::type::Type* inner) {
    return vec(inner, 4);
}

const core::type::Matrix* Manager::mat(const core::type::Type* inner,
                                       uint32_t cols,
                                       uint32_t rows) {
    return Get<core::type::Matrix>(vec(inner, rows), cols);
}

const core::type::Matrix* Manager::mat(const core::type::Vector* column_type, uint32_t cols) {
    return Get<core::type::Matrix>(column_type, cols);
}

const core::type::Matrix* Manager::mat2x2(const core::type::Type* inner) {
    return mat(inner, 2, 2);
}

const core::type::Matrix* Manager::mat2x3(const core::type::Type* inner) {
    return mat(inner, 2, 3);
}

const core::type::Matrix* Manager::mat2x4(const core::type::Type* inner) {
    return mat(inner, 2, 4);
}

const core::type::Matrix* Manager::mat3x2(const core::type::Type* inner) {
    return mat(inner, 3, 2);
}

const core::type::Matrix* Manager::mat3x3(const core::type::Type* inner) {
    return mat(inner, 3, 3);
}

const core::type::Matrix* Manager::mat3x4(const core::type::Type* inner) {
    return mat(inner, 3, 4);
}

const core::type::Matrix* Manager::mat4x2(const core::type::Type* inner) {
    return mat(inner, 4, 2);
}

const core::type::Matrix* Manager::mat4x3(const core::type::Type* inner) {
    return mat(inner, 4, 3);
}

const core::type::Matrix* Manager::mat4x4(const core::type::Type* inner) {
    return mat(inner, 4, 4);
}

const core::type::Array* Manager::array(const core::type::Type* elem_ty,
                                        uint32_t count,
                                        uint32_t stride /* = 0*/) {
    uint32_t implicit_stride = tint::RoundUp(elem_ty->Align(), elem_ty->Size());
    if (stride == 0) {
        stride = implicit_stride;
    }
    TINT_ASSERT(stride >= implicit_stride);

    return Get<core::type::Array>(/* element type */ elem_ty,
                                  /* element count */ Get<ConstantArrayCount>(count),
                                  /* array alignment */ elem_ty->Align(),
                                  /* array size */ count * stride,
                                  /* element stride */ stride,
                                  /* implicit stride */ implicit_stride);
}

const core::type::Array* Manager::runtime_array(const core::type::Type* elem_ty,
                                                uint32_t stride /* = 0 */) {
    if (stride == 0) {
        stride = elem_ty->Align();
    }
    return Get<core::type::Array>(
        /* element type */ elem_ty,
        /* element count */ Get<RuntimeArrayCount>(),
        /* array alignment */ elem_ty->Align(),
        /* array size */ stride,
        /* element stride */ stride,
        /* implicit stride */ elem_ty->Align());
}

const core::type::Pointer* Manager::ptr(core::AddressSpace address_space,
                                        const core::type::Type* subtype,
                                        core::Access access /* = core::Access::kReadWrite */) {
    return Get<core::type::Pointer>(address_space, subtype, access);
}

core::type::Struct* Manager::Struct(Symbol name, VectorRef<StructMemberDesc> md) {
    tint::Vector<const core::type::StructMember*, 4> members;
    uint32_t current_size = 0u;
    uint32_t max_align = 0u;
    for (const auto& m : md) {
        uint32_t index = static_cast<uint32_t>(members.Length());
        uint32_t align = std::max<uint32_t>(m.type->Align(), 1u);
        uint32_t offset = tint::RoundUp(align, current_size);
        members.Push(Get<core::type::StructMember>(m.name, m.type, index, offset, align,
                                                   m.type->Size(), std::move(m.attributes)));
        current_size = offset + m.type->Size();
        max_align = std::max(max_align, align);
    }
    return Get<core::type::Struct>(name, members, max_align, tint::RoundUp(max_align, current_size),
                                   current_size);
}

}  // namespace tint::core::type
