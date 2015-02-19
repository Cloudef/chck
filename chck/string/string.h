#ifndef __chck_string_h__
#define __chck_string_h__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct chck_string {
   char *data;
   size_t size;
   bool is_heap;
};

static inline bool
chck_string_eq(const struct chck_string *a, const struct chck_string *b)
{
   return (a->data == b->data) || (a->size == b->size && !memcmp(a->data, b->data, a->size));
}

static inline bool
chck_string_eq_cstr(const struct chck_string *a, const char *cstr)
{
   size_t len = (cstr ? strlen(cstr) : 0);
   return (len == a->size) && (cstr == a->data || !memcmp(a->data, cstr, a->size));
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

void chck_string_release(struct chck_string *string);
bool chck_string_set_cstr(struct chck_string *string, const char *data, bool is_heap);
bool chck_string_set_cstr_with_length(struct chck_string *string, const char *data, size_t len, bool is_heap);
bool chck_string_set(struct chck_string *string, const struct chck_string *other, bool is_heap);

#endif /* __chck_string_h__ */
