#include "pool.h"
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

typedef struct _chckPoolBuffer {
   unsigned char *buffer;
   size_t step, used, allocated, count, member;
} _chckPoolBuffer;

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

   pb->buffer = tmp;
   pb->allocated = size;
   return RETURN_OK;
}

static void chckPoolBufferInit(_chckPoolBuffer *pb, size_t growStep, size_t capacity, size_t memberSize)
{
   assert(memberSize > 0);

   pb->member = memberSize;
   pb->step = (growStep ? growStep : 32);

   if (capacity > 0)
      chckPoolBufferResize(pb, capacity * memberSize);
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

   if (data) {
      memcpy(pb->buffer + pos, data, pb->member);
   } else {
      memset(pb->buffer + pos, 0, pb->member);
   }

   if (pos + pb->member > pb->used)
      pb->used = pos + pb->member;

   if (outIndex)
      *outIndex = pos / pb->member;

   pb->count++;
   return pb->buffer + pos;
}

static void chckPoolBufferRemove(_chckPoolBuffer *pb, chckPoolIndex index)
{
   if (index * pb->member >= pb->used)
      return;

   if (index * pb->member + pb->member >= pb->used)
      pb->used -= pb->member;

   if (pb->used + pb->member * pb->step < pb->allocated)
      chckPoolBufferResize(pb, pb->allocated - pb->member * pb->step);

   pb->count--;
}

static void chckPoolBufferRemoveMove(_chckPoolBuffer *pb, chckPoolIndex index)
{
   if (index * pb->member >= pb->used)
      return;

   if (index * pb->member + pb->member < pb->used)
      memmove(pb->buffer + index * pb->member, pb->buffer + (index + 1) * pb->member, pb->used - (index + 1) * pb->member);

   pb->used -= pb->member;
   pb->count--;

   if (pb->used + pb->member * pb->step < pb->allocated)
      chckPoolBufferResize(pb, pb->allocated - pb->member * pb->step);
}

static void* chckPoolBufferIter(const _chckPoolBuffer *pb, size_t *iter)
{
   assert(iter);

   if (*iter * pb->member >= pb->used)
      return NULL;

   return pb->buffer + (*iter)++ * pb->member;
}

static int chckPoolBufferSetCArray(_chckPoolBuffer *pb, const void *items, size_t memb)
{
   void *copy = NULL;

   if (items && memb > 0) {
      if (!(copy = calloc(memb, pb->member)))
         return RETURN_FAIL;

      memcpy(copy, items, memb * pb->member);
   }

   chckPoolBufferFlush(pb);

   pb->buffer = copy;
   pb->used = pb->allocated = memb * pb->member;
   pb->count = memb;
   return RETURN_OK;
}

static void* chckPoolBufferToCArray(_chckPoolBuffer *pb, size_t *outMemb)
{
   if (outMemb)
      *outMemb = (pb->used / pb->member);

   return pb->buffer;
}

typedef struct _chckPool {
   _chckPoolBuffer items;
   _chckPoolBuffer removed;
} _chckPool;

static size_t chckPoolGetFreeSlot(chckPool *pool)
{
   if (pool->removed.count > 0) {
      chckPoolIndex last = *(chckPoolIndex*)pool->removed.buffer + pool->removed.used - pool->removed.member;
      chckPoolBufferRemove(&pool->removed, pool->removed.count - 1);
      return last * pool->items.member;
   }

   return pool->items.used;
}

chckPool* chckPoolNew(size_t growStep, size_t capacity, size_t memberSize)
{
   chckPool *pool = NULL;
   assert(memberSize > 0);

   if (!memberSize)
      goto fail;

   if (!(pool = calloc(1, sizeof(chckPool))))
      goto fail;

   chckPoolBufferInit(&pool->items, growStep, capacity, memberSize);
   chckPoolBufferInit(&pool->removed, growStep, 0, sizeof(chckPoolIndex));
   return pool;

fail:
   if (pool)
      chckPoolFree(pool);
   return NULL;
}

