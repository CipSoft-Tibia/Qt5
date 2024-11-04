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

#ifndef SRC_TINT_LANG_CORE_IR_TRANSFORM_DEMOTE_TO_HELPER_H_
#define SRC_TINT_LANG_CORE_IR_TRANSFORM_DEMOTE_TO_HELPER_H_

#include <string>

#include "src/tint/utils/result/result.h"

// Forward declarations.
namespace tint::core::ir {
class Module;
}

namespace tint::core::ir::transform {

/// DemoteToHelper is a transform that emulates demote-to-helper semantics for discard instructions.
///
/// For backend targets that implement discard by terminating the invocation, we need to change the
/// program to ensure that discarding the fragment does not affect uniformity with respect to
/// derivative operations. We do this by setting a global flag and masking all writes to storage
/// buffers and textures.
/// @param module the module to transform
/// @returns an error string on failure
Result<SuccessType, std::string> DemoteToHelper(Module* module);

}  // namespace tint::core::ir::transform

#endif  // SRC_TINT_LANG_CORE_IR_TRANSFORM_DEMOTE_TO_HELPER_H_
