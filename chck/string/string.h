#ifndef __chck_string_h__
#define __chck_string_h__

#include <chck/macros.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define CSTRE(x) (x ? x : "")

struct chck_string {
   char *data;
   size_t size;
   bool is_heap;
};

static inline bool
chck_cstr_is_empty(const char *data)
{
   return (!data || *data == 0);
}

static inline bool
chck_cstr_ends_with(const char *a, const char *b)
{
   const size_t lena = (a ? strlen(a) : 0), lenb = (b ? strlen(b) : 0);
   return (lena >= lenb && !memcmp(a + lena - lenb, CSTRE(b), lenb));
}

static inline bool
chck_cstr_starts_with(const char *a, const char *b)
{
   const size_t lena = (a ? strlen(a) : 0), lenb = (b ? strlen(b) : 0);
   return (lena >= lenb && !memcmp(CSTRE(a), CSTRE(b), lenb));
}

static inline bool
chck_cstreq(const char *a, const char *b)
{
   return (a == b) || (a && b && !strcmp(a, b));
}

static inline bool
chck_cstrneq(const char *a, const char *b, size_t len)
{
   return (a == b) || (a && b && !strncmp(a, b, len));
}

static inline bool
chck_string_is_empty(const struct chck_string *string)
{
   return chck_cstr_is_empty(string->data);
}

static inline bool
chck_string_ends_with_cstr(const struct chck_string *a, const char *cstr)
{
   const size_t len = (cstr ? strlen(cstr) : 0);
   return (a->size >= len && !memcmp(a->data + a->size - len, CSTRE(cstr), len));
}

static inline bool
chck_string_starts_with_cstr(const struct chck_string *a, const char *cstr)
{
   const size_t len = (cstr ? strlen(cstr) : 0);
   return (a->size >= len && !memcmp(a->data, CSTRE(cstr), len));
}

static inline bool
chck_string_ends_with(const struct chck_string *a, const struct chck_string *b)
{
   return (a->size >= b->size && !memcmp(a->data + a->size - b->size, CSTRE(b->data), b->size));
}

static inline bool
chck_string_starts_with(const struct chck_string *a, const struct chck_string *b)
{
   return (a->size >= b->size && !memcmp(CSTRE(a->data), CSTRE(b->data), b->size));
}

static inline bool
chck_string_eq(const struct chck_string *a, const struct chck_string *b)
{
   return (a->data == b->data) || (a->size == b->size && !memcmp(CSTRE(a->data), CSTRE(b->data), a->size));
}

static inline bool
chck_string_eq_cstr(const struct chck_string *a, const char *cstr)
{
   const size_t len = (cstr ? strlen(cstr) : 0);
   return (len == a->size) && (cstr == a->data || !memcmp(CSTRE(a->data), CSTRE(cstr), a->size));
}

#undef CSTRE

void chck_string_release(struct chck_string *string);
CHCK_NONULLV(1) bool chck_string_set_cstr(struct chck_string *string, const char *data, bool is_heap);
CHCK_NONULLV(1) bool chck_string_set_cstr_with_length(struct chck_string *string, const char *data, size_t len, bool is_heap);
CHCK_NONULL bool chck_string_set(struct chck_string *string, const struct chck_string *other, bool is_heap);
CHCK_NONULL CHCK_FORMAT(printf, 2, 3) bool chck_string_set_format(struct chck_string *string, const char *fmt, ...);
CHCK_NONULL bool chck_string_set_varg(struct chck_string *string, const char *fmt, va_list args);

#define decl_int_conv(T, PT, n, fun, range) \
static inline bool chck_cstr_to_##n(const char *data, T *out) { \
   if (chck_cstr_is_empty(data)) \
      return false; \
   char *end; \
   errno = 0; \
   const PT num = fun(data, &end, 10); \
   if (!end || *end != 0 || errno == ERANGE || errno == EINVAL || (range)) \
      return false; \
   if (out) \
      *out = num; \
   return true; \
}

#define decl_flt_conv(T, n, fun) \
static inline bool chck_cstr_to_##n(const char *data, T *out) { \
   if (chck_cstr_is_empty(data)) \
      return false; \
   char *end; \
   errno = 0; \
   const T num = fun(data, &end); \
   if (!end || *end != 0 || errno == ERANGE) \
      return false; \
   if (out) \
      *out = num; \
   return true; \
}

decl_int_conv(uint32_t, unsigned long int, u32, strtoul, num > UINT32_MAX);
decl_int_conv(uint16_t, unsigned long int, u16, strtoul, num > UINT16_MAX);
decl_int_conv(uint8_t, unsigned long int, u8, strtoul, num > UINT8_MAX);
decl_int_conv(int64_t, long long int, i64, strtoll, num > INT64_MAX || num < INT64_MIN);
decl_int_conv(int32_t, long int, i32, strtol, num > INT32_MAX || num < INT32_MIN);
decl_int_conv(int16_t, long int, i16, strtol, num > INT16_MAX || num < INT16_MIN);
decl_int_conv(int8_t, long int, i8, strtol, num > INT8_MAX || num < INT8_MIN);

decl_flt_conv(double, d, strtod);
decl_flt_conv(float, f, strtof);
decl_flt_conv(long double, ld, strtold);

#undef decl_int_conv
#undef decl_flt_conv

static inline bool
chck_cstr_to_u64(const char *data, uint64_t *out)
{
   if (chck_cstr_is_empty(data) || strcspn(data, "-") == strspn(data, " "))
      return false;

   char *end;
   errno = 0;
   const unsigned long long num = strtoull(data, &end, 10);
   if (!end || *end != 0 || errno == ERANGE)
      return false;

   if (out)
      *out = num;

   return true;
}

static inline bool
chck_cstr_to_bool(const char *data, bool *out)
{
   if (!data || (!chck_cstreq(data, "true") && !chck_cstreq(data, "false")))
      return false;

   if (out)
      *out = chck_cstreq(data, "true");

   return true;
}

#endif /* __chck_string_h__ */
