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

#include "src/tint/lang/wgsl/resolver/resolver.h"

#include "gmock/gmock.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

namespace tint::resolver {
namespace {

using ResolverIsHostShareable = ResolverTest;

TEST_F(ResolverIsHostShareable, Void) {
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Void>()));
}

TEST_F(ResolverIsHostShareable, Bool) {
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Bool>()));
}

TEST_F(ResolverIsHostShareable, NumericScalar) {
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::I32>()));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::U32>()));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::F32>()));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::F16>()));
}

TEST_F(ResolverIsHostShareable, NumericVector) {
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::I32>(), 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::I32>(), 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::I32>(), 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::U32>(), 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::U32>(), 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::U32>(), 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::F32>(), 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::F32>(), 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::F32>(), 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::F16>(), 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::F16>(), 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::F16>(), 4u)));
}

TEST_F(ResolverIsHostShareable, BoolVector) {
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 2u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 3u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 4u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 2u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 3u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 4u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 2u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 3u)));
    EXPECT_FALSE(r()->IsHostShareable(create<core::type::Vector>(create<core::type::Bool>(), 4u)));
}

TEST_F(ResolverIsHostShareable, Matrix) {
    auto* vec2_f32 = create<core::type::Vector>(create<core::type::F32>(), 2u);
    auto* vec3_f32 = create<core::type::Vector>(create<core::type::F32>(), 3u);
    auto* vec4_f32 = create<core::type::Vector>(create<core::type::F32>(), 4u);
    auto* vec2_f16 = create<core::type::Vector>(create<core::type::F16>(), 2u);
    auto* vec3_f16 = create<core::type::Vector>(create<core::type::F16>(), 3u);
    auto* vec4_f16 = create<core::type::Vector>(create<core::type::F16>(), 4u);

    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec2_f32, 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec2_f32, 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec2_f32, 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec3_f32, 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec3_f32, 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec3_f32, 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec4_f32, 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec4_f32, 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec4_f32, 4u)));

    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec2_f16, 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec2_f16, 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec2_f16, 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec3_f16, 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec3_f16, 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec3_f16, 4u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec4_f16, 2u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec4_f16, 3u)));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Matrix>(vec4_f16, 4u)));
}

TEST_F(ResolverIsHostShareable, Pointer) {
    auto* ptr = create<core::type::Pointer>(core::AddressSpace::kPrivate, create<core::type::I32>(),
                                            core::Access::kReadWrite);
    EXPECT_FALSE(r()->IsHostShareable(ptr));
}

TEST_F(ResolverIsHostShareable, Atomic) {
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Atomic>(create<core::type::I32>())));
    EXPECT_TRUE(r()->IsHostShareable(create<core::type::Atomic>(create<core::type::U32>())));
}

TEST_F(ResolverIsHostShareable, ArraySizedOfHostShareable) {
    auto* arr = create<core::type::Array>(
        create<core::type::I32>(), create<core::type::ConstantArrayCount>(5u), 4u, 20u, 4u, 4u);
    EXPECT_TRUE(r()->IsHostShareable(arr));
}

TEST_F(ResolverIsHostShareable, ArrayUnsizedOfHostShareable) {
    auto* arr = create<core::type::Array>(create<core::type::I32>(),
                                          create<core::type::RuntimeArrayCount>(), 4u, 4u, 4u, 4u);
    EXPECT_TRUE(r()->IsHostShareable(arr));
}

// Note: Structure tests covered in host_shareable_validation_test.cc

}  // namespace
}  // namespace tint::resolver
