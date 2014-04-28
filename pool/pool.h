#ifndef __chck_pool__
#define __chck_pool__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef struct _chckPool chckPool;
typedef size_t chckPoolItem;

#define chckPoolIterCall(pool, function, ...) \
{ size_t i; void *p; for (i = 0; (p = chckPoolIter(pool, &i, NULL));) function(p, ##__VA_ARGS__); }

chckPool* chckPoolNew(size_t growStep, size_t initialItems, size_t memberSize);
void chckPoolFree(chckPool *pool);
void chckPoolFlush(chckPool *pool);
size_t chckPoolCount(const chckPool *pool);
void* chckPoolGet(const chckPool *pool, chckPoolItem item);
void* chckPoolGetAt(const chckPool *pool, size_t index);
void* chckPoolGetLast(const chckPool *pool);
chckPoolItem chckPoolAdd(chckPool *pool, size_t size);
void* chckPoolAddEx(chckPool *pool, size_t size, chckPoolItem *item);
void chckPoolRemove(chckPool *pool, chckPoolItem item);
void* chckPoolIter(const chckPool *pool, size_t *iter, chckPoolItem *item);

#endif /* __chck_pool__ */

/* vim: set ts=8 sw=3 tw=0 :*/
