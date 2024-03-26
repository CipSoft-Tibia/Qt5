/*
  Configuration defines for Qt.
*/

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Set to 1 if __builtin_bswap16 is available */
/* #undef HAVE_BUILTIN_BSWAP16 */

/* Set to 1 if __builtin_bswap32 is available */
/* #undef HAVE_BUILTIN_BSWAP32 */

/* Set to 1 if __builtin_bswap64 is available */
/* #undef HAVE_BUILTIN_BSWAP64 */

/* Define to 1 if you have the <cpu-features.h> header file. */
/* #undef HAVE_CPU_FEATURES_H */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <GLUT/glut.h> header file. */
/* #undef #define HAVE_GLUT_GLUT_H */

/* Define to 1 if you have the <GL/glut.h> header file. */
/* #undef HAVE_GL_GLUT_H */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define to 1 if you have the <OpenGL/glut.h> header file. */
/* #undef HAVE_OPENGL_GLUT_H */

/* Have PTHREAD_PRIO_INHERIT. */
/* #undef HAVE_PTHREAD_PRIO_INHERIT */

/* Define to 1 if you have the <shlwapi.h> header file. */
/* #undef HAVE_SHLWAPI_H */

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdio.h> header file. */
/* #undef HAVE_STDIO_H */

/* Define to 1 if you have the <stdlib.h> header file. */
/* #undef HAVE_STDLIB_H */

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
/* #undef HAVE_STRING_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/types.h> header file. */
/* #undef HAVE_SYS_TYPES_H */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if you have the <wincodec.h> header file. */
/* #undef HAVE_WINCODEC_H */

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
/* #undef LT_OBJDIR ".libs/" */

/* Name of package */
#define PACKAGE "libwebp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://bugs.chromium.org/p/webp"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libwebp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libwebp 1.2.4"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libwebp"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://developers.google.com/speed/webp"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.2.4"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
/* #undef STDC_HEADERS */

/* Version number of package */
#define VERSION "1.2.4"

/* Enable experimental code */
/* #undef WEBP_EXPERIMENTAL_FEATURES */

/* Define to 1 to force aligned memory operations */
/* #undef WEBP_FORCE_ALIGNED */

/* Set to 1 if AVX2 is supported */
/* #undef WEBP_HAVE_AVX2 */

/* Set to 1 if GIF library is installed */
/* #undef WEBP_HAVE_GIF */

/* Set to 1 if OpenGL is supported */
/* #undef WEBP_HAVE_GL */

/* Set to 1 if JPEG library is installed */
/* #undef WEBP_HAVE_JPEG */

/* Set to 1 if NEON is supported */
/* #undef WEBP_HAVE_NEON */

/* Set to 1 if runtime detection of NEON is enabled */
/* #undef WEBP_HAVE_NEON_RTCD */

/* Set to 1 if PNG library is installed */
/* #undef WEBP_HAVE_PNG */

/* Set to 1 if SDL library is installed */
/* #undef WEBP_HAVE_SDL */

/* Set to 1 if SSE2 is supported */
/* #undef WEBP_HAVE_SSE2 */

/* Set to 1 if SSE4.1 is supported */
/* #undef WEBP_HAVE_SSE41 */

/* Set to 1 if TIFF library is installed */
/* #undef WEBP_HAVE_TIFF */

/* Enable near lossless encoding */
/* #undef WEBP_NEAR_LOSSLESS */

/* Undefine this to disable thread support. */
#define WEBP_USE_THREAD 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
/* #if defined AC_APPLE_UNIVERSAL_BUILD */
/* # if defined __BIG_ENDIAN__ */
/* #  define WORDS_BIGENDIAN 1 */
/* # endif */
/* #else */
/* # ifndef WORDS_BIGENDIAN */
/* /* #  undef WORDS_BIGENDIAN */
/* # endif */
/* #endif */
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
#define WORDS_BIGENDIAN 1
#endif
