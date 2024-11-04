// Copyright 2021 The Tint Authors.
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
#include "src/tint/lang/spirv/reader/ast_parser/function.h"
#include "src/tint/lang/spirv/reader/ast_parser/helper_test.h"
#include "src/tint/lang/spirv/reader/ast_parser/spirv_tools_helpers_test.h"
#include "src/tint/lang/wgsl/ast/call_statement.h"
#include "src/tint/lang/wgsl/sem/call.h"

namespace tint::spirv::reader::ast_parser {
namespace {

using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::StartsWith;

Program ParseAndBuild(std::string spirv) {
    const char* preamble = R"(OpCapability Shader
            OpMemoryModel Logical GLSL450
            OpEntryPoint GLCompute %main "main"
            OpExecutionMode %main LocalSize 1 1 1
            OpName %main "main"
)";

    auto p = std::make_unique<ASTParser>(test::Assemble(preamble + spirv));
    if (!p->BuildAndParseInternalModule()) {
        ProgramBuilder builder;
        builder.Diagnostics().add_error(diag::System::Reader, p->error());
        return Program(std::move(builder));
    }
    return p->Program();
}

TEST_F(SpirvASTParserTest, WorkgroupBarrier) {
    auto program = ParseAndBuild(R"(
               OpName %helper "helper"
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
   %uint_264 = OpConstant %uint 264
     %helper = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_2 %uint_264
               OpReturn
               OpFunctionEnd
     %main = OpFunction %void None %1
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )");
    ASSERT_TRUE(program.IsValid()) << program.Diagnostics().str();
    auto* helper = program.AST().Functions().Find(program.Symbols().Get("helper"));
    ASSERT_NE(helper, nullptr);
    ASSERT_GT(helper->body->statements.Length(), 0u);
    auto* call = helper->body->statements[0]->As<ast::CallStatement>();
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->expr->args.Length(), 0u);
    auto* sem_call = program.Sem().Get<sem::Call>(call->expr);
    ASSERT_NE(sem_call, nullptr);
    auto* builtin = sem_call->Target()->As<sem::Builtin>();
    ASSERT_NE(builtin, nullptr);
    EXPECT_EQ(builtin->Type(), core::Function::kWorkgroupBarrier);
}

TEST_F(SpirvASTParserTest, StorageBarrier) {
    auto program = ParseAndBuild(R"(
               OpName %helper "helper"
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
     %uint_1 = OpConstant %uint 1
    %uint_72 = OpConstant %uint 72
     %helper = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_2 %uint_72
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %1
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )");
    ASSERT_TRUE(program.IsValid()) << program.Diagnostics().str();
    auto* helper = program.AST().Functions().Find(program.Symbols().Get("helper"));
    ASSERT_NE(helper, nullptr);
    ASSERT_GT(helper->body->statements.Length(), 0u);
    auto* call = helper->body->statements[0]->As<ast::CallStatement>();
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->expr->args.Length(), 0u);
    auto* sem_call = program.Sem().Get<sem::Call>(call->expr);
    ASSERT_NE(sem_call, nullptr);
    auto* builtin = sem_call->Target()->As<sem::Builtin>();
    ASSERT_NE(builtin, nullptr);
    EXPECT_EQ(builtin->Type(), core::Function::kStorageBarrier);
}

TEST_F(SpirvASTParserTest, TextureBarrier) {
    auto program = ParseAndBuild(R"(
               OpName %helper "helper"
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
     %uint_1 = OpConstant %uint 1
  %uint_2056 = OpConstant %uint 2056
     %helper = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_2 %uint_2056
               OpReturn
               OpFunctionEnd
       %main = OpFunction %void None %1
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )");
    ASSERT_TRUE(program.IsValid()) << program.Diagnostics().str();
    auto* helper = program.AST().Functions().Find(program.Symbols().Get("helper"));
    ASSERT_NE(helper, nullptr);
    ASSERT_GT(helper->body->statements.Length(), 0u);
    auto* call = helper->body->statements[0]->As<ast::CallStatement>();
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->expr->args.Length(), 0u);
    auto* sem_call = program.Sem().Get<sem::Call>(call->expr);
    ASSERT_NE(sem_call, nullptr);
    auto* builtin = sem_call->Target()->As<sem::Builtin>();
    ASSERT_NE(builtin, nullptr);
    EXPECT_EQ(builtin->Type(), core::Function::kTextureBarrier);
}

