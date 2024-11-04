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

#include <sstream>
#include <string>

#include "src/tint/lang/core/ir/disassembler.h"
#include "src/tint/lang/wgsl/writer/ir_to_program/ir_to_program.h"
#include "src/tint/lang/wgsl/writer/ir_to_program/ir_to_program_test.h"
#include "src/tint/lang/wgsl/writer/writer.h"
#include "src/tint/utils/text/string.h"

namespace tint::wgsl::writer {

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

IRToProgramTest::Result IRToProgramTest::Run() {
    Result result;

    tint::core::ir::Disassembler d{mod};
    result.ir = d.Disassemble();

    auto output_program = IRToProgram(mod);
    if (!output_program.IsValid()) {
        result.err = output_program.Diagnostics().str();
        result.ast = Program::printer(&output_program);
        return result;
    }

    auto output = wgsl::writer::Generate(&output_program, {});
    if (!output) {
        std::stringstream ss;
        ss << "wgsl::Generate() errored: " << output.Failure();
        result.err = ss.str();
        return result;
    }

    result.wgsl = std::string(tint::TrimSpace(output->wgsl));
    if (!result.wgsl.empty()) {
        result.wgsl = "\n" + result.wgsl + "\n";
    }

    return result;
}

namespace {

TEST_F(IRToProgramTest, EmptyModule) {
    EXPECT_WGSL("");
}

TEST_F(IRToProgramTest, SingleFunction_Return) {
    auto* fn = b.Function("f", ty.void_());

    fn->Block()->Append(b.Return(fn));

    EXPECT_WGSL(R"(
fn f() {
}
)");
}

TEST_F(IRToProgramTest, SingleFunction_Return_i32) {
    auto* fn = b.Function("f", ty.i32());

    fn->Block()->Append(b.Return(fn, 42_i));

    EXPECT_WGSL(R"(
fn f() -> i32 {
  return 42i;
}
)");
}

TEST_F(IRToProgramTest, SingleFunction_Parameters) {
    auto* fn = b.Function("f", ty.i32());
    auto* i = b.FunctionParam("i", ty.i32());
    auto* u = b.FunctionParam("u", ty.u32());
    fn->SetParams({i, u});

    fn->Block()->Append(b.Return(fn, i));

    EXPECT_WGSL(R"(
fn f(i : i32, u : u32) -> i32 {
  return i;
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Unary ops
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, UnaryOp_Negate) {
    auto* fn = b.Function("f", ty.i32());
    auto* i = b.FunctionParam("i", ty.i32());
    fn->SetParams({i});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Negation(ty.i32(), i)); });

    EXPECT_WGSL(R"(
fn f(i : i32) -> i32 {
  return -(i);
}
)");
}

TEST_F(IRToProgramTest, UnaryOp_Complement) {
    auto* fn = b.Function("f", ty.u32());
    auto* i = b.FunctionParam("i", ty.u32());
    fn->SetParams({i});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Complement(ty.u32(), i)); });

    EXPECT_WGSL(R"(
fn f(i : u32) -> u32 {
  return ~(i);
}
)");
}

