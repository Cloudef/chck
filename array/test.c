#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void)
{
   /* TEST: array */
   {
      chckArray *array = chckArrayNew("MyArray", 32, 3);
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

      printf("The name of our array was: %s\n", chckArrayGetName(array));
      assert(strcmp(chckArrayGetName(array), "MyArray") == 0);

      chckArrayFlush(array);
      chckArrayFree(array);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
