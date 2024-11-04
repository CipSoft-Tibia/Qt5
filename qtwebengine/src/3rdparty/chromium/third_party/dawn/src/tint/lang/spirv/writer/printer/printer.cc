// Copyright 2023 The Tint Authors.
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

#include "src/tint/lang/spirv/writer/printer/printer.h"

#include <utility>

#include "spirv/unified1/GLSL.std.450.h"
#include "spirv/unified1/spirv.h"
#include "src/tint/lang/core/constant/scalar.h"
#include "src/tint/lang/core/constant/splat.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/access.h"
#include "src/tint/lang/core/ir/binary.h"
#include "src/tint/lang/core/ir/bitcast.h"
#include "src/tint/lang/core/ir/block.h"
#include "src/tint/lang/core/ir/block_param.h"
#include "src/tint/lang/core/ir/break_if.h"
#include "src/tint/lang/core/ir/construct.h"
#include "src/tint/lang/core/ir/continue.h"
#include "src/tint/lang/core/ir/convert.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/exit_if.h"
#include "src/tint/lang/core/ir/exit_loop.h"
#include "src/tint/lang/core/ir/exit_switch.h"
#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/intrinsic_call.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/load.h"
#include "src/tint/lang/core/ir/load_vector_element.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/multi_in_block.h"
#include "src/tint/lang/core/ir/next_iteration.h"
#include "src/tint/lang/core/ir/return.h"
#include "src/tint/lang/core/ir/store.h"
#include "src/tint/lang/core/ir/store_vector_element.h"
#include "src/tint/lang/core/ir/switch.h"
#include "src/tint/lang/core/ir/swizzle.h"
#include "src/tint/lang/core/ir/terminate_invocation.h"
#include "src/tint/lang/core/ir/terminator.h"
#include "src/tint/lang/core/ir/unreachable.h"
#include "src/tint/lang/core/ir/user_call.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/bool.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/f16.h"
#include "src/tint/lang/core/type/f32.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/sampler.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/lang/core/type/texture.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/core/type/void.h"
#include "src/tint/lang/spirv/writer/ast_printer/ast_printer.h"
#include "src/tint/lang/spirv/writer/common/module.h"
#include "src/tint/lang/spirv/writer/raise/builtin_polyfill.h"
#include "src/tint/utils/macros/scoped_assignment.h"
#include "src/tint/utils/rtti/switch.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::spirv::writer {
namespace {

constexpr uint32_t kWriterVersion = 1;

SpvStorageClass StorageClass(core::AddressSpace addrspace) {
    switch (addrspace) {
        case core::AddressSpace::kHandle:
            return SpvStorageClassUniformConstant;
        case core::AddressSpace::kFunction:
            return SpvStorageClassFunction;
        case core::AddressSpace::kIn:
            return SpvStorageClassInput;
        case core::AddressSpace::kPrivate:
            return SpvStorageClassPrivate;
        case core::AddressSpace::kPushConstant:
            return SpvStorageClassPushConstant;
        case core::AddressSpace::kOut:
            return SpvStorageClassOutput;
        case core::AddressSpace::kStorage:
            return SpvStorageClassStorageBuffer;
        case core::AddressSpace::kUniform:
            return SpvStorageClassUniform;
        case core::AddressSpace::kWorkgroup:
            return SpvStorageClassWorkgroup;
        default:
            return SpvStorageClassMax;
    }
}

const core::type::Type* DedupType(const core::type::Type* ty, core::type::Manager& types) {
    return Switch(
        ty,

        // Atomics are not a distinct type in SPIR-V.
        [&](const core::type::Atomic* atomic) { return atomic->Type(); },

        // Depth textures are always declared as sampled textures.
        [&](const core::type::DepthTexture* depth) {
            return types.Get<core::type::SampledTexture>(depth->dim(), types.f32());
        },
        [&](const core::type::DepthMultisampledTexture* depth) {
            return types.Get<core::type::MultisampledTexture>(depth->dim(), types.f32());
        },

        // Both sampler types are the same in SPIR-V.
        [&](const core::type::Sampler* s) -> const core::type::Type* {
            if (s->IsComparison()) {
                return types.Get<core::type::Sampler>(core::type::SamplerKind::kSampler);
            }
            return s;
        },

        // Dedup a SampledImage if its underlying image will be deduped.
        [&](const raise::SampledImage* si) -> const core::type::Type* {
            auto* img = DedupType(si->Image(), types);
            if (img != si->Image()) {
                return types.Get<raise::SampledImage>(img);
            }
            return si;
        },

        [&](Default) { return ty; });
}

}  // namespace

Printer::Printer(core::ir::Module* module, bool zero_init_workgroup_mem)
    : ir_(module), b_(*module), zero_init_workgroup_memory_(zero_init_workgroup_mem) {}

Result<std::vector<uint32_t>, std::string> Printer::Generate() {
    auto valid = core::ir::ValidateAndDumpIfNeeded(*ir_, "SPIR-V writer");
    if (!valid) {
        return std::move(valid.Failure());
    }

    // TODO(crbug.com/tint/1906): Check supported extensions.

    module_.PushCapability(SpvCapabilityShader);
    module_.PushMemoryModel(spv::Op::OpMemoryModel, {U32Operand(SpvAddressingModelLogical),
                                                     U32Operand(SpvMemoryModelGLSL450)});

    // TODO(crbug.com/tint/1906): Emit extensions.

    // Emit module-scope declarations.
    if (ir_->root_block) {
        EmitRootBlock(ir_->root_block);
    }

    // Emit functions.
    for (auto* func : ir_->functions) {
        EmitFunction(func);
    }

    // Serialize the module into binary SPIR-V.
    BinaryWriter writer;
    writer.WriteHeader(module_.IdBound(), kWriterVersion);
    writer.WriteModule(&module_);
    return std::move(writer.Result());
}

uint32_t Printer::Builtin(core::BuiltinValue builtin, core::AddressSpace addrspace) {
    switch (builtin) {
        case core::BuiltinValue::kPointSize:
            return SpvBuiltInPointSize;
        case core::BuiltinValue::kFragDepth:
            return SpvBuiltInFragDepth;
        case core::BuiltinValue::kFrontFacing:
            return SpvBuiltInFrontFacing;
        case core::BuiltinValue::kGlobalInvocationId:
            return SpvBuiltInGlobalInvocationId;
        case core::BuiltinValue::kInstanceIndex:
            return SpvBuiltInInstanceIndex;
        case core::BuiltinValue::kLocalInvocationId:
            return SpvBuiltInLocalInvocationId;
        case core::BuiltinValue::kLocalInvocationIndex:
            return SpvBuiltInLocalInvocationIndex;
        case core::BuiltinValue::kNumWorkgroups:
            return SpvBuiltInNumWorkgroups;
        case core::BuiltinValue::kPosition:
            if (addrspace == core::AddressSpace::kOut) {
                // Vertex output.
                return SpvBuiltInPosition;
            } else {
                // Fragment input.
                return SpvBuiltInFragCoord;
            }
        case core::BuiltinValue::kSampleIndex:
            module_.PushCapability(SpvCapabilitySampleRateShading);
            return SpvBuiltInSampleId;
        case core::BuiltinValue::kSampleMask:
            return SpvBuiltInSampleMask;
        case core::BuiltinValue::kSubgroupInvocationId:
            module_.PushCapability(SpvCapabilityGroupNonUniform);
            return SpvBuiltInSubgroupLocalInvocationId;
        case core::BuiltinValue::kSubgroupSize:
            module_.PushCapability(SpvCapabilityGroupNonUniform);
            return SpvBuiltInSubgroupSize;
        case core::BuiltinValue::kVertexIndex:
            return SpvBuiltInVertexIndex;
        case core::BuiltinValue::kWorkgroupId:
            return SpvBuiltInWorkgroupId;
        case core::BuiltinValue::kUndefined:
            return SpvBuiltInMax;
    }
    return SpvBuiltInMax;
}

uint32_t Printer::Constant(core::ir::Constant* constant) {
    // If it is a literal operand, just return the value.
    if (auto* literal = constant->As<raise::LiteralOperand>()) {
        return literal->Value()->ValueAs<uint32_t>();
    }

    auto id = Constant(constant->Value());

    // Set the name for the SPIR-V result ID if provided in the module.
    if (auto name = ir_->NameOf(constant)) {
        module_.PushDebug(spv::Op::OpName, {id, Operand(name.Name())});
    }

    return id;
}

