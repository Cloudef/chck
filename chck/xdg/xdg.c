#include "xdg.h"
#include <chck/overflow/overflow.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pwd.h>

static inline char*
ccopy(const char *str)
{
   assert(str);
   const size_t size = strlen(str);
   char *cpy = chck_calloc_add_of(size, 1);
   return (cpy ? memcpy(cpy, str, size) : NULL);
}

static inline char*
strip_slash(char *str)
{
   assert(str);
   size_t size;
   if ((size = strlen(str)) > 0) {
      for (char *s = str + size - 1; s >= str && *s == '/'; --s)
         *s = 0;
   }
   return str;
}

static inline char*
get_home(void)
{
   const char *env;
   if ((env = getenv("HOME")) && env[0] == '/')
      return ccopy(env);

   struct passwd *pwd;
   if (!(pwd = getpwuid(getuid())) || !pwd->pw_dir || pwd->pw_dir[0] != '/')
      return NULL;

   return ccopy(pwd->pw_dir);
}

char*
xdg_get_path(const char *xdg_env, const char *default_path)
{
   assert(xdg_env && default_path && default_path[0] != '/');

   if (!xdg_env || !default_path || default_path[0] == '/')
      return NULL;

   const char *xdg_dir;
   if ((xdg_dir = getenv(xdg_env)) && xdg_dir[0] == '/')
      return strip_slash(ccopy(xdg_dir));

   char *home;
   if (!(home = get_home()))
      return NULL; /** fatal! */

   size_t len = snprintf(NULL, 0, "%s/%s", home, default_path) + 1;

   char *path;
   if (!(path = calloc(1, len))) {
      free(home);
      return NULL; /** fatal! */
   }

   snprintf(path, len, "%s/%s", home, default_path);
   free(home);
   return path;
}

const char*
xdg_get_paths(const char *xdg_env, const char *default_paths, struct xdg_paths *state, uint32_t max_iter)
{
   assert(xdg_env && default_paths && state);

   if ((state->path && !*state->path) || state->iter == max_iter) {
      free((char*)state->paths);
      return NULL;
   }

   ++state->iter;

   if (!state->paths) {
      if (!xdg_env || !default_paths)
         return NULL;

      const char *paths;
      if (!(paths = getenv(xdg_env)) || !paths[0])
         paths = default_paths;

      state->path = state->paths = ccopy(paths);
   }

   if (!state->path || !state->paths)
      return NULL;

   char *path;
   do {
      size_t f;
      path = (char*)state->path;
      if ((f = strcspn(state->path, ":")) > 0) {
         state->path += f + (path[f] ? 1 : 0);
         path[f] = 0;
      }

      if (!*path) {
         free((char*)state->paths);
         return NULL;
      }
   } while (path[0] != '/');

   return strip_slash(path);
}
