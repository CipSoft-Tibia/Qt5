// Copyright 2020 The Tint Authors.
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

#ifndef SRC_TINT_LANG_SPIRV_READER_COMMON_OPTIONS_H_
#define SRC_TINT_LANG_SPIRV_READER_COMMON_OPTIONS_H_

namespace tint::spirv::reader {

/// Options that control how the SPIR-V parser should behave.
struct Options {
    /// Set to `true` to allow calls to derivative builtins in non-uniform control flow.
    bool allow_non_uniform_derivatives = false;
    /// Set to `true` to allow use of Chromium-specific extensions.
    bool allow_chromium_extensions = false;
};

}  // namespace tint::spirv::reader

#endif  // SRC_TINT_LANG_SPIRV_READER_COMMON_OPTIONS_H_
