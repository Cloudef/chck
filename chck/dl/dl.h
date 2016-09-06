#ifndef __chck_cdl__
#define __chck_cdl__

#include <chck/macros.h>

void* chck_dl_load(const char *file, const char **out_error);
void* chck_dl_load_symbol(void *handle, const char *name, const char **out_error);
void chck_dl_unload(void *handle);

#endif /* __chck_cdl__ */
