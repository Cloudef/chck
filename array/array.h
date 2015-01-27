#ifndef __chck_array__
#define __chck_array__

#include <stddef.h> /* for size_t */

typedef size_t chckArrayIndex;
typedef struct _chckArray chckArray;

#define chckArrayIterCall(array, function, ...) \
{ chckArrayIndex i; void *p; for (i = 0; (p = chckArrayIter(array, &i));) function(p, ##__VA_ARGS__); }

chckArray* chckArrayNew(size_t growStep, size_t initialItems);
chckArray* chckArrayNewFromCArray(void *items, size_t memb, size_t growStep);
void chckArrayFree(chckArray *array);
void chckArrayFlush(chckArray *array);
size_t chckArrayCount(const chckArray *array);
void* chckArrayGet(const chckArray *array, chckArrayIndex index);
void* chckArrayGetLast(const chckArray *array);
void* chckArrayAdd(chckArray *array, const void *item);
void* chckArrayAddAt(chckArray *array, const void *item, chckArrayIndex index);
void chckArrayRemove(chckArray *array, const void *item);
void chckArrayRemoveAt(chckArray *array, chckArrayIndex index);
void* chckArrayIter(const chckArray *array, chckArrayIndex *iter);
int chckArraySetCArray(chckArray *array, void *items, size_t memb); /* Item **cArray; */
void* chckArrayToCArray(chckArray *array, size_t *outMemb); /* Item **cArray; */

#endif /* __chck_array__ */

/* vim: set ts=8 sw=3 tw=0 :*/
