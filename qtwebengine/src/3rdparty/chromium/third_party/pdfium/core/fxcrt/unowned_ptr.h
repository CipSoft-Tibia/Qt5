// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_UNOWNED_PTR_H_
#define CORE_FXCRT_UNOWNED_PTR_H_

// UnownedPtr is a smart pointer class that behaves very much like a
// standard C-style pointer. The advantages of using it over native T*
// pointers are:
//
// 1. It documents the nature of the pointer with no need to add a comment
//    explaining that is it // Not owned.
//
// 2. An attempt to delete an unowned ptr will fail to compile rather
//    than silently succeeding, since it is a class and not a raw pointer.
//
// 3. When built using the memory tool ASAN, the class provides a destructor
//    which checks that the object being pointed to is still alive.
//
// 4. When built against PartitionAlloc's BRP feature, it provides the same
//    UaF protections as base::raw_ptr<T>
//
// 5. It is initialized to nullptr by default.
//
// Hence, when using UnownedPtr, no dangling pointers are ever permitted,
// even if they are not de-referenced after becoming dangling. The style of
// programming required is that the lifetime an object containing an
// UnownedPtr must be strictly less than the object to which it points.
//
// The same checks are also performed at assignment time to prove that the
// old value was not a dangling pointer, either.
//
// The array indexing operation [] is not supported on an unowned ptr,
// because an unowned ptr expresses a one to one relationship with some
// other heap object. Use pdfium::span<> for the cases where indexing
// into an unowned array is desired, which performs the same checks.

#include "build/build_config.h"

#if defined(PDF_USE_PARTITION_ALLOC)
#if defined(PDF_USE_PARTITION_ALLOC_NEW_LOCATION)
#include "base/allocator/partition_allocator/src/partition_alloc/partition_alloc_buildflags.h"
#else
#include "base/allocator/partition_allocator/partition_alloc_buildflags.h"
#endif

// Can only use base::raw_ptr<> impls that force nullptr initialization.
#if BUILDFLAG(ENABLE_BACKUP_REF_PTR_SUPPORT) || BUILDFLAG(USE_ASAN_UNOWNED_PTR)
#define UNOWNED_PTR_IS_BASE_RAW_PTR
#endif

#if BUILDFLAG(ENABLE_DANGLING_RAW_PTR_CHECKS) || BUILDFLAG(USE_ASAN_UNOWNED_PTR)
#define UNOWNED_PTR_DANGLING_CHECKS
#endif
#endif  // PDF_USE_PARTITION_ALLOC

#if defined(UNOWNED_PTR_IS_BASE_RAW_PTR)
#if defined(PDF_USE_PARTITION_ALLOC_NEW_LOCATION)
#include "base/allocator/partition_allocator/src/partition_alloc/pointers/raw_ptr.h"
#else
#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#endif

template <typename T>
using UnownedPtr = raw_ptr<T>;

#else  // UNOWNED_PTR_IS_BASE_RAW_PTR

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "core/fxcrt/unowned_ptr_exclusion.h"
#include "third_party/base/compiler_specific.h"

#if defined(ADDRESS_SANITIZER)
#define UNOWNED_PTR_DANGLING_CHECKS
#endif

namespace pdfium {

template <typename T>
class span;

}  // namespace pdfium

namespace fxcrt {

template <class T>
class TRIVIAL_ABI GSL_POINTER UnownedPtr {
 public:
  constexpr UnownedPtr() noexcept = default;

  // Deliberately implicit to allow returning nullptrs.
  // NOLINTNEXTLINE(runtime/explicit)
  constexpr UnownedPtr(std::nullptr_t ptr) {}

  explicit constexpr UnownedPtr(T* pObj) noexcept : m_pObj(pObj) {}

  // Copy-construct an UnownedPtr.
  // Required in addition to copy conversion constructor below.
  constexpr UnownedPtr(const UnownedPtr& that) noexcept
      : m_pObj(static_cast<T*>(that)) {}

  // Move-construct an UnownedPtr. After construction, |that| will be NULL.
  // Required in addition to move conversion constructor below.
  constexpr UnownedPtr(UnownedPtr&& that) noexcept
      : m_pObj(that.ExtractAsDangling()) {}

