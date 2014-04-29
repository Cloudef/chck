#ifndef __chck_pool__
#define __chck_pool__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef struct _chckPool chckPool;
typedef size_t chckPoolIndex;

#define chckPoolIterCall(pool, function, ...) \
{ size_t i; void *p; for (i = 0; (p = chckPoolIter(pool, &i));) function(p, ##__VA_ARGS__); }

chckPool* chckPoolNew(size_t growStep, size_t initialItems, size_t memberSize);
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

#endif /* __chck_pool__ */

/* vim: set ts=8 sw=3 tw=0 :*/
