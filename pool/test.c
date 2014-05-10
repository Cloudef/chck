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

   /* TEST: iter pool */
   {
      chckIterPool *pool = chckIterPoolNew(32, 3, sizeof(struct item));
      assert(pool != NULL);

      chckPoolIndex a, b, c;
      chckIterPoolAdd(pool, (&(struct item){1, NULL}), &a);
      chckIterPoolAdd(pool, NULL, &b);
      chckIterPoolAdd(pool, NULL, &c);

      assert(a == 0 && b == 1 && c == 2);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chckIterPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckIterPoolCount(pool) == 3);
      assert(iter == 3);
      chckIterPoolRemove(pool, b);

      iter = 0;
      while ((current = chckIterPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckIterPoolCount(pool) == 2);
      assert(iter == 2);

      chckIterPoolAdd(pool, NULL, NULL);

      iter = 0;
      while ((current = chckIterPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckIterPoolCount(pool) == 3);
      assert(iter == 3);

      chckIterPoolRemove(pool, c);

      iter = 0;
      while ((current = chckIterPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckIterPoolCount(pool) == 2);
      assert(iter == 2);

      chckIterPoolAdd(pool, NULL, NULL);

      iter = 0;
      while ((current = chckIterPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckIterPoolCount(pool) == 3);
      assert(iter == 3);

      struct item *itemA = chckIterPoolGet(pool, a);
      struct item *itemB = chckIterPoolGet(pool, b);
      struct item *itemC = chckIterPoolGet(pool, c);
      itemA->a = 1;
      itemB->a = 2;
      itemC->a = 3;

      assert(chckIterPoolGet(pool, 0) == itemA);
      assert(chckIterPoolGet(pool, 1) == itemB);
      assert(chckIterPoolGet(pool, 2) == itemC);
      assert(chckIterPoolGetLast(pool) == itemC);
      assert(&((struct item*)chckIterPoolToCArray(pool, NULL))[0] == itemA);
      assert(&((struct item*)chckIterPoolToCArray(pool, NULL))[1] == itemB);
      assert(&((struct item*)chckIterPoolToCArray(pool, NULL))[2] == itemC);
      chckIterPoolIterCall(pool, printa);

      chckIterPoolFlush(pool);
      chckIterPoolFree(pool);
   }

   /* TEST: ring pool */
   {
      chckRingPool *pool = chckRingPoolNew(32, 3, sizeof(struct item));
      assert(pool != NULL);

      void *a, *b, *c;
      a = chckRingPoolPushEnd(pool, (&(struct item){1, NULL}));
      b = chckRingPoolPushEnd(pool, (&(struct item){2, NULL}));
      c = chckRingPoolPushEnd(pool, (&(struct item){3, NULL}));

      assert(a != NULL && b != NULL && c != NULL);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chckRingPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckRingPoolCount(pool) == 3);
      assert(iter == 3);

      assert(((struct item*)chckRingPoolPopFirst(pool))->a == 1);

      iter = 0;
      while ((current = chckRingPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckRingPoolCount(pool) == 2);
      assert(iter == 2);

      chckRingPoolPushFront(pool, NULL);

      iter = 0;
      while ((current = chckRingPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckRingPoolCount(pool) == 3);
      assert(iter == 3);

      assert(((struct item*)chckRingPoolPopLast(pool))->a == 3);

      iter = 0;
      while ((current = chckRingPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckRingPoolCount(pool) == 2);
      assert(iter == 2);

      chckRingPoolPushEnd(pool, NULL);

      iter = 0;
      while ((current = chckRingPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckRingPoolCount(pool) == 3);
      assert(iter == 3);

      chckRingPoolFlush(pool);

      chckRingPoolPushEnd(pool, (&(struct item){1, NULL}));
      chckRingPoolPushFront(pool, (&(struct item){3, NULL}));
      chckRingPoolPushEnd(pool, (&(struct item){2, NULL}));

      assert(((struct item*)chckRingPoolToCArray(pool, NULL))[0].a == 3);
      assert(((struct item*)chckRingPoolToCArray(pool, NULL))[1].a == 1);
      assert(((struct item*)chckRingPoolToCArray(pool, NULL))[2].a == 2);
      chckRingPoolIterCall(pool, printa);

      assert(((struct item*)chckRingPoolPopLast(pool))->a == 2);
      assert(((struct item*)chckRingPoolPopFirst(pool))->a == 3);
      assert(((struct item*)chckRingPoolPopLast(pool))->a == 1);

      chckRingPoolFlush(pool);
      chckRingPoolFree(pool);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