TEST_F(IRToProgramTest, UnaryOp_Not) {
    auto* fn = b.Function("f", ty.bool_());
    auto* i = b.FunctionParam("b", ty.bool_());
    fn->SetParams({i});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Not(ty.bool_(), i)); });

    EXPECT_WGSL(R"(
fn f(b : bool) -> bool {
  return !(b);
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Binary ops
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, BinaryOp_Add) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Add(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a + b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Subtract) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Subtract(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a - b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Multiply) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Multiply(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a * b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Divide) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Divide(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a / b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Modulo) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Modulo(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a % b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_And) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.And(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a & b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Or) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Or(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a | b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Xor) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Xor(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> i32 {
  return (a ^ b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_Equal) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.Equal(ty.bool_(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> bool {
  return (a == b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_NotEqual) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.NotEqual(ty.bool_(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> bool {
  return (a != b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_LessThan) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.LessThan(ty.bool_(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> bool {
  return (a < b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_GreaterThan) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.GreaterThan(ty.bool_(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> bool {
  return (a > b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_LessThanEqual) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.LessThanEqual(ty.bool_(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> bool {
  return (a <= b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_GreaterThanEqual) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.i32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.GreaterThanEqual(ty.bool_(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : i32) -> bool {
  return (a >= b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_ShiftLeft) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.u32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.ShiftLeft(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : u32) -> i32 {
  return (a << b);
}
)");
}

TEST_F(IRToProgramTest, BinaryOp_ShiftRight) {
    auto* fn = b.Function("f", ty.i32());
    auto* pa = b.FunctionParam("a", ty.i32());
    auto* pb = b.FunctionParam("b", ty.u32());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] { b.Return(fn, b.ShiftRight(ty.i32(), pa, pb)); });

    EXPECT_WGSL(R"(
fn f(a : i32, b : u32) -> i32 {
  return (a >> b);
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Short-circuiting binary ops
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, ShortCircuit_And_Param_2) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(pa);
        if_->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if_->True(), [&] { b.ExitIf(if_, pb); });
        b.Append(if_->False(), [&] { b.ExitIf(if_, false); });

        b.Return(fn, if_->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool) -> bool {
  return (a && b);
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Param_3_ab_c) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, pb); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, pc); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  return ((a && b) && c);
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Param_3_a_bc) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] {
            auto* if2 = b.If(pb);
            if2->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if2->True(), [&] { b.ExitIf(if2, pc); });
            b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

            b.ExitIf(if1, if2->Result(0));
        });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });
        b.Return(fn, if1->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  return (a && (b && c));
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Let_2) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(pa);
        if_->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if_->True(), [&] { b.ExitIf(if_, pb); });
        b.Append(if_->False(), [&] { b.ExitIf(if_, false); });

        mod.SetName(if_->Result(0), "l");
        b.Return(fn, if_->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool) -> bool {
  let l = (a && b);
  return l;
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Let_3_ab_c) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, pb); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, pc); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

        mod.SetName(if2->Result(0), "l");
        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  let l = ((a && b) && c);
  return l;
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Let_3_a_bc) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] {
            auto* if2 = b.If(pb);
            if2->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if2->True(), [&] { b.ExitIf(if2, pc); });
            b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

            b.ExitIf(if1, if2->Result(0));
        });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        mod.SetName(if1->Result(0), "l");
        b.Return(fn, if1->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  let l = (a && (b && c));
  return l;
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Call_2) {
    auto* fn_a = b.Function("a", ty.bool_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(b.Call(ty.bool_(), fn_a));
        if_->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if_->True(), [&] { b.ExitIf(if_, b.Call(ty.bool_(), fn_b)); });
        b.Append(if_->False(), [&] { b.ExitIf(if_, false); });

        b.Return(fn, if_->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn f() -> bool {
  return (a() && b());
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Call_3_ab_c) {
    auto* fn_a = b.Function("a", ty.bool_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_c = b.Function("c", ty.bool_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(b.Call(ty.bool_(), fn_a));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, b.Call(ty.bool_(), fn_b)); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, b.Call(ty.bool_(), fn_c)); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn c() -> bool {
  return true;
}

fn f() -> bool {
  return ((a() && b()) && c());
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_And_Call_3_a_bc) {
    auto* fn_a = b.Function("a", ty.bool_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_c = b.Function("c", ty.bool_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(b.Call(ty.bool_(), fn_a));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] {
            auto* if2 = b.If(b.Call(ty.bool_(), fn_b));
            if2->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if2->True(), [&] { b.ExitIf(if2, b.Call(ty.bool_(), fn_c)); });
            b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

            b.ExitIf(if1, if2->Result(0));
        });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        b.Return(fn, if1->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn c() -> bool {
  return true;
}

fn f() -> bool {
  return (a() && (b() && c()));
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Param_2) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(pa);
        if_->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if_->True(), [&] { b.ExitIf(if_, true); });
        b.Append(if_->False(), [&] { b.ExitIf(if_, pb); });

        b.Return(fn, if_->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool) -> bool {
  return (a || b);
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Param_3_ab_c) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, pb); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, pc); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  return ((a || b) || c);
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Param_3_a_bc) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] {
            auto* if2 = b.If(pb);
            if2->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
            b.Append(if2->False(), [&] { b.ExitIf(if2, pc); });

            b.ExitIf(if1, if2->Result(0));
        });

        b.Return(fn, if1->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  return (a || (b || c));
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Let_2) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    fn->SetParams({pa, pb});

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(pa);
        if_->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if_->True(), [&] { b.ExitIf(if_, true); });
        b.Append(if_->False(), [&] { b.ExitIf(if_, pb); });

        mod.SetName(if_->Result(0), "l");
        b.Return(fn, if_->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool) -> bool {
  let l = (a || b);
  return l;
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Let_3_ab_c) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, pb); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, pc); });

        mod.SetName(if2->Result(0), "l");
        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  let l = ((a || b) || c);
  return l;
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Let_3_a_bc) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] {
            auto* if2 = b.If(pb);
            if2->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
            b.Append(if2->False(), [&] { b.ExitIf(if2, pc); });

            b.ExitIf(if1, if2->Result(0));
        });

        mod.SetName(if1->Result(0), "l");
        b.Return(fn, if1->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  let l = (a || (b || c));
  return l;
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Call_2) {
    auto* fn_a = b.Function("a", ty.bool_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(b.Call(ty.bool_(), fn_a));
        if_->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if_->True(), [&] { b.ExitIf(if_, true); });
        b.Append(if_->False(), [&] { b.ExitIf(if_, b.Call(ty.bool_(), fn_b)); });

        b.Return(fn, if_->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn f() -> bool {
  return (a() || b());
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Call_3_ab_c) {
    auto* fn_a = b.Function("a", ty.bool_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_c = b.Function("c", ty.bool_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(b.Call(ty.bool_(), fn_a));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, b.Call(ty.bool_(), fn_b)); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, b.Call(ty.bool_(), fn_c)); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn c() -> bool {
  return true;
}

fn f() -> bool {
  return ((a() || b()) || c());
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Or_Call_3_a_bc) {
    auto* fn_a = b.Function("a", ty.bool_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_c = b.Function("c", ty.bool_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(b.Call(ty.bool_(), fn_a));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] {
            auto* if2 = b.If(b.Call(ty.bool_(), fn_b));
            if2->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
            b.Append(if2->False(), [&] { b.ExitIf(if2, b.Call(ty.bool_(), fn_c)); });

            b.ExitIf(if1, if2->Result(0));
        });

        b.Return(fn, if1->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn c() -> bool {
  return true;
}

fn f() -> bool {
  return (a() || (b() || c()));
}
)");
}

TEST_F(IRToProgramTest, ShortCircuit_Mixed) {
    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_d = b.Function("d", ty.bool_());
    b.Append(fn_d->Block(), [&] { b.Return(fn_d, true); });

    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    fn->SetParams({pa, pc});

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(pa);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, b.Call(ty.bool_(), fn_b)); });

        auto* if2 = b.If(if1->Result(0));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] {
            auto* if3 = b.If(pc);
            if3->SetResults(b.InstructionResult(ty.bool_()));
            b.Append(if3->True(), [&] { b.ExitIf(if3, true); });
            b.Append(if3->False(), [&] { b.ExitIf(if3, b.Call(ty.bool_(), fn_d)); });

            b.ExitIf(if2, if3->Result(0));
        });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

        mod.SetName(if2->Result(0), "l");
        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn b() -> bool {
  return true;
}

fn d() -> bool {
  return true;
}

