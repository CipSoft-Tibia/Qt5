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

#include "src/tint/lang/wgsl/ast/assignment_statement.h"

#include "src/tint/lang/wgsl/ast/builder.h"
#include "src/tint/lang/wgsl/ast/clone_context.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::AssignmentStatement);

namespace tint::ast {

AssignmentStatement::AssignmentStatement(GenerationID pid,
                                         NodeID nid,
                                         const Source& src,
                                         const Expression* l,
                                         const Expression* r)
    : Base(pid, nid, src), lhs(l), rhs(r) {
    TINT_ASSERT(lhs);
    TINT_ASSERT_GENERATION_IDS_EQUAL_IF_VALID(lhs, generation_id);
    TINT_ASSERT(rhs);
    TINT_ASSERT_GENERATION_IDS_EQUAL_IF_VALID(rhs, generation_id);
}

AssignmentStatement::~AssignmentStatement() = default;

const AssignmentStatement* AssignmentStatement::Clone(CloneContext& ctx) const {
    // Clone arguments outside of create() call to have deterministic ordering
    auto src = ctx.Clone(source);
    auto* l = ctx.Clone(lhs);
    auto* r = ctx.Clone(rhs);
    return ctx.dst->create<AssignmentStatement>(src, l, r);
}

}  // namespace tint::ast
