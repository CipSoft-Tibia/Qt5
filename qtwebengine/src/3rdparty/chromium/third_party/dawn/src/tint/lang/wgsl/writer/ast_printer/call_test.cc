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

#include "src/tint/lang/wgsl/ast/call_statement.h"
#include "src/tint/lang/wgsl/writer/ast_printer/helper_test.h"
#include "src/tint/utils/text/string_stream.h"

#include "gmock/gmock.h"

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::wgsl::writer {
namespace {

using WgslASTPrinterTest = TestHelper;

TEST_F(WgslASTPrinterTest, EmitExpression_Call_WithoutParams) {
    Func("my_func", tint::Empty, ty.f32(),
         Vector{
             Return(1.23_f),
         });

    auto* call = Call("my_func");
    WrapInFunction(call);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, call);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "my_func()");
}

TEST_F(WgslASTPrinterTest, EmitExpression_Call_WithParams) {
    Func("my_func",
         Vector{
             Param(Sym(), ty.f32()),
             Param(Sym(), ty.f32()),
         },
         ty.f32(),
         Vector{
             Return(1.23_f),
         });
    GlobalVar("param1", ty.f32(), core::AddressSpace::kPrivate);
    GlobalVar("param2", ty.f32(), core::AddressSpace::kPrivate);

    auto* call = Call("my_func", "param1", "param2");
    WrapInFunction(call);

    ASTPrinter& gen = Build();

    StringStream out;
    gen.EmitExpression(out, call);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(out.str(), "my_func(param1, param2)");
}

TEST_F(WgslASTPrinterTest, EmitStatement_Call) {
    Func("my_func",
         Vector{
             Param(Sym(), ty.f32()),
             Param(Sym(), ty.f32()),
         },
         ty.void_(), tint::Empty, tint::Empty);
    GlobalVar("param1", ty.f32(), core::AddressSpace::kPrivate);
    GlobalVar("param2", ty.f32(), core::AddressSpace::kPrivate);

    auto* call = Call("my_func", "param1", "param2");
    auto* stmt = CallStmt(call);
    WrapInFunction(stmt);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();
    gen.EmitStatement(stmt);
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(gen.Result(), "  my_func(param1, param2);\n");
}

}  // namespace
}  // namespace tint::wgsl::writer
