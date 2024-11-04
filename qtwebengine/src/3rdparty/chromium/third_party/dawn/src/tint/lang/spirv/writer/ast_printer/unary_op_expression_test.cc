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
#include "src/tint/lang/spirv/writer/ast_printer/helper_test.h"
#include "src/tint/lang/spirv/writer/common/spv_dump_test.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::spirv::writer {
namespace {

using SpirvASTPrinterTest = TestHelper;

TEST_F(SpirvASTPrinterTest, UnaryOp_Negation_Integer) {
    auto* expr = create<ast::UnaryOpExpression>(core::UnaryOp::kNegation, Expr(1_i));
    WrapInFunction(expr);

    Builder& b = Build();

    b.PushFunctionForTesting();
    EXPECT_EQ(b.GenerateUnaryOpExpression(expr), 1u) << b.Diagnostics();
    EXPECT_EQ(DumpInstructions(b.Module().Types()), R"(%2 = OpTypeInt 32 1
%3 = OpConstant %2 1
)");
    EXPECT_EQ(DumpInstructions(b.CurrentFunction().instructions()),
              R"(%1 = OpSNegate %2 %3
)");
}

TEST_F(SpirvASTPrinterTest, UnaryOp_Negation_Float) {
    auto* expr = create<ast::UnaryOpExpression>(core::UnaryOp::kNegation, Expr(1_f));
    WrapInFunction(expr);

    Builder& b = Build();

    b.PushFunctionForTesting();
    EXPECT_EQ(b.GenerateUnaryOpExpression(expr), 1u) << b.Diagnostics();
    EXPECT_EQ(DumpInstructions(b.Module().Types()), R"(%2 = OpTypeFloat 32
%3 = OpConstant %2 1
)");
    EXPECT_EQ(DumpInstructions(b.CurrentFunction().instructions()),
              R"(%1 = OpFNegate %2 %3
)");
}

TEST_F(SpirvASTPrinterTest, UnaryOp_Complement) {
    auto* expr = create<ast::UnaryOpExpression>(core::UnaryOp::kComplement, Expr(1_i));
    WrapInFunction(expr);

    Builder& b = Build();

    b.PushFunctionForTesting();
    EXPECT_EQ(b.GenerateUnaryOpExpression(expr), 1u) << b.Diagnostics();
    EXPECT_EQ(DumpInstructions(b.Module().Types()), R"(%2 = OpTypeInt 32 1
%3 = OpConstant %2 1
)");
    EXPECT_EQ(DumpInstructions(b.CurrentFunction().instructions()),
              R"(%1 = OpNot %2 %3
)");
}

TEST_F(SpirvASTPrinterTest, UnaryOp_Not) {
    auto* expr = create<ast::UnaryOpExpression>(core::UnaryOp::kNot, Expr(false));
    WrapInFunction(expr);

    Builder& b = Build();

    b.PushFunctionForTesting();
    EXPECT_EQ(b.GenerateUnaryOpExpression(expr), 1u) << b.Diagnostics();
    EXPECT_EQ(DumpInstructions(b.Module().Types()), R"(%2 = OpTypeBool
%3 = OpConstantNull %2
)");
    EXPECT_EQ(DumpInstructions(b.CurrentFunction().instructions()),
              R"(%1 = OpLogicalNot %2 %3
)");
}

TEST_F(SpirvASTPrinterTest, UnaryOp_LoadRequired) {
    auto* var = Var("param", ty.vec3<f32>());

    auto* expr = create<ast::UnaryOpExpression>(core::UnaryOp::kNegation, Expr("param"));
    WrapInFunction(var, expr);

    Builder& b = Build();

    b.PushFunctionForTesting();
    EXPECT_TRUE(b.GenerateFunctionVariable(var)) << b.Diagnostics();
    EXPECT_EQ(b.GenerateUnaryOpExpression(expr), 6u) << b.Diagnostics();
    ASSERT_FALSE(b.has_error()) << b.Diagnostics();

    EXPECT_EQ(DumpInstructions(b.Module().Types()), R"(%4 = OpTypeFloat 32
%3 = OpTypeVector %4 3
%2 = OpTypePointer Function %3
%5 = OpConstantNull %3
)");
    EXPECT_EQ(DumpInstructions(b.CurrentFunction().variables()),
              R"(%1 = OpVariable %2 Function %5
)");
    EXPECT_EQ(DumpInstructions(b.CurrentFunction().instructions()),
              R"(%7 = OpLoad %3 %1
%6 = OpFNegate %3 %7
)");
}

}  // namespace
}  // namespace tint::spirv::writer
