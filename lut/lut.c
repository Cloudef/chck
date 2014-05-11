#include "lut.h"
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

typedef struct _chckLut {
   unsigned char *table;
   size_t size, member;
   int set;
} _chckLut;

static unsigned int hashint(unsigned int a)
{
   a += ~(a<<15);
   a ^=  (a>>10);
   a +=  (a<<3);
   a ^=  (a>>6);
   a += ~(a<<11);
   a ^=  (a>>16);
   return a;
}

static unsigned int hashstr(const char *str)
{
   int c;
   unsigned int hash = 5381;
   while ((c = *str++)) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
   return hash;
}

static char* chckStrdup(const char *str)
{
   char *cpy;
   size_t size = strlen(str);

   if (!(cpy = calloc(1, size + 1)))
      return NULL;

   memcpy(cpy, str, size);
   return cpy;
}

static void* chckLutGetIndex(chckLut *lut, unsigned int index)
{
   return lut->table + index * lut->member;
}

static void chckLutSetIndex(chckLut *lut, unsigned int index, const void *data)
{
   assert(index < lut->size);

   if (data) {
      memcpy(lut->table + index * lut->member, data, lut->member);
   } else {
      memset(lut->table + index * lut->member, lut->set, lut->member);
   }
}

chckLut* chckLutNew(size_t size, int set, size_t member)
{
   chckLut *lut = NULL;

   if (!(lut = calloc(1, sizeof(chckLut))))
      goto fail;

   if (!(lut->table = calloc(size, member)))
      goto fail;

   lut->set = set;
   lut->size = size;
   lut->member = member;

   if (lut->set != 0)
      chckLutFlush(lut);

   return lut;

fail:
   if (lut)
      chckLutFree(lut);
   return NULL;
}

void chckLutFree(chckLut *lut)
{
   assert(lut);

   if (lut->table)
      free(lut->table);

   free(lut);
}

void chckLutFlush(chckLut *lut)
{
   assert(lut);
   memset(lut->table, lut->set, lut->size * lut->member);
}

void* chckLutGet(chckLut *lut, unsigned int lookup)
{
   assert(lut);
   return chckLutGetIndex(lut, hashint(lookup) & (lut->size - 1));
}

void* chckLutStrGet(chckLut *lut, const char *str)
{
   assert(lut);
   return chckLutGetIndex(lut, hashstr(str) & (lut->size - 1));
}

void chckLutSet(chckLut *lut, unsigned int lookup, const void *data)
{
   assert(lut);
   chckLutSetIndex(lut, hashint(lookup) & (lut->size - 1), data);
}

void chckLutStrSet(chckLut *lut, const char *str, const void *data)
{
   assert(lut);
   chckLutSetIndex(lut, hashstr(str) & (lut->size - 1), data);
}

void* chckLutIter(chckLut *lut, size_t *iter)
{
   void *ptr;
   assert(lut);

   do {
      if (*iter >= lut->size)
         return NULL;

      ptr = lut->table + (*iter)++ * lut->member;
   } while (ptr && *(int*)ptr == lut->set);

   return ptr;
}

struct _chckHashItem {
   void *data;
   size_t size;
   char *strKey;
   unsigned int uintKey;
   struct _chckHashItem *next;
};

typedef struct _chckHashTable {
   struct _chckLut *lut;
} _chckHashTable;

static int chckHashItemInit(struct _chckHashItem *item, const char *strKey, unsigned int uintKey, const void *data, size_t member)
{
   void *dataCopy = NULL, *strCopy = NULL;

   if (data && member > 0) {
      if (!(dataCopy = malloc(member)))
         goto fail;

      memcpy(dataCopy, data, member);
   }

   if (strKey && !(strCopy = chckStrdup(strKey)))
      goto fail;

   memset(item, 0, sizeof(struct _chckHashItem));
   item->data = (member > 0 ? dataCopy : (void*)data);
   item->size = member;
   item->uintKey = uintKey;
   item->strKey = strCopy;
   return RETURN_OK;

fail:
   if (dataCopy)
      free(dataCopy);
   if (strCopy)
      free(strCopy);
   return RETURN_FAIL;
}

static struct _chckHashItem* chckHashItemCopy(const struct _chckHashItem *item)
{
   struct _chckHashItem *copy;
   assert(item);

   if (!(copy = calloc(1, sizeof(struct _chckHashItem))))
      return NULL;

   memcpy(copy, item, sizeof(struct _chckHashItem));
   return copy;
}

static void chckHashItemFree(struct _chckHashItem *item, int freeSelf)
{
   if (item->size > 0 && item->data)
      free(item->data);

   if (item->strKey)
      free(item->strKey);

   if (freeSelf)
      free(item);
}

