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

////////////////////////////////////////////////////////////////////////////////
// File generated by 'tools/src/cmd/gen' using the template:
//   src/tint/lang/core/extension_test.cc.tmpl
//
// To regenerate run: './tools/run gen'
//
//                       Do not modify this file directly
////////////////////////////////////////////////////////////////////////////////

#include "src/tint/lang/core/extension.h"

#include <gtest/gtest.h>

#include <string>

#include "src/tint/utils/text/string.h"

namespace tint::core {
namespace {

namespace parse_print_tests {

struct Case {
    const char* string;
    Extension value;
};

inline std::ostream& operator<<(std::ostream& out, Case c) {
    return out << "'" << std::string(c.string) << "'";
}

static constexpr Case kValidCases[] = {
    {"chromium_disable_uniformity_analysis", Extension::kChromiumDisableUniformityAnalysis},
    {"chromium_experimental_dp4a", Extension::kChromiumExperimentalDp4A},
    {"chromium_experimental_full_ptr_parameters",
     Extension::kChromiumExperimentalFullPtrParameters},
    {"chromium_experimental_push_constant", Extension::kChromiumExperimentalPushConstant},
    {"chromium_experimental_read_write_storage_texture",
     Extension::kChromiumExperimentalReadWriteStorageTexture},
    {"chromium_experimental_subgroups", Extension::kChromiumExperimentalSubgroups},
    {"chromium_internal_dual_source_blending", Extension::kChromiumInternalDualSourceBlending},
    {"chromium_internal_relaxed_uniform_layout", Extension::kChromiumInternalRelaxedUniformLayout},
    {"f16", Extension::kF16},
};

static constexpr Case kInvalidCases[] = {
    {"chromium_disableuniformiccy_analysis", Extension::kUndefined},
    {"chromil3_disable_unifority_analss", Extension::kUndefined},
    {"chromium_disable_Vniformity_analysis", Extension::kUndefined},
    {"chro1ium_experimental_dp4a", Extension::kUndefined},
    {"chrJmium_experiqqetal_dp4a", Extension::kUndefined},
    {"chromium_experimenll77l_dp4a", Extension::kUndefined},
    {"chroium_experimental_full_ptr_paqqppmetHHrs", Extension::kUndefined},
    {"chrium_evperiental_full_ptr_paraceters", Extension::kUndefined},
    {"chromium_expGimental_fullbptr_parameters", Extension::kUndefined},
    {"chvomium_experimental_push_constiint", Extension::kUndefined},
    {"chromiu8WWexperimental_push_constant", Extension::kUndefined},
    {"chromium_experiMental_push_costanxx", Extension::kUndefined},
    {"chromum_experimental_read_write_stggXage_texture", Extension::kUndefined},
    {"chromium_exVerimentl_rXad_writu_storge_texture", Extension::kUndefined},
    {"chromium_experimental3read_write_storage_texture", Extension::kUndefined},
    {"cEromium_experimental_subgroups", Extension::kUndefined},
    {"TThromium_experiPPental_sugroups", Extension::kUndefined},
    {"chddomium_experimental_subgroxxs", Extension::kUndefined},
    {"chromium_internal_44ual_source_blending", Extension::kUndefined},
    {"chromium_inteSSnal_dual_source_blendinVV", Extension::kUndefined},
    {"chromiuR_interna22dual_source_blenRing", Extension::kUndefined},
    {"chromium_int9rnal_relaxed_Fnifor_layout", Extension::kUndefined},
    {"chrmium_internal_relaxed_uniform_layout", Extension::kUndefined},
    {"VRhHomium_internal_relaxd_uniform_OOayout", Extension::kUndefined},
    {"y1", Extension::kUndefined},
    {"l77rrn6", Extension::kUndefined},
    {"4016", Extension::kUndefined},
};

using ExtensionParseTest = testing::TestWithParam<Case>;

TEST_P(ExtensionParseTest, Parse) {
    const char* string = GetParam().string;
    Extension expect = GetParam().value;
    EXPECT_EQ(expect, ParseExtension(string));
}

INSTANTIATE_TEST_SUITE_P(ValidCases, ExtensionParseTest, testing::ValuesIn(kValidCases));
INSTANTIATE_TEST_SUITE_P(InvalidCases, ExtensionParseTest, testing::ValuesIn(kInvalidCases));

using ExtensionPrintTest = testing::TestWithParam<Case>;

TEST_P(ExtensionPrintTest, Print) {
    Extension value = GetParam().value;
    const char* expect = GetParam().string;
    EXPECT_EQ(expect, tint::ToString(value));
}

INSTANTIATE_TEST_SUITE_P(ValidCases, ExtensionPrintTest, testing::ValuesIn(kValidCases));

}  // namespace parse_print_tests

}  // namespace
}  // namespace tint::core
