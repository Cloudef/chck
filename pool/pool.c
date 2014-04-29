#include "pool.h"
#include <stdio.h> /* for fprintf */
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

typedef struct _chckPoolBuffer {
   unsigned char *buffer;
   size_t step, used, allocated, count, member;
} _chckPoolBuffer;

typedef struct _chckPool {
   _chckPoolBuffer items;
   _chckPoolBuffer removed;
} _chckPool;

static int chckPoolBufferResize(_chckPoolBuffer *pb, size_t size)
{
   void *tmp = NULL;
   assert(size != 0 && size != pb->allocated);

   if (pb->buffer && pb->allocated < size)
      tmp = realloc(pb->buffer, size);

   if (!tmp) {
      if (!(tmp = malloc(size)))
         return RETURN_FAIL;

      if (pb->buffer) {
         memcpy(tmp, pb->buffer, (pb->used < size ? pb->used : size));
         free(pb->buffer);
      }
   }

   memset(tmp + pb->allocated, 0, size);
   pb->buffer = tmp;
   pb->allocated = size;
   return RETURN_OK;
}

static void chckPoolBufferFlush(_chckPoolBuffer *pb)
{
   if (pb->buffer)
      free(pb->buffer);

   pb->buffer = NULL;
   pb->count = pb->allocated = pb->used = 0;
}

static void* chckPoolBufferAdd(_chckPoolBuffer *pb, const void *data, size_t pos, chckPoolIndex *outIndex)
{
   if (pb->allocated < pos + pb->member && chckPoolBufferResize(pb, pb->allocated + pb->member * pb->step) != RETURN_OK)
      return NULL;

   if (data)
      memcpy(pb->buffer + pos, data, pb->member);

   if (pos + pb->member > pb->used)
      pb->used = pos + pb->member;

   if (outIndex)
      *outIndex = pb->count;

   pb->count++;
   return pb->buffer + pos;
}

static void chckPoolBufferRemove(_chckPoolBuffer *pb, chckPoolIndex index)
{
   if (index >= pb->count)
      return;

   if (index * pb->member + pb->member >= pb->used)
      pb->used -= pb->member;

   if (pb->used + pb->member * pb->step < pb->allocated)
      chckPoolBufferResize(pb, pb->allocated - pb->member * pb->step);

   pb->count--;
}

static size_t chckPoolGetFreeSlot(chckPool *pool)
{
   if (pool->removed.count > 0) {
      chckPoolIndex last = *(chckPoolIndex*)pool->removed.buffer + pool->removed.used - pool->removed.member;
      chckPoolBufferRemove(&pool->removed, pool->removed.count - 1);
      return last * pool->items.member;
   }

   return pool->items.used;
}

chckPool* chckPoolNew(size_t growStep, size_t initialItems, size_t memberSize)
{
   chckPool *pool = NULL;
   assert(memberSize > 0);

   if (!memberSize)
      goto fail;

   if (!(pool = calloc(1, sizeof(chckPool))))
      goto fail;

   pool->items.member = memberSize;
   pool->removed.member = sizeof(chckPoolIndex);
   pool->removed.step = pool->items.step = (growStep ? growStep : 32);

   if (initialItems > 0)
      chckPoolBufferResize(&pool->items, initialItems * pool->items.member);

   return pool;

fail:
   if (pool)
      chckPoolFree(pool);
   return NULL;
}

chckPool* chckPoolNewFromCArray(void *items, size_t memb, size_t growStep, size_t memberSize)
{
   chckPool *pool;

   if (!(pool = chckPoolNew(growStep, 0, memberSize)))
      return NULL;

   if (!chckPoolSetCArray(pool, items, memb))
      goto fail;

   return pool;

fail:
   chckPoolFree(pool);
   return NULL;
}

void chckPoolFree(chckPool *pool)
{
   assert(pool);
   chckPoolFlush(pool);
   free(pool);
}

void chckPoolFlush(chckPool *pool)
{
   assert(pool);
   chckPoolBufferFlush(&pool->items);
   chckPoolBufferFlush(&pool->removed);
}

size_t chckPoolCount(const chckPool *pool)
{
   assert(pool);
   return pool->items.count;
}

void* chckPoolGet(const chckPool *pool, chckPoolIndex index)
{
   if (index >= pool->items.count)
      return NULL;

   return pool->items.buffer + index * pool->items.member;
}

void* chckPoolGetLast(const chckPool *pool)
{
   return chckPoolGet(pool, pool->items.count - 1);
}

void* chckPoolAdd(chckPool *pool, const void *data, chckPoolIndex *outIndex)
{
   assert(pool);
   size_t slot = chckPoolGetFreeSlot(pool);
   return chckPoolBufferAdd(&pool->items, data, slot, outIndex);
}

void chckPoolRemove(chckPool *pool, chckPoolIndex index)
{
   assert(pool);

   char last = (index == pool->items.count - 1);
   chckPoolBufferRemove(&pool->items, index);

   if (!last)
      chckPoolBufferAdd(&pool->removed, &index, pool->removed.used, NULL);
}

void* chckPoolIter(const chckPool *pool, chckPoolIndex *iter)
{
   size_t i;
   char removed;
   unsigned char *current;
   assert(pool && iter);

   if (*iter >= pool->items.count)
      return NULL;

   do {
      current = pool->items.buffer + *iter * pool->items.member;

      for (removed = 0, i = 0; i < pool->removed.count; ++i) {
         chckPoolIndex index = *(chckPoolIndex*)pool->removed.buffer + i * pool->removed.member;
         if (index != *iter)
            continue;

         removed = 1;
         break;
      }

      *iter += 1;
   } while (*iter < pool->items.count && removed);

   return current;
}

int chckPoolSetCArray(chckPool *pool, void *items, size_t memb)
{
   void *copy;
   assert(pool);

   if (!(copy = calloc(memb, pool->items.member)))
      return RETURN_FAIL;

   chckPoolFlush(pool);
   memcpy(copy, items, pool->items.member);

   pool->items.buffer = copy;
   pool->items.used = pool->items.allocated = memb * pool->items.member;
   pool->items.count = memb;
   return RETURN_OK;
}

void* chckPoolToCArray(chckPool *pool, size_t *outMemb)
{
   assert(pool);

   if (outMemb)
      *outMemb = pool->items.count;

   return pool->items.buffer;
}

/* vim: set ts=8 sw=3 tw=0 :*/
