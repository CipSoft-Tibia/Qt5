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

#include "src/tint/lang/wgsl/ast/helper_test.h"
#include "src/tint/lang/wgsl/reader/parser/helper_test.h"

namespace tint::wgsl::reader {
namespace {

struct VariableStorageData {
    const char* input;
    core::AddressSpace address_space;
    core::Access access;
};
inline std::ostream& operator<<(std::ostream& out, VariableStorageData data) {
    out << std::string(data.input);
    return out;
}

class VariableQualifierTest : public WGSLParserTestWithParam<VariableStorageData> {};

TEST_P(VariableQualifierTest, ParsesAddressSpace) {
    auto params = GetParam();
    auto p = parser(std::string("var<") + params.input + "> name");

    auto sc = p->variable_decl();
    EXPECT_FALSE(p->has_error());
    EXPECT_FALSE(sc.errored);
    EXPECT_TRUE(sc.matched);
    if (params.address_space != core::AddressSpace::kUndefined) {
        ast::CheckIdentifier(sc->address_space, tint::ToString(params.address_space));
    } else {
        EXPECT_EQ(sc->address_space, nullptr);
    }
    if (params.access != core::Access::kUndefined) {
        ast::CheckIdentifier(sc->access, tint::ToString(params.access));
    } else {
        EXPECT_EQ(sc->access, nullptr);
    }

    auto& t = p->next();
    EXPECT_TRUE(t.IsEof());
}
INSTANTIATE_TEST_SUITE_P(
    WGSLParserTest,
    VariableQualifierTest,
    testing::Values(
        VariableStorageData{"uniform", core::AddressSpace::kUniform, core::Access::kUndefined},
        VariableStorageData{"workgroup", core::AddressSpace::kWorkgroup, core::Access::kUndefined},
        VariableStorageData{"storage", core::AddressSpace::kStorage, core::Access::kUndefined},
        VariableStorageData{"private", core::AddressSpace::kPrivate, core::Access::kUndefined},
        VariableStorageData{"function", core::AddressSpace::kFunction, core::Access::kUndefined},
        VariableStorageData{"storage, read", core::AddressSpace::kStorage, core::Access::kRead},
        VariableStorageData{"storage, write", core::AddressSpace::kStorage, core::Access::kWrite},
        VariableStorageData{"storage, read_write", core::AddressSpace::kStorage,
                            core::Access::kReadWrite}));

TEST_F(WGSLParserTest, VariableQualifier_Empty) {
    auto p = parser("var<> name");
    auto sc = p->variable_decl();
    EXPECT_TRUE(p->has_error());
    EXPECT_TRUE(sc.errored);
    EXPECT_FALSE(sc.matched);
    EXPECT_EQ(p->error(), R"(1:5: expected expression for 'var' address space)");
}

TEST_F(WGSLParserTest, VariableQualifier_MissingLessThan) {
    auto p = parser("private>");
    auto sc = p->variable_qualifier();
    EXPECT_FALSE(p->has_error());
    EXPECT_FALSE(sc.errored);
    EXPECT_FALSE(sc.matched);

    auto& t = p->next();
    ASSERT_TRUE(t.Is(Token::Type::kIdentifier));
}

TEST_F(WGSLParserTest, VariableQualifier_MissingLessThan_AfterSC) {
    auto p = parser("private, >");
    auto sc = p->variable_qualifier();
    EXPECT_FALSE(p->has_error());
    EXPECT_FALSE(sc.errored);
    EXPECT_FALSE(sc.matched);

    auto& t = p->next();
    ASSERT_TRUE(t.Is(Token::Type::kIdentifier));
}

TEST_F(WGSLParserTest, VariableQualifier_MissingGreaterThan) {
    auto p = parser("<private");
    auto sc = p->variable_qualifier();
    EXPECT_TRUE(p->has_error());
    EXPECT_TRUE(sc.errored);
    EXPECT_FALSE(sc.matched);
    EXPECT_EQ(p->error(), "1:1: missing closing '>' for variable declaration");
}

}  // namespace
}  // namespace tint::wgsl::reader
