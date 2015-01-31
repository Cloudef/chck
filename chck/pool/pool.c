#include "pool.h"
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

#if __GNUC__
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define unlikely(x) !!(x)
#endif

static void
pool_buffer_release(struct chck_pool_buffer *pb)
{
   if (!pb)
      return;

   if (pb->buffer)
      free(pb->buffer);

   pb->buffer = NULL;
   pb->count = pb->allocated = pb->used = 0;
}

static
bool pool_buffer_resize(struct chck_pool_buffer *pb, size_t size)
{
   assert(pb);

   if (unlikely(size == pb->allocated))
      return true;

   if (unlikely(size == 0)) {
      pool_buffer_release(pb);
      return true;
   }

   void *tmp = NULL;
   if (!(tmp = realloc(pb->buffer, size)))
      return false;

   pb->buffer = tmp;
   pb->allocated = size;
   return true;
}

static bool
pool_buffer(struct chck_pool_buffer *pb, size_t grow, size_t capacity, size_t member_size)
{
   assert(pb && member_size > 0);

   if (unlikely(!member_size))
      return false;

   pb->member = member_size;
   pb->step = (grow ? grow : 32);

   if (capacity > 0)
      pool_buffer_resize(pb, capacity * member_size);

   return true;
}

static void*
pool_buffer_add(struct chck_pool_buffer *pb, const void *data, size_t pos, size_t *out_index)
{
   assert(pb);

   if (pb->allocated < pos + pb->member && unlikely(!pool_buffer_resize(pb, pb->allocated + pb->member * pb->step)))
      return NULL;

   if (!pb->buffer)
      return NULL;

   if (data) {
      memcpy(pb->buffer + pos, data, pb->member);
   } else {
      memset(pb->buffer + pos, 0, pb->member);
   }

   if (pos + pb->member > pb->used)
      pb->used = pos + pb->member;

   if (out_index)
      *out_index = pos / pb->member;

   pb->count++;
   return pb->buffer + pos;
}

static void
pool_buffer_remove(struct chck_pool_buffer *pb, size_t index)
{
   assert(pb);

   if (unlikely(index * pb->member >= pb->used))
      return;

   if (index * pb->member + pb->member >= pb->used)
      pb->used -= pb->member;

   if (pb->used + pb->member * pb->step < pb->allocated)
      pool_buffer_resize(pb, pb->allocated - pb->member * pb->step);

   assert(pb->count > 0);
   pb->count--;
}

static void
pool_buffer_remove_move(struct chck_pool_buffer *pb, size_t index)
{
   assert(pb);

   if (unlikely(index * pb->member >= pb->used))
      return;

   if (index * pb->member + pb->member < pb->used)
      memmove(pb->buffer + index * pb->member, pb->buffer + (index + 1) * pb->member, pb->used - (index + 1) * pb->member);

   pb->used -= pb->member;
   pb->count--;

   if (pb->used + pb->member * pb->step < pb->allocated)
      pool_buffer_resize(pb, pb->allocated - pb->member * pb->step);
}

static void*
pool_buffer_iter(const struct chck_pool_buffer *pb, size_t *iter)
{
   assert(iter);

   if (*iter * pb->member >= pb->used)
      return NULL;

   return pb->buffer + (*iter)++ * pb->member;
}

static bool
pool_buffer_set_c_array(struct chck_pool_buffer *pb, const void *items, size_t memb)
{
   assert(pb);

   void *copy = NULL;
   if (items && memb > 0) {
      if (!(copy = malloc(memb * pb->member)))
         return false;

      memcpy(copy, items, memb * pb->member);
   }

   pool_buffer_release(pb);

   pb->buffer = copy;
   pb->used = pb->allocated = memb * pb->member;
   pb->count = memb;
   return true;
}

static void*
pool_buffer_to_c_array(struct chck_pool_buffer *pb, size_t *out_memb)
{
   assert(pb);

   if (out_memb)
      *out_memb = (pb->used / pb->member);

   return pb->buffer;
}

static size_t
pool_get_free_slot(struct chck_pool *pool)
{
   assert(pool);

   if (pool->removed.count > 0) {
      const size_t last = *(size_t*)(pool->removed.buffer + pool->removed.used - pool->removed.member);
      pool_buffer_remove(&pool->removed, pool->removed.count - 1);
      return last;
   }

   return pool->items.count;
}

