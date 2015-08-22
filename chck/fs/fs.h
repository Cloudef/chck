#ifndef __chck_fs__
#define __chck_fs__

#include <chck/macros.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

CHCK_PURE CHCK_NONULL static inline const char*
chck_basename(const char *path)
{
   assert(path);
   const char *base = strrchr(path, '/');
   return (base ? base + 1 : path);
}

CHCK_NONULL static inline bool
chck_filename_is_safe(const char *name)
{
   if (!name || !*name)
      return false;

   if (strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, "..") || strlen(name) > FILENAME_MAX)
      return false;

   return true;
}

char* chck_get_executable_path(void);

#endif /* __chck_fs__ */
