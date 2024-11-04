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

#ifndef SRC_TINT_LANG_CORE_IR_EXIT_LOOP_H_
#define SRC_TINT_LANG_CORE_IR_EXIT_LOOP_H_

#include "src/tint/lang/core/ir/exit.h"
#include "src/tint/utils/rtti/castable.h"

// Forward declarations
namespace tint::core::ir {
class Loop;
}  // namespace tint::core::ir

namespace tint::core::ir {

/// A exit loop instruction.
class ExitLoop : public Castable<ExitLoop, Exit> {
  public:
    /// The base offset in Operands() for the args
    static constexpr size_t kArgsOperandOffset = 0;

    /// Constructor
    /// @param loop the loop being exited
    /// @param args the target MultiInBlock arguments
    explicit ExitLoop(ir::Loop* loop, VectorRef<Value*> args = tint::Empty);
    ~ExitLoop() override;

    /// Re-associates the exit with the given loop instruction
    /// @param l the new loop to exit from
    void SetLoop(ir::Loop* l);

    /// @returns the loop being exited
    ir::Loop* Loop();

    /// @returns the friendly name for the instruction
    std::string_view FriendlyName() override { return "exit-loop"; }
};

}  // namespace tint::core::ir

#endif  // SRC_TINT_LANG_CORE_IR_EXIT_LOOP_H_
