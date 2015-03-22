#ifndef __chck_math_h__
#define __chck_math_h__

#include "macros.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <float.h>

/** <https://www.gnu.org/software/libc/manual/html_node/Mathematical-Constants.html> */

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#  define M_PI_2 1.57079632679489661923
#endif

#ifndef M_PI_4
#  define M_PI_4 0.78539816339744830962
#endif

#ifndef M_SQRT2
#  define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_SQRT1_2
#  define  M_SQRT1_2 0.70710678118654752440
#endif

#ifndef M_SQRT3
#  define M_SQRT3 1.73205080756887729352
#endif

#ifndef M_SQRT1_3
#  define M_SQRT1_3 0.57735026918962576450
#endif

#ifndef M_1_PI
#  define M_1_PI 0.318309886183790671538
#endif

#ifndef M_E
#  define M_E 2.7182818284590452354
#endif

#ifndef M_LOG2E
#  define M_LOG2E 1.4426950408889634074
#endif

#ifndef M_LOG10E
#  define M_LOG10E 0.43429448190325182765
#endif

#ifndef M_LN2
#  define M_LN2 0.69314718055994530942
#endif

#ifndef M_LN10
#  define M_LN10 2.30258509299404568402
#endif

#if defined(__GNUC__)
#  define NAN_FLT __builtin_nanf("")
#else
#  define NAN_FLT (*((float*)(&(int){0x7FC00000})))
#endif

/** declare generic math functions */

// T = type name, n = function suffix
#define decl_generics_for_type(T, n) \
   CHCK_CONST static inline T chck_min##n(T a, T b) { return (a < b ? a : b); } \
   CHCK_CONST static inline T chck_max##n(T a, T b) { return (a > b ? a : b); } \
   CHCK_CONST static inline T chck_clamp##n(T a, T min, T max) { return (chck_min##n(chck_max##n(a, min), max)); }

// T = signed type, FT = floating point type, n = signed function suffix
#define decl_signed_generics(T, FT, n) \
   decl_generics_for_type(T, n) \
   CHCK_CONST static inline T chck_modn##n(T x, T m) { return x - m * round((FT)x / (FT)m); } // modulus rounding to nearest int (-m/2, +m/2 range)

// UT = unsigned type, un = unsigned function suffix
// T = signed type, FT = floating point type, n = signed function suffix
#define decl_generics(UT, un, T, FT, n) \
   decl_generics_for_type(UT, un) \
   decl_signed_generics(T, FT, n)

decl_generics_for_type(size_t, sz)
decl_generics(uint64_t, u64, int64_t, double, 64)
decl_generics(uint32_t, u32, int32_t, float, 32)
decl_generics(uint16_t, u16, int16_t, float, 16)
decl_generics(uint8_t, u8, int8_t, float, 8)
decl_signed_generics(double, double, )
decl_signed_generics(float, float, f)

#undef decl_generics
#undef decl_signed_generics
#undef decl_generics_for_type

/** floating point almost equality checks */

CHCK_CONST static inline bool chck_equal(double a, double b, double error) {
   return (fabs(a - b) < error * DBL_EPSILON * fabs(a + b) || fabs(a - b) < DBL_MIN);
}

CHCK_CONST static inline bool chck_equalf(float a, float b, float error) {
   return (fabsf(a - b) < error * FLT_EPSILON * fabsf(a + b) || fabsf(a - b) < FLT_MIN);
}

#endif /* __chck_math_h__ */
