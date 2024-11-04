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

#include "src/tint/lang/spirv/writer/raise/builtin_polyfill.h"

#include <utility>

#include "spirv/unified1/spirv.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/builtin_structs.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/texture.h"
#include "src/tint/utils/ice/ice.h"

TINT_INSTANTIATE_TYPEINFO(tint::spirv::writer::raise::LiteralOperand);
TINT_INSTANTIATE_TYPEINFO(tint::spirv::writer::raise::SampledImage);

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::spirv::writer::raise {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module* ir = nullptr;

    /// The IR builder.
    core::ir::Builder b{*ir};

    /// The type manager.
    core::type::Manager& ty{ir->Types()};

    /// Process the module.
    void Process() {
        // Find the builtins that need replacing.
        Vector<core::ir::CoreBuiltinCall*, 4> worklist;
        for (auto* inst : ir->instructions.Objects()) {
            if (!inst->Alive()) {
                continue;
            }
            if (auto* builtin = inst->As<core::ir::CoreBuiltinCall>()) {
                switch (builtin->Func()) {
                    case core::Function::kArrayLength:
                    case core::Function::kAtomicAdd:
                    case core::Function::kAtomicAnd:
                    case core::Function::kAtomicCompareExchangeWeak:
                    case core::Function::kAtomicExchange:
                    case core::Function::kAtomicLoad:
                    case core::Function::kAtomicMax:
                    case core::Function::kAtomicMin:
                    case core::Function::kAtomicOr:
                    case core::Function::kAtomicStore:
                    case core::Function::kAtomicSub:
                    case core::Function::kAtomicXor:
                    case core::Function::kDot:
                    case core::Function::kSelect:
                    case core::Function::kTextureDimensions:
                    case core::Function::kTextureGather:
                    case core::Function::kTextureGatherCompare:
                    case core::Function::kTextureLoad:
                    case core::Function::kTextureNumLayers:
                    case core::Function::kTextureSample:
                    case core::Function::kTextureSampleBias:
                    case core::Function::kTextureSampleCompare:
                    case core::Function::kTextureSampleCompareLevel:
                    case core::Function::kTextureSampleGrad:
                    case core::Function::kTextureSampleLevel:
                    case core::Function::kTextureStore:
                        worklist.Push(builtin);
                        break;
                    default:
                        break;
                }
            }
        }

        // Replace the builtins that we found.
        for (auto* builtin : worklist) {
            core::ir::Value* replacement = nullptr;
            switch (builtin->Func()) {
                case core::Function::kArrayLength:
                    replacement = ArrayLength(builtin);
                    break;
                case core::Function::kAtomicAdd:
                case core::Function::kAtomicAnd:
                case core::Function::kAtomicCompareExchangeWeak:
                case core::Function::kAtomicExchange:
                case core::Function::kAtomicLoad:
                case core::Function::kAtomicMax:
                case core::Function::kAtomicMin:
                case core::Function::kAtomicOr:
                case core::Function::kAtomicStore:
                case core::Function::kAtomicSub:
                case core::Function::kAtomicXor:
                    replacement = Atomic(builtin);
                    break;
                case core::Function::kDot:
                    replacement = Dot(builtin);
                    break;
                case core::Function::kSelect:
                    replacement = Select(builtin);
                    break;
                case core::Function::kTextureDimensions:
                    replacement = TextureDimensions(builtin);
                    break;
                case core::Function::kTextureGather:
                case core::Function::kTextureGatherCompare:
                    replacement = TextureGather(builtin);
                    break;
                case core::Function::kTextureLoad:
                    replacement = TextureLoad(builtin);
                    break;
                case core::Function::kTextureNumLayers:
                    replacement = TextureNumLayers(builtin);
                    break;
                case core::Function::kTextureSample:
                case core::Function::kTextureSampleBias:
                case core::Function::kTextureSampleCompare:
                case core::Function::kTextureSampleCompareLevel:
                case core::Function::kTextureSampleGrad:
                case core::Function::kTextureSampleLevel:
                    replacement = TextureSample(builtin);
                    break;
                case core::Function::kTextureStore:
                    replacement = TextureStore(builtin);
                    break;
                default:
                    break;
            }
            TINT_ASSERT_OR_RETURN(replacement);

            // Replace the old builtin result with the new value.
            if (auto name = ir->NameOf(builtin->Result())) {
                ir->SetName(replacement, name);
            }
            builtin->Result()->ReplaceAllUsesWith(replacement);
            builtin->Destroy();
        }
    }

    /// Create a literal operand.
    /// @param value the literal value
    /// @returns the literal operand
    LiteralOperand* Literal(u32 value) {
        return ir->values.Create<LiteralOperand>(b.ConstantValue(value));
    }

    /// Handle an `arrayLength()` builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* ArrayLength(core::ir::CoreBuiltinCall* builtin) {
        // Strip away any let instructions to get to the original struct member access instruction.
        auto* ptr = builtin->Args()[0]->As<core::ir::InstructionResult>();
        while (auto* let = tint::As<core::ir::Let>(ptr->Source())) {
            ptr = let->Value()->As<core::ir::InstructionResult>();
        }
        TINT_ASSERT_OR_RETURN_VALUE(ptr, nullptr);

        auto* access = ptr->Source()->As<core::ir::Access>();
        TINT_ASSERT_OR_RETURN_VALUE(access, nullptr);
        TINT_ASSERT_OR_RETURN_VALUE(access->Indices().Length() == 1u, nullptr);
        TINT_ASSERT_OR_RETURN_VALUE(access->Object()->Type()->UnwrapPtr()->Is<core::type::Struct>(),
                                    nullptr);
        auto* const_idx = access->Indices()[0]->As<core::ir::Constant>();

        // Replace the builtin call with a call to the spirv.array_length intrinsic.
        auto* call =
            b.Call(builtin->Result()->Type(), core::ir::IntrinsicCall::Kind::kSpirvArrayLength,
                   Vector{access->Object(), Literal(u32(const_idx->Value()->ValueAs<uint32_t>()))});
        call->InsertBefore(builtin);
        return call->Result();
    }

    /// Handle an atomic*() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* Atomic(core::ir::CoreBuiltinCall* builtin) {
        auto* result_ty = builtin->Result()->Type();

        auto* pointer = builtin->Args()[0];
        auto* memory = [&]() -> core::ir::Value* {
            switch (pointer->Type()->As<core::type::Pointer>()->AddressSpace()) {
                case core::AddressSpace::kWorkgroup:
                    return b.Constant(u32(SpvScopeWorkgroup));
                case core::AddressSpace::kStorage:
                    return b.Constant(u32(SpvScopeDevice));
                default:
                    TINT_UNREACHABLE() << "unhandled atomic address space";
                    return nullptr;
            }
        }();
        auto* memory_semantics = b.Constant(u32(SpvMemorySemanticsMaskNone));

        // Helper to build the intrinsic call with the common operands.
        auto build = [&](const core::type::Type* type,
                         enum core::ir::IntrinsicCall::Kind intrinsic) {
            return b.Call(type, intrinsic, pointer, memory, memory_semantics);
        };

        // Create the replacement call instruction.
        core::ir::Call* call = nullptr;
        switch (builtin->Func()) {
            case core::Function::kAtomicAdd:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicIAdd);
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicAnd:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicAnd);
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicCompareExchangeWeak: {
                auto* cmp = builtin->Args()[1];
                auto* value = builtin->Args()[2];
                auto* int_ty = value->Type();
                call = build(int_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicCompareExchange);
                call->AppendArg(memory_semantics);
                call->AppendArg(value);
                call->AppendArg(cmp);
                call->InsertBefore(builtin);

                // Compare the original value to the comparator to see if an exchange happened.
                auto* original = call->Result();
                auto* compare = b.Equal(ty.bool_(), original, cmp);
                compare->InsertBefore(builtin);

                // Construct the atomicCompareExchange result structure.
                call = b.Construct(
                    core::type::CreateAtomicCompareExchangeResult(ty, ir->symbols, int_ty),
                    Vector{original, compare->Result()});
                break;
            }
            case core::Function::kAtomicExchange:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicExchange);
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicLoad:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicLoad);
                break;
            case core::Function::kAtomicOr:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicOr);
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicMax:
                if (result_ty->is_signed_integer_scalar()) {
                    call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicSMax);
                } else {
                    call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicUMax);
                }
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicMin:
                if (result_ty->is_signed_integer_scalar()) {
                    call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicSMin);
                } else {
                    call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicUMin);
                }
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicStore:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicStore);
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicSub:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicISub);
                call->AppendArg(builtin->Args()[1]);
                break;
            case core::Function::kAtomicXor:
                call = build(result_ty, core::ir::IntrinsicCall::Kind::kSpirvAtomicXor);
                call->AppendArg(builtin->Args()[1]);
                break;
            default:
                return nullptr;
        }

        call->InsertBefore(builtin);
        return call->Result();
    }

    /// Handle a `dot()` builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* Dot(core::ir::CoreBuiltinCall* builtin) {
        // OpDot only supports floating point operands, so we need to polyfill the integer case.
        // TODO(crbug.com/tint/1267): If SPV_KHR_integer_dot_product is supported, use that instead.
        if (builtin->Result()->Type()->is_integer_scalar()) {
            core::ir::Instruction* sum = nullptr;

            auto* v1 = builtin->Args()[0];
            auto* v2 = builtin->Args()[1];
            auto* vec = v1->Type()->As<core::type::Vector>();
            auto* elty = vec->type();
            for (uint32_t i = 0; i < vec->Width(); i++) {
                b.InsertBefore(builtin, [&] {
                    auto* e1 = b.Access(elty, v1, u32(i));
                    auto* e2 = b.Access(elty, v2, u32(i));
                    auto* mul = b.Multiply(elty, e1, e2);
                    if (sum) {
                        sum = b.Add(elty, sum, mul);
                    } else {
                        sum = mul;
                    }
                });
            }
            return sum->Result();
        }

        // Replace the builtin call with a call to the spirv.dot intrinsic.
        auto args = Vector<core::ir::Value*, 4>(builtin->Args());
        auto* call = b.Call(builtin->Result()->Type(), core::ir::IntrinsicCall::Kind::kSpirvDot,
                            std::move(args));
        call->InsertBefore(builtin);
        return call->Result();
    }

    /// Handle a `select()` builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* Select(core::ir::CoreBuiltinCall* builtin) {
        // Argument order is different in SPIR-V: (condition, true_operand, false_operand).
        Vector<core::ir::Value*, 4> args = {
            builtin->Args()[2],
            builtin->Args()[1],
            builtin->Args()[0],
        };

        // If the condition is scalar and the objects are vectors, we need to splat the condition
        // into a vector of the same size.
        // TODO(jrprice): We don't need to do this if we're targeting SPIR-V 1.4 or newer.
        auto* vec = builtin->Result()->Type()->As<core::type::Vector>();
        if (vec && args[0]->Type()->Is<core::type::Scalar>()) {
            Vector<core::ir::Value*, 4> elements;
            elements.Resize(vec->Width(), args[0]);

            auto* construct = b.Construct(ty.vec(ty.bool_(), vec->Width()), std::move(elements));
            construct->InsertBefore(builtin);
            args[0] = construct->Result();
        }

        // Replace the builtin call with a call to the spirv.select intrinsic.
        auto* call = b.Call(builtin->Result()->Type(), core::ir::IntrinsicCall::Kind::kSpirvSelect,
                            std::move(args));
        call->InsertBefore(builtin);
        return call->Result();
    }

    /// ImageOperands represents the optional image operands for an image instruction.
    struct ImageOperands {
        /// Bias
        core::ir::Value* bias = nullptr;
        /// Lod
        core::ir::Value* lod = nullptr;
        /// Grad (dx)
        core::ir::Value* ddx = nullptr;
        /// Grad (dy)
        core::ir::Value* ddy = nullptr;
        /// ConstOffset
        core::ir::Value* offset = nullptr;
        /// Sample
        core::ir::Value* sample = nullptr;
    };

    /// Append optional image operands to an image intrinsic argument list.
    /// @param operands the operands
    /// @param args the argument list
    /// @param insertion_point the insertion point for new instructions
    /// @param requires_float_lod true if the lod needs to be a floating point value
    void AppendImageOperands(ImageOperands& operands,
                             Vector<core::ir::Value*, 8>& args,
                             core::ir::Instruction* insertion_point,
                             bool requires_float_lod) {
        // Add a placeholder argument for the image operand mask, which we will fill in when we have
        // processed the image operands.
        uint32_t image_operand_mask = 0u;
        size_t mask_idx = args.Length();
        args.Push(nullptr);

        // Add each of the optional image operands if used, updating the image operand mask.
        if (operands.bias) {
            image_operand_mask |= SpvImageOperandsBiasMask;
            args.Push(operands.bias);
        }
        if (operands.lod) {
            image_operand_mask |= SpvImageOperandsLodMask;
            if (requires_float_lod && operands.lod->Type()->is_integer_scalar()) {
                auto* convert = b.Convert(ty.f32(), operands.lod);
                convert->InsertBefore(insertion_point);
                operands.lod = convert->Result();
            }
            args.Push(operands.lod);
        }
        if (operands.ddx) {
            image_operand_mask |= SpvImageOperandsGradMask;
            args.Push(operands.ddx);
            args.Push(operands.ddy);
        }
        if (operands.offset) {
            image_operand_mask |= SpvImageOperandsConstOffsetMask;
            args.Push(operands.offset);
        }
        if (operands.sample) {
            image_operand_mask |= SpvImageOperandsSampleMask;
            args.Push(operands.sample);
        }

        // Replace the image operand mask with the final mask value, as a literal operand.
        args[mask_idx] = Literal(u32(image_operand_mask));
    }

    /// Append an array index to a coordinate vector.
    /// @param coords the coordinate vector
    /// @param array_idx the array index
    /// @param insertion_point the insertion point for new instructions
    /// @returns the modified coordinate vector
    core::ir::Value* AppendArrayIndex(core::ir::Value* coords,
                                      core::ir::Value* array_idx,
                                      core::ir::Instruction* insertion_point) {
        auto* vec = coords->Type()->As<core::type::Vector>();
        auto* element_ty = vec->type();

        // Convert the index to match the coordinate type if needed.
        if (array_idx->Type() != element_ty) {
            auto* array_idx_converted = b.Convert(element_ty, array_idx);
            array_idx_converted->InsertBefore(insertion_point);
            array_idx = array_idx_converted->Result();
        }

        // Construct a new coordinate vector.
        auto num_coords = vec->Width();
        auto* coord_ty = ty.vec(element_ty, num_coords + 1);
        auto* construct = b.Construct(coord_ty, Vector{coords, array_idx});
        construct->InsertBefore(insertion_point);
        return construct->Result();
    }

    /// Handle a textureSample*() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* TextureSample(core::ir::CoreBuiltinCall* builtin) {
        // Helper to get the next argument from the call, or nullptr if there are no more arguments.
        uint32_t arg_idx = 0;
        auto next_arg = [&]() {
            return arg_idx < builtin->Args().Length() ? builtin->Args()[arg_idx++] : nullptr;
        };

        auto* texture = next_arg();
        auto* sampler = next_arg();
        auto* coords = next_arg();
        auto* texture_ty = texture->Type()->As<core::type::Texture>();

        // Use OpSampledImage to create an OpTypeSampledImage object.
        auto* sampled_image =
            b.Call(ty.Get<SampledImage>(texture_ty),
                   core::ir::IntrinsicCall::Kind::kSpirvSampledImage, Vector{texture, sampler});
        sampled_image->InsertBefore(builtin);

        // Append the array index to the coordinates if provided.
        auto* array_idx = IsTextureArray(texture_ty->dim()) ? next_arg() : nullptr;
        if (array_idx) {
            coords = AppendArrayIndex(coords, array_idx, builtin);
        }

        // Determine which SPIR-V intrinsic to use and which optional image operands are needed.
        enum core::ir::IntrinsicCall::Kind intrinsic;
        core::ir::Value* depth = nullptr;
        ImageOperands operands;
        switch (builtin->Func()) {
            case core::Function::kTextureSample:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageSampleImplicitLod;
                operands.offset = next_arg();
                break;
            case core::Function::kTextureSampleBias:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageSampleImplicitLod;
                operands.bias = next_arg();
                operands.offset = next_arg();
                break;
            case core::Function::kTextureSampleCompare:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageSampleDrefImplicitLod;
                depth = next_arg();
                operands.offset = next_arg();
                break;
            case core::Function::kTextureSampleCompareLevel:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageSampleDrefExplicitLod;
                depth = next_arg();
                operands.lod = b.Constant(0_f);
                operands.offset = next_arg();
                break;
            case core::Function::kTextureSampleGrad:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageSampleExplicitLod;
                operands.ddx = next_arg();
                operands.ddy = next_arg();
                operands.offset = next_arg();
                break;
            case core::Function::kTextureSampleLevel:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageSampleExplicitLod;
                operands.lod = next_arg();
                operands.offset = next_arg();
                break;
            default:
                return nullptr;
        }

        // Start building the argument list for the intrinsic.
        // The first two operands are always the sampled image and then the coordinates, followed by
        // the depth reference if used.
        Vector<core::ir::Value*, 8> intrinsic_args;
        intrinsic_args.Push(sampled_image->Result());
        intrinsic_args.Push(coords);
        if (depth) {
            intrinsic_args.Push(depth);
        }

        // Add the optional image operands, if any.
        AppendImageOperands(operands, intrinsic_args, builtin, /* requires_float_lod */ true);

        // Call the intrinsic.
        // If this is a depth comparison, the result is always f32, otherwise vec4f.
        auto* result_ty = depth ? static_cast<const core::type::Type*>(ty.f32()) : ty.vec4<f32>();
        auto* texture_call = b.Call(result_ty, intrinsic, std::move(intrinsic_args));
        texture_call->InsertBefore(builtin);

        auto* result = texture_call->Result();

        // If this is not a depth comparison but we are sampling a depth texture, extract the first
        // component to get the scalar f32 that SPIR-V expects.
        if (!depth &&
            texture_ty->IsAnyOf<core::type::DepthTexture, core::type::DepthMultisampledTexture>()) {
            auto* extract = b.Access(ty.f32(), result, 0_u);
            extract->InsertBefore(builtin);
            result = extract->Result();
        }

        return result;
    }

    /// Handle a textureGather*() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* TextureGather(core::ir::CoreBuiltinCall* builtin) {
        // Helper to get the next argument from the call, or nullptr if there are no more arguments.
        uint32_t arg_idx = 0;
        auto next_arg = [&]() {
            return arg_idx < builtin->Args().Length() ? builtin->Args()[arg_idx++] : nullptr;
        };

        auto* component = next_arg();
        if (!component->Type()->is_integer_scalar()) {
            // The first argument wasn't the component, so it must be the texture instead.
            // Use constant zero for the component.
            component = b.Constant(0_u);
            arg_idx--;
        }
        auto* texture = next_arg();
        auto* sampler = next_arg();
        auto* coords = next_arg();
        auto* texture_ty = texture->Type()->As<core::type::Texture>();

        // Use OpSampledImage to create an OpTypeSampledImage object.
        auto* sampled_image =
            b.Call(ty.Get<SampledImage>(texture_ty),
                   core::ir::IntrinsicCall::Kind::kSpirvSampledImage, Vector{texture, sampler});
        sampled_image->InsertBefore(builtin);

        // Append the array index to the coordinates if provided.
        auto* array_idx = IsTextureArray(texture_ty->dim()) ? next_arg() : nullptr;
        if (array_idx) {
            coords = AppendArrayIndex(coords, array_idx, builtin);
        }

        // Determine which SPIR-V intrinsic to use and which optional image operands are needed.
        enum core::ir::IntrinsicCall::Kind intrinsic;
        core::ir::Value* depth = nullptr;
        ImageOperands operands;
        switch (builtin->Func()) {
            case core::Function::kTextureGather:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageGather;
                operands.offset = next_arg();
                break;
            case core::Function::kTextureGatherCompare:
                intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageDrefGather;
                depth = next_arg();
                operands.offset = next_arg();
                break;
            default:
                return nullptr;
        }

        // Start building the argument list for the intrinsic.
        // The first two operands are always the sampled image and then the coordinates, followed by
        // either the depth reference or the component.
        Vector<core::ir::Value*, 8> intrinsic_args;
        intrinsic_args.Push(sampled_image->Result());
        intrinsic_args.Push(coords);
        if (depth) {
            intrinsic_args.Push(depth);
        } else {
            intrinsic_args.Push(component);
        }

        // Add the optional image operands, if any.
        AppendImageOperands(operands, intrinsic_args, builtin, /* requires_float_lod */ true);

        // Call the intrinsic.
        auto* result_ty = builtin->Result()->Type();
        auto* texture_call = b.Call(result_ty, intrinsic, std::move(intrinsic_args));
        texture_call->InsertBefore(builtin);
        return texture_call->Result();
    }

    /// Handle a textureLoad() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* TextureLoad(core::ir::CoreBuiltinCall* builtin) {
        // Helper to get the next argument from the call, or nullptr if there are no more arguments.
        uint32_t arg_idx = 0;
        auto next_arg = [&]() {
            return arg_idx < builtin->Args().Length() ? builtin->Args()[arg_idx++] : nullptr;
        };

        auto* texture = next_arg();
        auto* coords = next_arg();
        auto* texture_ty = texture->Type()->As<core::type::Texture>();

        // Append the array index to the coordinates if provided.
        auto* array_idx = IsTextureArray(texture_ty->dim()) ? next_arg() : nullptr;
        if (array_idx) {
            coords = AppendArrayIndex(coords, array_idx, builtin);
        }

        // Start building the argument list for the intrinsic.
        // The first two operands are always the texture and then the coordinates.
        Vector<core::ir::Value*, 8> intrinsic_args;
        intrinsic_args.Push(texture);
        intrinsic_args.Push(coords);

        // Add the optional image operands, if any.
        ImageOperands operands;
        if (texture_ty->IsAnyOf<core::type::MultisampledTexture,
                                core::type::DepthMultisampledTexture>()) {
            operands.sample = next_arg();
        } else {
            operands.lod = next_arg();
        }
        AppendImageOperands(operands, intrinsic_args, builtin, /* requires_float_lod */ false);

        // Call the intrinsic.
        // The result is always a vec4 in SPIR-V.
        auto* result_ty = builtin->Result()->Type();
        bool expects_scalar_result = result_ty->Is<core::type::Scalar>();
        if (expects_scalar_result) {
            result_ty = ty.vec4(result_ty);
        }
        auto* texture_call = b.Call(result_ty, core::ir::IntrinsicCall::Kind::kSpirvImageFetch,
                                    std::move(intrinsic_args));
        texture_call->InsertBefore(builtin);
        auto* result = texture_call->Result();

        // If we are expecting a scalar result, extract the first component.
        if (expects_scalar_result) {
            auto* extract = b.Access(ty.f32(), result, 0_u);
            extract->InsertBefore(builtin);
            result = extract->Result();
        }

        return result;
    }

    /// Handle a textureStore() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* TextureStore(core::ir::CoreBuiltinCall* builtin) {
        // Helper to get the next argument from the call, or nullptr if there are no more arguments.
        uint32_t arg_idx = 0;
        auto next_arg = [&]() {
            return arg_idx < builtin->Args().Length() ? builtin->Args()[arg_idx++] : nullptr;
        };

        auto* texture = next_arg();
        auto* coords = next_arg();
        auto* texture_ty = texture->Type()->As<core::type::Texture>();

        // Append the array index to the coordinates if provided.
        auto* array_idx = IsTextureArray(texture_ty->dim()) ? next_arg() : nullptr;
        if (array_idx) {
            coords = AppendArrayIndex(coords, array_idx, builtin);
        }

        auto* texel = next_arg();

        // Start building the argument list for the intrinsic.
        // The first two operands are always the texture and then the coordinates.
        Vector<core::ir::Value*, 8> intrinsic_args;
        intrinsic_args.Push(texture);
        intrinsic_args.Push(coords);
        intrinsic_args.Push(texel);

        ImageOperands operands;
        AppendImageOperands(operands, intrinsic_args, builtin, /* requires_float_lod */ false);

        // Call the intrinsic.
        auto* texture_call = b.Call(ty.void_(), core::ir::IntrinsicCall::Kind::kSpirvImageWrite,
                                    std::move(intrinsic_args));
        texture_call->InsertBefore(builtin);
        return texture_call->Result();
    }

    /// Handle a textureDimensions() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* TextureDimensions(core::ir::CoreBuiltinCall* builtin) {
        // Helper to get the next argument from the call, or nullptr if there are no more arguments.
        uint32_t arg_idx = 0;
        auto next_arg = [&]() {
            return arg_idx < builtin->Args().Length() ? builtin->Args()[arg_idx++] : nullptr;
        };

        auto* texture = next_arg();
        auto* texture_ty = texture->Type()->As<core::type::Texture>();

        Vector<core::ir::Value*, 8> intrinsic_args;
        intrinsic_args.Push(texture);

        // Determine which SPIR-V intrinsic to use, and add the Lod argument if needed.
        enum core::ir::IntrinsicCall::Kind intrinsic;
        if (texture_ty
                ->IsAnyOf<core::type::MultisampledTexture, core::type::DepthMultisampledTexture,
                          core::type::StorageTexture>()) {
            intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageQuerySize;
        } else {
            intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageQuerySizeLod;
            if (auto* lod = next_arg()) {
                intrinsic_args.Push(lod);
            } else {
                // Lod wasn't explicit, so assume 0.
                intrinsic_args.Push(b.Constant(0_u));
            }
        }

        // Add an extra component to the result vector for arrayed textures.
        auto* result_ty = builtin->Result()->Type();
        if (core::type::IsTextureArray(texture_ty->dim())) {
            auto* vec = result_ty->As<core::type::Vector>();
            result_ty = ty.vec(vec->type(), vec->Width() + 1);
        }

        // Call the intrinsic.
        auto* texture_call = b.Call(result_ty, intrinsic, std::move(intrinsic_args));
        texture_call->InsertBefore(builtin);

        auto* result = texture_call->Result();

        // Swizzle the first two components from the result for arrayed textures.
        if (core::type::IsTextureArray(texture_ty->dim())) {
            auto* swizzle = b.Swizzle(builtin->Result()->Type(), result, {0, 1});
            swizzle->InsertBefore(builtin);
            result = swizzle->Result();
        }

        return result;
    }

    /// Handle a textureNumLayers() builtin.
    /// @param builtin the builtin call instruction
    /// @returns the replacement value
    core::ir::Value* TextureNumLayers(core::ir::CoreBuiltinCall* builtin) {
        auto* texture = builtin->Args()[0];
        auto* texture_ty = texture->Type()->As<core::type::Texture>();

        Vector<core::ir::Value*, 2> intrinsic_args;
        intrinsic_args.Push(texture);

        // Determine which SPIR-V intrinsic to use, and add the Lod argument if needed.
        enum core::ir::IntrinsicCall::Kind intrinsic;
        if (texture_ty
                ->IsAnyOf<core::type::MultisampledTexture, core::type::DepthMultisampledTexture,
                          core::type::StorageTexture>()) {
            intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageQuerySize;
        } else {
            intrinsic = core::ir::IntrinsicCall::Kind::kSpirvImageQuerySizeLod;
            intrinsic_args.Push(b.Constant(0_u));
        }

        // Call the intrinsic.
        auto* texture_call = b.Call(ty.vec3<u32>(), intrinsic, std::move(intrinsic_args));
        texture_call->InsertBefore(builtin);

        // Extract the third component to get the number of array layers.
        auto* extract = b.Access(ty.u32(), texture_call->Result(), 2_u);
        extract->InsertBefore(builtin);
        return extract->Result();
    }
};

}  // namespace

Result<SuccessType, std::string> BuiltinPolyfill(core::ir::Module* ir) {
    auto result = ValidateAndDumpIfNeeded(*ir, "BuiltinPolyfill transform");
    if (!result) {
        return result;
    }

    State{ir}.Process();

    return Success;
}

LiteralOperand::LiteralOperand(const core::constant::Value* value) : Base(value) {}

LiteralOperand::~LiteralOperand() = default;

SampledImage::SampledImage(const core::type::Type* image)
    : Base(static_cast<size_t>(Hash(tint::TypeInfo::Of<SampledImage>().full_hashcode, image)),
           core::type::Flags{}),
      image_(image) {}

SampledImage* SampledImage::Clone(core::type::CloneContext& ctx) const {
    auto* image = image_->Clone(ctx);
    return ctx.dst.mgr->Get<SampledImage>(image);
}

}  // namespace tint::spirv::writer::raise
