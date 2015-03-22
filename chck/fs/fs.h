#ifndef __chck_fs__
#define __chck_fs__

#include "macros.h"
#include <string.h>
#include <assert.h>

CHCK_NONULL static inline const char*
chck_basename(const char *path)
{
   assert(path);
   const char *base = strrchr(path, '/');
   return (base ? base + 1 : path);
}

char* chck_get_executable_path(void);

#endif /* __chck_fs__ */
