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

#include "src/tint/lang/wgsl/ast/return_statement.h"

#include "src/tint/lang/wgsl/ast/builder.h"
#include "src/tint/lang/wgsl/ast/clone_context.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::ReturnStatement);

namespace tint::ast {

ReturnStatement::ReturnStatement(GenerationID pid, NodeID nid, const Source& src)
    : Base(pid, nid, src), value(nullptr) {}

ReturnStatement::ReturnStatement(GenerationID pid,
                                 NodeID nid,
                                 const Source& src,
                                 const Expression* val)
    : Base(pid, nid, src), value(val) {
    TINT_ASSERT_GENERATION_IDS_EQUAL_IF_VALID(value, generation_id);
}

ReturnStatement::~ReturnStatement() = default;

const ReturnStatement* ReturnStatement::Clone(CloneContext& ctx) const {
    // Clone arguments outside of create() call to have deterministic ordering
    auto src = ctx.Clone(source);
    auto* ret = ctx.Clone(value);
    return ctx.dst->create<ReturnStatement>(src, ret);
}

}  // namespace tint::ast