uint32_t Printer::Constant(const core::constant::Value* constant) {
    return constants_.GetOrCreate(constant, [&] {
        auto id = module_.NextId();
        auto* ty = constant->Type();
        Switch(
            ty,  //
            [&](const core::type::Bool*) {
                module_.PushType(
                    constant->ValueAs<bool>() ? spv::Op::OpConstantTrue : spv::Op::OpConstantFalse,
                    {Type(ty), id});
            },
            [&](const core::type::I32*) {
                module_.PushType(spv::Op::OpConstant, {Type(ty), id, constant->ValueAs<u32>()});
            },
            [&](const core::type::U32*) {
                module_.PushType(spv::Op::OpConstant,
                                 {Type(ty), id, U32Operand(constant->ValueAs<i32>())});
            },
            [&](const core::type::F32*) {
                module_.PushType(spv::Op::OpConstant, {Type(ty), id, constant->ValueAs<f32>()});
            },
            [&](const core::type::F16*) {
                module_.PushType(
                    spv::Op::OpConstant,
                    {Type(ty), id, U32Operand(constant->ValueAs<f16>().BitsRepresentation())});
            },
            [&](const core::type::Vector* vec) {
                OperandList operands = {Type(ty), id};
                for (uint32_t i = 0; i < vec->Width(); i++) {
                    operands.push_back(Constant(constant->Index(i)));
                }
                module_.PushType(spv::Op::OpConstantComposite, operands);
            },
            [&](const core::type::Matrix* mat) {
                OperandList operands = {Type(ty), id};
                for (uint32_t i = 0; i < mat->columns(); i++) {
                    operands.push_back(Constant(constant->Index(i)));
                }
                module_.PushType(spv::Op::OpConstantComposite, operands);
            },
            [&](const core::type::Array* arr) {
                TINT_ASSERT(arr->ConstantCount());
                OperandList operands = {Type(ty), id};
                for (uint32_t i = 0; i < arr->ConstantCount(); i++) {
                    operands.push_back(Constant(constant->Index(i)));
                }
                module_.PushType(spv::Op::OpConstantComposite, operands);
            },
            [&](const core::type::Struct* str) {
                OperandList operands = {Type(ty), id};
                for (uint32_t i = 0; i < str->Members().Length(); i++) {
                    operands.push_back(Constant(constant->Index(i)));
                }
                module_.PushType(spv::Op::OpConstantComposite, operands);
            },
            [&](Default) { TINT_ICE() << "unhandled constant type: " << ty->FriendlyName(); });
        return id;
    });
}

uint32_t Printer::ConstantNull(const core::type::Type* type) {
    return constant_nulls_.GetOrCreate(type, [&] {
        auto id = module_.NextId();
        module_.PushType(spv::Op::OpConstantNull, {Type(type), id});
        return id;
    });
}

uint32_t Printer::Undef(const core::type::Type* type) {
    return undef_values_.GetOrCreate(type, [&] {
        auto id = module_.NextId();
        module_.PushType(spv::Op::OpUndef, {Type(type), id});
        return id;
    });
}

uint32_t Printer::Type(const core::type::Type* ty,
                       core::AddressSpace addrspace /* = kUndefined */) {
    ty = DedupType(ty, ir_->Types());
    return types_.GetOrCreate(ty, [&] {
        auto id = module_.NextId();
        Switch(
            ty,  //
            [&](const core::type::Void*) { module_.PushType(spv::Op::OpTypeVoid, {id}); },
            [&](const core::type::Bool*) { module_.PushType(spv::Op::OpTypeBool, {id}); },
            [&](const core::type::I32*) {
                module_.PushType(spv::Op::OpTypeInt, {id, 32u, 1u});
            },
            [&](const core::type::U32*) {
                module_.PushType(spv::Op::OpTypeInt, {id, 32u, 0u});
            },
            [&](const core::type::F32*) {
                module_.PushType(spv::Op::OpTypeFloat, {id, 32u});
            },
            [&](const core::type::F16*) {
                module_.PushCapability(SpvCapabilityFloat16);
                module_.PushCapability(SpvCapabilityUniformAndStorageBuffer16BitAccess);
                module_.PushCapability(SpvCapabilityStorageBuffer16BitAccess);
                module_.PushCapability(SpvCapabilityStorageInputOutput16);
                module_.PushType(spv::Op::OpTypeFloat, {id, 16u});
            },
            [&](const core::type::Vector* vec) {
                module_.PushType(spv::Op::OpTypeVector, {id, Type(vec->type()), vec->Width()});
            },
            [&](const core::type::Matrix* mat) {
                module_.PushType(spv::Op::OpTypeMatrix,
                                 {id, Type(mat->ColumnType()), mat->columns()});
            },
            [&](const core::type::Array* arr) {
                if (arr->ConstantCount()) {
                    auto* count = b_.ConstantValue(u32(arr->ConstantCount().value()));
                    module_.PushType(spv::Op::OpTypeArray,
                                     {id, Type(arr->ElemType()), Constant(count)});
                } else {
                    TINT_ASSERT(arr->Count()->Is<core::type::RuntimeArrayCount>());
                    module_.PushType(spv::Op::OpTypeRuntimeArray, {id, Type(arr->ElemType())});
                }
                module_.PushAnnot(spv::Op::OpDecorate,
                                  {id, U32Operand(SpvDecorationArrayStride), arr->Stride()});
            },
            [&](const core::type::Pointer* ptr) {
                module_.PushType(spv::Op::OpTypePointer,
                                 {id, U32Operand(StorageClass(ptr->AddressSpace())),
                                  Type(ptr->StoreType(), ptr->AddressSpace())});
            },
            [&](const core::type::Struct* str) { EmitStructType(id, str, addrspace); },
            [&](const core::type::Texture* tex) { EmitTextureType(id, tex); },
            [&](const core::type::Sampler*) { module_.PushType(spv::Op::OpTypeSampler, {id}); },
            [&](const raise::SampledImage* s) {
                module_.PushType(spv::Op::OpTypeSampledImage, {id, Type(s->Image())});
            },
            [&](Default) { TINT_ICE() << "unhandled type: " << ty->FriendlyName(); });
        return id;
    });
}

uint32_t Printer::Value(core::ir::Instruction* inst) {
    return Value(inst->Result());
}

uint32_t Printer::Value(core::ir::Value* value) {
    return Switch(
        value,  //
        [&](core::ir::Constant* constant) { return Constant(constant); },
        [&](core::ir::Value*) {
            return values_.GetOrCreate(value, [&] { return module_.NextId(); });
        });
}

uint32_t Printer::Label(core::ir::Block* block) {
    return block_labels_.GetOrCreate(block, [&] { return module_.NextId(); });
}

void Printer::EmitStructType(uint32_t id,
                             const core::type::Struct* str,
                             core::AddressSpace addrspace /* = kUndefined */) {
    // Helper to return `type` or a potentially nested array element type within `type` as a matrix
    // type, or nullptr if no such matrix type is present.
    auto get_nested_matrix_type = [&](const core::type::Type* type) {
        while (auto* arr = type->As<core::type::Array>()) {
            type = arr->ElemType();
        }
        return type->As<core::type::Matrix>();
    };

    OperandList operands = {id};
    for (auto* member : str->Members()) {
        operands.push_back(Type(member->Type()));

        // Generate struct member offset decoration.
        module_.PushAnnot(
            spv::Op::OpMemberDecorate,
            {operands[0], member->Index(), U32Operand(SpvDecorationOffset), member->Offset()});

        // Generate shader IO decorations.
        const auto& attrs = member->Attributes();
        if (attrs.location) {
            module_.PushAnnot(
                spv::Op::OpMemberDecorate,
                {operands[0], member->Index(), U32Operand(SpvDecorationLocation), *attrs.location});
            if (attrs.interpolation) {
                switch (attrs.interpolation->type) {
                    case core::InterpolationType::kLinear:
                        module_.PushAnnot(
                            spv::Op::OpMemberDecorate,
                            {operands[0], member->Index(), U32Operand(SpvDecorationNoPerspective)});
                        break;
                    case core::InterpolationType::kFlat:
                        module_.PushAnnot(
                            spv::Op::OpMemberDecorate,
                            {operands[0], member->Index(), U32Operand(SpvDecorationFlat)});
                        break;
                    case core::InterpolationType::kPerspective:
                    case core::InterpolationType::kUndefined:
                        break;
                }
                switch (attrs.interpolation->sampling) {
                    case core::InterpolationSampling::kCentroid:
                        module_.PushAnnot(
                            spv::Op::OpMemberDecorate,
                            {operands[0], member->Index(), U32Operand(SpvDecorationCentroid)});
                        break;
                    case core::InterpolationSampling::kSample:
                        module_.PushCapability(SpvCapabilitySampleRateShading);
                        module_.PushAnnot(
                            spv::Op::OpMemberDecorate,
                            {operands[0], member->Index(), U32Operand(SpvDecorationSample)});
                        break;
                    case core::InterpolationSampling::kCenter:
                    case core::InterpolationSampling::kUndefined:
                        break;
                }
            }
        }
        if (attrs.builtin) {
            module_.PushAnnot(spv::Op::OpMemberDecorate,
                              {operands[0], member->Index(), U32Operand(SpvDecorationBuiltIn),
                               Builtin(*attrs.builtin, addrspace)});
        }
        if (attrs.invariant) {
            module_.PushAnnot(spv::Op::OpMemberDecorate,
                              {operands[0], member->Index(), U32Operand(SpvDecorationInvariant)});
        }

        // Emit matrix layout decorations if necessary.
        if (auto* matrix_type = get_nested_matrix_type(member->Type())) {
            const uint32_t effective_row_count = (matrix_type->rows() == 2) ? 2 : 4;
            module_.PushAnnot(spv::Op::OpMemberDecorate,
                              {id, member->Index(), U32Operand(SpvDecorationColMajor)});
            module_.PushAnnot(spv::Op::OpMemberDecorate,
                              {id, member->Index(), U32Operand(SpvDecorationMatrixStride),
                               Operand(effective_row_count * matrix_type->type()->Size())});
        }

        if (member->Name().IsValid()) {
            module_.PushDebug(spv::Op::OpMemberName,
                              {operands[0], member->Index(), Operand(member->Name().Name())});
        }
    }
    module_.PushType(spv::Op::OpTypeStruct, std::move(operands));

    // Add a Block decoration if necessary.
    if (str->StructFlags().Contains(core::type::StructFlag::kBlock)) {
        module_.PushAnnot(spv::Op::OpDecorate, {id, U32Operand(SpvDecorationBlock)});
    }

    if (str->Name().IsValid()) {
        module_.PushDebug(spv::Op::OpName, {operands[0], Operand(str->Name().Name())});
    }
}

