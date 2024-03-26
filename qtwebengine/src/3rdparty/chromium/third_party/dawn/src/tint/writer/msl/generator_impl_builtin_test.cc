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

#include "src/tint/ast/call_statement.h"
#include "src/tint/sem/call.h"
#include "src/tint/writer/msl/test_helper.h"

using namespace tint::number_suffixes;  // NOLINT

namespace tint::writer::msl {
namespace {

using BuiltinType = sem::BuiltinType;

using MslGeneratorImplTest = TestHelper;

enum class CallParamType {
    kF32,
    kU32,
    kBool,
    kF16,
};

struct BuiltinData {
    BuiltinType builtin;
    CallParamType type;
    const char* msl_name;
};
inline std::ostream& operator<<(std::ostream& out, BuiltinData data) {
    out << data.msl_name << "<";
    switch (data.type) {
        case CallParamType::kF32:
            out << "f32";
            break;
        case CallParamType::kU32:
            out << "u32";
            break;
        case CallParamType::kBool:
            out << "bool";
            break;
        case CallParamType::kF16:
            out << "f16";
            break;
    }
    out << ">";
    return out;
}

const ast::CallExpression* GenerateCall(BuiltinType builtin,
                                        CallParamType type,
                                        ProgramBuilder* builder) {
    std::string name;
    std::ostringstream str(name);
    str << builtin;
    switch (builtin) {
        case BuiltinType::kAcos:
        case BuiltinType::kAsin:
        case BuiltinType::kAtan:
        case BuiltinType::kCeil:
        case BuiltinType::kCos:
        case BuiltinType::kCosh:
        case BuiltinType::kDpdx:
        case BuiltinType::kDpdxCoarse:
        case BuiltinType::kDpdxFine:
        case BuiltinType::kDpdy:
        case BuiltinType::kDpdyCoarse:
        case BuiltinType::kDpdyFine:
        case BuiltinType::kExp:
        case BuiltinType::kExp2:
        case BuiltinType::kFloor:
        case BuiltinType::kFract:
        case BuiltinType::kFwidth:
        case BuiltinType::kFwidthCoarse:
        case BuiltinType::kFwidthFine:
        case BuiltinType::kInverseSqrt:
        case BuiltinType::kLength:
        case BuiltinType::kLog:
        case BuiltinType::kLog2:
        case BuiltinType::kNormalize:
        case BuiltinType::kRound:
        case BuiltinType::kSin:
        case BuiltinType::kSinh:
        case BuiltinType::kSqrt:
        case BuiltinType::kTan:
        case BuiltinType::kTanh:
        case BuiltinType::kTrunc:
        case BuiltinType::kSign:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2");
            } else {
                return builder->Call(str.str(), "f2");
            }
        case BuiltinType::kLdexp:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "i2");
            } else {
                return builder->Call(str.str(), "f2", "i2");
            }
        case BuiltinType::kAtan2:
        case BuiltinType::kDot:
        case BuiltinType::kDistance:
        case BuiltinType::kPow:
        case BuiltinType::kReflect:
        case BuiltinType::kStep:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2");
            } else {
                return builder->Call(str.str(), "f2", "f2");
            }
        case BuiltinType::kStorageBarrier:
            return builder->Call(str.str());
        case BuiltinType::kCross:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h3", "h3");
            } else {
                return builder->Call(str.str(), "f3", "f3");
            }
        case BuiltinType::kFma:
        case BuiltinType::kMix:
        case BuiltinType::kFaceForward:
        case BuiltinType::kSmoothstep:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2", "h2");
            } else {
                return builder->Call(str.str(), "f2", "f2", "f2");
            }
        case BuiltinType::kAll:
        case BuiltinType::kAny:
            return builder->Call(str.str(), "b2");
        case BuiltinType::kAbs:
            if (type == CallParamType::kF32) {
                return builder->Call(str.str(), "f2");
            } else if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2");
            } else {
                return builder->Call(str.str(), "u2");
            }
        case BuiltinType::kCountLeadingZeros:
        case BuiltinType::kCountOneBits:
        case BuiltinType::kCountTrailingZeros:
        case BuiltinType::kReverseBits:
            return builder->Call(str.str(), "u2");
        case BuiltinType::kExtractBits:
            return builder->Call(str.str(), "u2", "u1", "u1");
        case BuiltinType::kInsertBits:
            return builder->Call(str.str(), "u2", "u2", "u1", "u1");
        case BuiltinType::kMax:
        case BuiltinType::kMin:
            if (type == CallParamType::kF32) {
                return builder->Call(str.str(), "f2", "f2");
            } else if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2");
            } else {
                return builder->Call(str.str(), "u2", "u2");
            }
        case BuiltinType::kClamp:
            if (type == CallParamType::kF32) {
                return builder->Call(str.str(), "f2", "f2", "f2");
            } else if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2", "h2");
            } else {
                return builder->Call(str.str(), "u2", "u2", "u2");
            }
        case BuiltinType::kSelect:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2", "b2");
            } else {
                return builder->Call(str.str(), "f2", "f2", "b2");
            }
        case BuiltinType::kDeterminant:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "hm2x2");
            } else {
                return builder->Call(str.str(), "m2x2");
            }
        case BuiltinType::kPack2X16Snorm:
        case BuiltinType::kPack2X16Unorm:
            return builder->Call(str.str(), "f2");
        case BuiltinType::kPack4X8Snorm:
        case BuiltinType::kPack4X8Unorm:
            return builder->Call(str.str(), "f4");
        case BuiltinType::kUnpack4X8Snorm:
        case BuiltinType::kUnpack4X8Unorm:
        case BuiltinType::kUnpack2X16Snorm:
        case BuiltinType::kUnpack2X16Unorm:
            return builder->Call(str.str(), "u1");
        case BuiltinType::kWorkgroupBarrier:
            return builder->Call(str.str());
        case BuiltinType::kTranspose:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "hm3x2");
            } else {
                return builder->Call(str.str(), "m3x2");
            }
        default:
            break;
    }
    return nullptr;
}

