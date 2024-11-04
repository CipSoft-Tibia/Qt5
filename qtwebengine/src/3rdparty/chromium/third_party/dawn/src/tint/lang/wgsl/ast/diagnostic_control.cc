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

#include "src/tint/lang/wgsl/ast/diagnostic_control.h"

#include <string>

#include "src/tint/lang/core/diagnostic_severity.h"
#include "src/tint/lang/wgsl/ast/identifier.h"
#include "src/tint/lang/wgsl/ast/templated_identifier.h"

namespace tint::ast {

DiagnosticControl::DiagnosticControl() = default;

DiagnosticControl::DiagnosticControl(core::DiagnosticSeverity sev, const DiagnosticRuleName* rule)
    : severity(sev), rule_name(rule) {
    TINT_ASSERT(rule != nullptr);
}

DiagnosticControl::DiagnosticControl(DiagnosticControl&&) = default;

}  // namespace tint::ast
