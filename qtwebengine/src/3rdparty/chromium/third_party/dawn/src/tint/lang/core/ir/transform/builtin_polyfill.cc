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

#include "src/tint/lang/core/ir/transform/builtin_polyfill.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/sampled_texture.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The polyfill config.
    const BuiltinPolyfillConfig& config;

    /// The IR module.
    Module* ir = nullptr;

    /// The IR builder.
    Builder b{*ir};

    /// The type manager.
    core::type::Manager& ty{ir->Types()};

    /// The symbol table.
    SymbolTable& sym{ir->symbols};

    /// Process the module.
    void Process() {
        // Find the builtin call instructions that may need to be polyfilled.
        Vector<ir::CoreBuiltinCall*, 4> worklist;
        for (auto* inst : ir->instructions.Objects()) {
            if (!inst->Alive()) {
                continue;
            }
            if (auto* builtin = inst->As<ir::CoreBuiltinCall>()) {
                switch (builtin->Func()) {
                    case core::Function::kCountLeadingZeros:
                        if (config.count_leading_zeros) {
                            worklist.Push(builtin);
                        }
                        break;
                    case core::Function::kCountTrailingZeros:
                        if (config.count_trailing_zeros) {
                            worklist.Push(builtin);
                        }
                        break;
                    case core::Function::kFirstLeadingBit:
                        if (config.first_leading_bit) {
                            worklist.Push(builtin);
                        }
                        break;
                    case core::Function::kFirstTrailingBit:
                        if (config.first_trailing_bit) {
                            worklist.Push(builtin);
                        }
                        break;
                    case core::Function::kSaturate:
                        if (config.saturate) {
                            worklist.Push(builtin);
                        }
                        break;
                    case core::Function::kTextureSampleBaseClampToEdge:
                        if (config.texture_sample_base_clamp_to_edge_2d_f32) {
                            auto* tex =
                                builtin->Args()[0]->Type()->As<core::type::SampledTexture>();
                            if (tex && tex->dim() == core::type::TextureDimension::k2d &&
                                tex->type()->Is<core::type::F32>()) {
                                worklist.Push(builtin);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        // Polyfill the builtin call instructions that we found.
        for (auto* builtin : worklist) {
            ir::Value* replacement = nullptr;
            switch (builtin->Func()) {
                case core::Function::kCountLeadingZeros:
                    replacement = CountLeadingZeros(builtin);
                    break;
                case core::Function::kCountTrailingZeros:
                    replacement = CountTrailingZeros(builtin);
                    break;
                case core::Function::kFirstLeadingBit:
                    replacement = FirstLeadingBit(builtin);
                    break;
                case core::Function::kFirstTrailingBit:
                    replacement = FirstTrailingBit(builtin);
                    break;
                case core::Function::kSaturate:
                    replacement = Saturate(builtin);
                    break;
                case core::Function::kTextureSampleBaseClampToEdge:
                    replacement = TextureSampleBaseClampToEdge_2d_f32(builtin);
                    break;
                default:
                    break;
            }
            TINT_ASSERT_OR_RETURN(replacement);

            // Replace the old builtin call result with the new value.
            if (auto name = ir->NameOf(builtin->Result())) {
                ir->SetName(replacement, name);
            }
            builtin->Result()->ReplaceAllUsesWith(replacement);
            builtin->Destroy();
        }
    }

    /// Return a type with element type @p type that has the same number of vector components as
    /// @p match. If @p match is scalar just return @p type.
    /// @param el_ty the type to extend
    /// @param match the type to match the component count of
    /// @returns a type with the same number of vector components as @p match
    const core::type::Type* MatchWidth(const core::type::Type* el_ty,
                                       const core::type::Type* match) {
        if (auto* vec = match->As<core::type::Vector>()) {
            return ty.vec(el_ty, vec->Width());
        }
        return el_ty;
    }

    /// Return a constant that has the same number of vector components as @p match, each with the
    /// value @p element. If @p match is scalar just return @p element.
    /// @param element the value to extend
    /// @param match the type to match the component count of
    /// @returns a value with the same number of vector components as @p match
    ir::Constant* MatchWidth(ir::Constant* element, const core::type::Type* match) {
        if (auto* vec = match->As<core::type::Vector>()) {
            return b.Splat(MatchWidth(element->Type(), match), element, vec->Width());
        }
        return element;
    }

    /// Polyfill a `countLeadingZeros()` builtin call.
    /// @param call the builtin call instruction
    /// @returns the replacement value
    ir::Value* CountLeadingZeros(ir::CoreBuiltinCall* call) {
        auto* input = call->Args()[0];
        auto* result_ty = input->Type();
        auto* uint_ty = MatchWidth(ty.u32(), result_ty);
        auto* bool_ty = MatchWidth(ty.bool_(), result_ty);

        // Make an u32 constant with the same component count as result_ty.
        auto V = [&](uint32_t u) { return MatchWidth(b.Constant(u32(u)), result_ty); };

        Value* result = nullptr;
        b.InsertBefore(call, [&] {
            // %x = %input;
            // if (%x is signed) {
            //   %x = bitcast<u32>(%x)
            // }
            // %b16 = select(0, 16, %x <= 0x0000ffff);
            // %x <<= %b16;
            // %b8  = select(0, 8, %x <= 0x00ffffff);
            // %x <<= %b8;
            // %b4  = select(0, 4, %x <= 0x0fffffff);
            // %x <<= %b4;
            // %b2  = select(0, 2, %x <= 0x3fffffff);
            // %x <<= %b2;
            // %b1  = select(0, 1, %x <= 0x7fffffff);
            // %b0  = select(0, 1, %x == 0);
            // %result = (%b16 | %b8 | %b4 | %b2 | %b1) + %b0;

            auto* x = input;
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                x = b.Bitcast(uint_ty, x)->Result();
            }
            auto* b16 = b.Call(uint_ty, core::Function::kSelect, V(0), V(16),
                               b.LessThanEqual(bool_ty, x, V(0x0000ffff)));
            x = b.ShiftLeft(uint_ty, x, b16)->Result();
            auto* b8 = b.Call(uint_ty, core::Function::kSelect, V(0), V(8),
                              b.LessThanEqual(bool_ty, x, V(0x00ffffff)));
            x = b.ShiftLeft(uint_ty, x, b8)->Result();
            auto* b4 = b.Call(uint_ty, core::Function::kSelect, V(0), V(4),
                              b.LessThanEqual(bool_ty, x, V(0x0fffffff)));
            x = b.ShiftLeft(uint_ty, x, b4)->Result();
            auto* b2 = b.Call(uint_ty, core::Function::kSelect, V(0), V(2),
                              b.LessThanEqual(bool_ty, x, V(0x3fffffff)));
            x = b.ShiftLeft(uint_ty, x, b2)->Result();
            auto* b1 = b.Call(uint_ty, core::Function::kSelect, V(0), V(1),
                              b.LessThanEqual(bool_ty, x, V(0x7fffffff)));
            auto* b0 =
                b.Call(uint_ty, core::Function::kSelect, V(0), V(1), b.Equal(bool_ty, x, V(0)));
            result = b.Add(uint_ty,
                           b.Or(uint_ty, b16,
                                b.Or(uint_ty, b8,
                                     b.Or(uint_ty, b4, b.Or(uint_ty, b2, b.Or(uint_ty, b1, b0))))),
                           b0)
                         ->Result();
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                result = b.Bitcast(result_ty, result)->Result();
            }
        });
        return result;
    }

    /// Polyfill a `countTrailingZeros()` builtin call.
    /// @param call the builtin call instruction
    /// @returns the replacement value
    ir::Value* CountTrailingZeros(ir::CoreBuiltinCall* call) {
        auto* input = call->Args()[0];
        auto* result_ty = input->Type();
        auto* uint_ty = MatchWidth(ty.u32(), result_ty);
        auto* bool_ty = MatchWidth(ty.bool_(), result_ty);

        // Make an u32 constant with the same component count as result_ty.
        auto V = [&](uint32_t u) { return MatchWidth(b.Constant(u32(u)), result_ty); };

        Value* result = nullptr;
        b.InsertBefore(call, [&] {
            // %x = %input;
            // if (%x is signed) {
            //   %x = bitcast<u32>(%x)
            // }
            // %b16 = select(0, 16, (%x & 0x0000ffff) == 0);
            // %x >>= %b16;
            // %b8  = select(0, 8,  (%x & 0x000000ff) == 0);
            // %x >>= %b8;
            // %b4  = select(0, 4,  (%x & 0x0000000f) == 0);
            // %x >>= %b4;
            // %b2  = select(0, 2,  (%x & 0x00000003) == 0);
            // %x >>= %b2;
            // %b1  = select(0, 1,  (%x & 0x00000001) == 0);
            // %b0  = select(0, 1,  (%x & 0x00000001) == 0);
            // %result = (%b16 | %b8 | %b4 | %b2 | %b1) + %b0;

            auto* x = input;
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                x = b.Bitcast(uint_ty, x)->Result();
            }
            auto* b16 = b.Call(uint_ty, core::Function::kSelect, V(0), V(16),
                               b.Equal(bool_ty, b.And(uint_ty, x, V(0x0000ffff)), V(0)));
            x = b.ShiftRight(uint_ty, x, b16)->Result();
            auto* b8 = b.Call(uint_ty, core::Function::kSelect, V(0), V(8),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x000000ff)), V(0)));
            x = b.ShiftRight(uint_ty, x, b8)->Result();
            auto* b4 = b.Call(uint_ty, core::Function::kSelect, V(0), V(4),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x0000000f)), V(0)));
            x = b.ShiftRight(uint_ty, x, b4)->Result();
            auto* b2 = b.Call(uint_ty, core::Function::kSelect, V(0), V(2),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x00000003)), V(0)));
            x = b.ShiftRight(uint_ty, x, b2)->Result();
            auto* b1 = b.Call(uint_ty, core::Function::kSelect, V(0), V(1),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x00000001)), V(0)));
            auto* b0 =
                b.Call(uint_ty, core::Function::kSelect, V(0), V(1), b.Equal(bool_ty, x, V(0)));
            result = b.Add(uint_ty,
                           b.Or(uint_ty, b16,
                                b.Or(uint_ty, b8, b.Or(uint_ty, b4, b.Or(uint_ty, b2, b1)))),
                           b0)
                         ->Result();
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                result = b.Bitcast(result_ty, result)->Result();
            }
        });
        return result;
    }

    /// Polyfill a `firstLeadingBit()` builtin call.
    /// @param call the builtin call instruction
    /// @returns the replacement value
    ir::Value* FirstLeadingBit(ir::CoreBuiltinCall* call) {
        auto* input = call->Args()[0];
        auto* result_ty = input->Type();
        auto* uint_ty = MatchWidth(ty.u32(), result_ty);
        auto* bool_ty = MatchWidth(ty.bool_(), result_ty);

        // Make an u32 constant with the same component count as result_ty.
        auto V = [&](uint32_t u) { return MatchWidth(b.Constant(u32(u)), result_ty); };

        Value* result = nullptr;
        b.InsertBefore(call, [&] {
            // %x = %input;
            // if (%x is signed) {
            //   %x = select(u32(%x), ~u32(%x), x > 0x80000000);
            // }
            // %b16 = select(16, 0, (%x & 0xffff0000) == 0);
            // %x >>= %b16;
            // %b8  = select(8, 0,  (%x & 0x0000ff00) == 0);
            // %x >>= %b8;
            // %b4  = select(4, 0,  (%x & 0x000000f0) == 0);
            // %x >>= %b4;
            // %b2  = select(2, 0,  (%x & 0x0000000c) == 0);
            // %x >>= %b2;
            // %b1  = select(1, 0,  (%x & 0x00000002) == 0);
            // %result = %b16 | %b8 | %b4 | %b2 | %b1;
            // %result = select(%result, 0xffffffff, %x == 0);

            auto* x = input;
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                x = b.Bitcast(uint_ty, x)->Result();
                auto* inverted = b.Complement(uint_ty, x);
                x = b.Call(uint_ty, core::Function::kSelect, inverted, x,
                           b.LessThan(bool_ty, x, V(0x80000000)))
                        ->Result();
            }
            auto* b16 = b.Call(uint_ty, core::Function::kSelect, V(16), V(0),
                               b.Equal(bool_ty, b.And(uint_ty, x, V(0xffff0000)), V(0)));
            x = b.ShiftRight(uint_ty, x, b16)->Result();
            auto* b8 = b.Call(uint_ty, core::Function::kSelect, V(8), V(0),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x0000ff00)), V(0)));
            x = b.ShiftRight(uint_ty, x, b8)->Result();
            auto* b4 = b.Call(uint_ty, core::Function::kSelect, V(4), V(0),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x000000f0)), V(0)));
            x = b.ShiftRight(uint_ty, x, b4)->Result();
            auto* b2 = b.Call(uint_ty, core::Function::kSelect, V(2), V(0),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x0000000c)), V(0)));
            x = b.ShiftRight(uint_ty, x, b2)->Result();
            auto* b1 = b.Call(uint_ty, core::Function::kSelect, V(1), V(0),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x00000002)), V(0)));
            result = b.Or(uint_ty, b16, b.Or(uint_ty, b8, b.Or(uint_ty, b4, b.Or(uint_ty, b2, b1))))
                         ->Result();
            result = b.Call(uint_ty, core::Function::kSelect, result, V(0xffffffff),
                            b.Equal(bool_ty, x, V(0)))
                         ->Result();
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                result = b.Bitcast(result_ty, result)->Result();
            }
        });
        return result;
    }

    /// Polyfill a `firstTrailingBit()` builtin call.
    /// @param call the builtin call instruction
    /// @returns the replacement value
    ir::Value* FirstTrailingBit(ir::CoreBuiltinCall* call) {
        auto* input = call->Args()[0];
        auto* result_ty = input->Type();
        auto* uint_ty = MatchWidth(ty.u32(), result_ty);
        auto* bool_ty = MatchWidth(ty.bool_(), result_ty);

        // Make an u32 constant with the same component count as result_ty.
        auto V = [&](uint32_t u) { return MatchWidth(b.Constant(u32(u)), result_ty); };

        Value* result = nullptr;
        b.InsertBefore(call, [&] {
            // %x = %input;
            // if (%x is signed) {
            //   %x = bitcast<u32>(%x)
            // }
            // %b16 = select(0, 16, (%x & 0x0000ffff) == 0);
            // %x >>= %b16;
            // %b8  = select(0, 8,  (%x & 0x000000ff) == 0);
            // %x >>= %b8;
            // %b4  = select(0, 4,  (%x & 0x0000000f) == 0);
            // %x >>= %b4;
            // %b2  = select(0, 2,  (%x & 0x00000003) == 0);
            // %x >>= %b2;
            // %b1  = select(0, 1,  (%x & 0x00000001) == 0);
            // %result = %b16 | %b8 | %b4 | %b2 | %b1;
            // %result = select(%result, 0xffffffff, %x == 0);

            auto* x = input;
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                x = b.Bitcast(uint_ty, x)->Result();
            }
            auto* b16 = b.Call(uint_ty, core::Function::kSelect, V(0), V(16),
                               b.Equal(bool_ty, b.And(uint_ty, x, V(0x0000ffff)), V(0)));
            x = b.ShiftRight(uint_ty, x, b16)->Result();
            auto* b8 = b.Call(uint_ty, core::Function::kSelect, V(0), V(8),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x000000ff)), V(0)));
            x = b.ShiftRight(uint_ty, x, b8)->Result();
            auto* b4 = b.Call(uint_ty, core::Function::kSelect, V(0), V(4),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x0000000f)), V(0)));
            x = b.ShiftRight(uint_ty, x, b4)->Result();
            auto* b2 = b.Call(uint_ty, core::Function::kSelect, V(0), V(2),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x00000003)), V(0)));
            x = b.ShiftRight(uint_ty, x, b2)->Result();
            auto* b1 = b.Call(uint_ty, core::Function::kSelect, V(0), V(1),
                              b.Equal(bool_ty, b.And(uint_ty, x, V(0x00000001)), V(0)));
            result = b.Or(uint_ty, b16, b.Or(uint_ty, b8, b.Or(uint_ty, b4, b.Or(uint_ty, b2, b1))))
                         ->Result();
            result = b.Call(uint_ty, core::Function::kSelect, result, V(0xffffffff),
                            b.Equal(bool_ty, x, V(0)))
                         ->Result();
            if (result_ty->is_signed_integer_scalar_or_vector()) {
                result = b.Bitcast(result_ty, result)->Result();
            }
        });
        return result;
    }

    /// Polyfill a `saturate()` builtin call.
    /// @param call the builtin call instruction
    /// @returns the replacement value
    ir::Value* Saturate(ir::CoreBuiltinCall* call) {
        // Replace `saturate(x)` with `clamp(x, 0., 1.)`.
        auto* type = call->Result()->Type();
        ir::Constant* zero = nullptr;
        ir::Constant* one = nullptr;
        if (type->DeepestElement()->Is<core::type::F32>()) {
            zero = MatchWidth(b.Constant(0_f), type);
            one = MatchWidth(b.Constant(1_f), type);
        } else if (type->DeepestElement()->Is<core::type::F16>()) {
            zero = MatchWidth(b.Constant(0_h), type);
            one = MatchWidth(b.Constant(1_h), type);
        }
        auto* clamp = b.Call(type, core::Function::kClamp, Vector{call->Args()[0], zero, one});
        clamp->InsertBefore(call);
        return clamp->Result();
    }

    /// Polyfill a `textureSampleBaseClampToEdge()` builtin call for 2D F32 textures.
    /// @param call the builtin call instruction
    /// @returns the replacement value
    ir::Value* TextureSampleBaseClampToEdge_2d_f32(ir::CoreBuiltinCall* call) {
        // Replace `textureSampleBaseClampToEdge(%texture, %sample, %coords)` with:
        //   %dims       = vec2f(textureDimensions(%texture));
        //   %half_texel = vec2f(0.5) / dims;
        //   %clamped    = clamp(%coord, %half_texel, 1.0 - %half_texel);
        //   %result     = textureSampleLevel(%texture, %sampler, %clamped, 0);
        ir::Value* result = nullptr;
        auto* texture = call->Args()[0];
        auto* sampler = call->Args()[1];
        auto* coords = call->Args()[2];
        b.InsertBefore(call, [&] {
            auto* vec2f = ty.vec2<f32>();
            auto* dims = b.Call(ty.vec2<u32>(), core::Function::kTextureDimensions, texture);
            auto* fdims = b.Convert(vec2f, dims);
            auto* half_texel = b.Divide(vec2f, b.Splat(vec2f, 0.5_f, 2), fdims);
            auto* one_minus_half_texel = b.Subtract(vec2f, b.Splat(vec2f, 1_f, 2), half_texel);
            auto* clamped =
                b.Call(vec2f, core::Function::kClamp, coords, half_texel, one_minus_half_texel);
            result = b.Call(ty.vec4<f32>(), core::Function::kTextureSampleLevel, texture, sampler,
                            clamped, 0_f)
                         ->Result();
        });
        return result;
    }
};

}  // namespace

Result<SuccessType, std::string> BuiltinPolyfill(Module* ir, const BuiltinPolyfillConfig& config) {
    auto result = ValidateAndDumpIfNeeded(*ir, "BuiltinPolyfill transform");
    if (!result) {
        return result;
    }

    State{config, ir}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