chckPool* chckPoolNewFromCArray(const void *items, size_t memb, size_t growStep, size_t memberSize)
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
   if (index * pool->items.member >= pool->items.used)
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

   unsigned char last = (index == pool->items.count - 1);
   chckPoolBufferRemove(&pool->items, index);

   if (!last)
      chckPoolBufferAdd(&pool->removed, &index, pool->removed.used, NULL);
}

void* chckPoolIter(const chckPool *pool, chckPoolIndex *iter)
{
   assert(pool && iter);

   void *current = NULL;

   while (!current && *iter * pool->items.member < pool->items.used) {
      current = chckPoolBufferIter(&pool->items, iter);

      /**
       * We don't want to return pointer to removed indexes.
       */

      size_t i;
      for (i = 0; i < pool->removed.count; ++i) {
         chckPoolIndex index = *(chckPoolIndex*)pool->removed.buffer + i * pool->removed.member;
         if (index == *iter - 1) {
            current = NULL;
            break;
         }
      }
   }

   return current;
}

int chckPoolSetCArray(chckPool *pool, const void *items, size_t memb)
{
   assert(pool);

   if (chckPoolBufferSetCArray(&pool->items, items, memb) != RETURN_OK)
      return RETURN_FAIL;

   chckPoolBufferFlush(&pool->removed);
   return RETURN_OK;
}

void* chckPoolToCArray(chckPool *pool, size_t *outMemb)
{
   assert(pool);
   return chckPoolBufferToCArray(&pool->items, outMemb);
}

typedef struct _chckIterPool {
   _chckPoolBuffer items;
} _chckIterPool;

chckIterPool* chckIterPoolNew(size_t growStep, size_t capacity, size_t memberSize)
{
   chckIterPool *pool = NULL;
   assert(memberSize > 0);

   if (!memberSize)
      goto fail;

   if (!(pool = calloc(1, sizeof(chckIterPool))))
      goto fail;

   chckPoolBufferInit(&pool->items, growStep, capacity, memberSize);
   return pool;

fail:
   if (pool)
      chckIterPoolFree(pool);
   return NULL;
}

chckIterPool* chckIterPoolNewFromCArray(const void *items, size_t memb, size_t growStep, size_t memberSize)
{
   chckIterPool *pool;

   if (!(pool = chckIterPoolNew(growStep, 0, memberSize)))
      return NULL;

   if (!chckIterPoolSetCArray(pool, items, memb))
      goto fail;

   return pool;

fail:
   chckIterPoolFree(pool);
   return NULL;
}

void chckIterPoolFree(chckIterPool *pool)
{
   assert(pool);
   chckIterPoolFlush(pool);
   free(pool);
}

void chckIterPoolFlush(chckIterPool *pool)
{
   assert(pool);
   chckPoolBufferFlush(&pool->items);
}

size_t chckIterPoolCount(const chckIterPool *pool)
{
   assert(pool);
   return pool->items.count;
}

void* chckIterPoolGet(const chckIterPool *pool, chckPoolIndex index)
{
   if (index * pool->items.member >= pool->items.used)
      return NULL;

   return pool->items.buffer + index * pool->items.member;
}

void* chckIterPoolGetLast(const chckIterPool *pool)
{
   return chckIterPoolGet(pool, pool->items.count - 1);
}

void* chckIterPoolAdd(chckIterPool *pool, const void *data, chckPoolIndex *outIndex)
{
   assert(pool);
   return chckPoolBufferAdd(&pool->items, data, pool->items.used, outIndex);
}

void chckIterPoolRemove(chckIterPool *pool, chckPoolIndex index)
{
   assert(pool);
   chckPoolBufferRemoveMove(&pool->items, index);
}

void* chckIterPoolIter(const chckIterPool *pool, chckPoolIndex *iter)
{
   assert(pool && iter);
   return chckPoolBufferIter(&pool->items, iter);
}

int chckIterPoolSetCArray(chckIterPool *pool, const void *items, size_t memb)
{
   assert(pool);

   if (chckPoolBufferSetCArray(&pool->items, items, memb) != RETURN_OK)
      return RETURN_FAIL;

   return RETURN_OK;
}

