/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_SETTING
#define SKSL_SETTING

#include "src/sksl/SkSLContext.h"
#include "src/sksl/ir/SkSLExpression.h"

namespace SkSL {

/**
 * Represents a compile-time constant setting, such as sk_Caps.fbFetchSupport. These are generally
 * collapsed down to their constant representations during the compilation process.
 */
struct Setting : public Expression {
    static constexpr Kind kExpressionKind = Kind::kSetting;

    Setting(int offset, String name, std::unique_ptr<Expression> value)
    : INHERITED(offset, kExpressionKind, &value->type())
    , fName(std::move(name))
    , fValue(std::move(value)) {
        SkASSERT(fValue->isCompileTimeConstant());
    }

    std::unique_ptr<Expression> constantPropagate(const IRGenerator& irGenerator,
                                                  const DefinitionMap& definitions) override;

    std::unique_ptr<Expression> clone() const override {
        return std::unique_ptr<Expression>(new Setting(fOffset, fName, fValue->clone()));
    }

    String description() const override {
        return fName;
    }

    bool hasProperty(Property property) const override {
        return false;
    }

    bool isCompileTimeConstant() const override {
        return true;
    }

    const String fName;
    std::unique_ptr<Expression> fValue;

    using INHERITED = Expression;
};

}  // namespace SkSL

#endif
