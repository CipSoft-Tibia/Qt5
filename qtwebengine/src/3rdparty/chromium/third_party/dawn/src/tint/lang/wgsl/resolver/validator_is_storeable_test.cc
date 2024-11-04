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

#include "src/tint/lang/wgsl/resolver/validator.h"

#include "gmock/gmock.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

namespace tint::resolver {
namespace {

using ValidatorIsStorableTest = ResolverTest;

TEST_F(ValidatorIsStorableTest, Void) {
    EXPECT_FALSE(v()->IsStorable(create<core::type::Void>()));
}

TEST_F(ValidatorIsStorableTest, Scalar) {
    EXPECT_TRUE(v()->IsStorable(create<core::type::Bool>()));
    EXPECT_TRUE(v()->IsStorable(create<core::type::I32>()));
    EXPECT_TRUE(v()->IsStorable(create<core::type::U32>()));
    EXPECT_TRUE(v()->IsStorable(create<core::type::F32>()));
    EXPECT_TRUE(v()->IsStorable(create<core::type::F16>()));
}

TEST_F(ValidatorIsStorableTest, Vector) {
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::I32>(), 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::I32>(), 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::I32>(), 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::U32>(), 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::U32>(), 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::U32>(), 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::F32>(), 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::F32>(), 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::F32>(), 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::F16>(), 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::F16>(), 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Vector>(create<core::type::F16>(), 4u)));
}

TEST_F(ValidatorIsStorableTest, Matrix) {
    auto* vec2_f32 = create<core::type::Vector>(create<core::type::F32>(), 2u);
    auto* vec3_f32 = create<core::type::Vector>(create<core::type::F32>(), 3u);
    auto* vec4_f32 = create<core::type::Vector>(create<core::type::F32>(), 4u);
    auto* vec2_f16 = create<core::type::Vector>(create<core::type::F16>(), 2u);
    auto* vec3_f16 = create<core::type::Vector>(create<core::type::F16>(), 3u);
    auto* vec4_f16 = create<core::type::Vector>(create<core::type::F16>(), 4u);
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec2_f32, 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec2_f32, 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec2_f32, 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec3_f32, 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec3_f32, 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec3_f32, 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec4_f32, 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec4_f32, 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec4_f32, 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec2_f16, 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec2_f16, 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec2_f16, 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec3_f16, 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec3_f16, 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec3_f16, 4u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec4_f16, 2u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec4_f16, 3u)));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Matrix>(vec4_f16, 4u)));
}

TEST_F(ValidatorIsStorableTest, Pointer) {
    auto* ptr = create<core::type::Pointer>(core::AddressSpace::kPrivate, create<core::type::I32>(),
                                            core::Access::kReadWrite);
    EXPECT_FALSE(v()->IsStorable(ptr));
}

TEST_F(ValidatorIsStorableTest, Atomic) {
    EXPECT_TRUE(v()->IsStorable(create<core::type::Atomic>(create<core::type::I32>())));
    EXPECT_TRUE(v()->IsStorable(create<core::type::Atomic>(create<core::type::U32>())));
}

TEST_F(ValidatorIsStorableTest, ArraySizedOfStorable) {
    auto* arr = create<core::type::Array>(
        create<core::type::I32>(), create<core::type::ConstantArrayCount>(5u), 4u, 20u, 4u, 4u);
    EXPECT_TRUE(v()->IsStorable(arr));
}

TEST_F(ValidatorIsStorableTest, ArrayUnsizedOfStorable) {
    auto* arr = create<core::type::Array>(create<core::type::I32>(),
                                          create<core::type::RuntimeArrayCount>(), 4u, 4u, 4u, 4u);
    EXPECT_TRUE(v()->IsStorable(arr));
}

}  // namespace
}  // namespace tint::resolver
