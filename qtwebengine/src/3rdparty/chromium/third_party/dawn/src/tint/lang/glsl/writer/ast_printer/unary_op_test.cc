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

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/glsl/writer/ast_printer/helper_test.h"
#include "src/tint/utils/text/string_stream.h"

#include "gmock/gmock.h"

using namespace tint::core::fluent_types;  // NOLINT

namespace tint::glsl::writer {
namespace {

using GlslUnaryOpTest = TestHelper;

TEST_F(GlslUnaryOpTest, AddressOf) {
    GlobalVar("expr", ty.f32(), core::AddressSpace::kPrivate);
    auto* op = create<ast::UnaryOpExpression>(core::UnaryOp::kAddressOf, Expr("expr"));
    WrapInFunction(op);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, op);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "expr");
}

TEST_F(GlslUnaryOpTest, Complement) {
    GlobalVar("expr", ty.u32(), core::AddressSpace::kPrivate);
    auto* op = create<ast::UnaryOpExpression>(core::UnaryOp::kComplement, Expr("expr"));
    WrapInFunction(op);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, op);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "~(expr)");
}

TEST_F(GlslUnaryOpTest, Indirection) {
    GlobalVar("G", ty.f32(), core::AddressSpace::kPrivate);
    auto* p = Let("expr", create<ast::UnaryOpExpression>(core::UnaryOp::kAddressOf, Expr("G")));
    auto* op = create<ast::UnaryOpExpression>(core::UnaryOp::kIndirection, Expr("expr"));
    WrapInFunction(p, op);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, op);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "expr");
}

TEST_F(GlslUnaryOpTest, Not) {
    GlobalVar("expr", ty.bool_(), core::AddressSpace::kPrivate);
    auto* op = create<ast::UnaryOpExpression>(core::UnaryOp::kNot, Expr("expr"));
    WrapInFunction(op);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, op);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "!(expr)");
}

TEST_F(GlslUnaryOpTest, Negation) {
    GlobalVar("expr", ty.i32(), core::AddressSpace::kPrivate);
    auto* op = create<ast::UnaryOpExpression>(core::UnaryOp::kNegation, Expr("expr"));
    WrapInFunction(op);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, op);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "-(expr)");
}

TEST_F(GlslUnaryOpTest, IntMin) {
    auto* op = Expr(i32(std::numeric_limits<int32_t>::min()));
    WrapInFunction(op);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, op);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "(-2147483647 - 1)");
}

}  // namespace
}  // namespace tint::glsl::writer
