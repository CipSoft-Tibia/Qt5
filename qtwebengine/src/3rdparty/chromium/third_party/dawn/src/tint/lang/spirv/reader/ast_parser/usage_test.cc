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

#include <algorithm>
#include <vector>

#include "gmock/gmock.h"
#include "src/tint/lang/spirv/reader/ast_parser/helper_test.h"
#include "src/tint/utils/text/string_stream.h"

namespace tint::spirv::reader::ast_parser {
namespace {

using ::testing::Eq;

TEST_F(SpirvASTParserTest, Usage_Trivial_Properties) {
    Usage u;
    EXPECT_TRUE(u.IsValid());
    EXPECT_FALSE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_FALSE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());
}

TEST_F(SpirvASTParserTest, Usage_Trivial_Output) {
    StringStream ss;
    Usage u;
    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage()"));
}

TEST_F(SpirvASTParserTest, Usage_Equality_OneDifference) {
    const size_t num_usages = 9u;
    std::vector<Usage> usages(num_usages);
    usages[1].AddSampler();
    usages[2].AddComparisonSampler();
    usages[3].AddTexture();
    usages[4].AddSampledTexture();
    usages[5].AddMultisampledTexture();
    usages[6].AddDepthTexture();
    usages[7].AddStorageReadTexture();
    usages[8].AddStorageWriteTexture();
    for (size_t i = 0; i < num_usages; ++i) {
        for (size_t j = 0; j < num_usages; ++j) {
            const auto& lhs = usages[i];
            const auto& rhs = usages[j];
            if (i == j) {
                EXPECT_TRUE(lhs == rhs);
            } else {
                EXPECT_FALSE(lhs == rhs);
            }
        }
    }
}

TEST_F(SpirvASTParserTest, Usage_Add) {
    // Mix two nontrivial usages.
    Usage a;
    a.AddStorageReadTexture();

    Usage b;
    b.AddComparisonSampler();

    a.Add(b);

    EXPECT_FALSE(a.IsValid());
    EXPECT_FALSE(a.IsComplete());
    EXPECT_TRUE(a.IsSampler());
    EXPECT_TRUE(a.IsComparisonSampler());
    EXPECT_TRUE(a.IsTexture());
    EXPECT_FALSE(a.IsSampledTexture());
    EXPECT_FALSE(a.IsMultisampledTexture());
    EXPECT_FALSE(a.IsDepthTexture());
    EXPECT_TRUE(a.IsStorageReadOnlyTexture());
    EXPECT_FALSE(a.IsStorageReadWriteTexture());
    EXPECT_FALSE(a.IsStorageWriteOnlyTexture());

    StringStream ss;
    ss << a;
    EXPECT_THAT(ss.str(), Eq("Usage(Sampler( comparison )Texture( read ))"));
}

TEST_F(SpirvASTParserTest, Usage_AddSampler) {
    StringStream ss;
    Usage u;
    u.AddSampler();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_TRUE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_FALSE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Sampler( ))"));

    // Check idempotency
    auto copy(u);
    u.AddSampler();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddComparisonSampler) {
    StringStream ss;
    Usage u;
    u.AddComparisonSampler();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_TRUE(u.IsSampler());
    EXPECT_TRUE(u.IsComparisonSampler());
    EXPECT_FALSE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Sampler( comparison ))"));

    auto copy(u);
    u.AddComparisonSampler();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddTexture) {
    StringStream ss;
    Usage u;
    u.AddTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_FALSE(u.IsComplete());  // Don't know if it's sampled or storage
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( ))"));

    auto copy(u);
    u.AddTexture();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddSampledTexture) {
    StringStream ss;
    Usage u;
    u.AddSampledTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_TRUE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( is_sampled ))"));

    auto copy(u);
    u.AddSampledTexture();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddMultisampledTexture) {
    StringStream ss;
    Usage u;
    u.AddMultisampledTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_TRUE(u.IsSampledTexture());
    EXPECT_TRUE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( is_sampled ms ))"));

    auto copy(u);
    u.AddMultisampledTexture();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddDepthTexture) {
    StringStream ss;
    Usage u;
    u.AddDepthTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_TRUE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_TRUE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( is_sampled depth ))"));

    auto copy(u);
    u.AddDepthTexture();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddStorageReadTexture) {
    StringStream ss;
    Usage u;
    u.AddStorageReadTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_TRUE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( read ))"));

    auto copy(u);
    u.AddStorageReadTexture();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddStorageWriteTexture) {
    StringStream ss;
    Usage u;
    u.AddStorageWriteTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_FALSE(u.IsStorageReadWriteTexture());
    EXPECT_TRUE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( write ))"));

    auto copy(u);
    u.AddStorageWriteTexture();
    EXPECT_TRUE(u == copy);
}

TEST_F(SpirvASTParserTest, Usage_AddStorageReadWriteTexture) {
    StringStream ss;
    Usage u;
    u.AddStorageReadTexture();
    u.AddStorageWriteTexture();

    EXPECT_TRUE(u.IsValid());
    EXPECT_TRUE(u.IsComplete());
    EXPECT_FALSE(u.IsSampler());
    EXPECT_FALSE(u.IsComparisonSampler());
    EXPECT_TRUE(u.IsTexture());
    EXPECT_FALSE(u.IsSampledTexture());
    EXPECT_FALSE(u.IsMultisampledTexture());
    EXPECT_FALSE(u.IsDepthTexture());
    EXPECT_FALSE(u.IsStorageReadOnlyTexture());
    EXPECT_TRUE(u.IsStorageReadWriteTexture());
    EXPECT_FALSE(u.IsStorageWriteOnlyTexture());

    ss << u;
    EXPECT_THAT(ss.str(), Eq("Usage(Texture( read write ))"));

    auto copy(u);
    u.AddStorageReadTexture();
    u.AddStorageWriteTexture();
    EXPECT_TRUE(u == copy);
}

}  // namespace
}  // namespace tint::spirv::reader::ast_parser
