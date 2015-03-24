#include <chck/overflow/overflow.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#  include <mach-o/dyld.h>
#endif

#if defined(BSD)
#  include <sys/param.h>
#  include <sys/sysctl.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
static inline char*
ccopy(const char *str)
{
   assert(str);
   size_t size = strlen(str);
   char *cpy = chck_calloc_add_of(size, 1);
   return (cpy ? memcpy(cpy, str, size) : NULL);
}
#endif

static inline bool
resize(char **buf, size_t *size, size_t nsize)
{
   assert(buf && size);

   if (nsize == *size)
      return true;

   void *tmp;
   if (!(tmp = realloc(*buf, nsize)))
      return false;

   *buf = tmp;
   *size = nsize;
   return true;
}

static char*
get_executable_path_from(const char *path)
{
   char *buf = NULL;
   ssize_t rsize;
   size_t size = 1024;

   // here be dragons

#if (defined(__APPLE__) && defined(__MACH__))
   (void)path;
   unsigned int bsize = 0;
   _NSGetExecutablePath(NULL, &bsize);
   if (bsize == 0) goto fail;
   if (!(buf = malloc(bsize))) goto fail;
   _NSGetExecutablePath(buf, &bsize);
   buf[bsize] = 0;
   size = rsize = bsize;
#elif defined(_WIN32) || defined(_WIN64)
   (void)path;
   if (!(buf = malloc(size))) goto fail;
   while ((size_t)(rsize = GetModuleFileName(NULL, buf, size)) > size) {
      if (rsize <= 0) goto fail;
      if (!resize(&buf, &size, size * 2)) goto fail;
   }
#else
   if (!(buf = malloc(size))) goto fail;
   while ((size_t)(rsize = readlink(path, buf, size)) > size) {
      if (rsize <= 0) goto fail;
      if (!resize(&buf, &size, size * 2)) goto fail;
   }
#endif

   if (rsize <= 0) goto fail;
   if (rsize != -1) {
      if (!resize(&buf, &size, (size_t)rsize + 1)) goto fail;
      buf[rsize] = 0;
   }

#if (defined(__APPLE__) && defined(__MACH__))
   char *tmp;
   if (!(tmp = realpath(buf, NULL))) goto fail;
   free(buf); buf = tmp;
#endif
   return buf;

fail:
   free(buf);
   return NULL;
}

char*
chck_get_executable_path(void)
{
   const char *path = NULL;
   char *exepath = NULL;

#if defined(EMSCRIPTEN)
   (void)path;
   (void)exepath;
   return NULL;
#elif defined(_WIN32) || defined(_WIN64)
   if (_pgmptr && !(exepath = ccopy(_pgmptr))) return NULL;
   if (exepath) return exepath;
#elif defined(BSD) /* works on all BSD's? */
   int mib[4];
   mib[0] = CTL_KERN;
   mib[1] = KERN_PROC;
   mib[2] = KERN_PROC_PATHNAME;
   mib[3] = -1;
   size_t cb = 0;
   sysctl(mib, 4, NULL, &cb, NULL, 0);
   if (cb > 0) {
      if (!(exepath = malloc(cb))) return NULL;
      sysctl(mib, 4, exepath, &cb, NULL, 0);
      return exepath;
   }
#endif

#if defined(__linux__)
   path = "/proc/self/exe";
#elif defined(__NetBSD__)
   path = "/proc/curproc/exe";
#elif defined(BSD)
   path = "/proc/curproc/file";
#elif defined(__sun)
   path = "/proc/self/path/a.out";
#elif defined(_WIN32) || defined(_WIN64)
   path = NULL;
#elif defined(__APPLE__) && defined(__MACH__)
   path = NULL;
#elif defined(EMSCRIPTEN)
   path = NULL;
#else
#  error insert your OS here
#endif

   /* free this when not needed */
   exepath = get_executable_path_from(path);
   return exepath;
}
