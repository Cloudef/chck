#include "dl.h"
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
   /* TEST: library loading */
   {
      void *handle = NULL;
      const char *error = NULL;

#if defined(__APPLE__)
      assert((handle = chck_dl_load(DL_LIBRARY, &error)) != NULL);
      assert(error == NULL);
      assert(chck_dl_load_symbol(handle, "dlsym", &error) != NULL);
      assert(error == NULL);
#elif __unix__
      assert((handle = chck_dl_load(DL_LIBRARY, &error)) != NULL);
      assert(error == NULL);
      assert(chck_dl_load_symbol(handle, "dlsym", &error) != NULL);
      assert(error == NULL);
#elif defined(_WIN32) || defined(_WIN64)
      assert((handle = chck_dl_load("C:/windows/system32/kernel32.dll", &error)) != NULL);
      assert(error == NULL);
      assert(chck_dl_load_symbol(handle, "GetNativeSystemInfo", &error) != NULL);
      assert(error == NULL);
#else
      assert((handle = chck_dl_load("unsupported.os", &error)) == NULL);
      assert(error != NULL);
      return EXIT_SUCCESS;
#endif

      assert(chck_dl_load_symbol(handle, "iWantSomeFrenchFriesWithHotChickenPlease", &error) == NULL);
      assert(error != NULL);

      assert(chck_dl_load_symbol(handle, "iWantSomeFrenchFriesWithHotChickenPlease", NULL) == NULL);
      assert(chck_dl_load("iWantSomeFrenchFriesWithHotChickenPlease", NULL) == NULL);
      chck_dl_unload(handle);
   }
   return EXIT_SUCCESS;
}
