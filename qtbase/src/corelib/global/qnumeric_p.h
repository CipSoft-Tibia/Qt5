// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNUMERIC_P_H
#define QNUMERIC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/private/qglobal_p.h"
#include "QtCore/qnumeric.h"
#include "QtCore/qsimd.h"
#include <cmath>
#include <limits>
#include <type_traits>

#if !defined(Q_CC_MSVC) && defined(Q_OS_QNX)
#  include <math.h>
#  ifdef isnan
#    define QT_MATH_H_DEFINES_MACROS
QT_BEGIN_NAMESPACE
namespace qnumeric_std_wrapper {
// the 'using namespace std' below is cases where the stdlib already put the math.h functions in the std namespace and undefined the macros.
Q_DECL_CONST_FUNCTION static inline bool math_h_isnan(double d) { using namespace std; return isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isinf(double d) { using namespace std; return isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isfinite(double d) { using namespace std; return isfinite(d); }
Q_DECL_CONST_FUNCTION static inline int math_h_fpclassify(double d) { using namespace std; return fpclassify(d); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isnan(float f) { using namespace std; return isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isinf(float f) { using namespace std; return isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool math_h_isfinite(float f) { using namespace std; return isfinite(f); }
Q_DECL_CONST_FUNCTION static inline int math_h_fpclassify(float f) { using namespace std; return fpclassify(f); }
}
QT_END_NAMESPACE
// These macros from math.h conflict with the real functions in the std namespace.
#    undef signbit
#    undef isnan
#    undef isinf
#    undef isfinite
#    undef fpclassify
#  endif // defined(isnan)
#endif

QT_BEGIN_NAMESPACE

namespace qnumeric_std_wrapper {
#if defined(QT_MATH_H_DEFINES_MACROS)
#  undef QT_MATH_H_DEFINES_MACROS
Q_DECL_CONST_FUNCTION static inline bool isnan(double d) { return math_h_isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool isinf(double d) { return math_h_isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(double d) { return math_h_isfinite(d); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(double d) { return math_h_fpclassify(d); }
Q_DECL_CONST_FUNCTION static inline bool isnan(float f) { return math_h_isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool isinf(float f) { return math_h_isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(float f) { return math_h_isfinite(f); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(float f) { return math_h_fpclassify(f); }
#else
Q_DECL_CONST_FUNCTION static inline bool isnan(double d) { return std::isnan(d); }
Q_DECL_CONST_FUNCTION static inline bool isinf(double d) { return std::isinf(d); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(double d) { return std::isfinite(d); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(double d) { return std::fpclassify(d); }
Q_DECL_CONST_FUNCTION static inline bool isnan(float f) { return std::isnan(f); }
Q_DECL_CONST_FUNCTION static inline bool isinf(float f) { return std::isinf(f); }
Q_DECL_CONST_FUNCTION static inline bool isfinite(float f) { return std::isfinite(f); }
Q_DECL_CONST_FUNCTION static inline int fpclassify(float f) { return std::fpclassify(f); }
#endif
}

constexpr Q_DECL_CONST_FUNCTION static inline double qt_inf() noexcept
{
    static_assert(std::numeric_limits<double>::has_infinity,
                  "platform has no definition for infinity for type double");
    return std::numeric_limits<double>::infinity();
}

#if QT_CONFIG(signaling_nan)
constexpr Q_DECL_CONST_FUNCTION static inline double qt_snan() noexcept
{
    static_assert(std::numeric_limits<double>::has_signaling_NaN,
                  "platform has no definition for signaling NaN for type double");
    return std::numeric_limits<double>::signaling_NaN();
}
#endif

// Quiet NaN
constexpr Q_DECL_CONST_FUNCTION static inline double qt_qnan() noexcept
{
    static_assert(std::numeric_limits<double>::has_quiet_NaN,
                  "platform has no definition for quiet NaN for type double");
    return std::numeric_limits<double>::quiet_NaN();
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_inf(double d)
{
    return qnumeric_std_wrapper::isinf(d);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_nan(double d)
{
    return qnumeric_std_wrapper::isnan(d);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_finite(double d)
{
    return qnumeric_std_wrapper::isfinite(d);
}

Q_DECL_CONST_FUNCTION static inline int qt_fpclassify(double d)
{
    return qnumeric_std_wrapper::fpclassify(d);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_inf(float f)
{
    return qnumeric_std_wrapper::isinf(f);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_nan(float f)
{
    return qnumeric_std_wrapper::isnan(f);
}

Q_DECL_CONST_FUNCTION static inline bool qt_is_finite(float f)
{
    return qnumeric_std_wrapper::isfinite(f);
}

Q_DECL_CONST_FUNCTION static inline int qt_fpclassify(float f)
{
    return qnumeric_std_wrapper::fpclassify(f);
}

#ifndef Q_QDOC
namespace {
/*!
    Returns true if the double \a v can be converted to type \c T, false if
    it's out of range. If the conversion is successful, the converted value is
    stored in \a value; if it was not successful, \a value will contain the
    minimum or maximum of T, depending on the sign of \a d. If \c T is
    unsigned, then \a value contains the absolute value of \a v.

    This function works for v containing infinities, but not NaN. It's the
    caller's responsibility to exclude that possibility before calling it.
*/
template<typename T>
static inline bool convertDoubleTo(double v, T *value, bool allow_precision_upgrade = true)
{
    static_assert(std::numeric_limits<T>::is_integer);
    static_assert(std::is_integral_v<T>);
    constexpr bool TypeIsLarger = std::numeric_limits<T>::digits > std::numeric_limits<double>::digits;

    if constexpr (TypeIsLarger) {
        using S = std::make_signed_t<T>;
        constexpr S max_mantissa = S(1) << std::numeric_limits<double>::digits;
        // T has more bits than double's mantissa, so don't allow "upgrading"
        // to T (makes it look like the number had more precision than really
        // was transmitted)
        if (!allow_precision_upgrade && !(v <= double(max_mantissa) && v >= double(-max_mantissa - 1)))
            return false;
    }

    constexpr T Tmin = (std::numeric_limits<T>::min)();
    constexpr T Tmax = (std::numeric_limits<T>::max)();

    // The [conv.fpint] (7.10 Floating-integral conversions) section of the C++
    // standard says only exact conversions are guaranteed. Converting
    // integrals to floating-point with loss of precision has implementation-
    // defined behavior whether the next higher or next lower is returned;
    // converting FP to integral is UB if it can't be represented.
    //
    // That means we can't write UINT64_MAX+1. Writing ldexp(1, 64) would be
    // correct, but Clang, ICC and MSVC don't realize that it's a constant and
    // the math call stays in the compiled code.

#if defined(Q_PROCESSOR_X86_64) && defined(__SSE2__)
    // Of course, UB doesn't apply if we use intrinsics, in which case we are
    // allowed to dpeend on exactly the processor's behavior. This
    // implementation uses the truncating conversions from Scalar Double to
    // integral types (CVTTSD2SI and VCVTTSD2USI), which is documented to
    // return the "indefinite integer value" if the range of the target type is
    // exceeded. (only implemented for x86-64 to avoid having to deal with the
    // non-existence of the 64-bit intrinsics on i386)

    if (std::numeric_limits<T>::is_signed) {
        __m128d mv = _mm_set_sd(v);
#  ifdef __AVX512F__
        // use explicit round control and suppress exceptions
        if (sizeof(T) > 4)
            *value = T(_mm_cvtt_roundsd_i64(mv, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
        else
            *value = _mm_cvtt_roundsd_i32(mv, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
#  else
        *value = sizeof(T) > 4 ? T(_mm_cvttsd_si64(mv)) : _mm_cvttsd_si32(mv);
#  endif

        // if *value is the "indefinite integer value", check if the original
        // variable \a v is the same value (Tmin is an exact representation)
        if (*value == Tmin && !_mm_ucomieq_sd(mv, _mm_set_sd(Tmin))) {
            // v != Tmin, so it was out of range
            if (v > 0)
                *value = Tmax;
            return false;
        }

        // convert the integer back to double and compare for equality with v,
        // to determine if we've lost any precision
        __m128d mi = _mm_setzero_pd();
        mi = sizeof(T) > 4 ? _mm_cvtsi64_sd(mv, *value) : _mm_cvtsi32_sd(mv, *value);
        return _mm_ucomieq_sd(mv, mi);
    }

#  ifdef __AVX512F__
    if (!std::numeric_limits<T>::is_signed) {
        // Same thing as above, but this function operates on absolute values
        // and the "indefinite integer value" for the 64-bit unsigned
        // conversion (Tmax) is not representable in double, so it can never be
        // the result of an in-range conversion. This is implemented for AVX512
        // and later because of the unsigned conversion instruction. Converting
        // to unsigned without losing an extra bit of precision prior to AVX512
        // is left to the compiler below.

        v = fabs(v);
        __m128d mv = _mm_set_sd(v);

        // use explicit round control and suppress exceptions
        if (sizeof(T) > 4)
            *value = T(_mm_cvtt_roundsd_u64(mv, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
        else
            *value = _mm_cvtt_roundsd_u32(mv, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);

        if (*value == Tmax) {
            // no double can have an exact value of quint64(-1), but they can
            // quint32(-1), so we need to compare for that
            if (TypeIsLarger || _mm_ucomieq_sd(mv, _mm_set_sd(Tmax)))
                return false;
        }

        // return true if it was an exact conversion
        __m128d mi = _mm_setzero_pd();
        mi = sizeof(T) > 4 ? _mm_cvtu64_sd(mv, *value) : _mm_cvtu32_sd(mv, *value);
        return _mm_ucomieq_sd(mv, mi);
    }
#  endif
#endif

    double supremum;
    if (std::numeric_limits<T>::is_signed) {
        supremum = -1.0 * Tmin;     // -1 * (-2^63) = 2^63, exact (for T = qint64)
        *value = Tmin;
        if (v < Tmin)
            return false;
    } else {
        using ST = typename std::make_signed<T>::type;
        supremum = -2.0 * (std::numeric_limits<ST>::min)();   // -2 * (-2^63) = 2^64, exact (for T = quint64)
        v = fabs(v);
    }

    *value = Tmax;
    if (v >= supremum)
        return false;

    // Now we can convert, these two conversions cannot be UB
    *value = T(v);

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE

    return *value == v;

QT_WARNING_POP
}

template <typename T> inline bool add_overflow(T v1, T v2, T *r) { return qAddOverflow(v1, v2, r); }
template <typename T> inline bool sub_overflow(T v1, T v2, T *r) { return qSubOverflow(v1, v2, r); }
template <typename T> inline bool mul_overflow(T v1, T v2, T *r) { return qMulOverflow(v1, v2, r); }

template <typename T, T V2> bool add_overflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qAddOverflow<T, V2>(v1, std::integral_constant<T, V2>{}, r);
}

template <auto V2, typename T> bool add_overflow(T v1, T *r)
{
    return qAddOverflow<V2, T>(v1, r);
}

template <typename T, T V2> bool sub_overflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qSubOverflow<T, V2>(v1, std::integral_constant<T, V2>{}, r);
}

template <auto V2, typename T> bool sub_overflow(T v1, T *r)
{
    return qSubOverflow<V2, T>(v1, r);
}

template <typename T, T V2> bool mul_overflow(T v1, std::integral_constant<T, V2>, T *r)
{
    return qMulOverflow<T, V2>(v1, std::integral_constant<T, V2>{}, r);
}

template <auto V2, typename T> bool mul_overflow(T v1, T *r)
{
    return qMulOverflow<V2, T>(v1, r);
}
}
#endif // Q_QDOC

/*
    Safely narrows \a x to \c{To}. Let \c L be
    \c{std::numeric_limit<To>::min()} and \c H be \c{std::numeric_limit<To>::max()}.

    If \a x is less than L, returns L. If \a x is greater than H,
    returns H. Otherwise, returns \c{To(x)}.
*/
template <typename To, typename From>
static constexpr auto qt_saturate(From x)
{
    static_assert(std::is_integral_v<To>);
    static_assert(std::is_integral_v<From>);

    [[maybe_unused]]
    constexpr auto Lo = (std::numeric_limits<To>::min)();
    constexpr auto Hi = (std::numeric_limits<To>::max)();

    if constexpr (std::is_signed_v<From> == std::is_signed_v<To>) {
        // same signedness, we can accept regular integer conversion rules
        return x < Lo  ? Lo :
               x > Hi  ? Hi :
               /*else*/  To(x);
    } else {
        if constexpr (std::is_signed_v<From>) { // ie. !is_signed_v<To>
            if (x < From{0})
                return To{0};
        }

        // from here on, x >= 0
        using FromU = std::make_unsigned_t<From>;
        using ToU = std::make_unsigned_t<To>;
        return FromU(x) > ToU(Hi) ? Hi : To(x); // assumes Hi >= 0
    }
}

QT_END_NAMESPACE

#endif // QNUMERIC_P_H