void Printer::EmitTextureType(uint32_t id, const core::type::Texture* texture) {
    uint32_t sampled_type = Switch(
        texture,  //
        [&](const core::type::SampledTexture* t) { return Type(t->type()); },
        [&](const core::type::MultisampledTexture* t) { return Type(t->type()); },
        [&](const core::type::StorageTexture* t) { return Type(t->type()); },
        [&](Default) {
            TINT_ICE() << "unhandled texture type: " << texture->TypeInfo().name;
            return 0u;
        });

    uint32_t dim = SpvDimMax;
    uint32_t array = 0u;
    switch (texture->dim()) {
        case core::type::TextureDimension::kNone: {
            break;
        }
        case core::type::TextureDimension::k1d: {
            dim = SpvDim1D;
            if (texture->Is<core::type::SampledTexture>()) {
                module_.PushCapability(SpvCapabilitySampled1D);
            } else if (texture->Is<core::type::StorageTexture>()) {
                module_.PushCapability(SpvCapabilityImage1D);
            }
            break;
        }
        case core::type::TextureDimension::k2d: {
            dim = SpvDim2D;
            break;
        }
        case core::type::TextureDimension::k2dArray: {
            dim = SpvDim2D;
            array = 1u;
            break;
        }
        case core::type::TextureDimension::k3d: {
            dim = SpvDim3D;
            break;
        }
        case core::type::TextureDimension::kCube: {
            dim = SpvDimCube;
            break;
        }
        case core::type::TextureDimension::kCubeArray: {
            dim = SpvDimCube;
            array = 1u;
            if (texture->Is<core::type::SampledTexture>()) {
                module_.PushCapability(SpvCapabilitySampledCubeArray);
            }
            break;
        }
    }

    // The Vulkan spec says: The "Depth" operand of OpTypeImage is ignored.
    // In SPIRV, 0 means not depth, 1 means depth, and 2 means unknown.
    // Using anything other than 0 is problematic on various Vulkan drivers.
    uint32_t depth = 0u;

    uint32_t ms = 0u;
    if (texture->Is<core::type::MultisampledTexture>()) {
        ms = 1u;
    }

    uint32_t sampled = 2u;
    if (texture->IsAnyOf<core::type::MultisampledTexture, core::type::SampledTexture>()) {
        sampled = 1u;
    }

    uint32_t format = SpvImageFormat_::SpvImageFormatUnknown;
    if (auto* st = texture->As<core::type::StorageTexture>()) {
        format = TexelFormat(st->texel_format());
    }

    module_.PushType(spv::Op::OpTypeImage,
                     {id, sampled_type, dim, depth, array, ms, sampled, format});
}

void Printer::EmitFunction(core::ir::Function* func) {
    auto id = Value(func);

    // Emit the function name.
    module_.PushDebug(spv::Op::OpName, {id, Operand(ir_->NameOf(func).Name())});

    // Emit OpEntryPoint and OpExecutionMode declarations if needed.
    if (func->Stage() != core::ir::Function::PipelineStage::kUndefined) {
        EmitEntryPoint(func, id);
    }

    // Get the ID for the return type.
    auto return_type_id = Type(func->ReturnType());

    FunctionType function_type{return_type_id, {}};
    InstructionList params;

    // Generate function parameter declarations and add their type IDs to the function signature.
    for (auto* param : func->Params()) {
        auto param_type_id = Type(param->Type());
        auto param_id = Value(param);
        params.push_back(Instruction(spv::Op::OpFunctionParameter, {param_type_id, param_id}));
        function_type.param_type_ids.Push(param_type_id);
        if (auto name = ir_->NameOf(param)) {
            module_.PushDebug(spv::Op::OpName, {param_id, Operand(name.Name())});
        }
    }

    // Get the ID for the function type (creating it if needed).
    auto function_type_id = function_types_.GetOrCreate(function_type, [&] {
        auto func_ty_id = module_.NextId();
        OperandList operands = {func_ty_id, return_type_id};
        operands.insert(operands.end(), function_type.param_type_ids.begin(),
                        function_type.param_type_ids.end());
        module_.PushType(spv::Op::OpTypeFunction, operands);
        return func_ty_id;
    });

    // Declare the function.
    auto decl =
        Instruction{spv::Op::OpFunction,
                    {return_type_id, id, U32Operand(SpvFunctionControlMaskNone), function_type_id}};

    // Create a function that we will add instructions to.
    auto entry_block = module_.NextId();
    current_function_ = Function(decl, entry_block, std::move(params));
    TINT_DEFER(current_function_ = Function());

    // Emit the body of the function.
    EmitBlock(func->Block());

    // Add the function to the module.
    module_.PushFunction(current_function_);
}

void Printer::EmitEntryPoint(core::ir::Function* func, uint32_t id) {
    SpvExecutionModel stage = SpvExecutionModelMax;
    switch (func->Stage()) {
        case core::ir::Function::PipelineStage::kCompute: {
            stage = SpvExecutionModelGLCompute;
            module_.PushExecutionMode(
                spv::Op::OpExecutionMode,
                {id, U32Operand(SpvExecutionModeLocalSize), func->WorkgroupSize()->at(0),
                 func->WorkgroupSize()->at(1), func->WorkgroupSize()->at(2)});
            break;
        }
        case core::ir::Function::PipelineStage::kFragment: {
            stage = SpvExecutionModelFragment;
            module_.PushExecutionMode(spv::Op::OpExecutionMode,
                                      {id, U32Operand(SpvExecutionModeOriginUpperLeft)});
            break;
        }
        case core::ir::Function::PipelineStage::kVertex: {
            stage = SpvExecutionModelVertex;
            break;
        }
        case core::ir::Function::PipelineStage::kUndefined:
            TINT_ICE() << "undefined pipeline stage for entry point";
            return;
    }

    OperandList operands = {U32Operand(stage), id, ir_->NameOf(func).Name()};

    // Add the list of all referenced shader IO variables.
    if (ir_->root_block) {
        for (auto* global : *ir_->root_block) {
            auto* var = global->As<core::ir::Var>();
            if (!var) {
                continue;
            }

            auto* ptr = var->Result()->Type()->As<core::type::Pointer>();
            if (!(ptr->AddressSpace() == core::AddressSpace::kIn ||
                  ptr->AddressSpace() == core::AddressSpace::kOut)) {
                continue;
            }

            // Determine if this IO variable is used by the entry point.
            bool used = false;
            for (const auto& use : var->Result()->Usages()) {
                auto* block = use.instruction->Block();
                while (block->Parent()) {
                    block = block->Parent()->Block();
                }
                if (block == func->Block()) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                continue;
            }
            operands.push_back(Value(var));

            // Add the `DepthReplacing` execution mode if `frag_depth` is used.
            if (auto* str = ptr->StoreType()->As<core::type::Struct>()) {
                for (auto* member : str->Members()) {
                    if (member->Attributes().builtin == core::BuiltinValue::kFragDepth) {
                        module_.PushExecutionMode(spv::Op::OpExecutionMode,
                                                  {id, U32Operand(SpvExecutionModeDepthReplacing)});
                    }
                }
            }
        }
    }

    module_.PushEntryPoint(spv::Op::OpEntryPoint, operands);
}

void Printer::EmitRootBlock(core::ir::Block* root_block) {
    for (auto* inst : *root_block) {
        Switch(
            inst,  //
            [&](core::ir::Var* v) { return EmitVar(v); },
            [&](Default) {
                TINT_ICE() << "unimplemented root block instruction: " << inst->TypeInfo().name;
            });
    }
}

void Printer::EmitBlock(core::ir::Block* block) {
    // Emit the label.
    // Skip if this is the function's entry block, as it will be emitted by the function object.
    if (!current_function_.instructions().empty()) {
        current_function_.push_inst(spv::Op::OpLabel, {Label(block)});
    }

    // If there are no instructions in the block, it's a dead end, so we shouldn't be able to get
    // here to begin with.
    if (block->IsEmpty()) {
        current_function_.push_inst(spv::Op::OpUnreachable, {});
        return;
    }

    if (auto* mib = block->As<core::ir::MultiInBlock>()) {
        // Emit all OpPhi nodes for incoming branches to block.
        EmitIncomingPhis(mib);
    }

    // Emit the block's statements.
    EmitBlockInstructions(block);
}

