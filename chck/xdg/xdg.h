#ifndef __chck_xdg__
#define __chck_xdg__

#include "macros.h"
#include <stdint.h>

struct xdg_paths {
   const char *path;
   const char *paths;
   uint32_t iter;
};

CHCK_NONULL char* xdg_get_path(const char *xdg_env, const char *default_path);
CHCK_NONULL const char* xdg_get_paths(const char *xdg_env, const char *default_paths, struct xdg_paths *state, uint32_t max_iter);

#endif /* __chck_xdg__ */
