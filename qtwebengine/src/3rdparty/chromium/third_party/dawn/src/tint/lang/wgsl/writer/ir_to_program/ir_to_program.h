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

#ifndef SRC_TINT_LANG_WGSL_WRITER_IR_TO_PROGRAM_IR_TO_PROGRAM_H_
#define SRC_TINT_LANG_WGSL_WRITER_IR_TO_PROGRAM_IR_TO_PROGRAM_H_

#include "src/tint/lang/wgsl/program/program.h"

namespace tint::core::ir {
class Module;
}

namespace tint::wgsl::writer {

/// Builds a tint::Program from an core::ir::Module
/// @param module the IR module
/// @return the tint::Program.
/// @note Check the returned Program::Diagnostics() for any errors.
Program IRToProgram(core::ir::Module& module);

}  // namespace tint::wgsl::writer

#endif  // SRC_TINT_LANG_WGSL_WRITER_IR_TO_PROGRAM_IR_TO_PROGRAM_H_