fn f(a : bool, c : bool) -> bool {
  let l = ((a || b()) && (c || d()));
  return l;
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Non short-circuiting binary ops
// Similar to the above, but cannot be short-circuited as the RHS is evaluated
// outside of the if block.
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, NonShortCircuit_And_ParamCallParam_a_bc) {
    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam(ty.bool_());
    auto* pc = b.FunctionParam(ty.bool_());
    mod.SetName(pa, "a");
    mod.SetName(pc, "c");
    fn->SetParams({pa, pc});

    b.Append(fn->Block(), [&] {
        // 'b() && c' is evaluated before 'a'.
        auto* if1 = b.If(b.Call(ty.bool_(), fn_b));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, pc); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        auto* if2 = b.If(pa);
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, if1->Result(0)); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });
        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn b() -> bool {
  return true;
}

fn f(a : bool, c : bool) -> bool {
  let v = (b() && c);
  return (a && v);
}
)");
}

TEST_F(IRToProgramTest, NonShortCircuit_And_Call_3_a_bc) {
    auto* fn_a = b.Function("a", ty.bool_());

    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());

    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_c = b.Function("c", ty.bool_());

    b.Append(fn_c->Block(), [&] { b.Return(fn_c, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        // 'b() && c()' is evaluated before 'a()'.
        auto* if1 = b.If(b.Call(ty.bool_(), fn_b));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, b.Call(ty.bool_(), fn_c)); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        auto* if2 = b.If(b.Call(ty.bool_(), fn_a));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, if1->Result(0)); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn c() -> bool {
  return true;
}

fn f() -> bool {
  let v = (b() && c());
  return (a() && v);
}
)");
}

TEST_F(IRToProgramTest, NonShortCircuit_And_Param_3_a_bc_EarlyEval) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam(ty.bool_());
    auto* pb = b.FunctionParam(ty.bool_());
    auto* pc = b.FunctionParam(ty.bool_());
    mod.SetName(pa, "a");
    mod.SetName(pb, "b");
    mod.SetName(pc, "c");
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        // 'b && c' is evaluated outside the true block of if2, but these can be moved to the RHS
        // of the 'a &&' as the 'b && c' is not sequenced.
        auto* if1 = b.If(pb);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, pc); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, false); });

        auto* if2 = b.If(pa);
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, if1->Result(0)); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, false); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  let v = (b && c);
  return (a && v);
}
)");
}

TEST_F(IRToProgramTest, NonShortCircuit_Or_ParamCallParam_a_bc) {
    auto* fn_b = b.Function("b", ty.bool_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam(ty.bool_());
    auto* pc = b.FunctionParam(ty.bool_());
    mod.SetName(pa, "a");
    mod.SetName(pc, "c");
    fn->SetParams({pa, pc});

    b.Append(fn->Block(), [&] {
        // 'b() && c' is evaluated before 'a'.
        auto* if1 = b.If(b.Call(ty.bool_(), fn_b));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, pc); });

        auto* if2 = b.If(pa);
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, if1->Result(0)); });

        mod.SetName(if2->Result(0), "l");
        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn b() -> bool {
  return true;
}

fn f(a : bool, c : bool) -> bool {
  let v = (b() || c);
  let l = (a || v);
  return l;
}
)");
}

TEST_F(IRToProgramTest, NonShortCircuit_Or_Call_3_a_bc) {
    auto* fn_a = b.Function("a", ty.bool_());

    b.Append(fn_a->Block(), [&] { b.Return(fn_a, true); });

    auto* fn_b = b.Function("b", ty.bool_());

    b.Append(fn_b->Block(), [&] { b.Return(fn_b, true); });

    auto* fn_c = b.Function("c", ty.bool_());

    b.Append(fn_c->Block(), [&] { b.Return(fn_c, true); });

    auto* fn = b.Function("f", ty.bool_());

    b.Append(fn->Block(), [&] {
        auto* if1 = b.If(b.Call(ty.bool_(), fn_b));
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, b.Call(ty.bool_(), fn_c)); });

        auto* if2 = b.If(b.Call(ty.bool_(), fn_a));
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, if1->Result(0)); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn a() -> bool {
  return true;
}

fn b() -> bool {
  return true;
}

fn c() -> bool {
  return true;
}

fn f() -> bool {
  let v = (b() || c());
  return (a() || v);
}
)");
}

TEST_F(IRToProgramTest, NonShortCircuit_Or_Param_3_a_bc_EarlyEval) {
    auto* fn = b.Function("f", ty.bool_());
    auto* pa = b.FunctionParam(ty.bool_());
    auto* pb = b.FunctionParam(ty.bool_());
    auto* pc = b.FunctionParam(ty.bool_());
    mod.SetName(pa, "a");
    mod.SetName(pb, "b");
    mod.SetName(pc, "c");
    fn->SetParams({pa, pb, pc});

    b.Append(fn->Block(), [&] {
        // 'b || c' is evaluated outside the true block of if2, but these can be moved to the RHS
        // of the 'a ||' as the 'b || c' is not sequenced.
        auto* if1 = b.If(pb);
        if1->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if1->True(), [&] { b.ExitIf(if1, true); });
        b.Append(if1->False(), [&] { b.ExitIf(if1, pc); });

        auto* if2 = b.If(pa);
        if2->SetResults(b.InstructionResult(ty.bool_()));
        b.Append(if2->True(), [&] { b.ExitIf(if2, true); });
        b.Append(if2->False(), [&] { b.ExitIf(if2, if1->Result(0)); });

        b.Return(fn, if2->Result(0));
    });

    EXPECT_WGSL(R"(
fn f(a : bool, b : bool, c : bool) -> bool {
  let v = (b || c);
  return (a || v);
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Compound assignment
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, CompoundAssign_Increment) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Add(ty.i32(), b.Load(v), 1_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v + 1i);
}
)");
}

TEST_F(IRToProgramTest, CompoundAssign_Decrement) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Subtract(ty.i32(), b.Load(v), 1_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v - 1i);
}
)");
}