using MslBuiltinTest = TestParamHelper<BuiltinData>;
TEST_P(MslBuiltinTest, Emit) {
    auto param = GetParam();

    if (param.type == CallParamType::kF16) {
        Enable(builtin::Extension::kF16);

        GlobalVar("h2", ty.vec2<f16>(), builtin::AddressSpace::kPrivate);
        GlobalVar("h3", ty.vec3<f16>(), builtin::AddressSpace::kPrivate);
        GlobalVar("hm2x2", ty.mat2x2<f16>(), builtin::AddressSpace::kPrivate);
        GlobalVar("hm3x2", ty.mat3x2<f16>(), builtin::AddressSpace::kPrivate);
    }

    GlobalVar("f2", ty.vec2<f32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("f3", ty.vec3<f32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("f4", ty.vec4<f32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("u1", ty.u32(), builtin::AddressSpace::kPrivate);
    GlobalVar("u2", ty.vec2<u32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("i2", ty.vec2<i32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("b2", ty.vec2<bool>(), builtin::AddressSpace::kPrivate);
    GlobalVar("m2x2", ty.mat2x2<f32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("m3x2", ty.mat3x2<f32>(), builtin::AddressSpace::kPrivate);

    auto* call = GenerateCall(param.builtin, param.type, this);
    ASSERT_NE(nullptr, call) << "Unhandled builtin";
    Func("func", utils::Empty, ty.void_(), utils::Vector{Ignore(call)},
         utils::Vector{create<ast::StageAttribute>(ast::PipelineStage::kFragment)});

    GeneratorImpl& gen = Build();

    auto* sem = program->Sem().Get<sem::Call>(call);
    ASSERT_NE(sem, nullptr);
    auto* target = sem->Target();
    ASSERT_NE(target, nullptr);
    auto* builtin = target->As<sem::Builtin>();
    ASSERT_NE(builtin, nullptr);

    EXPECT_EQ(gen.generate_builtin_name(builtin), param.msl_name);
}
INSTANTIATE_TEST_SUITE_P(
    MslGeneratorImplTest,
    MslBuiltinTest,
    testing::Values(
        /* Logical built-in */
        BuiltinData{BuiltinType::kAll, CallParamType::kBool, "all"},
        BuiltinData{BuiltinType::kAny, CallParamType::kBool, "any"},
        BuiltinData{BuiltinType::kSelect, CallParamType::kF32, "select"},
        /* Float built-in */
        BuiltinData{BuiltinType::kAbs, CallParamType::kF32, "fabs"},
        BuiltinData{BuiltinType::kAbs, CallParamType::kF16, "fabs"},
        BuiltinData{BuiltinType::kAcos, CallParamType::kF32, "acos"},
        BuiltinData{BuiltinType::kAcos, CallParamType::kF16, "acos"},
        BuiltinData{BuiltinType::kAsin, CallParamType::kF32, "asin"},
        BuiltinData{BuiltinType::kAsin, CallParamType::kF16, "asin"},
        BuiltinData{BuiltinType::kAtan, CallParamType::kF32, "atan"},
        BuiltinData{BuiltinType::kAtan, CallParamType::kF16, "atan"},
        BuiltinData{BuiltinType::kAtan2, CallParamType::kF32, "atan2"},
        BuiltinData{BuiltinType::kAtan2, CallParamType::kF16, "atan2"},
        BuiltinData{BuiltinType::kCeil, CallParamType::kF32, "ceil"},
        BuiltinData{BuiltinType::kCeil, CallParamType::kF16, "ceil"},
        BuiltinData{BuiltinType::kClamp, CallParamType::kF32, "clamp"},
        BuiltinData{BuiltinType::kClamp, CallParamType::kF16, "clamp"},
        BuiltinData{BuiltinType::kCos, CallParamType::kF32, "cos"},
        BuiltinData{BuiltinType::kCos, CallParamType::kF16, "cos"},
        BuiltinData{BuiltinType::kCosh, CallParamType::kF32, "cosh"},
        BuiltinData{BuiltinType::kCosh, CallParamType::kF16, "cosh"},
        BuiltinData{BuiltinType::kCross, CallParamType::kF32, "cross"},
        BuiltinData{BuiltinType::kCross, CallParamType::kF16, "cross"},
        BuiltinData{BuiltinType::kDistance, CallParamType::kF32, "distance"},
        BuiltinData{BuiltinType::kDistance, CallParamType::kF16, "distance"},
        BuiltinData{BuiltinType::kExp, CallParamType::kF32, "exp"},
        BuiltinData{BuiltinType::kExp, CallParamType::kF16, "exp"},
        BuiltinData{BuiltinType::kExp2, CallParamType::kF32, "exp2"},
        BuiltinData{BuiltinType::kExp2, CallParamType::kF16, "exp2"},
        BuiltinData{BuiltinType::kFaceForward, CallParamType::kF32, "faceforward"},
        BuiltinData{BuiltinType::kFaceForward, CallParamType::kF16, "faceforward"},
        BuiltinData{BuiltinType::kFloor, CallParamType::kF32, "floor"},
        BuiltinData{BuiltinType::kFloor, CallParamType::kF16, "floor"},
        BuiltinData{BuiltinType::kFma, CallParamType::kF32, "fma"},
        BuiltinData{BuiltinType::kFma, CallParamType::kF16, "fma"},
        BuiltinData{BuiltinType::kFract, CallParamType::kF32, "fract"},
        BuiltinData{BuiltinType::kFract, CallParamType::kF16, "fract"},
        BuiltinData{BuiltinType::kInverseSqrt, CallParamType::kF32, "rsqrt"},
        BuiltinData{BuiltinType::kInverseSqrt, CallParamType::kF16, "rsqrt"},
        BuiltinData{BuiltinType::kLdexp, CallParamType::kF32, "ldexp"},
        BuiltinData{BuiltinType::kLdexp, CallParamType::kF16, "ldexp"},
        BuiltinData{BuiltinType::kLength, CallParamType::kF32, "length"},
        BuiltinData{BuiltinType::kLength, CallParamType::kF16, "length"},
        BuiltinData{BuiltinType::kLog, CallParamType::kF32, "log"},
        BuiltinData{BuiltinType::kLog, CallParamType::kF16, "log"},
        BuiltinData{BuiltinType::kLog2, CallParamType::kF32, "log2"},
        BuiltinData{BuiltinType::kLog2, CallParamType::kF16, "log2"},
        BuiltinData{BuiltinType::kMax, CallParamType::kF32, "fmax"},
        BuiltinData{BuiltinType::kMax, CallParamType::kF16, "fmax"},
        BuiltinData{BuiltinType::kMin, CallParamType::kF32, "fmin"},
        BuiltinData{BuiltinType::kMin, CallParamType::kF16, "fmin"},
        BuiltinData{BuiltinType::kNormalize, CallParamType::kF32, "normalize"},
        BuiltinData{BuiltinType::kNormalize, CallParamType::kF16, "normalize"},
        BuiltinData{BuiltinType::kPow, CallParamType::kF32, "pow"},
        BuiltinData{BuiltinType::kPow, CallParamType::kF16, "pow"},
        BuiltinData{BuiltinType::kReflect, CallParamType::kF32, "reflect"},
        BuiltinData{BuiltinType::kReflect, CallParamType::kF16, "reflect"},
        BuiltinData{BuiltinType::kSign, CallParamType::kF32, "sign"},
        BuiltinData{BuiltinType::kSign, CallParamType::kF16, "sign"},
        BuiltinData{BuiltinType::kSin, CallParamType::kF32, "sin"},
        BuiltinData{BuiltinType::kSin, CallParamType::kF16, "sin"},
        BuiltinData{BuiltinType::kSinh, CallParamType::kF32, "sinh"},
        BuiltinData{BuiltinType::kSinh, CallParamType::kF16, "sinh"},
        BuiltinData{BuiltinType::kSmoothstep, CallParamType::kF32, "smoothstep"},
        BuiltinData{BuiltinType::kSmoothstep, CallParamType::kF16, "smoothstep"},
        BuiltinData{BuiltinType::kSqrt, CallParamType::kF32, "sqrt"},
        BuiltinData{BuiltinType::kSqrt, CallParamType::kF16, "sqrt"},
        BuiltinData{BuiltinType::kStep, CallParamType::kF32, "step"},
        BuiltinData{BuiltinType::kStep, CallParamType::kF16, "step"},
        BuiltinData{BuiltinType::kTan, CallParamType::kF32, "tan"},
        BuiltinData{BuiltinType::kTan, CallParamType::kF16, "tan"},
        BuiltinData{BuiltinType::kTanh, CallParamType::kF32, "tanh"},
        BuiltinData{BuiltinType::kTanh, CallParamType::kF16, "tanh"},
        BuiltinData{BuiltinType::kTrunc, CallParamType::kF32, "trunc"},
        BuiltinData{BuiltinType::kTrunc, CallParamType::kF16, "trunc"},
        /* Integer built-in */
        BuiltinData{BuiltinType::kAbs, CallParamType::kU32, "abs"},
        BuiltinData{BuiltinType::kClamp, CallParamType::kU32, "clamp"},
        BuiltinData{BuiltinType::kCountLeadingZeros, CallParamType::kU32, "clz"},
        BuiltinData{BuiltinType::kCountOneBits, CallParamType::kU32, "popcount"},
        BuiltinData{BuiltinType::kCountTrailingZeros, CallParamType::kU32, "ctz"},
        BuiltinData{BuiltinType::kExtractBits, CallParamType::kU32, "extract_bits"},
        BuiltinData{BuiltinType::kInsertBits, CallParamType::kU32, "insert_bits"},
        BuiltinData{BuiltinType::kMax, CallParamType::kU32, "max"},
        BuiltinData{BuiltinType::kMin, CallParamType::kU32, "min"},
        BuiltinData{BuiltinType::kReverseBits, CallParamType::kU32, "reverse_bits"},
        BuiltinData{BuiltinType::kRound, CallParamType::kU32, "rint"},
        /* Matrix built-in */
        BuiltinData{BuiltinType::kDeterminant, CallParamType::kF32, "determinant"},
        BuiltinData{BuiltinType::kTranspose, CallParamType::kF32, "transpose"},
        /* Vector built-in */
        BuiltinData{BuiltinType::kDot, CallParamType::kF32, "dot"},
        /* Derivate built-in */
        BuiltinData{BuiltinType::kDpdx, CallParamType::kF32, "dfdx"},
        BuiltinData{BuiltinType::kDpdxCoarse, CallParamType::kF32, "dfdx"},
        BuiltinData{BuiltinType::kDpdxFine, CallParamType::kF32, "dfdx"},
        BuiltinData{BuiltinType::kDpdy, CallParamType::kF32, "dfdy"},
        BuiltinData{BuiltinType::kDpdyCoarse, CallParamType::kF32, "dfdy"},
        BuiltinData{BuiltinType::kDpdyFine, CallParamType::kF32, "dfdy"},
        BuiltinData{BuiltinType::kFwidth, CallParamType::kF32, "fwidth"},
        BuiltinData{BuiltinType::kFwidthCoarse, CallParamType::kF32, "fwidth"},
        BuiltinData{BuiltinType::kFwidthFine, CallParamType::kF32, "fwidth"},
        /* Data packing builtin */
        BuiltinData{BuiltinType::kPack4X8Snorm, CallParamType::kF32, "pack_float_to_snorm4x8"},
        BuiltinData{BuiltinType::kPack4X8Unorm, CallParamType::kF32, "pack_float_to_unorm4x8"},
        BuiltinData{BuiltinType::kPack2X16Snorm, CallParamType::kF32, "pack_float_to_snorm2x16"},
        BuiltinData{BuiltinType::kPack2X16Unorm, CallParamType::kF32, "pack_float_to_unorm2x16"},
        /* Data unpacking builtin */
        BuiltinData{BuiltinType::kUnpack4X8Snorm, CallParamType::kU32, "unpack_snorm4x8_to_float"},
        BuiltinData{BuiltinType::kUnpack4X8Unorm, CallParamType::kU32, "unpack_unorm4x8_to_float"},
        BuiltinData{BuiltinType::kUnpack2X16Snorm, CallParamType::kU32,
                    "unpack_snorm2x16_to_float"},
        BuiltinData{BuiltinType::kUnpack2X16Unorm, CallParamType::kU32,
                    "unpack_unorm2x16_to_float"}));

TEST_F(MslGeneratorImplTest, Builtin_Call) {
    GlobalVar("param1", ty.vec2<f32>(), builtin::AddressSpace::kPrivate);
    GlobalVar("param2", ty.vec2<f32>(), builtin::AddressSpace::kPrivate);

    auto* call = Call("dot", "param1", "param2");
    WrapInFunction(Decl(Var("r", call)));

    GeneratorImpl& gen = Build();

    std::stringstream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.error();
    EXPECT_EQ(out.str(), "dot(param1, param2)");
}

TEST_F(MslGeneratorImplTest, StorageBarrier) {
    auto* call = Call("storageBarrier");
    WrapInFunction(CallStmt(call));

    GeneratorImpl& gen = Build();

    std::stringstream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.error();
    EXPECT_EQ(out.str(), "threadgroup_barrier(mem_flags::mem_device)");
}

TEST_F(MslGeneratorImplTest, WorkgroupBarrier) {
    auto* call = Call("workgroupBarrier");
    WrapInFunction(CallStmt(call));

    GeneratorImpl& gen = Build();

    std::stringstream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.error();
    EXPECT_EQ(out.str(), "threadgroup_barrier(mem_flags::mem_threadgroup)");
}

TEST_F(MslGeneratorImplTest, Runtime_Modf_Scalar_f32) {
    WrapInFunction(Decl(Let("f", Expr(1.5_f))),  //
                   Decl(Let("v", Call("modf", "f"))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_f32 {
  float fract;
  float whole;
};
modf_result_f32 tint_modf(float param_0) {
  modf_result_f32 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

kernel void test_function() {
  float const f = 1.5f;
  modf_result_f32 const v = tint_modf(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Modf_Scalar_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Decl(Let("f", Expr(1.5_h))),  //
                   Decl(Let("v", Call("modf", "f"))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_f16 {
  half fract;
  half whole;
};
modf_result_f16 tint_modf(half param_0) {
  modf_result_f16 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

kernel void test_function() {
  half const f = 1.5h;
  modf_result_f16 const v = tint_modf(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Modf_Vector_f32) {
    WrapInFunction(Decl(Let("f", vec3<f32>(1.5_f, 2.5_f, 3.5_f))),  //
                   Decl(Let("v", Call("modf", "f"))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_vec3_f32 {
  float3 fract;
  float3 whole;
};
modf_result_vec3_f32 tint_modf(float3 param_0) {
  modf_result_vec3_f32 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

kernel void test_function() {
  float3 const f = float3(1.5f, 2.5f, 3.5f);
  modf_result_vec3_f32 const v = tint_modf(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Modf_Vector_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Decl(Let("f", vec3<f16>(1.5_h, 2.5_h, 3.5_h))),  //
                   Decl(Let("v", Call("modf", "f"))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_vec3_f16 {
  half3 fract;
  half3 whole;
};
modf_result_vec3_f16 tint_modf(half3 param_0) {
  modf_result_vec3_f16 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

kernel void test_function() {
  half3 const f = half3(1.5h, 2.5h, 3.5h);
  modf_result_vec3_f16 const v = tint_modf(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Modf_Scalar_f32) {
    WrapInFunction(Decl(Let("v", Call("modf", 1.5_f))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_f32 {
  float fract;
  float whole;
};
kernel void test_function() {
  modf_result_f32 const v = modf_result_f32{.fract=0.5f, .whole=1.0f};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Modf_Scalar_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("modf", 1.5_h))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_f16 {
  half fract;
  half whole;
};
kernel void test_function() {
  modf_result_f16 const v = modf_result_f16{.fract=0.5h, .whole=1.0h};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Modf_Vector_f32) {
    WrapInFunction(Decl(Let("v", Call("modf", vec3<f32>(1.5_f, 2.5_f, 3.5_f)))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_vec3_f32 {
  float3 fract;
  float3 whole;
};
kernel void test_function() {
  modf_result_vec3_f32 const v = modf_result_vec3_f32{.fract=float3(0.5f), .whole=float3(1.0f, 2.0f, 3.0f)};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Modf_Vector_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("modf", vec3<f16>(1.5_h, 2.5_h, 3.5_h)))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct modf_result_vec3_f16 {
  half3 fract;
  half3 whole;
};
kernel void test_function() {
  modf_result_vec3_f16 const v = modf_result_vec3_f16{.fract=half3(0.5h), .whole=half3(1.0h, 2.0h, 3.0h)};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Frexp_Scalar_f32) {
    WrapInFunction(Var("f", Expr(1_f)),  //
                   Var("v", Call("frexp", "f")));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_f32 {
  float fract;
  int exp;
};
frexp_result_f32 tint_frexp(float param_0) {
  frexp_result_f32 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}

kernel void test_function() {
  float f = 1.0f;
  frexp_result_f32 v = tint_frexp(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Frexp_Scalar_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Var("f", Expr(1_h)),  //
                   Var("v", Call("frexp", "f")));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_f16 {
  half fract;
  int exp;
};
frexp_result_f16 tint_frexp(half param_0) {
  frexp_result_f16 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}

kernel void test_function() {
  half f = 1.0h;
  frexp_result_f16 v = tint_frexp(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Frexp_Vector_f32) {
    WrapInFunction(Var("f", Expr(vec3<f32>())),  //
                   Var("v", Call("frexp", "f")));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_vec3_f32 {
  float3 fract;
  int3 exp;
};
frexp_result_vec3_f32 tint_frexp(float3 param_0) {
  frexp_result_vec3_f32 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}

kernel void test_function() {
  float3 f = float3(0.0f);
  frexp_result_vec3_f32 v = tint_frexp(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Runtime_Frexp_Vector_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Var("f", Expr(vec3<f16>())),  //
                   Var("v", Call("frexp", "f")));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_vec3_f16 {
  half3 fract;
  int3 exp;
};
frexp_result_vec3_f16 tint_frexp(half3 param_0) {
  frexp_result_vec3_f16 result;
  result.fract = frexp(param_0, result.exp);
  return result;
}

kernel void test_function() {
  half3 f = half3(0.0h);
  frexp_result_vec3_f16 v = tint_frexp(f);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Frexp_Scalar_f32) {
    WrapInFunction(Decl(Let("v", Call("frexp", 1_f))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_f32 {
  float fract;
  int exp;
};
kernel void test_function() {
  frexp_result_f32 const v = frexp_result_f32{.fract=0.5f, .exp=1};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Frexp_Scalar_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("frexp", 1_h))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_f16 {
  half fract;
  int exp;
};
kernel void test_function() {
  frexp_result_f16 const v = frexp_result_f16{.fract=0.5h, .exp=1};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Frexp_Vector_f32) {
    WrapInFunction(Decl(Let("v", Call("frexp", vec3<f32>()))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_vec3_f32 {
  float3 fract;
  int3 exp;
};
kernel void test_function() {
  frexp_result_vec3_f32 const v = frexp_result_vec3_f32{};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Const_Frexp_Vector_f16) {
    Enable(builtin::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("frexp", vec3<f16>()))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

struct frexp_result_vec3_f16 {
  half3 fract;
  int3 exp;
};
kernel void test_function() {
  frexp_result_vec3_f16 const v = frexp_result_vec3_f16{};
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Degrees_Scalar_f32) {
    auto* val = Var("val", ty.f32());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

float tint_degrees(float param_0) {
  return param_0 * 57.295779513082322865;
}

kernel void test_function() {
  float val = 0.0f;
  float const tint_symbol = tint_degrees(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Degrees_Vector_f32) {
    auto* val = Var("val", ty.vec3<f32>());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

float3 tint_degrees(float3 param_0) {
  return param_0 * 57.295779513082322865;
}

kernel void test_function() {
  float3 val = 0.0f;
  float3 const tint_symbol = tint_degrees(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Degrees_Scalar_f16) {
    Enable(builtin::Extension::kF16);

    auto* val = Var("val", ty.f16());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

half tint_degrees(half param_0) {
  return param_0 * 57.295779513082322865;
}

kernel void test_function() {
  half val = 0.0h;
  half const tint_symbol = tint_degrees(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Degrees_Vector_f16) {
    Enable(builtin::Extension::kF16);

    auto* val = Var("val", ty.vec3<f16>());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

half3 tint_degrees(half3 param_0) {
  return param_0 * 57.295779513082322865;
}

kernel void test_function() {
  half3 val = 0.0h;
  half3 const tint_symbol = tint_degrees(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Radians_Scalar_f32) {
    auto* val = Var("val", ty.f32());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

float tint_radians(float param_0) {
  return param_0 * 0.017453292519943295474;
}

kernel void test_function() {
  float val = 0.0f;
  float const tint_symbol = tint_radians(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Radians_Vector_f32) {
    auto* val = Var("val", ty.vec3<f32>());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

float3 tint_radians(float3 param_0) {
  return param_0 * 0.017453292519943295474;
}

kernel void test_function() {
  float3 val = 0.0f;
  float3 const tint_symbol = tint_radians(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Radians_Scalar_f16) {
    Enable(builtin::Extension::kF16);

    auto* val = Var("val", ty.f16());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

half tint_radians(half param_0) {
  return param_0 * 0.017453292519943295474;
}

kernel void test_function() {
  half val = 0.0h;
  half const tint_symbol = tint_radians(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Radians_Vector_f16) {
    Enable(builtin::Extension::kF16);

    auto* val = Var("val", ty.vec3<f16>());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

half3 tint_radians(half3 param_0) {
  return param_0 * 0.017453292519943295474;
}

kernel void test_function() {
  half3 val = 0.0h;
  half3 const tint_symbol = tint_radians(val);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Pack2x16Float) {
    auto* call = Call("pack2x16float", "p1");
    GlobalVar("p1", ty.vec2<f32>(), builtin::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));

    GeneratorImpl& gen = Build();

    std::stringstream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.error();
    EXPECT_EQ(out.str(), "as_type<uint>(half2(p1))");
}

TEST_F(MslGeneratorImplTest, Unpack2x16Float) {
    auto* call = Call("unpack2x16float", "p1");
    GlobalVar("p1", ty.u32(), builtin::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));

    GeneratorImpl& gen = Build();

    std::stringstream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.error();
    EXPECT_EQ(out.str(), "float2(as_type<half2>(p1))");
}

TEST_F(MslGeneratorImplTest, DotI32) {
    GlobalVar("v", ty.vec3<i32>(), builtin::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", Call("dot", "v", "v"))));

    GeneratorImpl& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;

template<typename T>
T tint_dot3(vec<T,3> a, vec<T,3> b) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
kernel void test_function() {
  thread int3 tint_symbol = 0;
  int r = tint_dot3(tint_symbol, tint_symbol);
  return;
}

)");
}

TEST_F(MslGeneratorImplTest, Ignore) {
    Func("f", utils::Vector{Param("a", ty.i32()), Param("b", ty.i32()), Param("c", ty.i32())},
         ty.i32(), utils::Vector{Return(Mul(Add("a", "b"), "c"))});

    Func("func", utils::Empty, ty.void_(), utils::Vector{CallStmt(Call("f", 1_i, 2_i, 3_i))},
         utils::Vector{
             Stage(ast::PipelineStage::kCompute),
             WorkgroupSize(1_i),
         });

    GeneratorImpl& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.error();
    EXPECT_EQ(gen.result(), R"(#include <metal_stdlib>

using namespace metal;
int f(int a, int b, int c) {
  return as_type<int>((as_type<uint>(as_type<int>((as_type<uint>(a) + as_type<uint>(b)))) * as_type<uint>(c)));
}

kernel void func() {
  f(1, 2, 3);
  return;
}

)");
}

}  // namespace
}  // namespace tint::writer::msl
