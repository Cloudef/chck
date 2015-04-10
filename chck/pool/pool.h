#ifndef __chck_pool__
#define __chck_pool__

#include <chck/macros.h>
#include <stddef.h>
#include <stdbool.h>

struct chck_pool_buffer {
   // pointer to contents
   void *buffer;

   // growth step and member size (growth is 'n' in n * step)
   size_t step, member;

   // how many bytes are used and allocated
   size_t used, allocated;

   // number of items in the buffer
   size_t count;
};

struct chck_pool {
   struct chck_pool_buffer items;
   struct chck_pool_buffer map;
   struct chck_pool_buffer removed;
};

struct chck_iter_pool {
   struct chck_pool_buffer items;
};

struct chck_ring_pool {
   struct chck_pool_buffer items;

   // storage for popped element so we can return it
   void *popped;
};

/**
 * Pools are manual memory buffers for your data (usually structs).
 * Pools may contain holes as whenever you remove item, the space is not removed, but instead marked as unused.
 * To access pool items, dont rely on the returned pointers, but use the indices instead.
 * The pointers may point to garbage whenever you add/remove item (as the buffer may be resized).
 *
 * Pools have very fast add/remove operation O(1) with expense of buffer of booleans and free list.
 */

#define chck_pool_for_each_call(pool, function, ...) \
{ void *_P; for (size_t _I = 0; (_P = chck_pool_iter(pool, &_I, false));) function(_P, ##__VA_ARGS__); }

#define chck_pool_for_each_call_reverse(pool, function, ...) \
{ void *_P; for (size_t _I = (pool)->items.count - 1; (_P = chck_pool_iter(pool, &_I, true));) function(_P, ##__VA_ARGS__); }

#define chck_pool_for_each(pool, pos) \
   for (size_t _I = 0; (pos = chck_pool_iter(pool, &_I, false));)

#define chck_pool_for_each_reverse(pool, pos) \
   for (size_t _I = (pool)->items.count - 1; (pos = chck_pool_iter(pool, &_I, true));)

CHCK_NONULL bool chck_pool(struct chck_pool *pool, size_t grow, size_t capacity, size_t member_size);
CHCK_NONULL bool chck_pool_from_c_array(struct chck_pool *pool, const void *items, size_t memb, size_t grow, size_t member_size);
void chck_pool_release(struct chck_pool *pool);
CHCK_NONULL void chck_pool_flush(struct chck_pool *pool);
CHCK_NONULL void* chck_pool_get(const struct chck_pool *pool, size_t index);
CHCK_NONULL void* chck_pool_get_last(const struct chck_pool *pool);
CHCK_NONULLV(1) void* chck_pool_add(struct chck_pool *pool, const void *data, size_t *out_index);
CHCK_NONULL void chck_pool_remove(struct chck_pool *pool, size_t index);
CHCK_NONULL void* chck_pool_iter(const struct chck_pool *pool, size_t *iter, bool reverse);
CHCK_NONULLV(1) bool chck_pool_set_c_array(struct chck_pool *pool, const void *items, size_t memb); /* struct item *c_array; */
CHCK_NONULLV(1) void* chck_pool_to_c_array(struct chck_pool *pool, size_t *memb); /* struct item *c_array; (contains holes) */

/**
 * IterPools don't have holes in buffer.
 * Whenever you remove a item from IterPool, the items after that get memmoved.
 * Thus the indices returned by IterPool functions are _not_ safe.
 *
 * As the name implies, use this pool only if you need to access items by iteration.
 */

#define chck_iter_pool_for_each_call(pool, function, ...) \
{ void *_P; for (size_t _I = 0; (_P = chck_iter_pool_iter(pool, &_I, false));) function(_P, ##__VA_ARGS__); }

#define chck_iter_pool_for_each_call_reverse(pool, function, ...) \
{ void *_P; for (size_t _I = (pool)->items.count - 1; (_P = chck_iter_pool_iter(pool, &_I, true));) function(_P, ##__VA_ARGS__); }

#define chck_iter_pool_for_each(pool, pos) \
   for (size_t _I = 0; (pos = chck_iter_pool_iter(pool, &_I, false));)

#define chck_iter_pool_for_each_reverse(pool, pos) \
   for (size_t _I = (pool)->items.count - 1; (pos = chck_iter_pool_iter(pool, &_I, true));)

CHCK_NONULL bool chck_iter_pool(struct chck_iter_pool *pool, size_t grow, size_t capacity, size_t member_size);
CHCK_NONULL bool chck_iter_pool_from_c_array(struct chck_iter_pool *pool, const void *items, size_t memb, size_t grow_step, size_t member_size);
void chck_iter_pool_release(struct chck_iter_pool *pool);
CHCK_NONULL void chck_iter_pool_flush(struct chck_iter_pool *pool);
CHCK_NONULL void chck_iter_pool_empty(struct chck_iter_pool *pool);
CHCK_NONULL void* chck_iter_pool_get(const struct chck_iter_pool *pool, size_t index);
CHCK_NONULL void* chck_iter_pool_get_last(const struct chck_iter_pool *pool);
CHCK_NONULLV(1) void* chck_iter_pool_push_front(struct chck_iter_pool *pool, const void *data);
CHCK_NONULLV(1) void* chck_iter_pool_push_back(struct chck_iter_pool *pool, const void *data);
CHCK_NONULLV(1) void* chck_iter_pool_insert(struct chck_iter_pool *pool, size_t index, const void *data);
CHCK_NONULL void chck_iter_pool_remove(struct chck_iter_pool *pool, size_t index);
CHCK_NONULL void* chck_iter_pool_iter(const struct chck_iter_pool *pool, size_t *iter, bool reverse);
CHCK_NONULLV(1) bool chck_iter_pool_set_c_array(struct chck_iter_pool *pool, const void *items, size_t memb); /* struct item *c_array; */
CHCK_NONULLV(1) void* chck_iter_pool_to_c_array(struct chck_iter_pool *pool, size_t *memb); /* struct item *c_array; */

/**
 * RingPools are circular pools.
 * You push items to them and the pool grows as needed.
 * You can push and pop to/from both sides.
 */

#define chck_ring_pool_for_each_call(pool, function, ...) \
{ void *_P; for (size_t _I = 0; (_P = chck_ring_pool_iter(pool, &_I, false));) function(_P, ##__VA_ARGS__); }

#define chck_ring_pool_for_each_call_reverse(pool, function, ...) \
{ void *_P; for (size_t _I = (pool)->items.count - 1; (_P = chck_ring_pool_iter(pool, &_I, true));) function(_P, ##__VA_ARGS__); }

#define chck_ring_pool_for_each(pool, pos) \
   for (size_t _I = 0; (pos = chck_ring_pool_iter(pool, &_I, false));)

#define chck_ring_pool_for_each_reverse(pool, pos) \
   for (size_t _I = (pool)->items.count - 1; (pos = chck_ring_pool_iter(pool, &_I, true));)

CHCK_NONULL bool chck_ring_pool(struct chck_ring_pool *pool, size_t grow, size_t capacity, size_t member_size);
CHCK_NONULL bool chck_ring_pool_from_c_array(struct chck_ring_pool *pool, const void *items, size_t memb, size_t growStep, size_t memberSize);
void chck_ring_pool_release(struct chck_ring_pool *pool);
CHCK_NONULL void chck_ring_pool_flush(struct chck_ring_pool *pool);
CHCK_NONULLV(1) void* chck_ring_pool_push_front(struct chck_ring_pool *pool, const void *data);
CHCK_NONULLV(1) void* chck_ring_pool_push_back(struct chck_ring_pool *pool, const void *data);
CHCK_NONULL void* chck_ring_pool_pop_first(struct chck_ring_pool *pool);
CHCK_NONULL void* chck_ring_pool_pop_last(struct chck_ring_pool *pool);
CHCK_NONULL void* chck_ring_pool_iter(const struct chck_ring_pool *pool, size_t *iter, bool reverse);
CHCK_NONULLV(1) bool chck_ring_pool_set_c_array(struct chck_ring_pool *pool, const void *items, size_t memb); /* struct item *c_array; */
CHCK_NONULLV(1) void* chck_ring_pool_to_c_array(struct chck_ring_pool *pool, size_t *memb); /* struct item *c_array; */

#endif /* __chck_pool__ */
