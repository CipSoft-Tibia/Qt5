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

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/msl/writer/ast_printer/helper_test.h"
#include "src/tint/lang/wgsl/ast/variable_decl_statement.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::msl::writer {
namespace {

using MslASTPrinterTest = TestHelper;

TEST_F(MslASTPrinterTest, Emit_Loop) {
    auto* body = Block(Break());
    auto* continuing = Block();
    auto* l = Loop(body, continuing);

    Func("F", tint::Empty, ty.void_(), Vector{l}, Vector{Stage(ast::PipelineStage::kFragment)});

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(l)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while (true) {
    break;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_LoopWithContinuing) {
    Func("a_statement", {}, ty.void_(), tint::Empty);

    auto* body = Block(Break());
    auto* continuing = Block(CallStmt(Call("a_statement")));
    auto* l = Loop(body, continuing);

    Func("F", tint::Empty, ty.void_(), Vector{l}, Vector{Stage(ast::PipelineStage::kFragment)});

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(l)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while (true) {
    break;
    {
      a_statement();
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_LoopWithContinuing_BreakIf) {
    Func("a_statement", {}, ty.void_(), {});

    auto* body = Block(Break());
    auto* continuing = Block(CallStmt(Call("a_statement")), BreakIf(true));
    auto* l = Loop(body, continuing);

    Func("F", tint::Empty, ty.void_(), Vector{l}, Vector{Stage(ast::PipelineStage::kFragment)});

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(l)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while (true) {
    break;
    {
      a_statement();
      if (true) { break; }
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_LoopNestedWithContinuing) {
    Func("a_statement", {}, ty.void_(), tint::Empty);

    GlobalVar("lhs", ty.f32(), core::AddressSpace::kPrivate);
    GlobalVar("rhs", ty.f32(), core::AddressSpace::kPrivate);

    auto* body = Block(Break());
    auto* continuing = Block(CallStmt(Call("a_statement")));
    auto* inner = Loop(body, continuing);

    body = Block(inner);

    continuing = Block(Assign("lhs", "rhs"), BreakIf(true));

    auto* outer = Loop(body, continuing);

    Func("F", tint::Empty, ty.void_(), Vector{outer}, Vector{Stage(ast::PipelineStage::kFragment)});

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(outer)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while (true) {
    while (true) {
      break;
      {
        a_statement();
      }
    }
    {
      lhs = rhs;
      if (true) { break; }
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_LoopWithVarUsedInContinuing) {
    // loop {
    //   var lhs : f32 = 2.5;
    //   var other : f32;
    //   continuing {
    //     lhs = rhs
    //   }
    // }
    //

    GlobalVar("rhs", ty.f32(), core::AddressSpace::kPrivate);

    auto* body = Block(Decl(Var("lhs", ty.f32(), Expr(2.5_f))),  //
                       Decl(Var("other", ty.f32())),             //
                       Break());

    auto* continuing = Block(Assign("lhs", "rhs"));
    auto* outer = Loop(body, continuing);
    WrapInFunction(outer);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(outer)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while (true) {
    float lhs = 2.5f;
    float other = 0.0f;
    break;
    {
      lhs = rhs;
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoop) {
    // for(; ; ) {
    //   return;
    // }

    auto* f = For(nullptr, nullptr, nullptr,  //
                  Block(Return()));
    WrapInFunction(f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  for(; ; ) {
    return;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithSimpleInit) {
    // for(var i : i32; ; ) {
    //   return;
    // }

    auto* f = For(Decl(Var("i", ty.i32())), nullptr, nullptr,  //
                  Block(Return()));
    WrapInFunction(f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  for(int i = 0; ; ) {
    return;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithMultiStmtInit) {
    // fn f(i : i32) {}
    //
    // var<workgroup> a : atomic<i32>;
    // for({f(1i); f(2i);}; ; ) {
    //   return;
    // }

    Func("f", Vector{Param("i", ty.i32())}, ty.void_(), tint::Empty);
    auto f = [&](auto&& expr) { return CallStmt(Call("f", expr)); };

    GlobalVar("a", ty.atomic<i32>(), core::AddressSpace::kWorkgroup);
    auto* multi_stmt = Block(f(1_i), f(2_i));
    auto* loop = For(multi_stmt, nullptr, nullptr,  //
                     Block(Return()));
    WrapInFunction(loop);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(loop)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  {
    {
      f(1);
      f(2);
    }
    for(; ; ) {
      return;
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithSimpleCond) {
    // for(; true; ) {
    //   return;
    // }

    auto* f = For(nullptr, true, nullptr,  //
                  Block(Return()));
    WrapInFunction(f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  for(; true; ) {
    return;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithSimpleCont) {
    // for(; ; i = i + 1) {
    //   return;
    // }

    auto* v = Decl(Var("i", ty.i32()));
    auto* f = For(nullptr, nullptr, Assign("i", Add("i", 1_i)),  //
                  Block(Return()));
    WrapInFunction(v, f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(),
              R"(  for(; ; i = as_type<int>((as_type<uint>(i) + as_type<uint>(1)))) {
    return;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithMultiStmtCont) {
    // fn f(i : i32) {}
    //
    // var<workgroup> a : atomic<i32>;
    // for(; ; { f(1i); f(2i); }) {
    //   return;
    // }

    Func("f", Vector{Param("i", ty.i32())}, ty.void_(), tint::Empty);
    auto f = [&](auto&& expr) { return CallStmt(Call("f", expr)); };

    GlobalVar("a", ty.atomic<i32>(), core::AddressSpace::kWorkgroup);
    auto* multi_stmt = Block(f(1_i), f(2_i));
    auto* loop = For(nullptr, nullptr, multi_stmt,  //
                     Block(Return()));
    WrapInFunction(loop);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(loop)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while (true) {
    return;
    {
      f(1);
      f(2);
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithSimpleInitCondCont) {
    // for(var i : i32; true; i = i + 1) {
    //   return;
    // }

    Func("a_statement", {}, ty.void_(), tint::Empty);

    auto* f = For(Decl(Var("i", ty.i32())), true, Assign("i", Add("i", 1_i)),
                  Block(CallStmt(Call("a_statement"))));
    WrapInFunction(f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(),
              R"(  for(int i = 0; true; i = as_type<int>((as_type<uint>(i) + as_type<uint>(1)))) {
    a_statement();
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_ForLoopWithMultiStmtInitCondCont) {
    // fn f(i : i32) {}
    //
    // var<workgroup> a : atomic<i32>;
    // for({ f(1i); f(2i); }; true; { f(3i); f(4i); }) {
    //   return;
    // }

    Func("f", Vector{Param("i", ty.i32())}, ty.void_(), tint::Empty);
    auto f = [&](auto&& expr) { return CallStmt(Call("f", expr)); };

    GlobalVar("a", ty.atomic<i32>(), core::AddressSpace::kWorkgroup);
    auto* multi_stmt_a = Block(f(1_i), f(2_i));
    auto* multi_stmt_b = Block(f(3_i), f(4_i));
    auto* loop = For(multi_stmt_a, Expr(true), multi_stmt_b,  //
                     Block(Return()));
    WrapInFunction(loop);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(loop)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  {
    {
      f(1);
      f(2);
    }
    while (true) {
      if (!(true)) { break; }
      return;
      {
        f(3);
        f(4);
      }
    }
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_While) {
    // while(true) {
    //   return;
    // }

    auto* f = While(Expr(true), Block(Return()));
    WrapInFunction(f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while(true) {
    return;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_While_WithContinue) {
    // while(true) {
    //   continue;
    // }

    auto* f = While(Expr(true), Block(Continue()));
    WrapInFunction(f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while(true) {
    continue;
  }
)");
}

TEST_F(MslASTPrinterTest, Emit_WhileWithMultiCond) {
    // while(true && false) {
    //   return;
    // }

    auto* t = Let("t", Expr(true));
    auto* multi_stmt = LogicalAnd(t, false);
    // create<ast::BinaryExpression>(core::BinaryOp::kLogicalAnd, Expr(t), Expr(false));
    auto* f = While(multi_stmt, Block(Return()));
    WrapInFunction(t, f);

    ASTPrinter& gen = Build();

    gen.IncrementIndent();

    ASSERT_TRUE(gen.EmitStatement(f)) << gen.Diagnostics();
    EXPECT_EQ(gen.Result(), R"(  while((t && false)) {
    return;
  }
)");
}

}  // namespace
}  // namespace tint::msl::writer
