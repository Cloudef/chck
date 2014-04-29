#ifndef __chck_pool__
#define __chck_pool__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef size_t chckPoolIndex;
typedef struct _chckPool chckPool;
typedef struct _chckIterPool chckIterPool;

/**
 * Pools are manual memory buffers for your data (usually structs).
 * Pools may contain holes as whenever you remove item, the space is not removed, but instead marked as unused.
 * To access pool items, dont rely on the returned pointers, but use the indices instead.
 * The pointers may point to garbage whenever you add/remove item (as the buffer may be resized).
 */

#define chckPoolIterCall(pool, function, ...) \
{ size_t i; void *p; for (i = 0; (p = chckPoolIter(pool, &i));) function(p, ##__VA_ARGS__); }

chckPool* chckPoolNew(size_t growStep, size_t capacity, size_t memberSize);
chckPool* chckPoolNewFromCArray(void *items, size_t memb, size_t growStep, size_t memberSize);
void chckPoolFree(chckPool *pool);
void chckPoolFlush(chckPool *pool);
size_t chckPoolCount(const chckPool *pool);
void* chckPoolGet(const chckPool *pool, chckPoolIndex index);
void* chckPoolGetLast(const chckPool *pool);
void* chckPoolAdd(chckPool *pool, const void *data, chckPoolIndex *outIndex);
void chckPoolRemove(chckPool *pool, chckPoolIndex index);
void* chckPoolIter(const chckPool *pool, chckPoolIndex *iter);
int chckPoolSetCArray(chckPool *pool, void *items, size_t memb); /* Item *cArray; */
void* chckPoolToCArray(chckPool *pool, size_t *memb); /* Item *cArray; */

/**
 * IterPools don't have holes in buffer.
 * Whenever you remove a item from IterPool, the items after that get memmoved.
 * Thus the indices returned by IterPool functions are _not_ safe.
 *
 * As the name implies, use this pool only if you need to access items by iteration.
 */

#define chckIterPoolIterCall(pool, function, ...) \
{ size_t i; void *p; for (i = 0; (p = chckIterPoolIter(pool, &i));) function(p, ##__VA_ARGS__); }

chckIterPool* chckIterPoolNew(size_t growStep, size_t capacity, size_t memberSize);
chckIterPool* chckIterPoolNewFromCArray(void *items, size_t memb, size_t growStep, size_t memberSize);
void chckIterPoolFree(chckIterPool *pool);
void chckIterPoolFlush(chckIterPool *pool);
size_t chckIterPoolCount(const chckIterPool *pool);
void* chckIterPoolGet(const chckIterPool *pool, chckPoolIndex index);
void* chckIterPoolGetLast(const chckIterPool *pool);
void* chckIterPoolIter(const chckIterPool *pool, chckPoolIndex *iter);
void* chckIterPoolAdd(chckIterPool *pool, const void *data, chckPoolIndex *outIndex);
void chckIterPoolRemove(chckIterPool *pool, chckPoolIndex index);
int chckIterPoolSetCArray(chckIterPool *pool, void *items, size_t memb); /* Item *cArray; */
void* chckIterPoolToCArray(chckIterPool *pool, size_t *memb); /* Item *cArray; */

#endif /* __chck_pool__ */

/* vim: set ts=8 sw=3 tw=0 :*/
