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

#include "src/tint/lang/core/ir/switch.h"

#include "gmock/gmock.h"
#include "gtest/gtest-spi.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"

namespace tint::core::ir {
namespace {

using namespace tint::core::number_suffixes;  // NOLINT
using IR_SwitchTest = IRTestHelper;

TEST_F(IR_SwitchTest, Usage) {
    auto* cond = b.Constant(true);
    auto* switch_ = b.Switch(cond);
    EXPECT_THAT(cond->Usages(), testing::UnorderedElementsAre(Usage{switch_, 0u}));
}

TEST_F(IR_SwitchTest, Results) {
    auto* cond = b.Constant(true);
    auto* switch_ = b.Switch(cond);
    EXPECT_FALSE(switch_->HasResults());
    EXPECT_FALSE(switch_->HasMultiResults());
}

TEST_F(IR_SwitchTest, Parent) {
    auto* switch_ = b.Switch(1_i);
    b.Case(switch_, {Switch::CaseSelector{nullptr}});
    EXPECT_THAT(switch_->Cases().Front().Block()->Parent(), switch_);
}

}  // namespace
}  // namespace tint::core::ir
