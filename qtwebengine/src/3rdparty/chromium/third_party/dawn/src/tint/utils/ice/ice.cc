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

#include "src/tint/utils/ice/ice.h"

#include <memory>
#include <string>

#include "src/tint/utils/debug/debugger.h"

namespace tint {
namespace {

InternalCompilerErrorReporter* ice_reporter = nullptr;

}  // namespace

void SetInternalCompilerErrorReporter(InternalCompilerErrorReporter* reporter) {
    ice_reporter = reporter;
}

InternalCompilerError::InternalCompilerError(const char* file, size_t line)
    : file_(file), line_(line) {}

InternalCompilerError::~InternalCompilerError() {
    if (ice_reporter) {
        ice_reporter(*this);
    }
    debugger::Break();
}

std::string InternalCompilerError::Error() const {
    return std::string(File()) + +":" + std::to_string(Line()) +
           " internal compiler error: " + Message();
}

}  // namespace tint
