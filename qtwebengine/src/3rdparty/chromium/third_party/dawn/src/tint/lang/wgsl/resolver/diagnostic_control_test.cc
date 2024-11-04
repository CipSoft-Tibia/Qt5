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

#include "src/tint/lang/wgsl/resolver/resolver.h"

#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::resolver {
namespace {

using ResolverDiagnosticControlTest = ResolverTest;

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_DefaultSeverity) {
    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(warning: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_ErrorViaDirective) {
    DiagnosticDirective(core::DiagnosticSeverity::kError, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts);

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), R"(error: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_WarningViaDirective) {
    DiagnosticDirective(core::DiagnosticSeverity::kWarning, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(warning: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_InfoViaDirective) {
    DiagnosticDirective(core::DiagnosticSeverity::kInfo, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(note: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_OffViaDirective) {
    DiagnosticDirective(core::DiagnosticSeverity::kOff, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts);

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_TRUE(r()->error().empty());
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_ErrorViaAttribute) {
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kError, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts, Vector{attr});

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), R"(error: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_WarningViaAttribute) {
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kWarning, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts, Vector{attr});

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(warning: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_InfoViaAttribute) {
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kInfo, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts, Vector{attr});

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(note: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_OffViaAttribute) {
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kOff, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts, Vector{attr});

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_TRUE(r()->error().empty());
}

TEST_F(ResolverDiagnosticControlTest, UnreachableCode_ErrorViaDirective_OverriddenViaAttribute) {
    // diagnostic(error, chromium.unreachable_code);
    //
    // @diagnostic(off, chromium.unreachable_code) fn foo() {
    //   return;
    //   return; // Should produce a warning
    // }
    DiagnosticDirective(core::DiagnosticSeverity::kError, "chromium", "unreachable_code");
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kWarning, "chromium", "unreachable_code");

    auto stmts = Vector{Return(), Return()};
    Func("foo", {}, ty.void_(), stmts, Vector{attr});

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(warning: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, FunctionAttributeScope) {
    // @diagnostic(off, chromium.unreachable_code) fn foo() {
    //   return;
    //   return; // Should not produce a diagnostic
    // }
    //
    // fn zoo() {
    //   return;
    //   return; // Should produce a warning (default severity)
    // }
    //
    // @diagnostic(info, chromium.unreachable_code) fn bar() {
    //   return;
    //   return; // Should produce an info
    // }
    {
        auto* attr =
            DiagnosticAttribute(core::DiagnosticSeverity::kOff, "chromium", "unreachable_code");
        Func("foo", {}, ty.void_(),
             Vector{
                 Return(),
                 Return(Source{{12, 34}}),
             },
             Vector{attr});
    }
    {
        Func("bar", {}, ty.void_(),
             Vector{
                 Return(),
                 Return(Source{{45, 67}}),
             });
    }
    {
        auto* attr =
            DiagnosticAttribute(core::DiagnosticSeverity::kInfo, "chromium", "unreachable_code");
        Func("zoo", {}, ty.void_(),
             Vector{
                 Return(),
                 Return(Source{{89, 10}}),
             },
             Vector{attr});
    }

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(45:67 warning: code is unreachable
89:10 note: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, BlockAttributeScope) {
    // fn foo() @diagnostic(off, chromium.unreachable_code) {
    //   {
    //     return;
    //     return; // Should not produce a diagnostic
    //   }
    //   @diagnostic(warning, chromium.unreachable_code) {
    //     if (true) @diagnostic(info, chromium.unreachable_code) {
    //       return;
    //       return; // Should produce an info
    //     } else {
    //       while (true) @diagnostic(off, chromium.unreachable_code) {
    //         return;
    //         return; // Should not produce a diagnostic
    //       }
    //       return;
    //       return; // Should produce an warning
    //     }
    //   }
    // }

    auto attr = [&](auto severity) {
        return Vector{DiagnosticAttribute(severity, "chromium", "unreachable_code")};
    };
    Func("foo", {}, ty.void_(),
         Vector{
             Return(),
             Return(Source{{12, 21}}),
             Block(Vector{
                 Block(
                     Vector{
                         If(Expr(true),
                            Block(
                                Vector{
                                    Return(),
                                    Return(Source{{34, 43}}),
                                },
                                attr(core::DiagnosticSeverity::kInfo)),
                            Else(Block(Vector{
                                While(Expr(true), Block(
                                                      Vector{
                                                          Return(),
                                                          Return(Source{{56, 65}}),
                                                      },
                                                      attr(core::DiagnosticSeverity::kOff))),
                                Return(),
                                Return(Source{{78, 87}}),
                            }))),
                     },
                     attr(core::DiagnosticSeverity::kWarning)),
             }),
         },
         attr(core::DiagnosticSeverity::kOff));

    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), R"(34:43 note: code is unreachable
78:87 warning: code is unreachable)");
}

