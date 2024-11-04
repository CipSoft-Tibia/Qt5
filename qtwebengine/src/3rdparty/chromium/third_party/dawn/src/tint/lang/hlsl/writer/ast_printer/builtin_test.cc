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

#include "gmock/gmock.h"
#include "src/tint/lang/hlsl/writer/ast_printer/helper_test.h"
#include "src/tint/lang/wgsl/ast/call_statement.h"
#include "src/tint/lang/wgsl/ast/stage_attribute.h"
#include "src/tint/lang/wgsl/sem/call.h"
#include "src/tint/utils/text/string_stream.h"

namespace tint::hlsl::writer {
namespace {

using ::testing::HasSubstr;
using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using HlslASTPrinterTest_Builtin = TestHelper;

enum class CallParamType {
    kF32,
    kU32,
    kBool,
    kF16,
};

struct BuiltinData {
    core::Function builtin;
    CallParamType type;
    const char* hlsl_name;
};
inline std::ostream& operator<<(std::ostream& out, BuiltinData data) {
    out << data.hlsl_name << "<";
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

const ast::CallExpression* GenerateCall(core::Function builtin,
                                        CallParamType type,
                                        ProgramBuilder* builder) {
    std::string name;
    StringStream str;
    str << name << builtin;
    switch (builtin) {
        case core::Function::kAcos:
        case core::Function::kAsin:
        case core::Function::kAtan:
        case core::Function::kCeil:
        case core::Function::kCos:
        case core::Function::kCosh:
        case core::Function::kDpdx:
        case core::Function::kDpdxCoarse:
        case core::Function::kDpdxFine:
        case core::Function::kDpdy:
        case core::Function::kDpdyCoarse:
        case core::Function::kDpdyFine:
        case core::Function::kExp:
        case core::Function::kExp2:
        case core::Function::kFloor:
        case core::Function::kFract:
        case core::Function::kFwidth:
        case core::Function::kFwidthCoarse:
        case core::Function::kFwidthFine:
        case core::Function::kInverseSqrt:
        case core::Function::kLength:
        case core::Function::kLog:
        case core::Function::kLog2:
        case core::Function::kNormalize:
        case core::Function::kRound:
        case core::Function::kSin:
        case core::Function::kSinh:
        case core::Function::kSqrt:
        case core::Function::kTan:
        case core::Function::kTanh:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2");
            } else {
                return builder->Call(str.str(), "f2");
            }
        case core::Function::kLdexp:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "i2");
            } else {
                return builder->Call(str.str(), "f2", "i2");
            }
        case core::Function::kAtan2:
        case core::Function::kDot:
        case core::Function::kDistance:
        case core::Function::kPow:
        case core::Function::kReflect:
        case core::Function::kStep:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2");
            } else {
                return builder->Call(str.str(), "f2", "f2");
            }
        case core::Function::kCross:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h3", "h3");
            } else {
                return builder->Call(str.str(), "f3", "f3");
            }
        case core::Function::kFma:
        case core::Function::kMix:
        case core::Function::kFaceForward:
        case core::Function::kSmoothstep:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2", "h2");
            } else {
                return builder->Call(str.str(), "f2", "f2", "f2");
            }
        case core::Function::kAll:
        case core::Function::kAny:
            return builder->Call(str.str(), "b2");
        case core::Function::kAbs:
            if (type == CallParamType::kF32) {
                return builder->Call(str.str(), "f2");
            } else if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2");
            } else {
                return builder->Call(str.str(), "u2");
            }
        case core::Function::kCountOneBits:
        case core::Function::kReverseBits:
            return builder->Call(str.str(), "u2");
        case core::Function::kMax:
        case core::Function::kMin:
            if (type == CallParamType::kF32) {
                return builder->Call(str.str(), "f2", "f2");
            } else if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2");
            } else {
                return builder->Call(str.str(), "u2", "u2");
            }
        case core::Function::kClamp:
            if (type == CallParamType::kF32) {
                return builder->Call(str.str(), "f2", "f2", "f2");
            } else if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2", "h2");
            } else {
                return builder->Call(str.str(), "u2", "u2", "u2");
            }
        case core::Function::kSelect:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "h2", "h2", "b2");
            } else {
                return builder->Call(str.str(), "f2", "f2", "b2");
            }
        case core::Function::kDeterminant:
            if (type == CallParamType::kF16) {
                return builder->Call(str.str(), "hm2x2");
            } else {
                return builder->Call(str.str(), "m2x2");
            }
        case core::Function::kTranspose:
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

