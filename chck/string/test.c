#include "string.h"
#include <chck/math/math.h>
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
   /* TEST: stripping */
   {
      struct chck_string v = {0};
      assert(chck_string_set_cstr(&v, "   contains some whitespace     ", true));
      assert(chck_cstr_strip(v.data) == v.data + 3);
      assert(strlen(v.data + 3) == 24);
      assert(chck_cstreq(v.data + 3, "contains some whitespace"));

      assert(chck_string_set_cstr(&v, "asd", true));
      assert(chck_cstr_strip(v.data) == v.data);
      assert(strlen(v.data) == 3);
      assert(chck_cstreq(v.data, "asd"));

      assert(chck_string_set_cstr(&v, "foo baz lol", true));
      assert(chck_cstr_remove_chars(v.data, "baz") == v.data);
      assert(chck_cstreq(v.data, "foo  lol"));
      chck_string_release(&v);
   }

   /* TEST: tokenizing */
   {
      const char *v = "  token: this :please  ";

      {
         size_t len, i = 0;
         const char *t, *state = NULL;
         const char *except[] = { "  token", " this ", "please  " };
         while ((t = chck_cstr_tokenize(v, &len, ":", false, &state))) {
            assert(i < 3);
            assert(len == strlen(except[i]));
            assert(chck_cstrneq(except[i], t, len));
            ++i;
         }
         assert(i == 3);
      }

      {
         size_t len, i = 0;
         const char *t, *state = NULL;
         const char *except[] = { "token", "this", "please" };
         while ((t = chck_cstr_tokenize(v, &len, ":", true, &state))) {
            assert(i < 3);
            assert(len == strlen(except[i]));
            assert(chck_cstrneq(except[i], t, len));
            ++i;
         }
         assert(i == 3);
      }

      v = "some : words : \"  but this is included  \" : \"yay : yoy\" : \"foo\"";

      {
         size_t len, i = 0;
         const char *t, *state = NULL;
         const char *except[] = { "some", "words", "  but this is included  ", "yay : yoy", "foo" };
         while ((t = chck_cstr_tokenize_quoted(v, &len, ":", "\"", &state))) {
            assert(i < 5);
            assert(len == strlen(except[i]));
            assert(chck_cstrneq(except[i], t, len));
            ++i;
         }
         assert(i == 5);
      }

      v = "some words  \"  but this is included  \" \"yay  yoy\"  \"foo\"";

      {
         size_t len, i = 0;
         const char *t, *state = NULL;
         const char *except[] = { "some", "words", "  but this is included  ", "yay  yoy", "foo" };
         while ((t = chck_cstr_tokenize_quoted(v, &len, " ", "\"", &state))) {
            assert(i < 5);
            assert(len == strlen(except[i]));
            assert(chck_cstrneq(except[i], t, len));
            ++i;
         }
         assert(i == 5);
      }
   }

   /* TEST: bool conversion tests */
   {
      bool v;
      assert(chck_cstr_to_bool("true", &v) && v == true);
      assert(chck_cstr_to_bool("false", &v) && v == false);
      assert(chck_cstr_to_bool("1", &v) && v == true);
      assert(chck_cstr_to_bool("0", &v) && v == false);
      assert(!chck_cstr_to_bool("falsef", NULL));
      assert(!chck_cstr_to_bool("fals", NULL));
      assert(!chck_cstr_to_bool("truee", NULL));
      assert(!chck_cstr_to_bool("tru", NULL));
      assert(!chck_cstr_to_bool("not-a-bool", NULL));
      assert(!chck_cstr_to_bool("5", NULL));
   }

   /* TEST: float conversion tests */
   {
      float v;
      assert(chck_cstr_to_f("0.123", &v) && chck_equalf(v, 0.123, 1.0f));
      assert(chck_cstr_to_f("0.1e2", &v) && chck_equalf(v, 0.1e2, 1.0f));
      assert(!chck_cstr_to_f("0.1e1000", NULL));
      assert(!chck_cstr_to_f("not-float", NULL));
   }

   /* TEST: double conversion tests */
   {
      double v;
      assert(chck_cstr_to_d("0.123", &v) && chck_equal(v, 0.123, 1.0));
      assert(chck_cstr_to_d("0.1e100", &v) && chck_equal(v, 0.1e100, 1.0));
      assert(!chck_cstr_to_d("0.1e1000", NULL));
      assert(!chck_cstr_to_d("not-double", NULL));
   }

   /* TEST: long double conversion tests */
   {
      long double v; // valgrind does not handle long double so we don't do 1e1000 here
      assert(chck_cstr_to_ld("0.123", &v) && chck_equalld(v, 0.123l, 1.0l));
      assert(chck_cstr_to_ld("0.1e100", &v) && chck_equalld(v, 0.1e100l, 1.0));
      assert(!chck_cstr_to_ld("0.1e100000", NULL));
      assert(!chck_cstr_to_ld("not-long-double", NULL));
   }

   /* TEST: i64 conversion tests */
   {
      int64_t v;
      assert(chck_cstr_to_i64("1", &v) && v == 1);
      assert(chck_cstr_to_i64("-1", &v) && v == -1);
      assert(chck_cstr_to_i64("9223372036854775807", &v) && v == INT64_MAX);
      assert(chck_cstr_to_i64("-9223372036854775808", &v) && v == INT64_MIN);
      assert(!chck_cstr_to_i64("9223372036854775808", &v));
      assert(!chck_cstr_to_i64("-9223372036854775809", &v));
      assert(!chck_cstr_to_i64("1.0", &v));

      uint64_t uv;
      assert(chck_cstr_to_u64("1", &uv) && uv == 1);
      assert(!chck_cstr_to_u64("-1", &uv));
      assert(chck_cstr_to_u64("18446744073709551615", &uv) && uv == UINT64_MAX);
      assert(!chck_cstr_to_u64("18446744073709551616", &uv));
      assert(!chck_cstr_to_u64("1.0", &uv));
   }

   /* TEST: i32 conversion tests */
   {
      int32_t v;
      assert(chck_cstr_to_i32("1", &v) && v == 1);
      assert(chck_cstr_to_i32("-1", &v) && v == -1);
      assert(chck_cstr_to_i32("2147483647", &v) && v == INT32_MAX);
      assert(chck_cstr_to_i32("-2147483648", &v) && v == INT32_MIN);
      assert(!chck_cstr_to_i32("2147483648", &v));
      assert(!chck_cstr_to_i32("-2147483649", &v));
      assert(!chck_cstr_to_i32("1.0", &v));

      uint32_t uv;
      assert(chck_cstr_to_u32("1", &uv) && uv == 1);
      assert(!chck_cstr_to_u32("-1", &uv));
      assert(chck_cstr_to_u32("4294967295", &uv) && uv == UINT32_MAX);
      assert(!chck_cstr_to_u32("4294967296", &uv));
      assert(!chck_cstr_to_u32("1.0", &uv));
   }

   /* TEST: i16 conversion tests */
   {
      int16_t v;
      assert(chck_cstr_to_i16("1", &v) && v == 1);
      assert(chck_cstr_to_i16("-1", &v) && v == -1);
      assert(chck_cstr_to_i16("32767", &v) && v == INT16_MAX);
      assert(chck_cstr_to_i16("-32768", &v) && v == INT16_MIN);
      assert(!chck_cstr_to_i16("32768", &v));
      assert(!chck_cstr_to_i16("-32769", &v));
      assert(!chck_cstr_to_i16("1.0", &v));

      uint16_t uv;
      assert(chck_cstr_to_u16("1", &uv) && uv == 1);
      assert(!chck_cstr_to_u16("-1", &uv));
      assert(chck_cstr_to_u16("65535", &uv) && uv == UINT16_MAX);
      assert(!chck_cstr_to_u16("65536", &uv));
      assert(!chck_cstr_to_u16("1.0", &uv));
   }

   /* TEST: i8 conversion tests */
   {
      int8_t v;
      assert(chck_cstr_to_i8("1", &v) && v == 1);
      assert(chck_cstr_to_i8("-1", &v) && v == -1);
      assert(chck_cstr_to_i8("127", &v) && v == INT8_MAX);
      assert(chck_cstr_to_i8("-128", &v) && v == INT8_MIN);
      assert(!chck_cstr_to_i8("128", &v));
      assert(!chck_cstr_to_i8("-129", &v));
      assert(!chck_cstr_to_i8("1.0", &v));

      uint8_t uv;
      assert(chck_cstr_to_u8("1", &uv) && uv == 1);
      assert(!chck_cstr_to_u8("-1", &uv));
      assert(chck_cstr_to_u8("255", &uv) && uv == UINT8_MAX);
      assert(!chck_cstr_to_u8("257", &uv));
      assert(!chck_cstr_to_u8("1.0", &uv));
   }

   /* TEST: string tests */
   {
      struct chck_string str = {0};
      assert(chck_string_set_cstr(&str, "foobar", false));
      assert(str.data && str.size == strlen("foobar") && chck_string_eq_cstr(&str, "foobar"));
      assert(chck_string_ends_with_cstr(&str, "") && chck_string_ends_with_cstr(&str, NULL) && chck_string_ends_with_cstr(&str, "foobar") && chck_string_ends_with_cstr(&str, "bar") && !chck_string_ends_with_cstr(&str, "bur"));
      assert(chck_string_starts_with_cstr(&str, "") && chck_string_starts_with_cstr(&str, NULL) && chck_string_starts_with_cstr(&str, "foobar") && chck_string_starts_with_cstr(&str, "foo") && !chck_string_starts_with_cstr(&str, "fuo"));
      assert(!chck_string_is_empty(&str));
      assert(chck_string_set_cstr(&str, "", false));
      assert(!str.data && str.size == strlen("") && chck_string_eq_cstr(&str, ""));
      assert(!str.data && str.size == 0 && chck_string_eq_cstr(&str, NULL)); // string_eq_cstr treats empty string as NULL
      assert(chck_string_ends_with_cstr(&str, "") && chck_string_ends_with_cstr(&str, NULL) && !chck_string_ends_with_cstr(&str, "foobar"));
      assert(chck_string_starts_with_cstr(&str, "") && chck_string_starts_with_cstr(&str, NULL) && !chck_string_starts_with_cstr(&str, "foobar"));
      assert(chck_string_is_empty(&str));
      assert(chck_string_set_cstr(&str, NULL, false));
      assert(!str.data && str.size == 0 && !chck_string_eq_cstr(&str, "foobar"));
      assert(chck_string_ends_with_cstr(&str, "") && chck_string_ends_with_cstr(&str, NULL) && !chck_string_ends_with_cstr(&str, "foobar"));
      assert(chck_string_starts_with_cstr(&str, "") && chck_string_starts_with_cstr(&str, NULL) && !chck_string_starts_with_cstr(&str, "foobar"));
      assert(chck_string_is_empty(&str));

      // is_heap is false, so nothing is copied.
      // since chck_strings do not copy when is_heap is false, printf for .data would print whole "foobar" in this case.
      // if is_heap is set to true, the string is copied and terminated correctly.
      assert(chck_string_set_cstr_with_length(&str, "foobar", 3, false));
      assert(str.data && str.size == strlen("foo") && chck_string_eq_cstr(&str, "foo") && !chck_string_eq_cstr(&str, "foobar"));

      struct chck_string str2 = {0};
      assert(chck_string_set(&str2, &str, false));
      assert(str.data == str2.data && str2.size == strlen("foo") && chck_string_eq(&str, &str2));
      assert(chck_string_ends_with(&str, &str2));
      assert(chck_string_starts_with(&str, &str2));
      assert(chck_string_set(&str2, &str, true));
      assert(str.data != str2.data && str2.size == strlen("foo") && chck_string_eq(&str, &str2));
      assert(chck_string_ends_with(&str, &str2));
      assert(chck_string_starts_with(&str, &str2));
      assert(chck_string_set_cstr(&str2, "foobar2", false));
      assert(str.data != str2.data && str2.size == strlen("foobar2") && !chck_string_eq(&str, &str2));
      assert(!chck_string_ends_with(&str, &str2));
      assert(!chck_string_starts_with(&str, &str2));

      assert(chck_string_set_format(&str, "%s-%s", "test", str2.data));
      assert(str.data && str.size == strlen("test-foobar2") && str.is_heap && chck_string_eq_cstr(&str, "test-foobar2"));

      chck_string_release(&str);
      chck_string_release(&str2);
      assert(!str.data && !str2.data && str.size + str2.size == 0);
      assert(chck_string_eq(&str, &str2));

      assert(chck_cstreq("foobar", "foobar"));
      assert(!chck_cstreq("foobar", "foo"));
      assert(!chck_cstreq("foobar", NULL));
      assert(chck_cstreq(NULL, NULL));

      assert(chck_cstrneq("foobar", "foo", 3));
      assert(chck_cstrneq("fo", "fo", 3));
      assert(!chck_cstrneq("foobar", "foa", 3));
      assert(chck_cstrneq(NULL, NULL, 3));
      assert(chck_cstrneq(NULL, NULL, 0));

      assert(!chck_cstr_is_empty("foobar"));
      assert(chck_cstr_ends_with("foobar", "foobar") && chck_cstr_ends_with("foobar", "bar") && !chck_cstr_ends_with("foobar", "bur"));
      assert(chck_cstr_starts_with("foobar", "foobar") && chck_cstr_starts_with("foobar", "foo") && !chck_cstr_starts_with("foobar", "fur"));
      assert(chck_cstr_is_empty("") && chck_cstr_is_empty(NULL));
      assert(chck_cstr_ends_with("", "") && chck_cstr_ends_with(NULL, NULL));
      assert(chck_cstr_starts_with("", "") && chck_cstr_starts_with(NULL, NULL));
   }
   return EXIT_SUCCESS;
}
