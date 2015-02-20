#include <stdlib.h>
#include <assert.h>
#include "string.h"

int main(void)
{
   /* TEST: string tests */
   {
      struct chck_string str = {0};
      assert(chck_string_set_cstr(&str, "foobar", false));
      assert(str.data && str.size == strlen("foobar") && chck_string_eq_cstr(&str, "foobar"));
      assert(chck_string_set_cstr(&str, "", false));
      assert(!str.data && str.size == strlen("") && chck_string_eq_cstr(&str, ""));
      assert(!str.data && str.size == 0 && chck_string_eq_cstr(&str, NULL)); // string_eq_cstr treats empty string as NULL
      assert(chck_string_set_cstr(&str, NULL, false));
      assert(!str.data && str.size == 0 && !chck_string_eq_cstr(&str, "foobar"));

      // is_heap is false, so nothing is copied.
      // since chck_strings do not copy when is_heap is false, printf for .data would print whole "foobar" in this case.
      // if is_heap is set to true, the string is copied and terminated correctly.
      assert(chck_string_set_cstr_with_length(&str, "foobar", 3, false));
      assert(str.data && str.size == strlen("foo") && chck_string_eq_cstr(&str, "foo") && !chck_string_eq_cstr(&str, "foobar"));

      struct chck_string str2 = {0};
      assert(chck_string_set(&str2, &str, false));
      assert(str.data == str2.data && str2.size == strlen("foo") && chck_string_eq(&str, &str2));
      assert(chck_string_set(&str2, &str, true));
      assert(str.data != str2.data && str2.size == strlen("foo") && chck_string_eq(&str, &str2));
      assert(chck_string_set_cstr(&str2, "foobar2", false));
      assert(str.data != str2.data && str2.size == strlen("foobar2") && !chck_string_eq(&str, &str2));

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
   }
   return EXIT_SUCCESS;
}
