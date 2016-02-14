#include "pool.h"
#include <chck/overflow/overflow.h>
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy */
#include <assert.h> /* for assert */

static void
pool_buffer_flush(struct chck_pool_buffer *pb, bool release)
{
   assert(pb);

   if (release){
      free(pb->buffer);
      pb->allocated = 0;
      pb->buffer = NULL;
   }

   pb->count = pb->used = 0;
}

static void
pool_buffer_release(struct chck_pool_buffer *pb)
{
   if (!pb)
      return;

   pool_buffer_flush(pb, true);
   *pb = (struct chck_pool_buffer){0};
}

static bool
pool_buffer_resize(struct chck_pool_buffer *pb, size_t size)
{
   assert(pb);

   if (unlikely(size == pb->allocated))
      return true;

   if (unlikely(size == 0)) {
      pool_buffer_flush(pb, true);
      return true;
   }

   uint8_t *tmp;
   if (!(tmp = realloc(pb->buffer, size)))
      return false;

   // make sure our buffer is always initialized, to avoid complexity
   if (size > pb->allocated)
      memset(tmp + pb->allocated, 0, size - pb->allocated);

   pb->buffer = tmp;
   pb->allocated = size;
   pb->used = (pb->allocated < pb->used ? pb->allocated : pb->used);
   return true;
}

static bool
pool_buffer_resize_add(struct chck_pool_buffer *pb, size_t base, size_t add)
{
   size_t sz;
   if (unlikely(chck_add_ofsz(base, add, &sz)))
      return false;

   return pool_buffer_resize(pb, sz);
}

static bool
pool_buffer_resize_mul(struct chck_pool_buffer *pb, size_t base, size_t mul)
{
   size_t sz;
   if (unlikely(chck_mul_ofsz(base, mul, &sz)))
      return false;

   return pool_buffer_resize(pb, sz);
}

static bool
pool_buffer(struct chck_pool_buffer *pb, size_t grow, size_t capacity, size_t member_size)
{
   assert(pb && member_size > 0);

   if (unlikely(!member_size) || unlikely(chck_mul_ofsz((grow ? grow : 32), member_size, &pb->step)))
      return false;

   pb->member = member_size;

   if (capacity > 0)
      pool_buffer_resize_mul(pb, capacity, member_size);

   return true;
}

static void*
pool_buffer_add(struct chck_pool_buffer *pb, const void *data, size_t pos, size_t *out_index)
{
   assert(pb && pb->member > 0);

   size_t tail;
   if (unlikely(chck_add_ofsz(pos, pb->member, &tail)))
      return NULL;

   while (pb->allocated < pos + pb->member) {
      if (unlikely(!pool_buffer_resize_add(pb, pb->allocated, pb->step)))
         return NULL;
   }

   if (!pb->buffer)
      return NULL;

   if (data) {
      memcpy(pb->buffer + pos, data, pb->member);
   } else {
      memset(pb->buffer + pos, 0, pb->member);
   }

   if (tail > pb->used)
      pb->used = tail;

   if (out_index)
      *out_index = pos / pb->member;

   pb->count++;
   return pb->buffer + pos;
}

static void*
pool_buffer_add_move(struct chck_pool_buffer *pb, const void *data, size_t pos, size_t *out_index)
{
   if (pos > pb->used)
      pos = pb->used;

   void *ptr;
   if (!(ptr = pool_buffer_add(pb, data, pb->used, out_index)))
      return NULL;

   assert(pb->used >= pb->member);
   assert(pb->used > pos);

   if (pb->used > pb->member) {
      const size_t shift = pb->used - (pos + pb->member);
      memmove(pb->buffer + pos + pb->member, pb->buffer + pos, shift);
      ptr = pb->buffer + pos;
   }

   if (data) {
      memcpy(ptr, data, pb->member);
   } else {
      memset(ptr, 0, pb->member);
   }

   return ptr;
}

static void
pool_buffer_remove(struct chck_pool_buffer *pb, size_t index, size_t (*get_used)(), void *userdata)
{
   assert(pb && get_used);

   size_t slot;
   if (unlikely(chck_mul_ofsz(index, pb->member, &slot)) || unlikely(slot >= pb->used))
      return;

   if (slot + pb->member >= pb->used)
      pb->used = get_used(pb, index, userdata);

   if (pb->used + pb->step < pb->allocated)
      pool_buffer_resize_add(pb, pb->used, pb->member);

   assert(pb->count > 0);
   pb->count--;
   assert((pb->count > 0 && pb->used > 0) || (!pb->count && !pb->used));
}

