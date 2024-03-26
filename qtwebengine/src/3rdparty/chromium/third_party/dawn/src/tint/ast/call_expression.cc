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

#include "src/tint/ast/call_expression.h"

#include <utility>

#include "src/tint/program_builder.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::CallExpression);

namespace tint::ast {

CallExpression::CallExpression(ProgramID pid,
                               NodeID nid,
                               const Source& src,
                               const IdentifierExpression* t,
                               utils::VectorRef<const Expression*> a)
    : Base(pid, nid, src), target(t), args(std::move(a)) {
    TINT_ASSERT(AST, target);
    TINT_ASSERT_PROGRAM_IDS_EQUAL_IF_VALID(AST, target, program_id);
    for (auto* arg : args) {
        TINT_ASSERT(AST, arg);
        TINT_ASSERT_PROGRAM_IDS_EQUAL_IF_VALID(AST, arg, program_id);
    }
}

CallExpression::CallExpression(CallExpression&&) = default;

CallExpression::~CallExpression() = default;

const CallExpression* CallExpression::Clone(CloneContext* ctx) const {
    // Clone arguments outside of create() call to have deterministic ordering
    auto src = ctx->Clone(source);
    auto p = ctx->Clone(args);
    auto t = ctx->Clone(target);
    return ctx->dst->create<CallExpression>(src, t, std::move(p));
}

}  // namespace tint::ast