bool
chck_pool(struct chck_pool *pool, size_t grow, size_t capacity, size_t member_size)
{
   assert(pool && member_size > 0);

   if (unlikely(!member_size))
      return false;

   memset(pool, 0, sizeof(struct chck_pool));
   assert(pool_buffer(&pool->items, grow, capacity, member_size));
   assert(pool_buffer(&pool->map, grow, capacity, sizeof(bool)));
   assert(pool_buffer(&pool->removed, grow, 0, sizeof(size_t)));
   return true;
}

bool
chck_pool_from_c_array(struct chck_pool *pool, const void *items, size_t memb, size_t grow, size_t member_size)
{
   return (chck_pool(pool, grow, 0, member_size) && chck_pool_set_c_array(pool, items, memb));
}

void
chck_pool_release(struct chck_pool *pool)
{
   if (!pool)
      return;

   pool_buffer_release(&pool->items);
   pool_buffer_release(&pool->map);
   pool_buffer_release(&pool->removed);
}

void*
chck_pool_get(const struct chck_pool *pool, size_t index)
{
   assert(pool);

   if (unlikely(index * pool->items.member >= pool->items.used))
      return NULL;

   return pool->items.buffer + index * pool->items.member;
}

void*
chck_pool_get_last(const struct chck_pool *pool)
{
   return chck_pool_get(pool, pool->items.count - 1);
}

void*
chck_pool_add(struct chck_pool *pool, const void *data, size_t *out_index)
{
   assert(pool);
   const size_t slot = pool_get_free_slot(pool);

   if (!pool_buffer_add(&pool->map, (bool[]){true}, slot * pool->map.member, NULL))
      return NULL;

   void *p;
   if (!(p = pool_buffer_add(&pool->items, data, slot * pool->items.member, out_index))) {
      pool_buffer_remove(&pool->map, slot * pool->map.member);
      return NULL;
   }

   return p;
}

void
chck_pool_remove(struct chck_pool *pool, size_t index)
{
   assert(pool);

   if (!unlikely(*(bool*)(pool->map.buffer + index * pool->map.member)))
      return;

   const bool last = (index * pool->items.member == pool->items.used);
   pool_buffer_remove(&pool->items, index);

   *(bool*)(pool->map.buffer + index * pool->map.member) = false;
   pool_buffer_remove(&pool->map, index);

   if (!last) {
      // Some heuristics to avoid large amount of heap allocations
      pool->removed.step = (pool->items.step < pool->items.count / 2 ? pool->items.count / 2 : pool->items.step);
      pool_buffer_add(&pool->removed, &index, pool->removed.used, NULL);
   }
}

void*
chck_pool_iter(const struct chck_pool *pool, size_t *iter)
{
   assert(pool && iter);

   void *current = NULL;
   while (!current && *iter * pool->items.member < pool->items.used) {
      current = pool_buffer_iter(&pool->items, iter);

      // We don't want to return pointer to removed indexes.
      if (!*(bool*)(pool->map.buffer + (*iter - 1) * pool->map.member))
         current = NULL;
   }

   return current;
}

bool
chck_pool_set_c_array(struct chck_pool *pool, const void *items, size_t memb)
{
   assert(pool);

   if (unlikely(!pool_buffer_set_c_array(&pool->items, items, memb)))
      return false;

   pool_buffer_release(&pool->removed);
   return true;
}

void*
chck_pool_to_c_array(struct chck_pool *pool, size_t *out_memb)
{
   assert(pool);
   return pool_buffer_to_c_array(&pool->items, out_memb);
}

bool
chck_iter_pool(struct chck_iter_pool *pool, size_t grow, size_t capacity, size_t member_size)
{
   assert(pool && member_size > 0);

   if (unlikely(!member_size))
      return false;

   assert(pool_buffer(&pool->items, grow, capacity, member_size));
   return true;
}

bool
chck_iter_pool_from_c_array(struct chck_iter_pool *pool, const void *items, size_t memb, size_t grow, size_t member_size)
{
   return unlikely(chck_iter_pool(pool, grow, 0, member_size) && chck_iter_pool_set_c_array(pool, items, memb));
}

void
chck_iter_pool_release(struct chck_iter_pool *pool)
{
   if (!pool)
      return;

   pool_buffer_release(&pool->items);
}

