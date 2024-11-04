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
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"
#include "src/tint/lang/wgsl/sem/struct.h"

using ::testing::UnorderedElementsAre;

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::resolver {
namespace {

using ResolverAddressSpaceUseTest = ResolverTest;

TEST_F(ResolverAddressSpaceUseTest, UnreachableStruct) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_TRUE(sem->AddressSpaceUsage().empty());
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableFromParameter) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});

    Func("f", Vector{Param("param", ty.Of(s))}, ty.void_(), tint::Empty, tint::Empty);

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kUndefined));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableFromReturnType) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});

    Func("f", tint::Empty, ty.Of(s), Vector{Return(Call(ty.Of(s)))}, tint::Empty);

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kUndefined));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableFromGlobal) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});

    GlobalVar("g", ty.Of(s), core::AddressSpace::kPrivate);

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kPrivate));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableViaGlobalAlias) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    auto* a = Alias("A", ty.Of(s));
    GlobalVar("g", ty.Of(a), core::AddressSpace::kPrivate);

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kPrivate));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableViaGlobalStruct) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    auto* o = Structure("O", Vector{Member("a", ty.Of(s))});
    GlobalVar("g", ty.Of(o), core::AddressSpace::kPrivate);

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kPrivate));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableViaGlobalArray) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    auto a = ty.array(ty.Of(s), 3_u);
    GlobalVar("g", a, core::AddressSpace::kPrivate);

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kPrivate));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableFromLocal) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});

    WrapInFunction(Var("g", ty.Of(s)));

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kFunction));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableViaLocalAlias) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    auto* a = Alias("A", ty.Of(s));
    WrapInFunction(Var("g", ty.Of(a)));

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kFunction));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableViaLocalStruct) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    auto* o = Structure("O", Vector{Member("a", ty.Of(s))});
    WrapInFunction(Var("g", ty.Of(o)));

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kFunction));
}

TEST_F(ResolverAddressSpaceUseTest, StructReachableViaLocalArray) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    auto a = ty.array(ty.Of(s), 3_u);
    WrapInFunction(Var("g", a));

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(), UnorderedElementsAre(core::AddressSpace::kFunction));
}

TEST_F(ResolverAddressSpaceUseTest, StructMultipleAddressSpaceUses) {
    auto* s = Structure("S", Vector{Member("a", ty.f32())});
    GlobalVar("x", ty.Of(s), core::AddressSpace::kUniform, Binding(0_a), Group(0_a));
    GlobalVar("y", ty.Of(s), core::AddressSpace::kStorage, core::Access::kRead, Binding(1_a),
              Group(0_a));
    WrapInFunction(Var("g", ty.Of(s)));

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem = TypeOf(s)->As<core::type::Struct>();
    ASSERT_NE(sem, nullptr);
    EXPECT_THAT(sem->AddressSpaceUsage(),
                UnorderedElementsAre(core::AddressSpace::kUniform, core::AddressSpace::kStorage,
                                     core::AddressSpace::kFunction));
}

}  // namespace
}  // namespace tint::resolver