static void
pool_buffer_remove_move(struct chck_pool_buffer *pb, size_t index)
{
   assert(pb);

   size_t slot;
   if (unlikely(chck_mul_ofsz(index, pb->member, &slot)) || unlikely(slot >= pb->used))
      return;

   if (slot + pb->member < pb->used)
      memmove(pb->buffer + slot, pb->buffer + slot + pb->member, pb->used - slot - pb->member);

   pb->used -= pb->member;

   if (pb->used + pb->step < pb->allocated)
      pool_buffer_resize_add(pb, pb->used, pb->member);

   assert(pb->count > 0);
   pb->count--;
   assert((pb->count > 0 && pb->used > 0) || (!pb->count && !pb->used));
}

CHCK_PURE static void*
pool_buffer_get(const struct chck_pool_buffer *pb, size_t index)
{
   assert(pb);

   size_t slot;
   if (unlikely(chck_mul_ofsz(index, pb->member, &slot)) || unlikely(slot >= pb->used))
      return NULL;

   return pb->buffer + slot;
}

static void*
pool_buffer_iter(const struct chck_pool_buffer *pb, size_t *iter, bool reverse)
{
   assert(pb && iter);

   if (!pb->member || *iter >= pb->used / pb->member)
      return NULL;

   void *ptr = pool_buffer_get(pb, *iter);
   (*iter) += (reverse ? -1 : 1);
   return ptr;
}

static bool
pool_buffer_set_c_array(struct chck_pool_buffer *pb, const void *items, size_t memb)
{
   assert(pb);

   if (items && memb > 0) {
      if (!pool_buffer_resize_mul(pb, memb, pb->member))
         return false;

      memcpy(pb->buffer, items, pb->allocated);
   } else {
      pool_buffer_release(pb);
      memb = 0;
   }

   pb->used = pb->allocated;
   pb->count = memb;
   return true;
}

static void*
pool_buffer_to_c_array(struct chck_pool_buffer *pb, size_t *out_memb)
{
   assert(pb);

   if (!pb->member) {
      if (out_memb)
         *out_memb = 0;
      return NULL;
   }

   if (out_memb)
      *out_memb = (pb->used / pb->member);

   return pb->buffer;
}

CHCK_PURE static bool
pool_is_mapped(const struct chck_pool *pool, size_t index)
{
   assert(pool);
   const bool *mapped = pool_buffer_get(&pool->map, index);
   return (mapped ? *mapped : false);
}

