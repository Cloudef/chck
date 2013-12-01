#include "fs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
   /* TEST: executable path */
   {
      char *path = chckGetExecutablePath();
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
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
