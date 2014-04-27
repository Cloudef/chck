#ifndef __chck_array__
#define __chck_array__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef struct _chckArray chckArray;
typedef size_t chckArrayIndex;

chckArray* chckArrayNew(size_t growStep, size_t initialItems);
void chckArrayFree(chckArray *array);
void chckArrayFlush(chckArray *array);
size_t chckArrayCount(const chckArray *array);
void* chckArrayGet(const chckArray *array, chckArrayIndex index);
void* chckArrayAdd(chckArray *array, void *item);
void* chckArrayAddAt(chckArray *array, void *item, chckArrayIndex index);
void chckArrayRemove(chckArray *array, void *item);
void chckArrayRemoveAt(chckArray *array, chckArrayIndex index);
void* chckArrayIter(const chckArray *array, size_t *iter);
void chckArrayIterCall(const chckArray *array, void (*function)(void *item));

#endif /* __chck_array__ */

/* vim: set ts=8 sw=3 tw=0 :*/
