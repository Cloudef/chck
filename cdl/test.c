#include "cdl.h"
#include <stdlib.h>
#include <assert.h>

int main(void)
{
   /* TEST: library loading */
   {
      void *handle = NULL;
      const char *error = NULL;

#if __unix__
      assert((handle = chckDlLoad("/usr/lib/libdl.so", &error)) != NULL);
      assert(error == NULL);
      assert(chckDlLoadSymbol(handle, "dlsym", &error) != NULL);
      assert(error == NULL);
#elif defined(_WIN32) || defined(_WIN64)
      assert((handle = chckDlLoad("C:/windows/system32/kernel32.dll", &error)) != NULL);
      assert(error == NULL);
      assert(chckDlLoadSymbol(handle, "GetNativeSystemInfo", &error) != NULL);
      assert(error == NULL);
#endif

      assert(chckDlLoadSymbol(handle, "iWantSomeFrenchFriesWithHotChickenPlease", &error) == NULL);
      assert(error != NULL);

      assert(chckDlLoadSymbol(handle, "iWantSomeFrenchFriesWithHotChickenPlease", NULL) == NULL);
      assert(chckDlLoad("iWantSomeFrenchFriesWithHotChickenPlease", NULL) == NULL);
      chckDlUnload(handle);
   }
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