TEST_F(IRToProgramTest, CompoundAssign_Add) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Add(ty.i32(), b.Load(v), 8_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v + 8i);
}
)");
}

TEST_F(IRToProgramTest, CompoundAssign_Subtract) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Subtract(ty.i32(), b.Load(v), 8_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v - 8i);
}
)");
}

TEST_F(IRToProgramTest, CompoundAssign_Multiply) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Multiply(ty.i32(), b.Load(v), 8_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v * 8i);
}
)");
}

TEST_F(IRToProgramTest, CompoundAssign_Divide) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Divide(ty.i32(), b.Load(v), 8_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v / 8i);
}
)");
}

TEST_F(IRToProgramTest, CompoundAssign_Xor) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        b.Store(v, b.Xor(ty.i32(), b.Load(v), 8_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var v : i32;
  v = (v ^ 8i);
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// let
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, LetUsedOnce) {
    auto* fn = b.Function("f", ty.u32());
    auto* i = b.FunctionParam("i", ty.u32());
    fn->SetParams({i});

    b.Append(fn->Block(), [&] {
        auto* v = b.Complement(ty.u32(), i);
        b.Return(fn, v);
        mod.SetName(v, "v");
    });

    EXPECT_WGSL(R"(
fn f(i : u32) -> u32 {
  let v = ~(i);
  return v;
}
)");
}

TEST_F(IRToProgramTest, LetUsedTwice) {
    auto* fn = b.Function("f", ty.i32());
    auto* i = b.FunctionParam("i", ty.i32());
    fn->SetParams({i});

    b.Append(fn->Block(), [&] {
        auto* v = b.Multiply(ty.i32(), i, 2_i);
        b.Return(fn, b.Add(ty.i32(), v, v));
        mod.SetName(v, "v");
    });

    EXPECT_WGSL(R"(
fn f(i : i32) -> i32 {
  let v = (i * 2i);
  return (v + v);
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Function-scope var
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, FunctionScopeVar_i32) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {  //
        b.Var("i", ty.ptr<function, i32>());

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var i : i32;
}
)");
}

TEST_F(IRToProgramTest, FunctionScopeVar_i32_InitLiteral) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* i = b.Var("i", ty.ptr<function, i32>());
        i->SetInitializer(b.Constant(42_i));

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var i : i32 = 42i;
}
)");
}

TEST_F(IRToProgramTest, FunctionScopeVar_Chained) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* va = b.Var("a", ty.ptr<function, i32>());
        va->SetInitializer(b.Constant(42_i));

        auto* la = b.Load(va)->Result();
        auto* vb = b.Var("b", ty.ptr<function, i32>());
        vb->SetInitializer(la);

        auto* lb = b.Load(vb)->Result();
        auto* vc = b.Var("c", ty.ptr<function, i32>());
        vc->SetInitializer(lb);

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var a : i32 = 42i;
  var b : i32 = a;
  var c : i32 = b;
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// If
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, If_CallFn) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto* if_ = b.If(cond);
        b.Append(if_->True(), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitIf(if_);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn f(cond : bool) {
  if (cond) {
    a();
  }
}
)");
}

TEST_F(IRToProgramTest, If_Return) {
    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto if_ = b.If(cond);
        b.Append(if_->True(), [&] { b.Return(fn); });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f(cond : bool) {
  if (cond) {
    return;
  }
}
)");
}

TEST_F(IRToProgramTest, If_Return_i32) {
    auto* fn = b.Function("f", ty.i32());

    b.Append(fn->Block(), [&] {
        auto* cond = b.Var("cond", ty.ptr<function, bool>());
        cond->SetInitializer(b.Constant(true));
        auto if_ = b.If(b.Load(cond));
        b.Append(if_->True(), [&] { b.Return(fn, 42_i); });

        b.Return(fn, 10_i);
    });

    EXPECT_WGSL(R"(
fn f() -> i32 {
  var cond : bool = true;
  if (cond) {
    return 42i;
  }
  return 10i;
}
)");
}

TEST_F(IRToProgramTest, If_CallFn_Else_CallFn) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn_b = b.Function("b", ty.void_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b); });

    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto if_ = b.If(cond);
        b.Append(if_->True(), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitIf(if_);
        });
        b.Append(if_->False(), [&] {
            b.Call(ty.void_(), fn_b);
            b.ExitIf(if_);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn b() {
}

fn f(cond : bool) {
  if (cond) {
    a();
  } else {
    b();
  }
}
)");
}

TEST_F(IRToProgramTest, If_Return_f32_Else_Return_f32) {
    auto* fn = b.Function("f", ty.f32());

    b.Append(fn->Block(), [&] {
        auto* cond = b.Var("cond", ty.ptr<function, bool>());
        cond->SetInitializer(b.Constant(true));
        auto if_ = b.If(b.Load(cond));
        b.Append(if_->True(), [&] { b.Return(fn, 1.0_f); });
        b.Append(if_->False(), [&] { b.Return(fn, 2.0_f); });

        b.Unreachable();
    });

    EXPECT_WGSL(R"(
fn f() -> f32 {
  var cond : bool = true;
  if (cond) {
    return 1.0f;
  } else {
    return 2.0f;
  }
}
)");
}

TEST_F(IRToProgramTest, If_Return_u32_Else_CallFn) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn_b = b.Function("b", ty.void_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b); });

    auto* fn = b.Function("f", ty.u32());

    b.Append(fn->Block(), [&] {
        auto* cond = b.Var("cond", ty.ptr<function, bool>());
        cond->SetInitializer(b.Constant(true));
        auto if_ = b.If(b.Load(cond));
        b.Append(if_->True(), [&] { b.Return(fn, 1_u); });
        b.Append(if_->False(), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitIf(if_);
        });
        b.Call(ty.void_(), fn_b);
        b.Return(fn, 2_u);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn b() {
}

fn f() -> u32 {
  var cond : bool = true;
  if (cond) {
    return 1u;
  } else {
    a();
  }
  b();
  return 2u;
}
)");
}

