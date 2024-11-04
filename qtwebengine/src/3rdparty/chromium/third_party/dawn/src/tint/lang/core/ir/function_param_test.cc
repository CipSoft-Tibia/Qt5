// Copyright 2023 The Tint Authors.
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

#include "src/tint/lang/core/ir/function_param.h"
#include "gtest/gtest-spi.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"

namespace tint::core::ir {
namespace {

using namespace tint::core::number_suffixes;  // NOLINT
using IR_FunctionParamTest = IRTestHelper;

TEST_F(IR_FunctionParamTest, Fail_NullType) {
    EXPECT_FATAL_FAILURE(
        {
            Module mod;
            Builder b{mod};
            b.FunctionParam(nullptr);
        },
        "");
}

TEST_F(IR_FunctionParamTest, Fail_SetDuplicateBuiltin) {
    EXPECT_FATAL_FAILURE(
        {
            Module mod;
            Builder b{mod};
            auto* fp = b.FunctionParam(mod.Types().f32());
            fp->SetBuiltin(FunctionParam::Builtin::kVertexIndex);
            fp->SetBuiltin(FunctionParam::Builtin::kSampleMask);
        },
        "");
}

}  // namespace
}  // namespace tint::core::ir
