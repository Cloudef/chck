#ifndef __chck_cdl__
#define __chck_cdl__

#include <chck/macros.h>

CHCK_NONULLV(1) void* chck_dl_load(const char *file, const char **out_error);
CHCK_NONULLV(1,2) void* chck_dl_load_symbol(void *handle, const char *name, const char **out_error);
CHCK_NONULL void chck_dl_unload(void *handle);

#endif /* __chck_cdl__ */
