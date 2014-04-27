#include "pool.h"
#include <stdio.h> /* for fprintf */
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

/* Pool data is following:
 * exist byte + item data
 * [1I|1I|0I|1I|1I|1I] */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

typedef struct _chckPool {
   char *name;
   unsigned char *buffer;
   size_t used, allocated, member;
} _chckPool;

static char *chckStrdup(const char *str)
{
   char *cpy;
   size_t size = strlen(str);

   if (!(cpy = calloc(1, size + 1)))
      return NULL;

   memcpy(cpy, str, size);
   return cpy;
}

static int chckPoolResize(chckPool *pool, size_t size)
{
   void *tmp = NULL;
   assert(size != 0 && size != pool->allocated);

   if (pool->used > size) {
      size_t i;
      for (i = pool->used - size; i; i += pool->member);
   }

   if (pool->buffer && pool->allocated < size)
      tmp = realloc(pool->buffer, size);

   if (!tmp) {
      if (!(tmp = malloc(size)))
         return RETURN_FAIL;

      if (pool->buffer) {
         memcpy(tmp, pool->buffer, (pool->used < size ? pool->used : size));
         free(pool->buffer);
      }
   }

   memset(tmp + pool->allocated, 0, size);
   pool->buffer = tmp;
   pool->allocated = size;
   return RETURN_OK;
}

chckPool* chckPoolNew(const char *name, size_t memberSize)
{
   chckPool *pool;
   assert(name && memberSize > 0);

   if (!(pool = calloc(1, sizeof(chckPool))))
      goto fail;

   if (name && !(pool->name = chckStrdup(name)))
      goto fail;

   pool->member = memberSize + 1;
   return pool;

fail:
   chckPoolFree(pool);
   return NULL;
}

void chckPoolFree(chckPool *pool)
{
   assert(pool);

   chckPoolFlush(pool);

   if (pool->name)
      free(pool->name);

   free(pool);
}

void chckPoolFlush(chckPool *pool)
{
   assert(pool);

   if (pool->buffer)
      free(pool->buffer);

   pool->buffer = NULL;
   pool->allocated = pool->used = 0;
}

const char* chckPoolGetName(const chckPool *pool)
{
   assert(pool);
   return pool->name;
}

void* chckPoolAdd(chckPool *pool, size_t size)
{
   assert(pool);
   assert(size + 1 == pool->member);

   if (size + 1 != pool->member) {
      fprintf(stderr, "chckPoolAdd: size should be same as member size when pool was created (%s)", (pool->name ? pool->name : "noname"));
      return NULL;
   }

   size_t next;
   for (next = 0; next < pool->used && *(pool->buffer + next) == 1; next += pool->member);

   if (pool->allocated < next + pool->member && chckPoolResize(pool, pool->allocated + pool->member * 32) != RETURN_OK)
      return NULL;

   *(pool->buffer + next) = 1; // exist
   void *ptr = pool->buffer + next + 1;

   if (next + pool->member > pool->used)
      pool->used = next + pool->member;

   return ptr;
}

void chckPoolRemove(chckPool *pool, void *ref)
{
   assert(pool);
   assert(ref);

   void *item;
   size_t iter = 0;
   while ((item = chckPoolIter(pool, &iter))) {
      if (item != ref)
         continue;

      if (iter == pool->used)
         pool->used -= pool->member;

      memset(item - 1, 0, pool->member);
      break;
   }

   if (pool->used + pool->member * 32 < pool->allocated)
      chckPoolResize(pool, pool->allocated - pool->member * 32);
}

void* chckPoolIter(const chckPool *pool, size_t *iter)
{
   unsigned char *current;
   assert(pool && iter);

   if (*iter >= pool->used)
      return NULL;

   do {
      current = pool->buffer + *iter;
      *iter += pool->member;
   } while (*iter < pool->used && *current == 0);

   return (*current == 0 ? NULL : current + 1);
}

/* vim: set ts=8 sw=3 tw=0 :*/
