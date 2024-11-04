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

#ifndef SRC_TINT_CMD_COMMON_GENERATE_EXTERNAL_TEXTURE_BINDINGS_H_
#define SRC_TINT_CMD_COMMON_GENERATE_EXTERNAL_TEXTURE_BINDINGS_H_

#include "src/tint/api/options/external_texture.h"

// Forward declarations
namespace tint {
class Program;
}

namespace tint::cmd {

ExternalTextureOptions::BindingsMap GenerateExternalTextureBindings(const Program* program);

}  // namespace tint::cmd

#endif  // SRC_TINT_CMD_COMMON_GENERATE_EXTERNAL_TEXTURE_BINDINGS_H_
