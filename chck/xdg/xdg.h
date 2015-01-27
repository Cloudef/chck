#ifndef __chck_xdg__
#define __chck_xdg__

struct xdg_paths {
   const char *path;
   const char *paths;
};

char* xdg_get_path(const char *xdg_env, const char *default_path);
const char* xdg_get_paths(const char *xdg_env, const char *default_paths, struct xdg_paths *state);

#endif /* __chck_xdg__ */
