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

#include "src/tint/sem/type_expression.h"

TINT_INSTANTIATE_TYPEINFO(tint::sem::TypeExpression);

namespace tint::sem {

TypeExpression::TypeExpression(const ast::Expression* declaration,
                               const Statement* statement,
                               const type::Type* type)
    : Base(declaration, statement), type_(type) {}

TypeExpression::~TypeExpression() = default;

}  // namespace tint::sem