TEST_F(IRToProgramTest, If_CallFn_ElseIf_CallFn) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn_b = b.Function("b", ty.void_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b); });

    auto* fn_c = b.Function("c", ty.void_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c); });

    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* cond = b.Var("cond", ty.ptr<function, bool>());
        cond->SetInitializer(b.Constant(true));
        auto if1 = b.If(b.Load(cond));
        b.Append(if1->True(), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitIf(if1);
        });
        b.Append(if1->False(), [&] {
            auto* if2 = b.If(b.Constant(false));
            b.Append(if2->True(), [&] {
                b.Call(ty.void_(), fn_b);
                b.ExitIf(if2);
            });
            b.ExitIf(if1);
        });
        b.Call(ty.void_(), fn_c);

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn b() {
}

fn c() {
}

fn f() {
  var cond : bool = true;
  if (cond) {
    a();
  } else if (false) {
    b();
  }
  c();
}
)");
}

TEST_F(IRToProgramTest, If_Else_Chain) {
    auto* x = b.Function("x", ty.bool_());
    auto* i = b.FunctionParam("i", ty.i32());
    x->SetParams({i});
    b.Append(x->Block(), [&] { b.Return(x, true); });

    auto* fn = b.Function("f", ty.void_());
    auto* pa = b.FunctionParam("a", ty.bool_());
    auto* pb = b.FunctionParam("b", ty.bool_());
    auto* pc = b.FunctionParam("c", ty.bool_());
    auto* pd = b.FunctionParam("d", ty.bool_());
    fn->SetParams({pa, pb, pc, pd});

    b.Append(fn->Block(), [&] {
        auto if1 = b.If(pa);
        b.Append(if1->True(), [&] {
            b.Call(ty.void_(), x, 0_i);
            b.ExitIf(if1);
        });
        b.Append(if1->False(), [&] {
            auto* if2 = b.If(pb);
            b.Append(if2->True(), [&] {
                b.Call(ty.void_(), x, 1_i);
                b.ExitIf(if2);
            });
            b.Append(if2->False(), [&] {
                auto* if3 = b.If(pc);
                b.Append(if3->True(), [&] {
                    b.Call(ty.void_(), x, 2_i);
                    b.ExitIf(if3);
                });
                b.Append(if3->False(), [&] {
                    b.Call(ty.void_(), x, 3_i);
                    b.ExitIf(if3);
                });
                b.ExitIf(if2);
            });
            b.ExitIf(if1);
        });

        b.Return(fn);
    });
    EXPECT_WGSL(R"(
fn x(i : i32) -> bool {
  return true;
}

fn f(a : bool, b : bool, c : bool, d : bool) {
  if (a) {
    x(0i);
  } else if (b) {
    x(1i);
  } else if (c) {
    x(2i);
  } else {
    x(3i);
  }
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Switch
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, Switch_Default) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        v->SetInitializer(b.Constant(42_i));

        auto s = b.Switch(b.Load(v));
        b.Append(b.Case(s, {core::ir::Switch::CaseSelector{}}), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitSwitch(s);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn f() {
  var v : i32 = 42i;
  switch(v) {
    default: {
      a();
    }
  }
}
)");
}

TEST_F(IRToProgramTest, Switch_3_Cases) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn_b = b.Function("b", ty.void_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b); });

    auto* fn_c = b.Function("c", ty.void_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c); });

    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        v->SetInitializer(b.Constant(42_i));

        auto s = b.Switch(b.Load(v));
        b.Append(b.Case(s, {core::ir::Switch::CaseSelector{b.Constant(0_i)}}), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitSwitch(s);
        });
        b.Append(b.Case(s,
                        {
                            core::ir::Switch::CaseSelector{b.Constant(1_i)},
                            core::ir::Switch::CaseSelector{},
                        }),
                 [&] {
                     b.Call(ty.void_(), fn_b);
                     b.ExitSwitch(s);
                 });
        b.Append(b.Case(s, {core::ir::Switch::CaseSelector{b.Constant(2_i)}}), [&] {
            b.Call(ty.void_(), fn_c);
            b.ExitSwitch(s);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn b() {
}

fn c() {
}

fn f() {
  var v : i32 = 42i;
  switch(v) {
    case 0i: {
      a();
    }
    case 1i, default: {
      b();
    }
    case 2i: {
      c();
    }
  }
}
)");
}

TEST_F(IRToProgramTest, Switch_3_Cases_AllReturn) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* v = b.Var("v", ty.ptr<function, i32>());
        v->SetInitializer(b.Constant(42_i));

        auto s = b.Switch(b.Load(v));
        b.Append(b.Case(s, {core::ir::Switch::CaseSelector{b.Constant(0_i)}}),
                 [&] { b.Return(fn); });
        b.Append(b.Case(s,
                        {
                            core::ir::Switch::CaseSelector{b.Constant(1_i)},
                            core::ir::Switch::CaseSelector{},
                        }),
                 [&] { b.Return(fn); });
        b.Append(b.Case(s, {core::ir::Switch::CaseSelector{b.Constant(2_i)}}),
                 [&] { b.Return(fn); });

        b.Call(ty.void_(), fn_a);
        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn a() {
}

fn f() {
  var v : i32 = 42i;
  switch(v) {
    case 0i: {
      return;
    }
    case 1i, default: {
      return;
    }
    case 2i: {
      return;
    }
  }
  a();
}
)");
}

