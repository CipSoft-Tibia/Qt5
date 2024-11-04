// Copyright 2021 The Tint Authors.
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

#ifndef SRC_TINT_LANG_WGSL_SEM_BEHAVIOR_H_
#define SRC_TINT_LANG_WGSL_SEM_BEHAVIOR_H_

#include "src/tint/utils/containers/enum_set.h"
#include "src/tint/utils/traits/traits.h"

namespace tint::sem {

/// Behavior enumerates the possible behaviors of an expression or statement.
/// @see https://www.w3.org/TR/WGSL/#behaviors
enum class Behavior {
    kReturn,
    kBreak,
    kContinue,
    kNext,
};

/// Behaviors is a set of Behavior
using Behaviors = tint::EnumSet<Behavior>;

/// @param behavior the behavior
/// @returns the string for the given enumerator
std::string_view ToString(Behavior behavior);

/// Writes the Behavior to the stream.
/// @param out the stream to write to
/// @param behavior the Behavior to write
/// @returns out so calls can be chained
template <typename STREAM, typename = traits::EnableIfIsOStream<STREAM>>
auto& operator<<(STREAM& out, Behavior behavior) {
    return out << ToString(behavior);
}

}  // namespace tint::sem

#endif  // SRC_TINT_LANG_WGSL_SEM_BEHAVIOR_H_
