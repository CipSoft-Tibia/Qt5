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

#ifndef SRC_TINT_LANG_CORE_IR_RETURN_H_
#define SRC_TINT_LANG_CORE_IR_RETURN_H_

#include "src/tint/lang/core/ir/terminator.h"
#include "src/tint/utils/rtti/castable.h"

// Forward declarations
namespace tint::core::ir {
class Function;
}  // namespace tint::core::ir

namespace tint::core::ir {

/// A return instruction.
class Return : public Castable<Return, Terminator> {
  public:
    /// The offset in Operands() for the function being returned
    static constexpr size_t kFunctionOperandOffset = 0;

    /// The offset in Operands() for the return argument
    static constexpr size_t kArgOperandOffset = 1;

    /// Constructor (no return value)
    /// @param func the function being returned
    explicit Return(Function* func);

    /// Constructor
    /// @param func the function being returned
    /// @param arg the return value
    Return(Function* func, ir::Value* arg);

    ~Return() override;

    /// @returns the function being returned
    Function* Func() { return tint::As<Function>(operands_[kFunctionOperandOffset]); }

    /// @returns the return value, or nullptr
    ir::Value* Value() const {
        return operands_.Length() > kArgOperandOffset ? operands_[kArgOperandOffset] : nullptr;
    }

    /// Sets the return value
    /// @param val the new return value
    void SetValue(ir::Value* val) { SetOperand(kArgOperandOffset, val); }

    /// @returns the return arguments
    tint::Slice<ir::Value* const> Args() override {
        return operands_.Slice().Offset(kArgOperandOffset);
    }

    /// @returns the friendly name for the instruction
    std::string_view FriendlyName() override { return "return"; }
};

}  // namespace tint::core::ir

#endif  // SRC_TINT_LANG_CORE_IR_RETURN_H_
