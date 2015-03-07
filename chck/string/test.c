#include <stdlib.h>
#include "string.h"

#undef NDEBUG
#include <assert.h>

int main(void)
{
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