void Printer::EmitIncomingPhis(core::ir::MultiInBlock* block) {
    // Emit Phi nodes for all the incoming block parameters
    for (size_t param_idx = 0; param_idx < block->Params().Length(); param_idx++) {
        auto* param = block->Params()[param_idx];
        OperandList ops{Type(param->Type()), Value(param)};

        for (auto* incoming : block->InboundSiblingBranches()) {
            auto* arg = incoming->Args()[param_idx];
            ops.push_back(Value(arg));
            ops.push_back(Label(incoming->Block()));
        }

        current_function_.push_inst(spv::Op::OpPhi, std::move(ops));
    }
}

void Printer::EmitBlockInstructions(core::ir::Block* block) {
    for (auto* inst : *block) {
        Switch(
            inst,                                                                 //
            [&](core::ir::Access* a) { EmitAccess(a); },                          //
            [&](core::ir::Binary* b) { EmitBinary(b); },                          //
            [&](core::ir::Bitcast* b) { EmitBitcast(b); },                        //
            [&](core::ir::CoreBuiltinCall* b) { EmitCoreBuiltinCall(b); },        //
            [&](core::ir::Construct* c) { EmitConstruct(c); },                    //
            [&](core::ir::Convert* c) { EmitConvert(c); },                        //
            [&](core::ir::IntrinsicCall* i) { EmitIntrinsicCall(i); },            //
            [&](core::ir::Load* l) { EmitLoad(l); },                              //
            [&](core::ir::LoadVectorElement* l) { EmitLoadVectorElement(l); },    //
            [&](core::ir::Loop* l) { EmitLoop(l); },                              //
            [&](core::ir::Switch* sw) { EmitSwitch(sw); },                        //
            [&](core::ir::Swizzle* s) { EmitSwizzle(s); },                        //
            [&](core::ir::Store* s) { EmitStore(s); },                            //
            [&](core::ir::StoreVectorElement* s) { EmitStoreVectorElement(s); },  //
            [&](core::ir::UserCall* c) { EmitUserCall(c); },                      //
            [&](core::ir::Unary* u) { EmitUnary(u); },                            //
            [&](core::ir::Var* v) { EmitVar(v); },                                //
            [&](core::ir::Let* l) { EmitLet(l); },                                //
            [&](core::ir::If* i) { EmitIf(i); },                                  //
            [&](core::ir::Terminator* t) { EmitTerminator(t); },                  //
            [&](Default) { TINT_ICE() << "unimplemented instruction: " << inst->TypeInfo().name; });

        // Set the name for the SPIR-V result ID if provided in the module.
        if (inst->Result() && !inst->Is<core::ir::Var>()) {
            if (auto name = ir_->NameOf(inst)) {
                module_.PushDebug(spv::Op::OpName, {Value(inst), Operand(name.Name())});
            }
        }
    }

    if (block->IsEmpty()) {
        // If the last emitted instruction is not a branch, then this should be unreachable.
        current_function_.push_inst(spv::Op::OpUnreachable, {});
    }
}

void Printer::EmitTerminator(core::ir::Terminator* t) {
    tint::Switch(  //
        t,         //
        [&](core::ir::Return*) {
            if (!t->Args().IsEmpty()) {
                TINT_ASSERT(t->Args().Length() == 1u);
                OperandList operands;
                operands.push_back(Value(t->Args()[0]));
                current_function_.push_inst(spv::Op::OpReturnValue, operands);
            } else {
                current_function_.push_inst(spv::Op::OpReturn, {});
            }
            return;
        },
        [&](core::ir::BreakIf* breakif) {
            current_function_.push_inst(spv::Op::OpBranchConditional,
                                        {
                                            Value(breakif->Condition()),
                                            loop_merge_label_,
                                            loop_header_label_,
                                        });
        },
        [&](core::ir::Continue* cont) {
            current_function_.push_inst(spv::Op::OpBranch, {Label(cont->Loop()->Continuing())});
        },
        [&](core::ir::ExitIf*) {
            current_function_.push_inst(spv::Op::OpBranch, {if_merge_label_});
        },
        [&](core::ir::ExitLoop*) {
            current_function_.push_inst(spv::Op::OpBranch, {loop_merge_label_});
        },
        [&](core::ir::ExitSwitch*) {
            current_function_.push_inst(spv::Op::OpBranch, {switch_merge_label_});
        },
        [&](core::ir::NextIteration*) {
            current_function_.push_inst(spv::Op::OpBranch, {loop_header_label_});
        },
        [&](core::ir::TerminateInvocation*) { current_function_.push_inst(spv::Op::OpKill, {}); },
        [&](core::ir::Unreachable*) { current_function_.push_inst(spv::Op::OpUnreachable, {}); },

        [&](Default) { TINT_ICE() << "unimplemented branch: " << t->TypeInfo().name; });
}

void Printer::EmitIf(core::ir::If* i) {
    auto* true_block = i->True();
    auto* false_block = i->False();

    // Generate labels for the blocks. We emit the true or false block if it:
    // 1. contains instructions other then the branch, or
    // 2. branches somewhere instead of exiting the loop (e.g. return or break), or
    // 3. the if returns a value
    // Otherwise we skip them and branch straight to the merge block.
    uint32_t merge_label = module_.NextId();
    TINT_SCOPED_ASSIGNMENT(if_merge_label_, merge_label);

    uint32_t true_label = merge_label;
    uint32_t false_label = merge_label;
    if (true_block->Length() > 1 || i->HasResults() ||
        (true_block->HasTerminator() && !true_block->Terminator()->Is<core::ir::ExitIf>())) {
        true_label = Label(true_block);
    }
    if (false_block->Length() > 1 || i->HasResults() ||
        (false_block->HasTerminator() && !false_block->Terminator()->Is<core::ir::ExitIf>())) {
        false_label = Label(false_block);
    }

    // Emit the OpSelectionMerge and OpBranchConditional instructions.
    current_function_.push_inst(spv::Op::OpSelectionMerge,
                                {merge_label, U32Operand(SpvSelectionControlMaskNone)});
    current_function_.push_inst(spv::Op::OpBranchConditional,
                                {Value(i->Condition()), true_label, false_label});

    // Emit the `true` and `false` blocks, if they're not being skipped.
    if (true_label != merge_label) {
        EmitBlock(true_block);
    }
    if (false_label != merge_label) {
        EmitBlock(false_block);
    }

    current_function_.push_inst(spv::Op::OpLabel, {merge_label});

    // Emit the OpPhis for the ExitIfs
    EmitExitPhis(i);
}

void Printer::EmitAccess(core::ir::Access* access) {
    auto* ty = access->Result()->Type();

    auto id = Value(access);
    OperandList operands = {Type(ty), id, Value(access->Object())};

    if (ty->Is<core::type::Pointer>()) {
        // Use OpAccessChain for accesses into pointer types.
        for (auto* idx : access->Indices()) {
            operands.push_back(Value(idx));
        }
        current_function_.push_inst(spv::Op::OpAccessChain, std::move(operands));
        return;
    }

    // For non-pointer types, we assume that the indices are constants and use OpCompositeExtract.
    // If we hit a non-constant index into a vector type, use OpVectorExtractDynamic for it.
    auto* source_ty = access->Object()->Type();
    for (auto* idx : access->Indices()) {
        if (auto* constant = idx->As<core::ir::Constant>()) {
            // Push the index to the chain and update the current type.
            auto i = constant->Value()->ValueAs<u32>();
            operands.push_back(i);
            source_ty = source_ty->Element(i);
        } else {
            // The VarForDynamicIndex transform ensures that only value types that are vectors
            // will be dynamically indexed, as we can use OpVectorExtractDynamic for this case.
            TINT_ASSERT(source_ty->Is<core::type::Vector>());

            // If this wasn't the first access in the chain then emit the chain so far as an
            // OpCompositeExtract, creating a new result ID for the resulting vector.
            auto vec_id = Value(access->Object());
            if (operands.size() > 3) {
                vec_id = module_.NextId();
                operands[0] = Type(source_ty);
                operands[1] = vec_id;
                current_function_.push_inst(spv::Op::OpCompositeExtract, std::move(operands));
            }

            // Now emit the OpVectorExtractDynamic instruction.
            operands = {Type(ty), id, vec_id, Value(idx)};
            current_function_.push_inst(spv::Op::OpVectorExtractDynamic, std::move(operands));
            return;
        }
    }
    current_function_.push_inst(spv::Op::OpCompositeExtract, std::move(operands));
}

