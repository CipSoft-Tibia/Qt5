// Copyright 2022 The Tint Authors.
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

#ifndef SRC_TINT_LANG_CORE_IR_IF_H_
#define SRC_TINT_LANG_CORE_IR_IF_H_

#include "src/tint/lang/core/ir/control_instruction.h"

// Forward declarations
namespace tint::core::ir {
class MultiInBlock;
}  // namespace tint::core::ir

namespace tint::core::ir {

/// If instruction.
///
/// ```
///                   in
///                    ┃
///         ┏━━━━━━━━━━┻━━━━━━━━━━┓
///         ▼                     ▼
///    ┌────────────┐      ┌────────────┐
///    │  True      │      │  False     │
///    | (optional) |      | (optional) |
///    └────────────┘      └────────────┘
///  ExitIf ┃                     ┃ ExitIf
///         ┗━━━━━━━━━━┳━━━━━━━━━━┛
///                    ▼
///                   out
/// ```
class If : public Castable<If, ControlInstruction> {
  public:
    /// The index of the condition operand
    static constexpr size_t kConditionOperandOffset = 0;

    /// Constructor
    /// @param cond the if condition
    /// @param t the true block
    /// @param f the false block
    If(Value* cond, ir::Block* t, ir::Block* f);
    ~If() override;

    /// @copydoc ControlInstruction::ForeachBlock
    void ForeachBlock(const std::function<void(ir::Block*)>& cb) override;

    /// @returns the if condition
    Value* Condition() { return operands_[kConditionOperandOffset]; }

    /// @returns the true block
    ir::Block* True() { return true_; }

    /// @returns the false block
    ir::Block* False() { return false_; }

    /// @returns the friendly name for the instruction
    std::string_view FriendlyName() override { return "if"; }

  private:
    ir::Block* true_ = nullptr;
    ir::Block* false_ = nullptr;
};

}  // namespace tint::core::ir

#endif  // SRC_TINT_LANG_CORE_IR_IF_H_
