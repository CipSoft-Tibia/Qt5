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
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

#include "gmock/gmock.h"

namespace tint::resolver {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

// Helpers and typedefs
template <typename T>
using DataType = builder::DataType<T>;
template <typename T>
using alias = builder::alias<T>;

struct ResolverInferredTypeTest : public resolver::TestHelper, public testing::Test {};

struct Params {
    // builder::ast_expr_func_ptr_default_arg create_value;
    builder::ast_expr_from_double_func_ptr create_value;
    builder::sem_type_func_ptr create_expected_type;
};

template <typename T>
constexpr Params ParamsFor() {
    // return Params{builder::CreateExprWithDefaultArg<T>(), DataType<T>::Sem};
    return Params{DataType<T>::ExprFromDouble, DataType<T>::Sem};
}

Params all_cases[] = {
    ParamsFor<bool>(),                //
    ParamsFor<u32>(),                 //
    ParamsFor<i32>(),                 //
    ParamsFor<f32>(),                 //
    ParamsFor<vec3<bool>>(),          //
    ParamsFor<vec3<i32>>(),           //
    ParamsFor<vec3<u32>>(),           //
    ParamsFor<vec3<f32>>(),           //
    ParamsFor<mat3x3<f32>>(),         //
    ParamsFor<alias<bool>>(),         //
    ParamsFor<alias<u32>>(),          //
    ParamsFor<alias<i32>>(),          //
    ParamsFor<alias<f32>>(),          //
    ParamsFor<alias<vec3<bool>>>(),   //
    ParamsFor<alias<vec3<i32>>>(),    //
    ParamsFor<alias<vec3<u32>>>(),    //
    ParamsFor<alias<vec3<f32>>>(),    //
    ParamsFor<alias<mat3x3<f32>>>(),  //
};

using ResolverInferredTypeParamTest = ResolverTestWithParam<Params>;

TEST_P(ResolverInferredTypeParamTest, GlobalConst_Pass) {
    auto& params = GetParam();

    auto* expected_type = params.create_expected_type(*this);

    // const a = <type initializer>;
    auto* ctor_expr = params.create_value(*this, 0);
    auto* a = GlobalConst("a", ctor_expr);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(TypeOf(a), expected_type);
}

TEST_P(ResolverInferredTypeParamTest, GlobalVar_Pass) {
    auto& params = GetParam();

    auto* expected_type = params.create_expected_type(*this);

    // var a = <type initializer>;
    auto* ctor_expr = params.create_value(*this, 0);
    auto* var = GlobalVar("a", core::AddressSpace::kPrivate, ctor_expr);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(TypeOf(var)->UnwrapRef(), expected_type);
}

TEST_P(ResolverInferredTypeParamTest, LocalLet_Pass) {
    auto& params = GetParam();

    auto* expected_type = params.create_expected_type(*this);

    // let a = <type initializer>;
    auto* ctor_expr = params.create_value(*this, 0);
    auto* var = Let("a", ctor_expr);
    WrapInFunction(var);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(TypeOf(var), expected_type);
}

TEST_P(ResolverInferredTypeParamTest, LocalVar_Pass) {
    auto& params = GetParam();

    auto* expected_type = params.create_expected_type(*this);

    // var a = <type initializer>;
    auto* ctor_expr = params.create_value(*this, 0);
    auto* var = Var("a", core::AddressSpace::kFunction, ctor_expr);
    WrapInFunction(var);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(TypeOf(var)->UnwrapRef(), expected_type);
}

INSTANTIATE_TEST_SUITE_P(ResolverTest, ResolverInferredTypeParamTest, testing::ValuesIn(all_cases));

TEST_F(ResolverInferredTypeTest, InferArray_Pass) {
    auto type = ty.array<u32, 10>();
    auto* expected_type = create<core::type::Array>(create<core::type::U32>(),
                                                    create<core::type::ConstantArrayCount>(10u), 4u,
                                                    4u * 10u, 4u, 4u);

    auto* ctor_expr = Call(type);
    auto* var = Var("a", core::AddressSpace::kFunction, ctor_expr);
    WrapInFunction(var);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(TypeOf(var)->UnwrapRef(), expected_type);
}

TEST_F(ResolverInferredTypeTest, InferStruct_Pass) {
    auto* member = Member("x", ty.i32());
    auto* str = Structure("S", Vector{member});

    auto* expected_type = create<sem::Struct>(
        str, str->name->symbol,
        Vector{create<sem::StructMember>(member, member->name->symbol, create<core::type::I32>(),
                                         0u, 0u, 0u, 4u, core::type::StructMemberAttributes{})},
        0u, 4u, 4u);

    auto* ctor_expr = Call(ty.Of(str));

    auto* var = Var("a", core::AddressSpace::kFunction, ctor_expr);
    WrapInFunction(var);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(TypeOf(var)->UnwrapRef(), expected_type);
}

}  // namespace
}  // namespace tint::resolver
