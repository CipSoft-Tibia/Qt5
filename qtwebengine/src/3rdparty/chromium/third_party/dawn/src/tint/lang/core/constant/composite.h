// Copyright 2022 The Tint Authors.
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

#ifndef SRC_TINT_LANG_CORE_CONSTANT_COMPOSITE_H_
#define SRC_TINT_LANG_CORE_CONSTANT_COMPOSITE_H_

#include "src/tint/lang/core/constant/value.h"
#include "src/tint/lang/core/number.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/math/hash.h"
#include "src/tint/utils/rtti/castable.h"

namespace tint::core::constant {

/// Composite holds a number of mixed child values.
/// Composite may be of a vector, matrix, array or structure type.
/// If each element is the same type and value, then a Splat would be a more efficient constant
/// implementation. Use CreateComposite() to create the appropriate type.
class Composite : public Castable<Composite, Value> {
  public:
    /// Constructor
    /// @param t the compsite type
    /// @param els the composite elements
    /// @param all_0 true if all elements are 0
    /// @param any_0 true if any element is 0
    Composite(const core::type::Type* t, VectorRef<const Value*> els, bool all_0, bool any_0);
    ~Composite() override;

    /// @copydoc Value::Type()
    const core::type::Type* Type() const override { return type; }

    /// @copydoc Value::Index()
    const Value* Index(size_t i) const override {
        return i < elements.Length() ? elements[i] : nullptr;
    }

    /// @copydoc Value::NumElements()
    size_t NumElements() const override { return elements.Length(); }

    /// @copydoc Value::AllZero()
    bool AllZero() const override { return all_zero; }

    /// @copydoc Value::AnyZero()
    bool AnyZero() const override { return any_zero; }

    /// @copydoc Value::Hash()
    size_t Hash() const override { return hash; }

    /// Clones the constant into the provided context
    /// @param ctx the clone context
    /// @returns the cloned node
    const Composite* Clone(CloneContext& ctx) const override;

    /// The composite type
    core::type::Type const* const type;
    /// The composite elements
    const Vector<const Value*, 4> elements;
    /// True if all elements are zero
    const bool all_zero;
    /// True if any element is zero
    const bool any_zero;
    /// The hash of the composite
    const size_t hash;

  protected:
    /// @copydoc Value::InternalValue()
    std::variant<std::monostate, AInt, AFloat> InternalValue() const override { return {}; }

  private:
    size_t CalcHash() {
        auto h = tint::Hash(type, all_zero, any_zero);
        for (auto* el : elements) {
            h = HashCombine(h, el->Hash());
        }
        return h;
    }
};

}  // namespace tint::core::constant

#endif  // SRC_TINT_LANG_CORE_CONSTANT_COMPOSITE_H_
