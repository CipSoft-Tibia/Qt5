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

TEST_F(WGSLParserTest, DiagnosticAttribute_Name) {
    auto p = parser("diagnostic(off, foo)");
    auto a = p->attribute();
    EXPECT_FALSE(p->has_error()) << p->error();
    EXPECT_TRUE(a.matched);
    auto* d = a.value->As<ast::DiagnosticAttribute>();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->control.severity, core::DiagnosticSeverity::kOff);
    auto* r = d->control.rule_name;
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->category, nullptr);
    ast::CheckIdentifier(r->name, "foo");
}

TEST_F(WGSLParserTest, DiagnosticAttribute_CategoryName) {
    auto p = parser("diagnostic(off, foo.bar)");
    auto a = p->attribute();
    EXPECT_FALSE(p->has_error()) << p->error();
    EXPECT_TRUE(a.matched);
    auto* d = a.value->As<ast::DiagnosticAttribute>();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->control.severity, core::DiagnosticSeverity::kOff);
    auto* r = d->control.rule_name;
    ASSERT_NE(r, nullptr);
    ast::CheckIdentifier(r->category, "foo");
    ast::CheckIdentifier(r->name, "bar");
}

}  // namespace
}  // namespace tint::wgsl::reader
