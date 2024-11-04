// Copyright 2021 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0(the "License");
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

#ifndef SRC_TINT_LANG_WGSL_SEM_CALL_TARGET_H_
#define SRC_TINT_LANG_WGSL_SEM_CALL_TARGET_H_

#include <vector>

#include "src/tint/lang/core/type/sampler.h"
#include "src/tint/lang/wgsl/sem/node.h"
#include "src/tint/lang/wgsl/sem/variable.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/math/hash.h"

namespace tint::sem {

/// CallTargetSignature holds the return type and parameters for a call target
struct CallTargetSignature {
    /// Constructor
    CallTargetSignature();

    /// Constructor
    /// @param ret_ty the call target return type
    /// @param params the call target parameters
    CallTargetSignature(const core::type::Type* ret_ty, VectorRef<const Parameter*> params);

    /// Copy constructor
    CallTargetSignature(const CallTargetSignature&);

    /// Destructor
    ~CallTargetSignature();

    /// The type of the call target return value
    const core::type::Type* return_type = nullptr;
    /// The parameters of the call target
    tint::Vector<const sem::Parameter*, 8> parameters;

    /// Equality operator
    /// @param other the signature to compare this to
    /// @returns true if this signature is equal to other
    bool operator==(const CallTargetSignature& other) const;

    /// @param usage the parameter usage to find
    /// @returns the index of the parameter with the given usage, or -1 if no
    /// parameter with the given usage exists.
    int IndexOf(core::ParameterUsage usage) const;

    /// @param usage  the parameter usage to find
    /// @returns the the parameter with the given usage, or nullptr if no parameter with the given
    /// usage exists.
    inline const sem::Parameter* Parameter(core::ParameterUsage usage) const {
        auto idx = IndexOf(usage);
        return (idx >= 0) ? parameters[static_cast<size_t>(idx)] : nullptr;
    }
};

/// CallTarget is the base for callable functions, builtins, value constructors and value
/// conversions.
class CallTarget : public Castable<CallTarget, Node> {
  public:
    /// Constructor
    /// @param stage the earliest evaluation stage for a call to this target
    /// @param must_use the result of the call target must be used, i.e. it cannot be used as a call
    /// statement.
    CallTarget(core::EvaluationStage stage, bool must_use);

    /// Constructor
    /// @param return_type the return type of the call target
    /// @param parameters the parameters for the call target
    /// @param stage the earliest evaluation stage for a call to this target
    /// @param must_use the result of the call target must be used, i.e. it cannot be used as a call
    /// statement.
    CallTarget(const core::type::Type* return_type,
               VectorRef<Parameter*> parameters,
               core::EvaluationStage stage,
               bool must_use);

    /// Copy constructor
    CallTarget(const CallTarget&);

    /// Destructor
    ~CallTarget() override;

    /// Sets the call target's return type
    /// @param ty the parameter
    void SetReturnType(const core::type::Type* ty) { signature_.return_type = ty; }

    /// @return the return type of the call target
    const core::type::Type* ReturnType() const { return signature_.return_type; }

    /// Adds a parameter to the call target
    /// @param parameter the parameter
    void AddParameter(Parameter* parameter) {
        parameter->SetOwner(this);
        signature_.parameters.Push(parameter);
    }

    /// @return the parameters of the call target
    auto& Parameters() const { return signature_.parameters; }

    /// @return the signature of the call target
    const CallTargetSignature& Signature() const { return signature_; }

    /// @return the earliest evaluation stage for a call to this target
    core::EvaluationStage Stage() const { return stage_; }

    /// @returns true if the result of the call target must be used, i.e. it cannot be used as a
    /// call statement.
    bool MustUse() const { return must_use_; }

  private:
    CallTargetSignature signature_;
    core::EvaluationStage stage_;
    const bool must_use_;
};

}  // namespace tint::sem

namespace std {

/// Custom std::hash specialization for tint::sem::CallTargetSignature so
/// CallTargetSignature can be used as keys for std::unordered_map and
/// std::unordered_set.
template <>
class hash<tint::sem::CallTargetSignature> {
  public:
    /// @param sig the CallTargetSignature to hash
    /// @return the hash value
    std::size_t operator()(const tint::sem::CallTargetSignature& sig) const;
};

}  // namespace std

#endif  // SRC_TINT_LANG_WGSL_SEM_CALL_TARGET_H_
