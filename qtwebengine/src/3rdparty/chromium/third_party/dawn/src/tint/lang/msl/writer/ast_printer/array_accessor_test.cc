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

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::msl::writer {
namespace {

using MslASTPrinterTest = TestHelper;

TEST_F(MslASTPrinterTest, IndexAccessor) {
    auto* ary = Var("ary", ty.array<i32, 10>());
    auto* expr = IndexAccessor("ary", 5_i);
    WrapInFunction(ary, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "ary[5]");
}

TEST_F(MslASTPrinterTest, IndexAccessor_OfDref) {
    GlobalVar("ary", ty.array<i32, 10>(), core::AddressSpace::kPrivate);

    auto* p = Let("p", AddressOf("ary"));
    auto* expr = IndexAccessor(Deref("p"), 5_i);
    WrapInFunction(p, expr);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, expr)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "(*(p))[5]");
}

}  // namespace
}  // namespace tint::msl::writer