TEST_F(ResolverDiagnosticControlTest, UnrecognizedCoreRuleName_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{12, 34}}, "derivative_uniform"));
    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(),
              R"(12:34 warning: unrecognized diagnostic rule 'derivative_uniform'
Did you mean 'derivative_uniformity'?
Possible values: 'derivative_uniformity')");
}

TEST_F(ResolverDiagnosticControlTest, UnrecognizedCoreRuleName_Attribute) {
    auto* attr = DiagnosticAttribute(core::DiagnosticSeverity::kError,
                                     DiagnosticRuleName(Source{{12, 34}}, "derivative_uniform"));
    Func("foo", {}, ty.void_(), {}, Vector{attr});
    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(),
              R"(12:34 warning: unrecognized diagnostic rule 'derivative_uniform'
Did you mean 'derivative_uniformity'?
Possible values: 'derivative_uniformity')");
}

TEST_F(ResolverDiagnosticControlTest, UnrecognizedChromiumRuleName_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_cod"));
    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(),
              R"(12:34 warning: unrecognized diagnostic rule 'chromium.unreachable_cod'
Did you mean 'chromium.unreachable_code'?
Possible values: 'chromium.unreachable_code')");
}

TEST_F(ResolverDiagnosticControlTest, UnrecognizedChromiumRuleName_Attribute) {
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kError,
                            DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_cod"));
    Func("foo", {}, ty.void_(), {}, Vector{attr});
    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(),
              R"(12:34 warning: unrecognized diagnostic rule 'chromium.unreachable_cod'
Did you mean 'chromium.unreachable_code'?
Possible values: 'chromium.unreachable_code')");
}

TEST_F(ResolverDiagnosticControlTest, UnrecognizedOtherRuleName_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{12, 34}}, "unknown", "unreachable_cod"));
    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), "");
}

TEST_F(ResolverDiagnosticControlTest, UnrecognizedOtherRuleName_Attribute) {
    auto* attr =
        DiagnosticAttribute(core::DiagnosticSeverity::kError,
                            DiagnosticRuleName(Source{{12, 34}}, "unknown", "unreachable_cod"));
    Func("foo", {}, ty.void_(), {}, Vector{attr});
    EXPECT_TRUE(r()->Resolve()) << r()->error();
    EXPECT_EQ(r()->error(), "");
}

TEST_F(ResolverDiagnosticControlTest, Conflict_SameNameSameSeverity_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_code"));
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{56, 78}}, "chromium", "unreachable_code"));
    EXPECT_TRUE(r()->Resolve()) << r()->error();
}

TEST_F(ResolverDiagnosticControlTest, Conflict_SameNameDifferentSeverity_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_code"));
    DiagnosticDirective(core::DiagnosticSeverity::kOff,
                        DiagnosticRuleName(Source{{56, 78}}, "chromium", "unreachable_code"));
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(56:78 error: conflicting diagnostic directive
12:34 note: severity of 'chromium.unreachable_code' set to 'off' here)");
}

