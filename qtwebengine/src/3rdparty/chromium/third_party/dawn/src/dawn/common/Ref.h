// Copyright 2023 The Dawn Authors
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

#ifndef SRC_DAWN_COMMON_REF_H_
#define SRC_DAWN_COMMON_REF_H_

#include <mutex>

#include "dawn/common/RefBase.h"
#include "dawn/common/RefCounted.h"

namespace dawn {

template <typename T>
class WeakRef;

namespace detail {

class WeakRefSupportBase;

template <typename T>
struct RefCountedTraits {
    static constexpr T* kNullValue = nullptr;
    static void Reference(T* value) { value->Reference(); }
    static void Release(T* value) { value->Release(); }
};

}  // namespace detail

template <typename T>
class Ref : public RefBase<T*, detail::RefCountedTraits<T>> {
  public:
    using RefBase<T*, detail::RefCountedTraits<T>>::RefBase;
};

template <typename T>
Ref<T> AcquireRef(T* pointee) {
    Ref<T> ref;
    ref.Acquire(pointee);
    return ref;
}

template <typename T>
struct UnwrapRef {
    using type = T;
};
template <typename T>
struct UnwrapRef<Ref<T>> {
    using type = T;
};

template <typename T>
struct IsRef {
    static constexpr bool value = false;
};
template <typename T>
struct IsRef<Ref<T>> {
    static constexpr bool value = true;
};

}  // namespace dawn

#endif  // SRC_DAWN_COMMON_REF_H_