static int chckHashTableSetIndex(chckHashTable *table, unsigned int index, struct _chckHashItem *newItem)
{
   assert(table && newItem);

   struct _chckHashItem *item = chckLutGetIndex(table->lut, index);
   struct _chckHashItem *first = item, *prev = item;

   if (item->strKey) {
      for (; item && (!item->strKey || strcmp(item->strKey, newItem->strKey)); item = item->next) prev = item;
   } else {
      for (; item && (item->strKey || item->uintKey != newItem->uintKey); item = item->next) prev = item;
   }

   if (item && item->data) {
      struct _chckHashItem *next = item->next;
      chckHashItemFree(item, (item != first));

      if (item != first) {
         if (newItem->data) {
            /* replace collision in list */
            if (!(prev->next = chckHashItemCopy(newItem)))
               return RETURN_FAIL;
         } else {
            /* remove collision from list */
            prev->next = next;
            chckHashItemFree(newItem, 0);
         }
      } else {
         if (newItem->data) {
            /* memcpy pure item (not a collision) */
            memcpy(item, newItem, sizeof(struct _chckHashItem));
            item->next = next;
         } else {
            /* remove item */
            chckLutSetIndex(table->lut, index, NULL);
            chckHashItemFree(newItem, 0);
         }
      }
   } else if (newItem->data && (item = prev) && item->data) {
      /* add collision to list */
      if (!(item->next = chckHashItemCopy(newItem)))
         return RETURN_FAIL;
   } else if (newItem->data) {
      /* add pure item to table (not a collision) */
      chckLutSetIndex(table->lut, index, newItem);
   } else {
      chckHashItemFree(newItem, 0);
   }

   return RETURN_OK;
}

chckHashTable* chckHashTableNew(size_t size)
{
   chckHashTable *table = NULL;

   if (!(table = calloc(1, sizeof(chckHashTable))))
      goto fail;

   if (!(table->lut = chckLutNew(size, 0, sizeof(struct _chckHashItem))))
      goto fail;

   return table;

fail:
   if (table)
      chckHashTableFree(table);
   return NULL;
}

void chckHashTableFree(chckHashTable *table)
{
   assert(table);

   chckHashTableFlush(table);

   if (table->lut)
      chckLutFree(table->lut);

   free(table);
}

void chckHashTableFlush(chckHashTable *table)
{
   size_t iter = 0;
   struct _chckHashItem *item;
   assert(table);

   while ((item = chckLutIter(table->lut, &iter))) {
      struct _chckHashItem *next = item->next;
      chckHashItemFree(item, 0);
      for (item = next; item; item = next) {
         next = item->next;
         chckHashItemFree(item, 1);
      }
   }

   chckLutFlush(table->lut);
}

int chckHashTableSet(chckHashTable *table, unsigned int key, const void *data, size_t member)
{
   assert(table);

   struct _chckHashItem item;
   if (chckHashItemInit(&item, NULL, key, data, member) != RETURN_OK)
      return RETURN_FAIL;

   return chckHashTableSetIndex(table, hashint(key) & (table->lut->size - 1), &item);
}

int chckHashTableStrSet(chckHashTable *table, const char *str, const void *data, size_t member)
{
   assert(table && str);

   struct _chckHashItem item;
   if (chckHashItemInit(&item, str, -1, data, member) != RETURN_OK)
      return RETURN_FAIL;

   return chckHashTableSetIndex(table, hashstr(str) & (table->lut->size - 1), &item);
}

void* chckHashTableGet(chckHashTable *table, unsigned int key)
{
   assert(table);
   struct _chckHashItem *item = chckLutGet(table->lut, key);
   for (; item && (item->strKey || item->uintKey != key); item = item->next);
   return (item ? (void*)item->data : NULL);
}

void* chckHashTableStrGet(chckHashTable *table, const char *str)
{
   assert(table && str);
   struct _chckHashItem *item = chckLutStrGet(table->lut, str);
   for (; item && (!item->strKey || strcmp(item->strKey, str)); item = item->next);
   return (item ? (void*)item->data : NULL);
}

void* chckHashTableIter(chckHashTable *table, chckHashTableIterator *iterator)
{
   assert(table);

   if (iterator->iter >= table->lut->size)
      return NULL;

   struct _chckHashItem *item = iterator->ptr;

   if (item) {
      iterator->ptr = item->next;
   } else {
      item = chckLutIter(table->lut, &iterator->iter);
      iterator->ptr = (item ? item->next : NULL);
   }

   return (item ? item->data : NULL);
}

/* vim: set ts=8 sw=3 tw=0 :*/