TEST_F(IRToProgramTest, Switch_Nested) {
    auto* fn_a = b.Function("a", ty.void_());
    b.Append(fn_a->Block(), [&] { b.Return(fn_a); });

    auto* fn_b = b.Function("b", ty.void_());
    b.Append(fn_b->Block(), [&] { b.Return(fn_b); });

    auto* fn_c = b.Function("c", ty.void_());
    b.Append(fn_c->Block(), [&] { b.Return(fn_c); });

    auto* fn = b.Function("f", ty.void_());
    b.Append(fn->Block(), [&] {
        auto* v1 = b.Var("v1", ty.ptr<function, i32>());
        v1->SetInitializer(b.Constant(42_i));

        auto* v2 = b.Var("v2", ty.ptr<function, i32>());
        v2->SetInitializer(b.Constant(24_i));

        auto s1 = b.Switch(b.Load(v1));
        b.Append(b.Case(s1, {core::ir::Switch::CaseSelector{b.Constant(0_i)}}), [&] {
            b.Call(ty.void_(), fn_a);
            b.ExitSwitch(s1);
        });
        b.Append(b.Case(s1,
                        {
                            core::ir::Switch::CaseSelector{b.Constant(1_i)},
                            core::ir::Switch::CaseSelector{},
                        }),
                 [&] {
                     auto s2 = b.Switch(b.Load(v2));
                     b.Append(b.Case(s2, {core::ir::Switch::CaseSelector{b.Constant(0_i)}}),
                              [&] { b.ExitSwitch(s2); });
                     b.Append(b.Case(s2,
                                     {
                                         core::ir::Switch::CaseSelector{b.Constant(1_i)},
                                         core::ir::Switch::CaseSelector{},
                                     }),
                              [&] { b.Return(fn); });

                     b.ExitSwitch(s1);
                 });
        b.Append(b.Case(s1, {core::ir::Switch::CaseSelector{b.Constant(2_i)}}), [&] {
            b.Call(ty.void_(), fn_c);
            b.ExitSwitch(s1);
        });

        b.Return(fn);
    });
    EXPECT_WGSL(R"(
fn a() {
}

fn b() {
}

fn c() {
}

fn f() {
  var v1 : i32 = 42i;
  var v2 : i32 = 24i;
  switch(v1) {
    case 0i: {
      a();
    }
    case 1i, default: {
      switch(v2) {
        case 0i: {
        }
        case 1i, default: {
          return;
        }
      }
    }
    case 2i: {
      c();
    }
  }
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// For
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, For_Empty) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Initializer(), [&] {
            auto* i = b.Var("i", ty.ptr<function, i32>());
            i->SetInitializer(b.Constant(0_i));
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* if_ = b.If(b.LessThan(ty.bool_(), b.Load(i), 5_i));
                b.Append(if_->True(), [&] { b.ExitIf(if_); });
                b.Append(if_->False(), [&] { b.ExitLoop(loop); });
                b.Continue(loop);
            });

            b.Append(loop->Continuing(), [&] {
                b.Store(i, b.Add(ty.i32(), b.Load(i), 1_i));
                b.NextIteration(loop);
            });
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  for(var i : i32 = 0i; (i < 5i); i = (i + 1i)) {
  }
}
)");
}

TEST_F(IRToProgramTest, For_Empty_NoInit) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* i = b.Var("i", ty.ptr<function, i32>());
        i->SetInitializer(b.Constant(0_i));

        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if_ = b.If(b.LessThan(ty.bool_(), b.Load(i), 5_i));
            b.Append(if_->True(), [&] { b.ExitIf(if_); });
            b.Append(if_->False(), [&] { b.ExitLoop(loop); });
            b.Continue(loop);
        });

        b.Append(loop->Continuing(), [&] {
            b.Store(i, b.Add(ty.i32(), b.Load(i), 1_i));
            b.NextIteration(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var i : i32 = 0i;
  for(; (i < 5i); i = (i + 1i)) {
  }
}
)");
}

TEST_F(IRToProgramTest, For_Empty_NoCont) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Initializer(), [&] {
            auto* i = b.Var("i", ty.ptr<function, i32>());
            i->SetInitializer(b.Constant(0_i));
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* if_ = b.If(b.LessThan(ty.bool_(), b.Load(i), 5_i));
                b.Append(if_->True(), [&] { b.ExitIf(if_); });
                b.Append(if_->False(), [&] { b.ExitLoop(loop); });
                b.Continue(loop);
            });
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  for(var i : i32 = 0i; (i < 5i); ) {
  }
}
)");
}

TEST_F(IRToProgramTest, For_ComplexBody) {
    auto* a = b.Function("a", ty.bool_());
    auto* v = b.FunctionParam("v", ty.i32());
    a->SetParams({v});
    b.Append(a->Block(), [&] { b.Return(a, b.Equal(ty.bool_(), v, 1_i)); });

    auto* fn = b.Function("f", ty.i32());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Initializer(), [&] {
            auto* i = b.Var("i", ty.ptr<function, i32>());
            i->SetInitializer(b.Constant(0_i));
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* if1 = b.If(b.LessThan(ty.bool_(), b.Load(i), 5_i));
                b.Append(if1->True(), [&] { b.ExitIf(if1); });
                b.Append(if1->False(), [&] { b.ExitLoop(loop); });

                auto* if2 = b.If(b.Call(ty.bool_(), a, 42_i));
                b.Append(if2->True(), [&] { b.Return(fn, 1_i); });
                b.Append(if2->False(), [&] { b.Return(fn, 2_i); });
                b.Unreachable();
            });

            b.Append(loop->Continuing(), [&] {
                b.Store(i, b.Add(ty.i32(), b.Load(i), 1_i));
                b.NextIteration(loop);
            });
        });

        b.Return(fn, 3_i);
    });

    EXPECT_WGSL(R"(
fn a(v : i32) -> bool {
  return (v == 1i);
}

fn f() -> i32 {
  for(var i : i32 = 0i; (i < 5i); i = (i + 1i)) {
    if (a(42i)) {
      return 1i;
    } else {
      return 2i;
    }
  }
  return 3i;
}
)");
}