  // Copy-conversion constructor.
  template <class U,
            typename = typename std::enable_if<
                std::is_convertible<U*, T*>::value>::type>
  UnownedPtr(const UnownedPtr<U>& that) : UnownedPtr(static_cast<U*>(that)) {}

  // Move-conversion constructor.
  template <class U,
            typename = typename std::enable_if<
                std::is_convertible<U*, T*>::value>::type>
  UnownedPtr(UnownedPtr<U>&& that) noexcept {
    Reset(that.ExtractAsDangling());
  }

  // Assign an UnownedPtr from nullptr.
  UnownedPtr& operator=(std::nullptr_t) noexcept {
    Reset();
    return *this;
  }

  // Assign an UnownedPtr from a raw ptr.
  UnownedPtr& operator=(T* that) noexcept {
    Reset(that);
    return *this;
  }

  // Copy-assign an UnownedPtr.
  // Required in addition to copy conversion assignment below.
  UnownedPtr& operator=(const UnownedPtr& that) noexcept {
    if (*this != that)
      Reset(static_cast<T*>(that));
    return *this;
  }

  // Move-assign an UnownedPtr. After assignment, |that| will be NULL.
  // Required in addition to move conversion assignment below.
  UnownedPtr& operator=(UnownedPtr&& that) noexcept {
    if (*this != that)
      Reset(that.ExtractAsDangling());
    return *this;
  }

  // Copy-convert assignment.
  template <class U,
            typename = typename std::enable_if<
                std::is_convertible<U*, T*>::value>::type>
  UnownedPtr& operator=(const UnownedPtr<U>& that) noexcept {
    if (*this != that)
      Reset(that);
    return *this;
  }

  // Move-convert assignment. After assignment, |that| will be NULL.
  template <class U,
            typename = typename std::enable_if<
                std::is_convertible<U*, T*>::value>::type>
  UnownedPtr& operator=(UnownedPtr<U>&& that) noexcept {
    if (*this != that)
      Reset(that.ExtractAsDangling());
    return *this;
  }

  ~UnownedPtr() {
    ProbeForLowSeverityLifetimeIssue();
    m_pObj = nullptr;
  }

  bool operator==(std::nullptr_t ptr) const { return m_pObj == nullptr; }
  bool operator==(const UnownedPtr& that) const {
    return m_pObj == static_cast<T*>(that);
  }
  bool operator<(const UnownedPtr& that) const {
    return std::less<T*>()(m_pObj, static_cast<T*>(that));
  }

  operator T*() const noexcept { return m_pObj; }
  T* get() const noexcept { return m_pObj; }

  T* ExtractAsDangling() {
    ProbeForLowSeverityLifetimeIssue();
    T* pTemp = nullptr;
    std::swap(pTemp, m_pObj);
    return pTemp;
  }

  explicit operator bool() const { return !!m_pObj; }
  T& operator*() const { return *m_pObj; }
  T* operator->() const { return m_pObj; }

 private:
  friend class pdfium::span<T>;

  void Reset(T* obj = nullptr) {
    ProbeForLowSeverityLifetimeIssue();
    m_pObj = obj;
  }

  inline void ProbeForLowSeverityLifetimeIssue() {
#if defined(ADDRESS_SANITIZER)
    if (m_pObj)
      reinterpret_cast<const volatile uint8_t*>(m_pObj)[0];
#endif
  }

  inline void ReleaseBadPointer() {
#if defined(ADDRESS_SANITIZER)
    m_pObj = nullptr;
#endif
  }

  UNOWNED_PTR_EXCLUSION T* m_pObj = nullptr;
};

}  // namespace fxcrt

using fxcrt::UnownedPtr;

#endif  // defined(UNOWNED_PTR_IS_BASE_RAW_PTR)

namespace pdfium {

// Type-deducing wrapper to make an UnownedPtr from an ordinary pointer,
// since equivalent constructor is explicit.
template <typename T>
UnownedPtr<T> WrapUnowned(T* that) {
  return UnownedPtr<T>(that);
}

}  // namespace pdfium

#endif  // CORE_FXCRT_UNOWNED_PTR_H_
