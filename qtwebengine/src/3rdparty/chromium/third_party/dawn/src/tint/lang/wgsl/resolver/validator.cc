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

#include "src/tint/lang/wgsl/resolver/validator.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/type/abstract_numeric.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/sampler.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/texture_dimension.h"
#include "src/tint/lang/wgsl/ast/alias.h"
#include "src/tint/lang/wgsl/ast/assignment_statement.h"
#include "src/tint/lang/wgsl/ast/bitcast_expression.h"
#include "src/tint/lang/wgsl/ast/break_statement.h"
#include "src/tint/lang/wgsl/ast/call_statement.h"
#include "src/tint/lang/wgsl/ast/continue_statement.h"
#include "src/tint/lang/wgsl/ast/disable_validation_attribute.h"
#include "src/tint/lang/wgsl/ast/discard_statement.h"
#include "src/tint/lang/wgsl/ast/for_loop_statement.h"
#include "src/tint/lang/wgsl/ast/id_attribute.h"
#include "src/tint/lang/wgsl/ast/if_statement.h"
#include "src/tint/lang/wgsl/ast/index_attribute.h"
#include "src/tint/lang/wgsl/ast/internal_attribute.h"
#include "src/tint/lang/wgsl/ast/interpolate_attribute.h"
#include "src/tint/lang/wgsl/ast/loop_statement.h"
#include "src/tint/lang/wgsl/ast/return_statement.h"
#include "src/tint/lang/wgsl/ast/switch_statement.h"
#include "src/tint/lang/wgsl/ast/traverse_expressions.h"
#include "src/tint/lang/wgsl/ast/unary_op_expression.h"
#include "src/tint/lang/wgsl/ast/variable_decl_statement.h"
#include "src/tint/lang/wgsl/ast/workgroup_attribute.h"
#include "src/tint/lang/wgsl/sem/break_if_statement.h"
#include "src/tint/lang/wgsl/sem/call.h"
#include "src/tint/lang/wgsl/sem/for_loop_statement.h"
#include "src/tint/lang/wgsl/sem/function.h"
#include "src/tint/lang/wgsl/sem/if_statement.h"
#include "src/tint/lang/wgsl/sem/loop_statement.h"
#include "src/tint/lang/wgsl/sem/materialize.h"
#include "src/tint/lang/wgsl/sem/member_accessor_expression.h"
#include "src/tint/lang/wgsl/sem/statement.h"
#include "src/tint/lang/wgsl/sem/struct.h"
#include "src/tint/lang/wgsl/sem/switch_statement.h"
#include "src/tint/lang/wgsl/sem/value_constructor.h"
#include "src/tint/lang/wgsl/sem/value_conversion.h"
#include "src/tint/lang/wgsl/sem/variable.h"
#include "src/tint/lang/wgsl/sem/while_statement.h"
#include "src/tint/utils/containers/map.h"
#include "src/tint/utils/containers/reverse.h"
#include "src/tint/utils/containers/transform.h"
#include "src/tint/utils/macros/defer.h"
#include "src/tint/utils/macros/scoped_assignment.h"
#include "src/tint/utils/math/math.h"
#include "src/tint/utils/text/string.h"
#include "src/tint/utils/text/string_stream.h"

using namespace tint::core::fluent_types;  // NOLINT