static size_t
pool_get_free_slot(struct chck_pool *pool)
{
   assert(pool);

   if (pool->removed.count > 0) {
      const size_t last = *(size_t*)pool_buffer_get(&pool->removed, pool->removed.count - 1);
      pool_buffer_remove_move(&pool->removed, pool->removed.count - 1);
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

   *pool = (struct chck_pool){0};
   return (pool_buffer(&pool->items, grow, capacity, member_size) &&
           pool_buffer(&pool->map, grow, capacity, sizeof(bool)) &&
           pool_buffer(&pool->removed, grow, 0, sizeof(size_t)));
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

void
chck_pool_flush(struct chck_pool *pool)
{
   assert(pool);
   pool_buffer_flush(&pool->items, true);
   pool_buffer_flush(&pool->map, true);
   pool_buffer_flush(&pool->removed, true);
}

CHCK_PURE void*
chck_pool_get(const struct chck_pool *pool, size_t index)
{
   assert(pool);

   void *ptr;
   if (!(ptr = pool_buffer_get(&pool->items, index)))
      return NULL;

   return (likely(pool_is_mapped(pool, index)) ? ptr : NULL);
}

void*
chck_pool_get_last(const struct chck_pool *pool)
{
   return chck_pool_get(pool, pool->items.count - 1);
}

void
chck_pool_print(const struct chck_pool *pool, FILE *out)
{
   assert(pool && out);

   fprintf(out, "pool: %p member: %zu items: %zu reversed: %zu used: %zu allocated: %zu\n",
         pool, pool->items.member, pool->map.used, pool->map.allocated, pool->items.used, pool->items.allocated);

   for (size_t i = 0; i < pool->map.used; ++i)
      fprintf(out, "%s%s", (pool_is_mapped(pool, i) ? "1" : "0"), ((i + 1) % 80 == 0 ? "\n" : ""));

   fprintf(out, "%s^^^\n", (pool->map.used % 80 == 0 ? "" : "\n"));
}

CHCK_PURE static size_t
pool_get_used(struct chck_pool_buffer *pb, size_t removed, struct chck_pool *pool)
{
   assert(pb && pool);
   assert(removed + 1 <= pool->map.used / pool->map.member);
   assert(pb->used > 0);

   // for chck_pool's, chck_pool_buffer can not know alone the used size,
   // so we need to help a bit with this function.

   size_t largest = removed + 1;
   for (; largest > 0 && (largest - 1 == removed || !pool_is_mapped(pool, largest - 1)); --largest);
   return largest * pb->member;
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
      pool_buffer_remove(&pool->map, slot * pool->map.member, pool_get_used, pool);
      return NULL;
   }

   return p;
}

void
chck_pool_remove(struct chck_pool *pool, size_t index)
{
   assert(pool);

   if (unlikely(!pool_is_mapped(pool, index)))
      return;

   const bool last = (index == pool->items.used / pool->items.member);
   pool_buffer_remove(&pool->items, index, pool_get_used, pool);

   *((bool*)pool_buffer_get(&pool->map, index)) = false;

   pool_buffer_resize_mul(&pool->map, (pool->items.allocated / pool->items.member), pool->map.member);
   pool->map.used = (pool->items.used / pool->items.member) * pool->map.member;

   if (!last) {
      // Some heuristics to avoid large amount of heap allocations
      pool->removed.step = (pool->items.step < pool->items.count / 2 ? pool->items.count / 2 : pool->items.step);
      pool_buffer_add(&pool->removed, &index, pool->removed.used, NULL);
   }
}

void*
chck_pool_iter(const struct chck_pool *pool, size_t *iter, bool reverse)
{
   assert(pool && iter);

   if (!pool->items.member)
      return NULL;

   void *current = NULL;
   do {
      const bool mapped = pool_is_mapped(pool, *iter);
      current = pool_buffer_iter(&pool->items, iter, reverse);
      current = (mapped ? current : NULL);
   } while (!current && *iter < pool->map.used / pool->map.member);

   return current;
}

bool
chck_pool_set_c_array(struct chck_pool *pool, const void *items, size_t memb)
{
   assert(pool);

   if (unlikely(!pool_buffer_set_c_array(&pool->items, items, memb)))
      return false;

   pool_buffer_flush(&pool->removed, true);
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

   *pool = (struct chck_iter_pool){0};
   return pool_buffer(&pool->items, grow, capacity, member_size);
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

void
chck_iter_pool_flush(struct chck_iter_pool *pool)
{
   assert(pool);
   pool_buffer_flush(&pool->items, true);
}

void
chck_iter_pool_empty(struct chck_iter_pool *pool)
{
   assert(pool);
   pool_buffer_flush(&pool->items, false);
}

void*
chck_iter_pool_get(const struct chck_iter_pool *pool, size_t index)
{
   assert(pool);
   return pool_buffer_get(&pool->items, index);
}

void*
chck_iter_pool_get_last(const struct chck_iter_pool *pool)
{
   return chck_iter_pool_get(pool, pool->items.count - 1);
}

void*
chck_iter_pool_insert(struct chck_iter_pool *pool, size_t index, const void *data)
{
   assert(pool);
   return pool_buffer_add_move(&pool->items, data, index * pool->items.member, NULL);
}

void*
chck_iter_pool_push_front(struct chck_iter_pool *pool, const void *data)
{
   assert(pool);
   return pool_buffer_add_move(&pool->items, data, 0, NULL);
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
chck_iter_pool_iter(const struct chck_iter_pool *pool, size_t *iter, bool reverse)
{
   assert(pool && iter);
   return pool_buffer_iter(&pool->items, iter, reverse);
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

   *pool = (struct chck_ring_pool){0};
   return pool_buffer(&pool->items, grow, capacity, member_size);
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

void
chck_ring_pool_flush(struct chck_ring_pool *pool)
{
   assert(pool);
   pool_buffer_flush(&pool->items, true);
   free(pool->popped);
   pool->popped = NULL;
}

void*
chck_ring_pool_push_front(struct chck_ring_pool *pool, const void *data)
{
   assert(pool);
   return pool_buffer_add_move(&pool->items, data, 0, NULL);
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

   const void *ptr = pool_buffer_get(&pool->items, pool->items.count - 1);
   if (!ptr || (!pool->popped && !(pool->popped = malloc(pool->items.member))))
      return NULL;

   memcpy(pool->popped, ptr, pool->items.member);
   pool_buffer_remove_move(&pool->items, pool->items.count - 1);
   return pool->popped;
}

void*
chck_ring_pool_iter(const struct chck_ring_pool *pool, size_t *iter, bool reverse)
{
   assert(pool && iter);
   return pool_buffer_iter(&pool->items, iter, reverse);
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
