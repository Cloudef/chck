#include "lut.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void printstr(const char **str)
{
   if (*str)
      printf("%s\n", *str);
}

int main(void)
{
   /* TEST: lut */
   {
      struct chck_lut lut;
      assert(chck_lut(&lut, 0, 32, sizeof(const char*)));

      const char *s0 = "(0) foobar";
      const char *s1 = "(1) penguin";
      const char *s2 = "(2) ismo";

      assert(chck_lut_set(&lut, 0, &s0));
      assert(chck_lut_set(&lut, 1, &s1));
      assert(chck_lut_set(&lut, 2, &s2));

      assert(*(const char**)chck_lut_get(&lut, 0) == s0);
      assert(*(const char**)chck_lut_get(&lut, 1) == s1);
      assert(*(const char**)chck_lut_get(&lut, 2) == s2);

      {
         const char **p;
         uint32_t i = 0;
         chck_lut_for_each(&lut, p) if (*p) ++i;
         assert(i == 3);
      }

      chck_lut_for_each_call(&lut, printstr);
      chck_lut_release(&lut);

      assert(chck_lut_str_set(&lut, "s0", 2, &s0));
      assert(chck_lut_str_set(&lut, "s1", 2, &s1));
      assert(chck_lut_str_set(&lut, "s2", 2, &s2));

      assert(*(const char**)chck_lut_str_get(&lut, "s0", 2) == s0);
      assert(*(const char**)chck_lut_str_get(&lut, "s1", 2) == s1);
      assert(*(const char**)chck_lut_str_get(&lut, "s2",2 ) == s2);

      chck_lut_for_each_call(&lut, printstr);

      {
         const char **p;
         uint32_t i = 0;
         chck_lut_for_each(&lut, p) if (*p) ++i;
         assert(i == 3);
      }

      chck_lut_str_set(&lut, "s0", 2, &s1);
      assert(*(const char**)chck_lut_str_get(&lut, "s0", 2) == s1);

      {
         const char **p;
         uint32_t i = 0;
         chck_lut_for_each(&lut, p) if (*p) ++i;
         assert(i == 3);
      }

      chck_lut_release(&lut);
   }

   /* TEST: hash table */
   {
      struct chck_hash_table table;
      assert(chck_hash_table(&table, 0, 32, sizeof(const char*)));

      const char *s0 = "(0) foobar";
      const char *s1 = "(1) penguin";
      const char *s2 = "(2) ismo";

      chck_hash_table_set(&table, 0, &s0);
      chck_hash_table_set(&table, 1, &s1);
      chck_hash_table_set(&table, 2, &s2);

      assert(*(const char**)chck_hash_table_get(&table, 0) == s0);
      assert(*(const char**)chck_hash_table_get(&table, 1) == s1);
      assert(*(const char**)chck_hash_table_get(&table, 2) == s2);

      {
         const char **p;
         uint32_t i = 0;
         chck_hash_table_for_each(&table, p) if (*p) ++i;
         assert(i == 3);
      }

      chck_hash_table_str_set(&table, "s0", 2, &s0);
      chck_hash_table_str_set(&table, "s1", 2, &s1);
      chck_hash_table_str_set(&table, "s2", 2, &s2);

      {
         const char **p;
         uint32_t i = 0;
         chck_hash_table_for_each(&table, p) if (*p) ++i;
         assert(i == 6);
      }

      chck_hash_table_for_each_call(&table, printstr);
      printf("[1] collisions: %u\n", chck_hash_table_collisions(&table));
      chck_hash_table_release(&table);

      chck_hash_table_str_set(&table, "s0", 2, &s0);
      chck_hash_table_str_set(&table, "s1", 2, &s1);
      chck_hash_table_str_set(&table, "s2", 2, &s2);

      assert(*(const char**)chck_hash_table_str_get(&table, "s0", 2) == s0);
      assert(*(const char**)chck_hash_table_str_get(&table, "s1", 2) == s1);
      assert(*(const char**)chck_hash_table_str_get(&table, "s2", 2) == s2);

      chck_hash_table_for_each_call(&table, printstr);

      {
         const char **p;
         uint32_t i = 0;
         chck_hash_table_for_each(&table, p) if (*p) ++i;
         assert(i == 3);
      }

      chck_hash_table_str_set(&table, "s0", 2, &s1);
      assert(*(const char**)chck_hash_table_str_get(&table, "s0", 2) == s1);

      {
         const char **p;
         uint32_t i = 0;
         chck_hash_table_for_each(&table, p) if (*p) ++i;
         assert(i == 3);
      }

      printf("[2] collisions: %u\n", chck_hash_table_collisions(&table));
      chck_hash_table_release(&table);
   }

   /* TEST: benchmark (default algorithm, number of collisions) */
   {
      static const uint32_t iters = 24;
      struct chck_hash_table table;
      assert(chck_hash_table(&table, -1, 128, sizeof(uint32_t)));

      uint32_t r[24] = {
         0xFFFFFF, 0xFFFF, 0xFF, 0xEFEF, 0xDEAD, 0xFFFFF,
         0x5B1E18, 0x7225E4, 0x17A414, 0xDC183C, 0xF8A0B8, 0xEEC494,
         0x371685, 0x0FB872, 0x414034, 0xDC4684, 0x404076, 0x9E3E7E,
         0x0C2555, 0x9DA908, 0xAC6E58, 0x773527, 0x860D2, 0x9F3CA9
      };

      for (uint32_t i = 0; i < iters; ++i) {
         chck_hash_table_set(&table, r[i], &i);
      }

      for (uint32_t i = iters / 2, d = iters / 2; i < iters; ++i, --d) {
         assert(*(uint32_t*)chck_hash_table_get(&table, r[i]) == i);
         assert(*(uint32_t*)chck_hash_table_get(&table, r[d]) == d);
      }

      printf("[3] collisions: %u\n", chck_hash_table_collisions(&table));
      chck_hash_table_release(&table);
   }

   /* TEST: benchmark (incremental algorithm with known range)
    *       mainly tests insert/get speed */
   {
      static const uint32_t iters = 0xFFFFF;
      struct chck_hash_table table;
      assert(chck_hash_table(&table, -1, iters, sizeof(uint32_t)));
      chck_hash_table_uint_algorithm(&table, chck_incremental_uint_hash);

      for (uint32_t i = 0; i < iters; ++i)
         chck_hash_table_set(&table, i, &i);

      for (uint32_t i = iters / 2, d = iters / 2; i < iters; ++i, --d) {
         assert(*(uint32_t*)chck_hash_table_get(&table, i) == i);
         assert(*(uint32_t*)chck_hash_table_get(&table, d) == d);
      }

      printf("[4] collisions: %u\n", chck_hash_table_collisions(&table));
      chck_hash_table_release(&table);
   }

   return EXIT_SUCCESS;
}
