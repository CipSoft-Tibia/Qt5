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

#include "src/tint/lang/wgsl/sem/array_count.h"

#include "src/tint/lang/wgsl/ast/identifier.h"

TINT_INSTANTIATE_TYPEINFO(tint::sem::NamedOverrideArrayCount);
TINT_INSTANTIATE_TYPEINFO(tint::sem::UnnamedOverrideArrayCount);

namespace tint::sem {

NamedOverrideArrayCount::NamedOverrideArrayCount(const GlobalVariable* var)
    : Base(static_cast<size_t>(tint::TypeInfo::Of<NamedOverrideArrayCount>().full_hashcode)),
      variable(var) {}
NamedOverrideArrayCount::~NamedOverrideArrayCount() = default;

bool NamedOverrideArrayCount::Equals(const UniqueNode& other) const {
    if (auto* v = other.As<NamedOverrideArrayCount>()) {
        return variable == v->variable;
    }
    return false;
}

std::string NamedOverrideArrayCount::FriendlyName() const {
    return variable->Declaration()->name->symbol.Name();
}

core::type::ArrayCount* NamedOverrideArrayCount::Clone(core::type::CloneContext&) const {
    TINT_UNREACHABLE() << "Named override array count clone not available";
    return nullptr;
}

UnnamedOverrideArrayCount::UnnamedOverrideArrayCount(const ValueExpression* e)
    : Base(static_cast<size_t>(tint::TypeInfo::Of<UnnamedOverrideArrayCount>().full_hashcode)),
      expr(e) {}
UnnamedOverrideArrayCount::~UnnamedOverrideArrayCount() = default;

bool UnnamedOverrideArrayCount::Equals(const UniqueNode& other) const {
    if (auto* v = other.As<UnnamedOverrideArrayCount>()) {
        return expr == v->expr;
    }
    return false;
}

std::string UnnamedOverrideArrayCount::FriendlyName() const {
    return "[unnamed override-expression]";
}

core::type::ArrayCount* UnnamedOverrideArrayCount::Clone(core::type::CloneContext&) const {
    TINT_UNREACHABLE() << "Unnamed override array count clone not available";
    return nullptr;
}

}  // namespace tint::sem
