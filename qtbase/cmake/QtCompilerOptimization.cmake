# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (MSVC)
    if (QT_64BIT)
        # SSE2 is mandatory on 64-bit mode, so skip the option. It triggers:
        # cl : Command line warning D9002 : ignoring unknown option '-arch:SSE2'
        set(QT_CFLAGS_SSE2   "")
    else()
        set(QT_CFLAGS_SSE2   "-arch:SSE2")
    endif()
    set(QT_CFLAGS_SSE3       "${QT_CFLAGS_SSE2}")
    set(QT_CFLAGS_SSSE3      "${QT_CFLAGS_SSE2}")
    set(QT_CFLAGS_SSE4_1     "${QT_CFLAGS_SSE2}")
    set(QT_CFLAGS_SSE4_2     "${QT_CFLAGS_SSE2}")
    set(QT_CFLAGS_AESNI      "${QT_CFLAGS_SSE2}")
    set(QT_CFLAGS_SHANI      "${QT_CFLAGS_SSE2}")

    set(QT_CFLAGS_AVX     "-arch:AVX")
    set(QT_CFLAGS_AVX2    "-arch:AVX2")
    set(QT_CFLAGS_F16C    "-arch:AVX")
    set(QT_CFLAGS_RDRND   "")
    set(QT_CFLAGS_RDSEED  "")
    set(QT_CFLAGS_AVX512F "-arch:AVX512")
    set(QT_CFLAGS_AVX512ER "-arch:AVX512")
    set(QT_CFLAGS_AVX512CD "-arch:AVX512")
    set(QT_CFLAGS_AVX512PF "-arch:AVX512")
    set(QT_CFLAGS_AVX512DQ "-arch:AVX512")
    set(QT_CFLAGS_AVX512BW "-arch:AVX512")
    set(QT_CFLAGS_AVX512VL "-arch:AVX512")
    set(QT_CFLAGS_AVX512IFMA "-arch:AVX512")
    set(QT_CFLAGS_AVX512VBMI "-arch:AVX512")
    set(QT_CFLAGS_AVX512VBMI2 "-arch:AVX512")
    set(QT_CFLAGS_VAES      "")
endif()

if(GCC OR CLANG OR QCC)
    set(QT_CFLAGS_SSE2       "-msse2")
    set(QT_CFLAGS_SSE3       "-msse3")
    set(QT_CFLAGS_SSSE3      "-mssse3")
    set(QT_CFLAGS_SSE4_1     "-msse4.1")
    set(QT_CFLAGS_SSE4_2     "-msse4.2")
    set(QT_CFLAGS_F16C       "-mf16c")
    set(QT_CFLAGS_RDRND      "-mrdrnd")
    set(QT_CFLAGS_RDSEED     "-mrdseed")
    set(QT_CFLAGS_AVX        "-mavx")
    set(QT_CFLAGS_AVX2       "-mavx2")
    set(QT_CFLAGS_ARCH_HASWELL "-march=haswell")
    set(QT_CFLAGS_AVX512F    "-mavx512f")
    set(QT_CFLAGS_AVX512ER   "-mavx512er")
    set(QT_CFLAGS_AVX512CD   "-mavx512cd")
    set(QT_CFLAGS_AVX512PF   "-mavx512pf")
    set(QT_CFLAGS_AVX512DQ   "-mavx512dq")
    set(QT_CFLAGS_AVX512BW   "-mavx512bw")
    set(QT_CFLAGS_AVX512VL   "-mavx512vl")
    set(QT_CFLAGS_AVX512IFMA "-mavx512ifma")
    set(QT_CFLAGS_AVX512VBMI "-mavx512vbmi")
    set(QT_CFLAGS_AVX512VBMI2 "-mavx512vbmi2")
    set(QT_CFLAGS_AESNI      "-maes")
    set(QT_CFLAGS_SHANI      "-msha")
    set(QT_CFLAGS_VAES       "-mvaes")
    if(NOT UIKIT AND NOT QT_64BIT)
        set(QT_CFLAGS_NEON   "-mfpu=neon")
    endif()
    set(QT_CFLAGS_MIPS_DSP   "-mdsp")
    set(QT_CFLAGS_MIPS_DSPR2 "-mdspr2")
endif()

# Fall through is important, so that more specific flags that might be missing are set by the
# previous base cases.
# This mirrors qmake's mkspecs QMAKE_CFLAGS_OPTIMIZE assignments (mostly).
#
# TODO: Missing mkspecs flags we don't handle below: win32-clang-g++, win32-clang-msvc, rtems-base
#
# gcc and clang base
if(GCC OR CLANG)
    set(QT_CFLAGS_OPTIMIZE "-O2")
    set(QT_CFLAGS_OPTIMIZE_FULL "-O3")
    set(QT_CFLAGS_OPTIMIZE_DEBUG "-Og")
    set(QT_CFLAGS_OPTIMIZE_SIZE "-Os")

    if(CLANG)
        set(QT_CFLAGS_OPTIMIZE_SIZE "-Oz")
    endif()
endif()

# Flags that CMake might set, aka flags the compiler would see as valid values.
if(GCC OR CLANG OR QCC)
    set(QT_CFLAGS_OPTIMIZE_VALID_VALUES "-O0" "-O1" "-O2" "-O3" "-Os" "-Oz")
endif()


# Windows MSVC
if(MSVC)
    set(QT_CFLAGS_OPTIMIZE "-O2 -Ob3") # -Ob3 was introduced in Visual Studio 2019 version 16.0
    set(QT_CFLAGS_OPTIMIZE_DEBUG "-Od")
    set(QT_CFLAGS_OPTIMIZE_SIZE "-O1")
    set(QT_CFLAGS_OPTIMIZE_VALID_VALUES "/O2" "/O1" "/Od" "/Ob0" "/Ob1" "/Ob2" "/Ob3" "/O0" "-O0")

    if(CLANG)
        set(QT_CFLAGS_OPTIMIZE_FULL "/clang:-O3")
        set(QT_CFLAGS_OPTIMIZE_SIZE "/clang:-Oz")
    endif()
endif()

# Android Clang
if(CLANG AND ANDROID)
    if(QT_FEATURE_ltcg)
        # When using LTCG, the linker cannot cope with -Oz.  See QTBUG-89472 for details.
        set(QT_CFLAGS_OPTIMIZE "-O2")
        set(QT_CFLAGS_OPTIMIZE_FULL "-O3")
    else()
        set(QT_CFLAGS_OPTIMIZE "-Oz")
        set(QT_CFLAGS_OPTIMIZE_FULL "-Oz")
    endif()
endif()

# qcc
if (QCC)
    set(QT_CFLAGS_OPTIMIZE "-O2")
    set(QT_CFLAGS_OPTIMIZE_FULL "-O3")
endif()

# Emscripten Clang
if(WASM)
    set(QT_CFLAGS_OPTIMIZE_DEBUG "-O2 -g") # -Og is not supported
    set(QT_CFLAGS_SSE2 "-O2 -msimd128 -msse -msse2")
endif()