using HlslBuiltinTest = TestParamHelper<BuiltinData>;
TEST_P(HlslBuiltinTest, Emit) {
    auto param = GetParam();

    if (param.type == CallParamType::kF16) {
        Enable(core::Extension::kF16);

        GlobalVar("h2", ty.vec2<f16>(), core::AddressSpace::kPrivate);
        GlobalVar("h3", ty.vec3<f16>(), core::AddressSpace::kPrivate);
        GlobalVar("hm2x2", ty.mat2x2<f16>(), core::AddressSpace::kPrivate);
        GlobalVar("hm3x2", ty.mat3x2<f16>(), core::AddressSpace::kPrivate);
    }

    GlobalVar("f2", ty.vec2<f32>(), core::AddressSpace::kPrivate);
    GlobalVar("f3", ty.vec3<f32>(), core::AddressSpace::kPrivate);
    GlobalVar("u2", ty.vec2<u32>(), core::AddressSpace::kPrivate);
    GlobalVar("i2", ty.vec2<i32>(), core::AddressSpace::kPrivate);
    GlobalVar("b2", ty.vec2<bool>(), core::AddressSpace::kPrivate);
    GlobalVar("m2x2", ty.mat2x2<f32>(), core::AddressSpace::kPrivate);
    GlobalVar("m3x2", ty.mat3x2<f32>(), core::AddressSpace::kPrivate);

    auto* call = GenerateCall(param.builtin, param.type, this);
    ASSERT_NE(nullptr, call) << "Unhandled builtin";
    Func("func", tint::Empty, ty.void_(),
         Vector{
             Assign(Phony(), call),
         },
         Vector{create<ast::StageAttribute>(ast::PipelineStage::kFragment)});

    ASTPrinter& gen = Build();

    auto* sem = program->Sem().Get<sem::Call>(call);
    ASSERT_NE(sem, nullptr);
    auto* target = sem->Target();
    ASSERT_NE(target, nullptr);
    auto* builtin = target->As<sem::Builtin>();
    ASSERT_NE(builtin, nullptr);

    EXPECT_EQ(gen.generate_builtin_name(builtin), param.hlsl_name);
}
INSTANTIATE_TEST_SUITE_P(
    HlslASTPrinterTest_Builtin,
    HlslBuiltinTest,
    testing::Values(/* Logical built-in */
                    BuiltinData{core::Function::kAll, CallParamType::kBool, "all"},
                    BuiltinData{core::Function::kAny, CallParamType::kBool, "any"},
                    /* Float built-in */
                    BuiltinData{core::Function::kAbs, CallParamType::kF32, "abs"},
                    BuiltinData{core::Function::kAbs, CallParamType::kF16, "abs"},
                    BuiltinData{core::Function::kAcos, CallParamType::kF32, "acos"},
                    BuiltinData{core::Function::kAcos, CallParamType::kF16, "acos"},
                    BuiltinData{core::Function::kAsin, CallParamType::kF32, "asin"},
                    BuiltinData{core::Function::kAsin, CallParamType::kF16, "asin"},
                    BuiltinData{core::Function::kAtan, CallParamType::kF32, "atan"},
                    BuiltinData{core::Function::kAtan, CallParamType::kF16, "atan"},
                    BuiltinData{core::Function::kAtan2, CallParamType::kF32, "atan2"},
                    BuiltinData{core::Function::kAtan2, CallParamType::kF16, "atan2"},
                    BuiltinData{core::Function::kCeil, CallParamType::kF32, "ceil"},
                    BuiltinData{core::Function::kCeil, CallParamType::kF16, "ceil"},
                    BuiltinData{core::Function::kClamp, CallParamType::kF32, "clamp"},
                    BuiltinData{core::Function::kClamp, CallParamType::kF16, "clamp"},
                    BuiltinData{core::Function::kCos, CallParamType::kF32, "cos"},
                    BuiltinData{core::Function::kCos, CallParamType::kF16, "cos"},
                    BuiltinData{core::Function::kCosh, CallParamType::kF32, "cosh"},
                    BuiltinData{core::Function::kCosh, CallParamType::kF16, "cosh"},
                    BuiltinData{core::Function::kCross, CallParamType::kF32, "cross"},
                    BuiltinData{core::Function::kCross, CallParamType::kF16, "cross"},
                    BuiltinData{core::Function::kDistance, CallParamType::kF32, "distance"},
                    BuiltinData{core::Function::kDistance, CallParamType::kF16, "distance"},
                    BuiltinData{core::Function::kExp, CallParamType::kF32, "exp"},
                    BuiltinData{core::Function::kExp, CallParamType::kF16, "exp"},
                    BuiltinData{core::Function::kExp2, CallParamType::kF32, "exp2"},
                    BuiltinData{core::Function::kExp2, CallParamType::kF16, "exp2"},
                    BuiltinData{core::Function::kFaceForward, CallParamType::kF32, "faceforward"},
                    BuiltinData{core::Function::kFaceForward, CallParamType::kF16, "faceforward"},
                    BuiltinData{core::Function::kFloor, CallParamType::kF32, "floor"},
                    BuiltinData{core::Function::kFloor, CallParamType::kF16, "floor"},
                    BuiltinData{core::Function::kFma, CallParamType::kF32, "mad"},
                    BuiltinData{core::Function::kFma, CallParamType::kF16, "mad"},
                    BuiltinData{core::Function::kFract, CallParamType::kF32, "frac"},
                    BuiltinData{core::Function::kFract, CallParamType::kF16, "frac"},
                    BuiltinData{core::Function::kInverseSqrt, CallParamType::kF32, "rsqrt"},
                    BuiltinData{core::Function::kInverseSqrt, CallParamType::kF16, "rsqrt"},
                    BuiltinData{core::Function::kLdexp, CallParamType::kF32, "ldexp"},
                    BuiltinData{core::Function::kLdexp, CallParamType::kF16, "ldexp"},
                    BuiltinData{core::Function::kLength, CallParamType::kF32, "length"},
                    BuiltinData{core::Function::kLength, CallParamType::kF16, "length"},
                    BuiltinData{core::Function::kLog, CallParamType::kF32, "log"},
                    BuiltinData{core::Function::kLog, CallParamType::kF16, "log"},
                    BuiltinData{core::Function::kLog2, CallParamType::kF32, "log2"},
                    BuiltinData{core::Function::kLog2, CallParamType::kF16, "log2"},
                    BuiltinData{core::Function::kMax, CallParamType::kF32, "max"},
                    BuiltinData{core::Function::kMax, CallParamType::kF16, "max"},
                    BuiltinData{core::Function::kMin, CallParamType::kF32, "min"},
                    BuiltinData{core::Function::kMin, CallParamType::kF16, "min"},
                    BuiltinData{core::Function::kMix, CallParamType::kF32, "lerp"},
                    BuiltinData{core::Function::kMix, CallParamType::kF16, "lerp"},
                    BuiltinData{core::Function::kNormalize, CallParamType::kF32, "normalize"},
                    BuiltinData{core::Function::kNormalize, CallParamType::kF16, "normalize"},
                    BuiltinData{core::Function::kPow, CallParamType::kF32, "pow"},
                    BuiltinData{core::Function::kPow, CallParamType::kF16, "pow"},
                    BuiltinData{core::Function::kReflect, CallParamType::kF32, "reflect"},
                    BuiltinData{core::Function::kReflect, CallParamType::kF16, "reflect"},
                    BuiltinData{core::Function::kSin, CallParamType::kF32, "sin"},
                    BuiltinData{core::Function::kSin, CallParamType::kF16, "sin"},
                    BuiltinData{core::Function::kSinh, CallParamType::kF32, "sinh"},
                    BuiltinData{core::Function::kSinh, CallParamType::kF16, "sinh"},
                    BuiltinData{core::Function::kSmoothstep, CallParamType::kF32, "smoothstep"},
                    BuiltinData{core::Function::kSmoothstep, CallParamType::kF16, "smoothstep"},
                    BuiltinData{core::Function::kSqrt, CallParamType::kF32, "sqrt"},
                    BuiltinData{core::Function::kSqrt, CallParamType::kF16, "sqrt"},
                    BuiltinData{core::Function::kStep, CallParamType::kF32, "step"},
                    BuiltinData{core::Function::kStep, CallParamType::kF16, "step"},
                    BuiltinData{core::Function::kTan, CallParamType::kF32, "tan"},
                    BuiltinData{core::Function::kTan, CallParamType::kF16, "tan"},
                    BuiltinData{core::Function::kTanh, CallParamType::kF32, "tanh"},
                    BuiltinData{core::Function::kTanh, CallParamType::kF16, "tanh"},
                    /* Integer built-in */
                    BuiltinData{core::Function::kAbs, CallParamType::kU32, "abs"},
                    BuiltinData{core::Function::kClamp, CallParamType::kU32, "clamp"},
                    BuiltinData{core::Function::kCountOneBits, CallParamType::kU32, "countbits"},
                    BuiltinData{core::Function::kMax, CallParamType::kU32, "max"},
                    BuiltinData{core::Function::kMin, CallParamType::kU32, "min"},
                    BuiltinData{core::Function::kReverseBits, CallParamType::kU32, "reversebits"},
                    BuiltinData{core::Function::kRound, CallParamType::kU32, "round"},
                    /* Matrix built-in */
                    BuiltinData{core::Function::kDeterminant, CallParamType::kF32, "determinant"},
                    BuiltinData{core::Function::kDeterminant, CallParamType::kF16, "determinant"},
                    BuiltinData{core::Function::kTranspose, CallParamType::kF32, "transpose"},
                    BuiltinData{core::Function::kTranspose, CallParamType::kF16, "transpose"},
                    /* Vector built-in */
                    BuiltinData{core::Function::kDot, CallParamType::kF32, "dot"},
                    BuiltinData{core::Function::kDot, CallParamType::kF16, "dot"},
                    /* Derivate built-in */
                    BuiltinData{core::Function::kDpdx, CallParamType::kF32, "ddx"},
                    BuiltinData{core::Function::kDpdxCoarse, CallParamType::kF32, "ddx_coarse"},
                    BuiltinData{core::Function::kDpdxFine, CallParamType::kF32, "ddx_fine"},
                    BuiltinData{core::Function::kDpdy, CallParamType::kF32, "ddy"},
                    BuiltinData{core::Function::kDpdyCoarse, CallParamType::kF32, "ddy_coarse"},
                    BuiltinData{core::Function::kDpdyFine, CallParamType::kF32, "ddy_fine"},
                    BuiltinData{core::Function::kFwidth, CallParamType::kF32, "fwidth"},
                    BuiltinData{core::Function::kFwidthCoarse, CallParamType::kF32, "fwidth"},
                    BuiltinData{core::Function::kFwidthFine, CallParamType::kF32, "fwidth"}));

