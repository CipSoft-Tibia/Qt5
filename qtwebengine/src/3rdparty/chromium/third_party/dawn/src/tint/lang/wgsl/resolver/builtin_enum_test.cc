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

#include "src/tint/lang/core/access.h"
#include "src/tint/lang/core/address_space.h"
#include "src/tint/lang/core/builtin_value.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/interpolation_sampling.h"
#include "src/tint/lang/core/interpolation_type.h"
#include "src/tint/lang/core/texel_format.h"
#include "src/tint/lang/wgsl/resolver/resolver.h"
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

#include "gmock/gmock.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::resolver {
namespace {

////////////////////////////////////////////////////////////////////////////////
// access
////////////////////////////////////////////////////////////////////////////////
using ResolverAccessUsedWithTemplateArgs = ResolverTestWithParam<const char*>;

TEST_P(ResolverAccessUsedWithTemplateArgs, Test) {
    // @group(0) @binding(0) var t : texture_storage_2d<rgba8unorm, ACCESS<T>>;
    auto* tmpl = Ident(Source{{12, 34}}, GetParam(), "T");
    GlobalVar("v", ty("texture_storage_2d", "rgba8unorm", tmpl), Group(0_u), Binding(0_u));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), "12:34 error: access '" + std::string(GetParam()) +
                                "' does not take template arguments");
}

INSTANTIATE_TEST_SUITE_P(,
                         ResolverAccessUsedWithTemplateArgs,
                         testing::ValuesIn(core::kAccessStrings));

////////////////////////////////////////////////////////////////////////////////
// address space
////////////////////////////////////////////////////////////////////////////////
using ResolverAddressSpaceUsedWithTemplateArgs = ResolverTestWithParam<const char*>;

TEST_P(ResolverAddressSpaceUsedWithTemplateArgs, Test) {
    // fn f(p : ptr<ADDRESS_SPACE<T>, f32) {}

    Enable(core::Extension::kChromiumExperimentalFullPtrParameters);
    auto* tmpl = Ident(Source{{12, 34}}, GetParam(), "T");
    Func("f", Vector{Param("p", ty("ptr", tmpl, ty.f32()))}, ty.void_(), tint::Empty);
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), "12:34 error: address space '" + std::string(GetParam()) +
                                "' does not take template arguments");
}

INSTANTIATE_TEST_SUITE_P(,
                         ResolverAddressSpaceUsedWithTemplateArgs,
                         testing::ValuesIn(core::kAddressSpaceStrings));

////////////////////////////////////////////////////////////////////////////////
// builtin value
////////////////////////////////////////////////////////////////////////////////
using ResolverBuiltinValueUsedWithTemplateArgs = ResolverTestWithParam<const char*>;

TEST_P(ResolverBuiltinValueUsedWithTemplateArgs, Test) {
    // fn f(@builtin(BUILTIN<T>) p : vec4<f32>) {}
    auto* tmpl = Ident(Source{{12, 34}}, GetParam(), "T");
    Func("f", Vector{Param("p", ty.vec4<f32>(), Vector{Builtin(tmpl)})}, ty.void_(), tint::Empty,
         Vector{Stage(ast::PipelineStage::kFragment)});
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), "12:34 error: builtin value '" + std::string(GetParam()) +
                                "' does not take template arguments");
}

INSTANTIATE_TEST_SUITE_P(,
                         ResolverBuiltinValueUsedWithTemplateArgs,
                         testing::ValuesIn(core::kBuiltinValueStrings));

////////////////////////////////////////////////////////////////////////////////
// interpolation sampling
////////////////////////////////////////////////////////////////////////////////
using ResolverInterpolationSamplingUsedWithTemplateArgs = ResolverTestWithParam<const char*>;

TEST_P(ResolverInterpolationSamplingUsedWithTemplateArgs, Test) {
    // @fragment
    // fn f(@location(0) @interpolate(linear, INTERPOLATION_SAMPLING<T>) p : vec4<f32>) {}
    auto* tmpl = Ident(Source{{12, 34}}, GetParam(), "T");
    Func("f",
         Vector{Param("p", ty.vec4<f32>(),
                      Vector{
                          Location(0_a),
                          Interpolate(core::InterpolationType::kLinear, tmpl),
                      })},
         ty.void_(), tint::Empty,
         Vector{
             Stage(ast::PipelineStage::kFragment),
         });
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), "12:34 error: interpolation sampling '" + std::string(GetParam()) +
                                "' does not take template arguments");
}

INSTANTIATE_TEST_SUITE_P(,
                         ResolverInterpolationSamplingUsedWithTemplateArgs,
                         testing::ValuesIn(core::kInterpolationSamplingStrings));

////////////////////////////////////////////////////////////////////////////////
// interpolation type
////////////////////////////////////////////////////////////////////////////////
using ResolverInterpolationTypeUsedWithTemplateArgs = ResolverTestWithParam<const char*>;

TEST_P(ResolverInterpolationTypeUsedWithTemplateArgs, Test) {
    // @fragment
    // fn f(@location(0) @interpolate(INTERPOLATION_TYPE<T>, center) p : vec4<f32>) {}
    auto* tmpl = Ident(Source{{12, 34}}, GetParam(), "T");
    Func("f",
         Vector{Param("p", ty.vec4<f32>(),
                      Vector{
                          Location(0_a),
                          Interpolate(tmpl, core::InterpolationSampling::kCenter),
                      })},
         ty.void_(), tint::Empty,
         Vector{
             Stage(ast::PipelineStage::kFragment),
         });
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), "12:34 error: interpolation type '" + std::string(GetParam()) +
                                "' does not take template arguments");
}

INSTANTIATE_TEST_SUITE_P(,
                         ResolverInterpolationTypeUsedWithTemplateArgs,
                         testing::ValuesIn(core::kInterpolationTypeStrings));

////////////////////////////////////////////////////////////////////////////////
// texel format
////////////////////////////////////////////////////////////////////////////////
using ResolverTexelFormatUsedWithTemplateArgs = ResolverTestWithParam<const char*>;

TEST_P(ResolverTexelFormatUsedWithTemplateArgs, Test) {
    // @group(0) @binding(0) var t : texture_storage_2d<TEXEL_FORMAT<T>, write>
    auto* tmpl = Ident(Source{{12, 34}}, GetParam(), "T");
    GlobalVar("t", ty("texture_storage_2d", ty(tmpl), "write"), Group(0_u), Binding(0_u));
    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), "12:34 error: texel format '" + std::string(GetParam()) +
                                "' does not take template arguments");
}

INSTANTIATE_TEST_SUITE_P(,
                         ResolverTexelFormatUsedWithTemplateArgs,
                         testing::ValuesIn(core::kTexelFormatStrings));

}  // namespace
}  // namespace tint::resolver
