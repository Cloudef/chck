#ifndef __chck_array__
#define __chck_array__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef struct _chckArray chckArray;
typedef size_t chckArrayIndex;

#define chckArrayIterCall(array, function, ...) \
{ size_t i; void *p; for (i = 0; (p = chckArrayIter(array, &i));) function(p, ##__VA_ARGS__); }

chckArray* chckArrayNew(size_t growStep, size_t initialItems);
chckArray* chckArrayNewFromCArray(void *items, size_t memb, size_t growStep);
void chckArrayFree(chckArray *array);
void chckArrayFlush(chckArray *array);
size_t chckArrayCount(const chckArray *array);
void* chckArrayGet(const chckArray *array, chckArrayIndex index);
void* chckArrayAdd(chckArray *array, const void *item);
void* chckArrayAddAt(chckArray *array, const void *item, chckArrayIndex index);
void chckArrayRemove(chckArray *array, const void *item);
void chckArrayRemoveAt(chckArray *array, chckArrayIndex index);
void* chckArrayIter(const chckArray *array, size_t *iter);
int chckArraySetCArray(chckArray *array, void *item, size_t memb);
void* chckArrayToCArray(chckArray *array, size_t *memb);

#endif /* __chck_array__ */

/* vim: set ts=8 sw=3 tw=0 :*/
