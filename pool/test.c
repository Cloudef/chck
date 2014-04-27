#include "pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void)
{
   /* TEST: pool */
   {
      struct item {
         int a;
         void *b;
      };

      chckPool *pool = chckPoolNew("MyPool", 32, 3, sizeof(struct item));
      assert(pool != NULL);

      chckPoolItem a = chckPoolAdd(pool, sizeof(struct item));
      chckPoolItem b = chckPoolAdd(pool, sizeof(struct item));
      chckPoolItem c = chckPoolAdd(pool, sizeof(struct item));

      assert(a != 0 && b != 0 && c != 0);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == (sizeof(struct item) + 1) * 3);
      chckPoolRemove(pool, b);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 2);
      assert(iter == (sizeof(struct item) + 1) * 3);

      b = chckPoolAdd(pool, sizeof(struct item));

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      chckPoolRemove(pool, c);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 2);
      assert(iter == (sizeof(struct item) + 1) * 2);

      c = chckPoolAdd(pool, sizeof(struct item));

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      assert(chckPoolCount(pool) == 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      printf("The name of our pool was: %s\n", chckPoolGetName(pool));
      assert(strcmp(chckPoolGetName(pool), "MyPool") == 0);

      chckPoolFlush(pool);
      chckPoolFree(pool);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
