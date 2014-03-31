#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

#if defined(_WIN32) || defined(_WIN64)
/* \brief portable strdup */
static char *chckStrdup(char *str)
{
   char *cpy;
   size_t size = strlen(str);

   if (!(cpy = calloc(1, size + 1)))
      return NULL;

   memcpy(cpy, str, size);
   return cpy;
}
#endif

/* \brief resize buffer */
static int chckResizeBuf(char **buf, size_t *size, size_t nsize)
{
   void *tmp;

   if (nsize < *size || !(tmp = realloc(*buf, nsize))) {
      if (!(tmp = malloc(nsize))) return RETURN_FAIL;
      memcpy(tmp, *buf, (nsize > *size ? *size : nsize));
      free(*buf);
   }

   *buf = tmp;
   *size = nsize;
   return RETURN_OK;
}

/* \brief read executable path cross-platform way, with arbitary sized buffer */
static char* chckGetExecutablePathFrom(const char *path)
{
   char *buf;
   ssize_t rsize;
   size_t size = 1024;

#if (defined(__APPLE__) && defined(__MACH__))
   (void)path;
   unsigned int bsize = 0;
   _NSGetExecutablePath(buf, &bsize);
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
      if (chckResizeBuf(&buf, &size, size * 2) != RETURN_OK) goto fail;
   }
#else
   if (!(buf = malloc(size))) goto fail;
   while ((size_t)(rsize = readlink(path, buf, size)) > size) {
      if (rsize <= 0) goto fail;
      if (chckResizeBuf(&buf, &size, size * 2) != RETURN_OK) goto fail;
   }
#endif

   if (rsize <= 0) goto fail;
   if (rsize != -1 && size != (size_t)rsize) {
      if (chckResizeBuf(&buf, &size, (size_t)rsize+1) != RETURN_OK) goto fail;
      buf[rsize] = 0;
   }

#if (defined(__APPLE__) && defined(__MACH__))
   char *tmp;
   if (!(tmp = realpath(buf, NULL))) goto fail;
   free(buf); buf = tmp;
#endif
   return buf;

fail:
   if (buf) free(buf);
   return NULL;
}

/* \brief try to get executable path.
 * if this fails you should fallback to relative paths. */
char* chckGetExecutablePath(void)
{
   const char *path = NULL;
   char *exepath = NULL;

#if defined(EMSCRIPTEN)
   (void)path;
   (void)exepath;
   return NULL;
#elif defined(_WIN32) || defined(_WIN64)
   if (_pgmptr && !(exepath = chckStrdup(_pgmptr))) return NULL;
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
   exepath = chckGetExecutablePathFrom(path);
   return exepath;
}

/* vim: set ts=8 sw=3 tw=0 :*/
