#include "string.h"
#include "overflow/overflow.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

static inline char*
ccopy(const char *str, size_t len)
{
   assert(str);
   char *cpy = chck_calloc_add_of(len, 1);
   return (cpy ? memcpy(cpy, str, len) : NULL);
}

void
chck_string_release(struct chck_string *string)
{
   if (!string)
      return;

   if (string->is_heap)
      free(string->data);

   memset(string, 0, sizeof(struct chck_string));
}

bool
chck_string_set_cstr_with_length(struct chck_string *string, const char *data, size_t len, bool is_heap)
{
   assert(string);

   char *copy = (char*)data;
   if (is_heap && data && len > 0 && !(copy = ccopy(data, len)))
      return false;

   chck_string_release(string);
   string->is_heap = is_heap;
   string->data = (len > 0 ? copy : NULL);
   string->size = len;
   return true;
}

bool
chck_string_set_cstr(struct chck_string *string, const char *data, bool is_heap)
{
   assert(string);
   return chck_string_set_cstr_with_length(string, data, (data ? strlen(data) : 0), is_heap);
}

bool
chck_string_set_varg(struct chck_string *string, const char *fmt, va_list args)
{
   assert(string && fmt);

   va_list cpy;
   va_copy(cpy, args);

   char *str = NULL;
   const size_t len = vsnprintf(NULL, 0, fmt, args);
   if (len > 0 && !(str = chck_malloc_add_of(len, 1)))
      return false;

   vsnprintf(str, len + 1, fmt, cpy);

   chck_string_release(string);
   string->is_heap = true;
   string->data = (len > 0 ? str : NULL);
   string->size = len;
   return true;
}

bool
chck_string_set_format(struct chck_string *string, const char *fmt, ...)
{
   va_list argp;
   va_start(argp, fmt);
   const bool ret = chck_string_set_varg(string, fmt, argp);
   va_end(argp);
   return ret;
}

bool
chck_string_set(struct chck_string *string, const struct chck_string *other, bool is_heap)
{
   assert(string && other);

   if (!is_heap && string->data == other->data) {
      string->size = other->size;
      return true;
   }

   return chck_string_set_cstr_with_length(string, other->data, other->size, is_heap);
}
