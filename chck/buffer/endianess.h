#ifndef __chck_endianess__
#define __chck_endianess__

#include <stdint.h>
#include <assert.h>
#include <string.h>

#if __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) !!(x)
#define unlikely(x) !!(x)
#endif

enum chck_endianess {
   CHCK_ENDIANESS_LITTLE = 0,
   CHCK_ENDIANESS_BIG    = 1,
   CHCK_ENDIANESS_NATIVE = 2,
};

#if defined(__clang__) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3 && !defined(__MINGW32__) && !defined(__MINGW64__))
#  define bswap16 __builtin_bswap16
#  define bswap32 __builtin_bswap32
#  define bswap64 __builtin_bswap64
#  define HAS_BYTESWAP 1
#elif defined(__GLIBC__)
#  include <byteswap.h>
#  define bswap16 __bswap_16
#  define bswap32 __bswap_32
#  define bswap64 __bswap_64
#  define HAS_BYTESWAP 1
#elif defined(__NetBSD__)
#  include <sys/types.h>
#  include <machine/bswap.h> /* already named bswap16/32/64 */
#  define HAS_BYTESWAP 1
#elif defined(_MSC_VER)
#  define bswap16 _byteswap_ushort
#  define bswap32 _byteswap_ulong
#  define bswap64 _byteswap_uint64
#  define HAS_BYTESWAP 1
#else
#  define HAS_BYTESWAP 0
#endif

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
    (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
// compiletime big endian
#  define chck_endianess() CHCK_ENDIANESS_BIG
#elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
    (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
// compiletime little endian
#  define chck_endianess() CHCK_ENDIANESS_LITTLE
#else
// runtime endianess check
static inline enum chck_endianess chck_endianess(void)
{
   union {
      uint32_t i;
      char c[4];
   } bint = { 0x01020304 };
   return (bint.c[0] == 1 ? CHCK_ENDIANESS_BIG : CHCK_ENDIANESS_LITTLE);
};
#endif

static inline void
chck_bswap(void *v, size_t size, size_t memb)
{
   if (unlikely(!v || size < 1))
      return;

   for (void *p = v; p < v + (memb * size); p += size) {
#if HAS_BYTESWAP
      if (size == sizeof(uint64_t)) *((uint64_t*)p) = bswap64(*((uint64_t*)p));
      if (size == sizeof(uint32_t)) *((uint32_t*)p) = bswap32(*((uint32_t*)p));
      if (size == sizeof(uint16_t)) *((uint16_t*)p) = bswap16(*((uint16_t*)p));
#else
      size_t s;
      uint8_t b[size];
      memcpy(b, p, size);
      for (s = 0; s < size; ++s) memset(p + s, b[size - s - 1], 1);
#endif
   }
}

#endif /* __chck_endianess__ */
