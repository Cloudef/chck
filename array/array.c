#include "array.h"
#include <stdio.h> /* for fprintf */
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

typedef struct _chckArray {
   char *name;
   void **buffer;
   size_t step, items, allocated;
} _chckArray;

static char *chckStrdup(const char *str)
{
   char *cpy;
   size_t size = strlen(str);

   if (!(cpy = calloc(1, size + 1)))
      return NULL;

   memcpy(cpy, str, size);
   return cpy;
}

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

   memset(tmp + array->allocated, 0, newItems * sizeof(void*));
   array->buffer = tmp;
   array->allocated = newItems;
   return RETURN_OK;
}

chckArray* chckArrayNew(const char *name, size_t growStep, size_t initialItems)
{
   chckArray *array;
   assert(name);

   if (!(array = calloc(1, sizeof(chckArray))))
      goto fail;

   if (name && !(array->name = chckStrdup(name)))
      goto fail;

   array->step = (growStep ? growStep : 32);

   if (initialItems > 0)
      chckArrayResize(array, initialItems);

   return array;

fail:
   chckArrayFree(array);
   return 0;
}

void chckArrayFree(chckArray *array)
{
   assert(array);

   chckArrayFlush(array);

   if (array->name)
      free(array->name);

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

const char* chckArrayGetName(const chckArray *array)
{
   assert(array);
   return array->name;
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

void* chckArrayAddAt(chckArray *array, void *item, chckArrayIndex index)
{
   assert(array && index <= array->items);

   if (array->allocated < index && chckArrayResize(array, array->allocated + array->step) != RETURN_OK)
      return 0;

   if (index >= array->items)
      array->items = index + 1;

   return (array->buffer[index] = item);
}

void* chckArrayAdd(chckArray *array, void *item)
{
   assert(array);
   return chckArrayAddAt(array, item, array->items);
}

void chckArrayRemoveAt(chckArray *array, chckArrayIndex index)
{
   assert(array && index <= array->items);

   if (index < array->items)
      memmove(&array->buffer[index], &array->buffer[index + 1], array->items - index);

   array->items--;
}

void chckArrayRemove(chckArray *array, void *item)
{
   assert(array && item);

   size_t iter;
   void *current;
   for (iter = 0; (current = chckArrayIter(array, &iter));) {
      if (current != item)
         continue;

      chckArrayRemoveAt(array, iter - 1);
      break;
   }
}

void* chckArrayIter(const chckArray *array, size_t *iter)
{
   assert(array && iter);

   if (*iter >= array->items)
      return NULL;

   return array->buffer[(*iter)++];
}

void chckArrayIterCall(const chckArray *array, void (*function)(void *item))
{
   assert(array);

   size_t iter;
   void *current;
   for (iter = 0; (current = chckArrayIter(array, &iter));)
      function(current);
}

/* vim: set ts=8 sw=3 tw=0 :*/
