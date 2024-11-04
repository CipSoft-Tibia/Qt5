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

#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/wgsl/resolver/resolver.h"
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

#include "gmock/gmock.h"

namespace tint::resolver {
namespace {

using namespace tint::core::number_suffixes;  // NOLINT

struct ResolverAtomicTest : public resolver::TestHelper, public testing::Test {};

TEST_F(ResolverAtomicTest, GlobalWorkgroupI32) {
    auto* g = GlobalVar("a", ty.atomic(Source{{12, 34}}, ty.i32()), core::AddressSpace::kWorkgroup);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    ASSERT_TRUE(TypeOf(g)->Is<core::type::Reference>());
    auto* atomic = TypeOf(g)->UnwrapRef()->As<core::type::Atomic>();
    ASSERT_NE(atomic, nullptr);
    EXPECT_TRUE(atomic->Type()->Is<core::type::I32>());
}

TEST_F(ResolverAtomicTest, GlobalWorkgroupU32) {
    auto* g = GlobalVar("a", ty.atomic(Source{{12, 34}}, ty.u32()), core::AddressSpace::kWorkgroup);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    ASSERT_TRUE(TypeOf(g)->Is<core::type::Reference>());
    auto* atomic = TypeOf(g)->UnwrapRef()->As<core::type::Atomic>();
    ASSERT_NE(atomic, nullptr);
    EXPECT_TRUE(atomic->Type()->Is<core::type::U32>());
}

TEST_F(ResolverAtomicTest, GlobalStorageStruct) {
    auto* s = Structure("s", Vector{Member("a", ty.atomic(Source{{12, 34}}, ty.i32()))});
    auto* g = GlobalVar("g", ty.Of(s), core::AddressSpace::kStorage, core::Access::kReadWrite,
                        Binding(0_a), Group(0_a));

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    ASSERT_TRUE(TypeOf(g)->Is<core::type::Reference>());
    auto* str = TypeOf(g)->UnwrapRef()->As<sem::Struct>();
    ASSERT_NE(str, nullptr);
    ASSERT_EQ(str->Members().Length(), 1u);
    auto* atomic = str->Members()[0]->Type()->As<core::type::Atomic>();
    ASSERT_NE(atomic, nullptr);
    ASSERT_TRUE(atomic->Type()->Is<core::type::I32>());
}

}  // namespace
}  // namespace tint::resolver
