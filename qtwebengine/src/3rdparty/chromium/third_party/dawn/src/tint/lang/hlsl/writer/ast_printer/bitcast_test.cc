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

#include "src/tint/lang/hlsl/writer/ast_printer/helper_test.h"
#include "src/tint/utils/text/string_stream.h"

#include "gmock/gmock.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::hlsl::writer {
namespace {

using HlslASTPrinterTest_Bitcast = TestHelper;

TEST_F(HlslASTPrinterTest_Bitcast, EmitExpression_Bitcast_Float) {
    auto* a = Let("a", Expr(1_i));
    auto* bitcast = Bitcast<f32>(Expr("a"));
    WrapInFunction(a, bitcast);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, bitcast)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "asfloat(a)");
}

TEST_F(HlslASTPrinterTest_Bitcast, EmitExpression_Bitcast_Int) {
    auto* a = Let("a", Expr(1_u));
    auto* bitcast = Bitcast<i32>(Expr("a"));
    WrapInFunction(a, bitcast);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, bitcast)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "asint(a)");
}

TEST_F(HlslASTPrinterTest_Bitcast, EmitExpression_Bitcast_Uint) {
    auto* a = Let("a", Expr(1_i));
    auto* bitcast = Bitcast<u32>(Expr("a"));
    WrapInFunction(a, bitcast);

    ASTPrinter& gen = Build();

    StringStream out;
    ASSERT_TRUE(gen.EmitExpression(out, bitcast)) << gen.Diagnostics();
    EXPECT_EQ(out.str(), "asuint(a)");
}

TEST_F(HlslASTPrinterTest_Bitcast, EmitExpression_Bitcast_F16_Vec2) {
    Enable(core::Extension::kF16);

    auto* a = Let("a", Call<vec2<f16>>(1_h, 2_h));
    auto* b = Let("b", Bitcast<i32>(Expr("a")));
    auto* c = Let("c", Bitcast<vec2<f16>>(Expr("b")));
    auto* d = Let("d", Bitcast<f32>(Expr("c")));
    auto* e = Let("e", Bitcast<vec2<f16>>(Expr("d")));
    auto* f = Let("f", Bitcast<u32>(Expr("e")));
    auto* g = Let("g", Bitcast<vec2<f16>>(Expr("f")));
    WrapInFunction(a, b, c, d, e, f, g);

    ASTPrinter& gen = Build();

    gen.Generate();
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(gen.Result(), R"(int tint_bitcast_from_f16(vector<float16_t, 2> src) {
  uint2 r = f32tof16(float2(src));
  return asint(uint((r.x & 0xffff) | ((r.y & 0xffff) << 16)));
}

vector<float16_t, 2> tint_bitcast_to_f16(int src) {
  uint v = asuint(src);
  float t_low = f16tof32(v & 0xffff);
  float t_high = f16tof32((v >> 16) & 0xffff);
  return vector<float16_t, 2>(t_low.x, t_high.x);
}

float tint_bitcast_from_f16_1(vector<float16_t, 2> src) {
  uint2 r = f32tof16(float2(src));
  return asfloat(uint((r.x & 0xffff) | ((r.y & 0xffff) << 16)));
}

vector<float16_t, 2> tint_bitcast_to_f16_1(float src) {
  uint v = asuint(src);
  float t_low = f16tof32(v & 0xffff);
  float t_high = f16tof32((v >> 16) & 0xffff);
  return vector<float16_t, 2>(t_low.x, t_high.x);
}

uint tint_bitcast_from_f16_2(vector<float16_t, 2> src) {
  uint2 r = f32tof16(float2(src));
  return asuint(uint((r.x & 0xffff) | ((r.y & 0xffff) << 16)));
}

vector<float16_t, 2> tint_bitcast_to_f16_2(uint src) {
  uint v = asuint(src);
  float t_low = f16tof32(v & 0xffff);
  float t_high = f16tof32((v >> 16) & 0xffff);
  return vector<float16_t, 2>(t_low.x, t_high.x);
}

[numthreads(1, 1, 1)]
void test_function() {
  const vector<float16_t, 2> a = vector<float16_t, 2>(float16_t(1.0h), float16_t(2.0h));
  const int b = tint_bitcast_from_f16(a);
  const vector<float16_t, 2> c = tint_bitcast_to_f16(b);
  const float d = tint_bitcast_from_f16_1(c);
  const vector<float16_t, 2> e = tint_bitcast_to_f16_1(d);
  const uint f = tint_bitcast_from_f16_2(e);
  const vector<float16_t, 2> g = tint_bitcast_to_f16_2(f);
  return;
}
)");
}

