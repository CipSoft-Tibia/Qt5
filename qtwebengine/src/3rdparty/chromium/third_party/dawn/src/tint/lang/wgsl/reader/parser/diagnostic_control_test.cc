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

#include "src/tint/lang/wgsl/reader/parser/helper_test.h"

#include "src/tint/lang/wgsl/ast/diagnostic_control.h"
#include "src/tint/lang/wgsl/ast/helper_test.h"

namespace tint::wgsl::reader {
namespace {

using SeverityPair = std::pair<std::string, core::DiagnosticSeverity>;
class DiagnosticControlParserTest : public WGSLParserTestWithParam<SeverityPair> {};

TEST_P(DiagnosticControlParserTest, DiagnosticControl_Name) {
    auto& params = GetParam();
    auto p = parser("(" + params.first + ", foo)");
    auto e = p->expect_diagnostic_control();
    EXPECT_FALSE(e.errored);
    EXPECT_FALSE(p->has_error()) << p->error();
    EXPECT_EQ(e->severity, params.second);

    auto* r = e->rule_name;
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->category, nullptr);
    ast::CheckIdentifier(r->name, "foo");
}
TEST_P(DiagnosticControlParserTest, DiagnosticControl_CategoryAndName) {
    auto& params = GetParam();
    auto p = parser("(" + params.first + ", foo.bar)");
    auto e = p->expect_diagnostic_control();
    EXPECT_FALSE(e.errored);
    EXPECT_FALSE(p->has_error()) << p->error();
    EXPECT_EQ(e->severity, params.second);

    auto* r = e->rule_name;
    ASSERT_NE(r, nullptr);
    ast::CheckIdentifier(r->category, "foo");
    ast::CheckIdentifier(r->name, "bar");
}
INSTANTIATE_TEST_SUITE_P(DiagnosticControlParserTest,
                         DiagnosticControlParserTest,
                         testing::Values(SeverityPair{"error", core::DiagnosticSeverity::kError},
                                         SeverityPair{"warning",
                                                      core::DiagnosticSeverity::kWarning},
                                         SeverityPair{"info", core::DiagnosticSeverity::kInfo},
                                         SeverityPair{"off", core::DiagnosticSeverity::kOff}));

TEST_F(WGSLParserTest, DiagnosticControl_Name_TrailingComma) {
    auto p = parser("(error, foo,)");
    auto e = p->expect_diagnostic_control();
    EXPECT_FALSE(e.errored);
    EXPECT_FALSE(p->has_error()) << p->error();
    EXPECT_EQ(e->severity, core::DiagnosticSeverity::kError);

    auto* r = e->rule_name;
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->category, nullptr);
    ast::CheckIdentifier(r->name, "foo");
}

TEST_F(WGSLParserTest, DiagnosticControl_CategoryAndName_TrailingComma) {
    auto p = parser("(error, foo.bar,)");
    auto e = p->expect_diagnostic_control();
    EXPECT_FALSE(e.errored);
    EXPECT_FALSE(p->has_error()) << p->error();
    EXPECT_EQ(e->severity, core::DiagnosticSeverity::kError);

    auto* r = e->rule_name;
    ASSERT_NE(r, nullptr);
    ast::CheckIdentifier(r->category, "foo");
    ast::CheckIdentifier(r->name, "bar");
}

TEST_F(WGSLParserTest, DiagnosticControl_MissingOpenParen) {
    auto p = parser("off, foo)");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:1: expected '(' for diagnostic control)");
}

TEST_F(WGSLParserTest, DiagnosticControl_MissingCloseParen) {
    auto p = parser("(off, foo");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:10: expected ')' for diagnostic control)");
}

TEST_F(WGSLParserTest, DiagnosticControl_MissingDiagnosticSeverity) {
    auto p = parser("(, foo");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:2: expected severity control
Possible values: 'error', 'info', 'off', 'warning')");
}

TEST_F(WGSLParserTest, DiagnosticControl_InvalidDiagnosticSeverity) {
    auto p = parser("(fatal, foo)");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:2: expected severity control
Possible values: 'error', 'info', 'off', 'warning')");
}

TEST_F(WGSLParserTest, DiagnosticControl_MissingComma) {
    auto p = parser("(off foo");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:6: expected ',' for diagnostic control)");
}

TEST_F(WGSLParserTest, DiagnosticControl_MissingRuleName) {
    auto p = parser("(off,)");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:6: expected diagnostic rule name)");
}

TEST_F(WGSLParserTest, DiagnosticControl_MissingRuleCategory) {
    auto p = parser("(off,for.foo)");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:6: expected diagnostic rule category)");
}

TEST_F(WGSLParserTest, DiagnosticControl_InvalidRuleName) {
    auto p = parser("(off, foo$bar)");
    auto e = p->expect_diagnostic_control();
    EXPECT_TRUE(e.errored);
    EXPECT_TRUE(p->has_error());
    EXPECT_EQ(p->error(), R"(1:10: invalid character found)");
}

}  // namespace
}  // namespace tint::wgsl::reader
