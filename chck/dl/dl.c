#include "dl.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#elif defined(__posix__) || defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#  include <dlfcn.h>
#else
#  warning "dl: unsupported os"
#endif

void*
chck_dl_load(const char *file, const char **out_error)
{
   assert(file);

   void *handle = NULL;
   const char *error = NULL;
#if defined(_WIN32) || defined(_WIN64)
#if defined(__WINRT__)
   /**
    * WinRT only publically supports LoadPackagedLibrary() for loading .dll
    * files. LoadLibrary() is a private API, and not available for apps
    * (that can be published to MS' Windows Store.)
    */
   handle = (void*)LoadPackagedLibrary(file, 0);
#else
   handle = (void*)LoadLibrary(file);
#endif

   if (!handle)
      error = "Failed to load dll file.";
#elif defined(__posix__) || defined(__unix__) || defined(__linux__) || defined(__APPLE__)
   if (!(handle = dlopen(file, RTLD_NOW | RTLD_LOCAL)))
      error = dlerror();
#else
   error = "cdl: unsupported os";
#endif

   if (out_error)
      *out_error = error;

   return handle;
}

void*
chck_dl_load_symbol(void *handle, const char *name, const char **out_error)
{
   assert(handle);
   void *symbol = NULL;
   const char *error = NULL;

#if defined(_WIN32) || defined(_WIN64)
   if (!(symbol = (void*)GetProcAddress((HMODULE)handle, name)))
      error = "Failed to load symbol.";
#elif defined(__posix__) || defined(__unix__) || defined(__linux__) || defined(__APPLE__)
   if (!(symbol = dlsym(handle, name))) {
      const size_t len = strlen(name) + 1;
      char *nname = calloc(1, len + 1);

      if (nname) {
         /* append an underscore for platforms that need that. */
         nname[0] = '_';
         memcpy(nname + 1, name, len);
         symbol = dlsym(handle, nname);
         free(nname);
      }
   }

   if (!symbol)
      error = dlerror();
#else
   error = "cdl: unsupported os";
#endif

   if (out_error)
      *out_error = error;

   return symbol;
}

void
chck_dl_unload(void *handle)
{
   assert(handle);

#if defined(_WIN32) || defined(_WIN64)
   FreeLibrary((HMODULE)handle);
#elif defined(__posix__) || defined(__unix__) || defined(__linux__) || defined(__APPLE__)
   dlclose(handle);
#endif
}