TEST_F(HlslASTPrinterTest_Bitcast, EmitExpression_Bitcast_F16_Vec4) {
    Enable(core::Extension::kF16);

    auto* a = Let("a", Call<vec4<f16>>(1_h, 2_h, 3_h, 4_h));
    auto* b = Let("b", Bitcast<vec2<i32>>(Expr("a")));
    auto* c = Let("c", Bitcast<vec4<f16>>(Expr("b")));
    auto* d = Let("d", Bitcast<vec2<f32>>(Expr("c")));
    auto* e = Let("e", Bitcast<vec4<f16>>(Expr("d")));
    auto* f = Let("f", Bitcast<vec2<u32>>(Expr("e")));
    auto* g = Let("g", Bitcast<vec4<f16>>(Expr("f")));
    WrapInFunction(a, b, c, d, e, f, g);

    ASTPrinter& gen = Build();

    gen.Generate();
    EXPECT_THAT(gen.Diagnostics(), testing::IsEmpty());
    EXPECT_EQ(gen.Result(), R"(int2 tint_bitcast_from_f16(vector<float16_t, 4> src) {
  uint4 r = f32tof16(float4(src));
  return asint(uint2((r.x & 0xffff) | ((r.y & 0xffff) << 16), (r.z & 0xffff) | ((r.w & 0xffff) << 16)));
}

vector<float16_t, 4> tint_bitcast_to_f16(int2 src) {
  uint2 v = asuint(src);
  float2 t_low = f16tof32(v & 0xffff);
  float2 t_high = f16tof32((v >> 16) & 0xffff);
  return vector<float16_t, 4>(t_low.x, t_high.x, t_low.y, t_high.y);
}

float2 tint_bitcast_from_f16_1(vector<float16_t, 4> src) {
  uint4 r = f32tof16(float4(src));
  return asfloat(uint2((r.x & 0xffff) | ((r.y & 0xffff) << 16), (r.z & 0xffff) | ((r.w & 0xffff) << 16)));
}

vector<float16_t, 4> tint_bitcast_to_f16_1(float2 src) {
  uint2 v = asuint(src);
  float2 t_low = f16tof32(v & 0xffff);
  float2 t_high = f16tof32((v >> 16) & 0xffff);
  return vector<float16_t, 4>(t_low.x, t_high.x, t_low.y, t_high.y);
}

uint2 tint_bitcast_from_f16_2(vector<float16_t, 4> src) {
  uint4 r = f32tof16(float4(src));
  return asuint(uint2((r.x & 0xffff) | ((r.y & 0xffff) << 16), (r.z & 0xffff) | ((r.w & 0xffff) << 16)));
}

vector<float16_t, 4> tint_bitcast_to_f16_2(uint2 src) {
  uint2 v = asuint(src);
  float2 t_low = f16tof32(v & 0xffff);
  float2 t_high = f16tof32((v >> 16) & 0xffff);
  return vector<float16_t, 4>(t_low.x, t_high.x, t_low.y, t_high.y);
}

[numthreads(1, 1, 1)]
void test_function() {
  const vector<float16_t, 4> a = vector<float16_t, 4>(float16_t(1.0h), float16_t(2.0h), float16_t(3.0h), float16_t(4.0h));
  const int2 b = tint_bitcast_from_f16(a);
  const vector<float16_t, 4> c = tint_bitcast_to_f16(b);
  const float2 d = tint_bitcast_from_f16_1(c);
  const vector<float16_t, 4> e = tint_bitcast_to_f16_1(d);
  const uint2 f = tint_bitcast_from_f16_2(e);
  const vector<float16_t, 4> g = tint_bitcast_to_f16_2(f);
  return;
}
)");
}

}  // namespace
}  // namespace tint::hlsl::writer
