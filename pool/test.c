#include "pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct item {
   int a;
   void *b;
};

static void printa(struct item *item)
{
   printf("item::%d\n", item->a);
}

int main(void)
{
   /* TEST: pool */
   {

      chckPool *pool = chckPoolNew(32, 3, sizeof(struct item));
      assert(pool != NULL);

      chckPoolItem a = chckPoolAdd(pool, sizeof(struct item));
      chckPoolItem b = chckPoolAdd(pool, sizeof(struct item));
      chckPoolItem c = chckPoolAdd(pool, sizeof(struct item));

      assert(a != 0 && b != 0 && c != 0);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chckPoolIter(pool, &iter, NULL)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == (sizeof(struct item) + 1) * 3);
      chckPoolRemove(pool, b);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter, NULL)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 2);
      assert(iter == (sizeof(struct item) + 1) * 3);

      chckPoolAdd(pool, sizeof(struct item));

      iter = 0;
      while ((current = chckPoolIter(pool, &iter, NULL)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      chckPoolRemove(pool, c);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter, NULL)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 2);
      assert(iter == (sizeof(struct item) + 1) * 2);

      chckPoolAdd(pool, sizeof(struct item));

      iter = 0;
      while ((current = chckPoolIter(pool, &iter, NULL)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      struct item *itemA = chckPoolGet(pool, a);
      struct item *itemB = chckPoolGet(pool, b);
      struct item *itemC = chckPoolGet(pool, c);
      itemA->a = 1;
      itemB->a = 2;
      itemC->a = 3;

      assert(chckPoolGetAt(pool, 0) == itemA);
      assert(chckPoolGetAt(pool, 1) == itemB);
      assert(chckPoolGetAt(pool, 2) == itemC);
      chckPoolIterCall(pool, printa);

      chckPoolFlush(pool);
      chckPoolFree(pool);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
