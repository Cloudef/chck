#include "fs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
   /* TEST: executable path */
   {
      char *path = chck_get_executable_path();
      assert(path != NULL);
#if __unix__
      assert(path[0] == '/');
      assert(!strcmp(path+strlen(path)-strlen("fs_test"), "fs_test"));
#elif defined(_WIN32) || defined(_WIN64)
      assert(path[1] == ':');
      assert(!strcmp(path+strlen(path)-strlen("fs_test.exe"), "fs_test.exe"));
#endif
      printf("%s\n", path);
      free(path);
   }

   /* TEST: basename */
   {
      assert(!strcmp(chck_basename("foo/bar"), "bar"));
      assert(!strcmp(chck_basename("foo"), "foo"));
      assert(!strcmp(chck_basename("foo/"), ""));
      assert(strcmp(chck_basename("foo/"), "foo"));
      assert(strcmp(chck_basename("foo/"), "/"));
      assert(!strcmp(chck_basename(""), ""));
      assert(strcmp(chck_basename("foo/bar"), "foo"));
   }

   /* TEST: safe filename */
   {
      assert(!chck_filename_is_safe(".."));
      assert(!chck_filename_is_safe("."));
      assert(!chck_filename_is_safe("foo/bar"));
      assert(chck_filename_is_safe("foo"));
      assert(chck_filename_is_safe("foo_01_bar"));

      char f[FILENAME_MAX + 2];
      memset(f, 'a', sizeof(f));
      f[FILENAME_MAX + 1] = 0;
      assert(!chck_filename_is_safe(f));
   }

   return EXIT_SUCCESS;
}