void*
chck_iter_pool_get(const struct chck_iter_pool *pool, size_t index)
{
   assert(pool);

   if (unlikely(index * pool->items.member >= pool->items.used))
      return NULL;

   return pool->items.buffer + index * pool->items.member;
}

void*
chck_iter_pool_get_last(const struct chck_iter_pool *pool)
{
   return chck_iter_pool_get(pool, pool->items.count - 1);
}

void*
chck_iter_pool_push_front(struct chck_iter_pool *pool, const void *data)
{
   assert(pool);

   void *ptr;
   if (!(ptr = pool_buffer_add(&pool->items, NULL, pool->items.used, NULL)))
      return NULL;

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

void*
chck_iter_pool_push_back(struct chck_iter_pool *pool, const void *data)
{
   assert(pool);
   return pool_buffer_add(&pool->items, data, pool->items.used, NULL);
}

void
chck_iter_pool_remove(struct chck_iter_pool *pool, size_t index)
{
   assert(pool);
   pool_buffer_remove_move(&pool->items, index);
}

void*
chck_iter_pool_iter(const struct chck_iter_pool *pool, size_t *iter)
{
   assert(pool && iter);
   return pool_buffer_iter(&pool->items, iter);
}

bool
chck_iter_pool_set_c_array(struct chck_iter_pool *pool, const void *items, size_t memb)
{
   assert(pool);
   return pool_buffer_set_c_array(&pool->items, items, memb);
}

void*
chck_iter_pool_to_c_array(struct chck_iter_pool *pool, size_t *out_memb)
{
   assert(pool);
   return pool_buffer_to_c_array(&pool->items, out_memb);
}

bool
chck_ring_pool(struct chck_ring_pool *pool, size_t grow, size_t capacity, size_t member_size)
{
   assert(pool && member_size > 0);

   if (unlikely(!member_size))
      return false;

   assert(pool_buffer(&pool->items, grow, capacity, member_size));
   return true;
}

bool
chck_ring_pool_from_c_array(struct chck_ring_pool *pool, const void *items, size_t memb, size_t grow, size_t member_size)
{
   return (!chck_ring_pool(pool, grow, 0, member_size) && !chck_ring_pool_set_c_array(pool, items, memb));
}

void
chck_ring_pool_release(struct chck_ring_pool *pool)
{
   if (!pool)
      return;

   pool_buffer_release(&pool->items);
   free(pool->popped);
   pool->popped = NULL;
}

void*
chck_ring_pool_push_front(struct chck_ring_pool *pool, const void *data)
{
   assert(pool);

   void *ptr;
   if (!(ptr = pool_buffer_add(&pool->items, NULL, pool->items.used, NULL)))
      return NULL;

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

void*
chck_ring_pool_push_back(struct chck_ring_pool *pool, const void *data)
{
   assert(pool);
   return pool_buffer_add(&pool->items, data, pool->items.used, NULL);
}

void*
chck_ring_pool_pop_first(struct chck_ring_pool *pool)
{
   assert(pool);

   if (unlikely(pool->items.count <= 0))
      return NULL;

   if (!pool->popped && !(pool->popped = malloc(pool->items.member)))
      return NULL;

   memcpy(pool->popped, pool->items.buffer, pool->items.member);
   pool_buffer_remove_move(&pool->items, 0);
   return pool->popped;
}

void*
chck_ring_pool_pop_last(struct chck_ring_pool *pool)
{
   assert(pool);

   if (unlikely(pool->items.count <= 0))
      return NULL;

   if (!pool->popped && !(pool->popped = malloc(pool->items.member)))
      return NULL;

   void *ptr = pool->items.buffer + pool->items.used - pool->items.member;
   memcpy(pool->popped, ptr, pool->items.member);
   pool_buffer_remove_move(&pool->items, pool->items.count - 1);
   return pool->popped;
}

void*
chck_ring_pool_iter(const struct chck_ring_pool *pool, size_t *iter)
{
   assert(pool && iter);
   return pool_buffer_iter(&pool->items, iter);
}

bool
chck_ring_pool_set_c_array(struct chck_ring_pool *pool, const void *items, size_t memb)
{
   assert(pool);
   return pool_buffer_set_c_array(&pool->items, items, memb);
}

void*
chck_ring_pool_to_c_array(struct chck_ring_pool *pool, size_t *out_memb)
{
   assert(pool);
   return pool_buffer_to_c_array(&pool->items, out_memb);
}
