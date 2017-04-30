#ifndef __chck_pool__
#define __chck_pool__

#include <chck/macros.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

struct chck_pool_buffer {
   // pointer to contents
   uint8_t *buffer;

   // growth step and member size (step == grow * member_size)
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

bool chck_pool(struct chck_pool *pool, size_t grow, size_t capacity, size_t member_size);
bool chck_pool_from_c_array(struct chck_pool *pool, const void *items, size_t memb, size_t grow, size_t member_size);
void chck_pool_release(struct chck_pool *pool);
void chck_pool_flush(struct chck_pool *pool);
void chck_pool_print(const struct chck_pool *pool, FILE *out);
void* chck_pool_get(const struct chck_pool *pool, size_t index);
void* chck_pool_get_last(const struct chck_pool *pool);
void* chck_pool_add(struct chck_pool *pool, const void *data, size_t *out_index);
void chck_pool_remove(struct chck_pool *pool, size_t index);
void* chck_pool_iter(const struct chck_pool *pool, size_t *iter, bool reverse);
bool chck_pool_set_c_array(struct chck_pool *pool, const void *items, size_t memb); /* struct item *c_array; */
void* chck_pool_to_c_array(struct chck_pool *pool, size_t *memb); /* struct item *c_array; (contains holes) */

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

bool chck_iter_pool(struct chck_iter_pool *pool, size_t grow, size_t capacity, size_t member_size);
bool chck_iter_pool_from_c_array(struct chck_iter_pool *pool, const void *items, size_t memb, size_t grow_step, size_t member_size);
void chck_iter_pool_release(struct chck_iter_pool *pool);
void chck_iter_pool_flush(struct chck_iter_pool *pool);
void chck_iter_pool_empty(struct chck_iter_pool *pool);
void* chck_iter_pool_get(const struct chck_iter_pool *pool, size_t index);
void* chck_iter_pool_get_last(const struct chck_iter_pool *pool);
void* chck_iter_pool_push_front(struct chck_iter_pool *pool, const void *data);
void* chck_iter_pool_push_back(struct chck_iter_pool *pool, const void *data);
void* chck_iter_pool_insert(struct chck_iter_pool *pool, size_t index, const void *data);
void chck_iter_pool_remove(struct chck_iter_pool *pool, size_t index);
void* chck_iter_pool_iter(const struct chck_iter_pool *pool, size_t *iter, bool reverse);
bool chck_iter_pool_set_c_array(struct chck_iter_pool *pool, const void *items, size_t memb); /* struct item *c_array; */
void* chck_iter_pool_to_c_array(struct chck_iter_pool *pool, size_t *memb); /* struct item *c_array; */

#endif /* __chck_pool__ */
