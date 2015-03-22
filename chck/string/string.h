#ifndef __chck_string_h__
#define __chck_string_h__

#include "macros.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

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
   return (lena >= lenb && !memcmp(a + lena - lenb, b, lenb));
}

static inline bool
chck_cstr_starts_with(const char *a, const char *b)
{
   const size_t lena = (a ? strlen(a) : 0), lenb = (b ? strlen(b) : 0);
   return (lena >= lenb && !memcmp(a, b, lenb));
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
   return (a->size >= len && !memcmp(a->data + a->size - len, cstr, len));
}

static inline bool
chck_string_starts_with_cstr(const struct chck_string *a, const char *cstr)
{
   const size_t len = (cstr ? strlen(cstr) : 0);
   return (a->size >= len && !memcmp(a->data, cstr, len));
}

static inline bool
chck_string_ends_with(const struct chck_string *a, const struct chck_string *b)
{
   return (a->size >= b->size && !memcmp(a->data + a->size - b->size, b->data, b->size));
}

static inline bool
chck_string_starts_with(const struct chck_string *a, const struct chck_string *b)
{
   return (a->size >= b->size && !memcmp(a->data, b->data, b->size));
}

static inline bool
chck_string_eq(const struct chck_string *a, const struct chck_string *b)
{
   return (a->data == b->data) || (a->size == b->size && !memcmp(a->data, b->data, a->size));
}

static inline bool
chck_string_eq_cstr(const struct chck_string *a, const char *cstr)
{
   const size_t len = (cstr ? strlen(cstr) : 0);
   return (len == a->size) && (cstr == a->data || !memcmp(a->data, cstr, a->size));
}

void chck_string_release(struct chck_string *string);
CHCK_NONULLV(1) bool chck_string_set_cstr(struct chck_string *string, const char *data, bool is_heap);
CHCK_NONULLV(1) bool chck_string_set_cstr_with_length(struct chck_string *string, const char *data, size_t len, bool is_heap);
CHCK_NONULL bool chck_string_set(struct chck_string *string, const struct chck_string *other, bool is_heap);
CHCK_NONULL CHCK_FORMAT(printf, 2, 3) bool chck_string_set_format(struct chck_string *string, const char *fmt, ...);
CHCK_NONULL bool chck_string_set_varg(struct chck_string *string, const char *fmt, va_list args);

#endif /* __chck_string_h__ */
