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

#ifndef SRC_TINT_AST_TEST_HELPER_H_
#define SRC_TINT_AST_TEST_HELPER_H_

#include <tuple>
#include <utility>

#include "gtest/gtest.h"
#include "src/tint/program_builder.h"

namespace tint::ast {

/// Helper base class for testing
template <typename BASE>
class TestHelperBase : public BASE, public ProgramBuilder {};

/// Helper class for testing that derives from testing::Test.
using TestHelper = TestHelperBase<testing::Test>;

/// Helper class for testing that derives from `T`.
template <typename T>
using TestParamHelper = TestHelperBase<testing::TestWithParam<T>>;

/// A structure to hold a TemplatedIdentifier matcher, used by the CheckIdentifier() test helper
template <typename... ARGS>
struct TemplatedIdentifierMatcher {
    /// The expected name of the TemplatedIdentifier
    std::string_view name;
    /// The expected arguments of the TemplatedIdentifier
    std::tuple<ARGS...> args;
};

/// Deduction guide for TemplatedIdentifierMatcher
template <typename... ARGS>
TemplatedIdentifierMatcher(std::string_view, std::tuple<ARGS...>&&)
    -> TemplatedIdentifierMatcher<ARGS...>;

/// A helper function for building a TemplatedIdentifierMatcher
/// @param name the name of the TemplatedIdentifier
/// @param args the template arguments
/// @return a TemplatedIdentifierMatcher
template <typename... ARGS>
auto Template(std::string_view name, ARGS&&... args) {
    return TemplatedIdentifierMatcher{name, std::make_tuple(std::forward<ARGS>(args)...)};
}

/// A traits helper for determining whether the type T is a TemplatedIdentifierMatcher.
template <typename T>
struct IsTemplatedIdentifierMatcher {
    /// True iff T is a TemplatedIdentifierMatcher
    static constexpr bool value = false;
};

/// IsTemplatedIdentifierMatcher specialization for TemplatedIdentifierMatcher.
template <typename... ARGS>
struct IsTemplatedIdentifierMatcher<TemplatedIdentifierMatcher<ARGS...>> {
    /// True iff T is a TemplatedIdentifierMatcher
    static constexpr bool value = true;
};

/// A testing utility for checking that an Identifier matches the expected values.
/// @param symbols the symbol table
/// @param got the identifier
/// @param expected the expected identifier name
template <typename... ARGS>
void CheckIdentifier(const SymbolTable& symbols, const Identifier* got, std::string_view expected) {
    EXPECT_FALSE(got->Is<TemplatedIdentifier>());
    EXPECT_EQ(symbols.NameFor(got->symbol), expected);
}

/// A testing utility for checking that an Identifier matches the expected name and template
/// arguments.
/// @param symbols the symbol table
/// @param ident the identifier
/// @param expected the expected identifier name and arguments
template <typename... ARGS>
void CheckIdentifier(const SymbolTable& symbols,
                     const Identifier* ident,
                     const TemplatedIdentifierMatcher<ARGS...>& expected) {
    EXPECT_EQ(symbols.NameFor(ident->symbol), expected.name);
    ASSERT_TRUE(ident->Is<TemplatedIdentifier>());
    auto* got = ident->As<TemplatedIdentifier>();
    ASSERT_EQ(got->arguments.Length(), std::tuple_size_v<decltype(expected.args)>);

    size_t arg_idx = 0;
    auto check_arg = [&](auto&& expected_arg) {
        const auto* got_arg = got->arguments[arg_idx++];

        using T = std::decay_t<decltype(expected_arg)>;
        if constexpr (traits::IsStringLike<T>) {
            ASSERT_TRUE(got_arg->Is<IdentifierExpression>());
            CheckIdentifier(symbols, got_arg->As<IdentifierExpression>()->identifier, expected_arg);
        } else if constexpr (IsTemplatedIdentifierMatcher<T>::value) {
            ASSERT_TRUE(got_arg->Is<IdentifierExpression>());
            auto* got_ident = got_arg->As<IdentifierExpression>()->identifier;
            ASSERT_TRUE(got_ident->Is<TemplatedIdentifier>());
            CheckIdentifier(symbols, got_ident->As<TemplatedIdentifier>(), expected_arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            ASSERT_TRUE(got_arg->Is<BoolLiteralExpression>());
            EXPECT_EQ(got_arg->As<BoolLiteralExpression>()->value, expected_arg);
        } else if constexpr (std::is_same_v<T, AInt>) {
            ASSERT_TRUE(got_arg->Is<IntLiteralExpression>());
            EXPECT_EQ(got_arg->As<IntLiteralExpression>()->suffix,
                      IntLiteralExpression::Suffix::kNone);
            EXPECT_EQ(AInt(got_arg->As<IntLiteralExpression>()->value), expected_arg);
        } else if constexpr (std::is_same_v<T, i32>) {
            ASSERT_TRUE(got_arg->Is<IntLiteralExpression>());
            EXPECT_EQ(got_arg->As<IntLiteralExpression>()->suffix,
                      IntLiteralExpression::Suffix::kI);
            EXPECT_EQ(i32(got_arg->As<IntLiteralExpression>()->value), expected_arg);
        } else if constexpr (std::is_same_v<T, u32>) {
            ASSERT_TRUE(got_arg->Is<IntLiteralExpression>());
            EXPECT_EQ(got_arg->As<IntLiteralExpression>()->suffix,
                      IntLiteralExpression::Suffix::kU);
            EXPECT_EQ(u32(got_arg->As<IntLiteralExpression>()->value), expected_arg);
        } else if constexpr (std::is_same_v<T, AFloat>) {
            ASSERT_TRUE(got_arg->Is<FloatLiteralExpression>());
            EXPECT_EQ(got_arg->As<FloatLiteralExpression>()->suffix,
                      FloatLiteralExpression::Suffix::kNone);
            EXPECT_EQ(AFloat(got_arg->As<FloatLiteralExpression>()->value), expected_arg);
        } else if constexpr (std::is_same_v<T, f32>) {
            ASSERT_TRUE(got_arg->Is<FloatLiteralExpression>());
            EXPECT_EQ(got_arg->As<FloatLiteralExpression>()->suffix,
                      FloatLiteralExpression::Suffix::kF);
            EXPECT_EQ(f32(got_arg->As<FloatLiteralExpression>()->value), expected_arg);
        } else if constexpr (std::is_same_v<T, f16>) {
            ASSERT_TRUE(got_arg->Is<FloatLiteralExpression>());
            EXPECT_EQ(got_arg->As<FloatLiteralExpression>()->suffix,
                      FloatLiteralExpression::Suffix::kH);
            EXPECT_EQ(f16(got_arg->As<FloatLiteralExpression>()->value), expected_arg);
        } else {
            FAIL() << "unhandled expected_args type";
        }
    };
    std::apply([&](auto&&... args) { ((check_arg(args)), ...); }, expected.args);
}

/// A testing utility for checking that an IdentifierExpression matches the expected values.
/// @param symbols the symbol table
/// @param expr the IdentifierExpression
/// @param expected the expected identifier name
template <typename... ARGS>
void CheckIdentifier(const SymbolTable& symbols,
                     const Expression* expr,
                     std::string_view expected) {
    auto* expr_ident = expr->As<IdentifierExpression>();
    ASSERT_NE(expr_ident, nullptr) << "expression is not a IdentifierExpression";
    CheckIdentifier(symbols, expr_ident->identifier, expected);
}

/// A testing utility for checking that an IdentifierExpression matches the expected name and
/// template arguments.
/// @param symbols the symbol table
/// @param expr the IdentifierExpression
/// @param expected the expected identifier name and arguments
template <typename... ARGS>
void CheckIdentifier(const SymbolTable& symbols,
                     const Expression* expr,
                     const TemplatedIdentifierMatcher<ARGS...>& expected) {
    auto* expr_ident = expr->As<IdentifierExpression>();
    ASSERT_NE(expr_ident, nullptr) << "expression is not a IdentifierExpression";
    CheckIdentifier(symbols, expr_ident->identifier, expected);
}

}  // namespace tint::ast

#endif  // SRC_TINT_AST_TEST_HELPER_H_
