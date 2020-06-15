/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
#undef AC_APPLE_UNIVERSAL_BUILD

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the `getisax' function. */
#undef HAVE_GETISAX

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Whether we have posix_memalign() */
#undef HAVE_POSIX_MEMALIGN

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

   /* Name of package */
#define PACKAGE WM_1

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME 

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "WM_1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "W"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1"

/* enable TIMER_BEGIN/TIMER_END macros */
#undef PIXMAN_TIMERS

/* The size of `long', as computed by sizeof. */
#undef SIZEOF_LONG

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* use ARM NEON assembly optimizations */
#undef USE_ARM_NEON

/* use ARM SIMD compiler intrinsics */
#undef USE_ARM_SIMD

/* use GNU-style inline assembler */
#undef USE_GCC_INLINE_ASM

/* use MMX compiler intrinsics */
#undef USE_MMX

/* use SSE2 compiler intrinsics */
#undef USE_SSE2

/* use VMX compiler intrinsics */
#undef USE_VMX

/* Version number of package */
#undef VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

   /* Define to `__inline__' or `__inline' if that's what the C compiler
	  calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif
