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

#include "src/tint/lang/wgsl/ast/transform/unshadow.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "src/tint/lang/wgsl/program/clone_context.h"
#include "src/tint/lang/wgsl/program/program_builder.h"
#include "src/tint/lang/wgsl/resolver/resolve.h"
#include "src/tint/lang/wgsl/sem/block_statement.h"
#include "src/tint/lang/wgsl/sem/function.h"
#include "src/tint/lang/wgsl/sem/statement.h"
#include "src/tint/lang/wgsl/sem/variable.h"
#include "src/tint/utils/rtti/switch.h"

TINT_INSTANTIATE_TYPEINFO(tint::ast::transform::Unshadow);

namespace tint::ast::transform {

/// PIMPL state for the transform
struct Unshadow::State {
    /// The source program
    const Program* const src;
    /// The target program builder
    ProgramBuilder b;
    /// The clone context
    program::CloneContext ctx = {&b, src, /* auto_clone_symbols */ true};

    /// Constructor
    /// @param program the source program
    explicit State(const Program* program) : src(program) {}

    /// Runs the transform
    /// @returns the new program or SkipTransform if the transform is not required
    Transform::ApplyResult Run() {
        auto& sem = src->Sem();

        // Maps a variable to its new name.
        Hashmap<const sem::Variable*, Symbol, 8> renamed_to;

        auto rename = [&](const sem::Variable* v) -> const Variable* {
            auto* decl = v->Declaration();
            auto name = decl->name->symbol.Name();
            auto symbol = b.Symbols().New(name);
            renamed_to.Add(v, symbol);

            auto source = ctx.Clone(decl->source);
            auto type = decl->type ? ctx.Clone(decl->type) : Type{};
            auto* initializer = ctx.Clone(decl->initializer);
            auto attributes = ctx.Clone(decl->attributes);
            return Switch(
                decl,  //
                [&](const Var* var) {
                    return b.Var(source, symbol, type, var->declared_address_space,
                                 var->declared_access, initializer, attributes);
                },
                [&](const Let*) { return b.Let(source, symbol, type, initializer, attributes); },
                [&](const Const*) {
                    return b.Const(source, symbol, type, initializer, attributes);
                },
                [&](const Parameter*) {  //
                    return b.Param(source, symbol, type, attributes);
                },
                [&](Default) {
                    TINT_ICE() << "unexpected variable type: " << decl->TypeInfo().name;
                    return nullptr;
                });
        };

        bool made_changes = false;

        for (auto* node : ctx.src->SemNodes().Objects()) {
            Switch(
                node,  //
                [&](const sem::LocalVariable* local) {
                    if (local->Shadows()) {
                        ctx.Replace(local->Declaration(), [&, local] { return rename(local); });
                        made_changes = true;
                    }
                },
                [&](const sem::Parameter* param) {
                    if (param->Shadows()) {
                        ctx.Replace(param->Declaration(), [&, param] { return rename(param); });
                        made_changes = true;
                    }
                });
        }

        if (!made_changes) {
            return SkipTransform;
        }

        ctx.ReplaceAll([&](const IdentifierExpression* ident) -> const IdentifierExpression* {
            if (auto* sem_ident = sem.GetVal(ident)) {
                if (auto* user = sem_ident->Unwrap()->As<sem::VariableUser>()) {
                    if (auto renamed = renamed_to.Find(user->Variable())) {
                        return b.Expr(*renamed);
                    }
                }
            }
            return nullptr;
        });

        ctx.Clone();
        return resolver::Resolve(b);
    }
};

Unshadow::Unshadow() = default;

Unshadow::~Unshadow() = default;

Transform::ApplyResult Unshadow::Apply(const Program* src, const DataMap&, DataMap&) const {
    return State(src).Run();
}

}  // namespace tint::ast::transform
