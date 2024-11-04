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

#ifndef SRC_TINT_LANG_CORE_IR_TERMINATOR_H_
#define SRC_TINT_LANG_CORE_IR_TERMINATOR_H_

#include "src/tint/lang/core/ir/operand_instruction.h"
#include "src/tint/lang/core/ir/value.h"
#include "src/tint/utils/rtti/castable.h"

// Forward declarations
namespace tint::core::ir {
class Block;
}  // namespace tint::core::ir

namespace tint::core::ir {

/// The base class of all instructions that terminate a block.
class Terminator : public Castable<Terminator, OperandInstruction<1, 0>> {
  public:
    ~Terminator() override;

    /// @returns the terminator arguments
    virtual tint::Slice<Value* const> Args() { return operands_.Slice(); }
};

}  // namespace tint::core::ir

#endif  // SRC_TINT_LANG_CORE_IR_TERMINATOR_H_
