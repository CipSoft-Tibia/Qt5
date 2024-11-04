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

#ifndef SRC_TINT_LANG_CORE_IR_USER_CALL_H_
#define SRC_TINT_LANG_CORE_IR_USER_CALL_H_

#include "src/tint/lang/core/ir/call.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/utils/rtti/castable.h"

namespace tint::core::ir {

/// A user call instruction in the IR.
class UserCall : public Castable<UserCall, Call> {
  public:
    /// The offset in Operands() for the function being called
    static constexpr size_t kFunctionOperandOffset = 0;

    /// The base offset in Operands() for the call arguments
    static constexpr size_t kArgsOperandOffset = 1;

    /// Constructor
    /// @param result the result value
    /// @param func the function being called
    /// @param args the function arguments
    UserCall(InstructionResult* result, Function* func, VectorRef<Value*> args);
    ~UserCall() override;

    /// @returns the call arguments
    tint::Slice<Value*> Args() override { return operands_.Slice().Offset(kArgsOperandOffset); }

    /// @returns the called function name
    Function* Func() { return operands_[kFunctionOperandOffset]->As<ir::Function>(); }

    /// @returns the friendly name for the instruction
    std::string_view FriendlyName() override { return "user-call"; }
};

}  // namespace tint::core::ir

#endif  // SRC_TINT_LANG_CORE_IR_USER_CALL_H_
