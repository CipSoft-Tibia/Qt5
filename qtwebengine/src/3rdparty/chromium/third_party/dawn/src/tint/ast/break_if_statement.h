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

#ifndef SRC_TINT_AST_BREAK_IF_STATEMENT_H_
#define SRC_TINT_AST_BREAK_IF_STATEMENT_H_

#include <utility>

#include "src/tint/ast/block_statement.h"
#include "src/tint/ast/expression.h"

namespace tint::ast {

/// A break-if statement
class BreakIfStatement final : public Castable<BreakIfStatement, Statement> {
  public:
    /// Constructor
    /// @param pid the identifier of the program that owns this node
    /// @param nid the unique node identifier
    /// @param src the source of this node
    /// @param condition the if condition
    BreakIfStatement(ProgramID pid, NodeID nid, const Source& src, const Expression* condition);

    /// Move constructor
    BreakIfStatement(BreakIfStatement&&);
    ~BreakIfStatement() override;

    /// Clones this node and all transitive child nodes using the `CloneContext` `ctx`.
    /// @param ctx the clone context
    /// @return the newly cloned node
    const BreakIfStatement* Clone(CloneContext* ctx) const override;

    /// The if condition or nullptr if none set
    const Expression* const condition;
};

}  // namespace tint::ast

#endif  // SRC_TINT_AST_BREAK_IF_STATEMENT_H_
