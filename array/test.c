#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void printint(int *item)
{
   printf("item::%d\n", *item);
}

int main(void)
{
   /* TEST: array */
   {
      chckArray *array = chckArrayNew(32, 3);
      assert(array != NULL);

      int aa = 0, bb = 1, cc = 2;

      int *a = chckArrayAdd(array, &aa);
      int *b = chckArrayAdd(array, &bb);
      int *c = chckArrayAdd(array, &cc);

      assert(a != NULL && b != NULL && c != NULL);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chckArrayIter(array, &iter)))
         assert(current != NULL);

      assert(iter == 3);
      chckArrayRemove(array, b);

      iter = 0;
      while ((current = chckArrayIter(array, &iter)))
         assert(current != NULL);

      assert(chckArrayCount(array) == 2);
      assert(iter == 2);

      assert(((int**)chckArrayToCArray(array, NULL))[0] == a);
      assert(((int**)chckArrayToCArray(array, NULL))[1] == c);

      chckArrayIterCall(array, printint);

      chckArrayFlush(array);
      chckArrayFree(array);
   }

   /* TEST: array from C array */
   {
      const char *ptrs[] = { "a", "b", "c" };
      const char *ptrs2[] = { "d" };
      chckArray *array = chckArrayNewFromCArray(ptrs, 3, 32);

      assert(chckArrayCount(array) == 3);
      assert(chckArrayGet(array, 0) == ptrs[0]);
      assert(chckArrayGet(array, 1) == ptrs[1]);
      assert(chckArrayGet(array, 2) == ptrs[2]);

      chckArraySetCArray(array, ptrs2, 1);
      assert(chckArrayCount(array) == 1);
      assert(chckArrayGet(array, 0) == ptrs2[0]);

      chckArrayAdd(array, ptrs[0]);
      assert(chckArrayCount(array) == 2);
      assert(chckArrayGet(array, 1) == ptrs[0]);
      assert(chckArrayGetLast(array) == ptrs[0]);

      chckArrayFree(array);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
