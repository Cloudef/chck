#ifndef __chck_macros_h__
#define __chck_macros_h__

#if __GNUC__
#  if !defined(likely) && !defined(unlikely)
#     define likely(x) __builtin_expect(!!(x), 1)
#     define unlikely(x) __builtin_expect(!!(x), 0)
#  endif
#  define CHCK_FORMAT(f, x, y) __attribute__((format(f, x, y)))
#  define CHCK_MALLOC __attribute__((malloc))
#else
#  if !defined(likely) && !defined(unlikely)
#     define likely(x) !!(x)
#     define unlikely(x) !!(x)
#  endif
#  define CHCK_FORMAT(f, x, y)
#  define CHCK_MALLOC
#endif

#endif /* __chck_macros_h__ */
