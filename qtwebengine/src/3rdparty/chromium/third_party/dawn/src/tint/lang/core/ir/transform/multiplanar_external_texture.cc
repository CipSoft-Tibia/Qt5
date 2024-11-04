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

#include "src/tint/lang/core/ir/transform/multiplanar_external_texture.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/external_texture.h"
#include "src/tint/lang/core/type/sampled_texture.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The external texture options.
    const ExternalTextureOptions& options;

    /// The IR module.
    Module* ir = nullptr;

    /// The IR builder.
    Builder b{*ir};

    /// The type manager.
    core::type::Manager& ty{ir->Types()};

    /// The symbol table.
    SymbolTable& sym{ir->symbols};

    /// The gamma transfer parameters structure.
    const core::type::Struct* gamma_transfer_params_struct = nullptr;

    /// The external texture parameters structure.
    const core::type::Struct* external_texture_params_struct = nullptr;

    /// The helper function that implements `textureLoad()`.
    Function* texture_load_external = nullptr;

    /// The helper function that implements `textureSampleBaseClampToEdge()`.
    Function* texture_sample_external = nullptr;

    /// The gamma correction helper function.
    Function* gamma_correction = nullptr;

    /// Process the module.
    void Process() {
        // Find module-scope variables that need to be replaced.
        if (ir->root_block) {
            Vector<Instruction*, 4> to_remove;
            for (auto inst : *ir->root_block) {
                auto* var = inst->As<Var>();
                if (!var) {
                    continue;
                }
                auto* ptr = var->Result()->Type()->As<core::type::Pointer>();
                if (ptr->StoreType()->Is<core::type::ExternalTexture>()) {
                    ReplaceVar(var);
                    to_remove.Push(var);
                }
            }
            for (auto* remove : to_remove) {
                remove->Destroy();
            }
        }

        // Find function parameters that need to be replaced.
        for (auto* func : ir->functions) {
            for (uint32_t index = 0; index < func->Params().Length(); index++) {
                auto* param = func->Params()[index];
                if (param->Type()->Is<core::type::ExternalTexture>()) {
                    ReplaceParameter(func, param, index);
                }
            }
        }
    }

    /// @returns a 2D sampled texture type with a f32 sampled type
    const core::type::SampledTexture* SampledTexture() {
        return ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.f32());
    }

    /// Replace an external texture variable declaration.
    /// @param old_var the variable declaration to replace
    void ReplaceVar(Var* old_var) {
        auto name = ir->NameOf(old_var);
        auto bp = old_var->BindingPoint();
        auto itr = options.bindings_map.find(bp.value());
        TINT_ASSERT_OR_RETURN(itr != options.bindings_map.end());
        const auto& new_binding_points = itr->second;

        // Create a sampled texture for the first plane.
        auto* plane_0 = b.Var(ty.ptr(handle, SampledTexture()));
        plane_0->SetBindingPoint(bp->group, bp->binding);
        plane_0->InsertBefore(old_var);
        if (name) {
            ir->SetName(plane_0, name.Name() + "_plane0");
        }

        // Create a sampled texture for the second plane.
        auto* plane_1 = b.Var(ty.ptr(handle, SampledTexture()));
        plane_1->SetBindingPoint(new_binding_points.plane_1.group,
                                 new_binding_points.plane_1.binding);
        plane_1->InsertBefore(old_var);
        if (name) {
            ir->SetName(plane_1, name.Name() + "_plane1");
        }

        // Create a uniform buffer for the external texture parameters.
        auto* external_texture_params = b.Var(ty.ptr(uniform, ExternalTextureParams()));
        external_texture_params->SetBindingPoint(new_binding_points.params.group,
                                                 new_binding_points.params.binding);
        external_texture_params->InsertBefore(old_var);
        if (name) {
            ir->SetName(external_texture_params, name.Name() + "_params");
        }

        // Replace all uses of the old variable with the new ones.
        ReplaceUses(old_var->Result(), plane_0->Result(), plane_1->Result(),
                    external_texture_params->Result());
    }

    /// Replace an external texture function parameter.
    /// @param func the function
    /// @param old_param the function parameter to replace
    /// @param index the index of the function parameter
    void ReplaceParameter(Function* func, FunctionParam* old_param, uint32_t index) {
        auto name = ir->NameOf(old_param);

        // Create a sampled texture for the first plane.
        auto* plane_0 = b.FunctionParam(SampledTexture());
        if (name) {
            ir->SetName(plane_0, name.Name() + "_plane0");
        }

        // Create a sampled texture for the second plane.
        auto* plane_1 = b.FunctionParam(SampledTexture());
        if (name) {
            ir->SetName(plane_1, name.Name() + "_plane1");
        }

        // Create the external texture parameters struct.
        auto* external_texture_params = b.FunctionParam(ExternalTextureParams());
        if (name) {
            ir->SetName(external_texture_params, name.Name() + "_params");
        }

        Vector<FunctionParam*, 4> new_params;
        for (uint32_t i = 0; i < func->Params().Length(); i++) {
            if (i == index) {
                new_params.Push(plane_0);
                new_params.Push(plane_1);
                new_params.Push(external_texture_params);
            } else {
                new_params.Push(func->Params()[i]);
            }
        }
        func->SetParams(std::move(new_params));

        // Replace all uses of the old parameter with the new ones.
        ReplaceUses(old_param, plane_0, plane_1, external_texture_params);
    }

    /// Recursively replace the uses of @p value with @p new_value.
    /// @param old_value the external texture value whose usages should be replaced
    /// @param plane_0 the first plane of the replacement texture
    /// @param plane_1 the second plane of the replacement texture
    /// @param params the parameters of the replacement texture
    void ReplaceUses(Value* old_value, Value* plane_0, Value* plane_1, Value* params) {
        old_value->ForEachUse([&](Usage use) {
            tint::Switch(
                use.instruction,
                [&](Load* load) {
                    // Load both of the planes and the parameters struct.
                    Value* plane_0_load = nullptr;
                    Value* plane_1_load = nullptr;
                    Value* params_load = nullptr;
                    b.InsertBefore(load, [&] {
                        plane_0_load = b.Load(plane_0)->Result();
                        plane_1_load = b.Load(plane_1)->Result();
                        params_load = b.Load(params)->Result();
                    });
                    ReplaceUses(load->Result(), plane_0_load, plane_1_load, params_load);
                    load->Destroy();
                },
                [&](CoreBuiltinCall* call) {
                    if (call->Func() == core::Function::kTextureDimensions) {
                        // Use the first plane for the `textureDimensions()` call.
                        call->SetOperand(use.operand_index, plane_0);
                    } else if (call->Func() == core::Function::kTextureLoad) {
                        // Convert the coordinates to unsigned integers if necessary.
                        auto* coords = call->Args()[1];
                        if (coords->Type()->is_signed_integer_vector()) {
                            auto* convert = b.Convert(ty.vec2<u32>(), coords);
                            convert->InsertBefore(call);
                            coords = convert->Result();
                        }

                        // Call the `TextureLoadExternal()` helper function.
                        auto* helper = b.Call(ty.vec4<f32>(), TextureLoadExternal(), plane_0,
                                              plane_1, params, coords);
                        helper->InsertBefore(call);
                        call->Result()->ReplaceAllUsesWith(helper->Result());
                        call->Destroy();
                    } else if (call->Func() == core::Function::kTextureSampleBaseClampToEdge) {
                        // Call the `TextureSampleExternal()` helper function.
                        auto* sampler = call->Args()[1];
                        auto* coords = call->Args()[2];
                        auto* helper = b.Call(ty.vec4<f32>(), TextureSampleExternal(), plane_0,
                                              plane_1, params, sampler, coords);
                        helper->InsertBefore(call);
                        call->Result()->ReplaceAllUsesWith(helper->Result());
                        call->Destroy();
                    } else {
                        TINT_ICE() << "unhandled texture_external builtin call: " << call->Func();
                    }
                },
                [&](UserCall* call) {
                    // Decompose the external texture operand into both planes and the parameters.
                    Vector<Value*, 4> operands;
                    for (uint32_t i = 0; i < call->Operands().Length(); i++) {
                        if (i == use.operand_index) {
                            operands.Push(plane_0);
                            operands.Push(plane_1);
                            operands.Push(params);
                        } else {
                            operands.Push(call->Operands()[i]);
                        }
                    }
                    call->SetOperands(std::move(operands));
                },
                [&](Default) {
                    TINT_ICE() << "unhandled instruction " << use.instruction->FriendlyName();
                });
        });
    }

    /// @returns the gamma transfer parameters struct
    const core::type::Struct* GammaTransferParams() {
        if (!gamma_transfer_params_struct) {
            gamma_transfer_params_struct = ty.Struct(sym.Register("tint_GammaTransferParams"),
                                                     {
                                                         {sym.Register("G"), ty.f32()},
                                                         {sym.Register("A"), ty.f32()},
                                                         {sym.Register("B"), ty.f32()},
                                                         {sym.Register("C"), ty.f32()},
                                                         {sym.Register("D"), ty.f32()},
                                                         {sym.Register("E"), ty.f32()},
                                                         {sym.Register("F"), ty.f32()},
                                                         {sym.Register("padding"), ty.u32()},
                                                     });
        }
        return gamma_transfer_params_struct;
    }

    /// @returns the external textures parameters struct
    const core::type::Struct* ExternalTextureParams() {
        if (!external_texture_params_struct) {
            external_texture_params_struct =
                ty.Struct(sym.Register("tint_ExternalTextureParams"),
                          {
                              {sym.Register("numPlanes"), ty.u32()},
                              {sym.Register("doYuvToRgbConversionOnly"), ty.u32()},
                              {sym.Register("yuvToRgbConversionMatrix"), ty.mat3x4<f32>()},
                              {sym.Register("gammaDecodeParams"), GammaTransferParams()},
                              {sym.Register("gammaEncodeParams"), GammaTransferParams()},
                              {sym.Register("gamutConversionMatrix"), ty.mat3x3<f32>()},
                              {sym.Register("coordTransformationMatrix"), ty.mat3x2<f32>()},
                          });
        }
        return external_texture_params_struct;
    }

    /// Gets or creates the gamma correction helper function.
    /// @returns the function
    Function* GammaCorrection() {
        if (gamma_correction) {
            return gamma_correction;
        }

        // The helper function implements the following:
        //   fn tint_GammaCorrection(v : vec3f, params : GammaTransferParams) -> vec3f {
        //     let abs_v = abs(v);
        //     let sign_v = sign(v);
        //     let cond = abs_v < vec3f(params.D);
        //     let t = sign_v * ((params.C * abs_v) + params.F);
        //     let f = sign_v * (pow((params.A * abs_v) + params.B, vec3f(params.G)) + params.E);
        //     return select(f, t, cond);
        //   }
        gamma_correction = b.Function("tint_GammaCorrection", ty.vec3<f32>());
        auto* v = b.FunctionParam("v", ty.vec3<f32>());
        auto* params = b.FunctionParam("params", GammaTransferParams());
        gamma_correction->SetParams({v, params});
        b.Append(gamma_correction->Block(), [&] {
            auto* vec3f = ty.vec3<f32>();
            auto* G = b.Access(ty.f32(), params, 0_u);
            auto* A = b.Access(ty.f32(), params, 1_u);
            auto* B = b.Access(ty.f32(), params, 2_u);
            auto* C = b.Access(ty.f32(), params, 3_u);
            auto* D = b.Access(ty.f32(), params, 4_u);
            auto* E = b.Access(ty.f32(), params, 5_u);
            auto* F = b.Access(ty.f32(), params, 6_u);
            auto* G_splat = b.Construct(vec3f, G);
            auto* D_splat = b.Construct(vec3f, D);
            auto* abs_v = b.Call(vec3f, core::Function::kAbs, v);
            auto* sign_v = b.Call(vec3f, core::Function::kSign, v);
            auto* cond = b.LessThan(ty.vec3<bool>(), abs_v, D_splat);
            auto* t = b.Multiply(vec3f, sign_v, b.Add(vec3f, b.Multiply(vec3f, C, abs_v), F));
            auto* f =
                b.Multiply(vec3f, sign_v,
                           b.Add(vec3f,
                                 b.Call(vec3f, core::Function::kPow,
                                        b.Add(vec3f, b.Multiply(vec3f, A, abs_v), B), G_splat),
                                 E));
            b.Return(gamma_correction, b.Call(vec3f, core::Function::kSelect, f, t, cond));
        });

        return gamma_correction;
    }

    /// Gets or creates the texture load helper function.
    /// @returns the function
    Function* TextureLoadExternal() {
        if (texture_load_external) {
            return texture_load_external;
        }

        // The helper function implements the following:
        //   fn tint_TextureLoadExternal(plane0 : texture_2d<f32>,
        //                               plane1 : texture_2d<f32>,
        //                               coords : vec2i,
        //                               params : ExternalTextureParams) -> vec4f {
        //     var rgb : vec3f;
        //     var alpha : f32;
        //     if ((params.numPlanes == 1)) {
        //       let texel = textureLoad(plane0, coord, 0);
        //       rgb = texel.rgb;
        //       alpha = texel.a;
        //     } else {
        //       let y = textureLoad(plane0, coord, 0).r;
        //       let coord_uv = (coord >> vec2u(1));
        //       let uv = textureLoad(plane1, coord_uv, 0).rg;
        //       rgb = vec4f(y, uv, 1) * params.yuvToRgbConversionMatrix;
        //       alpha = 1.0;
        //     }
        //
        //     if (params.doYuvToRgbConversionOnly == 0) {
        //       rgb = gammaCorrection(rgb, params.gammaDecodeParams);
        //       rgb = params.gamutConversionMatrix * rgb;
        //       rgb = gammaCorrection(rgb, params.gammaEncodeParams);
        //     }
        //
        //     return vec4f(rgb, alpha);
        //   }
        texture_load_external = b.Function("tint_TextureLoadExternal", ty.vec4<f32>());
        auto* plane_0 = b.FunctionParam("plane_0", SampledTexture());
        auto* plane_1 = b.FunctionParam("plane_1", SampledTexture());
        auto* params = b.FunctionParam("params", ExternalTextureParams());
        auto* coords = b.FunctionParam("coords", ty.vec2<u32>());
        texture_load_external->SetParams({plane_0, plane_1, params, coords});
        b.Append(texture_load_external->Block(), [&] {
            auto* vec2f = ty.vec2<f32>();
            auto* vec3f = ty.vec3<f32>();
            auto* vec4f = ty.vec4<f32>();
            auto* yuv_to_rgb_conversion_only = b.Access(ty.u32(), params, 1_u);
            auto* yuv_to_rgb_conversion = b.Access(ty.mat3x4<f32>(), params, 2_u);

            auto* rgb_result = b.InstructionResult(vec3f);
            auto* alpha_result = b.InstructionResult(ty.f32());
            auto* num_planes = b.Access(ty.u32(), params, 0_u);
            auto* if_planes_eq_1 = b.If(b.Equal(ty.bool_(), num_planes, 1_u));
            if_planes_eq_1->SetResults(rgb_result, alpha_result);
            b.Append(if_planes_eq_1->True(), [&] {
                // Load the texel from the first plane and split into separate rgb and a values.
                auto* texel = b.Call(vec4f, core::Function::kTextureLoad, plane_0, coords, 0_u);
                auto* rgb = b.Swizzle(vec3f, texel, {0u, 1u, 2u});
                auto* a = b.Access(ty.f32(), texel, 3_u);
                b.ExitIf(if_planes_eq_1, rgb, a);
            });
            b.Append(if_planes_eq_1->False(), [&] {
                // Load the y value from the first plane.
                auto* y = b.Access(
                    ty.f32(), b.Call(vec4f, core::Function::kTextureLoad, plane_0, coords, 0_u),
                    0_u);

                // Load the uv value from the second plane.
                auto* coord_uv =
                    b.ShiftRight(ty.vec2<u32>(), coords, b.Splat(ty.vec2<u32>(), 1_u, 2u));
                auto* uv = b.Swizzle(
                    vec2f, b.Call(vec4f, core::Function::kTextureLoad, plane_1, coord_uv, 0_u),
                    {0u, 1u});

                // Convert the combined yuv value into rgb and set the alpha to 1.0.
                b.ExitIf(if_planes_eq_1,
                         b.Multiply(vec3f, b.Construct(vec4f, y, uv, 1_f), yuv_to_rgb_conversion),
                         1_f);
            });

            // Apply gamma correction if needed.
            auto* final_result = b.InstructionResult(vec3f);
            auto* if_gamma_correct = b.If(b.Equal(ty.bool_(), yuv_to_rgb_conversion_only, 0_u));
            if_gamma_correct->SetResults(final_result);
            b.Append(if_gamma_correct->True(), [&] {
                auto* gamma_decode_params = b.Access(GammaTransferParams(), params, 3_u);
                auto* gamma_encode_params = b.Access(GammaTransferParams(), params, 4_u);
                auto* gamut_conversion_matrix = b.Access(ty.mat3x3<f32>(), params, 5_u);
                auto* decoded = b.Call(vec3f, GammaCorrection(), rgb_result, gamma_decode_params);
                auto* converted = b.Multiply(vec3f, gamut_conversion_matrix, decoded);
                auto* encoded = b.Call(vec3f, GammaCorrection(), converted, gamma_encode_params);
                b.ExitIf(if_gamma_correct, encoded);
            });
            b.Append(if_gamma_correct->False(), [&] {  //
                b.ExitIf(if_gamma_correct, rgb_result);
            });

            b.Return(texture_load_external, b.Construct(vec4f, final_result, alpha_result));
        });

        return texture_load_external;
    }

    /// Gets or creates the texture sample helper function.
    /// @returns the function
    Function* TextureSampleExternal() {
        if (texture_sample_external) {
            return texture_sample_external;
        }

        // The helper function implements the following:
        //   fn textureSampleExternal(plane0 : texture_2d<f32>,
        //                            plane1 : texture_2d<f32>,
        //                            smp : sampler,
        //                            coord : vec2f,
        //                            params : ExternalTextureParams) -> vec4f {
        //     let modified_coords = params.coordTransformationMatrix * vec3f(coord, 1);
        //     let plane0_dims = vec2f(textureDimensions(plane0));
        //     let plane0_half_texel = vec2f(0.5) / plane0_dims;
        //     let plane0_clamped = clamp(modified_coords, plane0_half_texel,
        //                                (1 - plane0_half_texel));
        //     let plane1_dims = vec2f(textureDimensions(plane1));
        //     let plane1_half_texel = vec2f(0.5) / plane1_dims;
        //     let plane1_clamped = clamp(modified_coords, plane1_half_texel,
        //                          (1 - plane1_half_texel));
        //     var rgb : vec3f;
        //     var alpha : f32;
        //     if ((params.numPlanes == 1)) {
        //       let texel = textureSampleLevel(plane0, smp, plane0_clamped, 0);
        //       rgb = texel.rgb;
        //       alpha = texel.a;
        //     } else {
        //       let y = textureSampleLevel(plane0, smp, plane0_clamped, 0).r;
        //       let uv = textureSampleLevel(plane1, smp, plane1_clamped, 0).rg;
        //       rgb = vec4f(y, uv, 1.0) * params.yuvToRgbConversionMatrix;
        //       alpha = 1.0;
        //     }
        //
        //     if (params.doYuvToRgbConversionOnly == 0) {
        //       rgb = gammaCorrection(rgb, params.gammaDecodeParams);
        //       rgb = params.gamutConversionMatrix * rgb;
        //       rgb = gammaCorrection(rgb, params.gammaEncodeParams);
        //     }
        //
        //     return vec4f(rgb, alpha);
        //   }
        texture_sample_external = b.Function("tint_TextureSampleExternal", ty.vec4<f32>());
        auto* plane_0 = b.FunctionParam("plane_0", SampledTexture());
        auto* plane_1 = b.FunctionParam("plane_1", SampledTexture());
        auto* params = b.FunctionParam("params", ExternalTextureParams());
        auto* sampler = b.FunctionParam("sampler", ty.sampler());
        auto* coords = b.FunctionParam("coords", ty.vec2<f32>());
        texture_sample_external->SetParams({plane_0, plane_1, params, sampler, coords});
        b.Append(texture_sample_external->Block(), [&] {
            auto* vec2f = ty.vec2<f32>();
            auto* vec3f = ty.vec3<f32>();
            auto* vec4f = ty.vec4<f32>();
            auto* yuv_to_rgb_conversion_only = b.Access(ty.u32(), params, 1_u);
            auto* yuv_to_rgb_conversion = b.Access(ty.mat3x4<f32>(), params, 2_u);
            auto* transformation_matrix = b.Access(ty.mat3x2<f32>(), params, 6_u);

            auto* modified_coords =
                b.Multiply(vec2f, transformation_matrix, b.Construct(vec3f, coords, 1_f));
            auto* plane0_dims = b.Convert(
                vec2f, b.Call(ty.vec2<u32>(), core::Function::kTextureDimensions, plane_0));
            auto* plane0_half_texel = b.Divide(vec2f, b.Splat(vec2f, 0.5_f, 2u), plane0_dims);
            auto* plane0_clamped =
                b.Call(vec2f, core::Function::kClamp, modified_coords, plane0_half_texel,
                       b.Subtract(vec2f, 1_f, plane0_half_texel));
            auto* plane1_dims = b.Convert(
                vec2f, b.Call(ty.vec2<u32>(), core::Function::kTextureDimensions, plane_1));
            auto* plane1_half_texel = b.Divide(vec2f, b.Splat(vec2f, 0.5_f, 2u), plane1_dims);
            auto* plane1_clamped =
                b.Call(vec2f, core::Function::kClamp, modified_coords, plane1_half_texel,
                       b.Subtract(vec2f, 1_f, plane1_half_texel));

            auto* rgb_result = b.InstructionResult(vec3f);
            auto* alpha_result = b.InstructionResult(ty.f32());
            auto* num_planes = b.Access(ty.u32(), params, 0_u);
            auto* if_planes_eq_1 = b.If(b.Equal(ty.bool_(), num_planes, 1_u));
            if_planes_eq_1->SetResults(rgb_result, alpha_result);
            b.Append(if_planes_eq_1->True(), [&] {
                // Sample the texel from the first plane and split into separate rgb and a values.
                auto* texel = b.Call(vec4f, core::Function::kTextureSampleLevel, plane_0, sampler,
                                     plane0_clamped, 0_f);
                auto* rgb = b.Swizzle(vec3f, texel, {0u, 1u, 2u});
                auto* a = b.Access(ty.f32(), texel, 3_u);
                b.ExitIf(if_planes_eq_1, rgb, a);
            });
            b.Append(if_planes_eq_1->False(), [&] {
                // Sample the y value from the first plane.
                auto* y = b.Access(ty.f32(),
                                   b.Call(vec4f, core::Function::kTextureSampleLevel, plane_0,
                                          sampler, plane0_clamped, 0_f),
                                   0_u);

                // Sample the uv value from the second plane.
                auto* uv = b.Swizzle(vec2f,
                                     b.Call(vec4f, core::Function::kTextureSampleLevel, plane_1,
                                            sampler, plane1_clamped, 0_f),
                                     {0u, 1u});

                // Convert the combined yuv value into rgb and set the alpha to 1.0.
                b.ExitIf(if_planes_eq_1,
                         b.Multiply(vec3f, b.Construct(vec4f, y, uv, 1_f), yuv_to_rgb_conversion),
                         1_f);
            });

            // Apply gamma correction if needed.
            auto* final_result = b.InstructionResult(vec3f);
            auto* if_gamma_correct = b.If(b.Equal(ty.bool_(), yuv_to_rgb_conversion_only, 0_u));
            if_gamma_correct->SetResults(final_result);
            b.Append(if_gamma_correct->True(), [&] {
                auto* gamma_decode_params = b.Access(GammaTransferParams(), params, 3_u);
                auto* gamma_encode_params = b.Access(GammaTransferParams(), params, 4_u);
                auto* gamut_conversion_matrix = b.Access(ty.mat3x3<f32>(), params, 5_u);
                auto* decoded = b.Call(vec3f, GammaCorrection(), rgb_result, gamma_decode_params);
                auto* converted = b.Multiply(vec3f, gamut_conversion_matrix, decoded);
                auto* encoded = b.Call(vec3f, GammaCorrection(), converted, gamma_encode_params);
                b.ExitIf(if_gamma_correct, encoded);
            });
            b.Append(if_gamma_correct->False(), [&] {  //
                b.ExitIf(if_gamma_correct, rgb_result);
            });

            b.Return(texture_sample_external, b.Construct(vec4f, final_result, alpha_result));
        });

        return texture_sample_external;
    }
};

}  // namespace

Result<SuccessType, std::string> MultiplanarExternalTexture(Module* ir,
                                                            const ExternalTextureOptions& options) {
    auto result = ValidateAndDumpIfNeeded(*ir, "MultiplanarExternalTexture transform");
    if (!result) {
        return result;
    }

    State{options, ir}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
