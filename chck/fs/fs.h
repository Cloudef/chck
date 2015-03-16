#ifndef __chck_fs__
#define __chck_fs__

#include <string.h>

static inline const char*
chck_basename(const char *path)
{
   const char *base = strrchr(path, '/');
   return (base ? base + 1 : path);
}

char* chck_get_executable_path(void);

#endif /* __chck_fs__ */