TEST_F(IRToProgramTest, For_ComplexBody_NoInit) {
    auto* a = b.Function("a", ty.bool_());
    auto* v = b.FunctionParam("v", ty.i32());
    a->SetParams({v});
    b.Append(a->Block(), [&] { b.Return(a, b.Equal(ty.bool_(), v, 1_i)); });

    auto* fn = b.Function("f", ty.i32());

    b.Append(fn->Block(), [&] {
        auto* i = b.Var("i", ty.ptr<function, i32>());
        i->SetInitializer(b.Constant(0_i));

        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if1 = b.If(b.LessThan(ty.bool_(), b.Load(i), 5_i));
            b.Append(if1->True(), [&] { b.ExitIf(if1); });
            b.Append(if1->False(), [&] { b.ExitLoop(loop); });

            auto* if2 = b.If(b.Call(ty.bool_(), a, 42_i));
            b.Append(if2->True(), [&] { b.Return(fn, 1_i); });
            b.Append(if2->False(), [&] { b.Return(fn, 2_i); });

            b.Continue(loop);
        });

        b.Append(loop->Continuing(), [&] {
            b.Store(i, b.Add(ty.i32(), b.Load(i), 1_i));
            b.NextIteration(loop);
        });

        b.Return(fn, 3_i);
    });

    EXPECT_WGSL(R"(
fn a(v : i32) -> bool {
  return (v == 1i);
}

fn f() -> i32 {
  var i : i32 = 0i;
  for(; (i < 5i); i = (i + 1i)) {
    if (a(42i)) {
      return 1i;
    } else {
      return 2i;
    }
  }
  return 3i;
}
)");
}

TEST_F(IRToProgramTest, For_ComplexBody_NoCont) {
    auto* a = b.Function("a", ty.bool_());
    auto* v = b.FunctionParam("v", ty.i32());
    a->SetParams({v});
    b.Append(a->Block(), [&] { b.Return(a, b.Equal(ty.bool_(), v, 1_i)); });

    auto* fn = b.Function("f", ty.i32());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Initializer(), [&] {
            auto* i = b.Var("i", ty.ptr<function, i32>());
            i->SetInitializer(b.Constant(0_i));
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* if1 = b.If(b.LessThan(ty.bool_(), b.Load(i), 5_i));
                b.Append(if1->True(), [&] { b.ExitIf(if1); });
                b.Append(if1->False(), [&] { b.ExitLoop(loop); });

                auto* if2 = b.If(b.Call(ty.bool_(), a, 42_i));
                b.Append(if2->True(), [&] { b.Return(fn, 1_i); });
                b.Append(if2->False(), [&] { b.Return(fn, 2_i); });

                b.NextIteration(loop);
            });
        });

        b.Return(fn, 3_i);
    });

    EXPECT_WGSL(R"(
fn a(v : i32) -> bool {
  return (v == 1i);
}

fn f() -> i32 {
  for(var i : i32 = 0i; (i < 5i); ) {
    if (a(42i)) {
      return 1i;
    } else {
      return 2i;
    }
  }
  return 3i;
}
)");
}

TEST_F(IRToProgramTest, For_CallInInitCondCont) {
    auto* fn_n = b.Function("n", ty.i32());
    auto* v = b.FunctionParam("v", ty.i32());
    fn_n->SetParams({v});
    b.Append(fn_n->Block(), [&] { b.Return(fn_n, b.Add(ty.i32(), v, 1_i)); });

    auto* fn_f = b.Function("f", ty.void_());

    b.Append(fn_f->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Initializer(), [&] {
            auto* n_0 = b.Call(ty.i32(), fn_n, 0_i)->Result();
            auto* i = b.Var("i", ty.ptr<function, i32>());
            i->SetInitializer(n_0);
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* load = b.Load(i);
                auto* call = b.Call(ty.i32(), fn_n, 1_i);
                auto* if_ = b.If(b.LessThan(ty.bool_(), load, call));
                b.Append(if_->True(), [&] { b.ExitIf(if_); });
                b.Append(if_->False(), [&] { b.ExitLoop(loop); });

                b.Continue(loop);
            });

            b.Append(loop->Continuing(), [&] {
                b.Store(i, b.Call(ty.i32(), fn_n, b.Load(i)));
                b.NextIteration(loop);
            });
        });

        b.Return(fn_f);
    });

    EXPECT_WGSL(R"(
fn n(v : i32) -> i32 {
  return (v + 1i);
}

fn f() {
  for(var i : i32 = n(0i); (i < n(1i)); i = n(i)) {
  }
}
)");
}

TEST_F(IRToProgramTest, For_IncInInit_Cmp) {
    // %b1 = block {  # root
    //   %i:ptr<storage, u32, read_write> = var @binding_point(0, 0)
    // }
    //
    // %f = func():void -> %b2 {
    //   %b2 = block {
    //     loop [i: %b3, b: %b4] {  # loop_1
    //       %b3 = block {  # initializer
    //         %3:u32 = load %i
    //         %4:u32 = add %3, 1u
    //         store %i, %4
    //         next_iteration %b4
    //       }
    //       %b4 = block {  # body
    //         %5:u32 = load %i
    //         %6:bool = lt %5, 10u
    //         if %6 [t: %b5, f: %b6] {  # if_1
    //           %b5 = block {  # true
    //             exit_if  # if_1
    //           }
    //           %b6 = block {  # false
    //             exit_loop  # loop_1
    //           }
    //         }
    //         continue %b7
    //       }
    //     }
    //     ret
    //   }
    // }

    b.Append(b.RootBlock(), [&] {
        auto* i = b.Var(ty.ptr<storage, u32, read_write>());
        i->SetBindingPoint(0, 0);

        auto* fn_f = b.Function("f", ty.void_());

        b.Append(fn_f->Block(), [&] {
            auto* loop = b.Loop();

            b.Append(loop->Initializer(), [&] {
                auto* load_i = b.Load(i);
                auto* inc_i = b.Add(ty.i32(), load_i, 1_u);
                b.Store(i, inc_i);
                b.NextIteration(loop);
            });

            b.Append(loop->Body(), [&] {
                auto* load_i = b.Load(i);
                auto* cmp = b.LessThan(ty.bool_(), load_i, 10_u);
                auto* if_ = b.If(cmp);
                b.Append(if_->True(), [&] { b.ExitIf(if_); });
                b.Append(if_->False(), [&] { b.ExitLoop(loop); });
                b.Continue(loop);
            });

            b.Return(fn_f);
        });
    });

    EXPECT_WGSL(R"(
@group(0) @binding(0) var<storage, read_write> v : u32;

fn f() {
  for(v = (v + 1u); (v < 10u); ) {
  }
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// While
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, While_Empty) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* cond = b.If(true);
            b.Append(cond->True(), [&] { b.ExitIf(cond); });
            b.Append(cond->False(), [&] { b.ExitLoop(loop); });

            b.Continue(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  while(true) {
  }
}
)");
}

