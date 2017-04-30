#ifndef __chck_bams_h__
#define __chck_bams_h__

#include <chck/macros.h>
#include <stdint.h>

typedef int64_t bams64;
typedef int32_t bams32;
typedef int16_t bams16;
typedef int8_t bams8;

#define decl_bams_func(B, R, T) \
   static inline R B##_from_##T(T x, T range) { return (x / range) * (B##_most + B##_erro); } \
   static inline T T##_from_##B(R x, T range) { return (x / (T)(B##_most + B##_erro)) * range; }

#define decl_bams(B, R) \
   static const R B##_erro = (sizeof(B) < sizeof(uint32_t)); \
   static const R B##_most = (B)~0; \
   static const R B##_half = ((B)~0 >> 1) + 1; \
   decl_bams_func(B, R, double) \
   decl_bams_func(B, R, float)

decl_bams(bams64, uint64_t)
decl_bams(bams32, uint32_t)
decl_bams(bams16, uint16_t)
decl_bams(bams8, uint8_t)

#undef decl_bams_func
#undef decl_bams

#endif /* __chck_bams_h__ */
