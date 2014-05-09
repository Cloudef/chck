#include "array.h"
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

typedef struct _chckArray {
   void **buffer;
   size_t step, items, allocated;
} _chckArray;

static int chckArrayResize(chckArray *array, size_t newItems)
{
   void *tmp = NULL;
   assert(newItems != 0 && newItems != array->allocated);

   if (array->buffer && array->allocated < newItems)
      tmp = realloc(array->buffer, newItems * sizeof(void*));

   if (!tmp) {
      if (!(tmp = malloc(newItems * sizeof(void*))))
         return RETURN_FAIL;

      if (array->buffer) {
         memcpy(tmp, array->buffer, (array->items < newItems ? array->items : newItems) * sizeof(void*));
         free(array->buffer);
      }
   }

   array->buffer = tmp;
   array->allocated = newItems;
   return RETURN_OK;
}

chckArray* chckArrayNew(size_t growStep, size_t initialItems)
{
   chckArray *array;

   if (!(array = calloc(1, sizeof(chckArray))))
      goto fail;

   array->step = (growStep ? growStep : 32);

   if (initialItems > 0)
      chckArrayResize(array, initialItems);

   return array;

fail:
   chckArrayFree(array);
   return NULL;
}

chckArray* chckArrayNewFromCArray(void *items, size_t memb, size_t growStep)
{
   chckArray *array;

   if (!(array = chckArrayNew(growStep, 0)))
      return NULL;

   if (!chckArraySetCArray(array, items, memb))
      goto fail;

   return array;

fail:
   chckArrayFree(array);
   return NULL;
}

void chckArrayFree(chckArray *array)
{
   assert(array);
   chckArrayFlush(array);
   free(array);
}

void chckArrayFlush(chckArray *array)
{
   assert(array);

   if (array->buffer)
      free(array->buffer);

   array->buffer = NULL;
   array->allocated = array->items = 0;
}

size_t chckArrayCount(const chckArray *array)
{
   assert(array);
   return array->items;
}

void* chckArrayGet(const chckArray *array, chckArrayIndex index)
{
   assert(index < array->items);
   return array->buffer[index];
}

void* chckArrayGetLast(const chckArray *array)
{
   return (array->items ? array->buffer[array->items - 1] : NULL);
}

void* chckArrayAddAt(chckArray *array, const void *item, chckArrayIndex index)
{
   assert(array && index <= array->items);

   if (array->allocated <= index && chckArrayResize(array, array->allocated + array->step) != RETURN_OK)
      return NULL;

   if (index >= array->items)
      array->items = index + 1;

   return (array->buffer[index] = (void*)item);
}

void* chckArrayAdd(chckArray *array, const void *item)
{
   assert(array);
   return chckArrayAddAt(array, item, array->items);
}

void chckArrayRemoveAt(chckArray *array, chckArrayIndex index)
{
   assert(array && index < array->items);

   if (index + 1 < array->items)
      memmove(&array->buffer[index], &array->buffer[index+ 1], (array->items - (index + 1)) * sizeof(void*));

   if (--array->items <= 0)
      chckArrayFlush(array);
}

void chckArrayRemove(chckArray *array, const void *item)
{
   assert(array && item);

   chckArrayIndex iter;
   void *current;
   for (iter = 0; (current = chckArrayIter(array, &iter));) {
      if (current != item)
         continue;

      chckArrayRemoveAt(array, iter - 1);
      break;
   }
}

void* chckArrayIter(const chckArray *array, chckArrayIndex *iter)
{
   assert(array && iter);

   if (*iter >= array->items)
      return NULL;

   return array->buffer[(*iter)++];
}

int chckArraySetCArray(chckArray *array, void *items, size_t memb)
{
   void **copy = NULL;
   assert(array);

   if (items && memb > 0) {
      if (!(copy = calloc(memb, sizeof(void*))))
         return RETURN_FAIL;

      memcpy(copy, items, memb * sizeof(void*));
   }

   chckArrayFlush(array);

   array->buffer = copy;
   array->items = array->allocated = memb;
   return RETURN_OK;
}

void* chckArrayToCArray(chckArray *array, size_t *outMemb)
{
   assert(array);

   if (outMemb)
      *outMemb = array->items;

   return array->buffer;
}

/* vim: set ts=8 sw=3 tw=0 :*/