TEST_F(HlslASTPrinterTest_Builtin, Builtin_Call) {
    auto* call = Call("dot", "param1", "param2");

    GlobalVar("param1", ty.vec3<f32>(), core::AddressSpace::kPrivate);
    GlobalVar("param2", ty.vec3<f32>(), core::AddressSpace::kPrivate);

    WrapInFunction(Decl(Var("r", call)));

    ASTPrinter& gen = Build();

    gen.IncrementIndent();
    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "dot(param1, param2)");
}

TEST_F(HlslASTPrinterTest_Builtin, Select_Scalar) {
    GlobalVar("a", Expr(1_f), core::AddressSpace::kPrivate);
    GlobalVar("b", Expr(2_f), core::AddressSpace::kPrivate);
    auto* call = Call("select", "a", "b", true);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    gen.IncrementIndent();
    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "(true ? b : a)");
}

TEST_F(HlslASTPrinterTest_Builtin, Select_Vector) {
    GlobalVar("a", Call<vec2<i32>>(1_i, 2_i), core::AddressSpace::kPrivate);
    GlobalVar("b", Call<vec2<i32>>(3_i, 4_i), core::AddressSpace::kPrivate);
    auto* call = Call("select", "a", "b", Call<vec2<bool>>(true, false));
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    gen.IncrementIndent();
    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, call)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "(bool2(true, false) ? b : a)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Modf_Scalar_f32) {
    WrapInFunction(Decl(Let("f", Expr(1.5_f))),  //
                   Decl(Let("v", Call("modf", "f"))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_f32 {
  float fract;
  float whole;
};
modf_result_f32 tint_modf(float param_0) {
  modf_result_f32 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  const float f = 1.5f;
  const modf_result_f32 v = tint_modf(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Modf_Scalar_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Decl(Let("f", Expr(1.5_h))),  //
                   Decl(Let("v", Call("modf", "f"))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_f16 {
  float16_t fract;
  float16_t whole;
};
modf_result_f16 tint_modf(float16_t param_0) {
  modf_result_f16 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  const float16_t f = float16_t(1.5h);
  const modf_result_f16 v = tint_modf(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Modf_Vector_f32) {
    WrapInFunction(Decl(Let("f", Call<vec3<f32>>(1.5_f, 2.5_f, 3.5_f))),  //
                   Decl(Let("v", Call("modf", "f"))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_vec3_f32 {
  float3 fract;
  float3 whole;
};
modf_result_vec3_f32 tint_modf(float3 param_0) {
  modf_result_vec3_f32 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  const float3 f = float3(1.5f, 2.5f, 3.5f);
  const modf_result_vec3_f32 v = tint_modf(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Modf_Vector_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Decl(Let("f", Call<vec3<f16>>(1.5_h, 2.5_h, 3.5_h))),  //
                   Decl(Let("v", Call("modf", "f"))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_vec3_f16 {
  vector<float16_t, 3> fract;
  vector<float16_t, 3> whole;
};
modf_result_vec3_f16 tint_modf(vector<float16_t, 3> param_0) {
  modf_result_vec3_f16 result;
  result.fract = modf(param_0, result.whole);
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  const vector<float16_t, 3> f = vector<float16_t, 3>(float16_t(1.5h), float16_t(2.5h), float16_t(3.5h));
  const modf_result_vec3_f16 v = tint_modf(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Modf_Scalar_f32) {
    WrapInFunction(Decl(Let("v", Call("modf", 1.5_f))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_f32 {
  float fract;
  float whole;
};
[numthreads(1, 1, 1)]
void test_function() {
  const modf_result_f32 v = {0.5f, 1.0f};
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Modf_Scalar_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("modf", 1.5_h))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_f16 {
  float16_t fract;
  float16_t whole;
};
[numthreads(1, 1, 1)]
void test_function() {
  const modf_result_f16 v = {float16_t(0.5h), float16_t(1.0h)};
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Modf_Vector_f32) {
    WrapInFunction(Decl(Let("v", Call("modf", Call<vec3<f32>>(1.5_f, 2.5_f, 3.5_f)))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_vec3_f32 {
  float3 fract;
  float3 whole;
};
[numthreads(1, 1, 1)]
void test_function() {
  const modf_result_vec3_f32 v = {(0.5f).xxx, float3(1.0f, 2.0f, 3.0f)};
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Modf_Vector_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("modf", Call<vec3<f16>>(1.5_h, 2.5_h, 3.5_h)))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_vec3_f16 {
  vector<float16_t, 3> fract;
  vector<float16_t, 3> whole;
};
[numthreads(1, 1, 1)]
void test_function() {
  const modf_result_vec3_f16 v = {(float16_t(0.5h)).xxx, vector<float16_t, 3>(float16_t(1.0h), float16_t(2.0h), float16_t(3.0h))};
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, NonInitializer_Modf_Vector_f32) {
    WrapInFunction(
        // Declare a variable with the result of a modf call.
        // This is required to infer the 'var' type.
        Decl(Var("v", Call("modf", Call<vec3<f32>>(1.5_f, 2.5_f, 3.5_f)))),
        // Now assign 'v' again with another modf call.
        // This requires generating a temporary variable for the struct initializer.
        Assign("v", Call("modf", Call<vec3<f32>>(4.5_a, 5.5_a, 6.5_a))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct modf_result_vec3_f32 {
  float3 fract;
  float3 whole;
};
[numthreads(1, 1, 1)]
void test_function() {
  modf_result_vec3_f32 v = {(0.5f).xxx, float3(1.0f, 2.0f, 3.0f)};
  const modf_result_vec3_f32 c = {(0.5f).xxx, float3(4.0f, 5.0f, 6.0f)};
  v = c;
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Frexp_Scalar_f32) {
    WrapInFunction(Var("f", Expr(1_f)),  //
                   Var("v", Call("frexp", "f")));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_f32 {
  float fract;
  int exp;
};
frexp_result_f32 tint_frexp(float param_0) {
  float exp;
  float fract = sign(param_0) * frexp(param_0, exp);
  frexp_result_f32 result = {fract, int(exp)};
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  float f = 1.0f;
  frexp_result_f32 v = tint_frexp(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Frexp_Scalar_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Var("f", Expr(1_h)),  //
                   Var("v", Call("frexp", "f")));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_f16 {
  float16_t fract;
  int exp;
};
frexp_result_f16 tint_frexp(float16_t param_0) {
  float16_t exp;
  float16_t fract = sign(param_0) * frexp(param_0, exp);
  frexp_result_f16 result = {fract, int(exp)};
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  float16_t f = float16_t(1.0h);
  frexp_result_f16 v = tint_frexp(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Frexp_Vector_f32) {
    WrapInFunction(Var("f", Call<vec3<f32>>()),  //
                   Var("v", Call("frexp", "f")));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_vec3_f32 {
  float3 fract;
  int3 exp;
};
frexp_result_vec3_f32 tint_frexp(float3 param_0) {
  float3 exp;
  float3 fract = sign(param_0) * frexp(param_0, exp);
  frexp_result_vec3_f32 result = {fract, int3(exp)};
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  float3 f = (0.0f).xxx;
  frexp_result_vec3_f32 v = tint_frexp(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Runtime_Frexp_Vector_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Var("f", Call<vec3<f16>>()),  //
                   Var("v", Call("frexp", "f")));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_vec3_f16 {
  vector<float16_t, 3> fract;
  int3 exp;
};
frexp_result_vec3_f16 tint_frexp(vector<float16_t, 3> param_0) {
  vector<float16_t, 3> exp;
  vector<float16_t, 3> fract = sign(param_0) * frexp(param_0, exp);
  frexp_result_vec3_f16 result = {fract, int3(exp)};
  return result;
}

[numthreads(1, 1, 1)]
void test_function() {
  vector<float16_t, 3> f = (float16_t(0.0h)).xxx;
  frexp_result_vec3_f16 v = tint_frexp(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Frexp_Scalar_f32) {
    WrapInFunction(Decl(Let("v", Call("frexp", 1_f))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_f32 {
  float fract;
  int exp;
};
[numthreads(1, 1, 1)]
void test_function() {
  const frexp_result_f32 v = {0.5f, 1};
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Frexp_Scalar_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("frexp", 1_h))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_f16 {
  float16_t fract;
  int exp;
};
[numthreads(1, 1, 1)]
void test_function() {
  const frexp_result_f16 v = {float16_t(0.5h), 1};
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Frexp_Vector_f32) {
    WrapInFunction(Decl(Let("v", Call("frexp", Call<vec3<f32>>()))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_vec3_f32 {
  float3 fract;
  int3 exp;
};
[numthreads(1, 1, 1)]
void test_function() {
  const frexp_result_vec3_f32 v = (frexp_result_vec3_f32)0;
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Const_Frexp_Vector_f16) {
    Enable(core::Extension::kF16);

    WrapInFunction(Decl(Let("v", Call("frexp", Call<vec3<f16>>()))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_vec3_f16 {
  vector<float16_t, 3> fract;
  int3 exp;
};
[numthreads(1, 1, 1)]
void test_function() {
  const frexp_result_vec3_f16 v = (frexp_result_vec3_f16)0;
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, NonInitializer_Frexp_Vector_f32) {
    WrapInFunction(
        // Declare a variable with the result of a frexp call.
        // This is required to infer the 'var' type.
        Decl(Var("v", Call("frexp", Call<vec3<f32>>(1.5_f, 2.5_f, 3.5_f)))),
        // Now assign 'v' again with another frexp call.
        // This requires generating a temporary variable for the struct initializer.
        Assign("v", Call("frexp", Call<vec3<f32>>(4.5_a, 5.5_a, 6.5_a))));

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(struct frexp_result_vec3_f32 {
  float3 fract;
  int3 exp;
};
[numthreads(1, 1, 1)]
void test_function() {
  frexp_result_vec3_f32 v = {float3(0.75f, 0.625f, 0.875f), int3(1, 2, 2)};
  const frexp_result_vec3_f32 c = {float3(0.5625f, 0.6875f, 0.8125f), (3).xxx};
  v = c;
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Degrees_Scalar_f32) {
    auto* val = Var("val", ty.f32());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float tint_degrees(float param_0) {
  return param_0 * 57.29577951308232286465;
}

[numthreads(1, 1, 1)]
void test_function() {
  float val = 0.0f;
  const float tint_symbol = tint_degrees(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Degrees_Vector_f32) {
    auto* val = Var("val", ty.vec3<f32>());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float3 tint_degrees(float3 param_0) {
  return param_0 * 57.29577951308232286465;
}

[numthreads(1, 1, 1)]
void test_function() {
  float3 val = float3(0.0f, 0.0f, 0.0f);
  const float3 tint_symbol = tint_degrees(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Degrees_Scalar_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.f16());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float16_t tint_degrees(float16_t param_0) {
  return param_0 * 57.29577951308232286465;
}

[numthreads(1, 1, 1)]
void test_function() {
  float16_t val = float16_t(0.0h);
  const float16_t tint_symbol = tint_degrees(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Degrees_Vector_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.vec3<f16>());
    auto* call = Call("degrees", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(vector<float16_t, 3> tint_degrees(vector<float16_t, 3> param_0) {
  return param_0 * 57.29577951308232286465;
}

[numthreads(1, 1, 1)]
void test_function() {
  vector<float16_t, 3> val = vector<float16_t, 3>(float16_t(0.0h), float16_t(0.0h), float16_t(0.0h));
  const vector<float16_t, 3> tint_symbol = tint_degrees(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Radians_Scalar_f32) {
    auto* val = Var("val", ty.f32());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float tint_radians(float param_0) {
  return param_0 * 0.01745329251994329547;
}

[numthreads(1, 1, 1)]
void test_function() {
  float val = 0.0f;
  const float tint_symbol = tint_radians(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Radians_Vector_f32) {
    auto* val = Var("val", ty.vec3<f32>());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float3 tint_radians(float3 param_0) {
  return param_0 * 0.01745329251994329547;
}

[numthreads(1, 1, 1)]
void test_function() {
  float3 val = float3(0.0f, 0.0f, 0.0f);
  const float3 tint_symbol = tint_radians(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Radians_Scalar_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.f16());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float16_t tint_radians(float16_t param_0) {
  return param_0 * 0.01745329251994329547;
}

[numthreads(1, 1, 1)]
void test_function() {
  float16_t val = float16_t(0.0h);
  const float16_t tint_symbol = tint_radians(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Radians_Vector_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.vec3<f16>());
    auto* call = Call("radians", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(vector<float16_t, 3> tint_radians(vector<float16_t, 3> param_0) {
  return param_0 * 0.01745329251994329547;
}

[numthreads(1, 1, 1)]
void test_function() {
  vector<float16_t, 3> val = vector<float16_t, 3>(float16_t(0.0h), float16_t(0.0h), float16_t(0.0h));
  const vector<float16_t, 3> tint_symbol = tint_radians(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Sign_Scalar_i32) {
    auto* val = Var("val", ty.i32());
    auto* call = Call("sign", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  int val = 0;
  const int tint_symbol = int(sign(val));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Sign_Vector_i32) {
    auto* val = Var("val", ty.vec3<i32>());
    auto* call = Call("sign", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  int3 val = int3(0, 0, 0);
  const int3 tint_symbol = int3(sign(val));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Sign_Scalar_f32) {
    auto* val = Var("val", ty.f32());
    auto* call = Call("sign", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  float val = 0.0f;
  const float tint_symbol = float(sign(val));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Sign_Vector_f32) {
    auto* val = Var("val", ty.vec3<f32>());
    auto* call = Call("sign", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  float3 val = float3(0.0f, 0.0f, 0.0f);
  const float3 tint_symbol = float3(sign(val));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Sign_Scalar_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.f16());
    auto* call = Call("sign", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  float16_t val = float16_t(0.0h);
  const float16_t tint_symbol = float16_t(sign(val));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Sign_Vector_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.vec3<f16>());
    auto* call = Call("sign", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  vector<float16_t, 3> val = vector<float16_t, 3>(float16_t(0.0h), float16_t(0.0h), float16_t(0.0h));
  const vector<float16_t, 3> tint_symbol = vector<float16_t, 3>(sign(val));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Trunc_Scalar_f32) {
    auto* val = Var("val", ty.f32());
    auto* call = Call("trunc", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float tint_trunc(float param_0) {
  return param_0 < 0 ? ceil(param_0) : floor(param_0);
}

[numthreads(1, 1, 1)]
void test_function() {
  float val = 0.0f;
  const float tint_symbol = tint_trunc(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Trunc_Vector_f32) {
    auto* val = Var("val", ty.vec3<f32>());
    auto* call = Call("trunc", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float3 tint_trunc(float3 param_0) {
  return param_0 < 0 ? ceil(param_0) : floor(param_0);
}

[numthreads(1, 1, 1)]
void test_function() {
  float3 val = float3(0.0f, 0.0f, 0.0f);
  const float3 tint_symbol = tint_trunc(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Trunc_Scalar_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.f16());
    auto* call = Call("trunc", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float16_t tint_trunc(float16_t param_0) {
  return param_0 < 0 ? ceil(param_0) : floor(param_0);
}

[numthreads(1, 1, 1)]
void test_function() {
  float16_t val = float16_t(0.0h);
  const float16_t tint_symbol = tint_trunc(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Trunc_Vector_f16) {
    Enable(core::Extension::kF16);

    auto* val = Var("val", ty.vec3<f16>());
    auto* call = Call("trunc", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(vector<float16_t, 3> tint_trunc(vector<float16_t, 3> param_0) {
  return param_0 < 0 ? ceil(param_0) : floor(param_0);
}

[numthreads(1, 1, 1)]
void test_function() {
  vector<float16_t, 3> val = vector<float16_t, 3>(float16_t(0.0h), float16_t(0.0h), float16_t(0.0h));
  const vector<float16_t, 3> tint_symbol = tint_trunc(val);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Pack4x8Snorm) {
    auto* call = Call("pack4x8snorm", "p1");
    GlobalVar("p1", ty.vec4<f32>(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(uint tint_pack4x8snorm(float4 param_0) {
  int4 i = int4(round(clamp(param_0, -1.0, 1.0) * 127.0)) & 0xff;
  return asuint(i.x | i.y << 8 | i.z << 16 | i.w << 24);
}

static float4 p1 = float4(0.0f, 0.0f, 0.0f, 0.0f);

[numthreads(1, 1, 1)]
void test_function() {
  uint r = tint_pack4x8snorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Pack4x8Unorm) {
    auto* call = Call("pack4x8unorm", "p1");
    GlobalVar("p1", ty.vec4<f32>(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(uint tint_pack4x8unorm(float4 param_0) {
  uint4 i = uint4(round(clamp(param_0, 0.0, 1.0) * 255.0));
  return (i.x | i.y << 8 | i.z << 16 | i.w << 24);
}

static float4 p1 = float4(0.0f, 0.0f, 0.0f, 0.0f);

[numthreads(1, 1, 1)]
void test_function() {
  uint r = tint_pack4x8unorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Pack2x16Snorm) {
    auto* call = Call("pack2x16snorm", "p1");
    GlobalVar("p1", ty.vec2<f32>(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(uint tint_pack2x16snorm(float2 param_0) {
  int2 i = int2(round(clamp(param_0, -1.0, 1.0) * 32767.0)) & 0xffff;
  return asuint(i.x | i.y << 16);
}

static float2 p1 = float2(0.0f, 0.0f);

[numthreads(1, 1, 1)]
void test_function() {
  uint r = tint_pack2x16snorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Pack2x16Unorm) {
    auto* call = Call("pack2x16unorm", "p1");
    GlobalVar("p1", ty.vec2<f32>(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(uint tint_pack2x16unorm(float2 param_0) {
  uint2 i = uint2(round(clamp(param_0, 0.0, 1.0) * 65535.0));
  return (i.x | i.y << 16);
}

static float2 p1 = float2(0.0f, 0.0f);

[numthreads(1, 1, 1)]
void test_function() {
  uint r = tint_pack2x16unorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Pack2x16Float) {
    auto* call = Call("pack2x16float", "p1");
    GlobalVar("p1", ty.vec2<f32>(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(uint tint_pack2x16float(float2 param_0) {
  uint2 i = f32tof16(param_0);
  return i.x | (i.y << 16);
}

static float2 p1 = float2(0.0f, 0.0f);

[numthreads(1, 1, 1)]
void test_function() {
  uint r = tint_pack2x16float(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Unpack4x8Snorm) {
    auto* call = Call("unpack4x8snorm", "p1");
    GlobalVar("p1", ty.u32(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float4 tint_unpack4x8snorm(uint param_0) {
  int j = int(param_0);
  int4 i = int4(j << 24, j << 16, j << 8, j) >> 24;
  return clamp(float4(i) / 127.0, -1.0, 1.0);
}

static uint p1 = 0u;

[numthreads(1, 1, 1)]
void test_function() {
  float4 r = tint_unpack4x8snorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Unpack4x8Unorm) {
    auto* call = Call("unpack4x8unorm", "p1");
    GlobalVar("p1", ty.u32(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float4 tint_unpack4x8unorm(uint param_0) {
  uint j = param_0;
  uint4 i = uint4(j & 0xff, (j >> 8) & 0xff, (j >> 16) & 0xff, j >> 24);
  return float4(i) / 255.0;
}

static uint p1 = 0u;

[numthreads(1, 1, 1)]
void test_function() {
  float4 r = tint_unpack4x8unorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Unpack2x16Snorm) {
    auto* call = Call("unpack2x16snorm", "p1");
    GlobalVar("p1", ty.u32(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float2 tint_unpack2x16snorm(uint param_0) {
  int j = int(param_0);
  int2 i = int2(j << 16, j) >> 16;
  return clamp(float2(i) / 32767.0, -1.0, 1.0);
}

static uint p1 = 0u;

[numthreads(1, 1, 1)]
void test_function() {
  float2 r = tint_unpack2x16snorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Unpack2x16Unorm) {
    auto* call = Call("unpack2x16unorm", "p1");
    GlobalVar("p1", ty.u32(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float2 tint_unpack2x16unorm(uint param_0) {
  uint j = param_0;
  uint2 i = uint2(j & 0xffff, j >> 16);
  return float2(i) / 65535.0;
}

static uint p1 = 0u;

[numthreads(1, 1, 1)]
void test_function() {
  float2 r = tint_unpack2x16unorm(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Unpack2x16Float) {
    auto* call = Call("unpack2x16float", "p1");
    GlobalVar("p1", ty.u32(), core::AddressSpace::kPrivate);
    WrapInFunction(Decl(Var("r", call)));
    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(float2 tint_unpack2x16float(uint param_0) {
  uint i = param_0;
  return f16tof32(uint2(i & 0xffff, i >> 16));
}

static uint p1 = 0u;

[numthreads(1, 1, 1)]
void test_function() {
  float2 r = tint_unpack2x16float(p1);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, StorageBarrier) {
    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("storageBarrier")),
         },
         Vector{
             Stage(ast::PipelineStage::kCompute),
             WorkgroupSize(1_i),
         });

    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void main() {
  DeviceMemoryBarrierWithGroupSync();
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, WorkgroupBarrier) {
    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("workgroupBarrier")),
         },
         Vector{
             Stage(ast::PipelineStage::kCompute),
             WorkgroupSize(1_i),
         });

    ASTPrinter& gen = Build();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void main() {
  GroupMemoryBarrierWithGroupSync();
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Dot4I8Packed) {
    Enable(core::Extension::kChromiumExperimentalDp4A);

    auto* val1 = Var("val1", ty.u32());
    auto* val2 = Var("val2", ty.u32());
    auto* call = Call("dot4I8Packed", val1, val2);
    WrapInFunction(val1, val2, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(int tint_dot4I8Packed(uint param_0, uint param_1) {
  int accumulator = 0;
  return dot4add_i8packed(param_0, param_1, accumulator);
}

[numthreads(1, 1, 1)]
void test_function() {
  uint val1 = 0u;
  uint val2 = 0u;
  const int tint_symbol = tint_dot4I8Packed(val1, val2);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, Dot4U8Packed) {
    Enable(core::Extension::kChromiumExperimentalDp4A);

    auto* val1 = Var("val1", ty.u32());
    auto* val2 = Var("val2", ty.u32());
    auto* call = Call("dot4U8Packed", val1, val2);
    WrapInFunction(val1, val2, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(uint tint_dot4U8Packed(uint param_0, uint param_1) {
  uint accumulator = 0u;
  return dot4add_u8packed(param_0, param_1, accumulator);
}

[numthreads(1, 1, 1)]
void test_function() {
  uint val1 = 0u;
  uint val2 = 0u;
  const uint tint_symbol = tint_dot4U8Packed(val1, val2);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, CountOneBits) {
    auto* val = Var("val1", ty.i32());
    auto* call = Call("countOneBits", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  int val1 = 0;
  const int tint_symbol = asint(countbits(asuint(val1)));
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Builtin, ReverseBits) {
    auto* val = Var("val1", ty.i32());
    auto* call = Call("reverseBits", val);
    WrapInFunction(val, call);

    ASTPrinter& gen = SanitizeAndBuild();

    ASSERT_TRUE(gen.Generate()) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"([numthreads(1, 1, 1)]
void test_function() {
  int val1 = 0;
  const int tint_symbol = asint(reversebits(asuint(val1)));
  return;
}
)");
}

}  // namespace
}  // namespace tint::hlsl::writer