TEST_F(ResolverDiagnosticControlTest, Conflict_SameUnknownNameDifferentSeverity_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError,
                        DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_codes"));
    DiagnosticDirective(core::DiagnosticSeverity::kOff,
                        DiagnosticRuleName(Source{{56, 78}}, "chromium", "unreachable_codes"));
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(12:34 warning: unrecognized diagnostic rule 'chromium.unreachable_codes'
Did you mean 'chromium.unreachable_code'?
Possible values: 'chromium.unreachable_code'
56:78 warning: unrecognized diagnostic rule 'chromium.unreachable_codes'
Did you mean 'chromium.unreachable_code'?
Possible values: 'chromium.unreachable_code'
56:78 error: conflicting diagnostic directive
12:34 note: severity of 'chromium.unreachable_codes' set to 'off' here)");
}

TEST_F(ResolverDiagnosticControlTest, Conflict_DifferentUnknownNameDifferentSeverity_Directive) {
    DiagnosticDirective(core::DiagnosticSeverity::kError, "chromium", "unreachable_codes");
    DiagnosticDirective(core::DiagnosticSeverity::kOff, "chromium", "unreachable_codex");
    EXPECT_TRUE(r()->Resolve()) << r()->error();
}

TEST_F(ResolverDiagnosticControlTest, Conflict_SameNameSameSeverity_Attribute) {
    auto* attr1 =
        DiagnosticAttribute(core::DiagnosticSeverity::kError,
                            DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_code"));
    auto* attr2 =
        DiagnosticAttribute(core::DiagnosticSeverity::kError,
                            DiagnosticRuleName(Source{{56, 78}}, "chromium", "unreachable_code"));
    Func("foo", {}, ty.void_(), {}, Vector{attr1, attr2});
    EXPECT_TRUE(r()->Resolve()) << r()->error();
}

TEST_F(ResolverDiagnosticControlTest, Conflict_SameNameDifferentSeverity_Attribute) {
    auto* attr1 =
        DiagnosticAttribute(core::DiagnosticSeverity::kError,
                            DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_code"));
    auto* attr2 =
        DiagnosticAttribute(core::DiagnosticSeverity::kOff,
                            DiagnosticRuleName(Source{{56, 78}}, "chromium", "unreachable_code"));
    Func("foo", {}, ty.void_(), {}, Vector{attr1, attr2});
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(56:78 error: conflicting diagnostic attribute
12:34 note: severity of 'chromium.unreachable_code' set to 'off' here)");
}

TEST_F(ResolverDiagnosticControlTest, Conflict_SameUnknownNameDifferentSeverity_Attribute) {
    auto* attr1 =
        DiagnosticAttribute(core::DiagnosticSeverity::kError,
                            DiagnosticRuleName(Source{{12, 34}}, "chromium", "unreachable_codes"));
    auto* attr2 =
        DiagnosticAttribute(core::DiagnosticSeverity::kOff,
                            DiagnosticRuleName(Source{{56, 78}}, "chromium", "unreachable_codes"));
    Func("foo", {}, ty.void_(), {}, Vector{attr1, attr2});
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(12:34 warning: unrecognized diagnostic rule 'chromium.unreachable_codes'
Did you mean 'chromium.unreachable_code'?
Possible values: 'chromium.unreachable_code'
56:78 warning: unrecognized diagnostic rule 'chromium.unreachable_codes'
Did you mean 'chromium.unreachable_code'?
Possible values: 'chromium.unreachable_code'
56:78 error: conflicting diagnostic attribute
12:34 note: severity of 'chromium.unreachable_codes' set to 'off' here)");
}

TEST_F(ResolverDiagnosticControlTest, Conflict_DifferentUnknownNameDifferentSeverity_Attribute) {
    auto* attr1 =
        DiagnosticAttribute(core::DiagnosticSeverity::kError, "chromium", "unreachable_codes");
    auto* attr2 =
        DiagnosticAttribute(core::DiagnosticSeverity::kOff, "chromium", "unreachable_codex");
    Func("foo", {}, ty.void_(), {}, Vector{attr1, attr2});
    EXPECT_TRUE(r()->Resolve()) << r()->error();
}

}  // namespace
}  // namespace tint::resolver