TEST_F(IRToProgramTest, While_Cond) {
    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if_ = b.If(cond);
            b.Append(if_->True(), [&] { b.ExitIf(if_); });
            b.Append(if_->False(), [&] { b.ExitLoop(loop); });

            b.Continue(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f(cond : bool) {
  while(cond) {
  }
}
)");
}

TEST_F(IRToProgramTest, While_Break) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* cond = b.If(true);
            b.Append(cond->True(), [&] { b.ExitIf(cond); });
            b.Append(cond->False(), [&] { b.ExitLoop(loop); });

            b.ExitLoop(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  while(true) {
    break;
  }
}
)");
}

TEST_F(IRToProgramTest, While_IfBreak) {
    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if1 = b.If(true);
            b.Append(if1->True(), [&] { b.ExitIf(if1); });
            b.Append(if1->False(), [&] { b.ExitLoop(loop); });

            auto* if2 = b.If(cond);
            b.Append(if2->True(), [&] { b.ExitLoop(loop); });

            b.Continue(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f(cond : bool) {
  while(true) {
    if (cond) {
      break;
    }
  }
}
)");
}

TEST_F(IRToProgramTest, While_IfReturn) {
    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if1 = b.If(true);
            b.Append(if1->True(), [&] { b.ExitIf(if1); });
            b.Append(if1->False(), [&] { b.ExitLoop(loop); });

            auto* if2 = b.If(cond);
            b.Append(if2->True(), [&] { b.Return(fn); });

            b.Continue(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f(cond : bool) {
  while(true) {
    if (cond) {
      return;
    }
  }
}
)");
}

////////////////////////////////////////////////////////////////////////////////
// Loop
////////////////////////////////////////////////////////////////////////////////
TEST_F(IRToProgramTest, Loop_Break) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] { b.ExitLoop(loop); });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  loop {
    break;
  }
}
)");
}

TEST_F(IRToProgramTest, Loop_IfBreak) {
    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if_ = b.If(cond);
            b.Append(if_->True(), [&] { b.ExitLoop(loop); });
            b.Continue(loop);
        });

        b.Return(fn);
    });
    EXPECT_WGSL(R"(
fn f(cond : bool) {
  loop {
    if (cond) {
      break;
    }
  }
}
)");
}

TEST_F(IRToProgramTest, Loop_IfReturn) {
    auto* fn = b.Function("f", ty.void_());
    auto* cond = b.FunctionParam("cond", ty.bool_());
    fn->SetParams({cond});

    b.Append(fn->Block(), [&] {
        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if_ = b.If(cond);
            b.Append(if_->True(), [&] { b.Return(fn); });
            b.Continue(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f(cond : bool) {
  loop {
    if (cond) {
      return;
    }
  }
}
)");
}

TEST_F(IRToProgramTest, Loop_IfContinuing) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* cond = b.Var("cond", ty.ptr<function, bool>());
        cond->SetInitializer(b.Constant(false));

        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* if_ = b.If(b.Load(cond));
            b.Append(if_->True(), [&] { b.Return(fn); });
            b.Continue(loop);
        });
        b.Append(loop->Continuing(), [&] {
            b.Store(cond, true);
            b.NextIteration(loop);
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var cond : bool = false;
  loop {
    if (cond) {
      return;
    }

    continuing {
      cond = true;
    }
  }
}
)");
}

TEST_F(IRToProgramTest, Loop_VarsDeclaredOutsideAndInside) {
    auto* fn = b.Function("f", ty.void_());

    b.Append(fn->Block(), [&] {
        auto* var_b = b.Var("b", ty.ptr<function, i32>());
        var_b->SetInitializer(b.Constant(1_i));

        auto* loop = b.Loop();

        b.Append(loop->Body(), [&] {
            auto* var_a = b.Var("a", ty.ptr<function, i32>());
            var_a->SetInitializer(b.Constant(2_i));

            auto* body_load_a = b.Load(var_a);
            auto* body_load_b = b.Load(var_b);
            auto* if_ = b.If(b.Equal(ty.bool_(), body_load_a, body_load_b));
            b.Append(if_->True(), [&] { b.Return(fn); });
            b.Append(if_->False(), [&] { b.ExitIf(if_); });
            b.Continue(loop);

            b.Append(loop->Continuing(), [&] {
                auto* cont_load_a = b.Load(var_a);
                auto* cont_load_b = b.Load(var_b);
                b.Store(var_b, b.Add(ty.i32(), cont_load_a, cont_load_b));
                b.NextIteration(loop);
            });
        });

        b.Return(fn);
    });

    EXPECT_WGSL(R"(
fn f() {
  var b : i32 = 1i;
  loop {
    var a : i32 = 2i;
    if ((a == b)) {
      return;
    }

    continuing {
      b = (a + b);
    }
  }
}
)");
}

}  // namespace
}  // namespace tint::wgsl::writer
