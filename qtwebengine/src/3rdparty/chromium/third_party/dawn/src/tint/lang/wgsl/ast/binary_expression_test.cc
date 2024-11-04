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

#include "gtest/gtest-spi.h"
#include "src/tint/lang/wgsl/ast/helper_test.h"

namespace tint::ast {
namespace {

using BinaryExpressionTest = TestHelper;

TEST_F(BinaryExpressionTest, Creation) {
    auto* lhs = Expr("lhs");
    auto* rhs = Expr("rhs");

    auto* r = create<BinaryExpression>(core::BinaryOp::kEqual, lhs, rhs);
    EXPECT_EQ(r->lhs, lhs);
    EXPECT_EQ(r->rhs, rhs);
    EXPECT_EQ(r->op, core::BinaryOp::kEqual);
}

TEST_F(BinaryExpressionTest, Creation_WithSource) {
    auto* lhs = Expr("lhs");
    auto* rhs = Expr("rhs");

    auto* r =
        create<BinaryExpression>(Source{Source::Location{20, 2}}, core::BinaryOp::kEqual, lhs, rhs);
    auto src = r->source;
    EXPECT_EQ(src.range.begin.line, 20u);
    EXPECT_EQ(src.range.begin.column, 2u);
}

TEST_F(BinaryExpressionTest, IsBinary) {
    auto* lhs = Expr("lhs");
    auto* rhs = Expr("rhs");

    auto* r = create<BinaryExpression>(core::BinaryOp::kEqual, lhs, rhs);
    EXPECT_TRUE(r->Is<BinaryExpression>());
}

TEST_F(BinaryExpressionTest, Assert_Null_LHS) {
    EXPECT_FATAL_FAILURE(
        {
            ProgramBuilder b;
            b.create<BinaryExpression>(core::BinaryOp::kEqual, nullptr, b.Expr("rhs"));
        },
        "internal compiler error");
}

TEST_F(BinaryExpressionTest, Assert_Null_RHS) {
    EXPECT_FATAL_FAILURE(
        {
            ProgramBuilder b;
            b.create<BinaryExpression>(core::BinaryOp::kEqual, b.Expr("lhs"), nullptr);
        },
        "internal compiler error");
}

TEST_F(BinaryExpressionTest, Assert_DifferentGenerationID_LHS) {
    EXPECT_FATAL_FAILURE(
        {
            ProgramBuilder b1;
            ProgramBuilder b2;
            b1.create<BinaryExpression>(core::BinaryOp::kEqual, b2.Expr("lhs"), b1.Expr("rhs"));
        },
        "internal compiler error");
}

TEST_F(BinaryExpressionTest, Assert_DifferentGenerationID_RHS) {
    EXPECT_FATAL_FAILURE(
        {
            ProgramBuilder b1;
            ProgramBuilder b2;
            b1.create<BinaryExpression>(core::BinaryOp::kEqual, b1.Expr("lhs"), b2.Expr("rhs"));
        },
        "internal compiler error");
}

}  // namespace
}  // namespace tint::ast
