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

#include "src/tint/ast/while_statement.h"

#include "src/tint/program_builder.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::WhileStatement);

namespace tint::ast {

WhileStatement::WhileStatement(ProgramID pid,
                               NodeID nid,
                               const Source& src,
                               const Expression* cond,
                               const BlockStatement* b)
    : Base(pid, nid, src), condition(cond), body(b) {
    TINT_ASSERT(AST, cond);
    TINT_ASSERT(AST, body);

    TINT_ASSERT_PROGRAM_IDS_EQUAL_IF_VALID(AST, condition, program_id);
    TINT_ASSERT_PROGRAM_IDS_EQUAL_IF_VALID(AST, body, program_id);
}

WhileStatement::WhileStatement(WhileStatement&&) = default;

WhileStatement::~WhileStatement() = default;

const WhileStatement* WhileStatement::Clone(CloneContext* ctx) const {
    // Clone arguments outside of create() call to have deterministic ordering
    auto src = ctx->Clone(source);

    auto* cond = ctx->Clone(condition);
    auto* b = ctx->Clone(body);
    return ctx->dst->create<WhileStatement>(src, cond, b);
}

}  // namespace tint::ast