void Printer::EmitBinary(core::ir::Binary* binary) {
    auto id = Value(binary);
    auto lhs = Value(binary->LHS());
    auto rhs = Value(binary->RHS());
    auto* ty = binary->Result()->Type();
    auto* lhs_ty = binary->LHS()->Type();

    // Determine the opcode.
    spv::Op op = spv::Op::Max;
    switch (binary->Kind()) {
        case core::ir::Binary::Kind::kAdd: {
            op = ty->is_integer_scalar_or_vector() ? spv::Op::OpIAdd : spv::Op::OpFAdd;
            break;
        }
        case core::ir::Binary::Kind::kDivide: {
            if (ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSDiv;
            } else if (ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpUDiv;
            } else if (ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFDiv;
            }
            break;
        }
        case core::ir::Binary::Kind::kMultiply: {
            if (ty->is_integer_scalar_or_vector()) {
                op = spv::Op::OpIMul;
            } else if (ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFMul;
            }
            break;
        }
        case core::ir::Binary::Kind::kSubtract: {
            op = ty->is_integer_scalar_or_vector() ? spv::Op::OpISub : spv::Op::OpFSub;
            break;
        }
        case core::ir::Binary::Kind::kModulo: {
            if (ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSRem;
            } else if (ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpUMod;
            } else if (ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFRem;
            }
            break;
        }

        case core::ir::Binary::Kind::kAnd: {
            if (ty->is_integer_scalar_or_vector()) {
                op = spv::Op::OpBitwiseAnd;
            } else if (ty->is_bool_scalar_or_vector()) {
                op = spv::Op::OpLogicalAnd;
            }
            break;
        }
        case core::ir::Binary::Kind::kOr: {
            if (ty->is_integer_scalar_or_vector()) {
                op = spv::Op::OpBitwiseOr;
            } else if (ty->is_bool_scalar_or_vector()) {
                op = spv::Op::OpLogicalOr;
            }
            break;
        }
        case core::ir::Binary::Kind::kXor: {
            op = spv::Op::OpBitwiseXor;
            break;
        }

        case core::ir::Binary::Kind::kShiftLeft: {
            op = spv::Op::OpShiftLeftLogical;
            break;
        }
        case core::ir::Binary::Kind::kShiftRight: {
            if (ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpShiftRightArithmetic;
            } else if (ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpShiftRightLogical;
            }
            break;
        }

        case core::ir::Binary::Kind::kEqual: {
            if (lhs_ty->is_bool_scalar_or_vector()) {
                op = spv::Op::OpLogicalEqual;
            } else if (lhs_ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFOrdEqual;
            } else if (lhs_ty->is_integer_scalar_or_vector()) {
                op = spv::Op::OpIEqual;
            }
            break;
        }
        case core::ir::Binary::Kind::kNotEqual: {
            if (lhs_ty->is_bool_scalar_or_vector()) {
                op = spv::Op::OpLogicalNotEqual;
            } else if (lhs_ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFOrdNotEqual;
            } else if (lhs_ty->is_integer_scalar_or_vector()) {
                op = spv::Op::OpINotEqual;
            }
            break;
        }
        case core::ir::Binary::Kind::kGreaterThan: {
            if (lhs_ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFOrdGreaterThan;
            } else if (lhs_ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSGreaterThan;
            } else if (lhs_ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpUGreaterThan;
            }
            break;
        }
        case core::ir::Binary::Kind::kGreaterThanEqual: {
            if (lhs_ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFOrdGreaterThanEqual;
            } else if (lhs_ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSGreaterThanEqual;
            } else if (lhs_ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpUGreaterThanEqual;
            }
            break;
        }
        case core::ir::Binary::Kind::kLessThan: {
            if (lhs_ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFOrdLessThan;
            } else if (lhs_ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSLessThan;
            } else if (lhs_ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpULessThan;
            }
            break;
        }
        case core::ir::Binary::Kind::kLessThanEqual: {
            if (lhs_ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFOrdLessThanEqual;
            } else if (lhs_ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSLessThanEqual;
            } else if (lhs_ty->is_unsigned_integer_scalar_or_vector()) {
                op = spv::Op::OpULessThanEqual;
            }
            break;
        }
    }

    // Emit the instruction.
    current_function_.push_inst(op, {Type(ty), id, lhs, rhs});
}

void Printer::EmitBitcast(core::ir::Bitcast* bitcast) {
    auto* ty = bitcast->Result()->Type();
    if (ty == bitcast->Val()->Type()) {
        values_.Add(bitcast->Result(), Value(bitcast->Val()));
        return;
    }
    current_function_.push_inst(spv::Op::OpBitcast,
                                {Type(ty), Value(bitcast), Value(bitcast->Val())});
}

