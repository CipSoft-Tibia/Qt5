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

#include <string>

#include "gtest/gtest-spi.h"
#include "src/tint/ast/diagnostic_control.h"
#include "src/tint/ast/test_helper.h"
#include "src/tint/builtin/diagnostic_severity.h"

namespace tint::ast {
namespace {

using DiagnosticControlTest = TestHelper;

TEST_F(DiagnosticControlTest, Assert_RuleNotTemplated) {
    EXPECT_FATAL_FAILURE(
        {
            ProgramBuilder b;
            DiagnosticControl control(builtin::DiagnosticSeverity::kWarning,
                                      b.Ident("name", "a", "b", "c"));
        },
        "internal compiler error");
}

}  // namespace
}  // namespace tint::ast
