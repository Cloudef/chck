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

      chckPoolIndex a, b, c;
      chckPoolAdd(pool, (&(struct item){1, NULL}), &a);
      chckPoolAdd(pool, NULL, &b);
      chckPoolAdd(pool, NULL, &c);

      assert(a == 0 && b == 1 && c == 2);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == 3);
      chckPoolRemove(pool, b);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 2);
      assert(iter == 2);

      chckPoolAdd(pool, NULL, NULL);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == 3);

      chckPoolRemove(pool, c);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 2);
      assert(iter == 2);

      chckPoolAdd(pool, NULL, NULL);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == 3);

      struct item *itemA = chckPoolGet(pool, a);
      struct item *itemB = chckPoolGet(pool, b);
      struct item *itemC = chckPoolGet(pool, c);
      itemA->a = 1;
      itemB->a = 2;
      itemC->a = 3;

      assert(chckPoolGet(pool, 0) == itemA);
      assert(chckPoolGet(pool, 1) == itemB);
      assert(chckPoolGet(pool, 2) == itemC);
      assert(chckPoolGetLast(pool) == itemC);
      assert(&((struct item*)chckPoolToCArray(pool, NULL))[0] == itemA);
      assert(&((struct item*)chckPoolToCArray(pool, NULL))[1] == itemB);
      assert(&((struct item*)chckPoolToCArray(pool, NULL))[2] == itemC);
      chckPoolIterCall(pool, printa);

      chckPoolFlush(pool);
      chckPoolFree(pool);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
