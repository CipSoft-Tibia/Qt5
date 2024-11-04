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

#include "src/tint/lang/wgsl/ast/builtin_attribute.h"

#include <string>

#include "src/tint/lang/wgsl/ast/builder.h"
#include "src/tint/lang/wgsl/ast/clone_context.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::BuiltinAttribute);

namespace tint::ast {

BuiltinAttribute::BuiltinAttribute(GenerationID pid,
                                   NodeID nid,
                                   const Source& src,
                                   const Expression* b)
    : Base(pid, nid, src), builtin(b) {
    TINT_ASSERT_GENERATION_IDS_EQUAL(b, generation_id);
}

BuiltinAttribute::~BuiltinAttribute() = default;

std::string BuiltinAttribute::Name() const {
    return "builtin";
}

const BuiltinAttribute* BuiltinAttribute::Clone(CloneContext& ctx) const {
    // Clone arguments outside of create() call to have deterministic ordering
    auto src = ctx.Clone(source);
    auto b = ctx.Clone(builtin);
    return ctx.dst->create<BuiltinAttribute>(src, b);
}

}  // namespace tint::ast