void* chckIterPoolToCArray(chckIterPool *pool, size_t *outMemb)
{
   assert(pool);
   return chckPoolBufferToCArray(&pool->items, outMemb);
}

typedef struct _chckRingPool {
   _chckPoolBuffer items;
   void *popped;
} _chckRingPool;

chckRingPool* chckRingPoolNew(size_t growStep, size_t capacity, size_t memberSize)
{
   chckRingPool *pool = NULL;
   assert(memberSize > 0);

   if (!memberSize)
      goto fail;

   if (!(pool = calloc(1, sizeof(chckRingPool))))
      goto fail;

   if (!(pool->popped = calloc(1, memberSize)))
      goto fail;

   chckPoolBufferInit(&pool->items, growStep, capacity, memberSize);
   return pool;

fail:
   if (pool)
      chckRingPoolFree(pool);
   return NULL;
}

chckRingPool* chckRingPoolNewFromCArray(const void *items, size_t memb, size_t growStep, size_t memberSize)
{
   chckRingPool *pool;

   if (!(pool = chckRingPoolNew(growStep, 0, memberSize)))
      return NULL;

   if (!chckRingPoolSetCArray(pool, items, memb))
      goto fail;

   return pool;

fail:
   chckRingPoolFree(pool);
   return NULL;
}

void chckRingPoolFree(chckRingPool *pool)
{
   assert(pool);
   chckRingPoolFlush(pool);

   if (pool->popped)
      free(pool->popped);

   free(pool);
}

void chckRingPoolFlush(chckRingPool *pool)
{
   assert(pool);
   chckPoolBufferFlush(&pool->items);
}

size_t chckRingPoolCount(const chckRingPool *pool)
{
   assert(pool);
   return pool->items.count;
}

void* chckRingPoolPushFront(chckRingPool *pool, const void *data)
{
   assert(pool);

   void *ptr = chckPoolBufferAdd(&pool->items, NULL, pool->items.used, NULL);

   if (pool->items.used > pool->items.member) {
      memmove(pool->items.buffer + pool->items.member, pool->items.buffer, pool->items.used - pool->items.member);
      ptr = pool->items.buffer;

      if (data) {
         memcpy(ptr, data, pool->items.member);
      } else {
         memset(ptr, 0, pool->items.member);
      }
   }

   return ptr;
}

void* chckRingPoolPushEnd(chckRingPool *pool, const void *data)
{
   assert(pool);
   return chckPoolBufferAdd(&pool->items, data, pool->items.used, NULL);
}

void* chckRingPoolPopFirst(chckRingPool *pool)
{
   assert(pool);

   if (pool->items.count <= 0)
      return NULL;

   memcpy(pool->popped, pool->items.buffer, pool->items.member);
   chckPoolBufferRemoveMove(&pool->items, 0);
   return pool->popped;
}

void* chckRingPoolPopLast(chckRingPool *pool)
{
   assert(pool);

   if (pool->items.count <= 0)
      return NULL;

   void *ptr = pool->items.buffer + pool->items.used - pool->items.member;
   memcpy(pool->popped, ptr, pool->items.member);
   chckPoolBufferRemoveMove(&pool->items, pool->items.count - 1);
   return pool->popped;
}

void* chckRingPoolIter(const chckRingPool *pool, chckPoolIndex *iter)
{
   assert(pool && iter);
   return chckPoolBufferIter(&pool->items, iter);
}

int chckRingPoolSetCArray(chckRingPool *pool, const void *items, size_t memb)
{
   assert(pool);

   if (chckPoolBufferSetCArray(&pool->items, items, memb) != RETURN_OK)
      return RETURN_FAIL;

   return RETURN_OK;
}

void* chckRingPoolToCArray(chckRingPool *pool, size_t *outMemb)
{
   assert(pool);
   return chckPoolBufferToCArray(&pool->items, outMemb);
}

/* vim: set ts=8 sw=3 tw=0 :*/