void Printer::EmitCoreBuiltinCall(core::ir::CoreBuiltinCall* builtin) {
    auto* result_ty = builtin->Result()->Type();

    if (builtin->Func() == core::Function::kAbs &&
        result_ty->is_unsigned_integer_scalar_or_vector()) {
        // abs() is a no-op for unsigned integers.
        values_.Add(builtin->Result(), Value(builtin->Args()[0]));
        return;
    }
    if ((builtin->Func() == core::Function::kAll || builtin->Func() == core::Function::kAny) &&
        builtin->Args()[0]->Type()->Is<core::type::Bool>()) {
        // all() and any() are passthroughs for scalar arguments.
        values_.Add(builtin->Result(), Value(builtin->Args()[0]));
        return;
    }

    auto id = Value(builtin);

    spv::Op op = spv::Op::Max;
    OperandList operands = {Type(result_ty), id};

    // Helper to set up the opcode and operand list for a GLSL extended instruction.
    auto glsl_ext_inst = [&](enum GLSLstd450 inst) {
        constexpr const char* kGLSLstd450 = "GLSL.std.450";
        op = spv::Op::OpExtInst;
        operands.push_back(imports_.GetOrCreate(kGLSLstd450, [&] {
            // Import the instruction set the first time it is requested.
            auto import = module_.NextId();
            module_.PushExtImport(spv::Op::OpExtInstImport, {import, Operand(kGLSLstd450)});
            return import;
        }));
        operands.push_back(U32Operand(inst));
    };

    // Determine the opcode.
    switch (builtin->Func()) {
        case core::Function::kAbs:
            if (result_ty->is_float_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450FAbs);
            } else if (result_ty->is_signed_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450SAbs);
            }
            break;
        case core::Function::kAll:
            op = spv::Op::OpAll;
            break;
        case core::Function::kAny:
            op = spv::Op::OpAny;
            break;
        case core::Function::kAcos:
            glsl_ext_inst(GLSLstd450Acos);
            break;
        case core::Function::kAcosh:
            glsl_ext_inst(GLSLstd450Acosh);
            break;
        case core::Function::kAsin:
            glsl_ext_inst(GLSLstd450Asin);
            break;
        case core::Function::kAsinh:
            glsl_ext_inst(GLSLstd450Asinh);
            break;
        case core::Function::kAtan:
            glsl_ext_inst(GLSLstd450Atan);
            break;
        case core::Function::kAtan2:
            glsl_ext_inst(GLSLstd450Atan2);
            break;
        case core::Function::kAtanh:
            glsl_ext_inst(GLSLstd450Atanh);
            break;
        case core::Function::kClamp:
            if (result_ty->is_float_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450NClamp);
            } else if (result_ty->is_unsigned_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450UClamp);
            } else if (result_ty->is_signed_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450SClamp);
            }
            break;
        case core::Function::kCeil:
            glsl_ext_inst(GLSLstd450Ceil);
            break;
        case core::Function::kCos:
            glsl_ext_inst(GLSLstd450Cos);
            break;
        case core::Function::kCosh:
            glsl_ext_inst(GLSLstd450Cosh);
            break;
        case core::Function::kCountOneBits:
            op = spv::Op::OpBitCount;
            break;
        case core::Function::kCross:
            glsl_ext_inst(GLSLstd450Cross);
            break;
        case core::Function::kDegrees:
            glsl_ext_inst(GLSLstd450Degrees);
            break;
        case core::Function::kDeterminant:
            glsl_ext_inst(GLSLstd450Determinant);
            break;
        case core::Function::kDistance:
            glsl_ext_inst(GLSLstd450Distance);
            break;
        case core::Function::kDpdx:
            op = spv::Op::OpDPdx;
            break;
        case core::Function::kDpdxCoarse:
            module_.PushCapability(SpvCapabilityDerivativeControl);
            op = spv::Op::OpDPdxCoarse;
            break;
        case core::Function::kDpdxFine:
            module_.PushCapability(SpvCapabilityDerivativeControl);
            op = spv::Op::OpDPdxFine;
            break;
        case core::Function::kDpdy:
            op = spv::Op::OpDPdy;
            break;
        case core::Function::kDpdyCoarse:
            module_.PushCapability(SpvCapabilityDerivativeControl);
            op = spv::Op::OpDPdyCoarse;
            break;
        case core::Function::kDpdyFine:
            module_.PushCapability(SpvCapabilityDerivativeControl);
            op = spv::Op::OpDPdyFine;
            break;
        case core::Function::kExp:
            glsl_ext_inst(GLSLstd450Exp);
            break;
        case core::Function::kExp2:
            glsl_ext_inst(GLSLstd450Exp2);
            break;
        case core::Function::kExtractBits:
            op = result_ty->is_signed_integer_scalar_or_vector() ? spv::Op::OpBitFieldSExtract
                                                                 : spv::Op::OpBitFieldUExtract;
            break;
        case core::Function::kFaceForward:
            glsl_ext_inst(GLSLstd450FaceForward);
            break;
        case core::Function::kFloor:
            glsl_ext_inst(GLSLstd450Floor);
            break;
        case core::Function::kFma:
            glsl_ext_inst(GLSLstd450Fma);
            break;
        case core::Function::kFract:
            glsl_ext_inst(GLSLstd450Fract);
            break;
        case core::Function::kFrexp:
            glsl_ext_inst(GLSLstd450FrexpStruct);
            break;
        case core::Function::kFwidth:
            op = spv::Op::OpFwidth;
            break;
        case core::Function::kFwidthCoarse:
            module_.PushCapability(SpvCapabilityDerivativeControl);
            op = spv::Op::OpFwidthCoarse;
            break;
        case core::Function::kFwidthFine:
            module_.PushCapability(SpvCapabilityDerivativeControl);
            op = spv::Op::OpFwidthFine;
            break;
        case core::Function::kInsertBits:
            op = spv::Op::OpBitFieldInsert;
            break;
        case core::Function::kInverseSqrt:
            glsl_ext_inst(GLSLstd450InverseSqrt);
            break;
        case core::Function::kLdexp:
            glsl_ext_inst(GLSLstd450Ldexp);
            break;
        case core::Function::kLength:
            glsl_ext_inst(GLSLstd450Length);
            break;
        case core::Function::kLog:
            glsl_ext_inst(GLSLstd450Log);
            break;
        case core::Function::kLog2:
            glsl_ext_inst(GLSLstd450Log2);
            break;
        case core::Function::kMax:
            if (result_ty->is_float_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450FMax);
            } else if (result_ty->is_signed_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450SMax);
            } else if (result_ty->is_unsigned_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450UMax);
            }
            break;
        case core::Function::kMin:
            if (result_ty->is_float_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450FMin);
            } else if (result_ty->is_signed_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450SMin);
            } else if (result_ty->is_unsigned_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450UMin);
            }
            break;
        case core::Function::kMix:
            glsl_ext_inst(GLSLstd450FMix);
            break;
        case core::Function::kModf:
            glsl_ext_inst(GLSLstd450ModfStruct);
            break;
        case core::Function::kNormalize:
            glsl_ext_inst(GLSLstd450Normalize);
            break;
        case core::Function::kPack2X16Float:
            glsl_ext_inst(GLSLstd450PackHalf2x16);
            break;
        case core::Function::kPack2X16Snorm:
            glsl_ext_inst(GLSLstd450PackSnorm2x16);
            break;
        case core::Function::kPack2X16Unorm:
            glsl_ext_inst(GLSLstd450PackUnorm2x16);
            break;
        case core::Function::kPack4X8Snorm:
            glsl_ext_inst(GLSLstd450PackSnorm4x8);
            break;
        case core::Function::kPack4X8Unorm:
            glsl_ext_inst(GLSLstd450PackUnorm4x8);
            break;
        case core::Function::kPow:
            glsl_ext_inst(GLSLstd450Pow);
            break;
        case core::Function::kQuantizeToF16:
            op = spv::Op::OpQuantizeToF16;
            break;
        case core::Function::kRadians:
            glsl_ext_inst(GLSLstd450Radians);
            break;
        case core::Function::kReflect:
            glsl_ext_inst(GLSLstd450Reflect);
            break;
        case core::Function::kRefract:
            glsl_ext_inst(GLSLstd450Refract);
            break;
        case core::Function::kReverseBits:
            op = spv::Op::OpBitReverse;
            break;
        case core::Function::kRound:
            glsl_ext_inst(GLSLstd450RoundEven);
            break;
        case core::Function::kSign:
            if (result_ty->is_float_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450FSign);
            } else if (result_ty->is_signed_integer_scalar_or_vector()) {
                glsl_ext_inst(GLSLstd450SSign);
            }
            break;
        case core::Function::kSin:
            glsl_ext_inst(GLSLstd450Sin);
            break;
        case core::Function::kSinh:
            glsl_ext_inst(GLSLstd450Sinh);
            break;
        case core::Function::kSmoothstep:
            glsl_ext_inst(GLSLstd450SmoothStep);
            break;
        case core::Function::kSqrt:
            glsl_ext_inst(GLSLstd450Sqrt);
            break;
        case core::Function::kStep:
            glsl_ext_inst(GLSLstd450Step);
            break;
        case core::Function::kStorageBarrier:
            op = spv::Op::OpControlBarrier;
            operands.clear();
            operands.push_back(Constant(b_.ConstantValue(u32(spv::Scope::Workgroup))));
            operands.push_back(Constant(b_.ConstantValue(u32(spv::Scope::Workgroup))));
            operands.push_back(
                Constant(b_.ConstantValue(u32(spv::MemorySemanticsMask::UniformMemory |
                                              spv::MemorySemanticsMask::AcquireRelease))));
            break;
        case core::Function::kSubgroupBallot:
            module_.PushCapability(SpvCapabilityGroupNonUniformBallot);
            op = spv::Op::OpGroupNonUniformBallot;
            operands.push_back(Constant(ir_->constant_values.Get(u32(spv::Scope::Subgroup))));
            operands.push_back(Constant(ir_->constant_values.Get(true)));
            break;
        case core::Function::kTan:
            glsl_ext_inst(GLSLstd450Tan);
            break;
        case core::Function::kTanh:
            glsl_ext_inst(GLSLstd450Tanh);
            break;
        case core::Function::kTextureBarrier:
            op = spv::Op::OpControlBarrier;
            operands.clear();
            operands.push_back(Constant(b_.ConstantValue(u32(spv::Scope::Workgroup))));
            operands.push_back(Constant(b_.ConstantValue(u32(spv::Scope::Workgroup))));
            operands.push_back(
                Constant(b_.ConstantValue(u32(spv::MemorySemanticsMask::ImageMemory |
                                              spv::MemorySemanticsMask::AcquireRelease))));
            break;
        case core::Function::kTextureNumLevels:
            module_.PushCapability(SpvCapabilityImageQuery);
            op = spv::Op::OpImageQueryLevels;
            break;
        case core::Function::kTextureNumSamples:
            module_.PushCapability(SpvCapabilityImageQuery);
            op = spv::Op::OpImageQuerySamples;
            break;
        case core::Function::kTranspose:
            op = spv::Op::OpTranspose;
            break;
        case core::Function::kTrunc:
            glsl_ext_inst(GLSLstd450Trunc);
            break;
        case core::Function::kUnpack2X16Float:
            glsl_ext_inst(GLSLstd450UnpackHalf2x16);
            break;
        case core::Function::kUnpack2X16Snorm:
            glsl_ext_inst(GLSLstd450UnpackSnorm2x16);
            break;
        case core::Function::kUnpack2X16Unorm:
            glsl_ext_inst(GLSLstd450UnpackUnorm2x16);
            break;
        case core::Function::kUnpack4X8Snorm:
            glsl_ext_inst(GLSLstd450UnpackSnorm4x8);
            break;
        case core::Function::kUnpack4X8Unorm:
            glsl_ext_inst(GLSLstd450UnpackUnorm4x8);
            break;
        case core::Function::kWorkgroupBarrier:
            op = spv::Op::OpControlBarrier;
            operands.clear();
            operands.push_back(Constant(b_.ConstantValue(u32(spv::Scope::Workgroup))));
            operands.push_back(Constant(b_.ConstantValue(u32(spv::Scope::Workgroup))));
            operands.push_back(
                Constant(b_.ConstantValue(u32(spv::MemorySemanticsMask::WorkgroupMemory |
                                              spv::MemorySemanticsMask::AcquireRelease))));
            break;
        default:
            TINT_ICE() << "unimplemented builtin function: " << builtin->Func();
    }
    TINT_ASSERT(op != spv::Op::Max);

    // Add the arguments to the builtin call.
    for (auto* arg : builtin->Args()) {
        operands.push_back(Value(arg));
    }

    // Emit the instruction.
    current_function_.push_inst(op, operands);
}

void Printer::EmitConstruct(core::ir::Construct* construct) {
    // If there is just a single argument with the same type as the result, this is an identity
    // constructor and we can just pass through the ID of the argument.
    if (construct->Args().Length() == 1 &&
        construct->Result()->Type() == construct->Args()[0]->Type()) {
        values_.Add(construct->Result(), Value(construct->Args()[0]));
        return;
    }

    OperandList operands = {Type(construct->Result()->Type()), Value(construct)};
    for (auto* arg : construct->Args()) {
        operands.push_back(Value(arg));
    }
    current_function_.push_inst(spv::Op::OpCompositeConstruct, std::move(operands));
}

