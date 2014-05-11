#include "lut.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void printstr(const char **str)
{
   printf("%s\n", *str);
}

static void printstr2(const char *str)
{
   printf("%s\n", str);
}

int main(void)
{
   /* TEST: lut */
   {
      chckLut *lut = chckLutNew(32, 0, sizeof(const char*));
      assert(lut != NULL);

      const char *s0 = "(0) foobar";
      const char *s1 = "(1) penguin";
      const char *s2 = "(2) ismo";

      chckLutSet(lut, 0, &s0);
      chckLutSet(lut, 1, &s1);
      chckLutSet(lut, 2, &s2);

      assert(*(const char**)chckLutGet(lut, 0) == s0);
      assert(*(const char**)chckLutGet(lut, 1) == s1);
      assert(*(const char**)chckLutGet(lut, 2) == s2);

      chckLutIterCall(lut, printstr);

      chckLutFlush(lut);

      chckLutStrSet(lut, "s0", &s0);
      chckLutStrSet(lut, "s1", &s1);
      chckLutStrSet(lut, "s2", &s2);

      assert(*(const char**)chckLutStrGet(lut, "s0") == s0);
      assert(*(const char**)chckLutStrGet(lut, "s1") == s1);
      assert(*(const char**)chckLutStrGet(lut, "s2") == s2);

      chckLutIterCall(lut, printstr);

      chckLutStrSet(lut, "s0", &s1);
      assert(*(const char**)chckLutStrGet(lut, "s0") == s1);

      chckLutFree(lut);
   }

   /* TEST: hash table */
   {
      chckHashTable *table = chckHashTableNew(1024);
      assert(table != NULL);

      const char *s0 = "(0) foobar";
      const char *s1 = "(1) penguin";
      const char *s2 = "(2) ismo";

      chckHashTableSet(table, 0, s0, 0);
      chckHashTableSet(table, 1, s1, 0);
      chckHashTableSet(table, 2, s2, 0);

      assert(chckHashTableGet(table, 0) == s0);
      assert(chckHashTableGet(table, 1) == s1);
      assert(chckHashTableGet(table, 2) == s2);

      unsigned int i;
      for (i = 0; i < 10000 * 10; ++i)
         chckHashTableSet(table, i, &i, sizeof(unsigned int));
      for (i = 0; i < 10000 * 10; ++i)
         assert(*(unsigned int*)chckHashTableGet(table, i) == i);

      chckHashTableFlush(table);

      chckHashTableStrSet(table, "s0", s0, 0);
      chckHashTableStrSet(table, "s1", s1, 0);
      chckHashTableStrSet(table, "s2", s2, 0);

      chckHashTableIterCall(table, printstr2);

      assert(chckHashTableStrGet(table, "s0") == s0);
      assert(chckHashTableStrGet(table, "s1") == s1);
      assert(chckHashTableStrGet(table, "s2") == s2);

      chckHashTableStrSet(table, "s0", s1, 0);
      assert(chckHashTableStrGet(table, "s0") == s1);

      chckHashTableFree(table);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
