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

#include "src/tint/sem/struct.h"

#include "src/tint/ast/struct_member.h"

TINT_INSTANTIATE_TYPEINFO(tint::sem::Struct);
TINT_INSTANTIATE_TYPEINFO(tint::sem::StructMember);

namespace tint::sem {

Struct::Struct(const ast::Struct* declaration,
               tint::Source source,
               Symbol name,
               utils::VectorRef<const StructMember*> members,
               uint32_t align,
               uint32_t size,
               uint32_t size_no_padding)
    : Base(source, name, members, align, size, size_no_padding), declaration_(declaration) {}

Struct::~Struct() = default;

StructMember::StructMember(const ast::StructMember* declaration,
                           tint::Source source,
                           Symbol name,
                           const type::Type* type,
                           uint32_t index,
                           uint32_t offset,
                           uint32_t align,
                           uint32_t size,
                           std::optional<uint32_t> location)
    : Base(source, name, type, index, offset, align, size, location), declaration_(declaration) {}

StructMember::~StructMember() = default;

}  // namespace tint::sem