namespace tint::resolver {
namespace {

constexpr size_t kMaxFunctionParameters = 255;
constexpr size_t kMaxSwitchCaseSelectors = 16383;

bool IsValidStorageTextureDimension(core::type::TextureDimension dim) {
    switch (dim) {
        case core::type::TextureDimension::k1d:
        case core::type::TextureDimension::k2d:
        case core::type::TextureDimension::k2dArray:
        case core::type::TextureDimension::k3d:
            return true;
        default:
            return false;
    }
}

bool IsValidStorageTextureTexelFormat(core::TexelFormat format) {
    switch (format) {
        case core::TexelFormat::kBgra8Unorm:
        case core::TexelFormat::kR32Uint:
        case core::TexelFormat::kR32Sint:
        case core::TexelFormat::kR32Float:
        case core::TexelFormat::kRg32Uint:
        case core::TexelFormat::kRg32Sint:
        case core::TexelFormat::kRg32Float:
        case core::TexelFormat::kRgba8Unorm:
        case core::TexelFormat::kRgba8Snorm:
        case core::TexelFormat::kRgba8Uint:
        case core::TexelFormat::kRgba8Sint:
        case core::TexelFormat::kRgba16Uint:
        case core::TexelFormat::kRgba16Sint:
        case core::TexelFormat::kRgba16Float:
        case core::TexelFormat::kRgba32Uint:
        case core::TexelFormat::kRgba32Sint:
        case core::TexelFormat::kRgba32Float:
            return true;
        default:
            return false;
    }
}

// Helper to stringify a pipeline IO attribute.
std::string AttrToStr(const ast::Attribute* attr) {
    return Switch(
        attr,  //
        [&](const ast::BuiltinAttribute*) { return "@builtin"; },
        [&](const ast::LocationAttribute*) { return "@location"; });
}

template <typename CALLBACK>
void TraverseCallChain(const sem::Function* from, const sem::Function* to, CALLBACK&& callback) {
    for (auto* f : from->TransitivelyCalledFunctions()) {
        if (f == to) {
            callback(f);
            return;
        }
        if (f->TransitivelyCalledFunctions().Contains(to)) {
            TraverseCallChain(f, to, callback);
            callback(f);
            return;
        }
    }
    TINT_ICE() << "TraverseCallChain() 'from' does not transitively call 'to'";
}

}  // namespace

Validator::Validator(
    ProgramBuilder* builder,
    SemHelper& sem,
    const core::Extensions& enabled_extensions,
    const Hashmap<const core::type::Type*, const Source*, 8>& atomic_composite_info,
    Hashset<TypeAndAddressSpace, 8>& valid_type_storage_layouts)
    : symbols_(builder->Symbols()),
      diagnostics_(builder->Diagnostics()),
      sem_(sem),
      enabled_extensions_(enabled_extensions),
      atomic_composite_info_(atomic_composite_info),
      valid_type_storage_layouts_(valid_type_storage_layouts) {
    // Set default severities for filterable diagnostic rules.
    diagnostic_filters_.Set(core::CoreDiagnosticRule::kDerivativeUniformity,
                            core::DiagnosticSeverity::kError);
    diagnostic_filters_.Set(core::ChromiumDiagnosticRule::kUnreachableCode,
                            core::DiagnosticSeverity::kWarning);
}

Validator::~Validator() = default;

void Validator::AddError(const std::string& msg, const Source& source) const {
    diagnostics_.add_error(diag::System::Resolver, msg, source);
}

void Validator::AddWarning(const std::string& msg, const Source& source) const {
    diagnostics_.add_warning(diag::System::Resolver, msg, source);
}

void Validator::AddNote(const std::string& msg, const Source& source) const {
    diagnostics_.add_note(diag::System::Resolver, msg, source);
}

bool Validator::AddDiagnostic(core::DiagnosticRule rule,
                              const std::string& msg,
                              const Source& source) const {
    auto severity = diagnostic_filters_.Get(rule);
    if (severity != core::DiagnosticSeverity::kOff) {
        diag::Diagnostic d{};
        d.severity = ToSeverity(severity);
        d.system = diag::System::Resolver;
        d.source = source;
        d.message = msg;
        diagnostics_.add(std::move(d));
        if (severity == core::DiagnosticSeverity::kError) {
            return false;
        }
    }
    return true;
}

// https://gpuweb.github.io/gpuweb/wgsl/#plain-types-section
bool Validator::IsPlain(const core::type::Type* type) const {
    return type->IsAnyOf<core::type::Scalar, core::type::Atomic, core::type::Vector,
                         core::type::Matrix, core::type::Array, core::type::Struct>();
}

// https://gpuweb.github.io/gpuweb/wgsl/#fixed-footprint-types
bool Validator::IsFixedFootprint(const core::type::Type* type) const {
    return Switch(
        type,                                             //
        [&](const core::type::Vector*) { return true; },  //
        [&](const core::type::Matrix*) { return true; },  //
        [&](const core::type::Atomic*) { return true; },
        [&](const core::type::Array* arr) {
            return !arr->Count()->Is<core::type::RuntimeArrayCount>() &&
                   IsFixedFootprint(arr->ElemType());
        },
        [&](const core::type::Struct* str) {
            for (auto* member : str->Members()) {
                if (!IsFixedFootprint(member->Type())) {
                    return false;
                }
            }
            return true;
        },
        [&](Default) { return type->Is<core::type::Scalar>(); });
}

// https://gpuweb.github.io/gpuweb/wgsl.html#host-shareable-types
bool Validator::IsHostShareable(const core::type::Type* type) const {
    if (type->IsAnyOf<core::type::I32, core::type::U32, core::type::F32, core::type::F16>()) {
        return true;
    }
    return Switch(
        type,  //
        [&](const core::type::Vector* vec) { return IsHostShareable(vec->type()); },
        [&](const core::type::Matrix* mat) { return IsHostShareable(mat->type()); },
        [&](const core::type::Array* arr) { return IsHostShareable(arr->ElemType()); },
        [&](const core::type::Struct* str) {
            for (auto* member : str->Members()) {
                if (!IsHostShareable(member->Type())) {
                    return false;
                }
            }
            return true;
        },
        [&](const core::type::Atomic* atomic) { return IsHostShareable(atomic->Type()); });
}

// https://gpuweb.github.io/gpuweb/wgsl.html#storable-types
bool Validator::IsStorable(const core::type::Type* type) const {
    return IsPlain(type) || type->IsAnyOf<core::type::Texture, core::type::Sampler>();
}

const ast::Statement* Validator::ClosestContinuing(bool stop_at_loop,
                                                   sem::Statement* current_statement) const {
    for (const auto* s = current_statement; s != nullptr; s = s->Parent()) {
        if (stop_at_loop && s->Is<sem::LoopStatement>()) {
            break;
        }
        if (s->Is<sem::LoopContinuingBlockStatement>()) {
            return s->Declaration();
        }
        if (auto* f = As<sem::ForLoopStatement>(s->Parent())) {
            if (f->Declaration()->continuing == s->Declaration()) {
                return s->Declaration();
            }
            if (stop_at_loop) {
                break;
            }
        }
        if (Is<sem::WhileStatement>(s->Parent())) {
            if (stop_at_loop) {
                break;
            }
        }
    }
    return nullptr;
}

bool Validator::Atomic(const ast::TemplatedIdentifier* a, const core::type::Atomic* s) const {
    // https://gpuweb.github.io/gpuweb/wgsl/#atomic-types
    // T must be either u32 or i32.
    if (!s->Type()->IsAnyOf<core::type::U32, core::type::I32>()) {
        AddError("atomic only supports i32 or u32 types", a->arguments[0]->source);
        return false;
    }
    return true;
}

bool Validator::Pointer(const ast::TemplatedIdentifier* a, const core::type::Pointer* s) const {
    if (s->AddressSpace() == core::AddressSpace::kUndefined) {
        AddError("ptr missing address space", a->source);
        return false;
    }

    if (a->arguments.Length() > 2) {  // ptr<address-space, type [, access]>
        // https://www.w3.org/TR/WGSL/#access-mode-defaults
        // When writing a variable declaration or a pointer type in WGSL source:
        // * For the storage address space, the access mode is optional, and defaults to read.
        // * For other address spaces, the access mode must not be written.
        if (s->AddressSpace() != core::AddressSpace::kStorage) {
            AddError("only pointers in <storage> address space may specify an access mode",
                     a->source);
            return false;
        }
    }

    return CheckTypeAccessAddressSpace(s->StoreType(), s->Access(), s->AddressSpace(), tint::Empty,
                                       a->source);
}

bool Validator::StorageTexture(const core::type::StorageTexture* t, const Source& source) const {
    switch (t->access()) {
        case core::Access::kRead:
            if (!enabled_extensions_.Contains(
                    core::Extension::kChromiumExperimentalReadWriteStorageTexture)) {
                AddError(
                    "read-only storage textures require the "
                    "chromium_experimental_read_write_storage_texture extension to be enabled",
                    source);
                return false;
            }
            break;
        case core::Access::kReadWrite:
            if (!enabled_extensions_.Contains(
                    core::Extension::kChromiumExperimentalReadWriteStorageTexture)) {
                AddError(
                    "read-write storage textures require the "
                    "chromium_experimental_read_write_storage_texture extension to be enabled",
                    source);
                return false;
            }
            break;
        case core::Access::kWrite:
            break;
        case core::Access::kUndefined:
            AddError("storage texture missing access control", source);
            return false;
    }

    if (!IsValidStorageTextureDimension(t->dim())) {
        AddError("cube dimensions for storage textures are not supported", source);
        return false;
    }

    if (!IsValidStorageTextureTexelFormat(t->texel_format())) {
        AddError(
            "image format must be one of the texel formats specified for storage "
            "textues in https://gpuweb.github.io/gpuweb/wgsl/#texel-formats",
            source);
        return false;
    }
    return true;
}

bool Validator::SampledTexture(const core::type::SampledTexture* t, const Source& source) const {
    if (!t->type()->UnwrapRef()->IsAnyOf<core::type::F32, core::type::I32, core::type::U32>()) {
        AddError("texture_2d<type>: type must be f32, i32 or u32", source);
        return false;
    }

    return true;
}

bool Validator::MultisampledTexture(const core::type::MultisampledTexture* t,
                                    const Source& source) const {
    if (t->dim() != core::type::TextureDimension::k2d) {
        AddError("only 2d multisampled textures are supported", source);
        return false;
    }

    if (!t->type()->UnwrapRef()->IsAnyOf<core::type::F32, core::type::I32, core::type::U32>()) {
        AddError("texture_multisampled_2d<type>: type must be f32, i32 or u32", source);
        return false;
    }

    return true;
}

bool Validator::Materialize(const core::type::Type* to,
                            const core::type::Type* from,
                            const Source& source) const {
    if (core::type::Type::ConversionRank(from, to) == core::type::Type::kNoConversion) {
        AddError("cannot convert value of type '" + sem_.TypeNameOf(from) + "' to type '" +
                     sem_.TypeNameOf(to) + "'",
                 source);
        return false;
    }
    return true;
}

bool Validator::VariableInitializer(const ast::Variable* v,
                                    const core::type::Type* storage_ty,
                                    const sem::ValueExpression* initializer) const {
    auto* initializer_ty = initializer->Type();
    auto* value_type = initializer_ty->UnwrapRef();  // Implicit load of RHS

    // Value type has to match storage type
    if (storage_ty != value_type) {
        StringStream s;
        s << "cannot initialize " << v->Kind() << " of type '" << sem_.TypeNameOf(storage_ty)
          << "' with value of type '" << sem_.TypeNameOf(initializer_ty) << "'";
        AddError(s.str(), v->source);
        return false;
    }

    return true;
}

bool Validator::AddressSpaceLayout(const core::type::Type* store_ty,
                                   core::AddressSpace address_space,
                                   Source source) const {
    // https://gpuweb.github.io/gpuweb/wgsl/#storage-class-layout-constraints

    auto is_uniform_struct_or_array = [address_space](const core::type::Type* ty) {
        return address_space == core::AddressSpace::kUniform &&
               ty->IsAnyOf<core::type::Array, core::type::Struct>();
    };

    auto is_uniform_struct = [address_space](const core::type::Type* ty) {
        return address_space == core::AddressSpace::kUniform && ty->Is<core::type::Struct>();
    };

    auto required_alignment_of = [&](const core::type::Type* ty) {
        uint32_t actual_align = ty->Align();
        uint32_t required_align = actual_align;
        if (is_uniform_struct_or_array(ty)) {
            required_align = tint::RoundUp(16u, actual_align);
        }
        return required_align;
    };

    auto member_name_of = [](const core::type::StructMember* sm) { return sm->Name().Name(); };

    // Only validate the [type + address space] once
    if (!valid_type_storage_layouts_.Add(TypeAndAddressSpace{store_ty, address_space})) {
        return true;
    }

    if (!core::IsHostShareable(address_space)) {
        return true;
    }

    auto note_usage = [&] {
        AddNote("'" + store_ty->FriendlyName() + "' used in address space '" +
                    tint::ToString(address_space) + "' here",
                source);
    };

    // Among three host-shareable address spaces, f16 is supported in "uniform" and
    // "storage" address space, but not "push_constant" address space yet.
    if (Is<core::type::F16>(store_ty->DeepestElement()) &&
        address_space == core::AddressSpace::kPushConstant) {
        AddError("using f16 types in 'push_constant' address space is not implemented yet", source);
        return false;
    }

    if (auto* str = store_ty->As<sem::Struct>()) {
        for (size_t i = 0; i < str->Members().Length(); ++i) {
            auto* const m = str->Members()[i];
            uint32_t required_align = required_alignment_of(m->Type());

            // Recurse into the member type.
            if (!AddressSpaceLayout(m->Type(), address_space, m->Declaration()->type->source)) {
                AddNote("see layout of struct:\n" + str->Layout(), str->Declaration()->source);
                note_usage();
                return false;
            }

            // Validate that member is at a valid byte offset
            if (m->Offset() % required_align != 0 &&
                !enabled_extensions_.Contains(
                    core::Extension::kChromiumInternalRelaxedUniformLayout)) {
                AddError("the offset of a struct member of type '" +
                             m->Type()->UnwrapRef()->FriendlyName() + "' in address space '" +
                             tint::ToString(address_space) + "' must be a multiple of " +
                             std::to_string(required_align) + " bytes, but '" + member_name_of(m) +
                             "' is currently at offset " + std::to_string(m->Offset()) +
                             ". Consider setting @align(" + std::to_string(required_align) +
                             ") on this member",
                         m->Declaration()->source);

                AddNote("see layout of struct:\n" + str->Layout(), str->Declaration()->source);

                if (auto* member_str = m->Type()->As<sem::Struct>()) {
                    AddNote("and layout of struct member:\n" + member_str->Layout(),
                            member_str->Declaration()->source);
                }

                note_usage();
                return false;
            }

            // For uniform buffers, validate that the number of bytes between the previous member of
            // type struct and the current is a multiple of 16 bytes.
            auto* const prev_member = (i == 0) ? nullptr : str->Members()[i - 1];
            if (prev_member && is_uniform_struct(prev_member->Type())) {
                const uint32_t prev_to_curr_offset = m->Offset() - prev_member->Offset();
                if (prev_to_curr_offset % 16 != 0 &&
                    !enabled_extensions_.Contains(
                        core::Extension::kChromiumInternalRelaxedUniformLayout)) {
                    AddError(
                        "uniform storage requires that the number of bytes between the start of "
                        "the previous member of type struct and the current member be a multiple "
                        "of 16 bytes, but there are currently " +
                            std::to_string(prev_to_curr_offset) + " bytes between '" +
                            member_name_of(prev_member) + "' and '" + member_name_of(m) +
                            "'. Consider setting @align(16) on this member",
                        m->Declaration()->source);

                    AddNote("see layout of struct:\n" + str->Layout(), str->Declaration()->source);

                    auto* prev_member_str = prev_member->Type()->As<sem::Struct>();
                    AddNote("and layout of previous member struct:\n" + prev_member_str->Layout(),
                            prev_member_str->Declaration()->source);
                    note_usage();
                    return false;
                }
            }
        }
    }

    // For uniform buffer array members, validate that array elements are aligned to 16 bytes
    if (auto* arr = store_ty->As<core::type::Array>()) {
        // Recurse into the element type.
        // TODO(crbug.com/tint/1388): Ideally we'd pass the source for nested element type here, but
        // we can't easily get that from the semantic node. We should consider recursing through the
        // AST type nodes instead.
        if (!AddressSpaceLayout(arr->ElemType(), address_space, source)) {
            return false;
        }

        if (address_space == core::AddressSpace::kUniform &&
            !enabled_extensions_.Contains(core::Extension::kChromiumInternalRelaxedUniformLayout)) {
            // We already validated that this array member is itself aligned to 16 bytes above, so
            // we only need to validate that stride is a multiple of 16 bytes.
            if (arr->Stride() % 16 != 0) {
                // Since WGSL has no stride attribute, try to provide a useful hint for how the
                // shader author can resolve the issue.
                std::string hint;
                if (arr->ElemType()->Is<core::type::Scalar>()) {
                    hint = "Consider using a vector or struct as the element type instead.";
                } else if (auto* vec = arr->ElemType()->As<core::type::Vector>();
                           vec && vec->type()->Size() == 4) {
                    hint = "Consider using a vec4 instead.";
                } else if (arr->ElemType()->Is<sem::Struct>()) {
                    hint = "Consider using the @size attribute on the last struct member.";
                } else {
                    hint =
                        "Consider wrapping the element type in a struct and using the @size "
                        "attribute.";
                }
                AddError(
                    "uniform storage requires that array elements are aligned to 16 bytes, but "
                    "array element of type '" +
                        arr->ElemType()->FriendlyName() + "' has a stride of " +
                        std::to_string(arr->Stride()) + " bytes. " + hint,
                    source);
                return false;
            }
        }
    }

    return true;
}

bool Validator::LocalVariable(const sem::Variable* local) const {
    auto* decl = local->Declaration();
    if (IsArrayWithOverrideCount(local->Type())) {
        RaiseArrayWithOverrideCountError(decl->type ? decl->type->source
                                                    : decl->initializer->source);
        return false;
    }
    return Switch(
        decl,  //
        [&](const ast::Var* var) {
            if (IsValidationEnabled(var->attributes,
                                    ast::DisabledValidation::kIgnoreAddressSpace)) {
                if (!local->Type()->UnwrapRef()->IsConstructible()) {
                    AddError("function-scope 'var' must have a constructible type",
                             var->type ? var->type->source : var->source);
                    return false;
                }
            }
            return Var(local);
        },                                            //
        [&](const ast::Let*) { return Let(local); },  //
        [&](const ast::Const*) { return true; },      //
        [&](Default) {
            TINT_ICE() << "Validator::Variable() called with a unknown variable type: "
                       << decl->TypeInfo().name;
            return false;
        });
}

bool Validator::GlobalVariable(
    const sem::GlobalVariable* global,
    const Hashmap<OverrideId, const sem::Variable*, 8>& override_ids) const {
    auto* decl = global->Declaration();
    if (global->AddressSpace() != core::AddressSpace::kWorkgroup &&
        IsArrayWithOverrideCount(global->Type())) {
        RaiseArrayWithOverrideCountError(decl->type ? decl->type->source
                                                    : decl->initializer->source);
        return false;
    }
    bool ok = Switch(
        decl,  //
        [&](const ast::Var* var) {
            if (auto* init = global->Initializer();
                init && init->Stage() > core::EvaluationStage::kOverride) {
                AddError("module-scope 'var' initializer must be a constant or override-expression",
                         init->Declaration()->source);
                return false;
            }

            if (!var->declared_address_space && !global->Type()->UnwrapRef()->is_handle()) {
                AddError(
                    "module-scope 'var' declarations that are not of texture or sampler types must "
                    "provide an address space",
                    decl->source);
                return false;
            }

            return Var(global);
        },
        [&](const ast::Override*) { return Override(global, override_ids); },
        [&](const ast::Const*) { return Const(global); },
        [&](Default) {
            TINT_ICE() << "Validator::GlobalVariable() called with a unknown variable type: "
                       << decl->TypeInfo().name;
            return false;
        });

    if (!ok) {
        return false;
    }

    if (global->AddressSpace() == core::AddressSpace::kFunction) {
        AddError("module-scope 'var' must not use address space 'function'", decl->source);
        return false;
    }

    switch (global->AddressSpace()) {
        case core::AddressSpace::kUniform:
        case core::AddressSpace::kStorage:
        case core::AddressSpace::kHandle: {
            // https://gpuweb.github.io/gpuweb/wgsl/#resource-interface
            // Each resource variable must be declared with both group and binding attributes.
            if (!decl->HasBindingPoint()) {
                AddError("resource variables require @group and @binding attributes", decl->source);
                return false;
            }
            break;
        }
        default: {
            auto* binding_attr = ast::GetAttribute<ast::BindingAttribute>(decl->attributes);
            auto* group_attr = ast::GetAttribute<ast::GroupAttribute>(decl->attributes);
            if (binding_attr || group_attr) {
                // https://gpuweb.github.io/gpuweb/wgsl/#attribute-binding
                // Must only be applied to a resource variable
                AddError("non-resource variables must not have @group or @binding attributes",
                         decl->source);
                return false;
            }
        }
    }

    return true;
}

bool Validator::Var(const sem::Variable* v) const {
    auto* var = v->Declaration()->As<ast::Var>();
    auto* store_ty = v->Type()->UnwrapRef();

    if (!IsStorable(store_ty)) {
        AddError(sem_.TypeNameOf(store_ty) + " cannot be used as the type of a var", var->source);
        return false;
    }

    if (store_ty->is_handle() && var->declared_address_space) {
        // https://gpuweb.github.io/gpuweb/wgsl/#module-scope-variables
        // If the store type is a texture type or a sampler type, then the variable declaration must
        // not have a address space attribute. The address space will always be handle.
        AddError("variables of type '" + sem_.TypeNameOf(store_ty) +
                     "' must not specifiy an address space",
                 var->source);
        return false;
    }

    if (var->declared_access) {
        // https://www.w3.org/TR/WGSL/#access-mode-defaults
        // When writing a variable declaration or a pointer type in WGSL source:
        // * For the storage address space, the access mode is optional, and defaults to read.
        // * For other address spaces, the access mode must not be written.
        if (v->AddressSpace() != core::AddressSpace::kStorage) {
            AddError("only variables in <storage> address space may specify an access mode",
                     var->source);
            return false;
        }
    }

    if (var->initializer) {
        switch (v->AddressSpace()) {
            case core::AddressSpace::kPrivate:
            case core::AddressSpace::kFunction:
                break;  // Allowed an initializer
            default:
                // https://gpuweb.github.io/gpuweb/wgsl/#var-and-let
                // Optionally has an initializer expression, if the variable is in the private or
                // function address spaces.
                AddError("var of address space '" + tint::ToString(v->AddressSpace()) +
                             "' cannot have an initializer. var initializers are only supported "
                             "for the address spaces 'private' and 'function'",
                         var->source);
                return false;
        }
    }

    if (!CheckTypeAccessAddressSpace(v->Type()->UnwrapRef(), v->Access(), v->AddressSpace(),
                                     var->attributes, var->source)) {
        return false;
    }

    if (IsValidationEnabled(var->attributes, ast::DisabledValidation::kIgnoreAddressSpace) &&
        (v->AddressSpace() == core::AddressSpace::kIn ||
         v->AddressSpace() == core::AddressSpace::kOut)) {
        AddError("invalid use of input/output address space", var->source);
        return false;
    }
    return true;
}

bool Validator::Let(const sem::Variable* v) const {
    auto* decl = v->Declaration();
    auto* storage_ty = v->Type()->UnwrapRef();

    if (!(storage_ty->IsConstructible() || storage_ty->Is<core::type::Pointer>())) {
        AddError(sem_.TypeNameOf(storage_ty) + " cannot be used as the type of a 'let'",
                 decl->source);
        return false;
    }
    return true;
}

bool Validator::Override(const sem::GlobalVariable* v,
                         const Hashmap<OverrideId, const sem::Variable*, 8>& override_ids) const {
    auto* decl = v->Declaration();
    auto* storage_ty = v->Type()->UnwrapRef();

    if (auto* init = v->Initializer(); init && init->Stage() > core::EvaluationStage::kOverride) {
        AddError("'override' initializer must be an override-expression",
                 init->Declaration()->source);
        return false;
    }

    for (auto* attr : decl->attributes) {
        if (attr->Is<ast::IdAttribute>()) {
            auto id = v->OverrideId();
            if (auto var = override_ids.Find(id); var && *var != v) {
                AddError("@id values must be unique", attr->source);
                AddNote(
                    "a override with an ID of " + std::to_string(id.value) +
                        " was previously declared here:",
                    ast::GetAttribute<ast::IdAttribute>((*var)->Declaration()->attributes)->source);
                return false;
            }
        }
    }

    if (!storage_ty->Is<core::type::Scalar>()) {
        AddError(sem_.TypeNameOf(storage_ty) + " cannot be used as the type of a 'override'",
                 decl->source);
        return false;
    }

    return true;
}

bool Validator::Const(const sem::Variable*) const {
    return true;
}

bool Validator::Parameter(const sem::Variable* var) const {
    auto* decl = var->Declaration();

    if (IsValidationDisabled(decl->attributes, ast::DisabledValidation::kFunctionParameter)) {
        return true;
    }

    if (auto* ref = var->Type()->As<core::type::Pointer>()) {
        if (IsValidationEnabled(decl->attributes, ast::DisabledValidation::kIgnoreAddressSpace)) {
            bool ok = false;

            auto sc = ref->AddressSpace();
            switch (sc) {
                case core::AddressSpace::kFunction:
                case core::AddressSpace::kPrivate:
                    ok = true;
                    break;
                case core::AddressSpace::kStorage:
                case core::AddressSpace::kUniform:
                case core::AddressSpace::kWorkgroup:
                    ok = enabled_extensions_.Contains(
                        core::Extension::kChromiumExperimentalFullPtrParameters);
                    break;
                default:
                    break;
            }
            if (!ok) {
                StringStream ss;
                ss << "function parameter of pointer type cannot be in '" << sc
                   << "' address space";
                AddError(ss.str(), decl->source);
                return false;
            }
        }
    }

    if (IsPlain(var->Type())) {
        if (!var->Type()->IsConstructible()) {
            AddError("type of function parameter must be constructible", decl->type->source);
            return false;
        }
    } else if (!var->Type()
                    ->IsAnyOf<core::type::Texture, core::type::Sampler, core::type::Pointer>()) {
        AddError("type of function parameter cannot be " + sem_.TypeNameOf(var->Type()),
                 decl->source);
        return false;
    }

    return true;
}

bool Validator::BuiltinAttribute(const ast::BuiltinAttribute* attr,
                                 const core::type::Type* storage_ty,
                                 ast::PipelineStage stage,
                                 const bool is_input) const {
    auto* type = storage_ty->UnwrapRef();
    StringStream stage_name;
    stage_name << stage;
    bool is_stage_mismatch = false;
    bool is_output = !is_input;
    auto builtin = sem_.Get(attr)->Value();
    switch (builtin) {
        case core::BuiltinValue::kPosition:
            if (stage != ast::PipelineStage::kNone &&
                !((is_input && stage == ast::PipelineStage::kFragment) ||
                  (is_output && stage == ast::PipelineStage::kVertex))) {
                is_stage_mismatch = true;
            }
            if (!(type->is_float_vector() && type->As<core::type::Vector>()->Width() == 4)) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'vec4<f32>'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kGlobalInvocationId:
        case core::BuiltinValue::kLocalInvocationId:
        case core::BuiltinValue::kNumWorkgroups:
        case core::BuiltinValue::kWorkgroupId:
            if (stage != ast::PipelineStage::kNone &&
                !(stage == ast::PipelineStage::kCompute && is_input)) {
                is_stage_mismatch = true;
            }
            if (!(type->is_unsigned_integer_vector() &&
                  type->As<core::type::Vector>()->Width() == 3)) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'vec3<u32>'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kFragDepth:
            if (stage != ast::PipelineStage::kNone &&
                !(stage == ast::PipelineStage::kFragment && !is_input)) {
                is_stage_mismatch = true;
            }
            if (!type->Is<core::type::F32>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'f32'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kFrontFacing:
            if (stage != ast::PipelineStage::kNone &&
                !(stage == ast::PipelineStage::kFragment && is_input)) {
                is_stage_mismatch = true;
            }
            if (!type->Is<core::type::Bool>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'bool'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kLocalInvocationIndex:
            if (stage != ast::PipelineStage::kNone &&
                !(stage == ast::PipelineStage::kCompute && is_input)) {
                is_stage_mismatch = true;
            }
            if (!type->Is<core::type::U32>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'u32'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kVertexIndex:
        case core::BuiltinValue::kInstanceIndex:
            if (stage != ast::PipelineStage::kNone &&
                !(stage == ast::PipelineStage::kVertex && is_input)) {
                is_stage_mismatch = true;
            }
            if (!type->Is<core::type::U32>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'u32'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kSampleMask:
            if (stage != ast::PipelineStage::kNone && !(stage == ast::PipelineStage::kFragment)) {
                is_stage_mismatch = true;
            }
            if (!type->Is<core::type::U32>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'u32'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kSampleIndex:
            if (stage != ast::PipelineStage::kNone &&
                !(stage == ast::PipelineStage::kFragment && is_input)) {
                is_stage_mismatch = true;
            }
            if (!type->Is<core::type::U32>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'u32'";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        case core::BuiltinValue::kSubgroupInvocationId:
        case core::BuiltinValue::kSubgroupSize:
            if (!enabled_extensions_.Contains(core::Extension::kChromiumExperimentalSubgroups)) {
                StringStream err;
                err << "use of @builtin(" << builtin
                    << ") attribute requires enabling extension 'chromium_experimental_subgroups'";
                AddError(err.str(), attr->source);
                return false;
            }
            if (!type->Is<core::type::U32>()) {
                StringStream err;
                err << "store type of @builtin(" << builtin << ") must be 'u32'";
                AddError(err.str(), attr->source);
                return false;
            }
            if (stage != ast::PipelineStage::kNone && stage != ast::PipelineStage::kCompute) {
                StringStream err;
                err << "@builtin(" << builtin << ") is only valid as a compute shader input";
                AddError(err.str(), attr->source);
                return false;
            }
            break;
        default:
            break;
    }

    if (is_stage_mismatch) {
        StringStream err;
        err << "@builtin(" << builtin << ") cannot be used in "
            << (is_input ? "input of " : "output of ") << stage_name.str() << " pipeline stage";
        AddError(err.str(), attr->source);
        return false;
    }

    return true;
}

bool Validator::InterpolateAttribute(const ast::InterpolateAttribute* attr,
                                     const core::type::Type* storage_ty) const {
    auto* type = storage_ty->UnwrapRef();

    auto i_type = sem_.AsInterpolationType(sem_.Get(attr->type));
    if (TINT_UNLIKELY(!i_type)) {
        return false;
    }

    if (type->is_integer_scalar_or_vector() && i_type->Value() != core::InterpolationType::kFlat) {
        AddError("interpolation type must be 'flat' for integral user-defined IO types",
                 attr->source);
        return false;
    }

    if (attr->sampling && i_type->Value() == core::InterpolationType::kFlat) {
        AddError("flat interpolation attribute must not have a sampling parameter", attr->source);
        return false;
    }

    return true;
}

bool Validator::Function(const sem::Function* func, ast::PipelineStage stage) const {
    auto* decl = func->Declaration();

    for (auto* attr : decl->attributes) {
        bool ok = Switch(
            attr,  //
            [&](const ast::WorkgroupAttribute*) {
                if (decl->PipelineStage() != ast::PipelineStage::kCompute) {
                    AddError("@workgroup_size is only valid for compute stages", attr->source);
                    return false;
                }
                return true;
            },
            [&](const ast::MustUseAttribute*) {
                if (func->ReturnType()->Is<core::type::Void>()) {
                    AddError("@must_use can only be applied to functions that return a value",
                             attr->source);
                    return false;
                }
                return true;
            },
            [&](Default) { return true; });
        if (!ok) {
            return false;
        }
    }

    if (decl->params.Length() > kMaxFunctionParameters) {
        AddError("function declares " + std::to_string(decl->params.Length()) +
                     " parameters, maximum is " + std::to_string(kMaxFunctionParameters),
                 decl->source);
        return false;
    }

    if (!func->ReturnType()->Is<core::type::Void>()) {
        if (!func->ReturnType()->IsConstructible()) {
            AddError("function return type must be a constructible type",
                     decl->return_type->source);
            return false;
        }

        if (decl->body) {
            sem::Behaviors behaviors{sem::Behavior::kNext};
            if (auto* last = decl->body->Last()) {
                behaviors = sem_.Get(last)->Behaviors();
            }
            if (behaviors.Contains(sem::Behavior::kNext)) {
                AddError("missing return at end of function", decl->source);
                return false;
            }
        } else if (TINT_UNLIKELY(IsValidationEnabled(
                       decl->attributes, ast::DisabledValidation::kFunctionHasNoBody))) {
            TINT_ICE() << "Function " << decl->name->symbol.Name() << " has no body";
        }
    }

    if (decl->IsEntryPoint()) {
        if (!EntryPoint(func, stage)) {
            return false;
        }
    }

    // https://www.w3.org/TR/WGSL/#behaviors-rules
    // a function behavior is always one of {}, or {Next}.
    if (TINT_UNLIKELY(func->Behaviors() != sem::Behaviors{} &&
                      func->Behaviors() != sem::Behavior::kNext)) {
        auto name = decl->name->symbol.Name();
        TINT_ICE() << "function '" << name << "' behaviors are: " << func->Behaviors();
    }

    return true;
}

bool Validator::EntryPoint(const sem::Function* func, ast::PipelineStage stage) const {
    auto* decl = func->Declaration();

    // Use a lambda to validate the entry point attributes for a type.
    // Persistent state is used to track which builtins and locations have already been seen, in
    // order to catch conflicts.
    // TODO(jrprice): This state could be stored in sem::Function instead, and then passed to
    // sem::Function since it would be useful there too.
    Hashset<core::BuiltinValue, 4> builtins;
    Hashset<std::pair<uint32_t, uint32_t>, 8> locationsAndIndexes;
    enum class ParamOrRetType {
        kParameter,
        kReturnType,
    };

    // Inner lambda that is applied to a type and all of its members.
    auto validate_entry_point_attributes_inner = [&](VectorRef<const ast::Attribute*> attrs,
                                                     const core::type::Type* ty, Source source,
                                                     ParamOrRetType param_or_ret,
                                                     bool is_struct_member,
                                                     std::optional<uint32_t> location,
                                                     std::optional<uint32_t> index) {
        // Scan attributes for pipeline IO attributes.
        // Check for overlap with attributes that have been seen previously.
        const ast::Attribute* pipeline_io_attribute = nullptr;
        const ast::LocationAttribute* location_attribute = nullptr;
        const ast::IndexAttribute* index_attribute = nullptr;
        const ast::InterpolateAttribute* interpolate_attribute = nullptr;
        const ast::InvariantAttribute* invariant_attribute = nullptr;
        for (auto* attr : attrs) {
            auto is_invalid_compute_shader_attribute = false;

            if (auto* builtin_attr = attr->As<ast::BuiltinAttribute>()) {
                auto builtin = sem_.Get(builtin_attr)->Value();

                if (pipeline_io_attribute) {
                    AddError("multiple entry point IO attributes", attr->source);
                    AddNote("previously consumed " + AttrToStr(pipeline_io_attribute),
                            pipeline_io_attribute->source);
                    return false;
                }
                pipeline_io_attribute = attr;

                if (builtins.Contains(builtin)) {
                    StringStream err;
                    err << "@builtin(" << builtin << ") appears multiple times as pipeline "
                        << (param_or_ret == ParamOrRetType::kParameter ? "input" : "output");
                    AddError(err.str(), decl->source);
                    return false;
                }

                if (!BuiltinAttribute(builtin_attr, ty, stage,
                                      /* is_input */ param_or_ret == ParamOrRetType::kParameter)) {
                    return false;
                }
                builtins.Add(builtin);
            } else if (auto* loc_attr = attr->As<ast::LocationAttribute>()) {
                location_attribute = loc_attr;
                if (pipeline_io_attribute) {
                    AddError("multiple entry point IO attributes", attr->source);
                    AddNote("previously consumed " + AttrToStr(pipeline_io_attribute),
                            pipeline_io_attribute->source);
                    return false;
                }
                pipeline_io_attribute = attr;

                bool is_input = param_or_ret == ParamOrRetType::kParameter;

                if (TINT_UNLIKELY(!location.has_value())) {
                    TINT_ICE() << "Location has no value";
                    return false;
                }

                if (!LocationAttribute(loc_attr, ty, stage, source, is_input)) {
                    return false;
                }
            } else if (auto* index_attr = attr->As<ast::IndexAttribute>()) {
                index_attribute = index_attr;
                return IndexAttribute(index_attr, stage);
            } else if (auto* interpolate = attr->As<ast::InterpolateAttribute>()) {
                if (decl->PipelineStage() == ast::PipelineStage::kCompute) {
                    is_invalid_compute_shader_attribute = true;
                } else if (!InterpolateAttribute(interpolate, ty)) {
                    return false;
                }
                interpolate_attribute = interpolate;
            } else if (auto* invariant = attr->As<ast::InvariantAttribute>()) {
                if (decl->PipelineStage() == ast::PipelineStage::kCompute) {
                    is_invalid_compute_shader_attribute = true;
                }
                invariant_attribute = invariant;
            }
            if (is_invalid_compute_shader_attribute) {
                std::string input_or_output =
                    param_or_ret == ParamOrRetType::kParameter ? "inputs" : "output";
                AddError("@" + attr->Name() + " is not valid for compute shader " + input_or_output,
                         attr->source);
                return false;
            }
        }

        if (IsValidationEnabled(attrs, ast::DisabledValidation::kEntryPointParameter)) {
            if (is_struct_member && ty->Is<core::type::Struct>()) {
                AddError("nested structures cannot be used for entry point IO", source);
                return false;
            }

            if (!ty->Is<core::type::Struct>() && !pipeline_io_attribute) {
                std::string err = "missing entry point IO attribute";
                if (!is_struct_member) {
                    err += (param_or_ret == ParamOrRetType::kParameter ? " on parameter"
                                                                       : " on return type");
                }
                AddError(err, source);
                return false;
            }

            if (pipeline_io_attribute && pipeline_io_attribute->Is<ast::LocationAttribute>()) {
                if (ty->is_integer_scalar_or_vector() && !interpolate_attribute) {
                    if (decl->PipelineStage() == ast::PipelineStage::kVertex &&
                        param_or_ret == ParamOrRetType::kReturnType) {
                        AddError(
                            "integral user-defined vertex outputs must have a flat interpolation "
                            "attribute",
                            source);
                        return false;
                    }
                    if (decl->PipelineStage() == ast::PipelineStage::kFragment &&
                        param_or_ret == ParamOrRetType::kParameter) {
                        AddError(
                            "integral user-defined fragment inputs must have a flat interpolation "
                            "attribute",
                            source);
                        return false;
                    }
                }
            }

            if (index_attribute) {
                if (Is<ast::LocationAttribute>(pipeline_io_attribute)) {
                    AddError("index attribute must only be used with @location",
                             index_attribute->source);
                    return false;
                }

                // Because HLSL specifies dual source blending targets with SV_Target0 and 1, we
                // should restrict targets with index attributes to location 0 for easy translation
                // in the backend writers.
                if (location.value() != 0) {
                    AddError("index attribute must only be used with @location(0)",
                             index_attribute->source);
                    return false;
                }
            }

            if (location_attribute) {
                uint32_t idx = 0xffffffff;
                if (index_attribute) {
                    idx = index.value();
                }

                std::pair<uint32_t, uint32_t> locationAndIndex(location.value(), idx);
                if (!locationsAndIndexes.Add(locationAndIndex)) {
                    StringStream err;
                    if (!index_attribute) {
                        err << "@location(" << location.value() << ") appears multiple times";
                    } else {
                        err << "@location(" << location.value() << ") @index(" << index.value()
                            << ") appears multiple times";
                    }
                    AddError(err.str(), location_attribute->source);
                    return false;
                }
            }

            if (interpolate_attribute) {
                if (!pipeline_io_attribute ||
                    !pipeline_io_attribute->Is<ast::LocationAttribute>()) {
                    AddError("interpolate attribute must only be used with @location",
                             interpolate_attribute->source);
                    return false;
                }
            }

            if (invariant_attribute) {
                bool has_position = false;
                if (pipeline_io_attribute) {
                    if (auto* builtin_attr = pipeline_io_attribute->As<ast::BuiltinAttribute>()) {
                        auto builtin = sem_.Get(builtin_attr)->Value();
                        has_position = (builtin == core::BuiltinValue::kPosition);
                    }
                }
                if (!has_position) {
                    AddError("invariant attribute must only be applied to a position builtin",
                             invariant_attribute->source);
                    return false;
                }
            }
        }
        return true;
    };

    // Outer lambda for validating the entry point attributes for a type.
    auto validate_entry_point_attributes = [&](VectorRef<const ast::Attribute*> attrs,
                                               const core::type::Type* ty, Source source,
                                               ParamOrRetType param_or_ret,
                                               std::optional<uint32_t> location,
                                               std::optional<uint32_t> index) {
        if (!validate_entry_point_attributes_inner(attrs, ty, source, param_or_ret,
                                                   /*is_struct_member*/ false, location, index)) {
            return false;
        }

        if (auto* str = ty->As<sem::Struct>()) {
            for (auto* member : str->Members()) {
                if (!validate_entry_point_attributes_inner(
                        member->Declaration()->attributes, member->Type(),
                        member->Declaration()->source, param_or_ret,
                        /*is_struct_member*/ true, member->Attributes().location,
                        member->Attributes().index)) {
                    AddNote("while analyzing entry point '" + decl->name->symbol.Name() + "'",
                            decl->source);
                    return false;
                }
            }
        }

        return true;
    };

    for (auto* param : func->Parameters()) {
        auto* param_decl = param->Declaration();
        if (!validate_entry_point_attributes(param_decl->attributes, param->Type(),
                                             param_decl->source, ParamOrRetType::kParameter,
                                             param->Location(), std::nullopt)) {
            return false;
        }
    }

    // Clear IO sets after parameter validation. Builtin and location attributes in return types
    // should be validated independently from those used in parameters.
    builtins.Clear();
    locationsAndIndexes.Clear();

    if (!func->ReturnType()->Is<core::type::Void>()) {
        if (!validate_entry_point_attributes(decl->return_type_attributes, func->ReturnType(),
                                             decl->source, ParamOrRetType::kReturnType,
                                             func->ReturnLocation(), func->ReturnIndex())) {
            return false;
        }
    }

    if (decl->PipelineStage() == ast::PipelineStage::kVertex &&
        !builtins.Contains(core::BuiltinValue::kPosition)) {
        // Check module-scope variables, as the SPIR-V sanitizer generates these.
        bool found = false;
        for (auto* global : func->TransitivelyReferencedGlobals()) {
            if (auto* builtin_attr =
                    ast::GetAttribute<ast::BuiltinAttribute>(global->Declaration()->attributes)) {
                auto builtin = sem_.Get(builtin_attr)->Value();
                if (builtin == core::BuiltinValue::kPosition) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            AddError("a vertex shader must include the 'position' builtin in its return type",
                     decl->source);
            return false;
        }
    }

    if (decl->PipelineStage() == ast::PipelineStage::kCompute) {
        if (!ast::HasAttribute<ast::WorkgroupAttribute>(decl->attributes)) {
            AddError("a compute shader must include 'workgroup_size' in its attributes",
                     decl->source);
            return false;
        }
    }

    // Validate there are no resource variable binding collisions
    Hashmap<BindingPoint, const ast::Variable*, 8> binding_points;
    for (auto* global : func->TransitivelyReferencedGlobals()) {
        auto* var_decl = global->Declaration()->As<ast::Var>();
        if (!var_decl) {
            continue;
        }
        auto bp = global->BindingPoint();
        if (!bp) {
            continue;
        }
        if (auto added = binding_points.Add(*bp, var_decl);
            !added &&
            IsValidationEnabled(decl->attributes,
                                ast::DisabledValidation::kBindingPointCollision) &&
            IsValidationEnabled((*added.value)->attributes,
                                ast::DisabledValidation::kBindingPointCollision)) {
            // https://gpuweb.github.io/gpuweb/wgsl/#resource-interface
            // Bindings must not alias within a shader stage: two different variables in the
            // resource interface of a given shader must not have the same group and binding values,
            // when considered as a pair of values.
            auto func_name = decl->name->symbol.Name();
            AddError(
                "entry point '" + func_name +
                    "' references multiple variables that use the same resource binding @group(" +
                    std::to_string(bp->group) + "), @binding(" + std::to_string(bp->binding) + ")",
                var_decl->source);
            AddNote("first resource binding usage declared here", (*added.value)->source);
            return false;
        }
    }

    return true;
}

bool Validator::EvaluationStage(const sem::ValueExpression* expr,
                                core::EvaluationStage latest_stage,
                                std::string_view constraint) const {
    if (expr->Stage() == core::EvaluationStage::kNotEvaluated) {
        return true;
    }
    if (expr->Stage() > latest_stage) {
        auto stage_name = [](core::EvaluationStage stage) -> std::string {
            switch (stage) {
                case core::EvaluationStage::kRuntime:
                    return "a runtime-expression";
                case core::EvaluationStage::kOverride:
                    return "an override-expression";
                case core::EvaluationStage::kConstant:
                    return "a const-expression";
                case core::EvaluationStage::kNotEvaluated:
                    return "an unevaluated expression";
            }
            return "<unknown>";
        };

        AddError(std::string(constraint) + " requires " + stage_name(latest_stage) +
                     ", but expression is " + stage_name(expr->Stage()),
                 expr->Declaration()->source);

        if (auto* stmt = expr->Stmt()) {
            if (auto* decl = As<ast::VariableDeclStatement>(stmt->Declaration())) {
                if (decl->variable->Is<ast::Const>()) {
                    AddNote("consider changing 'const' to 'let'", decl->source);
                }
            }
        }
        return false;
    }
    return true;
}

bool Validator::Statements(VectorRef<const ast::Statement*> stmts) const {
    for (auto* stmt : stmts) {
        if (!sem_.Get(stmt)->IsReachable()) {
            if (!AddDiagnostic(core::ChromiumDiagnosticRule::kUnreachableCode,
                               "code is unreachable", stmt->source)) {
                return false;
            }
            break;
        }
    }
    return true;
}

bool Validator::Bitcast(const ast::BitcastExpression* cast, const core::type::Type* to) const {
    auto* from = sem_.TypeOf(cast->expr)->UnwrapRef();
    if (!from->is_numeric_scalar_or_vector()) {
        AddError("'" + sem_.TypeNameOf(from) + "' cannot be bitcast", cast->expr->source);
        return false;
    }
    if (!to->is_numeric_scalar_or_vector()) {
        AddError("cannot bitcast to '" + sem_.TypeNameOf(to) + "'", cast->type->source);
        return false;
    }

    // Only bitcasts between scalar/vector types of the same bit width are allowed.
    if (from->Size() != to->Size()) {
        AddError(
            "cannot bitcast from '" + sem_.TypeNameOf(from) + "' to '" + sem_.TypeNameOf(to) + "'",
            cast->source);
        return false;
    }

    return true;
}

bool Validator::BreakStatement(const sem::Statement* stmt,
                               sem::Statement* current_statement) const {
    if (!stmt->FindFirstParent<sem::LoopBlockStatement, sem::CaseStatement>()) {
        AddError("break statement must be in a loop or switch case", stmt->Declaration()->source);
        return false;
    }
    if (ClosestContinuing(/*stop_at_loop*/ true, current_statement) != nullptr) {
        AddError(
            "`break` must not be used to exit from a continuing block. Use `break-if` instead.",
            stmt->Declaration()->source);
        return false;
    }
    return true;
}

bool Validator::ContinueStatement(const sem::Statement* stmt,
                                  sem::Statement* current_statement) const {
    if (auto* continuing = ClosestContinuing(/*stop_at_loop*/ true, current_statement)) {
        AddError("continuing blocks must not contain a continue statement",
                 stmt->Declaration()->source);
        if (continuing != stmt->Declaration() && continuing != stmt->Parent()->Declaration()) {
            AddNote("see continuing block here", continuing->source);
        }
        return false;
    }

    if (!stmt->FindFirstParent<sem::LoopBlockStatement>()) {
        AddError("continue statement must be in a loop", stmt->Declaration()->source);
        return false;
    }

    return true;
}

bool Validator::Call(const sem::Call* call, sem::Statement* current_statement) const {
    if (!call->Target()->MustUse()) {
        return true;
    }

    auto* expr = call->Declaration();
    bool is_call_stmt =
        current_statement && Is<ast::CallStatement>(current_statement->Declaration(),
                                                    [&](auto* stmt) { return stmt->expr == expr; });
    if (is_call_stmt) {
        // Call target is annotated with @must_use, but was used as a call statement.
        Switch(
            call->Target(),  //
            [&](const sem::Function* fn) {
                AddError("ignoring return value of function '" +
                             fn->Declaration()->name->symbol.Name() + "' annotated with @must_use",
                         call->Declaration()->source);
                sem_.NoteDeclarationSource(fn->Declaration());
            },
            [&](const sem::Builtin* b) {
                AddError("ignoring return value of builtin '" + tint::ToString(b->Type()) + "'",
                         call->Declaration()->source);
            },
            [&](const sem::ValueConversion*) {
                AddError("value conversion evaluated but not used", call->Declaration()->source);
            },
            [&](const sem::ValueConstructor*) {
                AddError("value constructor evaluated but not used", call->Declaration()->source);
            },
            [&](Default) {
                AddError("return value of call not used", call->Declaration()->source);
            });
        return false;
    }

    return true;
}

bool Validator::LoopStatement(const sem::LoopStatement* stmt) const {
    if (stmt->Behaviors().Empty()) {
        AddError("loop does not exit", stmt->Declaration()->source.Begin());
        return false;
    }
    return true;
}

bool Validator::ForLoopStatement(const sem::ForLoopStatement* stmt) const {
    if (stmt->Behaviors().Empty()) {
        AddError("for-loop does not exit", stmt->Declaration()->source.Begin());
        return false;
    }
    if (auto* cond = stmt->Condition()) {
        auto* cond_ty = cond->Type()->UnwrapRef();
        if (!cond_ty->Is<core::type::Bool>()) {
            AddError("for-loop condition must be bool, got " + sem_.TypeNameOf(cond_ty),
                     stmt->Condition()->Declaration()->source);
            return false;
        }
    }
    return true;
}

bool Validator::WhileStatement(const sem::WhileStatement* stmt) const {
    if (stmt->Behaviors().Empty()) {
        AddError("while does not exit", stmt->Declaration()->source.Begin());
        return false;
    }
    if (auto* cond = stmt->Condition()) {
        auto* cond_ty = cond->Type()->UnwrapRef();
        if (!cond_ty->Is<core::type::Bool>()) {
            AddError("while condition must be bool, got " + sem_.TypeNameOf(cond_ty),
                     stmt->Condition()->Declaration()->source);
            return false;
        }
    }
    return true;
}

bool Validator::BreakIfStatement(const sem::BreakIfStatement* stmt,
                                 sem::Statement* current_statement) const {
    auto* cond_ty = stmt->Condition()->Type()->UnwrapRef();
    if (!cond_ty->Is<core::type::Bool>()) {
        AddError("break-if statement condition must be bool, got " + sem_.TypeNameOf(cond_ty),
                 stmt->Condition()->Declaration()->source);
        return false;
    }

    for (const auto* s = current_statement; s != nullptr; s = s->Parent()) {
        if (s->Is<sem::LoopStatement>()) {
            break;
        }
        if (auto* continuing = s->As<sem::LoopContinuingBlockStatement>()) {
            if (continuing->Declaration()->statements.Back() != stmt->Declaration()) {
                AddError("break-if must be the last statement in a continuing block",
                         stmt->Declaration()->source);
                AddNote("see continuing block here", s->Declaration()->source);
                return false;
            }
            return true;
        }
    }

    AddError("break-if must be in a continuing block", stmt->Declaration()->source);
    return false;
}

bool Validator::IfStatement(const sem::IfStatement* stmt) const {
    auto* cond_ty = stmt->Condition()->Type()->UnwrapRef();
    if (!cond_ty->Is<core::type::Bool>()) {
        AddError("if statement condition must be bool, got " + sem_.TypeNameOf(cond_ty),
                 stmt->Condition()->Declaration()->source);
        return false;
    }
    return true;
}

bool Validator::BuiltinCall(const sem::Call* call) const {
    if (call->Type()->Is<core::type::Void>()) {
        bool is_call_statement = false;
        // Some built-in call are not owned by a statement, e.g. a built-in called in global
        // variable declaration. Calling no-return-value built-in in these context is invalid as
        // well.
        if (auto* call_stmt = call->Stmt()) {
            if (auto* call_stmt_ast = As<ast::CallStatement>(call_stmt->Declaration())) {
                if (call_stmt_ast->expr == call->Declaration()) {
                    is_call_statement = true;
                }
            }
        }
        if (!is_call_statement) {
            // https://gpuweb.github.io/gpuweb/wgsl/#function-call-expr
            // If the called function does not return a value, a function call statement should be
            // used instead.
            auto* builtin = call->Target()->As<sem::Builtin>();
            auto name = tint::ToString(builtin->Type());
            AddError("builtin '" + name + "' does not return a value", call->Declaration()->source);
            return false;
        }
    }

    return true;
}

bool Validator::TextureBuiltinFunction(const sem::Call* call) const {
    auto* builtin = call->Target()->As<sem::Builtin>();
    if (!builtin) {
        return false;
    }

    std::string func_name = builtin->str();
    auto& signature = builtin->Signature();

    auto check_arg_is_constexpr = [&](core::ParameterUsage usage, int min, int max) {
        auto signed_index = signature.IndexOf(usage);
        if (signed_index < 0) {
            return true;
        }
        auto index = static_cast<size_t>(signed_index);
        std::string name{core::ToString(usage)};
        auto* arg = call->Arguments()[index];
        if (auto values = arg->ConstantValue()) {
            if (auto* vector = values->Type()->As<core::type::Vector>()) {
                for (size_t i = 0; i < vector->Width(); i++) {
                    auto value = values->Index(i)->ValueAs<AInt>();
                    if (value < min || value > max) {
                        AddError("each component of the " + name + " argument must be at least " +
                                     std::to_string(min) + " and at most " + std::to_string(max) +
                                     ". " + name + " component " + std::to_string(i) + " is " +
                                     std::to_string(value),
                                 arg->Declaration()->source);
                        return false;
                    }
                }
            } else {
                auto value = values->ValueAs<AInt>();
                if (value < min || value > max) {
                    AddError("the " + name + " argument must be at least " + std::to_string(min) +
                                 " and at most " + std::to_string(max) + ". " + name + " is " +
                                 std::to_string(value),
                             arg->Declaration()->source);
                    return false;
                }
            }
            return true;
        }
        AddError("the " + name + " argument must be a const-expression",
                 arg->Declaration()->source);
        return false;
    };

    return check_arg_is_constexpr(core::ParameterUsage::kOffset, -8, 7) &&
           check_arg_is_constexpr(core::ParameterUsage::kComponent, 0, 3);
}

bool Validator::WorkgroupUniformLoad(const sem::Call* call) const {
    auto* builtin = call->Target()->As<sem::Builtin>();
    if (!builtin) {
        return false;
    }

    TINT_ASSERT(call->Arguments().Length() > 0);
    auto* arg = call->Arguments()[0];
    auto* ptr = arg->Type()->As<core::type::Pointer>();
    TINT_ASSERT(ptr != nullptr);
    auto* ty = ptr->StoreType();

    if (ty->Is<core::type::Atomic>() || atomic_composite_info_.Contains(ty)) {
        AddError(
            "workgroupUniformLoad must not be called with an argument that contains an atomic type",
            arg->Declaration()->source);
        return false;
    }

    return true;
}

bool Validator::RequiredExtensionForBuiltinFunction(const sem::Call* call) const {
    const auto* builtin = call->Target()->As<sem::Builtin>();
    if (!builtin) {
        return true;
    }

    const auto extension = builtin->RequiredExtension();
    if (extension == core::Extension::kUndefined) {
        return true;
    }

    if (!enabled_extensions_.Contains(extension)) {
        AddError("cannot call built-in function '" + std::string(builtin->str()) +
                     "' without extension " + tint::ToString(extension),
                 call->Declaration()->source);
        return false;
    }

    return true;
}

bool Validator::CheckF16Enabled(const Source& source) const {
    // Validate if f16 type is allowed.
    if (!enabled_extensions_.Contains(core::Extension::kF16)) {
        AddError("f16 type used without 'f16' extension enabled", source);
        return false;
    }
    return true;
}

bool Validator::FunctionCall(const sem::Call* call, sem::Statement* current_statement) const {
    auto* decl = call->Declaration();
    auto* target = call->Target()->As<sem::Function>();
    auto sym = target->Declaration()->name->symbol;
    auto name = sym.Name();

    if (!current_statement) {  // Function call at module-scope.
        AddError("functions cannot be called at module-scope", decl->source);
        return false;
    }

    if (target->Declaration()->IsEntryPoint()) {
        // https://www.w3.org/TR/WGSL/#function-restriction
        // An entry point must never be the target of a function call.
        AddError("entry point functions cannot be the target of a function call", decl->source);
        return false;
    }

    if (decl->args.Length() != target->Parameters().Length()) {
        bool more = decl->args.Length() > target->Parameters().Length();
        AddError("too " + (more ? std::string("many") : std::string("few")) +
                     " arguments in call to '" + name + "', expected " +
                     std::to_string(target->Parameters().Length()) + ", got " +
                     std::to_string(call->Arguments().Length()),
                 decl->source);
        return false;
    }

    for (size_t i = 0; i < call->Arguments().Length(); ++i) {
        const sem::Variable* param = target->Parameters()[i];
        const ast::Expression* arg_expr = decl->args[i];
        auto* param_type = param->Type();
        auto* arg_type = sem_.TypeOf(arg_expr)->UnwrapRef();

        if (param_type != arg_type) {
            AddError("type mismatch for argument " + std::to_string(i + 1) + " in call to '" +
                         name + "', expected '" + sem_.TypeNameOf(param_type) + "', got '" +
                         sem_.TypeNameOf(arg_type) + "'",
                     arg_expr->source);
            return false;
        }

        if (param_type->Is<core::type::Pointer>() &&
            !enabled_extensions_.Contains(
                core::Extension::kChromiumExperimentalFullPtrParameters)) {
            // https://gpuweb.github.io/gpuweb/wgsl/#function-restriction
            // Each argument of pointer type to a user-defined function must have the same memory
            // view as its root identifier.
            // We can validate this by just comparing the store type of the argument with that of
            // its root identifier, as these will match iff the memory view is the same.
            auto* arg_store_type = arg_type->As<core::type::Pointer>()->StoreType();
            auto* root = call->Arguments()[i]->RootIdentifier();
            auto* root_ptr_ty = root->Type()->As<core::type::Pointer>();
            auto* root_ref_ty = root->Type()->As<core::type::Reference>();
            TINT_ASSERT(root_ptr_ty || root_ref_ty);
            const core::type::Type* root_store_type;
            if (root_ptr_ty) {
                root_store_type = root_ptr_ty->StoreType();
            } else {
                root_store_type = root_ref_ty->StoreType();
            }
            if (root_store_type != arg_store_type &&
                IsValidationEnabled(param->Declaration()->attributes,
                                    ast::DisabledValidation::kIgnoreInvalidPointerArgument)) {
                AddError(
                    "arguments of pointer type must not point to a subset of the originating "
                    "variable",
                    arg_expr->source);
                return false;
            }
        }
    }

    if (call->Type()->Is<core::type::Void>()) {
        bool is_call_statement = false;
        if (auto* call_stmt = As<ast::CallStatement>(call->Stmt()->Declaration())) {
            if (call_stmt->expr == call->Declaration()) {
                is_call_statement = true;
            }
        }
        if (!is_call_statement) {
            // https://gpuweb.github.io/gpuweb/wgsl/#function-call-expr
            // If the called function does not return a value, a function call
            // statement should be used instead.
            AddError("function '" + name + "' does not return a value", decl->source);
            return false;
        }
    }

    return true;
}

bool Validator::StructureInitializer(const ast::CallExpression* ctor,
                                     const core::type::Struct* struct_type) const {
    if (!struct_type->IsConstructible()) {
        AddError("structure constructor has non-constructible type", ctor->source);
        return false;
    }

    if (ctor->args.Length() > 0) {
        if (ctor->args.Length() != struct_type->Members().Length()) {
            std::string fm = ctor->args.Length() < struct_type->Members().Length() ? "few" : "many";
            AddError("structure constructor has too " + fm + " inputs: expected " +
                         std::to_string(struct_type->Members().Length()) + ", found " +
                         std::to_string(ctor->args.Length()),
                     ctor->source);
            return false;
        }
        for (auto* member : struct_type->Members()) {
            auto* value = ctor->args[member->Index()];
            auto* value_ty = sem_.TypeOf(value);
            if (member->Type() != value_ty->UnwrapRef()) {
                AddError(
                    "type in structure constructor does not match struct member type: expected '" +
                        sem_.TypeNameOf(member->Type()) + "', found '" + sem_.TypeNameOf(value_ty) +
                        "'",
                    value->source);
                return false;
            }
        }
    }
    return true;
}

bool Validator::ArrayConstructor(const ast::CallExpression* ctor,
                                 const core::type::Array* array_type) const {
    auto& values = ctor->args;
    auto* elem_ty = array_type->ElemType();
    for (auto* value : values) {
        auto* value_ty = sem_.TypeOf(value)->UnwrapRef();
        if (core::type::Type::ConversionRank(value_ty, elem_ty) ==
            core::type::Type::kNoConversion) {
            AddError("'" + sem_.TypeNameOf(value_ty) +
                         "' cannot be used to construct an array of '" + sem_.TypeNameOf(elem_ty) +
                         "'",
                     value->source);
            return false;
        }
    }

    auto* c = array_type->Count();
    if (c->Is<core::type::RuntimeArrayCount>()) {
        AddError("cannot construct a runtime-sized array", ctor->source);
        return false;
    }

    if (c->IsAnyOf<sem::NamedOverrideArrayCount, sem::UnnamedOverrideArrayCount>()) {
        AddError("cannot construct an array that has an override-expression count", ctor->source);
        return false;
    }

    if (!elem_ty->IsConstructible()) {
        AddError("array constructor has non-constructible element type", ctor->source);
        return false;
    }

    if (TINT_UNLIKELY(!c->Is<core::type::ConstantArrayCount>())) {
        TINT_ICE() << "Invalid ArrayCount found";
        return false;
    }

    const auto count = c->As<core::type::ConstantArrayCount>()->value;
    if (!values.IsEmpty() && (values.Length() != count)) {
        std::string fm = values.Length() < count ? "few" : "many";
        AddError("array constructor has too " + fm + " elements: expected " +
                     std::to_string(count) + ", found " + std::to_string(values.Length()),
                 ctor->source);
        return false;
    }
    return true;
}

bool Validator::Vector(const core::type::Type* el_ty, const Source& source) const {
    if (!el_ty->Is<core::type::Scalar>()) {
        AddError("vector element type must be 'bool', 'f32', 'f16', 'i32' or 'u32'", source);
        return false;
    }
    return true;
}

bool Validator::Matrix(const core::type::Type* el_ty, const Source& source) const {
    if (!el_ty->is_float_scalar()) {
        AddError("matrix element type must be 'f32' or 'f16'", source);
        return false;
    }
    return true;
}

bool Validator::PipelineStages(VectorRef<sem::Function*> entry_points) const {
    auto backtrace = [&](const sem::Function* func, const sem::Function* entry_point) {
        if (func != entry_point) {
            TraverseCallChain(entry_point, func, [&](const sem::Function* f) {
                AddNote("called by function '" + f->Declaration()->name->symbol.Name() + "'",
                        f->Declaration()->source);
            });
            AddNote(
                "called by entry point '" + entry_point->Declaration()->name->symbol.Name() + "'",
                entry_point->Declaration()->source);
        }
    };

    auto check_workgroup_storage = [&](const sem::Function* func,
                                       const sem::Function* entry_point) {
        auto stage = entry_point->Declaration()->PipelineStage();
        if (stage != ast::PipelineStage::kCompute) {
            for (auto* var : func->DirectlyReferencedGlobals()) {
                if (var->AddressSpace() == core::AddressSpace::kWorkgroup) {
                    StringStream stage_name;
                    stage_name << stage;
                    for (auto* user : var->Users()) {
                        if (func == user->Stmt()->Function()) {
                            AddError("workgroup memory cannot be used by " + stage_name.str() +
                                         " pipeline stage",
                                     user->Declaration()->source);
                            break;
                        }
                    }
                    AddNote("variable is declared here", var->Declaration()->source);
                    backtrace(func, entry_point);
                    return false;
                }
            }
        }
        return true;
    };

    auto check_builtin_calls = [&](const sem::Function* func, const sem::Function* entry_point) {
        auto stage = entry_point->Declaration()->PipelineStage();
        for (auto* builtin : func->DirectlyCalledBuiltins()) {
            if (!builtin->SupportedStages().Contains(stage)) {
                auto* call = func->FindDirectCallTo(builtin);
                StringStream err;
                err << "built-in cannot be used by " << stage << " pipeline stage";
                AddError(err.str(),
                         call ? call->Declaration()->source : func->Declaration()->source);
                backtrace(func, entry_point);
                return false;
            }
        }
        return true;
    };

    auto check_no_discards = [&](const sem::Function* func, const sem::Function* entry_point) {
        if (auto* discard = func->DiscardStatement()) {
            auto stage = entry_point->Declaration()->PipelineStage();
            StringStream err;
            err << "discard statement cannot be used in " << stage << " pipeline stage";
            AddError(err.str(), discard->Declaration()->source);
            backtrace(func, entry_point);
            return false;
        }
        return true;
    };

    auto check_func = [&](const sem::Function* func, const sem::Function* entry_point) {
        if (!check_workgroup_storage(func, entry_point)) {
            return false;
        }
        if (!check_builtin_calls(func, entry_point)) {
            return false;
        }
        if (entry_point->Declaration()->PipelineStage() != ast::PipelineStage::kFragment) {
            if (!check_no_discards(func, entry_point)) {
                return false;
            }
        }
        return true;
    };

    for (auto* entry_point : entry_points) {
        if (!check_func(entry_point, entry_point)) {
            return false;
        }
        for (auto* func : entry_point->TransitivelyCalledFunctions()) {
            if (!check_func(func, entry_point)) {
                return false;
            }
        }
    }

    return true;
}

bool Validator::PushConstants(VectorRef<sem::Function*> entry_points) const {
    for (auto* entry_point : entry_points) {
        // State checked and modified by check_push_constant so that it remembers previously seen
        // push_constant variables for an entry-point.
        const sem::Variable* push_constant_var = nullptr;
        const sem::Function* push_constant_func = nullptr;

        auto check_push_constant = [&](const sem::Function* func, const sem::Function* ep) {
            for (auto* var : func->DirectlyReferencedGlobals()) {
                if (var->AddressSpace() != core::AddressSpace::kPushConstant ||
                    var == push_constant_var) {
                    continue;
                }

                if (push_constant_var == nullptr) {
                    push_constant_var = var;
                    push_constant_func = func;
                    continue;
                }

                AddError("entry point '" + ep->Declaration()->name->symbol.Name() +
                             "' uses two different 'push_constant' variables.",
                         ep->Declaration()->source);
                AddNote("first 'push_constant' variable declaration is here",
                        var->Declaration()->source);
                if (func != ep) {
                    TraverseCallChain(ep, func, [&](const sem::Function* f) {
                        AddNote(
                            "called by function '" + f->Declaration()->name->symbol.Name() + "'",
                            f->Declaration()->source);
                    });
                    AddNote(
                        "called by entry point '" + ep->Declaration()->name->symbol.Name() + "'",
                        ep->Declaration()->source);
                }
                AddNote("second 'push_constant' variable declaration is here",
                        push_constant_var->Declaration()->source);
                if (push_constant_func != ep) {
                    TraverseCallChain(ep, push_constant_func, [&](const sem::Function* f) {
                        AddNote(
                            "called by function '" + f->Declaration()->name->symbol.Name() + "'",
                            f->Declaration()->source);
                    });
                    AddNote(
                        "called by entry point '" + ep->Declaration()->name->symbol.Name() + "'",
                        ep->Declaration()->source);
                }
                return false;
            }

            return true;
        };

        if (!check_push_constant(entry_point, entry_point)) {
            return false;
        }
        for (auto* func : entry_point->TransitivelyCalledFunctions()) {
            if (!check_push_constant(func, entry_point)) {
                return false;
            }
        }
    }

    return true;
}

bool Validator::Array(const core::type::Array* arr, const Source& el_source) const {
    auto* el_ty = arr->ElemType();

    if (!IsPlain(el_ty)) {
        AddError(sem_.TypeNameOf(el_ty) + " cannot be used as an element type of an array",
                 el_source);
        return false;
    }

    if (!IsFixedFootprint(el_ty)) {
        AddError("an array element type cannot contain a runtime-sized array", el_source);
        return false;
    }

    if (IsArrayWithOverrideCount(el_ty)) {
        RaiseArrayWithOverrideCountError(el_source);
        return false;
    }

    return true;
}

bool Validator::ArrayStrideAttribute(const ast::StrideAttribute* attr,
                                     uint32_t el_size,
                                     uint32_t el_align) const {
    auto stride = attr->stride;
    bool is_valid_stride = (stride >= el_size) && (stride >= el_align) && (stride % el_align == 0);
    if (!is_valid_stride) {
        // https://gpuweb.github.io/gpuweb/wgsl/#array-layout-rules
        // Arrays decorated with the stride attribute must have a stride that is
        // at least the size of the element type, and be a multiple of the
        // element type's alignment value.
        AddError(
            "arrays decorated with the stride attribute must have a stride that is at least the "
            "size of the element type, and be a multiple of the element type's alignment value",
            attr->source);
        return false;
    }
    return true;
}

bool Validator::Alias(const ast::Alias*) const {
    return true;
}

bool Validator::Structure(const sem::Struct* str, ast::PipelineStage stage) const {
    if (str->Members().IsEmpty()) {
        AddError("structures must have at least one member", str->Declaration()->source);
        return false;
    }

    auto has_index = false;
    Hashset<std::pair<uint32_t, uint32_t>, 8> locationsAndIndexes;
    for (auto* member : str->Members()) {
        if (auto* r = member->Type()->As<core::type::Array>()) {
            if (r->Count()->Is<core::type::RuntimeArrayCount>()) {
                if (member != str->Members().Back()) {
                    AddError("runtime arrays may only appear as the last member of a struct",
                             member->Declaration()->source);
                    return false;
                }
            }

            if (IsArrayWithOverrideCount(member->Type())) {
                RaiseArrayWithOverrideCountError(member->Declaration()->type->source);
                return false;
            }
        } else if (!IsFixedFootprint(member->Type())) {
            AddError(
                "a struct that contains a runtime array cannot be nested inside another struct",
                member->Declaration()->source);
            return false;
        }

        auto has_position = false;
        const ast::IndexAttribute* index_attribute = nullptr;
        const ast::LocationAttribute* location_attribute = nullptr;
        const ast::InvariantAttribute* invariant_attribute = nullptr;
        const ast::InterpolateAttribute* interpolate_attribute = nullptr;
        for (auto* attr : member->Declaration()->attributes) {
            bool ok = Switch(
                attr,  //
                [&](const ast::InvariantAttribute* invariant) {
                    invariant_attribute = invariant;
                    return true;
                },
                [&](const ast::LocationAttribute* location) {
                    location_attribute = location;
                    TINT_ASSERT(member->Attributes().location.has_value());
                    if (!LocationAttribute(location, member->Type(), stage,
                                           member->Declaration()->source)) {
                        return false;
                    }
                    return true;
                },
                [&](const ast::IndexAttribute* index) {
                    index_attribute = index;
                    has_index = true;
                    return IndexAttribute(index, stage);
                },
                [&](const ast::BuiltinAttribute* builtin_attr) {
                    if (!BuiltinAttribute(builtin_attr, member->Type(), stage,
                                          /* is_input */ false)) {
                        return false;
                    }
                    auto builtin = sem_.Get(builtin_attr)->Value();
                    if (builtin == core::BuiltinValue::kPosition) {
                        has_position = true;
                    }
                    return true;
                },
                [&](const ast::InterpolateAttribute* interpolate) {
                    interpolate_attribute = interpolate;
                    if (!InterpolateAttribute(interpolate, member->Type())) {
                        return false;
                    }
                    return true;
                },
                [&](const ast::StructMemberSizeAttribute*) {
                    if (!member->Type()->HasCreationFixedFootprint()) {
                        AddError(
                            "@size can only be applied to members where the member's type size "
                            "can be fully determined at shader creation time",
                            attr->source);
                        return false;
                    }
                    return true;
                },
                [&](Default) { return true; });
            if (!ok) {
                return false;
            }
        }

        if (invariant_attribute && !has_position) {
            AddError("invariant attribute must only be applied to a position builtin",
                     invariant_attribute->source);
            return false;
        }

        if (index_attribute) {
            if (!location_attribute) {
                AddError("index attribute must only be used with @location",
                         index_attribute->source);
                return false;
            }

            // Because HLSL specifies dual source blending targets with SV_Target0 and 1, we should
            // restrict targets with index attributes to location 0 for easy translation in the
            // backend writers.
            if (member->Attributes().location.value() != 0) {
                AddError("index attribute must only be used with @location(0)",
                         index_attribute->source);
                return false;
            }
        }

        if (interpolate_attribute && !location_attribute) {
            AddError("interpolate attribute must only be used with @location",
                     interpolate_attribute->source);
            return false;
        }

        // Ensure all locations and index pairs are unique
        if (location_attribute) {
            uint32_t index = 0xffffffff;
            if (index_attribute) {
                index = member->Attributes().index.value();
            }
            uint32_t location = member->Attributes().location.value();
            if (has_index && location != 0) {
                StringStream err;
                err << "Multiple render targets are not allowed when using dual source blending. "
                       "The output @location("
                    << location << ") is not allowed as a render target.";
                AddError(err.str(), location_attribute->source);
                return false;
            }

            std::pair<uint32_t, uint32_t> locationAndIndex(location, index);
            if (!locationsAndIndexes.Add(locationAndIndex)) {
                StringStream err;
                if (!index_attribute) {
                    err << "@location(" << location << ") appears multiple times";
                    AddError(err.str(), location_attribute->source);
                } else {
                    err << "@location(" << location << ") @index(" << index
                        << ") appears multiple times";
                    AddError(err.str(), index_attribute->source);
                }

                return false;
            }
        }
    }

    return true;
}

bool Validator::LocationAttribute(const ast::LocationAttribute* loc_attr,
                                  const core::type::Type* type,
                                  ast::PipelineStage stage,
                                  const Source& source,
                                  const bool is_input) const {
    std::string inputs_or_output = is_input ? "inputs" : "output";
    if (stage == ast::PipelineStage::kCompute) {
        AddError("@" + loc_attr->Name() + " is not valid for compute shader " + inputs_or_output,
                 loc_attr->source);
        return false;
    }

    if (!type->is_numeric_scalar_or_vector()) {
        std::string invalid_type = sem_.TypeNameOf(type);
        AddError("cannot apply @location to declaration of type '" + invalid_type + "'", source);
        AddNote(
            "@location must only be applied to declarations of numeric scalar or numeric vector "
            "type",
            loc_attr->source);
        return false;
    }

    return true;
}

bool Validator::IndexAttribute(const ast::IndexAttribute* index_attr,
                               ast::PipelineStage stage) const {
    if (!enabled_extensions_.Contains(core::Extension::kChromiumInternalDualSourceBlending)) {
        AddError(
            "use of '@index' attribute requires enabling extension "
            "'chromium_internal_dual_source_blending'",
            index_attr->source);
        return false;
    }

    if (stage == ast::PipelineStage::kCompute) {
        AddError("@" + index_attr->Name() + " is not valid for compute shader output",
                 index_attr->source);
        return false;
    }

    if (stage == ast::PipelineStage::kVertex) {
        AddError("@" + index_attr->Name() + " is not valid for vertex shader output",
                 index_attr->source);
        return false;
    }

    return true;
}

bool Validator::Return(const ast::ReturnStatement* ret,
                       const core::type::Type* func_type,
                       const core::type::Type* ret_type,
                       sem::Statement* current_statement) const {
    if (func_type->UnwrapRef() != ret_type) {
        AddError("return statement type must match its function return type, returned '" +
                     sem_.TypeNameOf(ret_type) + "', expected '" + sem_.TypeNameOf(func_type) + "'",
                 ret->source);
        return false;
    }

    auto* sem = sem_.Get(ret);
    if (auto* continuing = ClosestContinuing(/*stop_at_loop*/ false, current_statement)) {
        AddError("continuing blocks must not contain a return statement", ret->source);
        if (continuing != sem->Declaration() && continuing != sem->Parent()->Declaration()) {
            AddNote("see continuing block here", continuing->source);
        }
        return false;
    }

    return true;
}

bool Validator::SwitchStatement(const ast::SwitchStatement* s) {
    if (s->body.Length() > kMaxSwitchCaseSelectors) {
        AddError("switch statement has " + std::to_string(s->body.Length()) +
                     " case selectors, max is " + std::to_string(kMaxSwitchCaseSelectors),
                 s->source);
        return false;
    }

    auto* cond_ty = sem_.TypeOf(s->condition);
    if (!cond_ty->is_integer_scalar()) {
        AddError("switch statement selector expression must be of a scalar integer type",
                 s->condition->source);
        return false;
    }

    const sem::CaseSelector* default_selector = nullptr;
    Hashmap<int64_t, Source, 4> selectors;

    for (auto* case_stmt : s->body) {
        auto* case_sem = sem_.Get<sem::CaseStatement>(case_stmt);
        for (auto* selector : case_sem->Selectors()) {
            if (selector->IsDefault()) {
                if (default_selector != nullptr) {
                    // More than one default clause
                    AddError("switch statement must have exactly one default clause",
                             selector->Declaration()->source);

                    AddNote("previous default case", default_selector->Declaration()->source);
                    return false;
                }
                default_selector = selector;
                continue;
            }

            auto* decl_ty = selector->Value()->Type();
            if (cond_ty != decl_ty) {
                AddError(
                    "the case selector values must have the same type as the selector expression.",
                    selector->Declaration()->source);
                return false;
            }

            auto value = selector->Value()->ValueAs<u32>();
            if (auto added = selectors.Add(value, selector->Declaration()->source); !added) {
                AddError("duplicate switch case '" +
                             (decl_ty->IsAnyOf<core::type::I32, core::type::AbstractNumeric>()
                                  ? std::to_string(i32(value))
                                  : std::to_string(value)) +
                             "'",
                         selector->Declaration()->source);
                AddNote("previous case declared here", *added.value);
                return false;
            }
        }
    }

    if (default_selector == nullptr) {
        // No default clause
        AddError("switch statement must have a default clause", s->source);
        return false;
    }

    return true;
}

bool Validator::Assignment(const ast::Statement* a, const core::type::Type* rhs_ty) const {
    const ast::Expression* lhs;
    const ast::Expression* rhs;
    if (auto* assign = a->As<ast::AssignmentStatement>()) {
        lhs = assign->lhs;
        rhs = assign->rhs;
    } else if (auto* compound = a->As<ast::CompoundAssignmentStatement>()) {
        lhs = compound->lhs;
        rhs = compound->rhs;
    } else {
        TINT_ICE() << "invalid assignment statement";
        return false;
    }

    if (lhs->Is<ast::PhonyExpression>()) {
        // https://www.w3.org/TR/WGSL/#phony-assignment-section
        auto* ty = rhs_ty->UnwrapRef();
        if (!ty->IsConstructible() &&
            !ty->IsAnyOf<core::type::Pointer, core::type::Texture, core::type::Sampler,
                         core::type::AbstractNumeric>()) {
            AddError("cannot assign '" + sem_.TypeNameOf(rhs_ty) +
                         "' to '_'. '_' can only be assigned a constructible, pointer, texture or "
                         "sampler type",
                     rhs->source);
            return false;
        }
        return true;  // RHS can be anything.
    }

    // https://gpuweb.github.io/gpuweb/wgsl/#assignment-statement
    auto const* lhs_sem = sem_.GetVal(lhs);
    auto const* lhs_ty = lhs_sem->Type();

    auto* lhs_ref = lhs_ty->As<core::type::Reference>();
    if (!lhs_ref) {
        // LHS is not a reference, so it has no storage.
        AddError("cannot assign to " + sem_.Describe(lhs_sem), lhs->source);

        auto* expr = lhs;
        while (expr) {
            expr = Switch(
                expr,  //
                [&](const ast::AccessorExpression* e) { return e->object; },
                [&](const ast::IdentifierExpression* i) {
                    if (auto user = sem_.Get<sem::VariableUser>(i)) {
                        Switch(
                            user->Variable()->Declaration(),  //
                            [&](const ast::Let* v) {
                                AddNote("'let' variables are immutable",
                                        user->Declaration()->source);
                                sem_.NoteDeclarationSource(v);
                            },
                            [&](const ast::Const* v) {
                                AddNote("'const' variables are immutable",
                                        user->Declaration()->source);
                                sem_.NoteDeclarationSource(v);
                            },
                            [&](const ast::Override* v) {
                                AddNote("'override' variables are immutable",
                                        user->Declaration()->source);
                                sem_.NoteDeclarationSource(v);
                            },
                            [&](const ast::Parameter* v) {
                                AddNote("parameters are immutable", user->Declaration()->source);
                                sem_.NoteDeclarationSource(v);
                            });
                    }
                    return nullptr;
                });
        }

        return false;
    }

    auto* storage_ty = lhs_ref->StoreType();
    auto* value_type = rhs_ty->UnwrapRef();  // Implicit load of RHS

    // Value type has to match storage type
    if (storage_ty != value_type) {
        AddError(
            "cannot assign '" + sem_.TypeNameOf(rhs_ty) + "' to '" + sem_.TypeNameOf(lhs_ty) + "'",
            a->source);
        return false;
    }
    if (!storage_ty->IsConstructible()) {
        AddError("storage type of assignment must be constructible", a->source);
        return false;
    }
    if (lhs_ref->Access() == core::Access::kRead) {
        AddError("cannot store into a read-only type '" + sem_.RawTypeNameOf(lhs_ty) + "'",
                 a->source);
        return false;
    }
    return true;
}

bool Validator::IncrementDecrementStatement(const ast::IncrementDecrementStatement* inc) const {
    const ast::Expression* lhs = inc->lhs;

    // https://gpuweb.github.io/gpuweb/wgsl/#increment-decrement

    if (auto* var_user = sem_.Get<sem::VariableUser>(lhs)) {
        auto* v = var_user->Variable()->Declaration();
        const char* err = Switch(
            v,  //
            [&](const ast::Parameter*) { return "cannot modify function parameter"; },
            [&](const ast::Let*) { return "cannot modify 'let'"; },
            [&](const ast::Override*) { return "cannot modify 'override'"; });
        if (err) {
            AddError(err, lhs->source);
            AddNote("'" + v->name->symbol.Name() + "' is declared here:", v->source);
            return false;
        }
    }

    auto const* lhs_ty = sem_.TypeOf(lhs);
    auto* lhs_ref = lhs_ty->As<core::type::Reference>();
    if (!lhs_ref) {
        // LHS is not a reference, so it has no storage.
        AddError("cannot modify value of type '" + sem_.TypeNameOf(lhs_ty) + "'", lhs->source);
        return false;
    }

    if (!lhs_ref->StoreType()->is_integer_scalar()) {
        const std::string kind = inc->increment ? "increment" : "decrement";
        AddError(kind + " statement can only be applied to an integer scalar", lhs->source);
        return false;
    }

    if (lhs_ref->Access() == core::Access::kRead) {
        AddError("cannot modify read-only type '" + sem_.RawTypeNameOf(lhs_ty) + "'", inc->source);
        return false;
    }
    return true;
}

bool Validator::NoDuplicateAttributes(VectorRef<const ast::Attribute*> attributes) const {
    Hashmap<const tint::TypeInfo*, Source, 8> seen;
    tint::Vector<const ast::DiagnosticControl*, 8> diagnostic_controls;
    for (auto* d : attributes) {
        if (auto* diag = d->As<ast::DiagnosticAttribute>()) {
            // Allow duplicate diagnostic attributes, and check for conflicts later.
            diagnostic_controls.Push(&diag->control);
        } else {
            auto added = seen.Add(&d->TypeInfo(), d->source);
            if (!added && !d->Is<ast::InternalAttribute>()) {
                AddError("duplicate " + d->Name() + " attribute", d->source);
                AddNote("first attribute declared here", *added.value);
                return false;
            }
        }
    }
    return DiagnosticControls(diagnostic_controls, "attribute");
}

bool Validator::DiagnosticControls(VectorRef<const ast::DiagnosticControl*> controls,
                                   const char* use) const {
    // Make sure that no two diagnostic controls conflict.
    // They conflict if the rule name is the same and the severity is different.
    Hashmap<std::pair<Symbol, Symbol>, const ast::DiagnosticControl*, 8> diagnostics;
    for (auto* dc : controls) {
        auto category = dc->rule_name->category ? dc->rule_name->category->symbol : Symbol();
        auto name = dc->rule_name->name->symbol;

        auto diag_added = diagnostics.Add(std::make_pair(category, name), dc);
        if (!diag_added && (*diag_added.value)->severity != dc->severity) {
            {
                StringStream ss;
                ss << "conflicting diagnostic " << use;
                AddError(ss.str(), dc->rule_name->source);
            }
            {
                StringStream ss;
                ss << "severity of '" << dc->rule_name->String() << "' set to '" << dc->severity
                   << "' here";
                AddNote(ss.str(), (*diag_added.value)->rule_name->source);
            }
            return false;
        }
    }
    return true;
}

bool Validator::IsValidationDisabled(VectorRef<const ast::Attribute*> attributes,
                                     ast::DisabledValidation validation) const {
    for (auto* attribute : attributes) {
        if (auto* dv = attribute->As<ast::DisableValidationAttribute>()) {
            if (dv->validation == validation) {
                return true;
            }
        }
    }
    return false;
}

bool Validator::IsValidationEnabled(VectorRef<const ast::Attribute*> attributes,
                                    ast::DisabledValidation validation) const {
    return !IsValidationDisabled(attributes, validation);
}

bool Validator::IsArrayWithOverrideCount(const core::type::Type* ty) const {
    if (auto* arr = ty->UnwrapRef()->As<core::type::Array>()) {
        if (arr->Count()->IsAnyOf<sem::NamedOverrideArrayCount, sem::UnnamedOverrideArrayCount>()) {
            return true;
        }
    }
    return false;
}

void Validator::RaiseArrayWithOverrideCountError(const Source& source) const {
    AddError(
        "array with an 'override' element count can only be used as the store type of a "
        "'var<workgroup>'",
        source);
}

std::string Validator::VectorPretty(uint32_t size, const core::type::Type* element_type) const {
    core::type::Vector vec_type(element_type, size);
    return vec_type.FriendlyName();
}

bool Validator::CheckTypeAccessAddressSpace(const core::type::Type* store_ty,
                                            core::Access access,
                                            core::AddressSpace address_space,
                                            VectorRef<const tint::ast::Attribute*> attributes,
                                            const Source& source) const {
    if (!AddressSpaceLayout(store_ty, address_space, source)) {
        return false;
    }

    if (address_space == core::AddressSpace::kPushConstant &&
        !enabled_extensions_.Contains(core::Extension::kChromiumExperimentalPushConstant) &&
        IsValidationEnabled(attributes, ast::DisabledValidation::kIgnoreAddressSpace)) {
        AddError(
            "use of variable address space 'push_constant' requires enabling extension "
            "'chromium_experimental_push_constant'",
            source);
        return false;
    }

    if (address_space == core::AddressSpace::kStorage && access == core::Access::kWrite) {
        // The access mode for the storage address space can only be 'read' or 'read_write'.
        AddError("access mode 'write' is not valid for the 'storage' address space", source);
        return false;
    }

    auto atomic_error = [&]() -> const char* {
        if (address_space != core::AddressSpace::kStorage &&
            address_space != core::AddressSpace::kWorkgroup) {
            return "atomic variables must have <storage> or <workgroup> address space";
        }
        if (address_space == core::AddressSpace::kStorage && access != core::Access::kReadWrite) {
            return "atomic variables in <storage> address space must have read_write access "
                   "mode";
        }
        return nullptr;
    };

    auto check_sub_atomics = [&] {
        if (auto atomic_use = atomic_composite_info_.Get(store_ty)) {
            if (auto* err = atomic_error()) {
                AddError(err, source);
                AddNote("atomic sub-type of '" + sem_.TypeNameOf(store_ty) + "' is declared here",
                        **atomic_use);
                return false;
            }
        }
        return true;
    };

    return Switch(
        store_ty,  //
        [&](const core::type::Atomic*) {
            if (auto* err = atomic_error()) {
                AddError(err, source);
                return false;
            }
            return true;
        },
        [&](const core::type::Struct*) { return check_sub_atomics(); },  //
        [&](const core::type::Array*) { return check_sub_atomics(); },   //
        [&](Default) { return true; });
}

}  // namespace tint::resolver
