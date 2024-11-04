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

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/msl/writer/ast_printer/helper_test.h"
#include "src/tint/utils/text/string_stream.h"

using namespace tint::core::fluent_types;  // NOLINT

namespace tint::msl::writer {
namespace {

struct BinaryData {
    const char* result;
    core::BinaryOp op;
};
inline std::ostream& operator<<(std::ostream& out, BinaryData data) {
    StringStream str;
    str << data.op;
    out << str.str();
    return out;
}
using MslBinaryTest = TestParamHelper<BinaryData>;
TEST_P(MslBinaryTest, Emit) {
    auto params = GetParam();

    auto type = [&] {
        return ((params.op == core::BinaryOp::kLogicalAnd) ||
                (params.op == core::BinaryOp::kLogicalOr))
                   ? ty.bool_()
                   : ty.u32();
    };

    auto* left = Var("left", type());
    auto* right = Var("right", type());

    auto* expr = create<ast::BinaryExpression>(params.op, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), params.result);
}
INSTANTIATE_TEST_SUITE_P(
    MslASTPrinterTest,
    MslBinaryTest,
    testing::Values(BinaryData{"(left & right)", core::BinaryOp::kAnd},
                    BinaryData{"(left | right)", core::BinaryOp::kOr},
                    BinaryData{"(left ^ right)", core::BinaryOp::kXor},
                    BinaryData{"(left && right)", core::BinaryOp::kLogicalAnd},
                    BinaryData{"(left || right)", core::BinaryOp::kLogicalOr},
                    BinaryData{"(left == right)", core::BinaryOp::kEqual},
                    BinaryData{"(left != right)", core::BinaryOp::kNotEqual},
                    BinaryData{"(left < right)", core::BinaryOp::kLessThan},
                    BinaryData{"(left > right)", core::BinaryOp::kGreaterThan},
                    BinaryData{"(left <= right)", core::BinaryOp::kLessThanEqual},
                    BinaryData{"(left >= right)", core::BinaryOp::kGreaterThanEqual},
                    BinaryData{"(left << right)", core::BinaryOp::kShiftLeft},
                    BinaryData{"(left >> right)", core::BinaryOp::kShiftRight},
                    BinaryData{"(left + right)", core::BinaryOp::kAdd},
                    BinaryData{"(left - right)", core::BinaryOp::kSubtract},
                    BinaryData{"(left * right)", core::BinaryOp::kMultiply},
                    BinaryData{"(left / right)", core::BinaryOp::kDivide},
                    BinaryData{"(left % right)", core::BinaryOp::kModulo}));

using MslBinaryTest_SignedOverflowDefinedBehaviour = TestParamHelper<BinaryData>;
TEST_P(MslBinaryTest_SignedOverflowDefinedBehaviour, Emit) {
    auto params = GetParam();

    auto a_type = ty.i32();
    auto b_type =
        (params.op == core::BinaryOp::kShiftLeft || params.op == core::BinaryOp::kShiftRight)
            ? ty.u32()
            : ty.i32();

    auto* a = Var("a", a_type);
    auto* b = Var("b", b_type);

    auto* expr = create<ast::BinaryExpression>(params.op, Expr(a), Expr(b));
    WrapInFunction(a, b, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), params.result);
}
using Op = core::BinaryOp;
constexpr BinaryData signed_overflow_defined_behaviour_cases[] = {
    {"as_type<int>((as_type<uint>(a) << b))", Op::kShiftLeft},
    {"(a >> b)", Op::kShiftRight},
    {"as_type<int>((as_type<uint>(a) + as_type<uint>(b)))", Op::kAdd},
    {"as_type<int>((as_type<uint>(a) - as_type<uint>(b)))", Op::kSubtract},
    {"as_type<int>((as_type<uint>(a) * as_type<uint>(b)))", Op::kMultiply}};
INSTANTIATE_TEST_SUITE_P(MslASTPrinterTest,
                         MslBinaryTest_SignedOverflowDefinedBehaviour,
                         testing::ValuesIn(signed_overflow_defined_behaviour_cases));

