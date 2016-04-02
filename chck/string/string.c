#include "string.h"
#include <chck/overflow/overflow.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#define WHITESPACE " \t\n\v\f\r"

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

   *string = (struct chck_string){0};
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

bool
chck_string_set_varg(struct chck_string *string, const char *fmt, va_list args)
{
   assert(string && fmt);

   va_list cpy;
   va_copy(cpy, args);

   char *str = NULL;
   const size_t len = vsnprintf(NULL, 0, fmt, args);
   if (len > 0 && !(str = chck_malloc_add_of(len, 1))) {
      va_end(cpy);
      return false;
   }

   vsnprintf(str, len + 1, fmt, cpy);
   va_end(cpy);

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

#pragma GCC diagnostic pop

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

char*
chck_cstr_strip(char *cstr)
{
   assert(cstr);

   char *e;
   cstr += strspn(cstr, WHITESPACE);
   for (e = strchr(cstr, 0); e > cstr; --e)
      if (!strchr(WHITESPACE, e[-1]))
         break;

   *e = 0;
   return cstr;
}

char*
chck_cstr_remove_chars(char *cstr, const char *bad)
{
   assert(cstr && bad);

   char *t, *f;
   for (f = cstr, t = cstr; *f; ++f) {
      if (strchr(bad, *f))
         continue;

      *(t++) = *f;
   }

   *t = 0;
   return cstr;
}

char*
chck_cstr_replace_char(char *cstr, char replace, char with)
{
   assert(cstr && replace != with);

   if (replace == with)
      return cstr;

   char *s = cstr;
   while ((s = strchr(s, replace)))
      *s = with;
   return cstr;
}

const char*
chck_cstr_tokenize(const char *cstr, size_t *out_len, const char *separator, bool skip_whitespace, const char **state)
{
   assert(out_len && separator && state);
   const char *current = (state && *state ? *state : cstr);

   if (chck_cstr_is_empty(current) || chck_cstr_is_empty(cstr))
      return NULL;

   current += strspn(current, separator);

   if (skip_whitespace)
      current += strspn(current, WHITESPACE);

   *out_len = strcspn(current, separator);
   *state = current + *out_len;

   if (**state != 0)
      ++(*state);

   if (skip_whitespace) {
      const size_t ws = strcspn(current, WHITESPACE);
      *out_len -= (ws < *out_len ? *out_len - ws : 0);
   }

   return current;
}

const char*
chck_cstr_tokenize_quoted(const char *cstr, size_t *out_len, const char *separator, const char *quotes, const char **state)
{
   assert(out_len && separator && quotes && state);
   const char *e, *current = chck_cstr_tokenize(cstr, out_len, separator, true, state);

   if (!current)
      return NULL;

   for (const char *q = quotes; *q; ++q) {
      if (*current != *q)
         continue;

      bool escaped = false;
      for (e = ++current; *e; ++e) {
         if (escaped)
            escaped = false;
         else if (*e == '\\')
            escaped = true;
         else if (*e == *q)
            break;
      }

      *out_len = e - current;
      e = (!*e ? e : e + 1);

      if (*e) {
         size_t tmp;
         const char *state2 = NULL;
         *state = chck_cstr_tokenize(e, &tmp, separator, true, &state2);
      } else {
         *state = e;
      }

      break;
   }

   return current;
}