TEST_F(SpirvASTParserTest, WorkgroupAndTextureAndStorageBarrier) {
    // Check that we emit multiple adjacent barrier calls when the flags
    // are combined.
    auto program = ParseAndBuild(R"(
               OpName %helper "helper"
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
   %uint_x948 = OpConstant %uint 0x948
     %helper = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_2 %uint_x948
               OpReturn
               OpFunctionEnd
     %main = OpFunction %void None %1
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )");
    ASSERT_TRUE(program.IsValid()) << program.Diagnostics().str();
    auto* helper = program.AST().Functions().Find(program.Symbols().Get("helper"));
    ASSERT_NE(helper, nullptr);
    ASSERT_GT(helper->body->statements.Length(), 2u);

    {
        auto* call = helper->body->statements[0]->As<ast::CallStatement>();
        ASSERT_NE(call, nullptr);
        EXPECT_EQ(call->expr->args.Length(), 0u);
        auto* sem_call = program.Sem().Get<sem::Call>(call->expr);
        ASSERT_NE(sem_call, nullptr);
        auto* builtin = sem_call->Target()->As<sem::Builtin>();
        ASSERT_NE(builtin, nullptr);
        EXPECT_EQ(builtin->Type(), core::Function::kWorkgroupBarrier);
    }
    {
        auto* call = helper->body->statements[1]->As<ast::CallStatement>();
        ASSERT_NE(call, nullptr);
        EXPECT_EQ(call->expr->args.Length(), 0u);
        auto* sem_call = program.Sem().Get<sem::Call>(call->expr);
        ASSERT_NE(sem_call, nullptr);
        auto* builtin = sem_call->Target()->As<sem::Builtin>();
        ASSERT_NE(builtin, nullptr);
        EXPECT_EQ(builtin->Type(), core::Function::kStorageBarrier);
    }
    {
        auto* call = helper->body->statements[2]->As<ast::CallStatement>();
        ASSERT_NE(call, nullptr);
        EXPECT_EQ(call->expr->args.Length(), 0u);
        auto* sem_call = program.Sem().Get<sem::Call>(call->expr);
        ASSERT_NE(sem_call, nullptr);
        auto* builtin = sem_call->Target()->As<sem::Builtin>();
        ASSERT_NE(builtin, nullptr);
        EXPECT_EQ(builtin->Type(), core::Function::kTextureBarrier);
    }
}

TEST_F(SpirvASTParserTest, ErrBarrierInvalidExecution) {
    auto program = ParseAndBuild(R"(
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
     %uint_2 = OpConstant %uint 2
   %uint_264 = OpConstant %uint 264
       %main = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_0 %uint_2 %uint_264
               OpReturn
               OpFunctionEnd
  )");
    EXPECT_FALSE(program.IsValid());
    EXPECT_THAT(program.Diagnostics().str(),
                HasSubstr("unsupported control barrier execution scope"));
}

TEST_F(SpirvASTParserTest, ErrBarrierSemanticsMissingAcquireRelease) {
    auto program = ParseAndBuild(R"(
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
     %uint_0 = OpConstant %uint 0
       %main = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_2 %uint_0
               OpReturn
               OpFunctionEnd
  )");
    EXPECT_FALSE(program.IsValid());
    EXPECT_THAT(program.Diagnostics().str(),
                HasSubstr("control barrier semantics requires acquire and release"));
}

TEST_F(SpirvASTParserTest, ErrBarrierInvalidSemantics) {
    auto program = ParseAndBuild(R"(
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
     %uint_9 = OpConstant %uint 9
       %main = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_2 %uint_9
               OpReturn
               OpFunctionEnd
  )");
    EXPECT_FALSE(program.IsValid());
    EXPECT_THAT(program.Diagnostics().str(), HasSubstr("unsupported control barrier semantics"));
}

TEST_F(SpirvASTParserTest, ErrWorkgroupBarrierInvalidMemory) {
    auto program = ParseAndBuild(R"(
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_2 = OpConstant %uint 2
     %uint_8 = OpConstant %uint 8
   %uint_264 = OpConstant %uint 264
       %main = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_8 %uint_264
               OpReturn
               OpFunctionEnd
  )");
    EXPECT_FALSE(program.IsValid());
    EXPECT_THAT(program.Diagnostics().str(),
                HasSubstr("workgroupBarrier requires workgroup memory scope"));
}

TEST_F(SpirvASTParserTest, ErrStorageBarrierInvalidMemory) {
    auto program = ParseAndBuild(R"(
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_1 = OpConstant %uint 1
     %uint_2 = OpConstant %uint 2
    %uint_72 = OpConstant %uint 72
       %main = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_1 %uint_72
               OpReturn
               OpFunctionEnd
  )");
    EXPECT_FALSE(program.IsValid());
    EXPECT_THAT(program.Diagnostics().str(),
                HasSubstr("storageBarrier requires workgroup memory scope"));
}

TEST_F(SpirvASTParserTest, ErrTextureBarrierInvalidMemory) {
    auto program = ParseAndBuild(R"(
       %void = OpTypeVoid
          %1 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
     %uint_1 = OpConstant %uint 1
     %uint_2 = OpConstant %uint 2
  %uint_2056 = OpConstant %uint 2056
       %main = OpFunction %void None %1
          %4 = OpLabel
               OpControlBarrier %uint_2 %uint_1 %uint_2056
               OpReturn
               OpFunctionEnd
  )");
    EXPECT_FALSE(program.IsValid());
    EXPECT_THAT(program.Diagnostics().str(),
                HasSubstr("textureBarrier requires workgroup memory scope"));
}

}  // namespace
}  // namespace tint::spirv::reader::ast_parser
