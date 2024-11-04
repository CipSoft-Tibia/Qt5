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

// GEN_BUILD:CONDITION(tint_build_ir)

#include "src/tint/lang/spirv/writer/common/helper_test.h"

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/binary.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::spirv::writer {
namespace {

/// A parameterized test case.
struct BinaryTestCase {
    /// The element type to test.
    TestElementType type;
    /// The binary operation.
    enum core::ir::Binary::Kind kind;
    /// The expected SPIR-V instruction.
    std::string spirv_inst;
    /// The expected SPIR-V result type name.
    std::string spirv_type_name;
};

using Arithmetic_Bitwise = SpirvWriterTestWithParam<BinaryTestCase>;
TEST_P(Arithmetic_Bitwise, Scalar) {
    auto params = GetParam();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* lhs = MakeScalarValue(params.type);
        auto* rhs = MakeScalarValue(params.type);
        auto* result = b.Binary(params.kind, MakeScalarType(params.type), lhs, rhs);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = " + params.spirv_inst + " %" + params.spirv_type_name);
}
TEST_P(Arithmetic_Bitwise, Vector) {
    auto params = GetParam();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* lhs = MakeVectorValue(params.type);
        auto* rhs = MakeVectorValue(params.type);
        auto* result = b.Binary(params.kind, MakeVectorType(params.type), lhs, rhs);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = " + params.spirv_inst + " %v2" + params.spirv_type_name);
}
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_I32,
    Arithmetic_Bitwise,
    testing::Values(BinaryTestCase{kI32, core::ir::Binary::Kind::kAdd, "OpIAdd", "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kSubtract, "OpISub", "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kMultiply, "OpIMul", "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kAnd, "OpBitwiseAnd", "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kOr, "OpBitwiseOr", "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kXor, "OpBitwiseXor", "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kShiftLeft, "OpShiftLeftLogical",
                                   "int"},
                    BinaryTestCase{kI32, core::ir::Binary::Kind::kShiftRight,
                                   "OpShiftRightArithmetic", "int"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_U32,
    Arithmetic_Bitwise,
    testing::Values(
        BinaryTestCase{kU32, core::ir::Binary::Kind::kAdd, "OpIAdd", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kSubtract, "OpISub", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kMultiply, "OpIMul", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kAnd, "OpBitwiseAnd", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kOr, "OpBitwiseOr", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kXor, "OpBitwiseXor", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kShiftLeft, "OpShiftLeftLogical", "uint"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kShiftRight, "OpShiftRightLogical", "uint"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_F32,
    Arithmetic_Bitwise,
    testing::Values(BinaryTestCase{kF32, core::ir::Binary::Kind::kAdd, "OpFAdd", "float"},
                    BinaryTestCase{kF32, core::ir::Binary::Kind::kSubtract, "OpFSub", "float"},
                    BinaryTestCase{kF32, core::ir::Binary::Kind::kMultiply, "OpFMul", "float"},
                    BinaryTestCase{kF32, core::ir::Binary::Kind::kDivide, "OpFDiv", "float"},
                    BinaryTestCase{kF32, core::ir::Binary::Kind::kModulo, "OpFRem", "float"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_F16,
    Arithmetic_Bitwise,
    testing::Values(BinaryTestCase{kF16, core::ir::Binary::Kind::kAdd, "OpFAdd", "half"},
                    BinaryTestCase{kF16, core::ir::Binary::Kind::kSubtract, "OpFSub", "half"},
                    BinaryTestCase{kF16, core::ir::Binary::Kind::kMultiply, "OpFMul", "half"},
                    BinaryTestCase{kF16, core::ir::Binary::Kind::kDivide, "OpFDiv", "half"},
                    BinaryTestCase{kF16, core::ir::Binary::Kind::kModulo, "OpFRem", "half"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_Bool,
    Arithmetic_Bitwise,
    testing::Values(BinaryTestCase{kBool, core::ir::Binary::Kind::kAnd, "OpLogicalAnd", "bool"},
                    BinaryTestCase{kBool, core::ir::Binary::Kind::kOr, "OpLogicalOr", "bool"}));

TEST_F(SpirvWriterTest, Binary_ScalarTimesVector_F32) {
    auto* scalar = b.FunctionParam("scalar", ty.f32());
    auto* vector = b.FunctionParam("vector", ty.vec4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({scalar, vector});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.vec4<f32>(), scalar, vector);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpVectorTimesScalar %v4float %vector %scalar");
}

TEST_F(SpirvWriterTest, Binary_VectorTimesScalar_F32) {
    auto* scalar = b.FunctionParam("scalar", ty.f32());
    auto* vector = b.FunctionParam("vector", ty.vec4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({scalar, vector});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.vec4<f32>(), vector, scalar);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpVectorTimesScalar %v4float %vector %scalar");
}

TEST_F(SpirvWriterTest, Binary_ScalarTimesMatrix_F32) {
    auto* scalar = b.FunctionParam("scalar", ty.f32());
    auto* matrix = b.FunctionParam("matrix", ty.mat3x4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({scalar, matrix});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.mat3x4<f32>(), scalar, matrix);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpMatrixTimesScalar %mat3v4float %matrix %scalar");
}

TEST_F(SpirvWriterTest, Binary_MatrixTimesScalar_F32) {
    auto* scalar = b.FunctionParam("scalar", ty.f32());
    auto* matrix = b.FunctionParam("matrix", ty.mat3x4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({scalar, matrix});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.mat3x4<f32>(), matrix, scalar);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpMatrixTimesScalar %mat3v4float %matrix %scalar");
}

TEST_F(SpirvWriterTest, Binary_VectorTimesMatrix_F32) {
    auto* vector = b.FunctionParam("vector", ty.vec4<f32>());
    auto* matrix = b.FunctionParam("matrix", ty.mat3x4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({vector, matrix});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.vec3<f32>(), vector, matrix);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpVectorTimesMatrix %v3float %vector %matrix");
}

TEST_F(SpirvWriterTest, Binary_MatrixTimesVector_F32) {
    auto* vector = b.FunctionParam("vector", ty.vec3<f32>());
    auto* matrix = b.FunctionParam("matrix", ty.mat3x4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({vector, matrix});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.vec4<f32>(), matrix, vector);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpMatrixTimesVector %v4float %matrix %vector");
}

TEST_F(SpirvWriterTest, Binary_MatrixTimesMatrix_F32) {
    auto* mat1 = b.FunctionParam("mat1", ty.mat4x3<f32>());
    auto* mat2 = b.FunctionParam("mat2", ty.mat3x4<f32>());
    auto* func = b.Function("foo", ty.void_());
    func->SetParams({mat1, mat2});
    b.Append(func->Block(), [&] {
        auto* result = b.Multiply(ty.mat3x3<f32>(), mat1, mat2);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = OpMatrixTimesMatrix %mat3v3float %mat1 %mat2");
}

using Comparison = SpirvWriterTestWithParam<BinaryTestCase>;
TEST_P(Comparison, Scalar) {
    auto params = GetParam();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* lhs = MakeScalarValue(params.type);
        auto* rhs = MakeScalarValue(params.type);
        auto* result = b.Binary(params.kind, ty.bool_(), lhs, rhs);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = " + params.spirv_inst + " %bool");
}

TEST_P(Comparison, Vector) {
    auto params = GetParam();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* lhs = MakeVectorValue(params.type);
        auto* rhs = MakeVectorValue(params.type);
        auto* result = b.Binary(params.kind, ty.vec2<bool>(), lhs, rhs);
        b.Return(func);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%result = " + params.spirv_inst + " %v2bool");
}
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_I32,
    Comparison,
    testing::Values(
        BinaryTestCase{kI32, core::ir::Binary::Kind::kEqual, "OpIEqual", "bool"},
        BinaryTestCase{kI32, core::ir::Binary::Kind::kNotEqual, "OpINotEqual", "bool"},
        BinaryTestCase{kI32, core::ir::Binary::Kind::kGreaterThan, "OpSGreaterThan", "bool"},
        BinaryTestCase{kI32, core::ir::Binary::Kind::kGreaterThanEqual, "OpSGreaterThanEqual",
                       "bool"},
        BinaryTestCase{kI32, core::ir::Binary::Kind::kLessThan, "OpSLessThan", "bool"},
        BinaryTestCase{kI32, core::ir::Binary::Kind::kLessThanEqual, "OpSLessThanEqual", "bool"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_U32,
    Comparison,
    testing::Values(
        BinaryTestCase{kU32, core::ir::Binary::Kind::kEqual, "OpIEqual", "bool"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kNotEqual, "OpINotEqual", "bool"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kGreaterThan, "OpUGreaterThan", "bool"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kGreaterThanEqual, "OpUGreaterThanEqual",
                       "bool"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kLessThan, "OpULessThan", "bool"},
        BinaryTestCase{kU32, core::ir::Binary::Kind::kLessThanEqual, "OpULessThanEqual", "bool"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_F32,
    Comparison,
    testing::Values(
        BinaryTestCase{kF32, core::ir::Binary::Kind::kEqual, "OpFOrdEqual", "bool"},
        BinaryTestCase{kF32, core::ir::Binary::Kind::kNotEqual, "OpFOrdNotEqual", "bool"},
        BinaryTestCase{kF32, core::ir::Binary::Kind::kGreaterThan, "OpFOrdGreaterThan", "bool"},
        BinaryTestCase{kF32, core::ir::Binary::Kind::kGreaterThanEqual, "OpFOrdGreaterThanEqual",
                       "bool"},
        BinaryTestCase{kF32, core::ir::Binary::Kind::kLessThan, "OpFOrdLessThan", "bool"},
        BinaryTestCase{kF32, core::ir::Binary::Kind::kLessThanEqual, "OpFOrdLessThanEqual",
                       "bool"}));
INSTANTIATE_TEST_SUITE_P(
    SpirvWriterTest_Binary_F16,
    Comparison,
    testing::Values(
        BinaryTestCase{kF16, core::ir::Binary::Kind::kEqual, "OpFOrdEqual", "bool"},
        BinaryTestCase{kF16, core::ir::Binary::Kind::kNotEqual, "OpFOrdNotEqual", "bool"},
        BinaryTestCase{kF16, core::ir::Binary::Kind::kGreaterThan, "OpFOrdGreaterThan", "bool"},
        BinaryTestCase{kF16, core::ir::Binary::Kind::kGreaterThanEqual, "OpFOrdGreaterThanEqual",
                       "bool"},
        BinaryTestCase{kF16, core::ir::Binary::Kind::kLessThan, "OpFOrdLessThan", "bool"},
        BinaryTestCase{kF16, core::ir::Binary::Kind::kLessThanEqual, "OpFOrdLessThanEqual",
                       "bool"}));
INSTANTIATE_TEST_SUITE_P(SpirvWriterTest_Binary_Bool,
                         Comparison,
                         testing::Values(BinaryTestCase{kBool, core::ir::Binary::Kind::kEqual,
                                                        "OpLogicalEqual", "bool"},
                                         BinaryTestCase{kBool, core::ir::Binary::Kind::kNotEqual,
                                                        "OpLogicalNotEqual", "bool"}));

TEST_F(SpirvWriterTest, Binary_Chain) {
    auto* func = b.Function("foo", ty.void_());

    b.Append(func->Block(), [&] {
        auto* sub = b.Subtract(ty.i32(), 1_i, 2_i);
        auto* add = b.Add(ty.i32(), sub, sub);
        b.Return(func);
        mod.SetName(sub, "sub");
        mod.SetName(add, "add");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST("%sub = OpISub %int %int_1 %int_2");
    EXPECT_INST("%add = OpIAdd %int %sub %sub");
}

TEST_F(SpirvWriterTest, Divide_u32_u32) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.u32()));
    args.Push(b.FunctionParam("rhs", ty.u32()));
    auto* func = b.Function("foo", ty.u32());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kDivide, ty.u32(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %uint None %5
        %lhs = OpFunctionParameter %uint
        %rhs = OpFunctionParameter %uint
          %6 = OpLabel
     %result = OpFunctionCall %uint %tint_div_u32 %lhs %rhs
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_div_u32
%tint_div_u32 = OpFunction %uint None %5
      %lhs_0 = OpFunctionParameter %uint
      %rhs_0 = OpFunctionParameter %uint
         %11 = OpLabel
         %12 = OpIEqual %bool %rhs_0 %uint_0
         %15 = OpSelect %uint %12 %uint_1 %rhs_0
         %17 = OpUDiv %uint %lhs_0 %15
               OpReturnValue %17
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Divide_i32_i32) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.i32()));
    args.Push(b.FunctionParam("rhs", ty.i32()));
    auto* func = b.Function("foo", ty.i32());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kDivide, ty.i32(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %int None %5
        %lhs = OpFunctionParameter %int
        %rhs = OpFunctionParameter %int
          %6 = OpLabel
     %result = OpFunctionCall %int %tint_div_i32 %lhs %rhs
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_div_i32
%tint_div_i32 = OpFunction %int None %5
      %lhs_0 = OpFunctionParameter %int
      %rhs_0 = OpFunctionParameter %int
         %11 = OpLabel
         %12 = OpIEqual %bool %rhs_0 %int_0
         %15 = OpIEqual %bool %lhs_0 %int_n2147483648
         %17 = OpIEqual %bool %rhs_0 %int_n1
         %19 = OpLogicalAnd %bool %15 %17
         %20 = OpLogicalOr %bool %12 %19
         %21 = OpSelect %int %20 %int_1 %rhs_0
         %23 = OpSDiv %int %lhs_0 %21
               OpReturnValue %23
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Divide_i32_vec4i) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.i32()));
    args.Push(b.FunctionParam("rhs", ty.vec4<i32>()));
    auto* func = b.Function("foo", ty.vec4<i32>());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kDivide, ty.vec4<i32>(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %v4int None %6
        %lhs = OpFunctionParameter %int
        %rhs = OpFunctionParameter %v4int
          %7 = OpLabel
          %8 = OpCompositeConstruct %v4int %lhs %lhs %lhs %lhs
     %result = OpFunctionCall %v4int %tint_div_v4i32 %8 %rhs
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_div_v4i32
%tint_div_v4i32 = OpFunction %v4int None %13
      %lhs_0 = OpFunctionParameter %v4int
      %rhs_0 = OpFunctionParameter %v4int
         %14 = OpLabel
         %15 = OpIEqual %v4bool %rhs_0 %16
         %20 = OpIEqual %v4bool %lhs_0 %21
         %23 = OpIEqual %v4bool %rhs_0 %24
         %26 = OpLogicalAnd %v4bool %20 %23
         %27 = OpLogicalOr %v4bool %15 %26
         %28 = OpSelect %v4int %27 %29 %rhs_0
         %31 = OpSDiv %v4int %lhs_0 %28
               OpReturnValue %31
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Divide_vec4i_i32) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.vec4<i32>()));
    args.Push(b.FunctionParam("rhs", ty.i32()));
    auto* func = b.Function("foo", ty.vec4<i32>());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kDivide, ty.vec4<i32>(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %v4int None %6
        %lhs = OpFunctionParameter %v4int
        %rhs = OpFunctionParameter %int
          %7 = OpLabel
          %8 = OpCompositeConstruct %v4int %rhs %rhs %rhs %rhs
     %result = OpFunctionCall %v4int %tint_div_v4i32 %lhs %8
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_div_v4i32
%tint_div_v4i32 = OpFunction %v4int None %13
      %lhs_0 = OpFunctionParameter %v4int
      %rhs_0 = OpFunctionParameter %v4int
         %14 = OpLabel
         %15 = OpIEqual %v4bool %rhs_0 %16
         %20 = OpIEqual %v4bool %lhs_0 %21
         %23 = OpIEqual %v4bool %rhs_0 %24
         %26 = OpLogicalAnd %v4bool %20 %23
         %27 = OpLogicalOr %v4bool %15 %26
         %28 = OpSelect %v4int %27 %29 %rhs_0
         %31 = OpSDiv %v4int %lhs_0 %28
               OpReturnValue %31
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Modulo_u32_u32) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.u32()));
    args.Push(b.FunctionParam("rhs", ty.u32()));
    auto* func = b.Function("foo", ty.u32());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kModulo, ty.u32(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %uint None %5
        %lhs = OpFunctionParameter %uint
        %rhs = OpFunctionParameter %uint
          %6 = OpLabel
     %result = OpFunctionCall %uint %tint_mod_u32 %lhs %rhs
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_mod_u32
%tint_mod_u32 = OpFunction %uint None %5
      %lhs_0 = OpFunctionParameter %uint
      %rhs_0 = OpFunctionParameter %uint
         %11 = OpLabel
         %12 = OpIEqual %bool %rhs_0 %uint_0
         %15 = OpSelect %uint %12 %uint_1 %rhs_0
         %17 = OpUDiv %uint %lhs_0 %15
         %18 = OpIMul %uint %17 %15
         %19 = OpISub %uint %lhs_0 %18
               OpReturnValue %19
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Modulo_i32_i32) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.i32()));
    args.Push(b.FunctionParam("rhs", ty.i32()));
    auto* func = b.Function("foo", ty.i32());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kModulo, ty.i32(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %int None %5
        %lhs = OpFunctionParameter %int
        %rhs = OpFunctionParameter %int
          %6 = OpLabel
     %result = OpFunctionCall %int %tint_mod_i32 %lhs %rhs
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_mod_i32
%tint_mod_i32 = OpFunction %int None %5
      %lhs_0 = OpFunctionParameter %int
      %rhs_0 = OpFunctionParameter %int
         %11 = OpLabel
         %12 = OpIEqual %bool %rhs_0 %int_0
         %15 = OpIEqual %bool %lhs_0 %int_n2147483648
         %17 = OpIEqual %bool %rhs_0 %int_n1
         %19 = OpLogicalAnd %bool %15 %17
         %20 = OpLogicalOr %bool %12 %19
         %21 = OpSelect %int %20 %int_1 %rhs_0
         %23 = OpSDiv %int %lhs_0 %21
         %24 = OpIMul %int %23 %21
         %25 = OpISub %int %lhs_0 %24
               OpReturnValue %25
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Modulo_i32_vec4i) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.i32()));
    args.Push(b.FunctionParam("rhs", ty.vec4<i32>()));
    auto* func = b.Function("foo", ty.vec4<i32>());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kModulo, ty.vec4<i32>(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %v4int None %6
        %lhs = OpFunctionParameter %int
        %rhs = OpFunctionParameter %v4int
          %7 = OpLabel
          %8 = OpCompositeConstruct %v4int %lhs %lhs %lhs %lhs
     %result = OpFunctionCall %v4int %tint_mod_v4i32 %8 %rhs
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_mod_v4i32
%tint_mod_v4i32 = OpFunction %v4int None %13
      %lhs_0 = OpFunctionParameter %v4int
      %rhs_0 = OpFunctionParameter %v4int
         %14 = OpLabel
         %15 = OpIEqual %v4bool %rhs_0 %16
         %20 = OpIEqual %v4bool %lhs_0 %21
         %23 = OpIEqual %v4bool %rhs_0 %24
         %26 = OpLogicalAnd %v4bool %20 %23
         %27 = OpLogicalOr %v4bool %15 %26
         %28 = OpSelect %v4int %27 %29 %rhs_0
         %31 = OpSDiv %v4int %lhs_0 %28
         %32 = OpIMul %v4int %31 %28
         %33 = OpISub %v4int %lhs_0 %32
               OpReturnValue %33
               OpFunctionEnd
)");
}

TEST_F(SpirvWriterTest, Modulo_vec4i_i32) {
    Vector<core::ir::FunctionParam*, 4> args;
    args.Push(b.FunctionParam("lhs", ty.vec4<i32>()));
    args.Push(b.FunctionParam("rhs", ty.i32()));
    auto* func = b.Function("foo", ty.vec4<i32>());
    func->SetParams(args);
    b.Append(func->Block(), [&] {
        auto* result = b.Binary(core::ir::Binary::Kind::kModulo, ty.vec4<i32>(), args[0], args[1]);
        b.Return(func, result);
        mod.SetName(result, "result");
    });

    ASSERT_TRUE(Generate()) << Error() << output_;
    EXPECT_INST(R"(
               ; Function foo
        %foo = OpFunction %v4int None %6
        %lhs = OpFunctionParameter %v4int
        %rhs = OpFunctionParameter %int
          %7 = OpLabel
          %8 = OpCompositeConstruct %v4int %rhs %rhs %rhs %rhs
     %result = OpFunctionCall %v4int %tint_mod_v4i32 %lhs %8
               OpReturnValue %result
               OpFunctionEnd

               ; Function tint_mod_v4i32
%tint_mod_v4i32 = OpFunction %v4int None %13
      %lhs_0 = OpFunctionParameter %v4int
      %rhs_0 = OpFunctionParameter %v4int
         %14 = OpLabel
         %15 = OpIEqual %v4bool %rhs_0 %16
         %20 = OpIEqual %v4bool %lhs_0 %21
         %23 = OpIEqual %v4bool %rhs_0 %24
         %26 = OpLogicalAnd %v4bool %20 %23
         %27 = OpLogicalOr %v4bool %15 %26
         %28 = OpSelect %v4int %27 %29 %rhs_0
         %31 = OpSDiv %v4int %lhs_0 %28
         %32 = OpIMul %v4int %31 %28
         %33 = OpISub %v4int %lhs_0 %32
               OpReturnValue %33
               OpFunctionEnd
)");
}

}  // namespace
}  // namespace tint::spirv::writer
