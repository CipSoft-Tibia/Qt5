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

#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/core/address_space.h"
#include "src/tint/lang/core/type/helper_test.h"

namespace tint::core::type {
namespace {

using ReferenceTest = TestHelper;

TEST_F(ReferenceTest, Creation) {
    auto* a =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);
    auto* b =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);
    auto* c =
        create<Reference>(core::AddressSpace::kStorage, create<F32>(), core::Access::kReadWrite);
    auto* d =
        create<Reference>(core::AddressSpace::kPrivate, create<I32>(), core::Access::kReadWrite);
    auto* e = create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kRead);

    EXPECT_TRUE(a->StoreType()->Is<I32>());
    EXPECT_EQ(a->AddressSpace(), core::AddressSpace::kStorage);
    EXPECT_EQ(a->Access(), core::Access::kReadWrite);

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
    EXPECT_NE(a, e);
}

TEST_F(ReferenceTest, Hash) {
    auto* a =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);
    auto* b =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);

    EXPECT_EQ(a->unique_hash, b->unique_hash);
}

TEST_F(ReferenceTest, Equals) {
    auto* a =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);
    auto* b =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);
    auto* c =
        create<Reference>(core::AddressSpace::kStorage, create<F32>(), core::Access::kReadWrite);
    auto* d =
        create<Reference>(core::AddressSpace::kPrivate, create<I32>(), core::Access::kReadWrite);
    auto* e = create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kRead);

    EXPECT_TRUE(a->Equals(*b));
    EXPECT_FALSE(a->Equals(*c));
    EXPECT_FALSE(a->Equals(*d));
    EXPECT_FALSE(a->Equals(*e));
    EXPECT_FALSE(a->Equals(Void{}));
}

TEST_F(ReferenceTest, FriendlyName) {
    auto* r = create<Reference>(core::AddressSpace::kUndefined, create<I32>(), core::Access::kRead);
    EXPECT_EQ(r->FriendlyName(), "ref<i32, read>");
}

TEST_F(ReferenceTest, FriendlyNameWithAddressSpace) {
    auto* r = create<Reference>(core::AddressSpace::kWorkgroup, create<I32>(), core::Access::kRead);
    EXPECT_EQ(r->FriendlyName(), "ref<workgroup, i32, read>");
}

TEST_F(ReferenceTest, Clone) {
    auto* a =
        create<Reference>(core::AddressSpace::kStorage, create<I32>(), core::Access::kReadWrite);

    core::type::Manager mgr;
    core::type::CloneContext ctx{{nullptr}, {nullptr, &mgr}};

    auto* ref = a->Clone(ctx);
    EXPECT_TRUE(ref->StoreType()->Is<I32>());
    EXPECT_EQ(ref->AddressSpace(), core::AddressSpace::kStorage);
    EXPECT_EQ(ref->Access(), core::Access::kReadWrite);
}

}  // namespace
}  // namespace tint::core::type
