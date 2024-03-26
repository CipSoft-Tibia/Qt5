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

#include "src/tint/ast/test_helper.h"
#include "src/tint/reader/wgsl/parser_impl_test_helper.h"

namespace tint::reader::wgsl {
namespace {

TEST_F(ParserImplTest, ParamList_Single) {
    auto p = parser("a : i32");

    auto e = p->expect_param_list();
    ASSERT_FALSE(p->has_error()) << p->error();
    ASSERT_FALSE(e.errored);
    EXPECT_EQ(e.value.Length(), 1u);

    EXPECT_EQ(e.value[0]->name->symbol, p->builder().Symbols().Get("a"));
    ast::CheckIdentifier(p->builder().Symbols(), e.value[0]->type, "i32");
    EXPECT_TRUE(e.value[0]->Is<ast::Parameter>());

    ASSERT_EQ(e.value[0]->source.range.begin.line, 1u);
    ASSERT_EQ(e.value[0]->source.range.begin.column, 1u);
    ASSERT_EQ(e.value[0]->source.range.end.line, 1u);
    ASSERT_EQ(e.value[0]->source.range.end.column, 2u);
}

TEST_F(ParserImplTest, ParamList_Multiple) {
    auto p = parser("a : i32, b: f32, c: vec2<f32>");

    auto e = p->expect_param_list();
    ASSERT_FALSE(p->has_error()) << p->error();
    ASSERT_FALSE(e.errored);
    EXPECT_EQ(e.value.Length(), 3u);

    EXPECT_EQ(e.value[0]->name->symbol, p->builder().Symbols().Get("a"));
    ast::CheckIdentifier(p->builder().Symbols(), e.value[0]->type, "i32");
    EXPECT_TRUE(e.value[0]->Is<ast::Parameter>());

    ASSERT_EQ(e.value[0]->source.range.begin.line, 1u);
    ASSERT_EQ(e.value[0]->source.range.begin.column, 1u);
    ASSERT_EQ(e.value[0]->source.range.end.line, 1u);
    ASSERT_EQ(e.value[0]->source.range.end.column, 2u);

    EXPECT_EQ(e.value[1]->name->symbol, p->builder().Symbols().Get("b"));
    ast::CheckIdentifier(p->builder().Symbols(), e.value[1]->type, "f32");
    EXPECT_TRUE(e.value[1]->Is<ast::Parameter>());

    ASSERT_EQ(e.value[1]->source.range.begin.line, 1u);
    ASSERT_EQ(e.value[1]->source.range.begin.column, 10u);
    ASSERT_EQ(e.value[1]->source.range.end.line, 1u);
    ASSERT_EQ(e.value[1]->source.range.end.column, 11u);

    EXPECT_EQ(e.value[2]->name->symbol, p->builder().Symbols().Get("c"));
    ast::CheckIdentifier(p->builder().Symbols(), e.value[2]->type, ast::Template("vec2", "f32"));
    EXPECT_TRUE(e.value[2]->Is<ast::Parameter>());

    ASSERT_EQ(e.value[2]->source.range.begin.line, 1u);
    ASSERT_EQ(e.value[2]->source.range.begin.column, 18u);
    ASSERT_EQ(e.value[2]->source.range.end.line, 1u);
    ASSERT_EQ(e.value[2]->source.range.end.column, 19u);
}

TEST_F(ParserImplTest, ParamList_Empty) {
    auto p = parser("");
    auto e = p->expect_param_list();
    ASSERT_FALSE(p->has_error());
    ASSERT_FALSE(e.errored);
    EXPECT_EQ(e.value.Length(), 0u);
}

TEST_F(ParserImplTest, ParamList_TrailingComma) {
    auto p = parser("a : i32,");
    auto e = p->expect_param_list();
    ASSERT_FALSE(p->has_error());
    ASSERT_FALSE(e.errored);
    EXPECT_EQ(e.value.Length(), 1u);
}

TEST_F(ParserImplTest, ParamList_Attributes) {
    auto p = parser("@builtin(position) coord : vec4<f32>, @location(1) loc1 : f32");

    auto e = p->expect_param_list();
    ASSERT_FALSE(p->has_error()) << p->error();
    ASSERT_FALSE(e.errored);
    ASSERT_EQ(e.value.Length(), 2u);

    EXPECT_EQ(e.value[0]->name->symbol, p->builder().Symbols().Get("coord"));
    ast::CheckIdentifier(p->builder().Symbols(), e.value[0]->type, ast::Template("vec4", "f32"));
    EXPECT_TRUE(e.value[0]->Is<ast::Parameter>());
    auto attrs_0 = e.value[0]->attributes;
    ASSERT_EQ(attrs_0.Length(), 1u);
    EXPECT_TRUE(attrs_0[0]->Is<ast::BuiltinAttribute>());
    ast::CheckIdentifier(p->builder().Symbols(), attrs_0[0]->As<ast::BuiltinAttribute>()->builtin,
                         "position");

    ASSERT_EQ(e.value[0]->source.range.begin.line, 1u);
    ASSERT_EQ(e.value[0]->source.range.begin.column, 20u);
    ASSERT_EQ(e.value[0]->source.range.end.line, 1u);
    ASSERT_EQ(e.value[0]->source.range.end.column, 25u);

    EXPECT_EQ(e.value[1]->name->symbol, p->builder().Symbols().Get("loc1"));
    ast::CheckIdentifier(p->builder().Symbols(), e.value[1]->type, "f32");
    EXPECT_TRUE(e.value[1]->Is<ast::Parameter>());
    auto attrs_1 = e.value[1]->attributes;
    ASSERT_EQ(attrs_1.Length(), 1u);

    ASSERT_TRUE(attrs_1[0]->Is<ast::LocationAttribute>());
    auto* attr = attrs_1[0]->As<ast::LocationAttribute>();
    ASSERT_TRUE(attr->expr->Is<ast::IntLiteralExpression>());
    auto* loc = attr->expr->As<ast::IntLiteralExpression>();
    EXPECT_EQ(loc->value, 1u);

    EXPECT_EQ(e.value[1]->source.range.begin.line, 1u);
    EXPECT_EQ(e.value[1]->source.range.begin.column, 52u);
    EXPECT_EQ(e.value[1]->source.range.end.line, 1u);
    EXPECT_EQ(e.value[1]->source.range.end.column, 56u);
}

}  // namespace
}  // namespace tint::reader::wgsl
