#include "pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(void)
{
   /* TEST: pool */
   {
      struct item {
         int a;
         void *b;
      };

      chckPool *pool = chckPoolNew("MyPool", sizeof(struct item));
      assert(pool != NULL);

      struct item *a = chckPoolAdd(pool, sizeof(struct item));
      assert(a != NULL);

      struct item *b = chckPoolAdd(pool, sizeof(struct item));
      assert(b != NULL);

      struct item *c = chckPoolAdd(pool, sizeof(struct item));
      assert(c != NULL);

      struct item *current;
      size_t iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      printf("%zu :: %zu\n", iter, (sizeof(struct item) + 1) * 3);
      assert(iter == (sizeof(struct item) + 1) * 3);
      chckPoolRemove(pool, b);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      printf("%zu :: %zu\n", iter, (sizeof(struct item) + 1) * 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      b = chckPoolAdd(pool, sizeof(struct item));

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      printf("%zu :: %zu\n", iter, (sizeof(struct item) + 1) * 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      chckPoolRemove(pool, c);

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      printf("%zu :: %zu\n", iter, (sizeof(struct item) + 1) * 3);
      assert(iter == (sizeof(struct item) + 1) * 2);

      c = chckPoolAdd(pool, sizeof(struct item));

      iter = 0;
      while ((current = chckPoolIter(pool, &iter)))
         assert(current != NULL);

      printf("%zu :: %zu\n", iter, (sizeof(struct item) + 1) * 3);
      assert(iter == (sizeof(struct item) + 1) * 3);

      chckPoolFlush(pool);
      chckPoolFree(pool);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