void Printer::EmitConvert(core::ir::Convert* convert) {
    auto* res_ty = convert->Result()->Type();
    auto* arg_ty = convert->Args()[0]->Type();

    OperandList operands = {Type(convert->Result()->Type()), Value(convert)};
    for (auto* arg : convert->Args()) {
        operands.push_back(Value(arg));
    }

    spv::Op op = spv::Op::Max;
    if (res_ty->is_signed_integer_scalar_or_vector() && arg_ty->is_float_scalar_or_vector()) {
        // float to signed int.
        op = spv::Op::OpConvertFToS;
    } else if (res_ty->is_unsigned_integer_scalar_or_vector() &&
               arg_ty->is_float_scalar_or_vector()) {
        // float to unsigned int.
        op = spv::Op::OpConvertFToU;
    } else if (res_ty->is_float_scalar_or_vector() &&
               arg_ty->is_signed_integer_scalar_or_vector()) {
        // signed int to float.
        op = spv::Op::OpConvertSToF;
    } else if (res_ty->is_float_scalar_or_vector() &&
               arg_ty->is_unsigned_integer_scalar_or_vector()) {
        // unsigned int to float.
        op = spv::Op::OpConvertUToF;
    } else if (res_ty->is_float_scalar_or_vector() && arg_ty->is_float_scalar_or_vector() &&
               res_ty->Size() != arg_ty->Size()) {
        // float to float (different bitwidth).
        op = spv::Op::OpFConvert;
    } else if (res_ty->is_integer_scalar_or_vector() && arg_ty->is_integer_scalar_or_vector() &&
               res_ty->Size() == arg_ty->Size()) {
        // int to int (same bitwidth, different signedness).
        op = spv::Op::OpBitcast;
    } else if (res_ty->is_bool_scalar_or_vector()) {
        if (arg_ty->is_integer_scalar_or_vector()) {
            // int to bool.
            op = spv::Op::OpINotEqual;
        } else {
            // float to bool.
            op = spv::Op::OpFUnordNotEqual;
        }
        operands.push_back(ConstantNull(arg_ty));
    } else if (arg_ty->is_bool_scalar_or_vector()) {
        // Select between constant one and zero, splatting them to vectors if necessary.
        core::ir::Constant* one = nullptr;
        core::ir::Constant* zero = nullptr;
        Switch(
            res_ty->DeepestElement(),  //
            [&](const core::type::F32*) {
                one = b_.Constant(1_f);
                zero = b_.Constant(0_f);
            },
            [&](const core::type::F16*) {
                one = b_.Constant(1_h);
                zero = b_.Constant(0_h);
            },
            [&](const core::type::I32*) {
                one = b_.Constant(1_i);
                zero = b_.Constant(0_i);
            },
            [&](const core::type::U32*) {
                one = b_.Constant(1_u);
                zero = b_.Constant(0_u);
            });
        TINT_ASSERT_OR_RETURN(one && zero);

        if (auto* vec = res_ty->As<core::type::Vector>()) {
            // Splat the scalars into vectors.
            one = b_.Splat(vec, one, vec->Width());
            zero = b_.Splat(vec, zero, vec->Width());
        }

        op = spv::Op::OpSelect;
        operands.push_back(Constant(b_.ConstantValue(one)));
        operands.push_back(Constant(b_.ConstantValue(zero)));
    } else {
        TINT_ICE() << "unhandled convert instruction";
    }

    current_function_.push_inst(op, std::move(operands));
}

void Printer::EmitIntrinsicCall(core::ir::IntrinsicCall* call) {
    auto id = Value(call);

    spv::Op op = spv::Op::Max;
    switch (call->Kind()) {
        case core::ir::IntrinsicCall::Kind::kSpirvArrayLength:
            op = spv::Op::OpArrayLength;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicIAdd:
            op = spv::Op::OpAtomicIAdd;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicISub:
            op = spv::Op::OpAtomicISub;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicAnd:
            op = spv::Op::OpAtomicAnd;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicCompareExchange:
            op = spv::Op::OpAtomicCompareExchange;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicExchange:
            op = spv::Op::OpAtomicExchange;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicLoad:
            op = spv::Op::OpAtomicLoad;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicOr:
            op = spv::Op::OpAtomicOr;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicSMax:
            op = spv::Op::OpAtomicSMax;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicSMin:
            op = spv::Op::OpAtomicSMin;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicStore:
            op = spv::Op::OpAtomicStore;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicUMax:
            op = spv::Op::OpAtomicUMax;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicUMin:
            op = spv::Op::OpAtomicUMin;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvAtomicXor:
            op = spv::Op::OpAtomicXor;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvDot:
            op = spv::Op::OpDot;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageFetch:
            op = spv::Op::OpImageFetch;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageGather:
            op = spv::Op::OpImageGather;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageDrefGather:
            op = spv::Op::OpImageDrefGather;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageQuerySize:
            module_.PushCapability(SpvCapabilityImageQuery);
            op = spv::Op::OpImageQuerySize;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageQuerySizeLod:
            module_.PushCapability(SpvCapabilityImageQuery);
            op = spv::Op::OpImageQuerySizeLod;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageSampleImplicitLod:
            op = spv::Op::OpImageSampleImplicitLod;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageSampleExplicitLod:
            op = spv::Op::OpImageSampleExplicitLod;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageSampleDrefImplicitLod:
            op = spv::Op::OpImageSampleDrefImplicitLod;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageSampleDrefExplicitLod:
            op = spv::Op::OpImageSampleDrefExplicitLod;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvImageWrite:
            op = spv::Op::OpImageWrite;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvMatrixTimesMatrix:
            op = spv::Op::OpMatrixTimesMatrix;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvMatrixTimesScalar:
            op = spv::Op::OpMatrixTimesScalar;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvMatrixTimesVector:
            op = spv::Op::OpMatrixTimesVector;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvSampledImage:
            op = spv::Op::OpSampledImage;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvSelect:
            op = spv::Op::OpSelect;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvVectorTimesMatrix:
            op = spv::Op::OpVectorTimesMatrix;
            break;
        case core::ir::IntrinsicCall::Kind::kSpirvVectorTimesScalar:
            op = spv::Op::OpVectorTimesScalar;
            break;
    }

    OperandList operands;
    if (!call->Result()->Type()->Is<core::type::Void>()) {
        operands = {Type(call->Result()->Type()), id};
    }
    for (auto* arg : call->Args()) {
        operands.push_back(Value(arg));
    }
    current_function_.push_inst(op, operands);
}

void Printer::EmitLoad(core::ir::Load* load) {
    current_function_.push_inst(spv::Op::OpLoad,
                                {Type(load->Result()->Type()), Value(load), Value(load->From())});
}

void Printer::EmitLoadVectorElement(core::ir::LoadVectorElement* load) {
    auto* vec_ptr_ty = load->From()->Type()->As<core::type::Pointer>();
    auto* el_ty = load->Result()->Type();
    auto* el_ptr_ty = ir_->Types().ptr(vec_ptr_ty->AddressSpace(), el_ty, vec_ptr_ty->Access());
    auto el_ptr_id = module_.NextId();
    current_function_.push_inst(
        spv::Op::OpAccessChain,
        {Type(el_ptr_ty), el_ptr_id, Value(load->From()), Value(load->Index())});
    current_function_.push_inst(spv::Op::OpLoad,
                                {Type(load->Result()->Type()), Value(load), el_ptr_id});
}

void Printer::EmitLoop(core::ir::Loop* loop) {
    auto init_label = loop->HasInitializer() ? Label(loop->Initializer()) : 0;
    auto body_label = Label(loop->Body());
    auto continuing_label = Label(loop->Continuing());

    auto header_label = module_.NextId();
    TINT_SCOPED_ASSIGNMENT(loop_header_label_, header_label);

    auto merge_label = module_.NextId();
    TINT_SCOPED_ASSIGNMENT(loop_merge_label_, merge_label);

    if (init_label != 0) {
        // Emit the loop initializer.
        current_function_.push_inst(spv::Op::OpBranch, {init_label});
        EmitBlock(loop->Initializer());
    } else {
        // No initializer. Branch to body.
        current_function_.push_inst(spv::Op::OpBranch, {header_label});
    }

    // Emit the loop body header, which contains the OpLoopMerge and OpPhis.
    // This then unconditionally branches to body_label
    current_function_.push_inst(spv::Op::OpLabel, {header_label});
    EmitIncomingPhis(loop->Body());
    current_function_.push_inst(
        spv::Op::OpLoopMerge, {merge_label, continuing_label, U32Operand(SpvLoopControlMaskNone)});
    current_function_.push_inst(spv::Op::OpBranch, {body_label});

    // Emit the loop body
    current_function_.push_inst(spv::Op::OpLabel, {body_label});
    EmitBlockInstructions(loop->Body());

    // Emit the loop continuing block.
    if (loop->Continuing()->HasTerminator()) {
        EmitBlock(loop->Continuing());
    } else {
        // We still need to emit a continuing block with a back-edge, even if it is unreachable.
        current_function_.push_inst(spv::Op::OpLabel, {continuing_label});
        current_function_.push_inst(spv::Op::OpBranch, {header_label});
    }

    // Emit the loop merge block.
    current_function_.push_inst(spv::Op::OpLabel, {merge_label});

    // Emit the OpPhis for the ExitLoops
    EmitExitPhis(loop);
}

void Printer::EmitSwitch(core::ir::Switch* swtch) {
    // Find the default selector. There must be exactly one.
    uint32_t default_label = 0u;
    for (auto& c : swtch->Cases()) {
        for (auto& sel : c.selectors) {
            if (sel.IsDefault()) {
                default_label = Label(c.Block());
            }
        }
    }
    TINT_ASSERT(default_label != 0u);

    // Build the operands to the OpSwitch instruction.
    OperandList switch_operands = {Value(swtch->Condition()), default_label};
    for (auto& c : swtch->Cases()) {
        auto label = Label(c.Block());
        for (auto& sel : c.selectors) {
            if (sel.IsDefault()) {
                continue;
            }
            switch_operands.push_back(sel.val->Value()->ValueAs<uint32_t>());
            switch_operands.push_back(label);
        }
    }

    uint32_t merge_label = module_.NextId();
    TINT_SCOPED_ASSIGNMENT(switch_merge_label_, merge_label);

    // Emit the OpSelectionMerge and OpSwitch instructions.
    current_function_.push_inst(spv::Op::OpSelectionMerge,
                                {merge_label, U32Operand(SpvSelectionControlMaskNone)});
    current_function_.push_inst(spv::Op::OpSwitch, switch_operands);

    // Emit the cases.
    for (auto& c : swtch->Cases()) {
        EmitBlock(c.Block());
    }

    // Emit the switch merge block.
    current_function_.push_inst(spv::Op::OpLabel, {merge_label});

    // Emit the OpPhis for the ExitSwitches
    EmitExitPhis(swtch);
}