using MslBinaryTest_SignedOverflowDefinedBehaviour_Chained = TestParamHelper<BinaryData>;
TEST_P(MslBinaryTest_SignedOverflowDefinedBehaviour_Chained, Emit) {
    auto params = GetParam();

    auto a_type = ty.i32();
    auto b_type =
        (params.op == core::BinaryOp::kShiftLeft || params.op == core::BinaryOp::kShiftRight)
            ? ty.u32()
            : ty.i32();

    auto* a = Var("a", a_type);
    auto* b = Var("b", b_type);

    auto* expr1 = create<ast::BinaryExpression>(params.op, Expr(a), Expr(b));
    auto* expr2 = create<ast::BinaryExpression>(params.op, expr1, Expr(b));
    WrapInFunction(a, b, expr2);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr2)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), params.result);
}
using Op = core::BinaryOp;
constexpr BinaryData signed_overflow_defined_behaviour_chained_cases[] = {
    {R"(as_type<int>((as_type<uint>(as_type<int>((as_type<uint>(a) << b))) << b)))",
     Op::kShiftLeft},
    {R"(((a >> b) >> b))", Op::kShiftRight},
    {R"(as_type<int>((as_type<uint>(as_type<int>((as_type<uint>(a) + as_type<uint>(b)))) + as_type<uint>(b))))",
     Op::kAdd},
    {R"(as_type<int>((as_type<uint>(as_type<int>((as_type<uint>(a) - as_type<uint>(b)))) - as_type<uint>(b))))",
     Op::kSubtract},
    {R"(as_type<int>((as_type<uint>(as_type<int>((as_type<uint>(a) * as_type<uint>(b)))) * as_type<uint>(b))))",
     Op::kMultiply}};
INSTANTIATE_TEST_SUITE_P(MslASTPrinterTest,
                         MslBinaryTest_SignedOverflowDefinedBehaviour_Chained,
                         testing::ValuesIn(signed_overflow_defined_behaviour_chained_cases));

TEST_F(MslBinaryTest, ModF32) {
    auto* left = Var("left", ty.f32());
    auto* right = Var("right", ty.f32());
    auto* expr = create<ast::BinaryExpression>(core::BinaryOp::kModulo, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "fmod(left, right)");
}

TEST_F(MslBinaryTest, ModF16) {
    Enable(core::Extension::kF16);

    auto* left = Var("left", ty.f16());
    auto* right = Var("right", ty.f16());
    auto* expr = create<ast::BinaryExpression>(core::BinaryOp::kModulo, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "fmod(left, right)");
}

TEST_F(MslBinaryTest, ModVec3F32) {
    auto* left = Var("left", ty.vec3<f32>());
    auto* right = Var("right", ty.vec3<f32>());
    auto* expr = create<ast::BinaryExpression>(core::BinaryOp::kModulo, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "fmod(left, right)");
}

TEST_F(MslBinaryTest, ModVec3F16) {
    Enable(core::Extension::kF16);

    auto* left = Var("left", ty.vec3<f16>());
    auto* right = Var("right", ty.vec3<f16>());
    auto* expr = create<ast::BinaryExpression>(core::BinaryOp::kModulo, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "fmod(left, right)");
}

TEST_F(MslBinaryTest, BoolAnd) {
    auto* left = Var("left", Expr(true));
    auto* right = Var("right", Expr(false));
    auto* expr = create<ast::BinaryExpression>(core::BinaryOp::kAnd, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "bool(left & right)");
}

TEST_F(MslBinaryTest, BoolOr) {
    auto* left = Var("left", Expr(true));
    auto* right = Var("right", Expr(false));
    auto* expr = create<ast::BinaryExpression>(core::BinaryOp::kOr, Expr(left), Expr(right));
    WrapInFunction(left, right, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "bool(left | right)");
}

}  // namespace
}  // namespace tint::msl::writer
