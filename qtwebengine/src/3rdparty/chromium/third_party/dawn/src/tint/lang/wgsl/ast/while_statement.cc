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

#include "src/tint/lang/wgsl/ast/while_statement.h"

#include <utility>

#include "src/tint/lang/wgsl/ast/builder.h"
#include "src/tint/lang/wgsl/ast/clone_context.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::WhileStatement);

namespace tint::ast {

WhileStatement::WhileStatement(GenerationID pid,
                               NodeID nid,
                               const Source& src,
                               const Expression* cond,
                               const BlockStatement* b,
                               VectorRef<const ast::Attribute*> attrs)
    : Base(pid, nid, src), condition(cond), body(b), attributes(std::move(attrs)) {
    TINT_ASSERT(cond);
    TINT_ASSERT(body);

    TINT_ASSERT_GENERATION_IDS_EQUAL_IF_VALID(condition, generation_id);
    TINT_ASSERT_GENERATION_IDS_EQUAL_IF_VALID(body, generation_id);
    for (auto* attr : attributes) {
        TINT_ASSERT(attr);
        TINT_ASSERT_GENERATION_IDS_EQUAL_IF_VALID(attr, generation_id);
    }
}

WhileStatement::~WhileStatement() = default;

const WhileStatement* WhileStatement::Clone(CloneContext& ctx) const {
    // Clone arguments outside of create() call to have deterministic ordering
    auto src = ctx.Clone(source);

    auto* cond = ctx.Clone(condition);
    auto* b = ctx.Clone(body);
    auto attrs = ctx.Clone(attributes);
    return ctx.dst->create<WhileStatement>(src, cond, b, std::move(attrs));
}

}  // namespace tint::ast