void Printer::EmitSwizzle(core::ir::Swizzle* swizzle) {
    auto id = Value(swizzle);
    auto obj = Value(swizzle->Object());
    OperandList operands = {Type(swizzle->Result()->Type()), id, obj, obj};
    for (auto idx : swizzle->Indices()) {
        operands.push_back(idx);
    }
    current_function_.push_inst(spv::Op::OpVectorShuffle, operands);
}

void Printer::EmitStore(core::ir::Store* store) {
    current_function_.push_inst(spv::Op::OpStore, {Value(store->To()), Value(store->From())});
}

void Printer::EmitStoreVectorElement(core::ir::StoreVectorElement* store) {
    auto* vec_ptr_ty = store->To()->Type()->As<core::type::Pointer>();
    auto* el_ty = store->Value()->Type();
    auto* el_ptr_ty = ir_->Types().ptr(vec_ptr_ty->AddressSpace(), el_ty, vec_ptr_ty->Access());
    auto el_ptr_id = module_.NextId();
    current_function_.push_inst(
        spv::Op::OpAccessChain,
        {Type(el_ptr_ty), el_ptr_id, Value(store->To()), Value(store->Index())});
    current_function_.push_inst(spv::Op::OpStore, {el_ptr_id, Value(store->Value())});
}

void Printer::EmitUnary(core::ir::Unary* unary) {
    auto id = Value(unary);
    auto* ty = unary->Result()->Type();
    spv::Op op = spv::Op::Max;
    switch (unary->Kind()) {
        case core::ir::Unary::Kind::kComplement:
            op = spv::Op::OpNot;
            break;
        case core::ir::Unary::Kind::kNegation:
            if (ty->is_float_scalar_or_vector()) {
                op = spv::Op::OpFNegate;
            } else if (ty->is_signed_integer_scalar_or_vector()) {
                op = spv::Op::OpSNegate;
            }
            break;
    }
    current_function_.push_inst(op, {Type(ty), id, Value(unary->Val())});
}

void Printer::EmitUserCall(core::ir::UserCall* call) {
    auto id = Value(call);
    OperandList operands = {Type(call->Result()->Type()), id, Value(call->Func())};
    for (auto* arg : call->Args()) {
        operands.push_back(Value(arg));
    }
    current_function_.push_inst(spv::Op::OpFunctionCall, operands);
}

void Printer::EmitVar(core::ir::Var* var) {
    auto id = Value(var);
    auto* ptr = var->Result()->Type()->As<core::type::Pointer>();
    auto ty = Type(ptr);

    switch (ptr->AddressSpace()) {
        case core::AddressSpace::kFunction: {
            TINT_ASSERT(current_function_);
            if (var->Initializer()) {
                current_function_.push_var({ty, id, U32Operand(SpvStorageClassFunction)});
                current_function_.push_inst(spv::Op::OpStore, {id, Value(var->Initializer())});
            } else {
                current_function_.push_var(
                    {ty, id, U32Operand(SpvStorageClassFunction), ConstantNull(ptr->StoreType())});
            }
            break;
        }
        case core::AddressSpace::kIn: {
            TINT_ASSERT(!current_function_);
            module_.PushType(spv::Op::OpVariable, {ty, id, U32Operand(SpvStorageClassInput)});
            break;
        }
        case core::AddressSpace::kPrivate: {
            TINT_ASSERT(!current_function_);
            OperandList operands = {ty, id, U32Operand(SpvStorageClassPrivate)};
            if (var->Initializer()) {
                TINT_ASSERT(var->Initializer()->Is<core::ir::Constant>());
                operands.push_back(Value(var->Initializer()));
            } else {
                operands.push_back(ConstantNull(ptr->StoreType()));
            }
            module_.PushType(spv::Op::OpVariable, operands);
            break;
        }
        case core::AddressSpace::kPushConstant: {
            TINT_ASSERT(!current_function_);
            module_.PushType(spv::Op::OpVariable,
                             {ty, id, U32Operand(SpvStorageClassPushConstant)});
            break;
        }
        case core::AddressSpace::kOut: {
            TINT_ASSERT(!current_function_);
            module_.PushType(spv::Op::OpVariable, {ty, id, U32Operand(SpvStorageClassOutput)});
            break;
        }
        case core::AddressSpace::kHandle:
        case core::AddressSpace::kStorage:
        case core::AddressSpace::kUniform: {
            TINT_ASSERT(!current_function_);
            module_.PushType(spv::Op::OpVariable,
                             {ty, id, U32Operand(StorageClass(ptr->AddressSpace()))});
            auto bp = var->BindingPoint().value();
            module_.PushAnnot(spv::Op::OpDecorate,
                              {id, U32Operand(SpvDecorationDescriptorSet), bp.group});
            module_.PushAnnot(spv::Op::OpDecorate,
                              {id, U32Operand(SpvDecorationBinding), bp.binding});
            break;
        }
        case core::AddressSpace::kWorkgroup: {
            TINT_ASSERT(!current_function_);
            OperandList operands = {ty, id, U32Operand(SpvStorageClassWorkgroup)};
            if (zero_init_workgroup_memory_) {
                // If requested, use the VK_KHR_zero_initialize_workgroup_memory to zero-initialize
                // the workgroup variable using an null constant initializer.
                operands.push_back(ConstantNull(ptr->StoreType()));
            }
            module_.PushType(spv::Op::OpVariable, operands);
            break;
        }
        default: {
            TINT_ICE() << "unimplemented variable address space " << ptr->AddressSpace();
        }
    }

    // Set the name if present.
    if (auto name = ir_->NameOf(var)) {
        module_.PushDebug(spv::Op::OpName, {id, Operand(name.Name())});
    }
}

void Printer::EmitLet(core::ir::Let* let) {
    auto id = Value(let->Value());
    values_.Add(let->Result(), id);
}

void Printer::EmitExitPhis(core::ir::ControlInstruction* inst) {
    struct Branch {
        uint32_t label = 0;
        core::ir::Value* value = nullptr;
        bool operator<(const Branch& other) const { return label < other.label; }
    };

    auto results = inst->Results();
    for (size_t index = 0; index < results.Length(); index++) {
        auto* result = results[index];
        auto* ty = result->Type();

        Vector<Branch, 8> branches;
        branches.Reserve(inst->Exits().Count());
        for (auto& exit : inst->Exits()) {
            branches.Push(Branch{Label(exit->Block()), exit->Args()[index]});
        }
        branches.Sort();  // Sort the branches by label to ensure deterministic output

        OperandList ops{Type(ty), Value(result)};
        for (auto& branch : branches) {
            if (branch.value == nullptr) {
                ops.push_back(Undef(ty));
            } else {
                ops.push_back(Value(branch.value));
            }
            ops.push_back(branch.label);
        }
        current_function_.push_inst(spv::Op::OpPhi, std::move(ops));
    }
}

uint32_t Printer::TexelFormat(const core::TexelFormat format) {
    switch (format) {
        case core::TexelFormat::kBgra8Unorm:
            TINT_ICE() << "bgra8unorm should have been polyfilled to rgba8unorm";
            return SpvImageFormatUnknown;
        case core::TexelFormat::kR32Uint:
            return SpvImageFormatR32ui;
        case core::TexelFormat::kR32Sint:
            return SpvImageFormatR32i;
        case core::TexelFormat::kR32Float:
            return SpvImageFormatR32f;
        case core::TexelFormat::kRgba8Unorm:
            return SpvImageFormatRgba8;
        case core::TexelFormat::kRgba8Snorm:
            return SpvImageFormatRgba8Snorm;
        case core::TexelFormat::kRgba8Uint:
            return SpvImageFormatRgba8ui;
        case core::TexelFormat::kRgba8Sint:
            return SpvImageFormatRgba8i;
        case core::TexelFormat::kRg32Uint:
            module_.PushCapability(SpvCapabilityStorageImageExtendedFormats);
            return SpvImageFormatRg32ui;
        case core::TexelFormat::kRg32Sint:
            module_.PushCapability(SpvCapabilityStorageImageExtendedFormats);
            return SpvImageFormatRg32i;
        case core::TexelFormat::kRg32Float:
            module_.PushCapability(SpvCapabilityStorageImageExtendedFormats);
            return SpvImageFormatRg32f;
        case core::TexelFormat::kRgba16Uint:
            return SpvImageFormatRgba16ui;
        case core::TexelFormat::kRgba16Sint:
            return SpvImageFormatRgba16i;
        case core::TexelFormat::kRgba16Float:
            return SpvImageFormatRgba16f;
        case core::TexelFormat::kRgba32Uint:
            return SpvImageFormatRgba32ui;
        case core::TexelFormat::kRgba32Sint:
            return SpvImageFormatRgba32i;
        case core::TexelFormat::kRgba32Float:
            return SpvImageFormatRgba32f;
        case core::TexelFormat::kUndefined:
            return SpvImageFormatUnknown;
    }
    return SpvImageFormatUnknown;
}

}  // namespace tint::spirv::writer
